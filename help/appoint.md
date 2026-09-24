# APPOINT

## Name
   appoint [0] -- initialize governor position and assign a password

## Syntax
   appoint [<governor #>] <password>

## Description
    This command initializes and assigns a password for a new governor for
your race. If a governor number (>= 2) is omitted, the lowest available
unassigned governor number is automatically selected. Use the governors
command to check the status of your appointed governors. If a governor slot
is currently appointed and you wish to reuse that number, use the revoke
command to remove the governor first.

## Example
appoint blahblah (appoint a new governor to the next available slot with
                  password 'blahblah')
appoint 2 blahblah (appoint a new governor to position 2 with password
                    'blahblah')

## See Also
	governors, revoke, grant
