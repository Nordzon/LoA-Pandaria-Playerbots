-- mod_arpg m9-champion-camps: creatures that never become Champions (on top of the code rules: menu/quest NPCs, scripted
-- story NPCs, monsters friendly to the player, critters, rares, bosses). Add any entry the owner reports. Idempotent.
CREATE TABLE IF NOT EXISTS `arpg_champion_exclude` (
  `entry`   INT UNSIGNED NOT NULL,
  `comment` VARCHAR(100) NOT NULL DEFAULT '',
  PRIMARY KEY (`entry`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

INSERT IGNORE INTO `arpg_champion_exclude` (`entry`, `comment`) VALUES
  (53714, 'Training Target (Wandering Isle)'),
  (54586, 'Huojin Trainee (sparring, Wandering Isle)'),
  (65470, 'Huojin Trainee (sparring, Wandering Isle)'),
  (54587, 'Tushui Trainee (sparring, Wandering Isle)'),
  (54611, 'Jaomin Ro (quest fight, Wandering Isle)'),
  (57750, 'Jaomin Ro (quest fight, Wandering Isle)'),
  (60895, 'Alliance Priest (Wreck of the Skyseeker)');
