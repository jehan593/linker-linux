# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

Linker for Linux: a native C + GTK3 port of `../linker-windows` (Avalonia/.NET), itself a
port of an Android app at `../linker`. Same Nord color palette, Martian Mono Nerd Font, and
feature set as the Windows app — registers itself as a browser candidate and shows its own
chooser popup instead of opening a browser directly. No build system beyond a plain Makefile;
single binary `linker`, source in `src/`.

Read `../linker-windows/CLAUDE.md` first for the *why* behind ported logic — every module here
has a doc comment naming the Windows/Android file it corresponds to and what's Linux-specific.

Dependency footprint is deliberately minimal: **GTK3 + GLib/GIO + libcurl only** — no json-glib
(JSON is hand-rolled in `json_min.c` since the schema is small and fully self-controlled), no
libsoup. Binary is ~130KB.

## Commands

```sh
make                              # builds ./linker
./linker                          # launch main window
./linker https://example.com      # exercise the chooser popup directly (same as a real
                                   # browser-link interception)
make install                      # installs to ~/.local (binary, fonts/icons, .desktop entry)
make PREFIX=/some/path install    # install elsewhere
make uninstall                    # removes the above; NEVER touches linker-data.json/settings.json
```

No test suite (matches the Windows app's own state). No CI.

## Module layout

- `main.c` — entry point; argv[1] URL-shaped → chooser window, else main window. Every launch
  is a fresh process (no daemon), mirroring the Windows app's model exactly.
- `app_context.c/h` — `AppState` (loaded `LinkerData*`, bundled asset dir, app icon path, main
  window handle), passed explicitly through GTK callback `user_data` rather than kept as hidden
  globals.
- `json_min.c/h` — minimal hand-rolled JSON parser/writer.
- `data_store.c/h` — `BrowserPrefEntity`/`SavedLinkEntity` + flock-guarded atomic-write JSON
  persistence at `$XDG_DATA_HOME/linker/linker-data.json`.
- `theme.c/h` — Nord dark/light palette tables, generates the whole app's GTK3 CSS as one
  stylesheet string, OS dark/light detection (xdg-desktop-portal D-Bus, GSettings fallback) with
  live watching. **See "Known GTK3 gotchas" below before touching this file.**
- `browsers.c/h` — `.desktop` file enumeration (`Categories` contains `WebBrowser`) + merge with
  saved prefs + order synthesis/materialization; 5-minute on-disk cache for the chooser's fast path.
- `launcher.c/h` — strips desktop-entry field codes, shell-splits, spawns.
- `xdg_default.c/h` — installs/updates our own `~/.local/share/applications/linker.desktop`;
  queries/sets default browser via GIO `GAppInfo` (can set directly — no OS settings roundtrip
  needed, unlike Windows).
- `icon_resolve.c/h` — resolves a browser's `Icon=` (themed name or path) to a `GdkPixbuf`, with a
  Cairo-drawn placeholder fallback.
- `toast.c/h` — shared fade-in/out bubble (`GtkRevealer`), used by main window (as a `GtkOverlay`
  child) and chooser window (as a fixed 34px row, matching the Windows app's differing treatment).
- `ui_widgets.c/h` — shared control factories (pill/text/icon buttons, labels, outlined
  entry/textview) applying the CSS classes from `theme.c`.
- `ui_main_window.c/h`, `ui_browsers_view.c/h`, `ui_edit_browser_dialog.c/h`,
  `ui_saved_links_view.c/h`, `ui_edit_saved_link_dialog.c/h`, `ui_chooser_window.c/h` — one file
  per screen/dialog, named after their Windows XAML
  counterparts. Mutation pattern throughout: after any edit, full rebuild-from-data-store rather
  than incremental widget patching (same philosophy the Windows CLAUDE.md describes for its own
  ViewModels). Row-owned signal handlers that might destroy their own row use the exported
  `*_refresh_deferred()` variant (`g_idle_add`) instead of calling `*_refresh()` synchronously, to
  avoid destroying a widget still on the call stack.
- `app_identity.h` — `LINKER_APP_ID`, `LINKER_DESKTOP_ID`, MIME type constants.

## Known GTK3 gotchas

- **GTK auto-adds style classes based on button content**: `GtkButton` gets `.text-button` for a
  text-only label and `.image-button` for image-only content, regardless of app code. Never reuse
  those two names for custom CSS classes — a collision lets GTK's own rule win the cascade over
  the intended one. App classes are prefixed `linker-*` (see `ui_text_button_new()` in
  `ui_widgets.c` and `generate_css()` in `theme.c`) specifically to rule this out.
- **Widget margin sits *outside* its own CSS background** — a widget with both a background-color
  class and `gtk_widget_set_margin_*()` renders the color inset from its container's edges instead
  of full-bleed. For a full-bleed colored bar with inset content, split into an outer unmargined
  box (carries the background class) wrapping an inner box (carries the margin/content) — see
  `top_bar`/`top_bar_inner` in `ui_main_window.c`.
- **`gtk_container_set_border_width()` is a no-op for child layout on this GTK build (3.24.52)** —
  it does not inset children the way it does on other GTK versions, and `gtk_widget_set_margin_*()`
  is not a substitute (margin offsets a widget relative to its own parent, not its children). Use
  the `.content-pad-16`/`.content-pad-20` CSS padding utility classes in `theme.c` instead, applied
  via `gtk_style_context_add_class()` — see `build_banner()` in `ui_main_window.c`, the chooser
window's `root` box in `ui_chooser_window.c`, and the dialog content areas in
   `ui_edit_saved_link_dialog.c`/`ui_edit_browser_dialog.c`.
- App buttons and switches are skipped by Tab focus (only text fields receive focus), **except** the chooser
  window's browser rows — those are real `GtkButton`s so Tab/Down-arrow cycle them and Enter opens one; the chooser
  opens with nothing focused until the user uses the keyboard. Inputs use a Nord9 focused border, and focused
  chooser rows get a smooth Nord9 border (`.chooser-row:focus` in `theme.c`).
- Every custom-colored CSS class gets a `:backdrop` twin so colors stay consistent when the window
  loses input focus (GTK dims by default otherwise), and `background-image: none` alongside any
  `background-color`, defending against a GTK theme's own gradient compositing over the flat color.

## Theme

Full Nord dark/light hex tables in `theme.c` (`DARK_PALETTE`/`LIGHT_PALETTE`), using Nord9 as
the primary accent in both modes and following `~/design.md`. `generate_css()` builds the entire
stylesheet as one string from these tables; **every** rule needs `background-image: none`
alongside any `background-color` (see "Known GTK3 gotchas" above) and should get a `:backdrop`
twin if it sets a color a user might actually see — keep doing this for new classes. Font is
Martian Mono, bundled as `data/fonts/*.ttf`, loaded privately via `FcConfigAppFontAddFile` at
startup (not installed system-wide), so it works on distros without the Nerd Font preinstalled
even if the package is already present locally.

## Icon

`data/icons/linker-{16,32,48,256}.png`, generated by `tools/generate_icon.c` (two interlocking
rings, Nord9 on Nord0) — **an approximation**, not a port of the real Android vector source, which
isn't present anywhere in this environment (`../linker` doesn't exist on this machine). Re-run
`tools/generate_icon.c` if the palette/motif changes; don't hand-edit the PNGs.

## Platform-adaptation notes (things that can't be 1:1 with Windows)

- Browser enumeration/launching: `.desktop` files (`browsers.c`) instead of the
  `StartMenuInternet` registry convention.
- Default-browser registration: self-installed `.desktop` + GIO `GAppInfo`
  (`xdg_default.c`) instead of registry keys — and notably *can* set itself as default directly,
  no OS settings deep-link needed (Windows has no such API for third-party apps).
- Icon glyphs: standard freedesktop symbolic icon names (`list-add-symbolic`, etc.) instead of
  Segoe MDL2 Assets — verified each one actually resolves in this system's icon theme
  (`gtk_icon_theme_has_icon`) before committing to a name; do this for any new icon you add,
  several plausible-sounding names (e.g. `external-link-symbolic`) don't exist everywhere and
  silently fall back to a "no entry" glyph instead of erroring.
- `Topmost`/`ShowInTaskbar=False` on the chooser window map to `gtk_window_set_keep_above` /
  `gtk_window_set_skip_taskbar_hint` — best-effort X11 WM hints, no guaranteed effect on Wayland.

## Testing

No test suite. Verification is manual: build, launch on a live X11 display (`DISPLAY=:0`),
screenshot via `gnome-screenshot`, and **pixel-sample screenshots with PIL rather than eyeballing
them** when checking color correctness — the eye is unreliable for judging whether a rendered
color matches an intended hex value; sample actual RGB values (`im.getpixel(...)`) and compare
against the palette table instead. `xdotool` is available for scripting clicks/typing into dialogs
during manual smoke tests.
