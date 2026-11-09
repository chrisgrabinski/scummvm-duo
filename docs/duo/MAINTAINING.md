# Maintaining the fork

## Branches

- `master` mirrors [scummvm/scummvm](https://github.com/scummvm/scummvm) `master`. Do not commit
  to it; this keeps syncing free of conflicts.
- `duo` holds the dual-screen changes as a small set of commits on top of `master`. Work happens
  here, or in feature branches started from it and merged back into it.

## Syncing with ScummVM

1. Update `master`, either with the "Sync fork" button on GitHub, or:

   ```bash
   git fetch upstream
   git push origin upstream/master:master
   ```

2. Rebase `duo` onto it and push:

   ```bash
   git switch duo
   git rebase upstream/master
   git push --force-with-lease origin duo
   ```

   A merge of `upstream/master` into `duo` also works if you prefer not to rewrite history.

3. Build and try the app again. Conflicts are most likely in `backends/graphics/windowed.h`,
   `backends/graphics/opengl/opengl-graphics.cpp`, `engines/scumm/verbs.cpp` and
   `common/system.h`.

Sync every month or two, or when you want a specific upstream fix, so that each rebase stays
small.

## Releases

Publish built APKs as GitHub Releases on tags (for example `duo-2026.11-1`) and note which
upstream commit they are based on. Do not commit APKs to the repository.

## Contributing to ScummVM

Anything meant for upstream should be a separate branch from `master`, and must follow
ScummVM's [AI-GUIDELINES.md](../../AI-GUIDELINES.md) and
[CONTRIBUTING.md](../../CONTRIBUTING.md). The dual-screen changes were written with AI
assistance and are not meant for upstream as they stand.
