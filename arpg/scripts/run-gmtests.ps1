<#
.SYNOPSIS
  Runs GM-command test scripts (arpg\tests\*.gmtest) against the running worldserver via SOAP.

.DESCRIPTION
  .gmtest format, one directive per line:
      # comment
      > .some gm command          send it; its reply becomes "the last reply"
      ? regex                     last reply must match (case-insensitive)
      ! regex                     last reply must NOT match
      ~ 500                       wait 500 ms (respawns, ticks)
      * regex                     like ?, but re-sends the last command every 2 s until it matches (max 20 s; timer events)
  A file passes if every ? and ! holds. Exit code = number of failed files.

.EXAMPLE
  .\arpg\scripts\run-gmtests.ps1                  # all tests
  .\arpg\scripts\run-gmtests.ps1 -Filter scaling  # files whose name contains "scaling"
#>
param([string]$Filter = '', [string]$Dir = (Join-Path $PSScriptRoot '..\tests'))
$ErrorActionPreference = 'Stop'
$gm = Join-Path $PSScriptRoot 'gm.ps1'
$failedFiles = 0

foreach ($file in Get-ChildItem $Dir -Filter '*.gmtest' | Where-Object { $_.Name -like "*$Filter*" }) {
  $last = ''; $fails = @(); $n = 0
  foreach ($line in Get-Content $file.FullName) {
    $n++
    $t = $line.Trim()
    if ($t -eq '' -or $t.StartsWith('#')) { continue }
    $op = $t.Substring(0, 1); $arg = $t.Substring(1).Trim()
    switch ($op) {
      '>' {
        # A command that throws (bad reply, SOAP down) is a failed check, not the end of the whole run.
        $lastCmd = $arg
        try { $last = (& $gm $arg 2>&1 | Out-String) } catch { $last = "EXCEPTION: $($_.Exception.Message)"; $fails += "line ${n}: $last" }
      }
      '*' {
        # Like '?', but re-runs the last command every 2 s until it matches (up to 20 s): for events that come on a timer.
        $deadline = (Get-Date).AddSeconds(20)
        while ($last -notmatch $arg -and (Get-Date) -lt $deadline) {
          Start-Sleep -Seconds 2
          try { $last = (& $gm $lastCmd 2>&1 | Out-String) } catch { $last = "EXCEPTION: $($_.Exception.Message)" }
        }
        if ($last -notmatch $arg) { $fails += "line ${n}: expected (within 20 s) /$arg/ in: $($last.Trim())" }
      }
      '?' { if ($last -notmatch $arg) { $fails += "line ${n}: expected /$arg/ in: $($last.Trim())" } }
      '!' { if ($last -match $arg) { $fails += "line ${n}: did not expect /$arg/ in: $($last.Trim())" } }
      '~' { Start-Sleep -Milliseconds ([int]$arg) }
      default { $fails += "line ${n}: unknown directive '$op'" }
    }
  }
  if ($fails.Count -eq 0) { Write-Host "PASS  $($file.Name)" -ForegroundColor Green }
  else {
    $failedFiles++
    Write-Host "FAIL  $($file.Name)" -ForegroundColor Red
    $fails | ForEach-Object { Write-Host "      $_" }
  }
}
exit $failedFiles
