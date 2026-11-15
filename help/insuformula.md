# INSUFORMULA

## Name
   [10]insurgency -- try to force enemy population into civil revolt

## Syntax

insurgency <victim> <funds>   spend a given amount of funds to persuade
                              the population of the victim to throw a 
                              bloody little revolt.

## Description

   The insurgency command is used to persuade the population of a planet,
through financial means, that they have had enough of their bloody
dictator and should rise in revolt. This command will only work at planet
scope.  You must have population in that planet's system, before you can
attempt an insurgency.  The probability of a successful insurgency is
given by the following formula, using morale_factor(d) = atan(d / 10000) / pi + 0.5:

 x = (funds * victim_tax_rate / victim_planet_pop)
     * morale_factor(your_morale - victim_morale)
     * morale_factor((your_system_pop - victim_system_pop) / 50)
     * morale_factor(10 * (your_fight * your_planet_troops
                           - victim_fight * victim_planet_troops))
     / 50

 chance (%) = round_rand((200 / pi) * atan(x))

   All funds invested in the insurgency are spent whether the attempt
succeeds or fails.

   If the attempt succeeds, each populated sector owned by the victim on
the planet is evaluated for revolt:
  1) Base sector revolt roll succeeds with probability equal to the
     victim's planetary tax rate (%).
  2) Garrisoned troops suppress the revolt when a uniform roll in
     1..sector_popn is <= 10 * victim_fight * sector_troops.
  3) Otherwise the sector transfers to the instigator, defending troops
     are eliminated, random civilian casualties occur, and the instigator
     inherits the victim's planetary tax rate.

## See Also
	money, tax, treasury
