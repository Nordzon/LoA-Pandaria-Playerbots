<#
.SYNOPSIS
  Starts the sign-up page (arpg/signup/signup.py) detached, if it isn't already listening (m8-friends). -Stop stops it.
  Settings: arpg/signup/signup.local.json (copy signup.local.json.example). start-stack.ps1 calls this when that file exists.
#>
[CmdletBinding()]
param([switch]$Stop)
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$script = Join-Path $root 'arpg\signup\signup.py'
$cfgFile = Join-Path $root 'arpg\signup\signup.local.json'
$port = 8090; $bind = '127.0.0.1'
if (Test-Path $cfgFile) { $c = Get-Content $cfgFile -Raw | ConvertFrom-Json; if ($c.port) { $port = [int]$c.port }; if ($c.bind) { $bind = $c.bind } }
$running = Get-CimInstance Win32_Process -Filter "Name = 'py.exe' OR Name = 'python.exe'" | Where-Object { $_.CommandLine -like '*signup.py*' -and $_.CommandLine -notlike '*--port*' }
if ($Stop) { $running | ForEach-Object { Stop-Process -Id $_.ProcessId -Force -Confirm:$false }; Write-Host "sign-up page stopped ($(@($running).Count))"; exit 0 }
if ($running) { Write-Host "sign-up page already running on ${bind}:$port"; exit 0 }
Start-Process py -ArgumentList '-I', "`"$script`"" -WindowStyle Hidden -WorkingDirectory $root
$probe = if ($bind -eq '0.0.0.0') { '127.0.0.1' } else { $bind }
$deadline = (Get-Date).AddSeconds(10)
do { Start-Sleep -Milliseconds 300; try { $ok = (Invoke-WebRequest "http://${probe}:$port/" -UseBasicParsing -TimeoutSec 2).StatusCode -eq 200 } catch { $ok = $false } } until ($ok -or (Get-Date) -gt $deadline)
if (-not $ok) { Write-Error "sign-up page did not answer on ${probe}:$port"; exit 1 }
Write-Host "sign-up page running: http://${bind}:$port/"
