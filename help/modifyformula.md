# MODIFYFORMULA

## Name
   modify [0] -- modify attributes of a ship being designed at a factory.

## Syntax
  modify <attribute> <value>

## Description

   Once a ship design has been specified for a factory, you may alter
the design attributes to suit your needs. Each attribute is compared to
the 'standard' design and both a build cost and a complexity (minimum
technology requirement) are evaluated. Specifically, the following
attributes may be altered:

	armor, hangar, crew, fuel, cargo, destruct, speed, mount, hyperdrive.

along with the special modification commands below.

   Build cost sums weighted attribute terms:
	base template cost + 1.0 * (primary + secondary guns)
	+ 0.05 * (crew + cargo + fuel + destruct)
	+ 0.50 * speed^1.5 + 0.50 * hangar + 3.50 * armor^1.5
	+ 0.003 * (cew_strength * cew_range)
multiplied by sqrt(1 + 0.5 * N), where N is the count of installed
hyperdrive, laser, cloak, and crystal mount systems.

   Design complexity compares each modified stat against its standard
template value via ((new + 1) / (standard + 1)) - 1, accumulating
positive terms as 'advantage' and negative magnitudes as 'disadvantage':
	deviation  = sqrt((1 + advantage) * exp(-disadvantage / 10))
	normalized = 1 + 10 * (deviation - 1) / sqrt(base_tech + 1)
	complexity = base_tech * normalized^2
If complexity exceeds your race's technology, the ship cannot be built.

   Base mass (empty hull) is:
	1.0 + 1.0 * armor + 0.2 * body_size + 0.1 * hangar
	+ 0.2 * (primary_guns * caliber_mult + secondary_guns * caliber_mult)
where caliber_mult is 1 (light), 2 (medium), or 3 (heavy).

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
