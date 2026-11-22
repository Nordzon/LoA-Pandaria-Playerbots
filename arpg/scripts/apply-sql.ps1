<#
.SYNOPSIS
  Applies mod_arpg SQL files (modules\mod_arpg\sql\world\*.sql -> world DB, sql\characters\*.sql -> characters DB),
  in file-name order. Files are idempotent by rule (CLAUDE.md), so re-running is safe.

.EXAMPLE
  powershell -ExecutionPolicy Bypass -File arpg\scripts\apply-sql.ps1
#>
[CmdletBinding()]
param([string]$Filter = '*')
$ErrorActionPreference = 'Stop'
$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$mysql = 'C:\Program Files\MySQL\MySQL Server 8.0\bin\mysql.exe'
$cnf = Join-Path $PSScriptRoot 'mysql.local.cnf'
$failed = 0
foreach ($pair in @(@('world', 'world'), @('characters', 'characters'))) {
  $dir = Join-Path $RepoRoot "modules\mod_arpg\sql\$($pair[0])"
  if (-not (Test-Path $dir)) { continue }
  foreach ($f in Get-ChildItem $dir -Filter "$Filter.sql" | Sort-Object Name) {
    $path = $f.FullName -replace '\\', '/'
    $out = & $mysql "--defaults-extra-file=$cnf" $pair[1] -e "source $path" 2>&1 | Out-String
    if ($LASTEXITCODE -ne 0) { $failed++; Write-Host "FAIL  $($pair[1])  $($f.Name)`n$out" -ForegroundColor Red }
    else { Write-Host "ok    $($pair[1])  $($f.Name)" }
  }
}
exit $failed
