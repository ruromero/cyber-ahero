# Issue tracker: Obsidian notes in planning/

Specs and tickets are Markdown notes under `planning/`, readable as an Obsidian vault
(YAML frontmatter, [[wikilinks]], #tags). Plain Markdown otherwise; no plugins needed.

## Layout

- One folder per feature: `planning/<feature-slug>/`
- Spec: `planning/<feature-slug>/spec.md`
- Tickets: one note each, `planning/<feature-slug>/issues/NN-<slug>.md`, numbered from 01, never a single combined tickets file

## Frontmatter

Spec:

```yaml
---
type: spec
status: open        # open | done
tags: [library, kiosk]
---
```

Ticket:

```yaml
---
type: ticket
status: open        # open | in-progress | done
tags: [library]
spec: "[[<feature-slug>/spec]]"
blocked-by: ["[[<feature-slug>/issues/01-<slug>]]"]   # omit if none
---
```

Tags come from `docs/agents/triage-labels.md`. Link with path-style wikilinks
(`[[<feature-slug>/spec]]`) so names stay unambiguous. Discussion goes at the
bottom of the note under a `## Comments` heading.

## When a skill says "publish to the issue tracker"

Create the note under `planning/<feature-slug>/` (creating the folder if needed) with the frontmatter above.

## When a skill says "fetch the relevant ticket"

Read the note at the path or number the user gives.

## When a skill changes a ticket's state

Update its `status:` line: `in-progress` when work starts, `done` when it's finished.
