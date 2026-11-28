# Tasks: m9-stacks-trainer

## 1. Stacks
- [x] 1.1 `Items/Stacks` (ApplyStacks at startup + `.arpg reload`, stock sizes remembered), settings `Arpg.Stacks.Max`
- [x] 1.2 SQL `2026_11_28_02_arpg_stacks.sql`: Item-sparse hotfix rows for stack 2..998 items
- [x] 1.3 `.arpg stacks info|apply|give`, GM test 62

## 2. Companion class trainer
- [x] 2.1 "Class trainer" page: reset spec / talents, Dual Specialization (cost setting), Proving Grounds
- [x] 2.2 "Learn a new profession" (Apprentice of any missing primary, also a bare skill without its spell)

## 3. Profession guardrails
- [x] 3.1 Hooks `ArpgHooks::SkillChanged` (Player::SetSkill) + `UnlearnSkillAllowed` (HandleUnlearnSkillOpcode)
- [x] 3.2 `Character/ProfessionGuard`: refuse the client abandon, log learn/remove, `.professions abandon <name>`
- [x] 3.3 `.arpg test unlearnskill|abandon`, GM test 62

## 4. Camera
- [x] 4.1 ArpgUI `Camera.lua` (cameraDistanceMax 50, factor 4, `/arpgcam`)

## 5. Owner checks
- [ ] 5.1 OWNER-CHECKS C: stacks to 999 in game, companion trainer page, learn a 4th profession, camera zoom
