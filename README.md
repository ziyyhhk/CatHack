# CatHack Menu

Mega Hack style mod menu for Geometry Dash 2.2081, built with Geode + ImGui.
Press **Tab** to open or close it. Windows are draggable and remember their
position and your settings.

> **Build status: passing in GitHub Actions.** Individual gameplay hooks still need
> in-game testing on Geometry Dash 2.2081.

## Working hacks

| Category | Hacks | Notes |
| --- | --- | --- |
| Bypass | Text Length, Unlock Icons | local only |
| Speedhack | Enabled + speed slider | marks run as cheated |
| Cosmetic | No Death Effect | |
| Level | Noclip | respects anticheat spike |
| Cheat Safety | Safe Mode, Auto Safe Mode | sets `m_isTestMode` on completion so the run isn't saved |
| Display | Unlock FPS, Physics TPS | Independent numeric targets; Physics TPS uses GD 2.2 fixed-step timing and is unavailable on macOS |
| CatHack | Interface Scale | ImGui font scale |

The Display panel accepts any positive finite numeric FPS and Physics TPS
value; invalid, zero, or negative entries are ignored. Physics TPS changes the
simulation step separately from rendering. When enabled, the effective tick
target is the higher of render FPS and Physics TPS. Any Physics TPS other than
240 is treated as cheating by Auto Safe Mode. Non-default or extreme rates can
change collision behavior, and this does not reproduce Geometry Dash 2.1
physics. To avoid runaway update loops, the bypass limits catch-up work to 4096
physics steps per rendered update; exceptionally high targets may therefore
fall behind rather than being fully reached.

Every other row is a dimmed placeholder with a "Not implemented yet" tooltip.
The menu is organized into MegaHack-style panels, including Bypass, Level,
Status, and Replay. The Level and Replay panels are currently UI scaffolding;
only the explicitly implemented toggles are active. In the Bypass window the
placeholders are Anti-Kick, Challenge Level, Keymaster, Main Levels, Music
Customiser, Slider Limit, Treasure Room, Unlock Shops and Unlock Vaults.

## UI

- Hand-drawn pink title bar with a +/- button that morphs between the two signs.
  Click it or double click the bar to collapse. Collapsed windows show how many
  hacks are enabled in them.
- Collapse is animated, and the whole menu fades in and out with Tab.
- Rows light up on hover, fade to pink when enabled, and grow a pink bar in the
  track on the right edge.
- Hover a row for about half a second to get a tooltip (add one with the
  `desc` field of an entry).
- Windows have a soft shadow, are draggable, and remember manually moved
  positions; automatically arranged panels close gaps when neighbors collapse.
- Panels automatically wrap into additional rows to fit the game viewport;
  oversized panels get an inner scrollbar, and saved positions reset when the
  viewport or interface scale changes.
- A compact default scale, tighter row spacing, and the bundled DejaVu Sans font
  replace ImGui's pixel-style default; long labels truncate cleanly and show in
  their tooltip.
- Animation lengths and colors are constants at the top of `src/menu.cpp`.

## Adding a hack

1. Add a `bool` to `State` in `src/Hacks.hpp`.
2. In `src/layout.cpp`, give the row a pointer, a save id and a tooltip:
   `{"No Glow", &s.noGlow, "no-glow", "Hides the glow on icons."}`
3. Read `cat::state().noGlow` from a hook. Bypass hacks go in `src/bypass.cpp`,
   everything else in `src/hooks.cpp` (or make a new file per category).

## If the build fails

- The Geode-compatible `gd-imgui-cocos` revision is pinned in `CMakeLists.txt`;
  update it only after checking its API and building all intended targets.
- Hook signatures (`PlayLayer::init`, `onTextFieldInsertText`, ...) and members
  (`m_anticheatSpike`, `m_isTestMode`, `m_maxLabelLength`) come from the Geode
  bindings and can change between game versions. Check them in the bindings repo.
- `GameManager::isIconUnlocked`, `isColorUnlocked` and the `CCTextInputNode`
  members were checked against the Geode docs. The Text Length approach itself
  is untested: if the limit is enforced somewhere other than
  `onTextFieldInsertText`, it won't do anything.
- Tab is handled through Geode's global keyboard event, so it works even while
  ImGui is closed. Mobile still needs an on-screen toggle button.

## Fonts

The menu bundles DejaVu Sans under its permissive font license to replace the
pixel-style ImGui default. The font is loaded from the mod's packaged resources.
