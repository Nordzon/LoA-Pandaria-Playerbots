<#
.SYNOPSIS
  Unattended m0 pipeline: build -> confs -> junctions -> map/vmap extraction -> server smoke test (headless)
  -> mmaps -> final headless test (navmesh walk) -> report. Resumable: each stage writes a .done marker.

.DESCRIPTION
  Runs on its own (no Claude needed). Keeps the PC awake while it runs (SetThreadExecutionState; no
  power-setting change). State lives in $Work (default D:\WoWMoPProject\ServerData\pipeline):
    status.json        current stage / state / message (read by check-ins)
    overnight.log      what happened, with timestamps
    <stage>.log        full output of each stage
    <stage>.done       marker; delete it to redo that stage
    report.md          morning summary
  Critical stages (build .. vmaps) stop the run on failure. Test failures are recorded and the run goes on.

  Client folder junctions: the extractors write to the folder they run in, so dbc/maps/vmaps/mmaps/
  cameras/Buildings inside the client folder are junctions to $Data. Output lands in the server's DataDir,
  and navpath (headless client) reads the very same mmaps as the worldserver.

.EXAMPLE
  # start detached (survives closing the terminal). Needs PowerShell 7 (pwsh): UTF-8 log redirection.
  Start-Process pwsh -ArgumentList '-ExecutionPolicy Bypass -File arpg\scripts\overnight.ps1' -WindowStyle Minimized
#>
[CmdletBinding()]
param(
  [string]$Client = 'D:\WoWMoPProject\MOP-5.4.8.18414-enUS-Repack',
  [string]$Data = 'D:\WoWMoPProject\ServerData\data',
  [string]$Work = 'D:\WoWMoPProject\ServerData\pipeline',
  [int]$MmapThreads = 16,
  [string]$CharName = 'Headlessone',
  [ValidateSet('on', 'off')][string]$Bots = 'on',   # for the final start (after mmaps); smoke always runs bots off
  [int]$BotCount = 500
)
$ErrorActionPreference = 'Stop'
$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$Bin = Join-Path $RepoRoot 'build\bin\RelWithDebInfo'
$Scripts = $PSScriptRoot
$Headless = Join-Path $RepoRoot 'contrib\headless-client'
New-Item -ItemType Directory -Force $Work, $Data | Out-Null
$LogFile = Join-Path $Work 'overnight.log'
$Results = [ordered]@{}

# --- keep the machine awake while this process lives ---
Add-Type -Namespace Arpg -Name Power -MemberDefinition '[DllImport("kernel32.dll")] public static extern uint SetThreadExecutionState(uint f);'
[void][Arpg.Power]::SetThreadExecutionState([uint32]'0x80000001')   # ES_CONTINUOUS | ES_SYSTEM_REQUIRED

function Log($m) { $line = "[$(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')] $m"; Add-Content $LogFile $line; Write-Host $line }
function Set-Status($stage, $state, $msg) {
  [ordered]@{ stage = $stage; state = $state; message = $msg; updated = (Get-Date -Format s); pid = $PID } |
    ConvertTo-Json | Set-Content (Join-Path $Work 'status.json')
}
function Done($s) { Test-Path (Join-Path $Work "$s.done") }
function Mark($s) { Set-Content (Join-Path $Work "$s.done") (Get-Date -Format s) }

# Runs a native program, appending all output to <stage>.log. Returns the exit code.
function Invoke-Logged($stage, $exe, [string[]]$argList, $cwd = $RepoRoot) {
  $log = Join-Path $Work "$stage.log"
  Add-Content $log "`n=== $(Get-Date -Format s)  $exe $($argList -join ' ')  (cwd $cwd)"
  Push-Location $cwd
  try {
    $old = $ErrorActionPreference; $ErrorActionPreference = 'Continue'
    '' | & $exe @argList *>> $log   # '' on stdin: tools that "Press any key" after an error don't hang
    $code = $LASTEXITCODE
    $ErrorActionPreference = $old
  } finally { Pop-Location }
  return $code
}

# Runs one stage. $critical = stop the whole run if it fails.
function Stage($name, [bool]$critical, [scriptblock]$body) {
  if (Done $name) { Log "skip  $name (done)"; $Results[$name] = 'done earlier'; return }
  Log "start $name"; Set-Status $name 'running' ''
  $t0 = Get-Date
  try {
    $msg = & $body
    $mins = [math]::Round(((Get-Date) - $t0).TotalMinutes, 1)
    Mark $name; $Results[$name] = "OK ($mins min) $msg"; Log "ok    $name ($mins min) $msg"
  } catch {
    $err = $_.Exception.Message
    $Results[$name] = "FAILED: $err"; Log "FAIL  $name : $err"
    if ($critical) { Set-Status $name 'failed' $err; Write-Report; exit 1 }
  }
}

function Headless([string[]]$argList, $stage, [int]$timeoutSec = 600) {
  $env:MOP_NAV_DATA = $Data
  $env:HEADLESS_LOGOUT = '1'
  $log = Join-Path $Work "$stage.log"
  Add-Content $log "`n=== $(Get-Date -Format s)  headless.py $($argList -join ' ')"
  # Start-Process joins arguments with spaces and does not quote them: quote any argument that contains whitespace.
  $argStr = (@('headless.py') + $argList | ForEach-Object { if ($_ -match '\s') { '"' + ($_ -replace '"', '\"') + '"' } else { $_ } }) -join ' '
  $p = Start-Process -FilePath 'py' -ArgumentList $argStr -WorkingDirectory $Headless `
        -NoNewWindow -PassThru -RedirectStandardOutput "$log.out" -RedirectStandardError "$log.err"
  if (-not $p.WaitForExit($timeoutSec * 1000)) { $p.Kill(); Add-Content $log 'TIMEOUT (killed)' }
  Get-Content "$log.out", "$log.err" -ErrorAction SilentlyContinue | Add-Content $log
  $out = (Get-Content "$log.out" -Raw -ErrorAction SilentlyContinue) + (Get-Content "$log.err" -Raw -ErrorAction SilentlyContinue)
  Remove-Item "$log.out", "$log.err" -ErrorAction SilentlyContinue
  return [pscustomobject]@{ Code = $p.ExitCode; Out = $out }
}

function Write-Report {
  $st = Get-Content (Join-Path $Work 'status.json') -Raw -ErrorAction SilentlyContinue
  $lines = @("# Overnight m0 run - $(Get-Date -Format 'yyyy-MM-dd HH:mm')", '', '| Stage | Result |', '| --- | --- |')
  foreach ($k in $Results.Keys) { $lines += "| $k | $($Results[$k] -replace '\|', '/') |" }
  $lines += @('', "Data: ``$Data``  Logs: ``$Work``", '', 'Last status:', '```', $st, '```')
  Set-Content (Join-Path $Work 'report.md') $lines
}

Log "==== overnight pipeline start (pid $PID)"

# 1. build (wait for any running build first; incremental, so cheap if it already finished)
Stage 'build' $true {
  while (Get-Process MSBuild -ErrorAction SilentlyContinue) { Set-Status 'build' 'waiting' 'another build is running'; Start-Sleep 30 }
  $code = Invoke-Logged 'build' 'cmake' @('--build', 'build', '--config', 'RelWithDebInfo', '--', '/m:2', '/p:CL_MPCount=6', '/nodeReuse:false')
  if ($code -ne 0) { throw "cmake --build exit $code (see build.log)" }
  if (-not (Test-Path (Join-Path $Bin 'worldserver.exe'))) { throw 'worldserver.exe missing after build' }
  'worldserver.exe built'
}

# 2. confs + DLLs
Stage 'confs' $true {
  $code = Invoke-Logged 'confs' 'powershell' @('-ExecutionPolicy', 'Bypass', '-File', (Join-Path $Scripts 'write-confs.ps1'), '-DataDir', ($Data -replace '\\', '/'))
  if ($code -ne 0) { throw "write-confs exit $code" }
  foreach ($d in 'C:\Program Files\OpenSSL-Win64-3.0\bin\libcrypto-3-x64.dll', 'C:\Program Files\OpenSSL-Win64-3.0\bin\libssl-3-x64.dll',
                 'C:\Program Files\OpenSSL-Win64-3.0\bin\legacy.dll', 'C:\Program Files\MySQL\MySQL Server 8.0\lib\libmysql.dll') {
    Copy-Item $d $Bin -Force
  }
  'confs written, OpenSSL 3.0 + libmysql DLLs copied'
}

# 3. junctions client\<x> -> $Data\<x>
Stage 'junctions' $true {
  foreach ($d in 'dbc', 'maps', 'vmaps', 'mmaps', 'cameras', 'Buildings') {
    $target = Join-Path $Data $d; $link = Join-Path $Client $d
    New-Item -ItemType Directory -Force $target | Out-Null
    $item = Get-Item $link -Force -ErrorAction SilentlyContinue
    if ($item) {
      if ($item.LinkType -eq 'Junction' -and ($item.Target -replace '\\$', '') -eq $target) { continue }
      if ($item.LinkType) { throw "$link is a $($item.LinkType) to $($item.Target), expected $target" }
      if (Get-ChildItem $link -Force | Select-Object -First 1) { throw "$link is a real folder with files; move it away first" }
      Remove-Item $link -Force
    }
    New-Item -ItemType Junction -Path $link -Target $target | Out-Null
  }
  '6 junctions in client folder'
}

# 4. maps + dbc + cameras
Stage 'mapextract' $true {
  $code = Invoke-Logged 'mapextract' (Join-Path $Bin 'mapextractor.exe') @() $Client
  $nDbc = @(Get-ChildItem (Join-Path $Data 'dbc') -Recurse -Filter *.db? -ErrorAction SilentlyContinue).Count
  $nMap = @(Get-ChildItem (Join-Path $Data 'maps') -Filter *.map -ErrorAction SilentlyContinue).Count
  if ($code -ne 0 -or $nDbc -lt 100 -or $nMap -lt 100) { throw "mapextractor exit $code, dbc files $nDbc, map files $nMap" }
  "dbc/db2 $nDbc, maps $nMap"
}

# 5. vmaps
Stage 'vmaps' $true {
  $code = Invoke-Logged 'vmaps' (Join-Path $Bin 'vmap4extractor.exe') @() $Client
  if ($code -ne 0) { throw "vmap4extractor exit $code" }
  $code = Invoke-Logged 'vmaps' (Join-Path $Bin 'vmap4assembler.exe') @('Buildings', 'vmaps') $Client
  $n = @(Get-ChildItem (Join-Path $Data 'vmaps') -Filter *.vmtree -ErrorAction SilentlyContinue).Count
  if ($code -ne 0 -or $n -lt 10) { throw "vmap4assembler exit $code, vmtree files $n" }
  "vmtree files $n"
}

# 6. smoke test before mmaps: servers up, SOAP, GM tests, headless auth + create + login
Stage 'smoke' $false {
  $code = Invoke-Logged 'smoke' 'powershell' @('-ExecutionPolicy', 'Bypass', '-File', (Join-Path $Scripts 'restart-world.ps1'))
  if ($code -ne 0) { throw "restart-world exit $code (see smoke.log and $Data\..\logs)" }
  $info = (& (Join-Path $Scripts 'gm.ps1') '.server info' 2>&1 | Out-String).Trim()
  Add-Content (Join-Path $Work 'smoke.log') "SOAP .server info:`n$info"
  [void](Invoke-Logged 'smoke' 'powershell' @('-ExecutionPolicy', 'Bypass', '-File', (Join-Path $Scripts 'run-gmtests.ps1'), '-Filter', 'smoke'))
  $r = Headless @('--cmd', 'test') 'smoke' 120
  if ($r.Code -ne 0) { throw "headless auth test exit $($r.Code)" }
  $r = Headless @('--cmd', 'create', '--create-name', $CharName, '--create-race', '1', '--create-class', '1') 'smoke' 120
  if ($r.Code -ne 0 -and $r.Out -notmatch 'NAME_IN_USE|CHAR_CREATE_NAME_IN_USE') { throw "headless create exit $($r.Code)" }
  $r = Headless @('--cmd', 'play', '--hold', '20', '--step', 'say hello from the headless client', '--step', 'quest-log', '--step', 'inventory') 'smoke' 300
  if ($r.Out -notmatch 'LOGIN-OK') { throw "headless play: no LOGIN-OK (exit $($r.Code))" }
  "SOAP OK, headless auth/create/login OK ($CharName)"
}
& (Join-Path $Scripts 'restart-world.ps1') -Stop *>> (Join-Path $Work 'smoke.log')

# 7. mmaps (hours)
Stage 'mmaps' $true {
  $code = Invoke-Logged 'mmaps' (Join-Path $Bin 'mmaps_generator.exe') @('--threads', "$MmapThreads", '--silent') $Client
  $n = @(Get-ChildItem (Join-Path $Data 'mmaps') -Filter *.mmtile -ErrorAction SilentlyContinue).Count
  if ($code -ne 0 -or $n -lt 1000) { throw "mmaps_generator exit $code, mmtile files $n" }
  "mmtile files $n"
}

# 7b. mod_arpg: build worldserver with the module (incremental), unit tests, module SQL
Stage 'arpg' $false {
  $code = Invoke-Logged 'arpg' 'cmake' @('--build', 'build', '--config', 'RelWithDebInfo', '--target', 'worldserver', 'arpg_unittests', '--', '/m:2', '/p:CL_MPCount=6', '/nodeReuse:false')
  if ($code -ne 0) { throw "build worldserver+arpg_unittests exit $code (see arpg.log)" }
  $code = Invoke-Logged 'arpg' 'ctest' @('--test-dir', 'build', '-C', 'RelWithDebInfo', '-R', 'arpg', '--output-on-failure')
  if ($code -ne 0) { throw "unit tests failed (see arpg.log)" }
  $code = Invoke-Logged 'arpg' 'powershell' @('-ExecutionPolicy', 'Bypass', '-File', (Join-Path $Scripts 'apply-sql.ps1'))
  if ($code -ne 0) { throw "apply-sql: $code file(s) failed" }
  'worldserver built with mod_arpg, unit tests pass, module SQL applied'
}

# 8. final: servers up with mmaps, navmesh check, headless walk in Northshire, GM tests
Stage 'final' $false {
  # Bots on now that mmaps exist (owner wants a populated world). First start creates bot accounts: slow.
  $code = Invoke-Logged 'final' 'powershell' @('-ExecutionPolicy', 'Bypass', '-File', (Join-Path $Scripts 'write-confs.ps1'), '-DataDir', ($Data -replace '\\', '/'), '-Bots', $Bots, '-BotCount', "$BotCount")
  if ($code -ne 0) { throw "write-confs (bots $Bots) exit $code" }
  $code = Invoke-Logged 'final' 'powershell' @('-ExecutionPolicy', 'Bypass', '-File', (Join-Path $Scripts 'restart-world.ps1'), '-StartTimeout', '5400')
  if ($code -ne 0) { throw "restart-world exit $code" }
  $nav = & (Join-Path $Headless 'navpath\build\navpath.exe') --data $Data --map 0 --from '-8914,-133,81' --to '-8775,-185,84' 2>&1 | Out-String
  Add-Content (Join-Path $Work 'final.log') "navpath: $nav"
  if ($nav -notmatch '"ok"\s*:\s*true') { throw 'navpath found no path in Northshire' }
  [void](Invoke-Logged 'final' 'powershell' @('-ExecutionPolicy', 'Bypass', '-File', (Join-Path $Scripts 'run-gmtests.ps1'), '-Filter', 'smoke'))
  $r = Headless @('--cmd', 'play', '--max-session', '400', '--step', 'goto-path -8914,-133,81', '--step', 'goto-path -8775,-185,84', '--step', 'objects') 'final' 480
  if ($r.Out -match 'STUCK') { throw "headless walk STUCK (exit $($r.Code))" }
  if ($r.Out -notmatch 'OK goto-path') { throw "headless walk: no 'OK goto-path' (exit $($r.Code))" }
  'navpath OK, headless walked a navmesh path in Northshire; servers left running'
}

# 8b. GM tests over SOAP (all arpg\tests\*.gmtest) with the test character Arpgtest held online by the headless client
Stage 'gmtests' $false {
  $mysql = 'C:\Program Files\MySQL\MySQL Server 8.0\bin\mysql.exe'; $cnf = Join-Path $Scripts 'mysql.local.cnf'
  $r = Headless @('--cmd', 'create', '--create-name', 'Arpgtest', '--create-race', '1', '--create-class', '1') 'gmtests' 120
  $guid = (& $mysql "--defaults-extra-file=$cnf" -N -e "SELECT guid FROM characters.characters WHERE name = 'Arpgtest'" 2>$null | Out-String).Trim()
  if (-not $guid) { throw "could not create Arpgtest (exit $($r.Code))" }
  $env:MOP_NAV_DATA = $Data
  $hold = Start-Process -FilePath 'py' -ArgumentList "headless.py --cmd play --char $guid --hold 420" -WorkingDirectory $Headless `
            -NoNewWindow -PassThru -RedirectStandardOutput (Join-Path $Work 'gmtests-hold.out') -RedirectStandardError (Join-Path $Work 'gmtests-hold.err')
  Start-Sleep 25
  $code = Invoke-Logged 'gmtests' 'powershell' @('-ExecutionPolicy', 'Bypass', '-File', (Join-Path $Scripts 'run-gmtests.ps1'))
  if (-not $hold.HasExited) { $hold.Kill() }
  $summary = (Select-String -Path (Join-Path $Work 'gmtests.log') -Pattern '^(PASS|FAIL)\s' | Select-Object -Last 10 | ForEach-Object { $_.Line.Trim() }) -join '; '
  if ($code -ne 0) { throw "$code gmtest file(s) failed: $summary" }
  "all passed: $summary"
}

# 9. bots: give them time to log in, then count characters online (bots are real characters)
Stage 'bots' $false {
  if ($Bots -ne 'on') { return 'bots off (skipped)' }
  $mysql = 'C:\Program Files\MySQL\MySQL Server 8.0\bin\mysql.exe'; $cnf = Join-Path $Scripts 'mysql.local.cnf'
  $n = 0
  for ($i = 0; $i -lt 40; $i++) {   # up to ~40 min
    Start-Sleep 60
    $n = [int](& $mysql "--defaults-extra-file=$cnf" -N -e 'SELECT COUNT(*) FROM characters.characters WHERE online = 1' 2>$null)
    Set-Status 'bots' 'running' "$n characters online"
    if ($n -ge [math]::Floor($BotCount * 0.8)) { break }
  }
  $zones = (& $mysql "--defaults-extra-file=$cnf" -N -e 'SELECT COUNT(DISTINCT zone) FROM characters.characters WHERE online = 1' 2>$null)
  if ($n -lt 10) { throw "only $n characters online after 40 min" }
  "$n characters online in $zones zones"
}

Set-Status 'finished' 'finished' 'see report.md'
Write-Report
Log '==== overnight pipeline finished'
