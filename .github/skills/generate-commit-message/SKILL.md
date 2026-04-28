---
name: generate-commit-message
description: 'Generate git commit messages from the current repository diff. Use when asked for a commit message, commit summary, or to review staged or unstaged changes before writing a commit message. Always inspect the git diff first and return the suggested message in markdown.'
argument-hint: 'Optional scope such as staged, unstaged, or a note about the intended change'
user-invocable: true
---

# Generate Commit Message

Use this skill when the task is to draft a git commit message from the repository state.

## Required Behavior

- Always inspect the full current repository diff before suggesting a message.
- Review both staged and unstaged changes unless the user explicitly narrows the scope.
- Start with a repository-wide scope check such as `git status --short`, `git diff --stat`, and `git diff --cached --stat` or equivalent tooling.
- If the diff summary is ambiguous, read the changed files or diff hunks before drafting the message.
- Make sure the proposed message covers all meaningful changes in the diff, not just the most recent file.
- Return the result in markdown.

## Procedure

1. Check the current git changes first.
   Use repository-wide diff tooling before writing anything.

   Minimum check:

   ```text
   git status --short
   git diff --stat
   git diff --cached --stat
   ```

   If that output suggests broader work than a single summary can safely capture, inspect the relevant diff hunks or changed files before drafting.

2. Identify the real change set.
   Group the edits into the smallest honest summary that explains what changed and why.

3. Write an imperative subject line.
   Keep it concise, specific, and scoped to the actual diff.

4. Add a body only when it improves clarity.
   Include the body when the diff has multiple related parts, test updates, refactors, or behavior changes that would be unclear from the subject alone.

5. Present the result in markdown.
   Preferred format:

```text
<subject line>

<body paragraphs if needed>
```

## Output Rules

- Do not invent changes that are not present in the diff.
- Do not summarize only the last file you touched when the repository diff contains broader changes.
- Do not omit tests, refactors, or infrastructure changes when they are a material part of the change set.
- Prefer one primary commit message suggestion.
- If useful, include one short alternate subject line after the primary suggestion.
- If there is no diff, say that no commit message can be drafted yet because there are no changes to summarize.

## Good Subjects

- refactor: add readonly entity list iterators
- test: cover readonly ship and entity list iteration
- fix: preserve const iteration for inline readonly lists

## Notes

- Match the repository's existing commit style when it is obvious from context.
- If the user asks for a conventional commit, follow that style after reviewing the diff.
