# Planetary Simulation Engine and Sector Dynamics

## Overview

In **Galactic Bloodshed**, settled worlds are dynamic living ecosystems and industrial powerhouses. During full turn updates, the planetary simulation engine processes physical climate dynamics, automated ground vehicles, agricultural and industrial extraction, demographic breeding and starvation, territorial colonist expansion, environmental disasters, colony plunder, imperial census tallies, enslavement mechanics, taxation, and research.

The simulation executes through ten sequential simulation passes across each planet's surface grid:

```mermaid
flowchart TD
    Start(["Turn Simulation Phase"]) --> P1["1. Turn Reset & Ecological Assessment"]
    P1 --> P2["2. Ground Vehicles & Surface Automation"]
    P2 --> P3["3. Climate Dynamics & Thermal Drift"]
    P3 --> P4["4. Sector Production & Colonist Expansion"]
    P4 --> P5["5. Planetary Island Exploration"]
    P5 --> P6["6. Environmental Toxicity & Disasters"]
    P6 --> P7["7. Conquered Stockpile Plunder"]
    P7 --> P8["8. Imperial Census & Power Ratings"]
    P8 --> P9["9. Enslavement & Slave Revolts"]
    P9 --> P10["10. Planetary Economy & Defenses"]
    P10 --> End(["Turn Finalized & Telegrams Dispatched"])
```

---

## 1. Turn Reset and Ecological Assessment

Before sector simulation begins, the environment performs foundational baseline setup:
- Clears transient turn production accumulators and discovery flags.
- Re-tallies active planetary populations, stationed ground troops, and available mineral deposits.
- Pre-computes race-to-planet atmospheric compatibility ratings based on temperature, gravity, and atmospheric gas ratios (methane, oxygen, carbon dioxide, helium, nitrogen, sulfur).

---

## 2. Ground Vehicles and Surface Automation

Active surface vehicles, autonomous terraformers, and orbital support craft execute operational orders across the planetary grid:

- **Autonomous Von Neumann Probes**: Extract mineral resources from surface sectors, refine propellant, and replicate new machine offspring when resources suffice.
- **Berserker Warships**: Orbiting autonomous dreadnoughts execute saturation bombardment runs against designated enemy colonies.
- **Terraformers**: Autonomous ground vehicles navigate across sectors, conditioning hostile terrain toward their species' ideal biosphere.
- **Space Plows**: Move across arable land, conditioning topsoil to increase agricultural fertility while generating trace industrial byproducts.
- **Domes**: Erect climate-controlled habitats, upgrading sector efficiency and shielding colonists from harsh atmospheric conditions.
- **Quarries**: Strip-mine heavy mineral veins, extracting raw industrial materials into colony stockpiles before leaving behind spent wasteland.
- **Gas Giant Harvesting**: Tankers and orbital stations stationed in low orbit around gas giants skim atmospheric hydrogen to replenish fleet fuel reserves.

---

## 3. Climate Dynamics and Thermal Drift

Planetary surface temperatures evolve based on heliocentric orbital distance, seasonal variations, and orbital engineering:

```mermaid
flowchart LR
    Stellar["Stellar Baseline\nLuminosity & Orbit"] --> Drift["Seasonal Drift\n(+/- 5°C Variance)"]
    Mirrors["Orbital Space Mirrors\nFocused Solar Beams"] --> Thermal["Net Planetary Surface Temperature"]
    Drift --> Thermal
```

- **Natural Seasonal Drift**: Planetary surface temperatures experience natural atmospheric fluctuations of $\pm 5^{\circ}\text{C}$ around their stellar baseline.
- **Orbital Space Mirrors**: Giant orbital reflector arrays aimed at the planet focus stellar energy into the upper atmosphere to warm freezing worlds or shade overheated biospheres: $\Delta T = \left\lfloor \frac{\text{Solar Radiation} \times \text{Mirror Efficiency}}{\max(1, \text{Planet Radius})} \right\rfloor$.

---

## 4. Sector Production, Demographics, and Colonist Spread

The economic and biological heart of the simulation processes every occupied sector on the planet:

### Supernova Impact
If the host star is undergoing a nova collapse, extreme radiation sweeps across the planet, degrading agricultural fertility, stripping surface vegetation, and searing vulnerable terrain into nuclear wasteland.

### Industrial Resource Extraction
Populated sectors extract raw minerals and petroleum:
- **Mineral Yield**: Populated sectors extract mineral ore based on racial metabolism and sector efficiency: $\text{Yield} = \min\left(\text{Sector Reserves}, \left\lfloor \text{Metabolism} \times \text{UniformRandom}(1, \text{Efficiency}) \right\rfloor\right)$.
- **Propellant Synthesis**: Extracting minerals simultaneously generates refined fuel. Sectors classified as Gas Fields yield double fuel output ($2 \times \text{Yield}$).
- **Munitions Diversion**: If a sector has undergone military mobilization, extracted minerals are automatically refined into destructive ordnance (`destruct`) rather than raw minerals.
- **Crystal Synthesis**: Advanced empires with crystal discovery extract rare crystalline deposits from mineral-rich sectors.

### Demographic Breeding and Overpopulation Famine

```mermaid
flowchart TD
    Pop["Current Sector Population"] --> Cap{"Compare vs. Max Support Capacity"}
    Cap -->|Population < Max Support| Grow["Breeding Growth\nBirthrate * (Max Support - Pop)"]
    Cap -->|Population == Max Support| Stable["Demographic Equilibrium\n(Delta Pop = 0)"]
    Cap -->|Population > Max Support| Starve["Overpopulation Famine\nCasualties in [0, 2 * Excess]"]
```

- **Maximum Demographic Support Capacity**: The sustainable population cap for a sector depends on infrastructure efficiency, soil fertility, atmospheric compatibility, and environmental toxicity: $\text{Max Population} = \left\lfloor (\text{Efficiency} + 1) \times \text{Fertility} \times 0.01 \times \text{Compatibility} \times \frac{100 - \text{Toxicity}}{100} \right\rfloor$.
- **Reproductive Threshold**: If sector population drops below the species' reproductive minimum ($\text{Population} < \text{Reproductive Sexes}$), reproduction ceases entirely.
- **Population Growth**: Below carrying capacity, populations expand according to racial birthrate: $\Delta \text{Population} = \left\lfloor (\text{Max Population} - \text{Population}) \times \text{Birthrate} \right\rfloor$.
- **Overpopulation Starvation**: When population exceeds support capacity, severe famine inflicts casualties within the range: $\text{Casualties} \in \left[0, \min\big(2 \times (\text{Population} - \text{Max Population}), \text{Population}\big)\right]$.

### Spontaneous Colonist Migration and Expansion
When a sector becomes crowded ($\text{Population} > 0.10 \times \text{Max Population}$), pioneer colonists look to expand into neighboring wilderness:
- **Migration Pool**: Adventurous colonists form migration parties: $\text{Available Migrants} = \left\lfloor \text{Population} \times \text{Adventurism} \times \frac{100 - \text{Fertility}}{100} \right\rfloor - \text{Reproductive Sexes}$.
- **Topological Navigation**: Migrants step into adjacent unowned sectors, honoring **toroidal east/west seam wrapping** across meridians while respecting **polar north/south limits**.
- **Settlement Volume**: Migrants settle eligible unowned territory with positive environmental affinity: $\Delta \text{Settlers} = \left\lfloor \text{Available Migrants} \times \text{Compatibility} \times \frac{\text{Habitat Preference}}{100} \right\rfloor$.
- **Territorial Claim**: Settlers claim newly occupied sectors, planting imperial colony flags and expanding empire boundaries.

### Infrastructure Development and Plating
- Colonists improve sector efficiency over time at a rate influenced by tax rates, racial metabolism, and habitat preference.
- Upon reaching $100\%$ efficiency, the sector automatically converts to **Plated** status, maximizing structural durability and defensive shielding.

---

## 5. Planetary Island Exploration

For worlds with uncharted island chains or hidden landmasses:
- An exploration countdown timer steadily decrements each turn.
- When the timer reaches zero, imperial survey teams discover new landmasses, automatically colonizing revealed territory and dispatching discovery bulletins.

---

## 6. Environmental Toxicity and Industrial Disasters

Heavy manufacturing, strip-mining, and orbital bombardment generate toxic byproducts:
- **Disaster Threshold**: When planetary pollution exceeds critical environmental safety thresholds ($> 30\%$ Toxicity), an ecological catastrophe triggers.
- **Disaster Impact**: An industrial disaster incinerates a random populated sector into nuclear wasteland, destroying local population and infrastructure while alerting the governing empire.

---

## 7. Conquered Stockpile Plunder

When planetary invaders eradicate the defending garrison and capture a world:
- The system evaluates diplomatic relations among all victorious conquerors.
- If victorious empires share mutual alliances, captured commodity stockpiles (fuel, minerals, destruct, crystals) are divided equitably based on troop participation and sector control.
- Plunder shares are transferred into conqueror inventories and victory recovery telegrams are dispatched.

---

## 8. Imperial Census and Power Ratings

A single comprehensive census traversal audits the planetary grid:
- Aggregates planetary mineral reserves, fuel yields, and crystal deposits.
- Tallies civilian population, military garrisons, and maximum planetary carrying capacity.
- Updates stellar system demographics and empire-wide galactic power scores.

---

## 9. Enslavement and Slave Revolts

Subjugated enemy populations on conquered worlds are managed through enslavement policies:

```mermaid
flowchart TD
    Pop["Enslaved Planetary Population"] --> Gar{"Master Military Garrison Check\nMaster Pop <= 0.1% of Total Pop?"}
    Gar -->|"No (Sufficient Guard)"| Tribute["Tribute Diverted\n100% Commodity Harvest Sent to Master"]
    Gar -->|"Yes (Garrison Too Weak)"| Revolt["SLAVE REVOLT TRIGGERED!\nViolent Uprising Breaks Out"]
    
    Revolt --> Devastate["Urban Devastation\nSectors Destroyed in Uprising"]
    Devastate --> Free["Planetary Shackles Broken\nSlaves Liberated to Free Citizens"]
```

### Tribute Extraction
On peaceful slave worlds, the entire output of newly harvested commodities (fuel, minerals, destruct, crystals) is diverted directly into the master empire's stockpiles.

### Slave Revolt Triggers and Uprisings
An enslaved population requires an active military presence to maintain order. If the master empire's population drops to or below **$0.1\%$ ($1/1000\text{th}$)** of the total planetary population:
- **Devastation**: Violent uprisings break out across the world, devastating $`N_{\text{devastated}} = \left\lfloor \frac{\text{Total Population}}{1000} \right\rfloor + 1`$ random populated sectors.
- **Intimidation Backlash**: Master-owned sectors in intimidated star systems face a $50\%$ chance of destruction.
- **Liberation**: The shackles of enslavement are broken, fully liberating the planetary population.

---

## 10. Planetary Economy, Taxation, and Defenses

The turn simulation finalizes local economic accounting and defense readiness:

- **Harvest Deposits**: Newly mined resources and synthesized fuels are credited to local colony stockpiles.
- **Tax Collection**: Civilian taxes are levied and transferred into the system governor's treasury. Tax rate increases are constrained by the $+5\%$ per turn update rate-limiting policy.
- **Scientific Research**: Planetary research grants are deducted from the governor's treasury, generating imperial technology advancement points.
- **Ground Defense Batteries**: Total sector mobilization readiness is converted into active ground defense gun batteries: $`N_{\text{guns}} = \min\left(20, \left\lfloor \frac{\text{Total Mobilization Points}}{1000} \right\rfloor\right)`$.
- **Automated Waste Canisters**: If environmental pollution exceeds the governor's configured toxicity threshold, the colony automatically expends minerals to construct a Toxic Waste Canister ship, purging up to $20$ points of toxicity from the biosphere.

---

## 11. Automated Telegrams and Communications

Upon completing simulation passes, automated intelligence bulletins and telegrams are dispatched to system governors:
- **Autoreports**: Summarize commodity production totals, newly mined crystals, and temperature shifts.
- **Disaster Notices**: Alert governors to industrial toxicity disasters and sector devastation.
- **Nova Warnings**: Emergency evacuation bulletins warn of stellar nova collapses and boiling seas.
- **Revolt Bulletins**: Urgent war notices signal slave uprisings or planetary liberation events.

---

## See Also
- [Planetary Mechanics, Colonization, and Surface Topography](planets.md)
- [Species Biology, Ecology, and Racial Genetics](races.md)
- [Geoengineering, Terraforming, and Ecological Warfare](geoengineering.md)
- [Imperial Economy, Planetary Stockpiles, and Technology Investment](economy.md)
- [Tactical Combat, Naval Gunnery, and Planetary Warfare](combat.md)
- [Governance, Capitals, and Imperial Administration](governance.md)
- [Turn Simulation Lifecycle and Scheduling](turn_cycle.md)
- [Starships, Orbital Hierarchies, and Naval Mechanics](ships.md)
