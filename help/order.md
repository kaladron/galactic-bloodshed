# ORDER

## Name
	order [0] -- give ships standing orders

## Syntax
	order <shipnumber or shiptypes> <order> <arguments...>

## Description
  Order gives standing orders to the ship specified.  A list of ship 
types can also be used instead of a ship number. This will order all ships 
of the designated types in the same level the same. For example, 
'order dBC <orders...>' will order all destroyers, battleships, and 
cruisers in the current scope.

  Certain ships cannot be given certain orders; The program will notify
the user of illegal orders.

 If no shipnumber is specified, the program will list the orders of all ships
in the current ship. If a shipnumber is specified, but no other arguments are
given, the orders for that ship will be displayed.

Currently the orders that can be changed are:

  merchant -- Controls whether or not ship is in merchant mode. See

	'help route' for details.
	Syntax: order (shipnumber) merchant (off/route number)

  destination -- The ship will attempt to move towards the star/planet/ship
	specified.  This order cannot be changed for ships that are landed
	or docked. 
	Syntax: order (shipnumber) destination (destination)

  speed -- Change the speed of a ship. All speed factors are between 0 and 9.
	Higher speeds used fuel less efficiently than lower speeds.
	Syntax: order (shipnumber) speed (speed factor)

  use stockpile -- Many of ships use stockplies. That means you don't need 
	to load	resources in ships to repair or use them. It is enaugh if 
	there is resos or fuel on planet where they are landed.
	Syntax: order stockpile (on or off)
	NOTE: Ships work before planet produces new goodies. So if you
	want your ships to use stockpile be sure there is one to use
	BEFORE an update.

  jump -- This orders the ship to activate it's hyper-space engines in order to
	jump to another star system. The ship cannot be ordered to navigate
	when the engines are activated. In order to use the hyperspace jump,
	the engines must reach a charge level of 1, then the ship is 'ready'.
	When the engines are activated and charge is complete, the ship will
	automatically jump to the destination system (specified with order). 
	Ships that do not have	hyper-drive may not use this option. 	
	Syntax: order jump (on or off)

  impact -- A missile can be designated an impact sector. Upon reaching
	the destination planet it will explode centered upon the designated
	sector. A missile must have destruct loaded on board to be effective.
	Syntax: order (shipnumber) impact (x,y)

  scatter -- A missile can be designated to impact on a random sector. When
	this is set, impact order above is undone. Ordering an impact sector
	undoes this order.
	Syntax: order (shipnumber) scatter

  switch -- Many devices have an on/off switch to activate or deactivate
	them. This option toggles the on or off status.
	Syntax: order <shipnumber> switch

  online -- Factories can be turned online by issuing this command.
	Syntax: order <shipnumber> online

  aim -- Change the ship/object 'aimed at'.  There are several ship
	types that can do this, with a different effect for each.
	For instance, Space Mirrors can be focussed at an object in the
	same system or 2000 distance away at a cost of 0.3 fuel for 
	maneuvering.) Ships being focused on will take a certain amount
	of damage per turn, and planets will experience a temperature change
	corresponding to the intensity.
	Syntax: order <shipnumber> aim <target>

  intensity -- change degree of focus for a space mirror.
	Syntax: order <shipnumber> intensity <new intensity>

  transport -- this sets the target device for a transporter.
	Syntax: order <shipnumber> transport <target device>

  evade -- this orders a ship to be performing evasive maneauvers. Although
	this makes a ship, when moving, harder to hit, it also uses up
	twice as much fuel as normal.
	Syntax: order <shipnumber> evade <on or off>

  primary/secondary -- activate primary or secondary gun systems. If no
	argument is given the old default retaliation strength is used.
	Syntax: order <shipnumber> primary [<strength>]
	Syntax: order <shipnumber> secondary [<strength>]

  retaliate -- normally, if a ship is attacked and suffers damage, it
	and all ships assigned to protect will immediately retaliate
	with one salvo against the violating ships. A ship can also
	be order *not* to retaliate.
	Syntax: order <shipnumber> retaliate <on or off>

  laser -- If this is set, the ship will use it's combat lasers to attack
	with instead of normal fire. The player must specify how many
	strength points will fire upon retaliation (or default fire power).
	For ever point of strength, a laser used 2 fuel points instead
	destruct for the attack. The ship must have lasers mounted as
	well as a crystal in the hyperdrive.
	Syntax: order <shipnumber> laser <on or off> <default strength>

  navigate -- while this is on the destination described above is 
	ignored and instead, the ship will travel in the direction
	specified by the course. The directions are specified with
	degrees between 0 and 360 where 0 is 'up', 90 is 'right',
	180 is 'down' etc. You must also specify the number of turns
	to do the maneuver. After every move the turn count is decremented
	one until it becomes zero. After the maneuver the navigate
	setting is automatically turned off and if a destination
	has been set the ship will then proceed in that direction.
	Syntax: order <shipnumber> navigate <course> <turns>

  protect -- this allows a ship to protect another ship. Suppose, for
	example, ship A is set to protect ship B. If another ship C
	fires at B then both B *and* A will retaliate. This allows
	players to design their own defense networks. If no argument is
	specified protect will be turned off.
	Syntax: order <shipnumber> protect <ship to protect>

  salvo -- This allows players to control the strength of their
	retaliate/defense/protect response.  By default it is set
	to the number of guns of the ship.
	Syntax: order <shipnumber> salvo <number of guns>

  defense -- If this is set 'on' it will retaliate
	if the planet that it is in orbit around or landed on is
	attacked. If it is set 'off' the ship will not retaliate if
	its host planet is attacked.
	Syntax: order <shipnumber> defense <on or off>

  switch -- Turn mines and transporters on or off.
	Syntax: order <shipnumber> switch

  explosive -- If the ship is a mine or a gamma ray laser, this command sets 
	it to be in explosive mode which can damage ships.
	Syntax: order <shipnumber> explosive

  radiative --  Similar to the previous command.  Radiation mines and 
	gamma ray lasers incapacitate ships.
	Syntax: order <shipnumber> radiative

  move  -- If the ship is a terraforming device of space plow, the direction
	it is to move across the planet.  The direction is given as a single
	digit, similar to those used in the regular "move" command.
	Syntax: order <shipnumber> move <x>

  hop -- If ship is terraformer, space plow or dome, hop order can be given.
	If hop is on (default) ship will automaticly change sector it's 
	working on when sector becomes ready (fertilty, efficiency reaches 
	% set by limit). Plow will hop into the lowest fertility, dome in
	lowest efficiency combatible own or empty sector, and terraformer into
	another non compatible and non wasteland, own or empty sector.

  limit -- Limit will set the target efficiency combapility or fertility when 
	Y, T or K will jump into next sector.
	Syntax: order <ship> limit <wanted %> 

  autoscrap -- When YKT type of ship got nothing else to do and autoscrap 
	is ordered on, ship will automaticly scrap itself.

  trigger -- If the ship is a mine the trigger radius can be specified.
	Syntax: order <shipnumber> trigger <trigger radius>

  disperse -- Will mine disperse when it becomes empty. Word of warning..
	empty mines are easy to capture by you enemy.

  Ships that are landed or docked, or have no crew and are not robotic
ships, cannot be given orders.

## See Also
 scope, actionpoints
