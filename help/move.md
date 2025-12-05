# MOVE

## Name
   move [variable] -- Move civilians from one sector to another. 

## Syntax
   move <origin sector x,y> <direction list> <number of population>

## Description
   This command is used to move population from one sector to an adjacent
sector. The player enters the origination sector, target sector, and the
amount of population to move. If a destination sector is occupied by 
aliens, the combat strengths of the player (attacker) and alien (defender) 
are computed. The assault is evaluated and casualties are removed from 
each sector. See 'combat' for more details.

   If no population argument is given, all population in the original sector 
will be moved. If a negative population is specified, the all population,
except for the amount specified, will move. 

  The direction orders the group to move in one of 8 compass directions:

				7   8   9
				 \  |  /
				  \ | /
 			       4 -- . -- 6
				  / | \
				 /  |  \
				1   2   3
The population will continue to move as long as a valid move is available.
Movement stops when an illegal move, lost/draw combat, or lack of
action points occurs. For example, 'move 10,4 6698 -2' will leave two
peopl behind in sector 10,4 move the rest to 11,4, leave 2, move the rest
to 12,4, leave 2, move the rest to 11,3, leave 2, finish the move to 11,2.
This move can be executed until the availability of action points (see below).

   This command cannot be used on planets that have been enslaved 
to another race.

   Moving civilians uses up more action points than moving troops.

+-----------------------+---+----+-----+------+------+-----+------+------+
| AP's			| 1 |  2 |  3  |   4  |   5  |  6  |   7  |   8  |
+-----------------------+---+----+-----+------+------+-----+------+------+
| Civilians	Min	| 1 |  2 |  7  |   20 |   54 | 148 |  403 | 1096 |
| 		Max	| * |  6 | 19  |   53 |  147 | 402 | 1095 |      |
+-----------------------+---+----+-----+------+------+-----+------+------+
| Military	Min	| 1 |  9 | 99  |  999 | 9999 |     |      |      |
|		MAx	| 8 | 98 | 998 | 9998 |   -  |     |      |      
+-----------------------+---+----+-----+------+------+-----+------+------+
 * NOTE: moving minsex cost only 1 ap.

For attacks (against alien occupied positions) add 1 to the action point cost.

## See Also
 combat, enslave, deploy
