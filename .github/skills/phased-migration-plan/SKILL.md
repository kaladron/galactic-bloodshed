---
name: phased-migration-plan
description: 'Author or update a phased plan-*.md document for a multi-step migration or refactor. Use when a change is too large for a single commit, when coordinating database/architecture migrations across many call sites, or when the user asks for a "plan" before implementation. Covers the standard plan structure used in this repo (plan-database, plan-enrol, plan-telegram, plan-strong-id-governor, plan-nogo), per-step completion markers, commit-message convention, and update workflow.'
user-invocable: false
---

# Phased Migration Plan

Large changes in this repo are tracked as `plan-<topic>.md` documents at the workspace root. They serve two audiences: the human reviewer who approves each step, and the agent (or future contributor) executing it. The format is consistent — match it.

## When to Write a Plan

- The change spans more than ~3 files or more than one logical step.
- It crosses architectural layers (DAL ↔ repository ↔ service ↔ command).
- It includes data migration (struct/schema changes that affect existing JSON).
- The user explicitly asks for a plan before implementation.
- A previous attempt got tangled and we want to retry deliberately.

For a single localized fix, skip the plan.

## Standard Sections

```markdown
# <Title>: <One-line goal>

## Overview
Short paragraph: what changes, why, and what stays the same.

## Background / Current State Analysis
What exists today, with file/line references. Include both the obvious
mechanism and any related infrastructure already in place.

## Implementation Steps

### Step 1: <Imperative title>  ✅ COMPLETED  (added on completion)
**Goal**: One sentence.
**Files to modify**: list.
**Changes**: bullets describing the edit.
**Verification**: `ninja -C build` (and/or specific test target).
**Commit message**: `area: short imperative summary`

### Step 2: ...
```

A few hard rules:

1. **One step = one commit = one verification.** Each step must compile on its own and ideally pass tests. Don’t combine steps that the user must review separately.
2. **Suggest the commit message inline.** The user runs `git`; the agent only proposes the message.
3. **Mark completion in place.** When the user reports a step is committed, edit the plan: append `✅ COMPLETED` to the step title and a brief note (commit hash, date, surprises). Don’t delete the original instructions.
4. **No git commands run by the agent.** Per the workspace `steps.md` instructions, the agent never runs `git add` or `git commit`.

## Per-Step Anatomy

```markdown
### Step 3: Migrate usage sites in doturncmd.cc

**Goal**: Replace filesystem flag check with ServerState.nogo.

**File to modify**: `gb/doturncmd.cc`

**Location 1**: `do_update()` (around line 970)

**Before**:
```cpp
bool fakeit = (!force && stat(nogofl.data(), &stbuf) >= 0);
```
**After**:
```cpp
auto state = em.peek_server_state();
bool fakeit = (!force && state->nogo);
```

**Verification**: `ninja -C build && (cd build && ctest)`

**Commit message**: `Migrate nogo check in do_update to ServerState`
```

The Before/After blocks are the most useful element — they let the reviewer pre-validate the change without reading the code first.

## Updating an Existing Plan

When a step is committed:

1. Append `✅ COMPLETED` to the step heading.
2. Add a 1–3 line completion note: date or commit subject, files touched, anything unexpected.
3. **Do not** rewrite the original step body — the historical record is the value.
4. If the next step needs adjustment based on what was learned, update *that* step’s instructions, not the completed one.

When a step turns out to be wrong:

1. Strike-through the now-invalid bullet (`~~~~`) or replace it inline with a "Revised:" note.
2. Document why so the next reader understands.

## Audit Snapshots

For long-running plans (`plan-database.md`), include a dated audit block at the top summarizing what is *currently* true in code vs what the plan still says is pending. This is essential when picking the work back up after a gap.

## Anti-Patterns

- ❌ Steps so large they cannot be committed independently.
- ❌ Plans that do not name the verification command.
- ❌ Missing commit-message suggestions — forces the reviewer to invent one.
- ❌ Editing-in-place to remove completed steps — history is lost.
- ❌ Plans that describe code structure but skip the migration of call sites and tests.
- ❌ Agent running git commands instead of returning the suggested message.

## Checklist

- [ ] Title and overview state the goal in one sentence
- [ ] Background section references existing code with file/line pointers
- [ ] Each step has Goal / Files / Changes / Verification / Commit message
- [ ] Each step is small enough to commit on its own
- [ ] Before/After snippets used for non-trivial edits
- [ ] Completion markers (`✅ COMPLETED`) added as work progresses
- [ ] No section instructs the agent to run `git add`/`git commit`
