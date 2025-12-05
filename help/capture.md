# CAPTURE

## Name
  capture [1] -- Order population to attempt to seize an alien ship
		landed in the same sector. 

## Syntax
    capture <#shipno> <boarders> [<what>]

## Description
   This command is similar to the 'assault' command and the aggressive
move command. The player designates which landed alien ship he wishes
assault. If the sector where the ship is located is occupied by the player, 
the combat strengths of the player (attacker) and alien (defender) are
computed.  The combat strengths depend on number of beings, fighting
ability, sector terrain and technology of the races as well as ship sizes 
morale and armor. The assault is evaluated and casualties are removed 
from each defender and attacker.

   A player may attack with as many population as the sector contains.
If the number of boarders isn't specified, all occupants of the sector
will assault the ship.

	Defense strengths of terrain:
		land  = 1
		sea   = 1
		mount = 3    wasted = 0
		gas   = 2
		ice   = 2
		forest= 3
		desert= 2
		plate = 4

   This command cannot be used on planets that have been enslaved to another
player.

   The player can specify whether he/she will use civilians or military for
the assault. Specify "civ" or "mil" for <what>, the default is military.

## See Also
    assault, enslave, morale
