<#
.SYNOPSIS
  The everyday dev loop, unattended: stop servers -> build worldserver + unit tests -> ctest -> module SQL -> start servers
  -> hold the test character Arpgtest online (headless client) -> run all GM tests -> leave the world running.

.DESCRIPTION
  Writes a one-line result per step to D:\WoWMoPProject\ServerData\pipeline\rebuild.log and the GM-test output to
  rebuild-gmtests.log. Exit code 0 = everything passed. Keeps the PC awake while it runs. Never runs two builds at once:
  it refuses to start while MSBuild is running.

.EXAMPLE
  Start-Process pwsh -ArgumentList '-NoProfile','-ExecutionPolicy','Bypass','-File','arpg\scripts\rebuild-and-test.ps1' -WindowStyle Minimized
  pwsh -File arpg\scripts\rebuild-and-test.ps1 -Filter scaling      # only GM tests whose name contains "scaling"
#>
[CmdletBinding()]
param(
  [string]$Filter = '',
  [switch]$TestsOnly,          # skip stop/build/restart: servers must be running; just hold Arpgtest and run GM tests
  [switch]$Force,              # restart even if real players are online (they get disconnected)
  [string]$Work = 'D:\WoWMoPProject\ServerData\pipeline',
  [string]$Data = 'D:\WoWMoPProject\ServerData\data'
)
$ErrorActionPreference = 'Stop'
$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$Scripts = $PSScriptRoot
$Headless = Join-Path $RepoRoot 'contrib\headless-client'
$Log = Join-Path $Work 'rebuild.log'
New-Item -ItemType Directory -Force $Work | Out-Null
Set-Location $RepoRoot

Add-Type -Namespace ArpgRb -Name Power -MemberDefinition '[DllImport("kernel32.dll")] public static extern uint SetThreadExecutionState(uint f);'
[void][ArpgRb.Power]::SetThreadExecutionState([uint32]'0x80000001')

function Say($m) { $l = "[$(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')] $m"; Add-Content $Log $l; Write-Host $l }
function Done($code, $m) { Say $m; exit $code }

Say "==== rebuild-and-test start (filter '$Filter', tests only: $TestsOnly)"
$unitOk = $true
if (-not $TestsOnly) {
if (Get-Process MSBuild -ErrorAction SilentlyContinue) { Done 10 'FAIL another build is running (MSBuild); try later' }
# Never kick a playing person by accident (owner 2026-11-27): a real account other than the test account online -> stop here.
$mysqlExe = 'C:\Program Files\MySQL\MySQL Server 8.0\bin\mysql.exe'
$online = (& $mysqlExe "--defaults-extra-file=$(Join-Path $Scripts 'mysql.local.cnf')" -N -e "SELECT GROUP_CONCAT(c.name) FROM characters.characters c JOIN auth.account a ON a.id = c.account WHERE c.online = 1 AND a.username NOT LIKE 'RNDBOT%' AND a.username NOT IN ('ARPGTEST', 'ARPGTEST2')" 2>$null | Out-String).Trim()
if ($online -and $online -ne 'NULL' -and -not $Force) { Done 11 "FAIL players online ($online): log them out first or pass -Force" }
# Backup of accounts + characters before anything is stopped or changed (owner 2026-11-27).
& powershell -ExecutionPolicy Bypass -File (Join-Path $Scripts 'backup-db.ps1') -Reason rebuild *>> $Log
if ($LASTEXITCODE -ne 0) { Done 12 'FAIL database backup (see rebuild.log); nothing was stopped' }
Say 'databases backed up'

& powershell -ExecutionPolicy Bypass -File (Join-Path $Scripts 'restart-world.ps1') -Stop *>> $Log
Say 'servers stopped'
# Client addons: keep the client's copy in step with arpg/addons (it is a copy, not a link; never while the client runs).
if (-not (Get-Process | Where-Object { $_.ProcessName -match '^wow' })) { & powershell -ExecutionPolicy Bypass -File (Join-Path $Scripts 'install-addons.ps1') *>> $Log }

$env:BOOST_ROOT = 'C:/local/boost_1_87_0'
cmake -S . -B build *>> (Join-Path $Work 'rebuild-build.log')
cmake --build build --config RelWithDebInfo --target worldserver arpg_unittests -- /m:2 /p:CL_MPCount=6 /nodeReuse:false *> (Join-Path $Work 'rebuild-build.log')
if ($LASTEXITCODE -ne 0) {
  $errs = (Select-String -Path (Join-Path $Work 'rebuild-build.log') -Pattern ' error ' | Select-Object -First 5 | ForEach-Object { $_.Line -replace '\[.*vcxproj\]', '' }) -join ' || '
  & powershell -ExecutionPolicy Bypass -File (Join-Path $Scripts 'restart-world.ps1') -StartTimeout 3600 *>> $Log   # old binary back up
  Done 1 "FAIL build: $errs"
}
Say 'build OK'

ctest --test-dir build -C RelWithDebInfo -R arpg --output-on-failure *>> $Log
if ($LASTEXITCODE -ne 0) { Say 'FAIL unit tests (see rebuild.log)' } else { Say 'unit tests OK' }
$unitOk = ($LASTEXITCODE -eq 0)

& powershell -ExecutionPolicy Bypass -File (Join-Path $Scripts 'apply-sql.ps1') *>> $Log
& powershell -ExecutionPolicy Bypass -File (Join-Path $Scripts 'write-confs.ps1') -DataDir ($Data -replace '\\', '/') -Bots on *>> $Log
& powershell -ExecutionPolicy Bypass -File (Join-Path $Scripts 'restart-world.ps1') -StartTimeout 3600 *>> $Log
if ($LASTEXITCODE -ne 0) { Done 2 "FAIL server start (code $LASTEXITCODE; see D:\WoWMoPProject\ServerData\logs\Server.log)" }
Say 'servers up, SOAP OK'
}

# Hold Arpgtest online for the GM tests.
$mysql = 'C:\Program Files\MySQL\MySQL Server 8.0\bin\mysql.exe'
$guid = (& $mysql "--defaults-extra-file=$(Join-Path $Scripts 'mysql.local.cnf')" -N -e "SELECT guid FROM characters.characters WHERE name = 'Arpgtest'" 2>$null | Out-String).Trim()
if (-not $guid) { Done 3 'FAIL test character Arpgtest missing (run overnight.ps1 gmtests stage once)' }
$env:MOP_NAV_DATA = $Data
$hold = Start-Process -FilePath 'py' -ArgumentList "headless.py --cmd play --char $guid --hold 1200" -WorkingDirectory $Headless `
          -NoNewWindow -PassThru -RedirectStandardOutput (Join-Path $Work 'rebuild-hold.out') -RedirectStandardError (Join-Path $Work 'rebuild-hold.err')
# Wait for the real login (with 300 bots loading it can take a while); a fixed sleep let tests run before Arpgtest was online.
$deadline = (Get-Date).AddSeconds(120)
while ((Get-Date) -lt $deadline -and -not (Select-String -Path (Join-Path $Work 'rebuild-hold.out') -Pattern 'LOGIN-OK' -Quiet -ErrorAction SilentlyContinue)) {
  if ($hold.HasExited) { break }
  Start-Sleep 2
}
if (-not (Select-String -Path (Join-Path $Work 'rebuild-hold.out') -Pattern 'LOGIN-OK' -Quiet -ErrorAction SilentlyContinue)) {
  if (-not $hold.HasExited) { $hold.Kill() }
  Done 5 'FAIL headless client could not log Arpgtest in within 120 s (see rebuild-hold.out/.err)'
}
Start-Sleep 3
Say 'Arpgtest online'

# m8: a second real test character (Arpgtesttwo, account ARPGTEST2 from headless.local.json account2/password2) for the
# multiplayer tests (55-friends). Optional: without it only that file fails.
$hold2 = $null
$hcfg = Get-Content (Join-Path $Headless 'headless.local.json') -Raw | ConvertFrom-Json
$guid2 = (& $mysql "--defaults-extra-file=$(Join-Path $Scripts 'mysql.local.cnf')" -N -e "SELECT guid FROM characters.characters WHERE name = 'Arpgtesttwo'" 2>$null | Out-String).Trim()
if ($hcfg.account2 -and $guid2) {
  $env:MOP_ACCOUNT = $hcfg.account2; $env:MOP_PASSWORD = $hcfg.password2
  $hold2 = Start-Process -FilePath 'py' -ArgumentList "headless.py --cmd play --char $guid2 --hold 1200" -WorkingDirectory $Headless `
            -NoNewWindow -PassThru -RedirectStandardOutput (Join-Path $Work 'rebuild-hold2.out') -RedirectStandardError (Join-Path $Work 'rebuild-hold2.err')
  Remove-Item Env:MOP_ACCOUNT, Env:MOP_PASSWORD -ErrorAction SilentlyContinue
  $deadline = (Get-Date).AddSeconds(120)
  while ((Get-Date) -lt $deadline -and -not $hold2.HasExited -and -not (Select-String -Path (Join-Path $Work 'rebuild-hold2.out') -Pattern 'LOGIN-OK' -Quiet -ErrorAction SilentlyContinue)) { Start-Sleep 2 }
  if (Select-String -Path (Join-Path $Work 'rebuild-hold2.out') -Pattern 'LOGIN-OK' -Quiet -ErrorAction SilentlyContinue) {
    # Parked away from every test spot: next to Arpgtest it would scale monsters first (its events, not Arpgtest's).
    & (Join-Path $Scripts 'gm.ps1') '.arpg test setup Arpgtesttwo 20 park-stormwind' *>> $Log
    Say 'Arpgtesttwo online (parked in Stormwind)'
  }
  else { Say 'WARN Arpgtesttwo could not log in (55-friends will fail; see rebuild-hold2.out/.err)' }
}

$args2 = @('-ExecutionPolicy', 'Bypass', '-File', (Join-Path $Scripts 'run-gmtests.ps1'))
if ($Filter) { $args2 += @('-Filter', $Filter) }
& powershell @args2 *> (Join-Path $Work 'rebuild-gmtests.log')
$failed = $LASTEXITCODE
# The live server must not keep the tests' ".arpg config set" values (2026-11-28: Champion camps stayed off after a run).
$reset = (& powershell -ExecutionPolicy Bypass -File (Join-Path $Scripts 'gm.ps1') '.arpg config reset' 2>&1 | Out-String).Trim()
Say "config overrides cleared: $reset"
if (-not $hold.HasExited) { $hold.Kill() }
if ($hold2 -and -not $hold2.HasExited) { $hold2.Kill() }
$summary = (Select-String -Path (Join-Path $Work 'rebuild-gmtests.log') -Pattern '^(PASS|FAIL)\s' | ForEach-Object { $_.Line.Trim() }) -join '; '
if ($failed -ne 0 -or -not $unitOk) { Done 4 "FAIL gmtests ($failed file(s)) unit=$unitOk : $summary" }
Done 0 "ALL OK: $summary (world left running, bots on)"
