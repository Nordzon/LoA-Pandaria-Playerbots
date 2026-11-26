<#
.SYNOPSIS
  Writes authserver.conf, worldserver.conf and playerbots.conf next to the server binaries,
  from the repo's .conf.dist files plus our overrides (DB login, port, data dir, SOAP, bots off).

.DESCRIPTION
  Re-run after an upstream sync that changes a .conf.dist, or to reset the confs. Existing confs are
  backed up to <name>.conf.bak first. DB user/password/host/port come from arpg\scripts\mysql.local.cnf.
  Every setting it changes is listed in $overrides below; anything else keeps the upstream default.

.EXAMPLE
  powershell -ExecutionPolicy Bypass -File arpg\scripts\write-confs.ps1
  powershell -ExecutionPolicy Bypass -File arpg\scripts\write-confs.ps1 -DataDir D:/WoWMoPProject/ServerData/data
#>
[CmdletBinding()]
param(
  [string]$BinDir,
  [string]$DataDir = 'D:/WoWMoPProject/ServerData/data',
  [string]$LogsDir = 'D:/WoWMoPProject/ServerData/logs',
  [ValidateSet('on', 'off')][string]$Bots = 'off',   # off for ARPG feature testing; on for play (needs mmaps)
  [int]$BotCount = 500,                              # SPEC 4.10 / owner 2026-11-26: 500 (was 300)
  [double]$XpRate = 1.5,                             # SPEC 2: slightly faster levelling (kill/quest/explore/gather)
  [int]$MaxProfessions = 11,                         # SPEC 4.8 (m1b): all 11 primary professions on one character
  # "Condensed experience" (SPEC 2, owner 2026-11-22; analysis in docs/BALANCE.md)
  [double]$MoveSpeed = 1.15,                         # players' base speed incl. mounts (not creatures)
  [double]$DropGreenBlue = 2.0,                      # Rate.Drop.Item.Uncommon / .Rare
  [double]$DropEpic = 1.5,                           # Rate.Drop.Item.Epic
  [double]$DropMoney = 1.5,                          # Rate.Drop.Money
  [int]$SkillGain = 2,                               # SkillGain.Gathering / .Crafting: points per skill-up
  [double]$GatherXpRate = 3.0                        # Rate.XP.Gather (overrides XpRate for gathering)
)
function Num([double]$v) { $v.ToString([cultureinfo]::InvariantCulture) }
$ErrorActionPreference = 'Stop'
$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
if (-not $BinDir) { $BinDir = Join-Path $RepoRoot 'build\bin\RelWithDebInfo' }
if (-not (Test-Path $BinDir)) { throw "Binary folder not found: $BinDir (build first)" }

# --- DB login from mysql.local.cnf ---
$cnfPath = Join-Path $PSScriptRoot 'mysql.local.cnf'
if (-not (Test-Path $cnfPath)) { throw "Missing $cnfPath (copy mysql.local.cnf.example and fill it in)" }
$cnf = @{}
foreach ($line in Get-Content $cnfPath) {
  if ($line -match '^\s*([a-z_]+)\s*=\s*(.*?)\s*$') { $cnf[$Matches[1]] = $Matches[2] }
}
foreach ($k in 'user', 'password', 'host', 'port') { if (-not $cnf[$k]) { throw "mysql.local.cnf has no '$k'" } }
function DbInfo($db) { '"{0};{1};{2};{3};{4}"' -f $cnf.host, $cnf.port, $cnf.user, $cnf.password, $db }

New-Item -ItemType Directory -Force $LogsDir | Out-Null

$overrides = @{
  'authserver.conf' = @{
    dist = 'src\server\authserver\authserver.conf.dist'
    set  = [ordered]@{
      'LoginDatabaseInfo' = (DbInfo 'auth')
      'LogsDir'           = "`"$LogsDir`""
      'BindIP'            = '"127.0.0.1"'
    }
  }
  'worldserver.conf' = @{
    dist = 'src\server\worldserver\worldserver.conf.dist'
    set  = [ordered]@{
      'LoginDatabaseInfo'      = (DbInfo 'auth')
      'WorldDatabaseInfo'      = (DbInfo 'world')
      'CharacterDatabaseInfo'  = (DbInfo 'characters')
      'PlayerbotsDatabaseInfo' = (DbInfo 'playerbots')
      'DataDir'                = "`"$DataDir`""
      'LogsDir'                = "`"$LogsDir`""
      'BindIP'                 = '"127.0.0.1"'
      'SOAP.Enabled'           = '1'
      'SOAP.IP'                = '"127.0.0.1"'
      'SOAP.Port'              = '7878'
      'Rate.XP.Kill'           = $XpRate.ToString([cultureinfo]::InvariantCulture)
      'Rate.XP.Quest'          = $XpRate.ToString([cultureinfo]::InvariantCulture)
      'Rate.XP.Explore'        = $XpRate.ToString([cultureinfo]::InvariantCulture)
      'Rate.XP.Gather'         = (Num $GatherXpRate)
      'MaxPrimaryTradeSkill'   = "$MaxProfessions"   # core reads it without a clamp (World.cpp); conf comment says 0-10
      'Rate.MoveSpeed'         = (Num $MoveSpeed)
      'Rate.Drop.Item.Uncommon' = (Num $DropGreenBlue)
      'Rate.Drop.Item.Rare'    = (Num $DropGreenBlue)
      'Rate.Drop.Item.Epic'    = (Num $DropEpic)
      'Rate.Drop.Money'        = (Num $DropMoney)
      'SkillGain.Crafting'     = "$SkillGain"
      'SkillGain.Gathering'    = "$SkillGain"
      'SkillChance.Yellow'     = '100'                # default 75
      'SkillChance.Green'      = '50'                 # default 25
      'AutoBalance.Enable.Global' = '1'             # m5: dungeon/raid scaling (mod_arpg replaces the player-count curve)
      'Instance.IgnoreRaid'    = '1'                # m5: raids are soloable (SPEC 4.6) - no raid group needed to enter
      # m7 Dungeon Finder (owner 2026-11-26: any dungeon at any level, bots +-3): upstream's #702 switch, no code of ours.
      'LFG.AnyLevel.Enable'    = '1'
      'LFG.AnyLevel.Random'    = '1'                # random 5-man queues too (pick from the entry's whole expansion group)
      'LFG.AnyLevel.Scenarios' = '1'
      # m7 living auction house (owner 2026-11-26: materials first): the core's seller, weighted to materials; mod_arpg
      # rolls affixes on Arpg.AH.AffixShare % of its gear listings (ArpgHooks::AuctionBotItem).
      'AuctionHouseBot.Seller.Enabled'    = '1'
      'AuctionHouseBot.Class.TradeGood'   = '12'
      'AuctionHouseBot.Class.Consumable'  = '8'
      'AuctionHouseBot.Class.Gem'         = '6'
      'AuctionHouseBot.Class.Recipe'      = '6'
      'AuctionHouseBot.Class.Glyph'       = '5'
      'AuctionHouseBot.Class.Weapon'      = '3'
      'AuctionHouseBot.Class.Armor'       = '3'
      'AuctionHouseBot.Class.Container'   = '2'
    }
  }
  'playerbots.conf' = @{
    dist = 'modules\mod_playerbots\config\playerbots.conf.dist'
    set  = [ordered]@{
      'AiPlayerbot.Enabled'       = $(if ($Bots -eq 'on') { '1' } else { '0' })
      'AiPlayerbot.MinRandomBots' = "$BotCount"
      'AiPlayerbot.MaxRandomBots' = "$BotCount"
      'AiPlayerbot.LfgAnyLevel.Enable'      = '1'   # m7: random bots fill an any-level queue
      'AiPlayerbot.LfgAnyLevel.LevelWindow' = '3'   # within +-3 levels of the player (SPEC 4.10)
    }
  }
}

foreach ($name in $overrides.Keys) {
  $o = $overrides[$name]
  $text = Get-Content (Join-Path $RepoRoot $o.dist) -Raw
  foreach ($key in $o.set.Keys) {
    $pattern = '(?m)^(' + [regex]::Escape($key) + '\s*=\s*).*$'
    if (-not [regex]::IsMatch($text, $pattern)) { throw "$name : setting '$key' not found in $($o.dist) (did upstream rename it?)" }
    $value = $o.set[$key]
    $text = [regex]::Replace($text, $pattern, { param($m) $m.Groups[1].Value + $value }, 1)
  }
  $out = Join-Path $BinDir $name
  if (Test-Path $out) { Copy-Item $out "$out.bak" -Force }
  [IO.File]::WriteAllText($out, $text, (New-Object Text.UTF8Encoding($false)))
  Write-Host ("{0,-17} -> {1}  ({2} settings changed)" -f $name, $out, $o.set.Count)
}
# mod_arpg settings: copied only when missing, so tuning edits survive (apply them with .arpg reload).
$arpgDist = Join-Path $RepoRoot 'modules\mod_arpg\conf\arpg.conf.dist'
$arpgConf = Join-Path $BinDir 'arpg.conf'
if ((Test-Path $arpgDist) -and -not (Test-Path $arpgConf)) {
  Copy-Item $arpgDist $arpgConf
  Write-Host ("{0,-17} -> {1}  (new copy of arpg.conf.dist)" -f 'arpg.conf', $arpgConf)
}
Write-Host "DataDir = $DataDir   LogsDir = $LogsDir   DB = $($cnf.user)@$($cnf.host):$($cnf.port)   Bots = $Bots ($BotCount)" -ForegroundColor Green
