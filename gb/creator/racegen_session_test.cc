// SPDX-License-Identifier: Apache-2.0

/// \file racegen_session_test.cc
/// \brief Unit tests for RacegenSession command parsing, modifications, and
/// interactive execution.

import std;
import gb.creator;
import gb.entities;
import test;

namespace {

void test_default_session_state() {
  std::istringstream in;
  std::ostringstream out;
  GB::creator::RacegenSession session(in, out);

  test::expect_eq(session.spec().name, "Unknown",
                  "default name must be Unknown");
  test::expect_eq(session.spec().home_planet_type, PlanetType::EARTH,
                  "default planet must be Earth");
  test::expect_false(session.spec().metamorph, "default race must be Normal");
  test::expect_eq(session.cost().points_remaining, 1225,
                  "default race points remaining must be 1225");
  test::expect_false(session.should_quit(),
                     "quit must not be requested initially");
}

void test_modify_attributes() {
  std::istringstream in;
  std::ostringstream out;
  GB::creator::RacegenSession session(in, out);

  test::expect_true(session.execute_command("modify mass 1.5"));
  test::expect_eq(session.spec().mass, 1.5, "mass must update to 1.5");

  test::expect_true(session.execute_command("modify birthrate 0.8"));
  test::expect_eq(session.spec().birthrate, 0.8,
                  "birthrate must update to 0.8");

  test::expect_true(session.execute_command("modify fighters 12"));
  test::expect_eq(session.spec().fighters, 12, "fighters must update to 12");

  test::expect_true(session.execute_command("modify metabolism 2.0"));
  test::expect_eq(session.spec().metabolism, 2.0,
                  "metabolism must update to 2.0");

  test::expect_true(session.execute_command("modify fertilize 25%"));
  test::expect_eq(session.spec().fertilize, 25, "fertilize must update to 25%");

  test::expect_true(session.cost().points_remaining < 1225,
                    "points remaining must decrease after buffing attributes");
}

void test_modify_attribute_bounds_rejection() {
  std::istringstream in;
  std::ostringstream out;
  GB::creator::RacegenSession session(in, out);

  // 1. Below minimum: Adventurism minimum is 0.05
  bool ok = session.modify_field("adventurism", "0.01");
  test::expect_false(ok, "must reject adventurism below 0.05");
  test::expect_eq(session.spec().adventurism, 0.4,
                  "adventurism must roll back to 0.4");
  test::expect_true(out.str().contains("Adventurism must be at least 0.05"),
                    "output must contain error message");

  // 2. Above maximum: Fighters maximum is 20
  out.str("");
  ok = session.modify_field("fighters", "50");
  test::expect_false(ok, "must reject fighters above 20");
  test::expect_eq(session.spec().fighters, 4, "fighters must roll back to 4");
  test::expect_true(out.str().contains("Fight may be at most 20.00"),
                    "output must report fight maximum bound error");
}

void test_modify_metadata() {
  std::istringstream in;
  std::ostringstream out;
  GB::creator::RacegenSession session(in, out);

  test::expect_true(session.execute_command("modify name Terran Republic"));
  test::expect_eq(session.spec().name, "Terran Republic",
                  "name with spaces must be preserved");

  test::expect_true(session.execute_command("modify password securepass"));
  test::expect_eq(session.spec().password, "securepass");

  test::expect_true(session.execute_command("modify gov_password govpass123"));
  test::expect_eq(session.spec().governor_password, "govpass123");

  test::expect_true(
      session.execute_command("modify address emperor@galaxy.org"));
  test::expect_eq(session.spec().address, "emperor@galaxy.org");
}

void test_modify_race_type() {
  std::istringstream in;
  std::ostringstream out;
  GB::creator::RacegenSession session(in, out);

  // Switch to metamorph
  test::expect_true(session.execute_command("modify race metamorph"));
  test::expect_true(session.spec().metamorph, "metamorph flag must be true");
  test::expect_true(session.spec().absorb, "absorb must be enabled");
  test::expect_true(session.spec().pods, "pods must be enabled");
  test::expect_true(session.spec().collective_iq,
                    "collective_iq must be enabled");
  test::expect_eq(session.spec().iq, 0, "metamorph active IQ must be 0");
  test::expect_eq(session.spec().iq_limit, 150,
                  "metamorph IQ limit must be 150");

  // Switch back to normal
  test::expect_true(session.execute_command("modify race normal"));
  test::expect_false(session.spec().metamorph, "metamorph flag must be false");
  test::expect_false(session.spec().absorb, "absorb must be disabled");
  test::expect_false(session.spec().pods, "pods must be disabled");
  test::expect_false(session.spec().collective_iq,
                     "collective_iq must be disabled");
  test::expect_eq(session.spec().iq, 150, "normal IQ must be restored");
  test::expect_eq(session.spec().iq_limit, 0, "normal IQ limit must be 0");
}

void test_modify_planet_and_gas_restrictions() {
  std::istringstream in;
  std::ostringstream out;
  GB::creator::RacegenSession session(in, out);

  // Switch to Jovian
  test::expect_true(session.execute_command("modify planet jovian"));
  test::expect_eq(session.spec().home_planet_type, PlanetType::GASGIANT);
  test::expect_eq(session.spec().sector_compatibilities[SectorType::SEC_GAS],
                  1.0, "Jovian home planet must set gas compatibility to 100%");
  test::expect_eq(session.spec().sector_compatibilities[SectorType::SEC_PLATED],
                  0.0, "Jovian home planet must clear plated compatibility");

  // Switch to Water
  test::expect_true(session.execute_command("modify planet water"));
  test::expect_eq(session.spec().home_planet_type, PlanetType::WATER);
  test::expect_eq(session.spec().sector_compatibilities[SectorType::SEC_GAS],
                  0.0, "non-Jovian home planet must clear gas compatibility");
  test::expect_eq(session.spec().sector_compatibilities[SectorType::SEC_PLATED],
                  1.0,
                  "non-Jovian home planet must restore plated compatibility");
}

void test_modify_sector_compatibilities() {
  std::istringstream in;
  std::ostringstream out;
  GB::creator::RacegenSession session(in, out);

  // Set Land to 100%
  test::expect_true(session.execute_command("modify land 100"));
  test::expect_eq(session.spec().sector_compatibilities[SectorType::SEC_LAND],
                  1.0);

  // Set Mountain to 50%
  test::expect_true(session.execute_command("modify mountain 50%"));
  test::expect_eq(session.spec().sector_compatibilities[SectorType::SEC_MOUNT],
                  0.5);

  // Plated sector compatibility is immutable
  out.str("");
  bool ok = session.modify_field("plated", "50");
  test::expect_false(ok, "plated sector modification must be rejected");
  test::expect_true(out.str().contains("Plated sector compatibility is fixed"),
                    "output must explain plated compatibility is fixed");
}

void test_print_output() {
  std::istringstream in;
  std::ostringstream out;
  GB::creator::RacegenSession session(in, out);

  test::expect_true(session.execute_command("print"));
  const std::string text = out.str();
  test::expect_true(text.contains("=== Race Specification ==="),
                    "must contain header");
  test::expect_true(text.contains("Class M"), "must show Class M planet");
  test::expect_true(text.contains("=== Attributes ==="),
                    "must show Attributes");
  test::expect_true(text.contains("=== Sector Compatibilities ==="),
                    "must show Sector Compatibilities");
  test::expect_true(text.contains("Points Remaining: 1225"),
                    "must show Points Remaining");
}

void test_help_command() {
  std::istringstream in;
  std::ostringstream out;
  GB::creator::RacegenSession session(in, out);

  test::expect_true(session.execute_command("help"));
  test::expect_true(
      out.str().contains("Galactic Bloodshed Race Generator Commands"),
      "help must show general commands");

  out.str("");
  test::expect_true(session.execute_command("help fields"));
  test::expect_true(out.str().contains("Modifiable Fields"),
                    "help fields must show modifiable fields list");
}

void test_unknown_command_and_missing_arguments() {
  std::istringstream in;
  std::ostringstream out;
  GB::creator::RacegenSession session(in, out);

  test::expect_true(session.execute_command("unknowncmd"));
  test::expect_true(out.str().contains("Unknown command 'unknowncmd'"));

  out.str("");
  test::expect_true(session.execute_command("modify"));
  test::expect_true(out.str().contains("Usage: modify <field> <value>"));

  out.str("");
  test::expect_true(session.execute_command("modify mass"));
  test::expect_true(out.str().contains("Missing value for field 'mass'"));
}

void test_interactive_run_loop() {
  std::istringstream in("modify mass 2.0\nmodify fighters 8\nquit\n");
  std::ostringstream out;
  GB::creator::RacegenSession session(in, out);

  session.run();

  test::expect_true(session.should_quit(), "quit must be flagged");
  test::expect_eq(session.spec().mass, 2.0, "mass modification must persist");
  test::expect_eq(session.spec().fighters, 8,
                  "fighters modification must persist");
  test::expect_true(out.str().contains("racegen> "), "prompt must be printed");
}

void test_eof_handling() {
  std::istringstream in("modify birthrate 0.75\n");
  std::ostringstream out;
  GB::creator::RacegenSession session(in, out);

  session.run();

  test::expect_false(session.should_quit(),
                     "quit is not set if stream simply reaches EOF");
  test::expect_eq(session.spec().birthrate, 0.75,
                  "birthrate modification must persist before EOF");
}

}  // namespace

int main() {
  test_default_session_state();
  test_modify_attributes();
  test_modify_attribute_bounds_rejection();
  test_modify_metadata();
  test_modify_race_type();
  test_modify_planet_and_gas_restrictions();
  test_modify_sector_compatibilities();
  test_print_output();
  test_help_command();
  test_unknown_command_and_missing_arguments();
  test_interactive_run_loop();
  test_eof_handling();

  std::println(std::cout, "✅ All RacegenSession tests passed!");
  return 0;
}
