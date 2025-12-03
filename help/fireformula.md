# FIREFORMULA

NOTE: This file includes all documented formulas related to fire command
     Some of them are old and not true anymore.

## Name
  [1] fire    -- shoot a ship at another ship

## Syntax
   fire <from> <to> [<strength>]

## Description

   The fire commands allow players to attack each others ships/planets
with available guns. Fire attempts to use stockpiled destructive capacity 
from one place, to give damage to another.

   The maximum number of guns for a ship is prortional to the maximum number
of guns which the ship was originally constructed with, and the damage level
of the ship. As the ship accumulates more damage the number of guns
available decreases.

   Ship-to-ship combat is the 'essence' of the Galactic Bloodshed
combat system. The program will compute the 'hit probability'.
This designates the chances that a particular gun will actually
hit the target ship with the potential of doing damage to it.
When a player designates a 'salvo' of so many guns, each gun is
evaluated for hit/miss, the sum of all hits is then used to
determine the results of the salvo.

   If a ship is firing on another ship, and no strength is given, the salvo
strength specified by the player (see 'order salvo <strength>') is used.

   The hit probability depends on several factors

  1) The technology of firing ship. The higher the technology,
	the more accurate the firing vessels shots are.

  2) The size of the target vessel. Larger targets are easier to hit.

  3) The speed of the firing and target ships. It is assumed that
	the motions of the ships can affect the chances of hitting
	the target.

  4) Whether or not 'evasive maneuvers' have been order for
	the ship in question.

   The hit probability can be found from the 'tactical' display
and the formula used to determine the hit probability is

	probability = 100 / ( 1 + range / 50% range).

When the range = 50% range the chances of hitting the target it
50%. The 50% range, or 'effective radius' of a ships guns is
determined from the above factors as

	50% range = log10(1+technology of ship) * 80 * body^(1/3)

			* 72 / (2 + fev) / ( 2 + fev) / (18 + fspd + tspd)

where

		tech = the technology of the firing vessel
			or of the race (for planet firing)

		body = target size

		fev = 0 or 1 for whether or not firing vessel
				has evasive maneuvers

		tev = defending ship evasive maneuvers

		fspd = firing ship speed

		tspd = target ship speed.

Note, that a ship must actually be 'going somewhere' (plotted
move in 'order') for the evasive and speed factors to be
taken into effect (otherwise they are set to zero).

   After the hit probability is determined each firing gun is
evaluated for hit/miss. Each hit is then checked to for penetration.
The probability of a hit penetrating depends on the ships armor:

						armor
		penetration probability = factor

The armor effectiveness factor is depends on the technology of both
the firing and target ship:

		  2 	         firing ship technology + 1
	factor = -- arctan( 5 * ---------------------------- )
		 PI	         target ship technology + 1

The structural damage to the ship is then evaluated by the formula

		% damage = 2 * caliber * penetrated hits / body.

Bigger ships (represented by body) can absorb more hits less ensuing 
damage than little ships. Also notice that smaller ships, although
harder to hit, are easier to damage *when* hit. 

			body = SQRT(size/10)

  Special rule: Every 5 hits in a salvo against a target reduces its
armor value for that attack by 1.

   After evaluating for structural damage, the program then checks for
'critical hits'. EACH hit is evaluated, whether or not any real
structural damage has occured on the target vessel. The chances
of a critical hit depends on the size of the target ship.
Specifically

		critical hit probability = size/caliber : 1.

Smaller ships are more likely to suffer damage from critical hits
than big ships. As an example, a Battleship with a size of 30
and has a 30:1 odds of taking a critical hit from each pentration.
A fighter group with a size of 10 and no armor has a 10:1 odds against 
such a hit. Each critical hit is then evaluated. A random amount of damage
between 1 and 100% is assessed for each hit, in addition to the 
normal structural damage.

Defensive and Offensive Combat Options

   A player may set up his own defense/response networks.
Specifically, in the 'order' command, a player may designate
his ships to retaliate (default) if they are attacked.
Whenever you fire at a ship, if you have done any damage to
his ship, your ship will return fire immediately, thereby
damaging the attacker. The retaliate option can be toggled on
and off, depending on the players intentions. A player may
also designate his ships to defend other ships, so that is
your ship is attacked and damaged, all other ships designated
to 'protect' the ship will fire back at the attacking vessel.

   Ships may also be designated to be on 'plantary defense'
alert. If the planet is attacked, all ships defending will
retaliate against the intruder.

## See Also
  bombard, defend, tactical, order
