# TACTICAL

## Name
   tactical [0] -- provides a report on a ship or ships ability to perform
	military manuevers.

## Syntax
   tactical <#ship> - provides tactical information for the given ship
   tactical <#ship> <race> - provides report of spesified race. 
   tactical <shiptype>   - provides tactical information for all ships (of 
		a given type) and planets at the curent scope

   tactical - provides tactical information for all ships and planets
               	in the current scope

## Description
   The tactical command allows a governor to survey their ships for the
purpose of conducting offensive/defensive activities.  This command
differs from REPORT in that it provides information on a ship's ability to
attack enemy ships or planets.  Information provided includes enemy range
and the potential damage from a full broadside of the attacking ship's
guns.  Enemy ships will only appear on tactical display of they are in the
same scope as the calling ship.  This limitation is the same as
the orbit command.

## See Also

   report, weapons, stats
