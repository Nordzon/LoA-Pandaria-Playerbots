<#
.SYNOPSIS
  Sets the address the authserver sends to clients for our realm (m8-friends). No restart: the authserver re-reads the realm
  list every 20 s (RealmsStateUpdateDelay).

.DESCRIPTION
  address = -Address, else $RealmAddress from network.local.ps1, else 127.0.0.1. localAddress stays 127.0.0.1: a client on
  this PC (the owner's) connects over loopback and gets loopback back (Realm::GetAddressForClient); mates get -Address.
.EXAMPLE
  powershell -ExecutionPolicy Bypass -File arpg\scripts\set-realm-address.ps1 -Address 100.101.102.103
  powershell -ExecutionPolicy Bypass -File arpg\scripts\set-realm-address.ps1 -Reset      # back to 127.0.0.1 only
#>
[CmdletBinding()]
param([string]$Address = '', [switch]$Reset)
$ErrorActionPreference = 'Stop'
$mysql = 'C:\Program Files\MySQL\MySQL Server 8.0\bin\mysql.exe'
$cnf = Join-Path $PSScriptRoot 'mysql.local.cnf'
if ($Reset) { $Address = '127.0.0.1' }
if (-not $Address) {
  $netConf = Join-Path $PSScriptRoot 'network.local.ps1'
  if (Test-Path $netConf) { . $netConf; $Address = $RealmAddress }
}
if (-not $Address) { $Address = '127.0.0.1' }
if ($Address -notmatch '^\d{1,3}(\.\d{1,3}){3}$') { Write-Error "address must be an IPv4 address (tailscale ip -4), got '$Address'"; exit 2 }
& $mysql "--defaults-extra-file=$cnf" -D auth -e "UPDATE realmlist SET address = '$Address', localAddress = '127.0.0.1', localSubnetMask = '255.0.0.0' WHERE id = 1; SELECT id, name, address, localAddress, port FROM realmlist;"
if ($LASTEXITCODE -ne 0) { Write-Error 'mysql failed'; exit 1 }
Write-Host "realm address = $Address (applies within 20 s; this PC keeps using 127.0.0.1)"
