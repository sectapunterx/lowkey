# Homebrew tap

`Casks/tabless.rb` is the cask for a tap of our own, `sectapunterx/homebrew-tabless`.
The official `homebrew/cask` needs a notarized app, so it waits for that (see DISTRIBUTION.md).

## The tap

Live at [`sectapunterx/homebrew-tabless`](https://github.com/sectapunterx/homebrew-tabless) since 2026-10-15.

## Set the tap up (done once)

1. Create the public repository `sectapunterx/homebrew-tabless`. The `homebrew-` prefix makes it a tap.
2. Copy `Casks/tabless.rb` into it, at the same path, and push.
3. Users install with:
   ```sh
   brew install --cask sectapunterx/tabless/tabless
   ```

## On each release

Bump `version` and `sha256` (the `tabless-v<version>-macos.dmg` line of the release's SHA256SUMS),
in this file and in the tap. `brew audit --cask --online sectapunterx/tabless/tabless` checks the cask
before a push.

The release is arm64 only (built on macos-14) and needs macOS 12 or later; the cask says so with
`depends_on`.
