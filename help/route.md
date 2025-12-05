# ROUTE

## Name
   route [0] -- sets and provides information about shipping routes.

## Syntax
   route - show all active shipping routes for the planet
   route <number> - show the route at the planet specified by <number>
   route <number> <path> - this will set the route number to the planet
			   given by the path. The route must be active.
   route <number> activate - activate the shipping route
   route <number> deactivate - deactivate the route
   route <number> land <x,y> - ships will land on sector x,y
   route <number> load <list of commodities> - load the commodies when landed
   route <number> unload <list of commodities> - unload the commodites when
			   when landed

## Description
   This command is used to set up automated shipping for your empires.
Each planet is allowed to have 4 routes activated, number 1 through 4.
A route has the following components:
	destination:  This is where ships on the route will travel to
			next, after loading and unloading at the planet.
			If a ship has hyperspace capability, jumps will
			be ordered. Destinations are set suring all
			movement segments and updates. Only ships with
			merchant shipping set to the route will be given
			the orders.
	activate/deactive: This will control whether route orders are
			given to arriving merchant ships.
	land: Ships must land on a sector to perform their loading/unloading
			orders. If a ship does not have the fuel to
			land, it won't. Merchant ships will not crash land.
			The player must also control the route sector, or
			it must be vacant.
	load/unload: Once landed, ships will embark and debark commodities
			specified at the planet. Note that the maximum 
			possible  transfer will always be ordered.
			Example:  route 3 load rfdx   will order route 3
					merchants to load r, f, d, and xtals.
			Example:  route 1 unload rd  will order route 1
					merchants to unload all r and d. Note
					that you should not order ships to
					unload fuel for obvious reasons!

## Example
	You produce destruction in /Vaasa/Hima
	resource in /Espoo/Otski and you want to move those in your
        homeplanet /Espoo/Nysvaekoto, using cargoship #357.

First create route.
 'cs /Espoo/Nysvaekoto'
 'route 1 activate'
 'route 1 land 0,0'      or what ever is good sector
 'route 1 load f'        now your ship got fuel to travel
 'route 1 unload rd'     commodities delivery
 'route 1 /Espoo/Otski'  next stop 	
 'cs /Espoo/Otski'
 'route 1 activate'
 'route 1 land 0,0'
 'route 1 load r'
 'route 1 /Vaasa/Hima'   next stop
 'cs /Vaasa/Hima'      
 'route 1 activate'
 'route 1 land 3,4'      again sector is just an example
 'route 1 load d'
 'route 1 /Espoo/Nysvaekoto' back to home and ship will start another round

secondly order ship into that route.
 'order #357 merchant 1'         now #357 know to follow route 1
 'order #357 dest /Espoo/Otski'  first travel must be ordered

Now ship is on it's way and will do everything else automaticly. 

## See Also
   order
