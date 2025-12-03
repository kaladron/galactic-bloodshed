# ASSAULT

## Name
   assault [1] -- attempt a capture of an enemy ship in space

## Syntax
   assault [#]<boarding ship #> [#]<target ship #> [<boarders> [civ | mil] ]

## Description
    An assault attempts to dock with and capture an alien ship. You 
must specify your attacking ship and the target ship. You may 
also specify the number of units to use in the boarding. If you 
do not specify this number, the maximum number of military units 
on your ship will be used. If you specify a number to board, the 
type of units to use can also be specified, civilian or 
military. If you do not specify this fourth argument, the 
boarders will be military units (see NOTE below, though).
    Both ships that you specify must be in the same scope and must 
be within a minimum docking distance of each other set by the 
deity (the default value is 10). An assault includes a small 
maneuver by the boarding ship and it must have sufficient fuel 
for the move:

            fuel = 0.05 + 0.05 * dist * Sqrt(ship mass)

    Before the actual assault occurs, the defending ship will get a 
chance to fire its defensive weapons. The details about 
defensive fire can be found under the command fire. If neither 
ship is destroyed in this phase, your ship docks with the target 
and the assault will occur. Along with unit casualties, ships 
will be damaged in the assault and may be destroyed. If both 
ships remain and you have eliminated all enemy crew on the 
target ship, you gain control of the ship and your boarding 
party moves in. If you fail to kill the entire crew, your 
assault is repulsed and your units retreat back to the boarding 
ship.
    If you succeed in gaining control of the enemy ship, you get 
morale point equal to the ship's build cost and the former owner 
loses this amount. If you must retreat, the enemy gains morale 
equal to your race's Fight, and you loose this amount. You also 
gain information about the race you are attacking, especially if 
you are successful.

## Note
** If you have no military units aboard your ship and do not 
explicitly specify civilians for the assault, you will receive a 
message that you have no troops to board with. The assault 
command always assumes military units unless you specify the 
number and type 'civ'.
** You can not use a docked ship or a ship in a hanger for the 
boarding ship.
** You can not use pods or terraformers  for boarding with the assault command.

## Example
assault #453 #998           (Assult ship #998 with sh #453 with all 
                                military units onboard)
assault #1202 #309 5 civ    (Assult ship #309 with ship #1202 with 
                                5 civilians)

## See Also
	combat, dock, fire
