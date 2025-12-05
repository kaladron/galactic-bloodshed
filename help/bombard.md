# BOMBARD

## Name
   bombard [1] -- fire a ship's weapons at a planet

## Syntax
   bombard <ship #> [ <sector x,y> [<strength>] ]

## Description
    This command is used to damage or destroy sectors on a planet 
from an orbiting or landed ship. If the ship is landed, the 
specified sector must be adjacent to the sector the ship 
occupies. If the sector to be hit is not specified, a random one 
will be computed by the server. If the strength of the attack is 
not specified, the ship will use the maximum strength possible 
(see the fire command for a discussion of this calculation). If 
a strength greater than this computed number is specified in the 
bombard command, the player will be notified that the ships guns 
have been reset to fire with the maximum possible strength. A 
planet can not be attacked from orbit if it still has 
operational Planetary Defense Networks (ship n).
    The damage from a bombard will be most intense in the targeted 
sector, but surrounding sectors may also be damaged. If the 
damage is severe and the sector type had previously been altered 
(plating or terraforming for example), there is a chance the 
sector may mutate back to its natural state. There is also a 
chance that the sector will be wasted, killing all civs, leaving 
a random number of troops, crippling the sector, and increasing 
the toxicity of the planet. If the sector is not wasted, normal 
damage will be inflicted.
    A planet will retaliate, if possible, using its planetary guns 
if at least one sector has been wasted in an attack. Any ships 
which are in orbit around the planet and have been ordered to 
protect the planet (using the order command) will retaliate 
against the bombarding ship. No retaliation, planetary or ship, 
will be triggered if the bombarding ship is a Mech (R). 
However, note that Mechs must be landed on the planet to 
initiate an attack and, thus, may only attack adjacent sectors.

## Example
bombard #985            (bombard a random sector using the default gun 
                            strength)
bombard #1042 10,3 5    (bombard sector 10,3 at a strength of 5)

## See Also
     combat, fire
