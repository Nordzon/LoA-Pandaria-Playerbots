-- mod_arpg m1 (SPEC 4.2): zone quests scale with the player (QuestLevel -1 = "player's level" in the core).
-- Idempotent and reversible: original levels are kept in arpg_quest_level_backup the first time this runs.
-- Revert:  UPDATE quest_template q JOIN arpg_quest_level_backup b ON b.ID = q.ID SET q.QuestLevel = b.QuestLevel;
CREATE TABLE IF NOT EXISTS `arpg_quest_level_backup` (
  `ID` INT UNSIGNED NOT NULL,
  `QuestLevel` SMALLINT NOT NULL,
  PRIMARY KEY (`ID`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='mod_arpg: QuestLevel before quest scaling';

-- Back up once (INSERT IGNORE keeps the first, true original value).
INSERT IGNORE INTO `arpg_quest_level_backup` (`ID`, `QuestLevel`)
SELECT `ID`, `QuestLevel` FROM `quest_template` WHERE `QuestLevel` > 0 AND `QuestSortID` > 0;

-- Zone-sorted quests (QuestSortID > 0 = a zone; negative = class/profession/holiday sorts, left alone).
UPDATE `quest_template` q
JOIN `arpg_quest_level_backup` b ON b.`ID` = q.`ID`
SET q.`QuestLevel` = -1;
