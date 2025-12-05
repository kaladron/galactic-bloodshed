# SPEED

   Speed reflects ship's ability to move through the space. The bigger
speed the ship has the faster it goes. Usually, the speed is one critical
factor when you design your ships. Watching a slow ship to reach its
destination is frustrating.

   The distance covered with each speed in ONE UPDATE can be calculated 
with a following formula:

	distance = SPEED * MOVEMENT LEVEL

where:
	SPEED corresponds to ship's speed, that is if the speed of the ship
is 2 the SPEED is 2
	MOVEMENT LEVEL depends from where your ship starts its move. This
reflects the fact that if you maneuver near planets you will not want to
use the cruising speed of the universe (and hit the planet). MOVEMENT
LEVEL is given by: 

	universe level 	= 1200
	system level 	=  300
	planet level	=  150

Note: Damaged ships move slower proportional to their damage.

-segment-
To calculate the distance ship moves in ONE SEGMENT you need to divide
MOVEMENT LEVEL by number of segments.
Example:
for 3 segments per update, MOVEMENT LEVEL is 400, 100 or 50
for 4 segments per update, MOVEMENT LEVEL is 300, 75, or 37.5

  The speed of the ship has also an effect on other things such as firing
and fuel usage. 

## See Also
	fire, fuel
