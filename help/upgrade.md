# UPGRADE

## Name
   upgrade [0] -- upgrade attributes of a ship.

## Syntax
   upgrade <attribute> <value>

## Description
   This command works just like 'modify' except that it is performed on
ships (non-factories) which have already been built. The player must
have enough resources on board the ship or on planet if it is landed 
to make the upgrade. The cost for an upgrade is 2 * (the difference 
between the ship with the alteration and the old value of the ship).

Upgrade is only used to increase a parameter (you may not decrease
a parameter).

## See Also
	modify, make, build
