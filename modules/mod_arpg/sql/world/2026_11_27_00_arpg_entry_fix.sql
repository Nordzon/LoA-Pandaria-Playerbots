-- mod_arpg fixes from the owner's quality gate (2026-11-26). Idempotent.
-- 1) A MoP creature GUID carries only 20 bits of the entry (max 1,048,575): our creatures at 5,000,000+ / 6,000,000+ /
--    7,000,001 reached the client with a cut entry, so it showed nothing (invisible companion and Superunique, no tag).
--    They now live at 400,000+ (Champion copies), 700,000+ (Unique copies) and 1,000,001 (companion); the old rows go.
DELETE FROM `creature_template_model` WHERE `CreatureID` = 7000001;
DELETE FROM `npc_vendor` WHERE `entry` = 7000001;
DELETE FROM `creature_template` WHERE `entry` = 7000001;

-- 2) Potions: the client's "Use:" line is the stock spell text (a client file); the description line says what ours do.
UPDATE `item_template` i JOIN `arpg_potion` p ON p.`item_id` = i.`entry`
SET i.`description` = CASE p.`kind`
    WHEN 0 THEN CONCAT('ARPG: heals ', p.`pct`, '% of your maximum health. 10 sec shared cooldown.')
    WHEN 1 THEN CONCAT('ARPG: restores ', p.`pct`, '% of your mana (half of that as energy; rage too). 10 sec shared cooldown.')
    ELSE CONCAT('ARPG: restores ', p.`pct`, '% of your health and mana. 10 sec shared cooldown.') END;
-- New hotfix date so clients that cached the old text read these items again.
UPDATE `hotfix_data` SET `hotfixDate` = '2026-11-27 00:00:00'
WHERE `type` = 2442913102 AND `entry` IN (SELECT `item_id` FROM `arpg_potion`);
