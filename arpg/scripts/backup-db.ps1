<#
.SYNOPSIS
  Backs up the accounts and characters databases (auth + characters) so players' characters can't be lost.

.DESCRIPTION
  mysqldump --single-transaction: consistent and safe while the servers run (nobody is kicked). One folder per run under
  D:\WoWMoPProject\ServerData\backups\<yyyyMMdd-HHmmss>-<reason>\ with auth.sql and characters.sql; the newest -Keep
  folders stay (default 30). rebuild-and-test.ps1 calls it before every stop/SQL change (owner 2026-11-27: "keep a backup of
  my account when we do implementations").
  Restore (servers stopped!):  mysql --defaults-extra-file=arpg\scripts\mysql.local.cnf characters < <folder>\characters.sql
  (and auth.sql the same way into auth).

.EXAMPLE
  powershell -ExecutionPolicy Bypass -File arpg\scripts\backup-db.ps1 -Reason manual
#>
[CmdletBinding()]
param(
  [ValidatePattern('^[A-Za-z0-9-]{1,30}$')][string]$Reason = 'manual',
  [string]$Root = 'D:\WoWMoPProject\ServerData\backups',
  [int]$Keep = 30
)
$ErrorActionPreference = 'Stop'
$dump = 'C:\Program Files\MySQL\MySQL Server 8.0\bin\mysqldump.exe'
$cnf = Join-Path $PSScriptRoot 'mysql.local.cnf'
if (-not (Test-Path $cnf)) { Write-Error "missing $cnf (MySQL login)"; exit 2 }
$dir = Join-Path $Root ("{0}-{1}" -f (Get-Date -Format 'yyyyMMdd-HHmmss'), $Reason)
New-Item -ItemType Directory -Force $dir | Out-Null
foreach ($db in 'auth', 'characters') {
  $out = Join-Path $dir "$db.sql"
  & $dump "--defaults-extra-file=$cnf" --single-transaction --quick --routines --no-tablespaces "--result-file=$out" $db 2>&1 |
    Where-Object { $_ -notmatch 'Using a password' } | ForEach-Object { Write-Host $_ }
  if ($LASTEXITCODE -ne 0 -or -not (Test-Path $out) -or (Get-Item $out).Length -lt 1000) { Write-Error "backup of $db failed"; exit 1 }
}
$size = [math]::Round(((Get-ChildItem $dir | Measure-Object Length -Sum).Sum) / 1MB, 1)
Get-ChildItem $Root -Directory | Sort-Object Name -Descending | Select-Object -Skip $Keep | Remove-Item -Recurse -Force -Confirm:$false
Write-Host "backup OK $dir ($size MB)"
exit 0
