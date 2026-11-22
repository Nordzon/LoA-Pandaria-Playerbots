<#
.SYNOPSIS
  Stops and (re)starts authserver + worldserver, then waits until SOAP answers ".server info".

.DESCRIPTION
  - Checks that the OpenSSL 3.0 / libmysql DLLs sit next to the exes (System32 holds OpenSSL 3.5.2 from
    another app, and Windows loads System32 before PATH) and copies them if missing.
  - Stops a running worldserver cleanly over SOAP (".server shutdown 1"), force-stops it after -StopTimeout.
  - Starts authserver if it isn't running, then worldserver, each in its own minimized console.
  - Waits up to -StartTimeout seconds for SOAP. Exit code 0 = server up and SOAP OK.

.EXAMPLE
  powershell -ExecutionPolicy Bypass -File arpg\scripts\restart-world.ps1
  powershell -ExecutionPolicy Bypass -File arpg\scripts\restart-world.ps1 -Stop        # stop both servers
#>
[CmdletBinding()]
param(
  [string]$BinDir,
  [switch]$Stop,
  [int]$StartTimeout = 900,
  [int]$StopTimeout = 90
)
$ErrorActionPreference = 'Stop'
$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
if (-not $BinDir) { $BinDir = Join-Path $RepoRoot 'build\bin\RelWithDebInfo' }
$gm = Join-Path $PSScriptRoot 'gm.ps1'

function Write-Step($m) { Write-Host "[$(Get-Date -Format HH:mm:ss)] $m" }

function Test-Soap {
  try { $r = (& $gm '.server info' 2>&1 | Out-String); return ($r -match '(?i)uptime') } catch { return $false }
}

function Stop-World {
  $ws = Get-Process worldserver -ErrorAction SilentlyContinue
  if ($ws) {
    Write-Step 'Stopping worldserver (SOAP .server shutdown 1)'
    try { & $gm '.server shutdown 1' | Out-Null } catch { }
    $deadline = (Get-Date).AddSeconds($StopTimeout)
    while ((Get-Process worldserver -ErrorAction SilentlyContinue) -and (Get-Date) -lt $deadline) { Start-Sleep 2 }
    if (Get-Process worldserver -ErrorAction SilentlyContinue) {
      Write-Step 'worldserver still running: force-stopping'
      Get-Process worldserver -ErrorAction SilentlyContinue | Stop-Process -Force -Confirm:$false
    }
  }
}

if ($Stop) {
  Stop-World
  Get-Process authserver -ErrorAction SilentlyContinue | Stop-Process -Force -Confirm:$false
  Write-Step 'Servers stopped.'; exit 0
}

# --- runtime DLLs next to the exes ---
$dlls = @{
  'libcrypto-3-x64.dll' = 'C:\Program Files\OpenSSL-Win64-3.0\bin\libcrypto-3-x64.dll'
  'libssl-3-x64.dll'    = 'C:\Program Files\OpenSSL-Win64-3.0\bin\libssl-3-x64.dll'
  'legacy.dll'          = 'C:\Program Files\OpenSSL-Win64-3.0\bin\legacy.dll'
  'libmysql.dll'        = 'C:\Program Files\MySQL\MySQL Server 8.0\lib\libmysql.dll'
}
foreach ($name in $dlls.Keys) {
  $dst = Join-Path $BinDir $name
  if (-not (Test-Path $dst) -or (Get-Item $dst).VersionInfo.ProductVersion -ne (Get-Item $dlls[$name]).VersionInfo.ProductVersion) {
    Copy-Item $dlls[$name] $dst -Force; Write-Step "Copied $name"
  }
}
foreach ($f in 'authserver.exe', 'worldserver.exe', 'authserver.conf', 'worldserver.conf') {
  if (-not (Test-Path (Join-Path $BinDir $f))) { throw "Missing $f in $BinDir (build, then write-confs.ps1)" }
}

# A click into a server console would pause the server (QuickEdit selection): switch it off for these two windows only.
function Disable-QuickEdit([int]$ProcessId) {
  Start-Sleep -Milliseconds 800   # let the console window exist
  Start-Process powershell -WindowStyle Hidden -ArgumentList '-NoProfile', '-ExecutionPolicy', 'Bypass', '-File',
    (Join-Path $PSScriptRoot 'no-quickedit.ps1'), '-ProcessId', $ProcessId | Out-Null
}

Stop-World
if (-not (Get-Process authserver -ErrorAction SilentlyContinue)) {
  Write-Step 'Starting authserver'
  $auth = Start-Process -FilePath (Join-Path $BinDir 'authserver.exe') -WorkingDirectory $BinDir -WindowStyle Minimized -PassThru
  Disable-QuickEdit $auth.Id
}
Write-Step 'Starting worldserver'
$proc = Start-Process -FilePath (Join-Path $BinDir 'worldserver.exe') -WorkingDirectory $BinDir -WindowStyle Minimized -PassThru
Disable-QuickEdit $proc.Id

$deadline = (Get-Date).AddSeconds($StartTimeout)
while ((Get-Date) -lt $deadline) {
  if ($proc.HasExited) { Write-Step "worldserver exited during startup (code $($proc.ExitCode)); see Server.log in LogsDir"; exit 2 }
  if (Test-Soap) { Write-Step "worldserver up, SOAP OK (pid $($proc.Id))"; exit 0 }
  Start-Sleep 5
}
Write-Step "SOAP did not answer within $StartTimeout s"; exit 3
