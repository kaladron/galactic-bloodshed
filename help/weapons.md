# WEAPONS

## Name
   weapons [0] --  show armament status of ships

## Syntax
   weapons <#shipnum or shiptype>

## Description
   Show armament report of ship or ships. 

Example:
> weapon
    #       name      laser   cew     safe     guns    damage   class
    2 @                        0/0       25    0 /  0       0%    Standard
  161 t                        0/0      136   10L/ 10       0%    Standard
  162 t                        0/0      136   10L/ 10       0%    Standard
  983 O                yes     0/0      253   50H/ 50       0%    Standard

       #: shipnumber and type
    name: possible name of ship
   laser: do this ship have a laser
     cew: cew strength/range
    safe: max lasersalvo ship can shoot without a chance of crystal explosion
    guns: number and caliber of primary/secondary guns
  damage: ships damage status
   class: ships class (see 'name' for info about creating different classes). 

## See Also
     report, tactical
