<#
.SYNOPSIS
  Checks (and optionally installs) everything needed to build and run the Pandaria ARPG server on Windows.

.DESCRIPTION
  Default: check only. Prints a table of what's installed, what's missing, and what's the wrong version.
  -Install:              install missing tools with winget (run from an elevated PowerShell).
  -AddDefenderExclusion: exclude the repo folder from Windows Defender real-time scanning (needs admin).
  -WowPath:              your WoW 5.4.8 client folder, so it can be checked too.

.EXAMPLE
  powershell -ExecutionPolicy Bypass -File arpg\scripts\setup-tools.ps1
  powershell -ExecutionPolicy Bypass -File arpg\scripts\setup-tools.ps1 -Install -AddDefenderExclusion -WowPath "D:\Games\WoW548"
#>
[CmdletBinding()]
param(
  [switch]$Install,
  [switch]$AddDefenderExclusion,
  [string]$WowPath = $env:ARPG_WOW_PATH
)

$ErrorActionPreference = 'Stop'
$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$results = New-Object System.Collections.Generic.List[object]

function Add-Result($name, $status, $detail) {
  $results.Add([pscustomobject]@{ Tool = $name; Status = $status; Detail = $detail })
}

function Test-Admin {
  $id = [Security.Principal.WindowsIdentity]::GetCurrent()
  return (New-Object Security.Principal.WindowsPrincipal($id)).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
}

function Update-SessionPath {
  $machine = [Environment]::GetEnvironmentVariable('Path', 'Machine')
  $user = [Environment]::GetEnvironmentVariable('Path', 'User')
  $env:Path = "$machine;$user"
}

function Get-CommandVersion($exe, [string[]]$argList, $pattern) {
  $cmd = Get-Command $exe -ErrorAction SilentlyContinue
  if (-not $cmd) { return $null }
  try {
    $out = (& $cmd.Source @argList 2>&1 | Out-String)
    $m = [regex]::Match($out, $pattern)
    if ($m.Success) { return $m.Groups[1].Value }
    # No version in the output = not really there (e.g. the Microsoft Store "python" stub,
    # which only prints "Python was not found..."). Real Store-installed Python prints its version.
    return $null
  } catch { return $null }
}

function Test-VersionRange($version, $min, $maxExclusive) {
  try {
    $v = [version]$version
    if ($min -and $v -lt [version]$min) { return $false }
    if ($maxExclusive -and $v -ge [version]$maxExclusive) { return $false }
    return $true
  } catch { return $false }
}

# Picks the newest version of a winget package that matches a regex (e.g. '^8\.0\.').
function Find-WingetVersion($id, $regex) {
  $lines = winget show --id $id --versions --accept-source-agreements 2>$null
  $versions = @($lines | ForEach-Object { $_.Trim() } | Where-Object { $_ -match $regex })
  if ($versions.Count -eq 0) { return $null }
  return ($versions | Sort-Object { try { [version]($_ -replace '[^0-9.].*$', '') } catch { [version]'0.0' } } -Descending)[0]
}

function Install-Winget($id, $version, $extraArgs) {
  if (-not $Install) { return $false }
  if (-not (Get-Command winget -ErrorAction SilentlyContinue)) {
    Write-Warning "winget not found. Install 'App Installer' from the Microsoft Store, then re-run."
    return $false
  }
  $wingetArgs = @('install', '--id', $id, '-e', '--accept-package-agreements', '--accept-source-agreements')
  if ($version) { $wingetArgs += @('--version', $version) }
  if ($extraArgs) { $wingetArgs += $extraArgs }
  Write-Host "  -> winget $($wingetArgs -join ' ')" -ForegroundColor Cyan
  & winget @wingetArgs
  Update-SessionPath
  return $true
}

# Generic "check a CLI tool, install if missing" helper.
function Ensure-Tool($name, $exe, [string[]]$verArgs, $pattern, $wingetId, $min, $maxExclusive, $versionRegex) {
  $ver = Get-CommandVersion $exe $verArgs $pattern
  if ($ver -and ($ver -eq 'unknown' -or (Test-VersionRange $ver $min $maxExclusive))) {
    Add-Result $name 'OK' $ver; return
  }
  $why = if ($ver) { "found $ver, need >= $min" + $(if ($maxExclusive) { " and < $maxExclusive" } else { '' }) } else { 'missing' }
  $pin = $null
  if ($versionRegex) { $pin = Find-WingetVersion $wingetId $versionRegex }
  if (Install-Winget $wingetId $pin $null) {
    $ver = Get-CommandVersion $exe $verArgs $pattern
    if ($ver) { Add-Result $name 'INSTALLED' $ver } else { Add-Result $name 'CHECK' 'installed, open a new terminal to pick up PATH' }
  } else {
    Add-Result $name 'MISSING' "$why  (winget id: $wingetId)"
  }
}

Write-Host "Pandaria ARPG tool check  (repo: $RepoRoot)" -ForegroundColor Green
if ($Install -and -not (Test-Admin)) {
  Write-Warning 'Run from an elevated PowerShell (Run as administrator) so installers can finish without prompts.'
}

# --- Command-line tools ---------------------------------------------------------
Ensure-Tool 'Git'      'git'    @('--version') '(\d+\.\d+\.\d+)'  'Git.Git'              '2.30'   $null $null
Ensure-Tool 'CMake'    'cmake'  @('--version') '(\d+\.\d+\.\d+)'  'Kitware.CMake'        '3.27.2' $null $null
# Python answers to different names depending on how it was installed (py launcher, python3, python).
$pyExe = $null
foreach ($cand in @('py', 'python3', 'python')) {
  if (Get-CommandVersion $cand @('--version') '(\d+\.\d+\.\d+)') { $pyExe = $cand; break }
}
if ($pyExe) { Ensure-Tool 'Python 3' $pyExe @('--version') '(\d+\.\d+\.\d+)' 'Python.Python.3.12' '3.10' $null $null }
else { Ensure-Tool 'Python 3' 'python' @('--version') '(\d+\.\d+\.\d+)' 'Python.Python.3.12' '3.10' $null $null }
Ensure-Tool 'VS Code'  'code'   @('--version') '(\d+\.\d+\.\d+)'  'Microsoft.VisualStudioCode' '1.90' $null $null
Ensure-Tool 'Node.js (optional, for OpenSpec CLI)' 'node' @('--version') '(\d+\.\d+\.\d+)' 'OpenJS.NodeJS.LTS' '20.19.0' $null $null

# --- MSVC (Build Tools 2022 with the C++ workload) ------------------------------------
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$vcPath = $null
if (Test-Path $vswhere) {
  $vcPath = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
}
if ($vcPath) {
  Add-Result 'MSVC C++ build tools' 'OK' $vcPath
} elseif (Install-Winget 'Microsoft.VisualStudio.2022.BuildTools' $null @('--override', '--wait --passive --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended')) {
  Add-Result 'MSVC C++ build tools' 'INSTALLED' 'Build Tools 2022 + Desktop development with C++'
} else {
  Add-Result 'MSVC C++ build tools' 'MISSING' 'Build Tools for Visual Studio 2022, workload "Desktop development with C++"'
}

# --- MySQL 8.0 (the repo supports 5.7 and 8.0-8.1, not 8.4/9.x) ----------------------
$mysqlVer = Get-CommandVersion 'mysql' @('--version') '(\d+\.\d+\.\d+)'
if (-not $mysqlVer) {
  $mysqlExe = Get-ChildItem "$env:ProgramFiles\MySQL" -Recurse -Filter mysql.exe -ErrorAction SilentlyContinue | Select-Object -First 1
  if ($mysqlExe) { $mysqlVer = ([regex]::Match((& $mysqlExe.FullName --version), '(\d+\.\d+\.\d+)')).Groups[1].Value }
}
if ($mysqlVer -and (Test-VersionRange $mysqlVer '8.0.0' '8.2.0')) {
  Add-Result 'MySQL' 'OK' $mysqlVer
} else {
  $pin = Find-WingetVersion 'Oracle.MySQL' '^8\.0\.'
  if ($pin -and (Install-Winget 'Oracle.MySQL' $pin $null)) {
    Add-Result 'MySQL' 'INSTALLED' "$pin  (run MySQL Configurator to set the root password)"
  } else {
    $found = if ($mysqlVer) { "found $mysqlVer (unsupported)" } else { 'missing' }
    Add-Result 'MySQL' 'MISSING' "$found. Need 8.0.x: https://dev.mysql.com/downloads/installer/ (Archives tab for 8.0)"
  }
}

# --- OpenSSL 3.0-3.1.1, full (with headers), Win64 -----------------------------------
# Several OpenSSL versions can sit side by side (e.g. 4.0 for other work). Pick the first root that is
# 3.0.0-3.1.1 and has headers + import libs; the build points OPENSSL_ROOT_DIR at it (.vscode/settings.json).
$sslRoots = @("$env:ProgramFiles\OpenSSL-Win64-3.0", "$env:ProgramFiles\OpenSSL-Win64", "$env:ProgramFiles\OpenSSL")
$sslVer = $null; $sslHeaders = $null; $sslSeen = @()
foreach ($root in $sslRoots) {
  $exe = Join-Path $root 'bin\openssl.exe'
  if (-not (Test-Path $exe)) { continue }
  $v = ([regex]::Match((& $exe version), '(\d+\.\d+\.\d+)')).Groups[1].Value
  $sslSeen += "$v at $root"
  $hasLib = (Test-Path (Join-Path $root 'lib\VC\x64\MD\libcrypto.lib')) -or (Test-Path (Join-Path $root 'lib\libcrypto.lib'))
  if ((Test-VersionRange $v '3.0.0' '3.1.2') -and (Test-Path (Join-Path $root 'include\openssl\ssl.h')) -and $hasLib) {
    $sslVer = $v; $sslHeaders = $root; break
  }
}
if ($sslVer) {
  Add-Result 'OpenSSL (dev)' 'OK' "$sslVer at $sslHeaders"
} else {
  $sslVer = ($sslSeen -join '; ')
  $pin = Find-WingetVersion 'ShiningLight.OpenSSL.Dev' '^3\.(0\.|1\.[01]$)'
  if ($pin -and (Install-Winget 'ShiningLight.OpenSSL.Dev' $pin $null)) {
    Add-Result 'OpenSSL (dev)' 'INSTALLED' $pin
  } else {
    $found = if ($sslVer) { "found $sslVer" } else { 'missing' }
    Add-Result 'OpenSSL (dev)' 'MISSING' "$found. Need Win64 OpenSSL 3.0.x or 3.1.0-3.1.1 *full/dev* (not Light, not 3.2+): https://slproweb.com/products/Win32OpenSSL.html"
  }
}

# --- Boost (no reliable winget package; version comes from the repo's install guide) ----
# BOOST_ROOT may be a user env var or only set for CMake in .vscode/settings.json (cmake.configureEnvironment).
$boostRoot = $env:BOOST_ROOT
$vsSettings = Join-Path $RepoRoot '.vscode\settings.json'
if (-not $boostRoot -and (Test-Path $vsSettings)) {
  $m = [regex]::Match((Get-Content $vsSettings -Raw), '"BOOST_ROOT"\s*:\s*"([^"]+)"')
  if ($m.Success) { $boostRoot = $m.Groups[1].Value }
}
if ($boostRoot -and (Test-Path (Join-Path $boostRoot 'boost\version.hpp'))) {
  $hpp = Get-Content (Join-Path $boostRoot 'boost\version.hpp') -Raw
  $lib = ([regex]::Match($hpp, 'BOOST_LIB_VERSION "([^"]+)"')).Groups[1].Value
  # The build links these static, multithreaded, x64 release libs (dep/boost/CMakeLists.txt).
  $missing = @('filesystem', 'program_options', 'iostreams', 'regex', 'locale') | Where-Object {
    -not (Get-ChildItem $boostRoot -Recurse -Depth 1 -Filter "libboost_$_-vc143-mt-x64-$lib.lib" -ErrorAction SilentlyContinue | Select-Object -First 1)
  }
  if ($missing) { Add-Result 'Boost' 'MISSING' "$lib at $boostRoot, but no msvc-14.3 x64 static libs for: $($missing -join ', ')" }
  elseif (-not (Test-VersionRange ($lib -replace '_', '.') '1.78' $null)) { Add-Result 'Boost' 'MISSING' "$lib at $boostRoot, need >= 1.78 (upstream CI uses 1.87)" }
  else { Add-Result 'Boost' 'OK' "$lib at $boostRoot" }
} else {
  Add-Result 'Boost' 'MANUAL' 'If the repo install guide lists Boost: install the prebuilt msvc-14.3 64-bit binaries it names, then set BOOST_ROOT'
}

# --- GUI and modding tools ------------------------------------------------------------
$heidi = Test-Path "$env:ProgramFiles\HeidiSQL\heidisql.exe"
if ($heidi) { Add-Result 'HeidiSQL' 'OK' '' }
elseif (Install-Winget 'HeidiSQL.HeidiSQL' $null $null) { Add-Result 'HeidiSQL' 'INSTALLED' '' }
else { Add-Result 'HeidiSQL (optional)' 'SKIPPED' 'any MySQL client works (DBeaver is fine)' }

$toolsDir = Join-Path $RepoRoot 'arpg\tools\bin'
foreach ($t in @(
    @{ Name = "Ladik's MPQ Editor"; File = 'MPQEditor.exe'; Url = 'http://www.zezula.net (mirror: https://www.hiveworkshop.com/threads/ladiks-mpq-editor.249562/)' },
    @{ Name = 'WDBX Editor';        File = 'WDBX Editor.exe'; Url = 'https://github.com/WowDevTools/WDBXEditor/releases' })) {
  $hit = Get-ChildItem $toolsDir -Recurse -Filter $t.File -ErrorAction SilentlyContinue | Select-Object -First 1
  if ($hit) { Add-Result $t.Name 'OK' $hit.FullName }
  else { Add-Result $t.Name 'MANUAL' "download from $($t.Url) and unzip into arpg\tools\bin\" }
}

# --- VS Code extensions ----------------------------------------------------------------
$wantedExt = @('ms-vscode.cpptools', 'ms-vscode.cmake-tools', 'sumneko.lua', 'anthropic.claude-code')
if (Get-Command code -ErrorAction SilentlyContinue) {
  $haveExt = @(code --list-extensions 2>$null) | ForEach-Object { $_.ToLower() }
  foreach ($e in $wantedExt) {
    if ($haveExt -contains $e) { Add-Result "VS Code ext: $e" 'OK' '' ; continue }
    if ($Install) { code --install-extension $e --force | Out-Null; Add-Result "VS Code ext: $e" 'INSTALLED' '' }
    else { Add-Result "VS Code ext: $e" 'MISSING' '' }
  }
}

# --- OpenSpec CLI (optional; the markdown workflow works without it) ---------------------
if (Get-Command openspec -ErrorAction SilentlyContinue) {
  Add-Result 'OpenSpec CLI (optional)' 'OK' ((openspec --version) -join '')
} elseif ($Install -and (Get-Command npm -ErrorAction SilentlyContinue)) {
  npm install -g '@fission-ai/openspec@latest' | Out-Null
  Add-Result 'OpenSpec CLI (optional)' 'INSTALLED' ''
} else {
  Add-Result 'OpenSpec CLI (optional)' 'MISSING' 'npm install -g @fission-ai/openspec@latest'
}

# --- WoW client -------------------------------------------------------------------------
if ($WowPath) {
  # Repacks rename the exe (e.g. Wow-SF-64.exe); take Wow.exe if present, else any Wow*.exe.
  $exe = Get-ChildItem $WowPath -Filter 'Wow*.exe' -ErrorAction SilentlyContinue |
    Sort-Object @{ Expression = { $_.Name -ne 'Wow.exe' } }, Name | Select-Object -First 1
  $enUS = Join-Path $WowPath 'Data\enUS'
  if ($exe -and (Test-Path $enUS)) {
    $fv = $exe.VersionInfo.FileVersion
    $ok = $fv -match '18414'
    Add-Result 'WoW client' $(if ($ok) { 'OK' } else { 'CHECK' }) "$($exe.Name) $fv, enUS data present$(if (-not $ok) { ' - expected build 18414' })"
  } else {
    Add-Result 'WoW client' 'MISSING' "no Wow*.exe + Data\enUS under $WowPath"
  }
} else {
  Add-Result 'WoW client' 'SKIPPED' 'pass -WowPath or set ARPG_WOW_PATH'
}

# --- Defender exclusion --------------------------------------------------------------------
if ($AddDefenderExclusion) {
  if (Test-Admin) {
    Add-MpPreference -ExclusionPath $RepoRoot
    Add-Result 'Defender exclusion' 'OK' $RepoRoot
  } else {
    Add-Result 'Defender exclusion' 'MISSING' 'needs an elevated PowerShell'
  }
}

# --- Report --------------------------------------------------------------------------------
$results | Format-Table -AutoSize -Wrap
$bad = @($results | Where-Object { $_.Status -in @('MISSING') })
$manual = @($results | Where-Object { $_.Status -in @('MANUAL', 'CHECK') })
if ($bad.Count -eq 0 -and $manual.Count -eq 0) {
  Write-Host 'All set.' -ForegroundColor Green; exit 0
}
if ($bad.Count -gt 0 -and -not $Install) { Write-Host "Re-run with -Install to fix the MISSING rows." -ForegroundColor Yellow }
if ($manual.Count -gt 0) { Write-Host 'MANUAL/CHECK rows need a quick look by hand (see Detail).' -ForegroundColor Yellow }
exit [int]($bad.Count -gt 0)
