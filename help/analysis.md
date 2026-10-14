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
mobilization, population, and population capacity (^Popn). 

    The population capacity (^Popn) shows the maximum population 
each sector could support for YOUR race, based on your race's 
compatibility with the planet and sector type preferences. This 
helps you identify good expansion targets, regardless of who 
currently owns the sector.

    You can not analyze a planet you have not explored or have 
received information about.

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
                Specify 0 to report only unoccupied (unowned) sectors.

scope           If a planet's scope is specified, that planet's sectors 
                will be analyzed and reported. If a star scope is specified, 
                all explored planets in the specified star system will be 
                reported. If no scope is specified, the current scope 
                will be used.

## Output Format

### Ranked Sector Lists (Highest / Lowest 5)
    The top section lists up to five sectors for each category (Troops, Res,
Eff, Frt, Mob, Popn, ^Popn). Each entry is formatted as:

        <value><terrain>(<x>,<y>)

    For example, '20^( 1, 0)' indicates a value of 20 on a mountain ('^')
sector at coordinates (1, 0), and '8.( 0, 1)' indicates a value of 8 on an
ocean ('.') sector at (0, 1).

### Planetary Holdings Summary Table
    The bottom table summarizes all sectors on the planet grouped by owner
(regardless of any filter applied to the ranked lists above):

> Pl  sec  popn  troops  a.eff  a.mob  res  x   .   *   ^   ~   #   )   -   o   %
> 0   20   0     0       0.0    0.0    488  0  18   0   0   1   0   0   1   0   0
> 1   3    3500  35      80.0   50.0   330  1   0   1   1   0   0   1   0   0   0
> 2   2    1400  14      55.0   27.5   90   0   1   0   0   0   1   0   0   0   0
> Tl  25   4900  49      14.0   8.2    908  1  19   1   1   1   1   1   1   0   0

    Pl: Player (owner) ID, not planet number:
          0    -- Unoccupied (unowned) sectors
          1..N -- Sectors owned by that player number
          Tl   -- Planet-wide totals across all sectors
   sec: Count of sectors owned by that player (or total sectors on the planet
        in the 'Tl' row)
  popn: Total civilian population across those sectors
troops: Total military troops across those sectors
 a.eff: Average efficiency (%) across those sectors. In the 'Tl' row, this is
        averaged across all sectors on the planet, including unoccupied ones.
 a.mob: Average mobilization (%) across those sectors. In the 'Tl' row, this is
        averaged across all sectors on the planet, including unoccupied ones.
   res: Total resource deposits across those sectors
     x: Number of crystal sectors owned by that player (reported only if your
        race has discovered crystal technology; otherwise 0)
 . * ^ ~ # ) - o %:
        Count of sectors of each terrain condition owned by that player:
          '.' Ocean/Sea    '*' Land       '^' Mountain
          '~' Gas          '#' Ice        ')' Forest
          '-' Desert       'o' Plated     '%' Wasted
        Across each row, these nine terrain counts sum to the 'sec' column.

## Example
analysis        (produce analysis from all sectors of planet of the 
                    current scope)
analysis 0      (produce analysis from highest 5 unoccupied sectors of
                    the planet of the current scope)
analysis - # 7  (produce analysis from lowest 5 ice sectors owned by 
                    player 7 of the planet of the current scope)

## See Also
     survey, map
