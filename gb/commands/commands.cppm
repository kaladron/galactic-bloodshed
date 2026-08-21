// SPDX-License-Identifier: Apache-2.0

export module commands;

export import :spec;

import gblib;
import session; // For SessionRegistry full definition

namespace GB::commands {
// Server-level commands (take Session& instead of GameObj&)
// TODO(Step 9): Convert these to (const command_t&, GameObj&) after GameObj can
// access Session
export void who(const command_t&, Session&);
export void emulate(const command_t&, Session&);

// God commands
export bool help(const command_t&, GameObj&);
export extern const CommandDescriptor help_cmd;
export bool quit(const command_t&, GameObj&);
export extern const CommandDescriptor quit_cmd;
export bool purge(const command_t&, GameObj&);
export extern const CommandDescriptor purge_cmd;
export bool shutdown(const command_t&, GameObj&);
export extern const CommandDescriptor shutdown_cmd;

// Regular commands (take GameObj&)
export void analysis(const command_t&, GameObj&);
export void announce(const command_t&, GameObj&);
export void arm(const command_t&, GameObj&);
export void autoreport(const command_t&, GameObj&);
export void bid(const command_t& argv, GameObj&);
export bool bless(const command_t&, GameObj&);
export extern const CommandDescriptor bless_cmd;
export void block(const command_t&, GameObj&);
export bool bombard(const command_t&, GameObj&);
export extern const CommandDescriptor bombard_cmd;
export bool build(const command_t&, GameObj&);
export extern const CommandDescriptor build_cmd;
export bool capture(const command_t&, GameObj&);
export extern const CommandDescriptor capture_cmd;
export bool capital(const command_t&, GameObj&);
export extern const CommandDescriptor capital_cmd;
export bool center(const command_t&, GameObj&);
export extern const CommandDescriptor center_cmd;
export void colonies(const command_t&, GameObj&);
export bool cs(const command_t&, GameObj&);
export extern const CommandDescriptor cs_cmd;
export void declare(const command_t&, GameObj&);
export bool defend(const command_t&, GameObj&);
export extern const CommandDescriptor defend_cmd;
export bool detonate(const command_t& argv, GameObj&);
export extern const CommandDescriptor detonate_cmd;
export bool dissolve(const command_t&, GameObj&);
export extern const CommandDescriptor dissolve_cmd;
export void distance(const command_t&, GameObj&);
export bool dock(const command_t&, GameObj&);
export extern const CommandDescriptor dock_cmd;
export bool assault(const command_t&, GameObj&);
export extern const CommandDescriptor assault_cmd;
export bool dump(const command_t&, GameObj&);
export extern const CommandDescriptor dump_cmd;
export bool enslave(const command_t&, GameObj&);
export extern const CommandDescriptor enslave_cmd;
export void examine(const command_t&, GameObj&);
export void explore(const command_t&, GameObj&);
export bool fire(const command_t&, GameObj&);
export extern const CommandDescriptor fire_cmd;
export bool cew(const command_t&, GameObj&);
export extern const CommandDescriptor cew_cmd;
export bool fix(const command_t&, GameObj&);
export extern const CommandDescriptor fix_cmd;
export void give(const command_t&, GameObj&);
export bool governors(const command_t&, GameObj&);
export extern const CommandDescriptor governors_cmd;
export void grant(const command_t&, GameObj&);
export void highlight(const command_t&, GameObj&);
export bool insurgency(const command_t&, GameObj&);
export extern const CommandDescriptor insurgency_cmd;
export void invite(const command_t&, GameObj&);
export bool jettison(const command_t&, GameObj&);
export extern const CommandDescriptor jettison_cmd;
export bool land(const command_t&, GameObj&);
export extern const CommandDescriptor land_cmd;
export bool launch(const command_t&, GameObj&);
export extern const CommandDescriptor launch_cmd;
export bool load(const command_t&, GameObj&);
export extern const CommandDescriptor load_cmd;
export extern const CommandDescriptor unload_cmd;
export void make_mod(const command_t&, GameObj&);
export void map(const command_t&, GameObj&);
export void mobilize(const command_t&, GameObj&);
export void motto(const command_t&, GameObj&);
export bool move_popn(const command_t&, GameObj&);
export extern const CommandDescriptor move_cmd;
export bool deploy(const command_t&, GameObj&);
export extern const CommandDescriptor deploy_cmd;
export bool mount(const command_t&, GameObj&);
export extern const CommandDescriptor mount_cmd;
export extern const CommandDescriptor dismount_cmd;
export void name(const command_t&, GameObj&);
export void order(const command_t&, GameObj&);
export bool orbit(const command_t&, GameObj&);
export extern const CommandDescriptor orbit_cmd;
export void page(const command_t&, GameObj&);
export void pay(const command_t&, GameObj&);
export void personal(const command_t&, GameObj&);
export void pledge(const command_t&, GameObj&);
export void power(const command_t&, GameObj&);
export void production(const command_t&, GameObj&);
export bool proj_fuel(const command_t&, GameObj&);
export extern const CommandDescriptor fuel_cmd;
export void profile(const command_t&, GameObj&);
export void read_messages(const command_t&, GameObj&);
export void relation(const command_t&, GameObj&);
export bool repair(const command_t&, GameObj&);
export extern const CommandDescriptor repair_cmd;
export void reset(const command_t&, GameObj&);
export void route(const command_t&, GameObj&);
export void rst(const command_t&, GameObj&);
export void tactical(const command_t&, GameObj&);
export void sale(const command_t&, GameObj&);
export bool scrap(const command_t&, GameObj&);
export extern const CommandDescriptor scrap_cmd;
export void send_message(const command_t&, GameObj&);
export bool sell(const command_t&, GameObj&);
export extern const CommandDescriptor sell_cmd;
export void star_locations(const command_t&, GameObj&);
export void survey(const command_t&, GameObj&);
export bool tax(const command_t&, GameObj&);
export extern const CommandDescriptor tax_cmd;
export bool technology(const command_t&, GameObj&);
export extern const CommandDescriptor technology_cmd;
export void tech_status(const command_t&, GameObj&);
export void toggle(const command_t&, GameObj&);
export void toxicity(const command_t&, GameObj&);
export void transfer(const command_t&, GameObj&);
export void treasury(const command_t&, GameObj&);
export void unpledge(const command_t&, GameObj&);
export void upgrade(const command_t&, GameObj&);
export void victory(const command_t&, GameObj&);
export void vote(const command_t&, GameObj&);
export void walk(const command_t&, GameObj&);
export void whois(const command_t&, GameObj&);
export bool zoom(const command_t&, GameObj&);
export extern const CommandDescriptor zoom_cmd;
}  // namespace GB::commands
