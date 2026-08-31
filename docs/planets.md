# Planetary Mechanics, Colonization, and Surface Topography

## Overview

Planets form the core sovereign territory, population centers, and industrial engine of empires in **Galactic Bloodshed**. This guide details surface topography, cylindrical grid navigation, thermal dynamics, military mobilization, ground defense batteries, automated ecological cleanup, and enslavement revolt mechanics.

```mermaid
flowchart TD
    Planet["Planetary World"] --> Surface["Surface Grid Topology\nToroidal East/West Seam & Polar Boundaries"]
    Planet --> Climate["Climate & Thermal Equilibrium\nStellar Luminosity & Space Mirror Redirection"]
    Planet --> Industry["Industrial & Demographic Production\nResource Mines, Farmland & Plated Cities"]
    Planet --> Military["Defense & Mobilization\nGround Batteries & Combat Readiness"]
    Planet --> Ecology["Ecological Management\nToxicity Mitigation & Waste Canisters"]
```

---

## 1. Planetary Grid Topology and Surface Navigation

Planetary surfaces are modeled as discrete 2D coordinate grids spanning $[0, W - 1] \times [0, H - 1]$, where $W$ is the planet's equatorial width and $H$ is the polar height.

```mermaid
flowchart LR
    subgraph Grid ["Planetary Coordinate Geometry"]
        direction TB
        North["North Pole (y = 0)\n5 Neighbors (No Northward Movement)"]
        Equator["Temperate / Equatorial Zones (0 < y < H - 1)\n8 Full Neighbors (Orthogonal & Diagonal)"]
        South["South Pole (y = H - 1)\n5 Neighbors (No Southward Movement)"]
        North --- Equator --- South
    end

    West["West Boundary\n(x = 0)"] <-->|Continuous Toroidal Seam| East["East Boundary\n(x = W - 1)"]
```

### Surface Geometry & Seam Wrapping
- **East/West Toroidal Wrapping ($X$ Dimension)**: Moving east past the eastern boundary ($x = W - 1$) wraps continuously around to $x = 0$. Moving west past $x = 0$ wraps seamlessly to $x = W - 1$.
- **North/South Polar Boundaries ($Y$ Dimension)**: The vertical axis represents latitude, bounded between the North Pole ($y = 0$) and the South Pole ($y = H - 1$). Ground units cannot step north of the North Pole or south of the South Pole; traversal attempts bounce inward.

### Sector Adjacency and Neighborhood Calculations
- **Equatorial / Temperate Sectors** ($0 < y < H - 1$): Have **8 topological neighbors** (including diagonal moves and wrapped longitudinal seams).
- **Polar Sectors** ($y = 0$ or $y = H - 1$): Have **5 topological neighbors** due to polar boundary clamping.

Autonomous robotic probes and migrating colonists utilize these topological rules to navigate across landmasses without boundary clipping errors.

---

## 2. Climate, Thermal Dynamics, and Space Mirrors

Each world possesses a natural baseline temperature determined by its star's spectral luminosity, stellar radius, and orbital distance.

```mermaid
flowchart LR
    Sun["Host Star\nLuminosity & Orbit"] --> Base["Baseline Temperature"]
    Drift["Atmospheric Drift\n(+/- 5°C per Turn)"] --> Net["Net Planetary Temperature"]
    Mirrors["Orbital Space Mirrors\nFocused Stellar Energy"] --> Net
    Base --> Net
```

### Thermal Variance and Environmental Modification
During each full turn update, surface temperature evolves according to:

$$T_{\text{surface}} = T_{\text{base}} + \Delta T_{\text{mirrors}} \pm 5^{\circ}\text{C}$$

- **Seasonal Atmospheric Drift**: Natural stochastic fluctuations of $\pm 5^{\circ}\text{C}$ simulate seasonal weather shifts and atmospheric turbulence.
- **Orbital Space Mirrors**: Giant orbital reflector arrays stationed in orbit focus stellar energy into the upper atmosphere to warm freezing worlds, melt glaciated biospheres, or shade overheated planets:
  $$\Delta T_{\text{mirrors}} = \left\lfloor \frac{\text{Solar Radiation} \times \text{Mirror Efficiency}}{\max(1, \text{Target Planet Radius})} \right\rfloor$$

---

## 3. Sector Mobilization and Ground Defense Batteries

Planetary military readiness is built from the ground up through sector-level mobilization.

### Sector Mobilization and Combat Readiness
- Governors mobilize individual sectors ($0\%$ to $100\%$) for military service using the `mobilize` command.
- The average mobilization across all colonized sectors determines the planet's imperial **combat readiness**. Highly mobilized sectors divert industrial mineral extraction directly into munitions synthesis (`destruct`).

### Planetary Defense Batteries
Total mobilization points across all owned sectors translate directly into ground-based planetary defense batteries:

$$N_{\text{guns}} = \min\left(20, \left\lfloor \frac{\text{Total Mobilization Points}}{1000} \right\rfloor\right)$$

```mermaid
flowchart TD
    Mob["Sector Mobilization Points\n(Sum across all owned sectors)"] --> Calc["Divide by 1000 & Floor"]
    Calc --> Cap{"Compare with 20 Gun Maximum"}
    Cap --> Guns["Ground Defense Guns Active\n(Up to 20 Batteries)"]
    Guns --> Defend["Defend Command\nFire at Hostile Orbiting Craft & Landers"]
```

- **Operational Role**: Up to $20$ heavy surface batteries can fire on hostile warships in orbit or intercept invading assault landers during descent.
- **Munition Supply**: Firing surface batteries consumes destructive ordnance stored in local colony stockpiles.

---

## 4. Automated Ecological Cleanup and Waste Canisters

Heavy manufacturing, strip-mining, and nuclear bombardment generate toxic industrial contaminants that degrade habitability.

### Automated Waste Canister Fabrication
Governors configure an automated environmental cleanup policy using the `toxicity` command to set a threshold ($0\%$ to $100\%$):
1. **Automated Trigger**: When planetary toxicity meets or exceeds the configured threshold, and the colony maintains sufficient mineral resources, local shipyards automatically fabricate a **Toxic Waste Canister** vessel.
2. **Ecological Purge**: Constructing the canister absorbs up to $20$ points of toxicity from the biosphere, trapping contaminants within the vessel's containment hold for subsequent orbital launch, deep-space disposal, or environmental deployment against hostile worlds.

---

## 5. Enslavement and Slave Revolts

When capturing foreign worlds, conquerors can subjugate the native population using the `enslave` command, compelling them to labor for the master empire.

```mermaid
flowchart TD
    Sub["Subjugated Colony Population"] --> Check{"Garrison Evaluation\nMaster Pop <= 0.1% Total Pop?"}
    Check -->|Sufficient Military Guard| Tributary["Tribute Diverted\n100% of Harvest Sent to Master"]
    Check -->|Garrison Insufficient| Uprising["SLAVE REVOLT DETONATES!"]
    
    Uprising --> Casualties["Urban Devastation\nPopulated Sectors Destroyed"]
    Uprising --> Backlash["Regional Backlash\n50% Chance of Devastation on Intimidated Stars"]
    Uprising --> Freedom["Shackles Broken\nPopulation Liberated to Free Citizens"]
```

### Tribute Extraction
On peaceful slave worlds, the entire output of newly harvested commodities (fuel, minerals, destruct, crystals) is automatically diverted directly into the master empire's planetary stockpiles.

### Slave Revolt Triggers
Subjugated populations require an active military garrison to enforce servitude. If the master empire's population drops to or below **$0.1\%$ ($1/1000\text{th}$)** of the total planetary population:

$$\text{Population}_{\text{master}} \le \left\lfloor \frac{\text{Population}_{\text{total}}}{1000} \right\rfloor$$

a planetary **slave revolt** is immediately triggered.

### Consequences of a Slave Revolt
1. **Urban and Ecological Devastation**: Uprisings ignite widespread fighting across the world, completely devastating:
   $$N_{\text{devastated}} = \left\lfloor \frac{\text{Population}_{\text{total}}}{1000} \right\rfloor + 1$$
   random populated sectors into ruined wasteland.
2. **Regional Intimidation Backlash**: Master-owned sectors located within intimidated star systems suffer a $50\%$ probability of devastation.
3. **Liberation**: Imperial shackles are broken, immediately restoring the subjugated population to free citizen status.

---

## See Also
- [Planetary Simulation Engine and Sector Dynamics](planetary_simulation.md)
- [Imperial Economy, Planetary Stockpiles, and Technology Investment](economy.md)
- [Governance, Capitals, and Imperial Administration](governance.md)
- [Starships, Orbital Hierarchies, and Naval Mechanics](ships.md)
- [Turn Simulation Lifecycle and Scheduling](turn_cycle.md)
- [Autonomous Machine AI and Berserkers](von_neumann.md)
