# MAP

## Name
   map [0] -- Cartesian coordinate map of planet's surface

## Synopsis
   map <path>

## Description
  The map command gives a sector map of the planet in [path].  If [path]
is not a planet, map will instead run orbit (see orbit) with lastx,lasty 
both equal to 0.0, or starmap of whole galaxy.  A player cannot map a 
planet that has not yet been explored.

  These are the different types of terrain:

   '*' -- Unexplored wilderness or undeveloped land
   '^' -- Mountainous area
   '.' -- Area containing water
   '#' -- Area covered with ice
   '~' -- Gas
   ')' -- Forest
   '-' -- Desert
   '%' -- Wasteland
   'o' -- A steel (or whatever) plated sector, much like the planet
	    Trantor (from Asimov's _Foundation_)

  In addition are the designations that map gives the player:

	  Areas owned by your race use 'reverse background' so you
	  can see the terrain occupied as well.

   '?' -- Area owned by another player where ? is the one's digit
	  of the players number.
   'A' -- There is allied troops in sector.
   'N' -- There is neural troops.
   'X' -- There is enemy troops.	 
   '<sign of ship>' -- a ship of that type is landed on the sector
   'x' -- Sector is  containing crystals.

Use the 'geography' option to toggle whether or not aliens are displayed
at all. The default value is that they are displayed.

## See Also
  orbit, cs, scope
