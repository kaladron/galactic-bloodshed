// SPDX-License-Identifier: Apache-2.0

/// \file treasury.cc
/// \brief Treasury command implementation.

module;

import gb.entities;
import gb.presentation;
import gb.services;

module commands;

namespace GB::commands {
bool treasury(const command_t&, GameObj& g) {
  const auto& gov = g.current_governor();
  g.present(GB::presentation::TreasuryViewModel{
      .total_income = gov.income + gov.profit_market,
      .total_costs = gov.maintain + gov.cost_tech + gov.cost_market,
      .profit_market = gov.profit_market,
      .cost_market = gov.cost_market,
      .income_taxes = gov.income,
      .cost_tech = gov.cost_tech,
      .maintain = gov.maintain,
      .money = gov.money,
  });
  return true;
}

const CommandDescriptor treasury_cmd{
    .name = "treasury",
    .roles = {},
    .scopes = AllowedScopes::any(),
    .ap = APCost::free(),
    .min_args = 1,
    .syntax = "treasury",
    .description = "Display current governor treasury, income, and costs",
    .handler = &treasury,
};

}  // namespace GB::commands
