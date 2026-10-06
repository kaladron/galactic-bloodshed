// SPDX-License-Identifier: Apache-2.0

/// \file fuel.cc
/// \brief Fuel consumption and travel time estimation functions.

module;

import std;

module gb.mechanics;

namespace {

struct ArrivalEstimate {
  ArrivalTimeStatus status;
  std::time_t time;
};

ArrivalEstimate compute_arrival_estimate(const ServerState* state,
                                         const segments_t segs) {
  if (!state) {
    return {
        .status = ArrivalTimeStatus::ServerStateUnavailable,
        .time = 0,
    };
  }

  if (state->segments == 0 || state->nsegments_done > state->segments) {
    return {
        .status = ArrivalTimeStatus::SegmentDiscrepancy,
        .time = 0,
    };
  }

  const std::time_t additional_segs =
      (segs > 0) ? static_cast<std::time_t>(segs - 1) : 0;
  const std::time_t update_mins =
      static_cast<std::time_t>(state->update_time_minutes);
  const std::time_t effective_time =
      (state->segments == 1)
          ? state->next_update_time + (additional_segs * update_mins * 60)
          : state->next_segment_time +
                (additional_segs *
                 (update_mins / static_cast<std::time_t>(state->segments)) *
                 60);
  return {
      .status = ArrivalTimeStatus::Available,
      .time = effective_time,
  };
}

bool has_reached_trip_destination(const SimulatedShip& tmpship,
                                  const UniverseCoordinates dest_coords) {
  const double tmpdist = tmpship.coordinates().distance_to(dest_coords);
  switch (tmpship.whatdest()) {
    case ScopeLevel::LEVEL_STAR:
      return tmpdist <= SYSTEMSIZE;
    case ScopeLevel::LEVEL_PLAN:
      return tmpdist <= PLORBITSIZE;
    case ScopeLevel::LEVEL_SHIP:
      return tmpdist <= DIST_TO_LAND;
    default:
      return true;
  }
}

}  // namespace

/**
 * @brief Computes fuel consumption details and estimated arrival time for a
 * simulated trip.
 *
 * @param em The EntityManager for reading server segment state.
 * @param dist The total trip distance.
 * @param fuel The amount of fuel consumed.
 * @param grav The planetary gravity factor at launch (0.0 if already in space).
 * @param mass The ship's mass at launch.
 * @param segs The number of movement segments required.
 * @param plan_buf The launch planet path string.
 * @return Structured TripEstimate for presentation rendering.
 */
TripEstimate compute_trip_estimate(EntityManager& em, const double dist,
                                   const double fuel, const double grav,
                                   const double mass, const segments_t segs,
                                   const std::string_view plan_buf) {
  const auto [arrival_status, estimated_arrival_time] =
      compute_arrival_estimate(em.peek_server_state(), segs);

  return TripEstimate{
      .distance = dist,
      .segments = segs,
      .fuel_used = fuel,
      .launch_gravity_fuel =
          (grav > 0.00) ? (grav * mass * LAUNCH_GRAV_MASS_FACTOR) : 0.0,
      .launch_planet_name =
          (grav > 0.00) ? std::string(plan_buf) : std::string{},
      .arrival_status = arrival_status,
      .estimated_arrival_time = estimated_arrival_time,
  };
}

/**
 * @brief Performs a trip for a ship to a destination.
 *
 * This function calculates the number of segments required for a ship to reach
 * a destination. The ship's fuel, gravity factor, and starting coordinates are
 * used to determine the trip details.
 *
 * @param tmpdest The temporary destination place.
 * @param tmpship The ship to perform the trip.
 * @param fuel The amount of fuel available for the trip.
 * @param gravity_factor The gravity factor affecting the ship's movement.
 * @param dest_coords The universe coordinates of the destination.
 * @param entity_manager The EntityManager for entity access.
 *
 * @return A tuple containing a boolean indicating if the trip was resolved
 * successfully and the number of segments taken.
 */
std::tuple<bool, segments_t> do_trip(const Place& tmpdest,
                                     SimulatedShip& tmpship, const double fuel,
                                     const double gravity_factor,
                                     const UniverseCoordinates dest_coords,
                                     EntityManager& entity_manager) {
  const auto* state = entity_manager.peek_server_state();
  if (!state) {
    // Can't do trip calculations without server state
    return {false, 0};
  }

  tmpship.set_simulated_fuel(fuel); /* load up the pseudo-ship */
  domass(tmpship, entity_manager);
  segments_t effective_segment_number = state->nsegments_done;

  /* Launch or undock the ship before setting its destination. */
  if (tmpship.is_landed()) {
    const double gravity_fuel =
        gravity_factor * tmpship.mass() * LAUNCH_GRAV_MASS_FACTOR;
    if (tmpship.fuel() < gravity_fuel) {
      return {false, 0};
    }
    tmpship.consume_fuel(gravity_fuel);
    tmpship.launch_to_orbit(ScopeLevel::LEVEL_PLAN);
  } else if (tmpship.is_docked()) {
    tmpship.undock_from_ship();
  }

  /* Set our temporary destination.... */
  tmpship.set_destination(tmpdest.level, tmpdest.snum, tmpdest.pnum,
                          tmpdest.shipno);

  bool trip_resolved = false;
  segments_t number_segments = 0; /* Reset counter. */

  while (!trip_resolved) {
    domass(tmpship, entity_manager);
    const double fuel_level1 = tmpship.fuel();
    moveship(entity_manager, tmpship,
             (effective_segment_number == state->segments), false, true);
    number_segments++;
    effective_segment_number = (effective_segment_number == state->segments)
                                   ? 1
                                   : (effective_segment_number + 1);
    trip_resolved = has_reached_trip_destination(tmpship, dest_coords);
    if (!trip_resolved && tmpship.fuel() == fuel_level1 &&
        !tmpship.hyper_drive().on) {
      return {false, number_segments};
    }
  }
  return {true, number_segments};
}
