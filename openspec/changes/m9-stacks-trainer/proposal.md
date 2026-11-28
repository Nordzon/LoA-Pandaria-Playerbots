# m9-stacks-trainer: 999 stacks, class trainer on the companion, profession guardrails, farther camera

## Why (owner 2026-11-28, playing)
- "Can you make any stackable item actually go to 999. I really detest some of the inventory management."
- "And add a class trainer to the pet."
- "I want the client to allow more zooming out, ideally 2x the current distance."
- Bug: "Went to a profession trainer and learned apprentice herbalism and apprentice jewelcrafting and it looks like my other
  professions were wiped. Can we add some guardrails around this. Can someone continue to the next stages of ALL professions?"

## Scope
- Stacks: every item with a stock stack size of 2..998 stacks to `Arpg.Stacks.Max` (999), in memory (item_template stays
  stock); Item-sparse hotfix rows so the client learns the new size. `.arpg stacks info|apply|give`.
- Companion "Class trainer" page: what a MoP class trainer offers (spells come by themselves at level-up): reset
  specialization / talents (the game's own confirm box), Dual Specialization (`Arpg.Companion.DualSpecCost`), Proving
  Grounds at 90, plus **Learn a new profession** (Apprentice rank, any of the 11; the client's own trainer refuses a third
  profession because it only has two profession slots).
- Profession guardrails: the client's "abandon profession" (CMSG_UNLEARN_SKILL) is refused for primary professions
  (`Arpg.Professions.AllowClientAbandon` 0) with a chat line; `.professions abandon <name>` is the deliberate way. Every
  primary profession learned / removed goes to Server.log ("[ARPG] <char> profession ...") and the test events.
- Camera: ArpgUI `Camera.lua` sets `cameraDistanceMax` 50 and `cameraDistanceMaxFactor` 4 at login; `/arpgcam <factor>`.

## Not in scope
- Finding the exact cause of the lost professions: no server path drops a profession on learning; the log line will show it
  next time (open question in PROGRESS).
