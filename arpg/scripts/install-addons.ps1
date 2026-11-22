<#
.SYNOPSIS
  Copies our client addons (arpg\addons\*) into the game client's Interface\AddOns folder. Re-run after addon changes,
  then /reload in game (or restart the client).
.EXAMPLE
  pwsh -File arpg\scripts\install-addons.ps1
#>
[CmdletBinding()]
param(
  [string]$Client = 'D:\WoWMoPProject\MOP-5.4.8.18414-enUS-Repack'
)
$ErrorActionPreference = 'Stop'
$src = Join-Path $PSScriptRoot '..\addons'
$dst = Join-Path $Client 'Interface\AddOns'
if (-not (Test-Path $dst)) { throw "client AddOns folder not found: $dst" }
Get-ChildItem $src -Directory | ForEach-Object {
  $target = Join-Path $dst $_.Name
  if (Test-Path $target) { Remove-Item $target -Recurse -Force }
  Copy-Item $_.FullName $target -Recurse
  Write-Host "installed $($_.Name) -> $target"
}
