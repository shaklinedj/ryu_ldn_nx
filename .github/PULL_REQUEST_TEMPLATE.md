Thank you for your contribution! Please fill out this template completely.
All commits should be DCO-signed (`git commit -s`).

## Summary

<!-- Brief description of what this PR changes and why. -->
<!-- This PR [adds/fixes/changes/removes] [what] in order to [why]. -->

## Type of Change

- [ ] Bug fix (non-breaking change that fixes an issue)
- [ ] New feature (non-breaking change that adds functionality)
- [ ] Breaking change (fix or feature that would cause existing behavior to change)
- [ ] Documentation update
- [ ] Code refactoring
- [ ] Build/CI changes

## Related Issues

<!-- Reference related issue numbers (e.g., Fixes #123, Related to #456). -->

## Changes Made

<!-- List the key changes in this PR. -->
<!-- -  -->

## How to Test

<!-- Step-by-step instructions for verifying this change. -->
<!-- 1. -->
<!-- 2. -->
<!-- 3. -->

## Tested On

- [ ] Nintendo Switch with Atmosphere
- [ ] Connection to LDN server (relay mode)
- [ ] Connection to LDN server (P2P mode)
- [ ] Game tested (specify title and version)
- [ ] Build succeeds (`docker compose run --rm build`)
- [ ] Unit tests pass (`docker compose run --rm test`)

## Code Quality

- [ ] Code follows project style (4-space indent, same-line braces, Doxygen `/** */` on public symbols)
- [ ] No dynamic allocations in hot paths (fixed buffers, `constinit` statics, or stack-allocated work areas)
- [ ] Protocol structures maintain binary compatibility with C# server (12-byte `LdnHeader`, no `Pack=1`)

## Testing & Documentation

- [ ] Changes are tested thoroughly
- [ ] No new compiler warnings introduced
- [ ] Documentation updated if needed (comments, README, docs/)

## DCO

- [ ] All commits are signed off using `git commit -s`

---

By submitting this pull request, I confirm that my contribution is made under the terms of the GPLv3 license and that I have the right to submit it under the open source license indicated in the file.

**Note**: Use `git commit -s` to add the `Signed-off-by:` line automatically from your git config. Never hardcode a developer name or email.
