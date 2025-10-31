// SPDX-License-Identifier: Apache-2.0

export module gblib:services;

import dallib;
import :repositories;
import :types;
import std.compat;

// Forward declaration
export class EntityManager;

// RAII wrapper for entities - auto-saves on destruction if modified
export template <typename T>
class EntityHandle {
  EntityManager* manager;
  T* entity;
  std::function<void(const T&)> save_fn;
  bool dirty = false;

 public:
  EntityHandle(EntityManager* mgr, T* ent, std::function<void(const T&)> save)
      : manager(mgr), entity(ent), save_fn(std::move(save)) {}

  ~EntityHandle() {
    if (dirty && entity) {
      save_fn(*entity);
    }
    // Note: EntityManager will be notified via release mechanism
  }

  // Delete copy, allow move
  EntityHandle(const EntityHandle&) = delete;
  EntityHandle& operator=(const EntityHandle&) = delete;
  EntityHandle(EntityHandle&&) noexcept = default;
  EntityHandle& operator=(EntityHandle&&) noexcept = default;

  // Non-const access marks entity as dirty
  T& operator*() {
    dirty = true;
    return *entity;
  }
  const T& operator*() const { return *entity; }

  T* operator->() {
    dirty = true;
    return entity;
  }
  const T* operator->() const { return entity; }

  T* get() {
    dirty = true;
    return entity;
  }
  const T* get() const { return entity; }

  // Explicit read-only access (doesn't mark dirty)
  const T& read() const { return *entity; }

  // Force save without waiting for destructor
  void save() {
    if (entity && dirty) {
      save_fn(*entity);
      dirty = false;
    }
  }
};
