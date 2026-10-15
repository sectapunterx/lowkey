# Distribution channels

Every build of Tabless ships from [GitHub Releases](https://github.com/sectapunterx/tabless/releases).
Package-manager manifests reuse those same release assets. This doc tracks each
free channel, the manifest that feeds it, and the exact steps to submit/update.

| Channel | Manifest | Status | Submission |
|---------|----------|--------|------------|
| **Scoop** (Windows) | [`bucket/tabless.json`](../bucket/tabless.json) | ✅ live in this repo | none — this repo *is* the bucket |
| **winget** (Windows) | [`packaging/winget/`](../packaging/winget/) | ⏳ submitted: [microsoft/winget-pkgs#453436](https://github.com/microsoft/winget-pkgs/pull/453436) | PR to `microsoft/winget-pkgs` |
| **Flathub** (Linux) | [`packaging/flatpak/`](../packaging/flatpak/) | 📝 draft, test then PR | PR to `flathub/flathub` |
| **Homebrew tap** (macOS) | [`packaging/homebrew/`](../packaging/homebrew/) | ✅ live: [`sectapunterx/homebrew-tabless`](https://github.com/sectapunterx/homebrew-tabless) | push the bumped cask to the tap on each release |

Other channels (AUR, Snap, Chocolatey, Homebrew) are not set up yet — they need
external accounts. See "Not yet done" below.

---

## Scoop — works now

This repository is a Scoop bucket (`bucket/tabless.json`; `bucket/lowkey.json`
and `bucket/heap.json` are the same manifest under the old names, so an app
installed as `lowkey` or `heap` before a rename keeps updating). Users install with:

```powershell
scoop bucket add tabless https://github.com/sectapunterx/tabless
scoop install tabless
```

The manifest installs Tabless 0.9.4 (`tabless-v0.9.4-windows-portable.zip`) with
`tabless` and `tabless-cli` on PATH. Its `post_install` deletes `tabless-portable.txt`
from the unpacked folder: Scoop owns the updates of that folder, and with the marker
the in-app updater would overwrite it. The app then points to the release page
instead. `autoupdate` rewrites the URL and hash but never `bin`, so a rename of the
executable is a hand edit of all three files.

`checkver: github` + `autoupdate` mean the manifest tracks the latest GitHub
release automatically; on a new release update `version`/`url`/`hash` in all three files (or run
`scoop update`/`checkver -u` from a Scoop clone).

To also get into the shared **Extras** bucket (more discoverable), open a PR to
[`ScoopInstaller/Extras`](https://github.com/ScoopInstaller/Extras) with a copy
of `bucket/tabless.json`.

## winget — submitted

Manifests live in `packaging/winget/` (schema 1.12.0, what winget-pkgs asks for):
`sectapunterx.Tabless.yaml`, `.installer.yaml`, `.locale.en-US.yaml` (package
identifier `sectapunterx.Tabless`, moniker `tabless`).

1. Validate + test locally (Windows):
   ```powershell
   winget validate --manifest packaging\winget
   winget install --manifest packaging\winget   # optional smoke install
   ```
2. Submit — easiest with wingetcreate:
   ```powershell
   winget install Microsoft.WingetCreate
   wingetcreate submit --token <gh-token> packaging\winget
   ```
   or manually fork [`microsoft/winget-pkgs`](https://github.com/microsoft/winget-pkgs),
   copy the three files to
   `manifests/s/sectapunterx/Tabless/<version>/`, and open a PR.
3. On each release, bump `PackageVersion` + `InstallerUrl` + `InstallerSha256`
   (`sha256sum tabless-<ver>-windows-setup.exe`) and repeat. `wingetcreate update`
   automates this.

> The installer is currently **unsigned**; winget's automated validation still
> accepts it, but signing (see [PACKAGING.md](PACKAGING.md)) is recommended.

## Flathub — draft, test before PR

Files in `packaging/flatpak/`: manifest `io.github.sectapunterx.tabless.yaml`,
`io.github.sectapunterx.tabless.metainfo.xml`, `io.github.sectapunterx.tabless.desktop`
(app id `io.github.sectapunterx.tabless`).
The manifest builds from the `v0.4.2` git tag against the KDE 6 runtime.

1. Build + run locally:
   ```bash
   flatpak install flathub org.kde.Platform//6.8 org.kde.Sdk//6.8
   flatpak-builder --user --install --force-clean build-dir \
     packaging/flatpak/io.github.sectapunterx.tabless.yaml
   flatpak run io.github.sectapunterx.tabless
   ```
2. Validate metadata:
   ```bash
   flatpak run org.freedesktop.appstream.cli validate \
     packaging/flatpak/io.github.sectapunterx.tabless.metainfo.xml
   ```
   (If `post-install` install paths fail, adjust them relative to the build cwd.)
3. Submit: fork [`flathub/flathub`](https://github.com/flathub/flathub), create a
   branch off `new-pr`, add the manifest, open a PR. A reviewer runs the build and
   may request tweaks. On approval you get a dedicated `flathub/io.github.sectapunterx.tabless`
   repo; bump the `tag`/`commit` there for each release.

---

## Not yet done (need external accounts)

| Channel | What's needed |
|---------|---------------|
| **Homebrew Cask** (official) | PR to `homebrew/homebrew-cask` — needs notarization and has a notability bar; the own tap above covers it until then |
| **AUR** (Arch) | AUR account + SSH key; push a `PKGBUILD` |
| **Snap Store** | Snapcraft account; `snapcraft.yaml` + `snapcraft upload` |
| **Chocolatey** | community account + API key; `.nuspec` + `choco push` |
| **Microsoft Store** | $19 one-time dev account + MSIX packaging (removes SmartScreen entirely) |
