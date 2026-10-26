---
name: command-test-matrix
description: 'Standard 4-way unit testing strategy for player commands in gb/commands/*_test.cc. Covers testing Happy Path, Insufficient AP, Scope/Role Rejection, and Domain Logic Errors with exact AP accounting and state verification.'
user-invocable: false
---

# Command Test Matrix Pattern

Every player command test in `gb/commands/*_test.cc` must implement the **4-Way Test Matrix** to guarantee robust error handling, permission enforcement, and atomic Action Point (AP) accounting.

## The 4-Way Matrix Specification

| Test Case | Scenario Setup | Expected Result | AP Verification |
| :--- | :--- | :--- | :--- |
| **1. Happy Path** | Valid scope, valid role, sufficient AP, valid arguments | Command succeeds, returns `true`, state updated | Star/Univ AP decreased by exact cost |
| **2. Insufficient AP** | Star/Univ AP set to `< cost` (e.g. `0`) | Dispatcher rejects, returns `false`, state untouched | Star/Univ AP remains unchanged |
| **3. Scope / Role Rejection** | Scope set to invalid level (e.g. `LEVEL_UNIV`) or unauthorized role (e.g. `Guest = true`, `governor = 2`) | Dispatcher rejects, returns `false`, state untouched | Star/Univ AP remains unchanged |
| **4. Domain Error** | Bad arguments (e.g. invalid target, ship not landed) | Handler returns `false`, state untouched | Star/Univ AP remains unchanged |

## Test Implementation Template

```cpp
// SPDX-License-Identifier: Apache-2.0

import commands;
import dallib;
import gb.entities;
import gb.services;
import test;
import std;

#include <cassert>

namespace {

void test_command_matrix() {
  TestContext ctx;
  ctx.with_standard_universe();
  auto& registry = get_test_session_registry();
  GameObj g(ctx.em, registry);

  // 1. Setup entities (Race, Star, Planet, Ship)
  ctx.setup_game_obj(g, 1, 1);
  g.set_level(ScopeLevel::LEVEL_PLAN);
  g.set_snum(1);
  g.set_pnum(1);

  // Set initial star AP to 20
  ctx.em.mutate_star(1, [](Star& star) {
    star.AP(1) = 20;
  });

  // --- Case 1: Happy Path ---
  assert(GB::commands::dispatch_command(g, GB::commands::example_cmd, {"example", "arg"}));
  assert(ctx.em.peek_star(1)->AP(1) == 19); // 1 AP deducted

  // --- Case 2: Insufficient AP ---
  ctx.em.mutate_star(1, [](Star& star) {
    star.AP(1) = 0;
  });
  g.out.clear();
  assert(!GB::commands::dispatch_command(g, GB::commands::example_cmd, {"example", "arg"}));
  assert(ctx.em.peek_star(1)->AP(1) == 0); // 0 AP deducted

  // Reset AP for subsequent tests
  ctx.em.mutate_star(1, [](Star& star) {
    star.AP(1) = 20;
  });

  // --- Case 3: Invalid Scope / Role ---
  g.set_level(ScopeLevel::LEVEL_UNIV);
  g.out.clear();
  assert(!GB::commands::dispatch_command(g, GB::commands::example_cmd, {"example", "arg"}));
  assert(ctx.em.peek_star(1)->AP(1) == 20); // 0 AP deducted
  g.set_level(ScopeLevel::LEVEL_PLAN);

  // --- Case 4: Domain Error / Bad Arguments ---
  g.out.clear();
  assert(!GB::commands::dispatch_command(g, GB::commands::example_cmd, {"example", "invalid_arg"}));
  assert(ctx.em.peek_star(1)->AP(1) == 20); // 0 AP deducted
}

}  // namespace

int main() {
  test_command_matrix();
  std::println(std::cout, "✓ example_test passed!");
  return 0;
}
```

## Key Guidelines

1. **Always verify AP invariant**: When a command fails or is rejected, assert that AP is unmodified (`initial == final`).
2. **Use `dispatch_command` directly or `ctx.dispatch()`**: Run tests through the dispatch pipeline to exercise full validation.
3. **Mutate via `ctx.em.mutate_*()`**: Always use monadic `mutate_*` calls to ensure changes persist before subsequent reads.
