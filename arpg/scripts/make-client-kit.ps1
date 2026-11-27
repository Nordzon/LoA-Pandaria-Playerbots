<#
.SYNOPSIS
  Builds the zip a mate needs to play on this server (m8-friends): our client patch, the ArpgUI addon and an installer.

.DESCRIPTION
  The mate brings their own MoP 5.4.8 (build 18414) client. The kit adds:
    Data\enUS\wow-update-enUS-20000.MPQ   our patch (affix lines, names, race/class pairs, ...)
    Interface\AddOns\ArpgUI\              our addon (loot tooltips, Paragon, filter, professions)
    install-kit.ps1                       copies both into the client, sets the realmlist, removes a portal line
    README.txt
  -IncludeExe adds Wow-64-all.exe (the unlocked client exe that loads custom patches; third-party file - only if the owner
  is fine sharing it). Output: D:\WoWMoPProject\ServerData\client-kit\pandaria-arpg-kit-<date>.zip
.EXAMPLE
  powershell -ExecutionPolicy Bypass -File arpg\scripts\make-client-kit.ps1 -Realm my-pc.tailnet.ts.net
#>
[CmdletBinding()]
param(
  [string]$Realm = '',                 # what mates put in their realmlist: the owner's Tailscale address or MagicDNS name
  [string]$Client = 'D:\WoWMoPProject\MOP-5.4.8.18414-enUS-Repack',
  [string]$Out = 'D:\WoWMoPProject\ServerData\client-kit',
  [switch]$IncludeExe
)
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
if (-not $Realm) {
  $netConf = Join-Path $PSScriptRoot 'network.local.ps1'
  if (Test-Path $netConf) { . $netConf; $Realm = $RealmAddress }
}
if (-not $Realm) { Write-Error 'pass -Realm <your Tailscale address or name> (or set $RealmAddress in network.local.ps1)'; exit 2 }
if ($Realm -notmatch '^[A-Za-z0-9.-]{1,253}$') { Write-Error "bad realm address '$Realm'"; exit 2 }

$mpq = Join-Path $Client 'Data\enUS\wow-update-enUS-20000.MPQ'
if (-not (Test-Path $mpq)) { Write-Error "patch not found: $mpq (build it: arpg\client-patch\build-client-patch.py --install)"; exit 1 }
$stage = Join-Path $env:TEMP ('arpg-kit-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Force (Join-Path $stage 'Data\enUS'), (Join-Path $stage 'Interface\AddOns') | Out-Null
Copy-Item $mpq (Join-Path $stage 'Data\enUS\')
Copy-Item (Join-Path $root 'arpg\addons\ArpgUI') (Join-Path $stage 'Interface\AddOns\') -Recurse
if ($IncludeExe) { Copy-Item (Join-Path $Client 'Wow-64-all.exe') $stage }

@"
# Installs the Pandaria ARPG kit into a MoP 5.4.8 client. Right-click -> Run with PowerShell, or:
#   powershell -ExecutionPolicy Bypass -File install-kit.ps1 -Client "C:\Games\MoP"
param([string]`$Client = '')
`$ErrorActionPreference = 'Stop'
if (-not `$Client) { `$Client = Read-Host 'Folder of your MoP 5.4.8 client (the one with Wow-64.exe)' }
if (-not (Test-Path (Join-Path `$Client 'Data'))) { throw "not a WoW client folder: `$Client" }
`$here = Split-Path -Parent `$MyInvocation.MyCommand.Path
Copy-Item (Join-Path `$here 'Data\enUS\wow-update-enUS-20000.MPQ') (Join-Path `$Client 'Data\enUS\') -Force
New-Item -ItemType Directory -Force (Join-Path `$Client 'Interface\AddOns') | Out-Null
if (Test-Path (Join-Path `$Client 'Interface\AddOns\ArpgUI')) { Remove-Item (Join-Path `$Client 'Interface\AddOns\ArpgUI') -Recurse -Force }
Copy-Item (Join-Path `$here 'Interface\AddOns\ArpgUI') (Join-Path `$Client 'Interface\AddOns\') -Recurse
if (Test-Path (Join-Path `$here 'Wow-64-all.exe')) { Copy-Item (Join-Path `$here 'Wow-64-all.exe') `$Client -Force }
`$wtf = Join-Path `$Client 'WTF\Config.wtf'
New-Item -ItemType Directory -Force (Split-Path `$wtf) | Out-Null
`$rest = if (Test-Path `$wtf) { Get-Content `$wtf | Where-Object { `$_ -notmatch '^SET (realmlist|portal) ' } } else { @() }
@('SET realmlist "$Realm"') + `$rest | Set-Content `$wtf -Encoding ascii
Write-Host 'Installed. Start Wow-64-all.exe (not Wow-64.exe) and log in with the account you made on the sign-up page.'
"@ | Set-Content (Join-Path $stage 'install-kit.ps1') -Encoding utf8

@"
Pandaria ARPG - client kit ($(Get-Date -Format 'yyyy-MM-dd'))

You need: a World of Warcraft Mists of Pandaria 5.4.8 (build 18414) enUS client, and Tailscale connected to the owner's
network (the owner sends you an invite).

1. Unzip this folder anywhere.
2. Right-click install-kit.ps1 -> Run with PowerShell, and give it your client folder.
3. Make an account on the sign-up page the owner gave you (a web address on his Tailscale network).
4. Start Wow-64-all.exe from your client folder (the normal Wow-64.exe ignores our patch: plain items, no affix lines).
   No Wow-64-all.exe in your folder? Ask the owner for it.
5. Log in with your account. Realm: $Realm

Updates: the owner sends a new kit when the patch or the addon changes; run install-kit.ps1 again.
"@ | Set-Content (Join-Path $stage 'README.txt') -Encoding utf8

New-Item -ItemType Directory -Force $Out | Out-Null
$zip = Join-Path $Out ("pandaria-arpg-kit-{0}.zip" -f (Get-Date -Format 'yyyyMMdd-HHmm'))
Compress-Archive -Path (Join-Path $stage '*') -DestinationPath $zip -Force
Remove-Item $stage -Recurse -Force
$kb = [math]::Round((Get-Item $zip).Length / 1KB)
Write-Host "client kit: $zip ($kb KB; realm $Realm$(if ($IncludeExe) { '; includes Wow-64-all.exe' }))"
