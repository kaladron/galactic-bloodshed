# ANALYSIS

## Name
   analysis [0] -- provide sector report for planet

## Syntax
   analysis [ [-] [<sector type>] [<race #>] [<scope>] ]

## Description
    Use analysis to obtain a detailed listing of the highest or 
lowest five sectors in each sector category and a summary of 
holdings, yours and alien, on the planet. The sector categories 
reported are troops, resources, efficiency, fertilization, 
mobilization, population, and population capacity. You can not 
analyze a planet you have not explored or have received 
information about.
    If a '-' is specified, the lowest five sectors will be 
reported. All additional parameters are completely optional, but 
when specified, restrict the type of sector reported:

sector type     Reports only sectors of specified type. The sector 
                type is specified by the character used to represent
                the terrain of the sector and must be from the set of 
                characters:
                        . * ^ ) # d o % ~
                ('d' is used to denote desert sectors to avoid confusion 
                with the leading '-' in the analysis command.) 

race #          Reports only sectors belonging to the specified race.

scope           If a planet's scope is specified, that planet's sectors 
                will be analyzed and reported. If a star scope is specified, 
                all explored planets in the specified star system will be 
                reported. If no scope is specified, the current scope 
                will be used.

## Example
analysis        (produce analysis from all sectors of planet of the 
                    current scope)
analysis - # 7  (produce analysis from lowest 5 ice sectors owned by 
                    player 7 of the planet of the current scope)

## See Also
     survey, map
