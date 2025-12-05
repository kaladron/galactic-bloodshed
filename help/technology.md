# TECHNOLOGY

## Name
   technology [1] -- invest money into tech production

## Syntax
   technology <money from treasury per update>

## Description
	This command sets an quota for technological investment. Each turn,
each race's technology is decreased by 1% then intelligence/100 is
added to its technology. If your race is disadvantaged by a low IQ you
can balance this discrepancy by investing money into research and development.
	Each turn the quota is deducted from your races' treasury (if
there is enough) and added logarithmically to your race's total tech
production.  As you increase the quota on one planet, you will notice, the
actual tech production increases less and less; thus, it is important to have
a wider cultural base (lots of planets) for better tech development.

	The additional technology increase per update for a planet is given by
a rather complex formula :

	tech = 0.01 *  log10(investment * SCALE + 1)

where SCALE = population/10000.

## See Also
	status, treasury, money
