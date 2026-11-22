<#
.SYNOPSIS
  One-click play: closes memory-hungry apps, makes sure MySQL runs, starts authserver + worldserver,
  then launches the WoW client. -Stop shuts the servers down. Used by the desktop shortcuts.

.DESCRIPTION
  - Won't touch the servers while the overnight pipeline is running.
  - MySQL80 is a manual-start service; if it's stopped, a UAC prompt asks to start it.
  - Bots: -Bots on|off rewrites playerbots.conf (and restarts the world) only if it differs from the current conf.
  - Already running and SOAP OK with the same bot setting? It just launches the client.
  Local overrides (gitignored) in arpg\scripts\stack.local.ps1, e.g.:
      $AppsToClose = @('Discord', 'Steam', 'steamwebhelper')
      $ClientExe   = 'D:\WoWMoPProject\MOP-5.4.8.18414-enUS-Repack\Wow-64.exe'

.EXAMPLE
  pwsh -ExecutionPolicy Bypass -File arpg\scripts\start-stack.ps1
  pwsh -ExecutionPolicy Bypass -File arpg\scripts\start-stack.ps1 -Bots off -NoClient
  pwsh -ExecutionPolicy Bypass -File arpg\scripts\start-stack.ps1 -Stop
#>
[CmdletBinding()]
param(
  [ValidateSet('on', 'off')][string]$Bots = 'on',
  [switch]$NoClient,
  [switch]$NoCloseApps,
  [switch]$Stop
)
$ErrorActionPreference = 'Stop'
$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$Bin = Join-Path $RepoRoot 'build\bin\RelWithDebInfo'
$Data = 'D:/WoWMoPProject/ServerData/data'
$Pipeline = 'D:\WoWMoPProject\ServerData\pipeline\status.json'

# Defaults; override in stack.local.ps1
$AppsToClose = @('Discord', 'Zoom', 'TeamViewer', 'dbeaver', 'PowerToys*', 'Viber', 'olk', 'AWS VPN Client',
                 'steam', 'steamwebhelper', 'CultivationLegend', 'Endless Hunter')
# Wow-64-all.exe: unlocked 5.4.8 x64 exe (wowmodding.net file 367; audited 2026-11-22: 18 bytes changed, no new code): classic
# login + custom MPQs (our client patch). Fallback: Wow-64.exe (login only). NOT Wow-SF*.exe: Stormforge builds force Battle.net login.
$ClientExe = 'D:\WoWMoPProject\MOP-5.4.8.18414-enUS-Repack\Wow-64-all.exe'
$local = Join-Path $PSScriptRoot 'stack.local.ps1'
if (Test-Path $local) { . $local }

function Say($m, $c = 'Gray') { Write-Host "[$(Get-Date -Format HH:mm:ss)] $m" -ForegroundColor $c }
function Fail($m) { Say $m 'Red'; Read-Host 'Press Enter to close'; exit 1 }

try {
  if ($Stop) {
    & (Join-Path $PSScriptRoot 'restart-world.ps1') -Stop
    Say 'Servers stopped.' 'Green'; Start-Sleep 3; exit 0
  }

  # 0. don't fight the overnight pipeline
  if (Test-Path $Pipeline) {
    $st = Get-Content $Pipeline -Raw | ConvertFrom-Json
    if ($st.state -in 'running', 'waiting' -and (Get-Process -Id $st.pid -ErrorAction SilentlyContinue)) {
      Fail "The overnight pipeline is still running (stage '$($st.stage)': $($st.message)). Try again when it has finished."
    }
  }

  # 1. close apps
  if (-not $NoCloseApps) {
    $procs = foreach ($n in $AppsToClose) { Get-Process -Name $n -ErrorAction SilentlyContinue }
    if ($procs) {
      Say "Closing: $(($procs.Name | Sort-Object -Unique) -join ', ')"
      $procs | ForEach-Object { try { [void]$_.CloseMainWindow() } catch { } }
      Start-Sleep 4
      foreach ($n in $AppsToClose) { Get-Process -Name $n -ErrorAction SilentlyContinue | Stop-Process -Force -ErrorAction SilentlyContinue }
    }
  }

  # 2. MySQL
  $svc = Get-Service MySQL80
  if ($svc.Status -ne 'Running') {
    Say 'Starting MySQL (a Windows permission prompt may appear)...' 'Yellow'
    Start-Process powershell -Verb RunAs -WindowStyle Hidden -ArgumentList '-NoProfile', '-Command', 'Start-Service MySQL80' -Wait
    $svc.WaitForStatus('Running', [TimeSpan]::FromSeconds(60))
  }
  Say 'MySQL running.'

  # 3. bot setting
  $pbConf = Join-Path $Bin 'playerbots.conf'
  $want = if ($Bots -eq 'on') { '1' } else { '0' }
  $have = if (Test-Path $pbConf) { ([regex]::Match((Get-Content $pbConf -Raw), '(?m)^AiPlayerbot\.Enabled\s*=\s*(\d)')).Groups[1].Value } else { '' }
  $confChanged = $false
  if ($have -ne $want) {
    Say "Writing confs (bots $Bots)"
    & powershell -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot 'write-confs.ps1') -DataDir $Data -Bots $Bots | Out-Host
    if ($LASTEXITCODE -ne 0) { Fail 'write-confs failed' }
    $confChanged = $true
  }

  # 4. servers
  $up = $false
  if ((Get-Process worldserver -ErrorAction SilentlyContinue) -and -not $confChanged) {
    try { $up = ((& (Join-Path $PSScriptRoot 'gm.ps1') '.server info' 2>&1 | Out-String) -match '(?i)uptime') } catch { }
  }
  if ($up) { Say 'Servers already running.' 'Green' }
  else {
    Say 'Starting servers (first start with bots can take several minutes)...'
    & powershell -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot 'restart-world.ps1') -StartTimeout 3600 | Out-Host
    if ($LASTEXITCODE -ne 0) { Fail "Server start failed (code $LASTEXITCODE). Logs: D:\WoWMoPProject\ServerData\logs" }
  }

  # 5. client
  if (-not $NoClient) {
    if (-not (Test-Path $ClientExe)) { Fail "Client not found: $ClientExe" }
    # Classic login only: realmlist = our authserver, no portal line (a portal makes the client try Battle.net login).
    $wtf = Join-Path (Split-Path $ClientExe) 'WTF\Config.wtf'
    if (Test-Path $wtf) {
      $rest = Get-Content $wtf | Where-Object { $_ -notmatch '^SET (realmlist|portal) ' }
      @('SET realmlist "127.0.0.1"') + $rest | Set-Content $wtf -Encoding ascii
    }
    Say "Launching $(Split-Path $ClientExe -Leaf)" 'Green'
    Start-Process -FilePath $ClientExe -WorkingDirectory (Split-Path $ClientExe)
  }
  Say 'Done. Log in with ARPGADMIN (password: arpg\scripts\soap.local.ps1).' 'Green'
  Start-Sleep 5
} catch {
  Fail "Error: $($_.Exception.Message)"
}
