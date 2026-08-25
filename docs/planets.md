# Planetary Mechanics and Colonization

## Overview

Planets form the core territory and industrial engine of empires in Galactic Bloodshed. This document describes the environmental dynamics, military mobilization, automated waste cleanup, and slave revolt systems governing planets.

## Climate and Thermal Dynamics

Each planet possesses a natural baseline temperature (`RTEMP`) determined by its star's luminosity and orbital distance.

### Temperature Fluctuations
During each turn update (`Planet::update_climate()`):
$$T_{\text{surface}} = T_{\text{base}} + \Delta_{\text{mirrors}} \pm 5^{\circ}\text{C}$$
- **Natural Variance**: A stochastic variance of $\pm 5^{\circ}\text{C}$ simulates seasonal and atmospheric shifts.
- **Orbital Space Mirrors**: Landed or orbiting orbital mirrors can focus stellar radiation onto the planet to warm cold worlds or reflect it to cool hot worlds.

## Sector Mobilization and Defense Guns

Military readiness on a planet is built from the ground up across individual sectors.

### Combat Readiness (`comread`)
- Individual sectors can be mobilized ($0\%$ to $100\%$) for military duty using the `mobilize` command.
- The average mobilization across all owned sectors determines the planet's overall **combat readiness** (`comread`).

### Planetary Defense Guns (`guns`)
Total mobilization points across all owned sectors translate directly into ground-based planetary defense batteries:
$$N_{\text{guns}} = \min\left(20, \left\lfloor \frac{\text{Total Mobilization Points}}{1000} \right\rfloor\right)$$

- **Capabilities**: Up to 20 defense guns can fire at enemy ships in orbit or landing on the surface using the `defend` command.
- **Ammunition**: Guns consume destructive potential (`destruct`) stored in planetary stockpiles.

## Automated Toxic Waste Management

Planetary industry and orbital bombardment generate toxic waste, raising the planet's toxicity level (`conditions(TOXIC)`).

### Automated Waste Can Fabrication
Players can set a toxicity threshold (`tox_thresh`) via the `toxicity` command:
1. When environmental toxicity meets or exceeds `tox_thresh`, and the colony possesses sufficient resources, the colony automatically spends resources to construct a **Toxic Waste Can** ship (`OTYPE_TOXWC`).
2. Constructing the waste can removes toxicity (up to `TOXMAX`) from the planetary biosphere, locking it into the cannister ship for orbital launch or disposal.

## Enslavement and Slave Revolts

When conquering inhabited enemy worlds, victors can enslave the subjugated population (`enslave` command), forcing them into labor.

### Revolt Trigger Condition
An enslaved population requires active military presence to maintain control. If the master player's population on the planet falls to or below **$0.1\%$ ($1/1000\text{th}$)** of the total planet population:
$$\text{popn}_{\text{master}} \le \left\lfloor \frac{\text{popn}_{\text{total}}}{1000} \right\rfloor$$
a planetary **slave revolt** is automatically triggered (`Planet::is_slave_revolt_triggered()`).

### Consequences of a Slave Revolt
1. **Devastation**: The uprisings cause violent urban and ecological destruction, devastating:
   $$N_{\text{devastated}} = \left\lfloor \frac{\text{popn}_{\text{total}}}{1000} \right\rfloor + 1$$
   random populated sectors, wiping out population and infrastructure.
2. **Regional Intimidation Devastation**: In addition, master-owned sectors on intimidated stars have a $50\%$ chance of being devastated.
3. **Liberation**: The planetary shackles are broken, resetting `slaved_to = 0` via `Planet::free_slaves()`.

## See Also
- [Governance and Administration](governance.md)
- [Economy and Taxation](economy.md)
