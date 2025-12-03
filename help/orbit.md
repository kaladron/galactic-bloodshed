# ORBIT

## Name
   orbit [0] -- Graphic representation of objects in current scope

## Syntax
   orbit <[-n] [-S] [-p] [-s] [-ship/planet number]> <path>

## Description
  Orbit gives a visual map of all stars, planets, and ships in the scope
of path or the current scope if none is specified.  You cannot map an
area that has not been visited by one of your ships.

  If path is the root, orbit will show all the stars in the game,
and all ships travelling in interstellar space that belong to the player.

  If path is a star, orbit will display the solar system there
with its star in the center and orbiting planets around it, plus all ships
that are the player's (or can be detected).

  If path is a planet orbit will display that planet and orbiting
ships.

  If path is in the form #shipnum orbit will display a map of 
whichever system or planet the ship is at.

  If no path has been specified, the current scope is used, and display will
be centered around the current lastx,lasty coordinates.  These coordinates
represent the x,y location of the last scope the player was in.  For instance,
if a player types the following:

 [xx] / >cs #5/..
 [yy] / Sol / Sol-III >orbit

  The orbit display will be centered around the location of ship #5.

  When no path is specified, the zoom factor comes into play as well.  Display
will appear larger or smaller according to the zoom factor (1.0 being normal).
If there is a specified path argument to orbit, zoom and last coordinates are
not dealt with.  (It is as if they are 0,0 and 1.0, respectively.)

## Options

  when invoking orbit the options given (except for -(number)) can be 
either in the form "-pSs" or spread out as in "-p -S -s -3".

  -p : If this option is set, orbit will not display planet names.

  -S : Do not display star names.

  -s : Do not display ships.

  -(number) : Do not display that #'d ship or planet (should it obstruct
		the view of another object).  Stars or planets are numbered
		sequentially from 1, while ships can be referenced by their
		ship number.

## See Also
  map, cs, scope, zoom
