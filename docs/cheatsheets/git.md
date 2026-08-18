# git — everyday recipes

*tags: git, cli*

A starter sheet. Add your own hard-won recipes as you hit them.

## Undo

| Goal | Command |
|---|---|
| Unstage a file (keep changes) | `git restore --staged <file>` |
| Discard working-tree changes | `git restore <file>` |
| Amend the last commit message | `git commit --amend` |
| Undo last commit, keep changes staged | `git reset --soft HEAD~1` |

## Inspect

| Goal | Command |
|---|---|
| Compact history graph | `git log --oneline --graph --all` |
| What changed in a commit | `git show <sha>` |
| Who last touched each line | `git blame <file>` |

!!! note "This is a sample cheatsheet"
    Replace or extend it with the recipes *you* keep re-googling.
