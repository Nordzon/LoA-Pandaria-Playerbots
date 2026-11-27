<#
.SYNOPSIS
  Tests the sign-up page end to end (m8-friends 2.3). No server restart; safe while someone plays.

.DESCRIPTION
  Starts arpg/signup/signup.py on a test port with a test config (invite code, cap), then: a valid sign-up creates account
  SIGNUPTEST1 (DB row: player, tagged signup, sha hash); the headless client logs in with it; a wrong password fails;
  a taken name, different passwords, a bad name, a reserved name and a wrong invite code are refused; the existing account's
  hash is unchanged. The test account is deleted at the start and the end. Exit 0 = all OK.
#>
[CmdletBinding()]
param([int]$Port = 8099)
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$mysql = 'C:\Program Files\MySQL\MySQL Server 8.0\bin\mysql.exe'
$cnf = Join-Path $PSScriptRoot 'mysql.local.cnf'
$name = 'SIGNUPTEST1'; $pass = 'Test4567'
$fails = 0
function Q($s) { (& $mysql "--defaults-extra-file=$cnf" -D auth -N -B -e $s 2>$null | Out-String).Trim() }
function Check($what, $ok) { if ($ok) { Write-Host "PASS  $what" } else { Write-Host "FAIL  $what" -ForegroundColor Red; $script:fails++ } }
function Cleanup { Q "DELETE r FROM realmcharacters r JOIN account a ON a.id = r.acctid WHERE a.username = '$name'; DELETE FROM account WHERE username = '$name';" | Out-Null }
function Post($fields) {
  try { $r = Invoke-WebRequest -Uri "http://127.0.0.1:$Port/register" -Method Post -Body $fields -UseBasicParsing -TimeoutSec 20 }
  catch { $r = $_.Exception.Response; if ($r) { return @{ Status = [int]$r.StatusCode; Body = [string]$_.ErrorDetails.Message } } throw }
  @{ Status = [int]$r.StatusCode; Body = $r.Content }
}
function Login($account, $password) {
  $env:MOP_ACCOUNT = $account; $env:MOP_PASSWORD = $password
  $ErrorActionPreference = 'Continue'   # a refused login ends with a traceback on stderr: an exit code, not a script error
  Push-Location (Join-Path $root 'contrib\headless-client')
  try { $out = & py headless.py --cmd login 2>&1 | Out-String; $code = $LASTEXITCODE } finally { Pop-Location; Remove-Item Env:MOP_ACCOUNT, Env:MOP_PASSWORD -ErrorAction SilentlyContinue }
  @{ Code = $code; Out = $out }
}

Cleanup
$cfg = Join-Path $env:TEMP 'signup-test.json'
@{ bind = '127.0.0.1'; port = $Port; invite_code = 'letmein'; max_accounts = 1000; rate_max = 100; log = (Join-Path $env:TEMP 'signup-test.log') } | ConvertTo-Json | Set-Content $cfg
$proc = Start-Process py -ArgumentList '-I', (Join-Path $root 'arpg\signup\signup.py'), '--config', $cfg -PassThru -WindowStyle Hidden
try {
  $deadline = (Get-Date).AddSeconds(15)
  do { Start-Sleep -Milliseconds 300; try { $up = (Invoke-WebRequest "http://127.0.0.1:$Port/" -UseBasicParsing -TimeoutSec 2).StatusCode -eq 200 } catch { $up = $false } } until ($up -or (Get-Date) -gt $deadline)
  Check 'page answers' $up

  $r = Post @{ name = 'Signuptest1'; password = $pass; confirm = $pass; invite = 'letmein' }
  Check "valid sign-up -> 200 'created' ($($r.Status))" ($r.Status -eq 200 -and $r.Body -match 'Account SIGNUPTEST1 created')
  $row = Q "SELECT CONCAT(a.invited_by, '|', LENGTH(a.sha_pass_hash), '|', (SELECT COUNT(*) FROM account_access x WHERE x.id = a.id), '|', (SELECT COUNT(*) FROM realmcharacters c WHERE c.acctid = a.id)) FROM account a WHERE a.username = '$name'"
  Check "DB row: tag signup, 40-char hash, no GM, realm row ($row)" ($row -eq 'signup|40|0|1')
  $hash = Q "SELECT sha_pass_hash FROM account WHERE username = '$name'"

  $l = Login $name $pass
  Check "headless login with the new account (exit $($l.Code))" ($l.Code -eq 0)
  $l = Login $name 'Wrong999'
  Check "wrong password refused (exit $($l.Code))" ($l.Code -ne 0)

  $r = Post @{ name = $name; password = 'Other123'; confirm = 'Other123'; invite = 'letmein' }
  Check 'taken name refused' ($r.Status -eq 400 -and $r.Body -match 'taken')
  Check 'existing password unchanged' ((Q "SELECT sha_pass_hash FROM account WHERE username = '$name'") -eq $hash)
  $r = Post @{ name = 'Signuptest2'; password = 'Abcd1234'; confirm = 'Abcd9999'; invite = 'letmein' }
  Check 'different passwords refused' ($r.Status -eq 400 -and $r.Body -match 'differ')
  $r = Post @{ name = "x'; DROP"; password = 'Abcd1234'; confirm = 'Abcd1234'; invite = 'letmein' }
  Check 'bad name refused' ($r.Status -eq 400 -and $r.Body -match 'letters or digits')
  $r = Post @{ name = 'Rndbot999'; password = 'Abcd1234'; confirm = 'Abcd1234'; invite = 'letmein' }
  Check 'reserved name refused' ($r.Status -eq 400 -and $r.Body -match 'reserved')
  $r = Post @{ name = 'Signuptest2'; password = 'Abcd1234'; confirm = 'Abcd1234'; invite = 'nope' }
  Check 'wrong invite code refused' ($r.Status -eq 400 -and $r.Body -match 'invite')
  Check 'no stray accounts' ((Q "SELECT COUNT(*) FROM account WHERE username IN ('SIGNUPTEST2', 'RNDBOT999')") -eq '0')
} finally {
  Stop-Process -Id $proc.Id -Force -ErrorAction SilentlyContinue
  Cleanup
  Remove-Item $cfg -ErrorAction SilentlyContinue
}
if ($fails) { Write-Host "test-signup: $fails FAILED" -ForegroundColor Red; exit 1 }
Write-Host 'test-signup: ALL OK'
exit 0
