<#
.SYNOPSIS
  Sends one GM command to the running worldserver over SOAP and prints the reply.

.DESCRIPTION
  Needs in worldserver.conf:  SOAP.Enabled = 1, SOAP.IP = "127.0.0.1", SOAP.Port = 7878
  Credentials: a GM account (level 3). Put them in arpg\scripts\soap.local.ps1 (gitignored):
      $SoapUser = 'ADMIN'; $SoapPass = 'yourpass'
  or set ARPG_SOAP_USER / ARPG_SOAP_PASS.

  SOAP has no player session: there is no "selected target". Commands that need one must name
  the player or GUID explicitly - all our .arpg debug commands accept that for this reason.

.EXAMPLE
  .\arpg\scripts\gm.ps1 ".server info"
  .\arpg\scripts\gm.ps1 ".arpg scale info Dimitar"
#>
param(
  [Parameter(Mandatory = $true, Position = 0)][string]$Command,
  [string]$Server = '127.0.0.1',
  [int]$Port = 7878,
  [switch]$Raw
)
$ErrorActionPreference = 'Stop'

$SoapUser = $env:ARPG_SOAP_USER
$SoapPass = $env:ARPG_SOAP_PASS
$local = Join-Path $PSScriptRoot 'soap.local.ps1'
if (Test-Path $local) { . $local }
if (-not $SoapUser -or -not $SoapPass) { throw 'No SOAP credentials: create arpg\scripts\soap.local.ps1 or set ARPG_SOAP_USER/ARPG_SOAP_PASS.' }

# SOAP commands are sent without the leading dot.
$cmd = $Command.Trim().TrimStart('.')
$escaped = [System.Security.SecurityElement]::Escape($cmd)
$body = @"
<?xml version="1.0" encoding="utf-8"?>
<SOAP-ENV:Envelope xmlns:SOAP-ENV="http://schemas.xmlsoap.org/soap/envelope/" xmlns:ns1="urn:TC">
  <SOAP-ENV:Body><ns1:executeCommand><command>$escaped</command></ns1:executeCommand></SOAP-ENV:Body>
</SOAP-ENV:Envelope>
"@

$auth = [Convert]::ToBase64String([Text.Encoding]::UTF8.GetBytes("$SoapUser`:$SoapPass"))
try {
  $resp = Invoke-WebRequest -Uri "http://$Server`:$Port/" -Method Post -Body $body `
    -ContentType 'application/xml' -Headers @{ Authorization = "Basic $auth" } -UseBasicParsing
  $text = $resp.Content
} catch {
  # Failed commands come back as a SOAP fault with HTTP 500; still print the message.
  if ($_.ErrorDetails -and $_.ErrorDetails.Message) {
    $text = $_.ErrorDetails.Message           # PowerShell 7
  } elseif ($_.Exception.Response -and $_.Exception.Response.GetResponseStream) {
    $reader = New-Object IO.StreamReader($_.Exception.Response.GetResponseStream())
    $text = $reader.ReadToEnd()               # Windows PowerShell 5.1
  } else { throw }
}

if ($Raw) { $text; return }
# Failed commands may come back as a SOAP fault that is not well-formed XML: fall back to the plain text.
try { $xml = [xml]$text } catch {
  $plain = (($text -replace '<[^>]+>', ' ') -replace '&#xD;', '' -replace '\s+', ' ').Trim()
  Write-Output "FAULT: $plain"; exit 1
}
$result = $xml.SelectSingleNode("//*[local-name()='result']")
$fault = $xml.SelectSingleNode("//*[local-name()='faultstring']")
if ($result) { $result.InnerText.TrimEnd() }
elseif ($fault) { Write-Output "FAULT: $($fault.InnerText.TrimEnd())"; exit 1 }
else { $text }
