# BLESS

## Name
   bless [0] -- bestow divine blessings upon a player (deity only)

## Syntax
   bless <player> <property> <amount>

## Description
   Modifies racial attributes, planet exploration/colony metadata, or system
commodities for `<player>`. Requires deity privileges and planet scope, and
notifies the target race leader.

   Racial flags (3rd argument still required on the command line):
     pods, nopods, collectiveiq, nocollectiveiq, guest, god, mortal

   Racial attributes and sector preferences:
     password <str>, money <+amt>, morale <+amt>, technology <+amt>,
     fertility <val>, IQ <val>, maxiq <val>, fight <val>, mass <val>,
     metabolism <val>, adventurism <val>, birthrate <val>,
     water/land/mountain/gas/ice/forest/desert/plated <pct>

   Planet and star properties (at current planet/star):
     explorebit, noexplorebit, inhabited, planetpopulation <val>,
     numsectsowned <val>

   Planetary stockpiles and star AP (`+amount` or `-amount`):
     r/resource/resources, d/destruct, f/fuel, x/crystal/crystals,
     a/ap/action

## See Also
   fix, emulate, profile
