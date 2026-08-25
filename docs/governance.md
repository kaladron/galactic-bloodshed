# Governance and Administration

## Overview

In Galactic Bloodshed, imperial governance is centered around a designated capital ship known as the **Governmental Center** (`OTYPE_GOV`). The existence and operational status of this center dictate the empire's ability to generate action points (APs), collect planetary taxes, and fund scientific research.

## The Governmental Center (`Gov_ship`)

Every empire begins with a landed governmental center designated as its seat of government.

### Designation
A player can inspect or re-designate their capital using the `capital` command:
- **Query Capital**: `capital` displays the capital's ship number and operational efficiency.
- **Set Capital**: `capital <#ship>` designates a new landed `OTYPE_GOV` ship as the seat of government (costs 50 AP in the destination star system).

### Capital Efficiency
The efficiency of a governmental center depends on two factors:
- **Crew**: The number of officials/civilians staffing the ship relative to its maximum capacity.
- **Damage**: Structural damage sustained by the ship.

While efficiency reflects governmental health and operational responsiveness, complete loss of the ship causes total governmental collapse.

## State of Anarchy (`Gov_ship == 0`)

If an empire's governmental center is destroyed in combat, scrapped, or lost, the empire enters a **state of anarchy**:
1. **Zero Action Point Production**: The empire generates 0 global and system-level APs during turn updates.
2. **Tax Collection Collapse**: Planetary tax collection is completely disabled (`prod_money = 0`) across every colony in the galaxy.
3. **Research Stoppage**: Technology investment orders on all planets are suspended (`prod_tech = 0`).
4. **Login Notification**: Governors logging in receive immediate warnings that no AP will be generated until a new governmental center is constructed and designated.

## Governors and Administrative Scopes

Empires are divided administratively into star systems managed by governors (`governor_t`):
- **Governor 0 (Prime/Deity/Leader)**: The supreme ruler of the empire, controlling the home system and unassigned stars.
- **System Governors**: Appointed governors assigned to manage specific star systems.
- **Treasuries**: Each governor maintains an independent treasury (`money`), tracking planetary tax income (`income`), ship maintenance costs (`maintain`), technology research expenditures (`cost_tech`), and market transactions (`cost_market` / `profit_market`).

## See Also
- [Economy and Taxation](economy.md)
- [Planets and Colonization](planets.md)
