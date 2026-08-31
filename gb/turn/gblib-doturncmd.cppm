// SPDX-License-Identifier: Apache-2.0

/// \file gblib-doturncmd.cppm
/// \brief Module interface partition for turn orchestration and scheduling.

export module gblib:doturncmd;

import :gameobj;
import :race;
import :services;
import :star;
import :turnstats;
import :types;
import std;

class SessionRegistry;

export void do_turn(EntityManager&, SessionRegistry&, bool update);
export void do_next_thing(EntityManager&, SessionRegistry&);
export void do_update(EntityManager&, SessionRegistry&, bool = false);
export void do_segment(EntityManager&, SessionRegistry&, int, int);
export void handle_victory(EntityManager&);
export void compute_power_blocks(EntityManager&);
/// \brief Evaluates open interstellar market lots, transfers purchased
/// commodities, charges freight shipping fees, and deposits goods at recipient
/// planets.
/// \param em Entity manager for market queries and mutations.
export void process_market_transactions(EntityManager& em);

/// \brief Computes whether an empire has an active, operational government
/// center.
/// \param race The race to inspect.
/// \param entity_manager Entity manager for ship state validation.
/// \return True if the race has an active, properly docked government center.
export bool compute_governed_status(const Race& race,
                                    EntityManager& entity_manager);

/// \brief Calculates Action Points awarded to an empire in a star system based
/// on planetary population and ship crew presence.
/// \param num_ships Total ship presence / crew scaling in the system.
/// \param popn Total planetary population in the system.
/// \param race Race receiving the Action Points.
/// \param entity_manager Entity manager for governance status evaluation.
/// \return Action points calculated for the star system.
export ap_t compute_star_action_points(int num_ships, population_t popn,
                                       const Race& race,
                                       EntityManager& entity_manager);

/// \brief Distributes universe-level Action Points to all governed empires
/// based on accumulated planetary points.
/// \param entity_manager Entity manager for universe and race mutations.
export void distribute_universe_action_points(EntityManager& entity_manager);

/// \brief Generates combat news bulletins for all ground assaults that occurred
/// during turn simulation segments and resets the assault counters.
/// \param em Entity manager for star/race queries and news dispatch.
export void output_ground_attacks(EntityManager& em);

/// \brief Computes victory points for all empires based on sector ownership,
/// fleet assets, stockpiled resources, treasury funds, and morale scaling.
/// \param entity_manager Database entity manager for entity queries and
/// mutations.
export void calculate_victory_scores(EntityManager& entity_manager);

/// \brief Evaluates the Von Neumann machine collective target across all
/// empires, tracking total aggression and identifying the priority target.
/// \param em Entity manager for universe hitlist queries.
/// \param stats Turn statistics struct storing VN collective memory.
export void update_von_neumann_target(EntityManager& em, TurnStats& stats);

/// \brief Checks whether an empire has attained tech thresholds to unlock
/// specialized capabilities (hyperdrive, laser, cew, vn, tractor beam,
/// transporter, avpm, cloak, wormhole, crystal) and dispatches telegram
/// notices.
/// \param em Entity manager for telegram dispatch.
/// \param r Race entity undergoing tech progression.
export void check_technological_discoveries(EntityManager& em, Race& r);

export void fix_stability(EntityManager& em, Star& s);

/// Schedule status info for display commands
export struct ScheduleInfo {
  std::string start_buf;    // "Server started  : <time>"
  std::string update_buf;   // "Last Update N : <time>"
  std::string segment_buf;  // "Last Segment N : <time>"
  unsigned int nupdates_done;
  std::time_t last_update_time;
  std::time_t last_segment_time;
};

/// Get current schedule status for display
export const ScheduleInfo& get_schedule_info();

/// Set server start time (called once at startup)
export void set_server_start_time(std::time_t start_time);
