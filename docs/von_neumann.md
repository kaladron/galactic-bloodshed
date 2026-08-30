# Von Neumann Machines and Berserker Warships

## Overview

Von Neumann machines (`OTYPE_VN`) and Berserkers (`OTYPE_BERS`) are autonomous, self-replicating robotic intelligences in Galactic Bloodshed. Originally seeded across the galaxy by Deity (Player 1) or deployed by advanced star empires, these automated probes travel interstellar distances, land on mineral-rich worlds, strip-mine planetary biospheres, and replicate exponentially.

## Machine Consciousness and Lineage

Every autonomous machine possesses an internal cybernetic consciousness and lineage tracking:

- **Progenitor**: The empire or player that originally created or deployed the ancestral seed machine (defaults to Player 1).
- **Generation**: The reproductive lineage counter ($g_0, g_1, \dots$). Each time a machine replicates, its offspring increments the generation:
  $$g_{\text{child}} = g_{\text{parent}} + 1$$
- **Technological Evolution**: Each generation of replication incorporates iterative hardware improvements, advancing the offspring's technology level by **$+20.0$ Tech** over its parent. Offspring also gain **$+1$ Armor** rating.
- **Binary Designations**: Newborn probes automatically assign themselves thematic binary strings (e.g. `"1010011"`, `"01101"`) as their cosmetic ship names.
- **Tampering and Subversion**: Alien empires can attempt to capture or tamper with machine minds, altering their target assignments and reprogramming their mission parameters.

## Planetary Surface Operations and Strip-Mining

When a machine reaches a planetary system, it enters orbit and initiates surface colonization:

### Landing Selection
- Probes scan the planets in the system, rejecting Gas Giants (which cannot support landings).
- The probe scans the surface sectors of the world, identifying and landing directly on a sector with non-zero mineral resources.

### Sector Strip-Mining
Once landed, a machine strip-mines its occupied sector:
$$\text{Yield} = \left\lfloor \text{Sector Resources} \times 0.5 \right\rfloor$$

- **Resource Cargo**: The extracted mineral yield is loaded directly into the probe's cargo hold.
- **Propellant Synthesis**: The probe simultaneously refines the mined minerals into an equal amount of fuel propellant ($\Delta \text{Fuel} = \text{Yield}$).
- **Berserker Ordnance Synthesis**: Berserker warships convert mined minerals into $5\times$ destructive ordnance ($\Delta \text{Destruct} = 5 \times \text{Yield}$).

### Sector Roaming and Toroidal Navigation
When a sector is stripped of all mineral resources ($0$ resources remaining), the machine automatically wanders to a random adjacent sector. It safely navigates across the planet's toroidal east/west wrap-around boundaries while respecting northern and southern polar limits.

### Colony Resource Theft
If player colonies exist on the world, landed probes will raid planetary stockpiles:
- The probe steals up to its base construction cost in resources from a randomly chosen colony on the planet.
- The victim player receives an automated telegram alert warning that autonomous probes have raided their resource depots.

## Self-Replication and Production Cycles

During the planetary simulation phase, landed probes evaluate their accumulated cargo reserves:

$$\text{Offspring Count} = \left\lfloor \frac{\text{Stored Resources}}{\text{Ship Build Cost}} \right\rfloor$$

For each child machine constructed:
1. **Resource Consumption**: The build cost is deducted from the parent's cargo hold.
2. **Propellant Sharing**: The parent splits its stored fuel reserves evenly with the newborn offspring ($50\%$ to parent, $50\%$ to child).
3. **Lineage Inheritance**: The offspring inherits the ancestral progenitor ID and advances to generation $g_{\text{parent}} + 1$.

## Galactic Aggression and Berserker Mobilization

The collective machine network monitors galactic combat and tracks hostility directed against its probes. When players attack or destroy Von Neumann machines, the network records the incident:

- The machine intelligence logs an aggression metric for each offending empire, identifying the player responsible for the highest casualties (`most_mad`).
- If total galactic aggression exceeds the alert threshold:
  $$\text{Total Galactic Aggression} > 100$$
- Landed probes initiate **war mobilization**: each replication cycle has a **$50\%$ chance** to construct a heavily armed **Berserker Warship** (`OTYPE_BERS`) instead of a peaceful probe.

### Berserker Warship Specifications

| Subsystem / Attribute | Specification |
| :--- | :--- |
| **Retaliation Target** | Programmed to hunt down the empire with the highest hostility rating |
| **Technology Bonus** | **$+100.0$ Tech** leap over the parent probe |
| **Armor Rating** | Parent Armor $+11$ |
| **Destructive Ordnance** | 500 destructive warheads |
| **Propulsion** | $5\times$ parent fuel capacity |
| **Hyperdrive Jump Engine** | Equipped with crystal-mounted jump drive, pre-charged and ready for hyperspace jumps |
| **Orbital Bombardment** | Planetary bombardment systems active (`bombard = true`) |
| **Combat Doctrine** | Automatic retaliation enabled with primary battery heavy guns |

## Interstellar Navigation and Galactic Exploration

Autonomous machines navigate the galaxy through automated behavioral stages:

1. **Deep Space Launch**:
   - Once a probe has accumulated full fuel tanks and completed its surface mining operations, it launches from the planetary surface into deep space.
2. **Target Star Selection**:
   - **Peaceful Probes**: Scan neighboring star systems within operational range, prioritizing uninhabited systems with terrestrial worlds while avoiding systems composed entirely of gas giants.
   - **Berserkers**: Scan the galaxy specifically for star systems colonized by their designated target empire.
3. **Orbital Arrival and Descent**:
   - Upon arriving in the destination star system, the machine maneuvers into planetary orbit, scans the surface grid, and lands on a resource-bearing sector to begin the lifecycle anew.

## See Also
- [Planetary Mechanics and Colonization](planets.md)
- [Planetary Simulation Engine](planetary_simulation.md)
- [Governance and Administration](governance.md)
- [Galactic Economic Model](economy.md)
