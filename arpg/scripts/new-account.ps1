<#
.SYNOPSIS
  Creates (or resets the password of) a game account directly in the auth DB. No server console needed.

.DESCRIPTION
  Same storage as the worldserver's "account create": sha_pass_hash = UPPER(SHA1(UPPER(name):UPPER(pass))),
  v/s left empty (authserver derives them at login). -GmLevel sets account_access for all realms (-1).
  DB login comes from arpg\scripts\mysql.local.cnf. Names/passwords: letters and digits, password <= 16.

.EXAMPLE
  powershell -ExecutionPolicy Bypass -File arpg\scripts\new-account.ps1 -Name ARPGADMIN -Password Secret123 -GmLevel 3
#>
[CmdletBinding()]
param(
  [Parameter(Mandatory)][ValidatePattern('^[A-Za-z0-9]{3,16}$')][string]$Name,
  [Parameter(Mandatory)][ValidatePattern('^[A-Za-z0-9]{4,16}$')][string]$Password,
  [ValidateRange(0, 3)][int]$GmLevel = 0
)
$ErrorActionPreference = 'Stop'
$mysql = 'C:\Program Files\MySQL\MySQL Server 8.0\bin\mysql.exe'
$cnf = Join-Path $PSScriptRoot 'mysql.local.cnf'
$u = $Name.ToUpperInvariant(); $p = $Password.ToUpperInvariant()
$sha = [Security.Cryptography.SHA1]::Create()
$hash = -join ($sha.ComputeHash([Text.Encoding]::UTF8.GetBytes("${u}:${p}")) | ForEach-Object { $_.ToString('X2') })

$sql = @"
INSERT INTO account (username, sha_pass_hash, email, reg_mail, joindate, expansion)
  VALUES ('$u', '$hash', '', '', NOW(), 4)
  ON DUPLICATE KEY UPDATE sha_pass_hash = VALUES(sha_pass_hash), v = '', s = '', sessionkey = '', failed_logins = 0, locked = 0;
INSERT INTO realmcharacters (realmid, acctid, numchars)
  SELECT DISTINCT realmlist.id, account.id, 0 FROM realmlist, account
  LEFT JOIN realmcharacters ON acctid = account.id WHERE acctid IS NULL;
DELETE FROM account_access WHERE id = (SELECT id FROM account WHERE username = '$u');
$(if ($GmLevel -gt 0) { "INSERT INTO account_access (id, gmlevel, RealmID) SELECT id, $GmLevel, -1 FROM account WHERE username = '$u';" })
SELECT a.id, a.username, IFNULL(aa.gmlevel, 0) AS gmlevel FROM account a LEFT JOIN account_access aa ON aa.id = a.id WHERE a.username = '$u';
"@
$sql | & $mysql "--defaults-extra-file=$cnf" auth
if ($LASTEXITCODE -ne 0) { throw "mysql failed ($LASTEXITCODE)" }
