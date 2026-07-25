# Linker

A native C + GTK3 browser chooser for Linux: registers itself as a browser candidate and shows
its own chooser popup instead of opening a browser directly, so you pick which browser (or which
profile — work vs. personal, Brave vs. Firefox, etc.) handles each link. Same Nord color palette,
Martian Mono Nerd Font, and feature set as the [Windows](https://github.com/jehan593/linker-windows)
and Android versions of this app.

No daemon, no background process — every launch is a fresh, short-lived process. Per-user install,
no root required. Saved links and settings live under `$XDG_DATA_HOME/linker`.

## Install / update

```sh
curl -fsSL https://raw.githubusercontent.com/jehan593/linker-linux/main/scripts/get.sh | bash
```

Downloads the latest release, installs to `~/.local`, and registers the app + `.desktop` entry.
Re-run the same command any time to update to the latest release.

Then open Linker, go to the browsers tab, and click **Set as default browser**.

Only prebuilt for `x86_64` right now. For other architectures, or if you'd rather not run a
prebuilt binary, build from source instead (see below).

## Uninstall

```sh
curl -fsSL https://raw.githubusercontent.com/jehan593/linker-linux/main/scripts/get.sh | bash -s -- --uninstall
```

Removes the binary, desktop entry, and bundled fonts/icons. Never touches your saved links or
Notesnook settings under `$XDG_DATA_HOME/linker` — remove that directory yourself if you want a
clean slate.

## Build from source

Requires GTK3, GLib/GIO, and libcurl development headers (e.g. on Debian/Ubuntu:
`sudo apt install build-essential pkg-config libgtk-3-dev libglib2.0-dev libcurl4-openssl-dev libfontconfig-dev`).

```sh
git clone https://github.com/jehan593/linker-linux
cd linker-linux
make
make install          # installs to ~/.local
make PREFIX=/some/path install
make uninstall
```

## Development

See [CLAUDE.md](CLAUDE.md) for module layout and implementation notes.
