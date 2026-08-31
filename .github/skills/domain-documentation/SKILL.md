---
name: domain-documentation
description: 'Author player-facing technical guides in docs/ explaining game mechanics, mathematical formulas, and simulation lifecycles without exposing internal C++ code or implementation details.'
user-invocable: false
---

# Domain & Game Mechanics Documentation

When modernizing or discovering game subsystems (e.g., naval operations, orbital mechanics, economics, planetary ecology, governance, combat simulation), capture and document these domain rules in human-readable markdown guides within `docs/`.

---

## 1. Audience & Documentation Perspective

`docs/*.md` guides are **player-facing technical manuals** written for players and game strategists who want to understand exact game mechanics and subsystem behavior.

### 🚫 Rules of Perspective:
- **NO Source Code Identifiers**: Never use C++ class names, function names, enum tags, or variable types (e.g. avoid `doship()`, `doown()`, `ScopeLevel::LEVEL_UNIV`, `OTYPE_PROBE`, `player_t`, `EntityManager`).
- **Use Canonical Domain Terminology**: Use in-game concepts ("Universe Scope / Deep Space", "Star System Orbit", "Carrier Hangars", "Sensor Probes", "Mobile Factories", "Simulation Segments", "Turn Updates").
- **Pure Gameplay Mechanics**: Focus on cause and effect, tactical trade-offs, resource costs, lifecycle stages, and domain invariants.

---

## 2. Mathematical Rigor & Formulas

Document exact numerical formulas and probability distributions using LaTeX math:

### ⚠️ GitHub Math Formatting Rules:
- **Inline Math in Lists**: When including formulas inside bullet points (`- ...`) or numbered lists (`1. ...`), **ALWAYS use inline math (`$...$`)** directly on the list item line. GitHub's markdown parser fails to render `$$...$$` display blocks nested inside list items.
  ```markdown
  - **Tax Strain**: High taxation depresses effective metabolism: $\text{Metabolism}_{\text{effective}} = \text{Metabolism}_{\text{base}} \times \left(1 - \frac{\text{Tax Rate}}{100}\right)$.
  ```
- **Display Math Blocks**: Use `$$...$$` display blocks **strictly for standalone paragraphs** separated by blank lines before and after:
  ```markdown
  The maximum demographic capacity is calculated as:

  $$\text{Max Population} = \left\lfloor (\text{Efficiency} + 1) \times \text{Fertility} \times 0.01 \times \text{Compatibility} \times \frac{100 - \text{Toxicity}}{100} \right\rfloor$$
  ```
- **Delimiter Spacing**: Do not place spaces immediately after opening `$` or before closing `$` (e.g. `$x = 1$`, not `$ x = 1 $`) to ensure reliable KaTeX rendering.

- **Probabilities**: State exact success and failure distributions (e.g. $P(\text{Immobilized}) = \frac{\text{Radiation}}{100}$).
- **Rates and Scaling**: Express formulas for crew scaling, efficiency ratios, and resource costs:
  $$r_{\text{crew}} = \frac{\text{Current Crew}}{\text{Maximum Crew Capacity}}$$
  $$\text{Max Repair} = \text{Base Repair Rate} \times r_{\text{crew}}$$
  $$\text{Resource Cost} = \left\lfloor 0.005 \times \text{Max Repair} \times \text{Ship Construction Cost} \right\rfloor$$
- **Continuous & Discrete Degradation**: Detail degradation curves, attrition rates, and dissipation formulas (e.g. $\Delta \text{Damage} = \left\lfloor \frac{5 \times \text{Nova Stage}}{(\text{Effective Armor} + 1) \times S} \right\rfloor$).
- **Displacement & Mass Accumulation**: Express dynamic totals encompassing chassis, stored consumables, and nested units.

---

## 3. Visual Lifecycles & Mermaid Diagrams

Use Mermaid flowcharts and state diagrams to illustrate multi-step simulation pipelines, spatial hierarchies, and behavioral state transitions.

### ⚠️ GitHub Mermaid Syntax Rules:
- **Quote Edge Labels with Special Characters**: In flowcharts, any edge label containing parentheses `()`, slashes `/`, percentages `%`, or punctuation **MUST be enclosed in double quotes** (`-->|"Yes (50% Chance)"|` or `-->|"Tax Rate (0-100%)"|`). Unquoted parentheses are misinterpreted as node shape boundaries by Mermaid, breaking GitHub rendering.
- **Quote All Node Text**: Always wrap node labels in double quotes inside brackets: `Node["Text with (parens), / slashes, and \n newlines"]` or `Decision{"Condition <= 100?"}`.

```mermaid
flowchart TD
    Univ["Universe Scope (Deep Space)\nInterstellar Coordinates"] --> Star["Star System Orbit\nHeliocentric Position"]
    Star --> Plan["Planetary Scope\nOrbital Track or Surface Grid"]
    Plan --> Carrier["Carrier Hangars\nDocked Inside Host Ship or Station"]
    Plan --> Check{"Condition Check\nStored Resources >= Build Cost?"}
    Check -->|"Yes (50% Chance)"| Build["Construct Unit"]
    Check -->|No| Standby["Standby Mode"]
```

---

## 4. Standard Document Structure

Every domain guide in `docs/` should follow a structured layout:

1. **Title and Overview**: High-level conceptual summary of the subsystem.
2. **Core Concepts / Reference Frames**: Spatial, political, or economic hierarchies.
3. **Domain Mechanics & Subsystem Breakdown**: Section-by-section walkthrough of specific rules, actions, and constraints.
4. **Mathematical Models & Turn Lifecycles**: Step-by-step turn execution phases with LaTeX formulas.
5. **See Also**: Comprehensive, bidirectional relative markdown links to all related topic guides in `docs/` using exact guide titles.

---

## 5. CMake Registration

Whenever creating a new document in `docs/`:

1. Save the file to `docs/<topic>.md`.
2. Register the filename in `docs/CMakeLists.txt` under `install(FILES ...)`:
   ```cmake
   install(FILES combat.md covert_ops.md diplomacy.md economy.md gb_FAQ.txt
                 geoengineering.md governance.md navigation.md
                 planetary_simulation.md planets.md RACEGEN.COMPILE.HELP
                 RACEGEN.PLAYER.HELP races.md ships.md ship_types.md stars.md
                 turn_cycle.md von_neumann.md
           DESTINATION "${CMAKE_INSTALL_DOCDIR}")
   ```
3. Run `ninja -C build` to verify CMake configuration.
