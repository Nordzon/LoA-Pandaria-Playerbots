-- mod_arpg m9-stacks-trainer: stackable items stack to Arpg.Stacks.Max (999). The server raises the stack size in memory
-- (Items/Stacks.cpp, item_template stays stock); these Item-sparse hotfix rows (type 2442913102) make the client ask the
-- server for the item record, which carries the new size. Idempotent: rows are only added where none exist.
INSERT INTO `hotfix_data` (`entry`, `type`, `hotfixDate`)
SELECT it.`entry`, 2442913102, '2026-11-28 12:00:00'
FROM `item_template` it
WHERE it.`stackable` BETWEEN 2 AND 998
  AND NOT EXISTS (SELECT 1 FROM `hotfix_data` h WHERE h.`entry` = it.`entry` AND h.`type` = 2442913102);
