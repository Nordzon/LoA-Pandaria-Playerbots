-- mod_arpg m2 task 4.3: tuning of effect affixes (procs, heals, auras). Read at startup and by .arpg reload. Idempotent.
--   spell_id     existing spell used for the combat log line / visual, the root (Frost Nova) or the DoT aura; 0 = none
--   hit_pct      proc damage as % of the hit that triggered it (DoTs: total over the duration)
--   radius       area procs: yards around the target (Fire Nova, Frost Nova) or chain jump range (Chain Lightning)
--   max_targets  area/chain procs: most units hit
--   duration_ms  DoTs: length; Phoenix/Warding: unused
--   cooldown_ms  per-character cooldown (Phoenix); 0 = none
--   school_mask  1 physical, 4 fire, 8 nature, 16 frost, 32 shadow, 64 arcane
CREATE TABLE IF NOT EXISTS `arpg_affix_effect` (
  `effect` VARCHAR(32) NOT NULL, `spell_id` INT UNSIGNED NOT NULL DEFAULT 0, `hit_pct` TINYINT UNSIGNED NOT NULL DEFAULT 0,
  `radius` FLOAT NOT NULL DEFAULT 0, `max_targets` TINYINT UNSIGNED NOT NULL DEFAULT 1, `duration_ms` INT UNSIGNED NOT NULL DEFAULT 0,
  `cooldown_ms` INT UNSIGNED NOT NULL DEFAULT 0, `school_mask` INT UNSIGNED NOT NULL DEFAULT 1,
  PRIMARY KEY (`effect`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='mod_arpg effect affix tuning';

REPLACE INTO `arpg_affix_effect` VALUES
('proc_fire_nova',    1535, 50,  8, 10, 0,     0,      4),
('proc_chain_light',   421, 60, 10,  3, 0,     0,      8),
('proc_frost_nova',    122, 30,  8, 10, 0,     0,     16),
('proc_poison',       2818, 100, 0,  1, 8000,  0,      8),
('proc_bleed',        1943, 80,  0,  1, 6000,  0,      1),
('proc_struck_chain',  421, 60, 10,  3, 0,     0,      8),
('leech_pct',        15290, 0,   0,  1, 0,     0,      32),
('phoenix_heal_pct',     0, 0,   0,  1, 0,     120000, 4),
('move_speed_pct',   13889, 0,   0,  1, 0,     0,      1),
('cast_speed_pct',   44605, 0,   0,  1, 0,     0,      1);
