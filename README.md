# Linker

[![Release](https://img.shields.io/github/v/release/jehan593/linker-linux)](https://github.com/jehan593/linker-linux/releases/latest)
[![License: MIT](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)

A browser chooser for Linux (C + GTK3). When you click a link, Linker pops up and lets
you pick which browser opens it — work vs. personal, Brave vs. Firefox, and so on.
Same look and feature set as the [Windows](https://github.com/jehan593/linker-windows)
and Android versions.

No background process — every launch is a fresh, short-lived app. Installs per-user,
no root needed. Saved links and settings live under `$XDG_DATA_HOME/linker`.

> **FYI: This project is fully vibe coded.**

## Install

```sh
curl -fsSL https://raw.githubusercontent.com/jehan593/linker-linux/main/scripts/get.sh | bash
```

Then open Linker, go to the Browsers tab, and click **Set as default browser**. Re-run
the same command to update.

Only prebuilt for `x86_64` right now. For other architectures, build from source:

```sh
sudo apt install build-essential pkg-config libgtk-3-dev libglib2.0-dev libcurl4-openssl-dev libfontconfig-dev
git clone https://github.com/jehan593/linker-linux
cd linker-linux
make
make install
```

## Uninstall

```sh
curl -fsSL https://raw.githubusercontent.com/jehan593/linker-linux/main/scripts/get.sh | bash -s -- --uninstall
```

This never touches your saved links. Remove `$XDG_DATA_HOME/linker` yourself for a
clean slate.

## License

[MIT](LICENSE)