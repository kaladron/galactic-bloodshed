# DEFEND

## Name
   defend [1]  -- shoot a planet at a ship

## Syntax
   defend <target> <x,y> [<strength>]

## Description
   When using the 'defend' command the player must have the planet in question
as the current scope. Only ships in orbit or landed at the planet can be
designated.

   The maximum number of guns for a planet depends on planet size.
The actual number of guns a planet has available for use begins at 0, 
and will increase as the sector mobility status increases. 

 Number of planetary guns = number_of_sectors * average mobility * 0.001

   The formulas for calculating hit probability and damage are the same as for
the "fire" command.

## See Also
  fire, tactical, mobilize
