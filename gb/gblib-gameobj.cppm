// SPDX-License-Identifier: Apache-2.0

export module gblib:gameobj;

import :types;
import :race;
import :sessionregistry;  // For SessionRegistry interface
import std.compat;

// Forward declaration for EntityManager (defined in gblib:services)
export class EntityManager;

export class GameObj {
public:
  EntityManager& entity_manager;  ///< Entity lifecycle manager
  std::stringstream out;          ///< Output stream (temporary - Step 4A)
  const Race* race = nullptr;     ///< Pointer to current player's race (valid
                                  ///< during command execution)
  SessionRegistry& session_registry;  ///< Session registry for notifications

  // Public utility fields (direct access retained for legacy code patterns)
  double lastx[2] = {0.0, 0.0};
  double lasty[2] = {0.0, 0.0};
  double zoom[2] = {0.5, 0.5};  ///< last coords for zoom

  // Constructor for new Server-based architecture
  explicit GameObj(EntityManager& em, SessionRegistry& registry)
      : entity_manager(em), session_registry(registry) {}

  GameObj(const GameObj&) = delete;
  GameObj& operator=(const GameObj&) = delete;

  // Getters - return local storage
  player_t player() const {
    return player_;
  }
  governor_t governor() const {
    return governor_;
  }
  bool god() const {
    return god_;
  }
  bool disconnect_requested() const {
    return disconnect_requested_;
  }
  bool shutdown_requested() const {
    return shutdown_requested_;
  }
  starnum_t snum() const {
    return snum_;
  }
  planetnum_t pnum() const {
    return pnum_;
  }
  shipnum_t shipno() const {
    return shipno_;
  }
  ScopeLevel level() const {
    return level_;
  }

  // Setters - update local storage
  void set_player(player_t p) {
    player_ = p;
  }
  void set_governor(governor_t g) {
    governor_ = g;
  }
  void set_god(bool g) {
    god_ = g;
  }
  void set_disconnect_requested(bool value) {
    disconnect_requested_ = value;
  }
  void set_shutdown_requested(bool value) {
    shutdown_requested_ = value;
  }
  void set_snum(starnum_t s) {
    snum_ = s;
  }
  void set_pnum(planetnum_t p) {
    pnum_ = p;
  }
  void set_shipno(shipnum_t s) {
    shipno_ = s;
  }
  void set_level(ScopeLevel l) {
    level_ = l;
  }

private:
  // All state stored locally
  player_t player_ = 0;
  governor_t governor_ = 0;
  bool god_ = false;
  bool disconnect_requested_ = false;
  bool shutdown_requested_ = false;
  starnum_t snum_ = 0;
  planetnum_t pnum_ = 0;
  shipnum_t shipno_ = 0;
  ScopeLevel level_ = ScopeLevel::LEVEL_PLAN;
};
