# MORALE

## Concept

   Morale represents the willingness of your race to be successful
in battle. The morale of race depends on its success in colonization
as well as combat situations. As the morale of a race increases,
it becomes more effective against other races in combat.

   Each race starts of with a morale of 0. Each update, each race
loses 10% of its morale, which is good thing if its negative, and
receives 0.1 morale point for each sector. Morale points are cumulative.
Players also receive morale points for capturing alien occupied
sectors and destroying or capturing alien ships:

	+defenders fight	capture an alien occupied sector
	-your races fight	lose a sector in land combat
	+cost	capture/destroy an alien ship
	-cost	lose 	 ship to an alien

	+0.1/update	for each sector 
	-10%/update

   During assault/capture and land combat morale between
opposing forces is compared and the 'morale factor' is computed
which modifies attack/defense strengths. Morale factor is
given by

				 1		1
		morale_factor = -- Arctan(x) + --
				PI		2

where x=(morale difference)/10000.

## See Also
   assault, capture, move, fire
