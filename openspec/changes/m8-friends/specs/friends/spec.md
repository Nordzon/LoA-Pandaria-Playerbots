## ADDED Requirements

### Requirement: Self-service sign-up
The sign-up page SHALL create a normal player account from a name and a password typed twice, without GM rights, and SHALL
refuse invalid names or passwords, mismatched passwords, reserved names, a wrong invite code (when one is set), names that
already exist, attempts above the rate limit and accounts above the cap. It SHALL never change an existing account.

#### Scenario: A mate signs up and logs in
- WHEN a mate submits name "Mate1" and the same valid password twice
- THEN account MATE1 exists, and the game client logs in with that name and password

#### Scenario: Taken name
- WHEN someone submits the name of an existing account
- THEN the page says the name is taken and the existing account's password is unchanged

### Requirement: Backups before changes
Before the build-and-test script stops the servers, it SHALL back up the auth and characters databases, and it SHALL refuse
to stop them while a real player is online unless forced.

#### Scenario: The owner is playing
- WHEN rebuild-and-test runs while the owner's character is online
- THEN it stops with "players online" and nothing is restarted
