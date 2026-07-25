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
- `notesnook_store.c/h`, `notesnook_api.c/h` — separate `settings.json`; libcurl POST to
  `https://inbox.notesnook.com/` (`source: "linker-linux"`).
- `theme.c/h` — Nord dark/light palette tables, generates the whole app's GTK3 CSS as one
  stylesheet string, OS dark/light detection (xdg-desktop-portal D-Bus, GSettings fallback) with
  live watching. **See "Known issue" below before touching this file.**
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
  `ui_saved_links_view.c/h`, `ui_edit_saved_link_dialog.c/h`, `ui_settings_dialog.c/h`,
  `ui_chooser_window.c/h` — one file per screen/dialog, named after their Windows XAML
  counterparts. Mutation pattern throughout: after any edit, full rebuild-from-data-store rather
  than incremental widget patching (same philosophy the Windows CLAUDE.md describes for its own
  ViewModels). Row-owned signal handlers that might destroy their own row use the exported
  `*_refresh_deferred()` variant (`g_idle_add`) instead of calling `*_refresh()` synchronously, to
  avoid destroying a widget still on the call stack.
- `app_identity.h` — `LINKER_APP_ID`, `LINKER_DESKTOP_ID`, MIME type constants.

## Fixed issue — pill-button rendered the wrong color (GTK auto-class name collision)

Symptom was the banner's "Set as default browser" `.pill-button` rendering with background
`#434C5E` (Nord2 / `surface_container_highest`) instead of the intended `#88C0D0` (`primary`).

Root cause: **GTK3's `GtkButton` automatically adds the built-in style class `.text-button` to any
button whose content is just a text label** (and `.image-button` for image-only content) — this is
documented GTK3 behavior, not app code, and applies to every `gtk_button_new_with_label()` button
in the app, pill button included. The app's own `ui_text_button_new()` happened to *also* use
`"text-button"` as a custom class name for its own distinct styling. That name collision meant the
pill button silently matched both `.pill-button` and the app's own `.text-button` rule; with equal
CSS specificity, the later-declared rule in `generate_css()` (`.text-button`, styled dark gray)
won the cascade over `.pill-button` (styled Nord8 blue).

Confirmed via `gtk_style_context_to_string()` on the live widget, which showed
`[button.flat.text-button.pill-button:dir(ltr)]` even though no code path ever called
`add_class(..., "text-button")` on that widget — the class was GTK's own auto-added one, not app
code, which is what made it invisible to a source-level search.

Fix: renamed the app's custom class from `"text-button"` to `"linker-text-button"` in
`ui_widgets.c` (`ui_text_button_new()`) and `theme.c` (`generate_css()`), so it can never collide
with GTK's automatic per-content-type class names again. The `text-button-primary/-neutral/-error`
modifier classes were left as-is — they don't collide with anything GTK adds automatically.
Verified fixed by pixel-sampling a fresh screenshot: the button samples as `(136, 192, 208)` =
`#88C0D0` exactly.

**Takeaway for any future custom CSS class name**: avoid `text-button` and `image-button` (GTK's
auto-added content-type classes on `GtkButton`) as literal class names — prefix custom classes
(e.g. `linker-*`) to rule this out categorically rather than re-litigating it per class.

The debug scaffolding mentioned in earlier notes (`/tmp/linker-debug.css` dump in `apply_css()`,
`gtk_style_context_to_string()` dump in `build_banner()`) has been removed now that the root cause
is confirmed. `GError` checking in `apply_css()` was kept permanently, as intended.

## Other things fixed this session (margin/background-vs-margin bugs)

- **GTK3 widget margin sits *outside* a widget's own CSS background** — any widget that both
  had a background-color CSS class *and* `gtk_widget_set_margin_*()` calls on itself would show
  the colored band shrunk away from its container's edges instead of a full-bleed background with
  inset content. Found via pixel-diffing a screenshot of the top bar (`.top-bar` class + margins
  on the same box). Fixed by splitting into an outer unmargined box (carries the background
  class, spans full width) wrapping an inner box (carries the margin, holds the actual content) —
  see `top_bar`/`top_bar_inner` in `ui_main_window.c`. **Audit any future full-bleed colored bar
  the same way**: if CSS `padding` (real inside-the-background inset, see next bullet) isn't
  suitable because the design wants asymmetric horizontal/vertical insets, use the
  outer-background/inner-margin split, never margin directly on a background-classed widget.
- **`gtk_container_set_border_width()` is a no-op for child layout on this GTK build (3.24.52)** —
  discovered while investigating why the banner's title/body text rendered almost flush against
  the card edges (~3px gap) instead of the intended 16px. Confirmed two ways: (1) allocation dump
  showed the title label's `GtkAllocation` exactly equal to its parent `GtkBox`'s own allocation —
  border-width should have offset it inward and didn't; (2) a minimal standalone reproducer
  (`GtkDialog` + `gtk_container_set_border_width(content_area, 20)`) showed the same — child
  allocation identical to the container's own. This affects **any** `GtkBox`, including
  `gtk_dialog_get_content_area()`'s box, not just custom ones. `gtk_widget_set_margin_*()` does
  **not** fix it either — margin only offsets a widget relative to *its own parent*, it does
  nothing to that widget's *own children*'s inset (verified with the same reproducer). The actual
  fix is real CSS `padding` on the container — added generic `.content-pad-16`/`.content-pad-20`
  utility classes in `theme.c` for this, applied via `gtk_style_context_add_class()` wherever
  `gtk_container_set_border_width()` was previously used (`build_banner()`'s `.banner` class in
  `ui_main_window.c`, the chooser window's `root` box in `ui_chooser_window.c`, and the dialog
  content areas in `ui_settings_dialog.c`/`ui_edit_saved_link_dialog.c`/`ui_edit_browser_dialog.c`).
  **Do not use `gtk_container_set_border_width()` for new UI on this codebase — use a CSS padding
  class instead**, and don't assume `gtk_widget_set_margin_*()` is a substitute since it solves a
  different problem (position-within-parent, not children-inset).
- Suppressed GTK's default dashed keyboard-focus ring on buttons/switches/entries/textviews
  (`button:focus, switch:focus, entry:focus, textview:focus { outline-style: none; }` in
  `theme.c`) — was rendering as a stray dotted box around the "Browsers" tab.
- Added `:backdrop` CSS overrides for all custom-colored classes so colors stay consistent when
  the window loses input focus (GTK dims by default otherwise) — real-world impact of this one is
  unconfirmed since the pill-button bug above was masking/confounding the visual check.
- Added `background-image: none` alongside `background-color` on every custom button/box class,
  defending against a theme's own `background-image` gradient compositing over our flat color —
  didn't turn out to be the pill-button root cause but is correct defensive CSS regardless, keep it.
- Bumped several `margin_end`/right-edge insets (saved-link row action icons, browser-list row,
  ManageBrowsersView header) from 16px to 20px, and outlined-entry/textview internal text padding
  from 8px to 10-12px, per user-reported "content touching the border" screenshots.

## Theme

Full Nord dark/light hex tables in `theme.c` (`DARK_PALETTE`/`LIGHT_PALETTE`), hex-for-hex ported
from `../linker-windows/Linker/Theme/Colors.axaml`. `generate_css()` builds the entire
stylesheet as one string from these tables; **every** rule needs `background-image: none`
alongside any `background-color` (see above) and should get a `:backdrop` twin if it sets a color
a user might actually see (learned the hard way — keep doing this for new classes). Font is
Martian Mono, bundled as `data/fonts/*.ttf`, loaded privately via `FcConfigAppFontAddFile` at
startup (not installed system-wide) so it works on distros without the Nerd Font preinstalled —
this dev machine happens to already have it via package manager, don't rely on that when testing
elsewhere.

## Icon

`data/icons/linker-{16,32,48,256}.png`, generated by `tools/generate_icon.c` (two interlocking
rings, Nord8 on Nord0) — **an approximation**, not a port of the real Android vector source, which
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

No test suite. Verification has been manual: build, launch on the live X11 display
(`DISPLAY=:0` in this dev environment), screenshot via `gnome-screenshot`, and — critically, learned
this session — **pixel-sample screenshots with PIL rather than eyeballing them** when checking
color correctness; the eye is bad at judging whether a rendered color matches an intended hex
value, and this session's whole styling bug was only conclusively pinned down by sampling actual
RGB values (`im.getpixel(...)`) and comparing against the palette table. `xdotool` is available
for scripting clicks/typing into dialogs during manual smoke tests.
