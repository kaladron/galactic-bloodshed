# CEW

## Name
   cew [1] -- fire CEWs 'confined energy weapons'

## Syntax
   cew <firing ship> <target ship>

## Description
   CEWs (confined energy weapons) are energy weapons which have an optimal
range. Unlike lasers and guns which have their maximum hit percentage at 
close range, CEWs can be designed to have an optimal range. The optimal
range is set using the modify command during ship design. The hit probability
is independent of the firing/target ship's disposition and has a maximum
at the optimal range R:

	hit probability = 100 * exp(-50*(f-1)^2)

where
		f = (r+1)/(R+1).

At the optimal range r=R, the hit probability is normalized to 100%.

   CEWs also have a strength which is prescribed during the ship modification
procedure. The strength of a CEW is in equivalent destruct units. Each CEW
attack uses 2*destruct fuel units to execute. Unlike normal fire attacks
(lasers and destruct), a CEW attack either hits or misses, according to the
above hit probability formula. If the attack hits, the hits (equal to the
CEW attack strength) are evaluated for penetration and damage according to
the normal damage procedures. If the hit misses, no damage is done.

   Once designed, a CEW range can be modified only upwards.

   CEW require a special technology. Some ships cannot install CEW technology.

Technology limits on functional crystal ranges are proportional to
the amount of damage on the ship. Heaviliy damaged ships may
find their crystals breaking at lower limits.

## See Also
   fire, modify
