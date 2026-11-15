# FIX

## Name
   fix [0] -- inspect or override planet and ship state (deity only)

## Syntax
   fix planet <option> [<value>]
   fix ship <option> [<value>]

## Description
   Reads or administratively overrides entity fields without triggering combat
morale or fallout side effects. Omitting `<value>` prints the current setting.

   At planet scope (`fix planet`):
     xpos, ypos, rtemp, temperature, toxic, methane, oxygen, co2, hydrogen,
     nitrogen, sulfur, helium, other

   At ship scope (`fix ship`):
     fuel, max_fuel, destruct, resource, damage
     dead  -- immediately destroys and removes the current ship

## See Also
   bless, cs, survey, stats
