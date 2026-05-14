#!/usr/bin/env python3
"""Install git hooks for ryu_ldn_nx.

Copies pre-commit hook from scripts/hooks/ to .git/hooks/
and makes it executable.
"""

import os
import shutil
import stat
import sys


def main():
    repo_root = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    git_hooks = os.path.join(repo_root, ".git", "hooks")
    source_hook = os.path.join(repo_root, "scripts", "hooks", "pre-commit")

    if not os.path.exists(source_hook):
        print(f"ERROR: Hook source not found: {source_hook}", file=sys.stderr)
        sys.exit(1)

    if not os.path.isdir(git_hooks):
        os.makedirs(git_hooks)

    dest = os.path.join(git_hooks, "pre-commit")

    if os.path.exists(dest):
        # Remove old hook to avoid conflicts
        os.remove(dest)

    shutil.copy2(source_hook, dest)
    os.chmod(dest, os.stat(dest).st_mode | stat.S_IXUSR | stat.S_IXGRP | stat.S_IXOTH)

    print(f"Installed pre-commit hook: {dest}")
    print("Hook will run unit tests and enforce >= 80% coverage before each commit.")
    print("Set MIN_COVERAGE env var to override the threshold (e.g., MIN_COVERAGE=90).")


if __name__ == "__main__":
    main()