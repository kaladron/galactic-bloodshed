# APPOINT

## Name
   appoint [0] -- initialize governor position and assign a password

## Syntax
   appoint <governor #> <password>  <rank>

## Description
    This command initializes and assigns a password for a new governor for 
your race. The governor number must be a currently inactive spot. Use the
governors command to check the status of your governor positions. If 
a position is labeled ACTIVE, but is not in use anymore and you wish to 
re-initialize the spot, use the revoke command to free the position before 
executing appoint for a new governor.

 Rank is one of these: 

novice  - can communicate and gets general game info (post, victory..)
private - like novice + general race info (power, colonies..) 
captain - like private + ship and combat commands (build, fire, assault..)
general	- almost as powerful as leader

 If rank is not specified, governor will be private.

## Example
appoint 1 blahblah general (appoint a new general governor to position 1 with 
                        password 'blahblah')

## See Also
	governors, revoke, grant, rank, promote
