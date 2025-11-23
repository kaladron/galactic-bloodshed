// SPDX-License-Identifier: Apache-2.0

import dallib;
import gblib;
import commands;
import std.compat;

#include <cassert>

int main() {
  Database db(":memory:");
  initialize_schema(db);
  EntityManager em(db);
  JsonStore store(db);

  // Create payer race via repository
  Race payer{};
  payer.Playernum = 1;
  payer.name = "Payer";
  payer.Guest = false;
  payer.governor[0].money = 10000;
  payer.governor[1].money = 5000;
  RaceRepository races(store);
  races.save(payer);

  // Create payee race via repository
  Race payee{};
  payee.Playernum = 2;
  payee.name = "Payee";
  payee.Guest = false;
  payee.governor[0].money = 1000;
  races.save(payee);

  // Test: Pay 500 from player 1 to player 2
  {
    auto payer_handle = em.get_race(1);
    auto payee_handle = em.get_race(2);
    auto& p = *payer_handle;
    auto& a = *payee_handle;

    int amount = 500;
    p.governor[0].money -= amount;
    a.governor[0].money += amount;
  }

  // Verify: Money transferred
  {
    const auto* saved_payer = em.peek_race(1);
    const auto* saved_payee = em.peek_race(2);
    assert(saved_payer);
    assert(saved_payee);
    assert(saved_payer->governor[0].money == 9500);
    assert(saved_payee->governor[0].money == 1500);
    std::println("✓ Money transfer saved correctly");
  }

  // Test: Large transfer
  {
    auto payer_handle = em.get_race(1);
    auto payee_handle = em.get_race(2);
    auto& p = *payer_handle;
    auto& a = *payee_handle;

    int amount = 5000;
    p.governor[0].money -= amount;
    a.governor[0].money += amount;
  }

  // Verify: Large transfer completed
  {
    const auto* saved_payer = em.peek_race(1);
    const auto* saved_payee = em.peek_race(2);
    assert(saved_payer);
    assert(saved_payee);
    assert(saved_payer->governor[0].money == 4500);
    assert(saved_payee->governor[0].money == 6500);
    std::println("✓ Large transfer saved correctly");
  }

  // Test: Transfer from governor (not leader)
  {
    auto payer_handle = em.get_race(1);
    auto payee_handle = em.get_race(2);
    auto& p = *payer_handle;
    auto& a = *payee_handle;

    int amount = 1000;
    p.governor[1].money -= amount;  // From governor 1
    a.governor[0].money += amount;  // To leader
  }

  // Verify: Governor transfer completed
  {
    const auto* saved_payer = em.peek_race(1);
    const auto* saved_payee = em.peek_race(2);
    assert(saved_payer);
    assert(saved_payee);
    assert(saved_payer->governor[1].money == 4000);
    assert(saved_payee->governor[0].money == 7500);
    std::println("✓ Governor transfer saved correctly");
  }

  // Test: Multiple sequential transfers
  {
    for (int i = 0; i < 5; i++) {
      auto payer_handle = em.get_race(1);
      auto payee_handle = em.get_race(2);
      auto& p = *payer_handle;
      auto& a = *payee_handle;

      int amount = 100;
      p.governor[0].money -= amount;
      a.governor[0].money += amount;
    }
  }

  // Verify: All transfers accumulated
  {
    const auto* saved_payer = em.peek_race(1);
    const auto* saved_payee = em.peek_race(2);
    assert(saved_payer);
    assert(saved_payee);
    assert(saved_payer->governor[0].money == 4000);  // 4500 - 500
    assert(saved_payee->governor[0].money == 8000);  // 7500 + 500
    std::println("✓ Multiple transfers accumulated correctly");
  }

  // Test: Zero balance scenarios
  {
    auto payer_handle = em.get_race(1);
    auto& p = *payer_handle;
    p.governor[0].money = 0;
  }

  // Verify: Zero balance saved
  {
    const auto* saved = em.peek_race(1);
    assert(saved);
    assert(saved->governor[0].money == 0);
    std::println("✓ Zero balance saved correctly");
  }

  std::println("All pay tests passed!");
  return 0;
}
