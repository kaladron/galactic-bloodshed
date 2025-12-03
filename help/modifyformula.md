# MODIFYFORMULA

## Name
  modify -- modify attributes of a ship being designed at a factory.

## Syntax
  modify <attribute> <value>

## Description

   Once a ship design has been specified for a factory, you may alter
the design attributes to suit your needs. Each attribute is compared to
the 'standard' design and a cost for producing the ship is evaluated
based on these attributes. Specifically, the following attributes may
be altered:

	armor, size, crew, fuel, cargo, destruct, speed, hyperdrive.

along with the special modification commands below.

   The modify command reports the current cost of producing the ship
type at the factory. Basically, the cost of producing the ship is 
determined, after evaluating the total 'advantage' and total
'disadvantage', by
		(cost) = (base cost) * ( 1 + advantage ) / ( 1 + disadvantage ).
An advantage/disadvantage is computed, for each modifiable attribute,
by	( (new value) + 1) / ( (standard value) + 1) - 1.
For each attribute, if this number is positive it is considered to be an
'advantage' and if it is negative, the absolute value of it is considered
to be a 'disadvantage'.

   All ships built at a factory may have hyper-space jump capabilities
by installing 'hyperdrive'. Having this is assumed to be an advantage
of 1.

   Each point of armor above the standard amount is considered as an
advantage. Each point below the standard is a disadvantage.

   The maximum value by which an attribute may be modified to is dependent
on the technology the factory used to construct the ship by
	(max value) = MAX(1 ,tech/100) * (standard value)

   A base mass is also computed, the mass of the ship with nothing loaded 
on board. Each point of armor has a mass of 1 while each gun and point 
of size has a mass of 1/10.

Special systems:
	laser - A player can mount/dismount combat lasers by using
		'modify laser'

	CEWs  - A player can modify 'confined energy weapon' parameters.
		A CEW is assumed to be active if it has strength. To
		specify a CEW strength do 'mod cew strength <strength>'.
		To modify the CEW range do 'mod cew range <range>'.

	primary - A player may modify the primary gun caliber and strength with
		'mod primary strength <new strength>' and
		'mod primary caliber <light/medium/heavy>'.
		A particular ship type have a limit to the maximum gun caliber.
		Light guns are considered a disadvantage of 0.5, while
		heavy guns are considered and advantage of 0.5.

	secondary - Same as for primary except the word 'secondary' is used.
		Some ships may not have secondary gun systems.

## See Also
	make, build, lasers, cew
