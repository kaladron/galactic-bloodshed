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

/// \brief Executes a turn simulation pass (movement segment or full turn
/// update).
/// \param em Entity manager for world queries and mutations.
/// \param reg Session registry for player event notifications.
/// \param update If true, executes a full update; if false, executes a movement
/// segment.
export void do_turn(EntityManager& em, SessionRegistry& reg, bool update);

/// \brief Advances game simulation by one discrete interval (movement segment
/// or full update) based on the current completed segment count.
/// \param entity_manager Database entity manager.
/// \param session_registry Session registry for player notifications.
export void do_next_thing(EntityManager& entity_manager,
                          SessionRegistry& session_registry);

/// \brief Executes a full turn update across all systems, including planetary
/// production, AP distribution, market fulfillment, and tech progression.
/// \param entity_manager Database entity manager.
/// \param session_registry Session registry for player notifications.
/// \param force If true, ignores server pause state and advances immediately.
export void do_update(EntityManager& entity_manager,
                      SessionRegistry& session_registry, bool force = false);

/// \brief Executes a movement simulation segment (ship trajectory steps,
/// repairs, ABM tracking).
/// \param entity_manager Database entity manager.
/// \param session_registry Session registry for player notifications.
/// \param override If true, forces segment execution regardless of segments
/// setting.
/// \param segment Specific target segment index if overriding schedule.
export void do_segment(EntityManager& entity_manager,
                       SessionRegistry& session_registry, int override,
                       int segment);

/// \brief Evaluates victory condition thresholds (controlled planets, victory
/// turns) and broadcasts game-over victory bulletins.
/// \param em Database entity manager.
export void handle_victory(EntityManager& em);

/// \brief Aggregates power block membership statistics across all empires and
/// alliances.
/// \param em Database entity manager.
export void compute_power_blocks(EntityManager& em);

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

/// \brief Fluctuates star solar stability and checks for nova initiation.
/// \param em Entity manager for news notifications.
/// \param s Star entity undergoing stability check.
export void fix_stability(EntityManager& em, Star& s);

/// \brief Schedule status info for display commands.
export struct ScheduleInfo {
  std::string start_buf;    // "Server started  : <time>"
  std::string update_buf;   // "Last Update N : <time>"
  std::string segment_buf;  // "Last Segment N : <time>"
  unsigned int nupdates_done{0};
  std::time_t last_update_time{0};
  std::time_t last_segment_time{0};
};

/// \brief Gets current schedule status for display commands.
export const ScheduleInfo& get_schedule_info();

/// \brief Sets server start time timestamp and formatted status string.
/// \param start_time UNIX epoch timestamp when the server started.
export void set_server_start_time(std::time_t start_time);
