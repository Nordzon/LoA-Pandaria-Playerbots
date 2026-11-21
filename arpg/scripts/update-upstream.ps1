<#
.SYNOPSIS
  Brings upstream (LoA-Pandaria-Playerbots) fixes into our fork safely:
  back up the databases -> tag the current state -> merge -> build -> test -> report.

.DESCRIPTION
  Steps (each can stop the run with a clear message):
    1. Refuses to run with uncommitted changes.
    2. Fetches the upstream branch and lists incoming commits.
    3. Flags incoming changes to files we patched (any file outside mod_arpg containing "// ARPG:")
       and to upstream SQL updates (one-way database changes).
    4. -DryRun stops here.
    5. Dumps every database with mysqldump into arpg\backups\<timestamp>\.
    6. Tags HEAD as pre-sync-<timestamp>.
    7. Merges (no rebase). On conflict: lists the files and stops.
    8. Builds, runs unit tests, restarts the world server (if restart-world.ps1 exists) and runs the GM tests.
    9. Prints the rollback commands and a PROGRESS log line.

  Settings are read from arpg\scripts\upstream.local.ps1 if present (gitignored), e.g.:
      $UpstreamBranch = 'main'
      $Databases = @('auth','characters','world','playerbots')
      $MySqlDump = 'C:\Program Files\MySQL\MySQL Server 8.0\bin\mysqldump.exe'
  and MySQL credentials from arpg\scripts\mysql.local.cnf (gitignored):
      [client]
      user=root
      password=yourpass
      port=3306

.EXAMPLE
  .\arpg\scripts\update-upstream.ps1 -DryRun          # just show what's new
  .\arpg\scripts\update-upstream.ps1                  # full sync
  .\arpg\scripts\update-upstream.ps1 -CherryPick abc1234   # take one commit (e.g. from beta)
#>
[CmdletBinding()]
param(
  [string]$Remote = 'upstream',
  [string]$Branch,
  [switch]$DryRun,
  [switch]$SkipBackup,
  [switch]$SkipTests,
  [string]$CherryPick
)
$ErrorActionPreference = 'Stop'
$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
Set-Location $RepoRoot

# Defaults, overridable in upstream.local.ps1
$UpstreamBranch = 'main'
$Databases = @('auth', 'characters', 'world', 'playerbots')
$MySqlDump = 'mysqldump'
$BuildConfig = 'RelWithDebInfo'
$localSettings = Join-Path $PSScriptRoot 'upstream.local.ps1'
if (Test-Path $localSettings) { . $localSettings }
if (-not $Branch) { $Branch = $UpstreamBranch }
$mysqlCnf = Join-Path $PSScriptRoot 'mysql.local.cnf'
$stamp = Get-Date -Format 'yyyyMMdd-HHmm'
$tag = "pre-sync-$stamp"

function Step($text) { Write-Host "`n== $text" -ForegroundColor Cyan }
function Fail($text) { Write-Host "STOP: $text" -ForegroundColor Red; exit 1 }
function Invoke-Git { & git.exe @args; if ($LASTEXITCODE -ne 0) { Fail "git $($args -join ' ') failed" } }

# 1. Clean tree
Step 'Checking working tree'
$dirty = git status --porcelain
if ($dirty) { Fail "uncommitted changes. Commit or stash first:`n$dirty" }

# 2. Fetch + incoming
Step "Fetching $Remote/$Branch"
if (-not (git remote | Where-Object { $_ -eq $Remote })) { Fail "no remote '$Remote'. Add it: git remote add $Remote <upstream repo url>" }
Invoke-Git fetch $Remote --tags

if ($CherryPick) {
  Step "Cherry-picking $CherryPick"
  git log -1 --stat $CherryPick
} else {
  $incoming = @(git log --oneline --no-merges "HEAD..$Remote/$Branch")
  if ($incoming.Count -eq 0) { Write-Host 'Already up to date.' -ForegroundColor Green; exit 0 }
  Write-Host "$($incoming.Count) new upstream commit(s):"
  $incoming | Select-Object -First 60 | ForEach-Object { Write-Host "  $_" }
  if ($incoming.Count -gt 60) { Write-Host "  ... and $($incoming.Count - 60) more" }
}

# 3. Risk flags
Step 'Checking what the incoming changes touch'
$range = if ($CherryPick) { "$CherryPick~1..$CherryPick" } else { "HEAD...$Remote/$Branch" }
$changed = @(git diff --name-only $range)
$ourPatched = @(git grep -l '// ARPG:' -- . ':(exclude)modules/mod_arpg' 2>$null)
$hookHits = @($changed | Where-Object { $ourPatched -contains $_ })
$sqlHits = @($changed | Where-Object { $_ -match '(^|/)sql/' -and $_ -notmatch 'modules/mod_arpg' })
$botHits = @($changed | Where-Object { $_ -like 'modules/mod_playerbots/*' })
Write-Host "Files changed upstream: $($changed.Count)  (playerbots: $($botHits.Count))"
if ($hookHits.Count) { Write-Host "Touches files we patched (possible conflicts):" -ForegroundColor Yellow; $hookHits | ForEach-Object { Write-Host "  $_" } }
else { Write-Host 'No overlap with our // ARPG: patches.' -ForegroundColor Green }
if ($sqlHits.Count) { Write-Host "Includes database updates (one-way, backup matters):" -ForegroundColor Yellow; $sqlHits | ForEach-Object { Write-Host "  $_" } }
if ($DryRun) { Write-Host "`nDry run: nothing changed." -ForegroundColor Green; exit 0 }

# 5. Backup
if (-not $SkipBackup) {
  Step 'Backing up databases'
  if (-not (Test-Path $mysqlCnf)) { Fail "missing $mysqlCnf (MySQL credentials). See the header of this script, or pass -SkipBackup." }
  $backupDir = Join-Path $RepoRoot "arpg\backups\$stamp"
  New-Item -ItemType Directory -Force -Path $backupDir | Out-Null
  foreach ($db in $Databases) {
    $out = Join-Path $backupDir "$db.sql"
    & $MySqlDump "--defaults-extra-file=$mysqlCnf" --single-transaction --routines --triggers --result-file="$out" $db
    if ($LASTEXITCODE -ne 0) { Fail "mysqldump failed for $db" }
    Write-Host "  $db -> $out"
  }
}

# 6. Tag
Step "Tagging current state as $tag"
Invoke-Git tag $tag

# 7. Merge / cherry-pick
if ($CherryPick) {
  Step "Applying $CherryPick"
  git cherry-pick -x $CherryPick
} else {
  Step "Merging $Remote/$Branch"
  git merge --no-ff "$Remote/$Branch" -m "Sync upstream $Branch ($($incoming.Count) commits)"
}
if ($LASTEXITCODE -ne 0) {
  $conflicts = @(git diff --name-only --diff-filter=U)
  Write-Host 'Conflicts in:' -ForegroundColor Red
  $conflicts | ForEach-Object { Write-Host "  $_" }
  Write-Host "`nResolve them (ask Claude: 'resolve the upstream merge conflicts, keep our // ARPG: hooks'), then:"
  Write-Host '  git add <files>; git commit        and re-run with -SkipBackup to build and test'
  Write-Host "Or give up:  git merge --abort  (or git cherry-pick --abort)"
  exit 1
}

# 8. Build + tests
Step "Building ($BuildConfig)"
cmake --build build --config $BuildConfig -- /m:2 /p:CL_MPCount=6 /nodeReuse:false   # capped: uncapped runs out of memory
if ($LASTEXITCODE -ne 0) { Fail "build failed. Fix it, or roll back: git reset --hard $tag" }

if (-not $SkipTests) {
  Step 'Unit tests'
  ctest --test-dir build -C $BuildConfig -R arpg --output-on-failure
  $unitFail = $LASTEXITCODE -ne 0

  $restart = Join-Path $PSScriptRoot 'restart-world.ps1'
  $gmFail = $false
  if (Test-Path $restart) {
    Step 'Restarting world server (applies upstream DB updates)'
    & $restart
    $gm = Join-Path $PSScriptRoot 'gm.ps1'
    $up = $false
    for ($i = 0; $i -lt 60 -and -not $up; $i++) {
      Start-Sleep -Seconds 5
      try { $reply = (& $gm '.server info' 2>&1 | Out-String); $up = ($reply -match 'uptime') } catch { $up = $false }
    }
    if (-not $up) { Fail "world server didn't answer on SOAP within 5 minutes. Check its console/log. Roll back: git reset --hard $tag" }
    Step 'GM tests'
    & (Join-Path $PSScriptRoot 'run-gmtests.ps1')
    $gmFail = $LASTEXITCODE -ne 0
  } else {
    Write-Host 'restart-world.ps1 not found: restart the world server yourself, then run arpg\scripts\run-gmtests.ps1' -ForegroundColor Yellow
  }
  if ($unitFail -or $gmFail) {
    Write-Host "`nTests failed. Ask Claude to look at the failures against the new upstream commits," -ForegroundColor Red
    Write-Host "or roll back (see below)." -ForegroundColor Red
  }
}

# 9. Report
Step 'Done'
$head = git rev-parse --short HEAD
Write-Host "Now at $head. Previous state: tag $tag"
Write-Host "`nRollback if needed:"
Write-Host "  git reset --hard $tag"
if (-not $SkipBackup) {
  Write-Host "  restore each DB: mysql --defaults-extra-file=arpg\scripts\mysql.local.cnf <db> < arpg\backups\$stamp\<db>.sql"
}
$what = if ($CherryPick) { "cherry-picked $CherryPick" } else { "merged $($incoming.Count) commits from $Remote/$Branch" }
Write-Host "`nPROGRESS log line:`n- $(Get-Date -Format 'yyyy-MM-dd') - Upstream sync: $what (tag $tag)."
