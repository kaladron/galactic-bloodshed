# Economy, Taxation, and Technology

## Overview

The economy of Galactic Bloodshed operates at the intersection of planetary colonies, system governor treasuries, and racial technology advancement.

## Planetary Stockpiles

Each planet maintains individual commodity and resource stockpiles for every inhabiting player (`plinfo`):
- **Fuel**: Used for powering planetary defenses, launching ships, and fueling engines.
- **Resources**: Raw materials required for constructing ships, installations, and ground weapons.
- **Destructive Potential (Destruct)**: Munitions used for ship weapons and planetary defense guns.
- **Crystals**: Rare crystalline materials essential for advanced technologies, sensors, and hyperspace drives.

During each turn update, planetary production deposits newly harvested outputs into these stockpiles via `deposit_production()`.

## Taxation System

Colonists on settled planets generate financial revenue for their governing star system.

### Setting Tax Rates
Players use the `tax` command to inspect or adjust tax policies:
- `tax`: Displays current active tax rate and target tax rate.
- `tax <rate>`: Sets the target tax rate ($0\%$ to $100\%$).

### Revenue Formula
Turn tax revenue is calculated as:
$$\text{Revenue} = \text{round\_rand}\left(0.2 \times \frac{\text{tax}\%}{100} \times \text{population}\right) = \frac{\text{tax}\% \times \text{population}}{5}$$

Tax revenue is deposited directly into the treasury of the governor managing the host star system.

### Rate-Limiting Policy (+5% Max Increase)
To prevent severe economic and social shock to populations:
- **Tax Increases**: Capped at a maximum increase of **$+5\%$ per turn update** towards the target rate.
- **Tax Decreases**: Take effect **immediately** on the subsequent turn update.

### Consequences of Taxation
- **Metabolism Impact**: Higher taxes strain the populace, reducing effective birthrate and metabolism:
  $$\text{metabolism}_{\text{eff}} = \text{metabolism}_{\text{base}} \times \left(1 - \frac{\text{tax}\%}{100}\right)$$
- **Insurgency Vulnerability**: High tax rates create public discontent, significantly increasing the success odds of enemy `insurgency` operations.

## Technology Investment

Scientific research is budgeted per planet and financed by the local star system governor.

### Investment Budgeting
Players allocate funding on a planet using the `technology` command (`technology <money>`).

### Processing Research
During turn updates:
1. **Treasury Check**: The system governor's treasury must contain at least `tech_invest` funds. If insufficient, research fails for that turn (`prod_tech = 0`).
2. **Deduction**: The investment is deducted from the governor's treasury and recorded under `gov.cost_tech`.
3. **Advancement Calculation**: Tech points generated are computed based on investment and planetary population (`tech_prod(invest, popn)`).
4. **Global Advancement**: The generated research points are added directly to the empire's global technology rating (`race.tech`).

## Central Authority Prerequisite

All tax collection and technology investment require an active **Governmental Center** (`race.has_government_center()`). In a state of anarchy (`Gov_ship == 0`), no taxes are collected and all tech investments are suspended across the entire empire.

## See Also
- [Governance and Administration](governance.md)
- [Planets and Colonization](planets.md)
