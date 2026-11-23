<#
.SYNOPSIS
  m6-any-race-class check: creates a character of a race x class pair stock MoP doesn't allow (default: Undead Monk) on the
  headless test account, checks it in the DB (race, class, level, start map), then erases it again. Exit 0 = OK.
.EXAMPLE
  pwsh -File arpg\scripts\test-char-create.ps1                 # Undead (5) Monk (10)
  pwsh -File arpg\scripts\test-char-create.ps1 -Race 6 -Class 4  # Tauren Rogue
#>
[CmdletBinding()]
param(
  [int]$Race = 5,
  [int]$Class = 10,
  [string]$Name = 'Arpgmixtest'
)
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$mysql = 'C:\Program Files\MySQL\MySQL Server 8.0\bin\mysql.exe'
$cnf = Join-Path $PSScriptRoot 'mysql.local.cnf'
function Q([string]$sql) { (& $mysql "--defaults-extra-file=$cnf" -N -e $sql 2>$null | Out-String).Trim() }

# Leftover from an earlier run: erase it first.
if (Q "SELECT guid FROM characters.characters WHERE name = '$Name'") { & (Join-Path $PSScriptRoot 'gm.ps1') ".character erase $Name" | Out-Null; Start-Sleep 2 }

Push-Location (Join-Path $root 'contrib\headless-client')
try {
  $out = & py headless.py --cmd create --create-name $Name --create-race $Race --create-class $Class 2>&1 | Out-String
} finally { Pop-Location }
$row = Q "SELECT race, class, level, map FROM characters.characters WHERE name = '$Name'"
if (-not $row) {
  Write-Host "FAIL: $Name (race $Race class $Class) not created"
  Write-Host ($out -split "`n" | Select-String 'char_create|ERROR' | Select-Object -Last 5 | Out-String)
  exit 1
}
$f = $row -split "`t"
Write-Host "OK created $Name race=$($f[0]) class=$($f[1]) level=$($f[2]) map=$($f[3])"
& (Join-Path $PSScriptRoot 'gm.ps1') ".character erase $Name" | Out-Null
if ([int]$f[0] -ne $Race -or [int]$f[1] -ne $Class) { Write-Host 'FAIL: wrong race/class stored'; exit 1 }
exit 0
