# Starships, Orbital Hierarchies, and Naval Mechanics

## Overview

Starships in **Galactic Bloodshed** represent an empire's primary instruments of interstellar exploration, orbital transport, colonization, planetary bombardment, and space warfare. Vessels range from unmanned sensor probes and atmospheric terraformers to massive carrier flagships, mobile factory ships, and crystal-mounted dreadnoughts.

This guide details naval operations, orbital reference frames, carrier docking hierarchies, environmental hazards (radiation, supernovae), hull maintenance, and turn update mechanics.

---

## 1. Orbital Reference Frames and Spatial Positioning

A vessel in Galactic Bloodshed operates within one of four spatial reference frames:

```mermaid
flowchart TD
    Univ["Universe Scope (Deep Space)\nInterstellar Coordinates"] --> Star["Star System Orbit\nHeliocentric System Position"]
    Star --> Plan["Planetary Orbit / Surface\nOrbital Track or Surface Sector Grid"]
    Plan --> Carrier["Carrier Hangars\nDocked Inside Host Ship or Station"]
    Star --> Carrier
    Univ --> Carrier
```

| Reference Frame | Operational Context | Navigation & Orders |
| :--- | :--- | :--- |
| **Universe Scope** | Deep space between star systems | Long-range interstellar transit, hyperspace jump routes, deep space sensor reconnaissance |
| **Star System Orbit** | In orbit around a star | Interplanetary patrols, space mirror positioning, system defense, system interception |
| **Planetary Scope** | In low orbit around a world or landed on the planetary surface | Planetary bombardment, cargo loading/unloading, ground troop deployment, surface mining |
| **Carrier Hangars** | Docked inside a host carrier or space station | Parasite craft transport, carrier protection, hangar bay maintenance |

---

## 2. Carrier Docking and Fleet Ownership

Capital vessels and space stations (such as Fleet Carriers, Mobile Factories, and Orbital Stations) are equipped with internal hangar bays capable of docking smaller parasite craft (such as Fighters, Shuttles, and Probes).

### Hangar Operations
- **Docking**: Smaller vessels can dock with friendly carriers or stations to be transported across interstellar distances without expending their own fuel.
- **Fleet Allegiance**: Docked craft operate under the direct command of the host carrier. Whenever a vessel is docked inside a carrier, the carrier's commanding empire and governor maintain operational control of all carried craft. If a carrier changes allegiance, all docked craft within its hangars transition with the carrier.

### Dynamic Operational Mass and Displacement Metrics
A vessel's total displacement includes its baseline structure, stored consumables, carried populations, and any docked parasite craft:

$$\text{Mass}_{\text{total}} = \text{Base Hull Mass} + (\text{Fuel} \times 0.05) + (\text{Resources} \times 0.10) + (\text{Destruct} \times 0.15) + (\text{Crew} + \text{Troops}) \times M_{\text{race}} + \sum \text{Mass}_{\text{docked}}$$

where $`M_{\text{race}}`$ is the physical body mass per individual colonist or soldier of the carried species.

#### Base Hull Mass
Empty hull mass depends on defensive armor plating, chassis volume, internal hangar bays, and kinetic gun mounts:

$$\text{Base Hull Mass} = 1.0 + \text{Armor} \times 1.0 + \text{Hull Volume} \times 0.2 + \text{Hangar Capacity} \times 0.1 + 0.2 \times \Big(N_{\text{primary}} \times K_{\text{primary}} + N_{\text{secondary}} \times K_{\text{secondary}}\Big)$$

where $N$ is the number of operational gun mounts and $K$ is the caliber mass multiplier ($1$ for Light Guns, $2$ for Medium Guns, $3$ for Heavy Guns, and $0$ for Unarmed).

#### Displacement Sizing
A ship's physical displacement profile determines its target signature in tactical combat and maximum internal capacity:

$$\text{Displacement Size} = \left\lfloor 1.0 + 0.1 \times (N_{\text{primary}} + N_{\text{secondary}}) + 0.01 \times \text{Crew Capacity} + 0.02 \times \text{Resource Capacity} + 0.01 \times \text{Fuel Capacity} + 0.02 \times \text{Destruct Capacity} + \text{Hangar Capacity} \right\rfloor$$

#### Joint Berthing Capacity and Crew Compartments
A critical structural invariant of starship architecture is that **civilian crew and military ground troops share the exact same physical living quarters**:

$$\text{Civilian Crew} + \text{Military Troops} \le \text{Maximum Berthing Capacity}$$

- **Trade-Off**: Embarking planetary assault troops directly displaces civilian crew berths. If civilian crew drops below the operational requirement for active gun mounts ($1\text{ crew per gun}$) or damage control, the vessel's combat and repair readiness suffers.
- **Dynamic Casualty Mass Updates**: When combat fire or radiation sickness inflicts casualties, the ship's operational displacement drops instantaneously:

$$\Delta \text{Mass} = -(\Delta \text{Crew} + \Delta \text{Troops}) \times M_{\text{race}}$$

#### Strategic Warp Crystals and Drive Racks
Strategic warp crystals are required to power faster-than-light hyperspace jump drives and direct-fire combat lasers:
- **Zero Displacement**: Warp crystals possess zero physical mass, allowing ships to carry strategic reserves without incurring propulsion penalties.
- **Drive Racks (`mount` command)**: Jump-capable warships and assault transports are equipped with internal drive racks. A crystal must be explicitly mounted into the drive core (`mount <ship>`) before hyperdrives can be charged or combat lasers activated.
- **Shatter Risks**: Executing long-range hyperspace jumps or drawing peak power for combat lasers stresses the crystal matrix, carrying risks of matrix degradation or sudden crystallization shatter.

---

## 3. Planetary and Stellar Exploration

An empire expands its galactic awareness through exploration-capable starships.

### Exploration Criteria
A starship can survey star systems and map planetary biospheres if it meets either of the following requirements:
1. **Living Crew**: Any starship carrying living colonists or crew members ($\text{Colonists} > 0$).
2. **Dedicated Sensor Probes**: Automated reconnaissance probes specially engineered for unmanned exploration and deep space telemetry.

Uncrewed freighters, unmanned cargo pods, and empty hulls cannot map worlds or discover star systems.

### Discovery Mechanics
- **Star Systems**: Entering a star system with an exploration-capable vessel surveys the system for the empire, revealing all orbited planets, orbital distances, and stellar classifications.
- **Planetary Biospheres**: Establishing planetary orbit or landing on a world surveys the planet, uncovering surface terrain composition, atmospheric toxicity, temperature, and colony presence.

---

## 4. Naval Turn Lifecycle

During each simulation segment and turn update, all active vessels in the galaxy are processed through sequential naval subsystems:

```mermaid
flowchart TD
    Start["Turn Simulation Phase"] --> Rad["1. Radiation & Mobility Evaluation"]
    Rad --> Nova["2. Supernova Blast Hazards"]
    Nova --> Factory["3. Mobile Factory Tech Upgrades"]
    Factory --> Move["4. Propulsion & Course Navigation"]
    Move --> CarrierSync["5. Hangar Ownership Synchronization"]
    CarrierSync --> Explore["6. Stellar & Planetary Exploration"]
    Explore --> Census["7. Galactic Census & Power Scores"]
    Census --> Bombard["8. Orbital Bombardment Staging"]
    Bombard --> Repair["9. Hull Repair & Resource Consumption"]
    Repair --> Special["10. Special Systems (Mirrors, Habitats, Mines)"]
```

### 1. Structural Damage, Peak-Dose Radiation, and Crew Attrition
Hulls endure environmental hazards and weapon fire with strict physical damage boundaries:
- **Structural Integrity and Instant Destruction**: Hull damage is bounded strictly within $[0\%, 100\%]$. When cumulative structural damage reaches or exceeds $100\%$, the vessel suffers catastrophic structural failure and is instantaneously destroyed and removed from active naval registries.
- **Peak-Dose Radiation Model**: Vessels exposed to nuclear detonations, stellar flares, or toxic fallout accumulate radiation using peak-dose semantics: minor subsequent radiation exposures never overwrite or dilute higher historical contamination levels.
- **Guidance and Propulsion Immobilization**: Severe radiation fries avionics and incapacitates helm crews. During movement segments, contaminated ships face an immobilization risk directly proportional to radiation severity:

$$P(\text{Immobilized}) = \frac{\text{Radiation Level}}{100}$$

- **Radiation Sickness & Crew Attrition**: During full turn updates, lethal radiation sickness claims $20\%$ of living crew and carried military troops:

$$\text{Crew}_{\text{new}} = \left\lfloor \text{Crew}_{\text{old}} \times 0.80 \right\rfloor, \quad \text{Troops}_{\text{new}} = \left\lfloor \text{Troops}_{\text{old}} \times 0.80 \right\rfloor$$

- **Natural Decontamination**: Radiation dissipates gradually over time during update passes: $\Delta \text{Radiation} = -\text{UniformRandom}\Big(0, \min\big(\text{Radiation}, \text{Base Decontamination Rate}\big)\Big)$.

### 2. Supernova Blast Waves
Vessels caught in a star system undergoing a nova collapse suffer extreme radiant heat and physical shockwave damage:

$$\Delta \text{Damage} = \left\lfloor \frac{5 \times \text{Nova Stage}}{(\text{Effective Armor} + 1) \times S} \right\rfloor$$

where $S$ is the number of simulation segments per update pass. If cumulative structural damage reaches or exceeds $100\%$, the vessel is destroyed by the blast.

### 3. Mobile Factory Technology Upgrades
Offline Mobile Factories automatically modernize their internal manufacturing tooling to match the owning empire's current imperial technology level ($`\text{Tech}_{\text{factory}} \leftarrow \text{Tech}_{\text{empire}}`$). Powering down a factory allows it to absorb the latest technological breakthroughs before resuming production.

### 4. Hull Maintenance and Resource Consumption
Damaged vessels ($\text{Damage} > 0$) attempt structural repairs during turn updates:
- **Free Station Maintenance**: Orbital repair stations and vessels docked with them perform hull repairs without consuming stored resources.
- **Crew Repair Scaling**: The effective repair output scales with available crew staffing: $`r_{\text{crew}} = \frac{\text{Current Crew}}{\text{Maximum Crew Capacity}}`$, yielding maximum repair potential $`\text{Max Repair} = \text{Base Repair Rate} \times r_{\text{crew}}`$.
- **Resource Cost**: Repairing hull damage consumes refined minerals: $\text{Resource Cost} = \left\lfloor 0.005 \times \text{Max Repair} \times \text{Ship Construction Cost} \right\rfloor$.
- **Partial Maintenance**: If stored resources are insufficient to cover full maintenance, all available resources are expended for proportional partial repairs: $\text{Damage Repaired} = \left\lfloor \text{Max Repair} \times \left(\frac{\text{Stored Resources}}{\text{Resource Cost}}\right) \right\rfloor$. Unmanned sensor probes safely bypass crewed maintenance formulas.

### 5. Atmospheric Propellant Harvesting (Gas Giant Skimming)
Gas giant planets function as natural, limitless propellant refueling hubs for orbital naval forces. During each turn update, vessels stationed in low orbit around a gas giant world automatically scoop volatile atmospheric hydrogen to replenish fuel tanks:

| Vessel Class | Turn Update Fuel Harvested | Strategic Operational Role |
| :--- | :---: | :--- |
| **Tanker (`t`)** | **$+100.0\text{ fuel}$** | Deep-space mobile filling station; harvests high-volume fuel loads for fleet replenishment. |
| **Habitat (`H`)** | **$+200.0\text{ fuel}$** | Giant orbital biome; harvests massive fuel volumes to sustain population life support and synthesis. |
| **Space Station (`S`)** | **$+100.0\text{ fuel}$** | Orbital defense and staging depot; maintains permanent fuel stockpiles for passing warships. |
| **Standard Starships** | **$+5.0\text{ fuel}$** | Scout craft, transports, and combatants maintain basic maneuvering reserves without tanker support. |

Harvested fuel increases the vessel's operational mass dynamically ($+0.05\text{ mass per fuel unit}$), automatically updating launch thrust and hyperjump requirements.

---

## 5. Specialized Naval Equipment and Planetary Geoengineering

Certain starship classes are equipped with advanced scientific, industrial, or biological subsystems that alter planetary environments, manufacture munitions, or incubate populations during turn updates.

### Space Mirrors and Solar Redirection
Space Mirrors are massive orbital reflector arrays designed to redirect stellar radiation onto planetary biospheres for terraforming, heating, or climate stabilization:
- **Stellar Alignment**: A space mirror must be actively aimed at its host star to function. Mirrors in an unaimed standby mode do not redirect energy.
- **Planetary Thermal Modification**: When stationed in planetary orbit, the mirror focuses stellar energy into the target world's upper atmosphere: $\Delta T = \left\lfloor \frac{\text{Solar Radiation} \times \text{Mirror Efficiency}}{\max(1, \text{Target Planet Radius})} \right\rfloor$. Mirrors can be configured to heat freezing worlds or shaded to cool overheated greenhouse planets toward species-compatible equilibrium temperatures.

### Atmosphere Processors
Atmosphere Processors perform large-scale planetary geoengineering by converting ambient gases into breathable atmosphere:
- **Atmospheric Modification**: Active processors operating on planetary surfaces modify local atmospheric gas concentrations (methane, oxygen, carbon dioxide, helium, nitrogen, sulfur) by calibrated per-segment increments.
- **Safety Clamping**: Planetary atmospheric concentrations and toxicity ratings are strictly bounded within $[0\%, 100\%]$ to prevent ecological collapse or unphysical gas densities.

### Orbital Habitats and Population Incubators
Orbital Habitats function as specialized bioship incubators capable of generating civilian population in deep space or orbit:
- **Incubator Operations**: Active habitats consume stored fuel and raw minerals to synthesize life-support biomass and incubate new colonists: $\Delta \text{Population} = \left\lfloor \text{Incubation Rate} \times \frac{\text{Current Population}}{\text{Maximum Crew Capacity}} \right\rfloor$.
- **Dynamic Displacement**: As new colonists are generated, the ship's operational mass increases proportionally based on the biological body mass of the incubated species ($`M_{\text{race}}`$).

### Weapon Plants and Munitions Manufacturing
Weapon Plants are automated manufacturing modules that convert raw industrial resources into destructive ordnance (`destruct`):
- **Munitions Synthesis**: Each turn segment, operational weapon plants produce new destructive ordnance: $\Delta \text{Destruct} = \min\Big(\text{Available Crew Staffing}, \text{Stored Resources}, \text{Stored Fuel} \times 2, \text{Unallocated Ammo Capacity}\Big)$.
- **Resource Depletion**: Synthesizing ammo consumes resources and fuel at a $1:1$ resource and $0.5:1$ fuel ratio, dynamically adjusting total vessel mass.

### Biological Spore Pods and Climate Modifiers
- **Spore Pods**: Bio-seeding craft capable of dispersing alien spores across planetary sectors. In multi-planet star systems, unmanned biological pods select target worlds across the system to initiate planetary seeding.
- **Canisters and Greenhouses**: Specialized payload canisters and greenhouse modules release dense greenhouse agents to raise planetary temperatures or stabilize atmospheric pressure.

---

## 6. Naval Weapon Systems, Battery Calibers, and Tactical Fire Control

Starships engage in tactical naval combat through modular kinetic gun batteries, directed-energy beam weapons, and stored destructive munitions.

### Dual Battery Architecture

Warships support up to two distinct weapon installations: a **Primary Battery** and a **Secondary Battery**. Each battery operates with independent mount capacity (gun count) and an assigned weapon **Caliber** (Light, Medium, or Heavy). A battery with zero functional guns is always uncalibrated (None), and an uncalibrated mount cannot hold functional guns:

```mermaid
flowchart LR
    Ship["Naval Vessel\nOperational Combat Mode"] --> Switch{"Active Battery Mode"}
    Switch -->|"PRIMARY"| Prim["Primary Battery\nOperational Mounts & Caliber"]
    Switch -->|"SECONDARY"| Sec["Secondary Battery\nOperational Mounts & Caliber"]
    Switch -->|"NONE"| Off["Weapons Standby / Offline\nActive Firepower = 0"]
```

### Weapon Calibers and Caliber Multipliers

Gun calibers dictate engagement tracking precision, structural damage scaling, critical hit vulnerability, and physical displacement weight:

| Caliber Designation | Caliber Multiplier ($K$) | Relative 50% Tracking Range ($c \propto 1/K$) | Displacement Mass | Tactical Characteristics & Fleet Role | Ammo Profile |
| :--- | :---: | :---: | :---: | :--- | :--- |
| **None / Unarmed** | $0$ | N/A | $0.0$ mass / gun | Unarmed battery mount or depleted weapon slot. | 0 destruct / round |
| **Light Guns** | $1$ | **$3.0\times$** (High precision) | $0.2$ mass / gun | High tracking precision against agile strike craft and missiles; rapid point defense. | 1 destruct / round |
| **Medium Guns** | $2$ | **$1.5\times$** (Moderate precision) | $0.4$ mass / gun | General-purpose fleet battery; balanced tracking and damage for destroyers and cruisers. | 1 destruct / round |
| **Heavy Guns** | $3$ | **$1.0\times$** (Close-range / large targets) | $0.6$ mass / gun | Heavy capital ship spinal mounts and siege cannons; massive kinetic impact against armored hulls. | 1 destruct / round |

### Active Battery Selection and Standby Modes

During combat encounters, a vessel's tactical fire control directs kinetic broadsides through the currently selected active battery:
- **Primary Battery**: Fire control is routed through the primary battery mount.
- **Secondary Battery**: Fire control is routed through the secondary battery mount.
- **Standby / Disarmed**: All kinetic batteries remain offline with zero offensive output (essential for unarmed transports, stealth vessels, or factories during retooling).
- **Empty Battery Fallback**: If the currently selected battery contains zero operational guns (either built without guns or reduced to zero by combat damage), tactical fire control treats the weapon system as offline—the ship cannot fire kinetic ordnance or retaliate.

Captains select the active battery using the `order` command:
```text
order <ship> primary [<salvo>]    # Activates primary battery (optional salvo cap)
order <ship> secondary [<salvo>]  # Activates secondary battery (optional salvo cap)
order <ship> none                 # Places kinetic weapons on standby
```

### Automated Retaliation and Salvo Limits

Warships can be integrated into automated fleet defense networks using standing retaliation orders:
- **Retaliation Toggle (`order <ship> retaliate on|off`)**: Enables or disables automated defensive counter-fire when struck by hostile fire.
- **Salvo Cap (`order <ship> salvo <guns>`)**: Regulates ammunition expenditure by capping the maximum number of guns fired per defensive volley (clamped to the active battery's operational gun count).
- **Laser Retaliation (`order <ship> laser on <strength>`)**: Configures the vessel to return fire using combat lasers instead of kinetic guns (consuming $2.0\text{ fuel per strength point}$, requiring an operational propulsion crystal).

#### Retaliation Gating and Firepower Resolution
Defensive kinetic counter-fire resolves automatically before subsequent tactical commands:
1. **Operational Readiness**: The ship must be active and operational. Ships immobilized by radiation sickness cannot return fire.
2. **Mobility Prerequisite**: The ship must have a positive base propulsion speed ($> 0$), or be landed on a planetary surface. Unlanded orbital platforms and space stations in deep space cannot retaliate.
3. **Crew Staffing Constraint**: Standard warships require $1$ living crew member per active gun. Automated AI warships (such as Berserkers) and specialized strike craft (Fighters and Armored Fighting Vehicles) are exempt from this staffing limit.
4. **Effective Firepower**:
   $$\text{Defensive Counter-Fire} = \min\Big(\text{Programmed Salvo Limit}, \text{Active Battery Gun Count}, \text{Effective Crew Staffing}, \text{Stored Destruct Ammo}\Big)$$

### Combat Collateral Damage and Battery Depletion

Penetrating hostile fire inflicts critical subsystem damage and collateral casualties:
- **Collateral Probability**: For every hit that penetrates defensive armor, each carried colonist, troop, primary gun, and secondary gun faces an independent casualty risk:
  $$P(\text{Casualty or Gun Loss}) = \frac{\text{Inflicted Damage Percentage}}{100}$$
- **Battery Depletion & Caliber Degradation**: When collateral damage destroys the last remaining gun in a battery, its count reaches zero and its caliber automatically degrades to None. If the depleted battery was the active battery, the ship immediately loses its ability to return fire until repaired at a shipyard or switched to an alternate functional battery.

### Directed Energy Weapons and Munitions

In addition to kinetic gun batteries, vessels can mount specialized energy projection systems:
- **Combat Lasers**: Direct-fire optical beam weapons providing instantaneous, armor-piercing point defense and short-range interception.
- **Concentrated Energy Weapons (CEW)**: High-yield particle and plasma projectors with dedicated beam ratings and tunable focus ranges.
- **Destructive Munitions (Destruct)**: Physical kinetic warheads and explosive ordnance stored in cargo bays, consumed proportionally with each kinetic battery salvo and planetary bombardment strike.

### Naval Batteries vs. Planetary Defense Installations

A critical distinction in empire defense architectures is the separation between mobile naval systems and fixed planetary installations:
- **Naval Batteries**: Mobile ship-mounted weapon systems utilizing active battery switching and vessel-stored destruct munitions.
- **Planetary Defense Batteries**: Ground-based surface defensive installations permanently mounted across planetary sectors, defending colonies from landing assaults and returning retaliatory fire during orbital bombardment runs.

---

## 7. Point Defense, Interception, and Autonomous Combat

Planetary and naval engagements feature autonomous defensive networks, interceptor batteries, and automated orbital bombardment systems.

```mermaid
flowchart TD
    Target["Incoming Threat / Target Detected"] --> Type{"Engagement Type"}
    Type -->|"Hostile Missile or Mine"| ABM["Anti-Ballistic Missile (ABM)\nScan for unallied ordnance & intercept"]
    Type -->|Naval Intruder| Mine["Proximity Mine\nDetonate on unallied ships entering range"]
    Type -->|Orbital Bombardment| PDNCheck{"Are Planetary Defense\nNetworks (PDNs) Present?"}
    PDNCheck -->|Yes| Cancel["Bombardment Deterred\nCancel strike & alert commanding governor"]
    PDNCheck -->|No| Bombard["Berserker Orbital Strike\nPrioritize war targets & saturation bomb surface"]
```

### Point Defense Networks (PDNs) and Bombardment Deterrence
Point Defense Networks (PDNs) are specialized heavy defense installations stationed on planetary surfaces or in low orbit:
- **Strategic Deterrence**: The presence of any operational, unallied PDN on a planet acts as an absolute strategic deterrent against automated Berserker saturation bombing. Automated bombardment runs are immediately aborted upon detecting active foreign PDNs.

### Autonomous Berserker Saturation Bombardment
Automated Berserker warships orbiting foreign planets execute tactical saturation bombardment against surface colonies:
- **Targeting Priority**:
  1. Active colonies belonging to empires with which the ship's empire is **at war**.
  2. Colonies belonging to a **specifically programmed target species**.
  3. Any unallied foreign colony on the planetary surface.
- **Bombardment Firepower**: Effective orbital strike power is determined by operational gun mounts, structural damage, and available destructive ordnance: $\text{Strike Power} = \min\left(\left\lfloor \text{Template Gun Mounts} \times \frac{100 - \text{Damage}}{100} \right\rfloor, \text{Stored Destruct Ammo}\right)$.
- **Sector Devastation & Retaliation**: The bombardment converts target sectors into nuclear wasteland, reduces planetary population, and expends destruct ammo. Defending surface batteries return retaliatory ground fire, and automated alert bulletins are dispatched to the planetary governors of all affected empires.

### Proximity Minefields
Proximity Mines are stationary spatial munitions deployed in star systems or planetary orbits:
- **Autonomous Detonation**: Mines continuously monitor local space for moving vessels. When an unallied vessel enters triggering range, the mine detonates its full destructive payload.
- **Alliance Safety**: Allied and coalition fleets sharing friendly diplomatic relations safely navigate through friendly minefields without triggering detonations.

### Anti-Ballistic Missile (ABM) Interception
ABM platforms provide automated point defense against incoming space-to-space ordnance:
- **Threat Identification**: ABM batteries continuously scan local orbital tracks for incoming missiles and drifting mines belonging to hostile or unallied empires.
- **Precision Interception**: Upon detecting hostile ordnance, the ABM fires interceptor rounds to neutralize the threat before it reaches target vessels, while sparing friendly missiles and allied torpedoes.

---

## 8. Imperial Power and the Galactic Census

During turn updates, active ships report their operational readiness to Imperial Intelligence and the Galactic Census:
- **Empire Power Ratings**: Active starships contribute directly to an empire's global strength rating based on hull count, propellant reserves, mineral stockpiles, carried destructive ordnance, colonist populations, and military troop strength.
- **Demographic Distribution**: Census reports aggregate vessel counts and carried populations across deep space corridors and localized star systems, reflecting imperial expansion across the galaxy.

---

## See Also
- [Ship Classes and Construction Catalog](ship_types.md)
- [Tactical Combat, Naval Gunnery, and Planetary Warfare](combat.md)
- [Interstellar Navigation, Propulsion, and Hyperspace Mechanics](navigation.md)
- [Stellar Mechanics, Spectral Classes, and Nova Lifecycles](stars.md)
- [Planetary Mechanics, Colonization, and Surface Topography](planets.md)
- [Planetary Simulation Engine and Sector Dynamics](planetary_simulation.md)
- [Governance, Capitals, and Imperial Administration](governance.md)
- [Imperial Economy, Planetary Stockpiles, and Technology Investment](economy.md)
- [Turn Simulation Lifecycle and Scheduling](turn_cycle.md)
- [Autonomous Machine AI, Von Neumann Probes, and Berserker Warships](von_neumann.md)
