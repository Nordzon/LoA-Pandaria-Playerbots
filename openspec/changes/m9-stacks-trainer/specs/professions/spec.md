## ADDED Requirements
### Requirement: No accidental abandon
The client's abandon of a primary profession SHALL be refused while `Arpg.Professions.AllowClientAbandon` is 0, with a chat
line naming `.professions abandon <name>`; that command SHALL remove the profession.

#### Scenario: Client abandon refused
- WHEN the client asks to abandon Herbalism
- THEN the character keeps Herbalism and the event "profession abandon refused herbalism" is recorded

#### Scenario: Deliberate abandon
- WHEN `.professions abandon mining` is used
- THEN Mining is removed

### Requirement: Profession changes are logged
Every primary profession learned or removed SHALL be written to Server.log for real players ("[ARPG] <name> profession
learned|removed <profession>") and recorded as a test event.

#### Scenario: Logged removal
- WHEN a profession is removed by any path
- THEN the event "profession removed <name>" is recorded
