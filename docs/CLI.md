# Tabless from the terminal

<img src="assets/img/readme/cli.gif" width="100%" alt="tabless today lists what is overdue and due, tabless add captures a task, and after git checkout of a task branch tabless now and the shell prompt show that task">


Note a task or check the current one without leaving the terminal. With Tabless open on the same data
directory, commands go to the window — a change shows up there at once and can be undone there; with it
closed, Tabless reads and saves `state.json` itself. Nothing here talks to the network.

```sh
tabless add "fix login tomorrow 14:00 p1 #backend // check the refresh token"   # read like quick capture
tabless add "call vendor +Globex"   # +Profile adds it there (an explicit --profile wins)
tabless now                    # the task with a running timer, else the one your git branch names
tabless list --status prog     # also --profile <name>, --json
tabless today                  # overdue first, then in progress, scheduled or due today
tabless done APP-12
tabless open APP-12            # show it in the window (starts Tabless if it is closed)
tabless sched . tomorrow 14:00 # plan it for a day ("." = the task your git branch names); "none" clears
tabless due APP-12 fri         # its due date; on a tracker card it is your own date, the tracker keeps its own
tabless est . 1h30m            # the estimate; "none" clears
tabless someday APP-12         # park it (and "someday APP-12 off" to take it back)
tabless help
```

Each change answers with one line of fact in the app's language (`--json` gives the task instead).

`tabless now` prints nothing and exits 0 when there is no current task, so it fits a shell prompt.
`--format` takes `{id} {key} {title} {status} {priority} {profile} {source} {elapsed}`:

```toml
# starship.toml
[custom.tabless]
command = "tabless now --format '{id} {title}'"
when = true
format = "[$output]($style) "
```

```sh
# bash / zsh
PS1='$(tabless now --format "[{id}] ")'"$PS1"
```

More prompts, all reading the same line. Each one shows nothing when there is no current task.

```fish
# fish — ~/.config/fish/functions/fish_right_prompt.fish
function fish_right_prompt
    tabless now --format '{id} {title}'
end
```

```zsh
# powerlevel10k — in ~/.p10k.zsh, then add `tabless` to POWERLEVEL9K_RIGHT_PROMPT_ELEMENTS
function prompt_tabless() {
  local t=$(tabless now --format '{id} {title}')
  [[ -n $t ]] && p10k segment -f 208 -t "$t"
}
```

```json
// oh-my-posh — a segment in your theme's blocks
{
  "type": "command",
  "style": "plain",
  "foreground": "#f2a65a",
  "properties": {
    "shell": "bash",
    "command": "tabless now --format '{id} {title}'"
  }
}
```

```tmux
# tmux — the status bar, refreshed every 15 s
set -g status-interval 15
set -g status-right '#(tabless now --format "{id} {title}") %H:%M'
```

```powershell
# PowerShell — in $PROFILE; tabless-cli answers `now` itself, fast enough for every prompt
function prompt {
  $t = tabless-cli now --format '[{id}] '
  "$t" + "PS $($executionContext.SessionState.Path.CurrentLocation)> "
}
```

`{title}` can be long in a prompt; `{id}` alone is the short form.

Exit codes: `0` ok, `1` usage, `2` no such task, profile or column, `3` data error. `--data-dir` and
`HEAP_DATA_DIR` work as for the app.

**Windows:** in cmd and PowerShell use `tabless-cli` (it sits next to `tabless.exe`): the app itself is a
windowed program, so those shells neither wait for it nor see its output. `tabless-cli` answers `now`,
`list` and `today` itself, fast enough for a prompt, and passes the rest to `tabless.exe`. Add the install
folder to `PATH`, and `Set-Alias tabless tabless-cli` in your PowerShell profile if you like the short name.
In git-bash plain `tabless` works too.

