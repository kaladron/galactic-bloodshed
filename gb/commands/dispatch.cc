// SPDX-License-Identifier: Apache-2.0

/// \file dispatch.cc
/// \brief Centralized player command precondition, transaction, and Guest
/// read-only dispatch pipeline.

module;

import gb.entities;
import gb.presentation;
import gb.services;
import std;

module commands;

namespace GB::commands {

bool dispatch_command(GameObj& g, const CommandDescriptor& desc,
                      const command_t& argv) {
  // Ensure g.race is valid and synchronized with current player
  if (g.player() > 0) {
    g.race = g.entity_manager.peek_race(g.player());
  } else {
    g.race = nullptr;
  }

  // 1. Role & Permission Verification
  if (desc.roles.god_only && !g.god()) {
    g.out << "Only deity can use this command.\n";
    return false;
  }
  if (desc.roles.no_guests && g.race && g.race->Guest) {
    g.out << "Guest races cannot use this command.\n";
    return false;
  }
  if (desc.roles.leader_only && !g.is_leader()) {
    g.out << "Only the leader (Governor 1) may use this command.\n";
    return false;
  }
  if (desc.roles.star_control) {
    try {
      const auto& star = *g.entity_manager.peek_star(g.snum());
      if (!star.control(g.player(), g.governor())) {
        g.out << "You are not authorized to do that in this system.\n";
        return false;
      }
    } catch (const EntityNotFoundError&) {
      g.out << "You are not authorized to do that in this system.\n";
      return false;
    }
  }

  // 2. Scope Verification
  if (!desc.scopes.allows(g.level())) {
    g.out << "Invalid scope for this command.\n";
    return false;
  }

  // 3. Argument Count Check
  if (argv.size() < desc.min_args) {
    g.out << std::format("Syntax: {}\n", desc.syntax);
    return false;
  }

  // 4. Fixed-Cost AP Pre-check
  if (desc.ap.model == APModel::FixedStar) {
    try {
      const auto& star = *g.entity_manager.peek_star(g.snum());
      if (star.AP(g.player()) < desc.ap.amount) {
        g.out << std::format("You don't have {} action points there.\n",
                             desc.ap.amount);
        return false;
      }
    } catch (const EntityNotFoundError&) {
      g.out << std::format("You don't have {} action points there.\n",
                           desc.ap.amount);
      return false;
    }
  } else if (desc.ap.model == APModel::FixedUniv) {
    const auto& univ = *g.entity_manager.peek_universe();
    if (univ.get_AP(g.player()) < desc.ap.amount) {
      g.out << std::format("You need {} universe action points.\n",
                           desc.ap.amount);
      return false;
    }
  }

  // 5. Open Transaction and Execute Command
  if (!desc.handler) {
    return false;
  }

  bool track_json_deltas = false;
  if (g.ui_mode() == UiMode::JSON) {
    if (g.player() > 0) {
      track_json_deltas = true;
      g.entity_manager.begin_mutation_tracking();
    }
  }

  std::optional<EntityManager::Transaction> txn;
  if (desc.transactional) {
    txn.emplace(g.entity_manager.begin_transaction());
    g.session_registry.begin_outbox();
  }

  const auto abort_dispatch = [&]() {
    if (track_json_deltas) {
      g.entity_manager.clear_dirty_entities();
      g.entity_manager.stop_mutation_tracking();
    }
    if (txn) {
      g.session_registry.rollback_outbox();
      txn->rollback();
    }
  };

  const bool guest_read_only = (g.race != nullptr && g.race->Guest && !g.god());
  const std::size_t out_checkpoint = g.out.view().size();

  bool success = false;
  try {
    std::optional<EntityManager::ReadOnlyScope> guest_guard;
    if (guest_read_only) {
      guest_guard.emplace(g.entity_manager);
    }
    success = desc.handler(argv, g);

    // 6. Deduct Fixed AP on success
    if (success && desc.ap.amount > 0) {
      if (desc.ap.model == APModel::FixedStar) {
        g.entity_manager.mutate_star(g.snum(), [&](Star& star) {
          star.AP(g.player()) -= desc.ap.amount;
        });
      } else if (desc.ap.model == APModel::FixedUniv) {
        g.entity_manager.mutate_universe(
            [&](Universe& u) { u.deduct_AP(g.player(), desc.ap.amount); });
      }
    }
  } catch (const ReadOnlyError&) {
    abort_dispatch();
    if (!guest_read_only) {
      throw;
    }
    g.out.truncate(out_checkpoint);
    g.present(GB::presentation::make_output_message(
        "Guest races cannot modify game state.\n"));
    return false;
  } catch (...) {
    abort_dispatch();
    throw;
  }

  if (txn) {
    if (success) {
      txn->commit();
      for (const auto& item : g.session_registry.commit_outbox()) {
        if (!guest_read_only) {
          push_telegram(g.entity_manager, item.player, item.governor,
                        item.message);
        }
      }
    } else {
      g.session_registry.rollback_outbox();
      txn->rollback();
    }
  }

  if (track_json_deltas) {
    g.entity_manager.stop_mutation_tracking();
    if (success) {
      const auto dirty = g.entity_manager.take_dirty_entities();
      g.out << GB::presentation::render_dirty_entities_json(
          g.entity_manager, g.scope_context(), dirty);
    } else {
      g.entity_manager.clear_dirty_entities();
    }
  }

  return success;
}

bool dispatch_command(GameObj& g, const command_t& argv) {
  if (argv.empty()) {
    return false;
  }
  const auto* desc = find_command_descriptor(argv[0]);
  if (!desc) {
    return false;
  }
  return dispatch_command(g, *desc, argv);
}

}  // namespace GB::commands
