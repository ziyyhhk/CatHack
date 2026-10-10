# CatHack Menu

Mega Hack style mod menu for Geometry Dash 2.2081, targeting Geode SDK 5.11.0
with ImGui. Press **Tab** to open or close it. Windows are draggable and
remember their position and your settings.

> **Verification:** GitHub Actions builds Windows, macOS, iOS, Android32, and
> Android64 targets. A successful build only verifies compilation; gameplay,
> presentation, and native-setting behavior still require in-game testing on
> Geometry Dash 2.2081.

## Working hacks

| Category | Hacks | Notes |
| --- | --- | --- |
| Bypass | Text Length, Unlock Icons | local only |
| Speedhack | Enabled + speed slider | marks run as cheated |
| Cosmetic | No Death Effect | |
| Level | Noclip | respects anticheat spike |
| Cheat Safety | Safe Mode, Auto Safe Mode | sets `m_isTestMode` on completion so the run isn't saved |
| Display | FPS Bypass, TPS Bypass, HZ Bypass, Frame Extrapolation, Vertical Sync | separate controls for render interval, gameplay update cadence, native ForceTimer, render-only prediction, and VSync |
| CatHack | Interface Scale | ImGui font scale |

## Display hacks

FPS, TPS, HZ, Frame Extrapolation, and VSync have independent toggle/setting
state. The FPS and TPS targets are separate positive finite numbers; changing
one does not rewrite the other.

- **FPS Bypass** applies the FPS target through Cocos' `setAnimationInterval`.
  It accepts any positive finite value; intervals outside 1 microsecond to
  60 seconds are retained but not applied. The displayed frame-callback rate
  counts CatHack's ImGui draw callbacks—it is not a measurement of presented
  frames or physical monitor refresh.
- **TPS Bypass** calls the active `GJBaseGameLayer::update(float)` in fixed-size
  substeps while PlayLayer is active, so it changes actual gameplay update-call
  cadence rather than merely changing a displayed target or render interval.
  Targets from 4 through 1,000,000 TPS are accepted, including 3000 TPS, with
  no 240-TPS cap. A 512-substep-per-outer-update budget bounds catch-up; under
  overload the substeps widen to preserve elapsed time and the UI reports the
  measured update-call rate and budget/fallback status instead of claiming the
  requested rate was achieved. Any applied target other than 240 TPS marks the
  run as cheated and may affect physics, collisions, inputs, triggers, and
  `postUpdate` side effects. This is a full game-layer update path, not an
  isolated physics-only step API. It has not yet been runtime-validated.
- **HZ Bypass** calls Geometry Dash 2.2081's native
  `PlatformToolbox::toggleForceTimer` wrapper on Windows, macOS, and iOS. It is
  distinct from the FPS interval and VSync controls, but its exact platform
  effect is not confirmed; it does **not** set a monitor's physical refresh
  rate. The Android bindings do not expose this dynamic setter, so the row is
  disabled there.
- **Frame Extrapolation** records previous/current `PlayerObject` simulation
  positions, simulation-step deltas, and monotonic timestamps. During the active
  PlayLayer's draw traversal it temporarily applies a bounded predicted offset
  to the base Cocos node transform, then restores the original transform before
  the visit returns. It does not alter authoritative player position or
  collision state. History is reset on pause/resume, restart, death, teleport
  or discontinuity, and scene exit. The required `GJBaseGameLayer::visit` hook
  is bound for Windows, macOS, and iOS, but not Android; the Android row is
  disabled instead of presenting an inert option.
- **Vertical Sync** requests Geometry Dash's native
  `PlatformToolbox::toggleVerticalSync` setting. It is separate from FPS/TPS
  and may clamp rendering to the display. The Android bindings do not expose
  the supported dynamic control. When no CatHack override has been saved, the
  setting follows Geometry Dash's own value; saved overrides are applied after
  video settings load. macOS/iOS reloads are hookable. Windows' 2.2081
  `GameManager::loadVideoSettings` binding is inline, so CatHack reapplies after
  a render-context reset or viewport resize, but a transition that triggers
  neither may reset the native setting.

The TPS/extrapolation implementations and native display toggles have not yet
been tested in a live game. The UI reports requests and observed callback/update
rates; it does not claim hardware presentation or monitor refresh measurements.
Geode 5.11 exposes no mod-unload event, so native timer/VSync preferences cannot
be reliably restored to their pre-mod values on dynamic unload; they remain
applied for the game process lifetime.

Most other menu rows are still UI scaffolding and remain dimmed/non-interactive.
The menu is organized into MegaHack-style panels, including Bypass, Level,
Status, and Replay. The Level and Replay panels are currently incomplete. In
the Bypass window the placeholders include Anti-Kick, Challenge Level,
Keymaster, Main Levels, Music Customiser, Slider Limit, Treasure Room, Unlock
Shops and Unlock Vaults.

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

1. Add state and declarations in `src/Hacks.hpp` or a focused feature header.
2. Add a row in `src/layout.cpp` with its save id and tooltip.
3. Put display-specific controls, configuration, and hooks in
   `src/DisplayHack/`; the CMake source glob includes that directory. Other
   hacks can go in `src/bypass.cpp` or `src/hooks.cpp`.

## If the build fails

- The Geode-compatible `gd-imgui-cocos` revision is pinned in `CMakeLists.txt`;
  update it only after checking its API and building all intended targets.
- Hook signatures and members come from the Geode 5.11 bindings and can change
  between game versions. Check them against the GD 2.2081 bindings.
- `GameManager::isIconUnlocked`, `isColorUnlocked`, and the `CCTextInputNode`
  members were checked against the Geode docs. The Text Length approach itself
  is untested: if the limit is enforced somewhere other than
  `onTextFieldInsertText`, it won't do anything.
- Tab is handled through Geode's global keyboard event, so it works even while
  ImGui is closed. Mobile still needs an on-screen toggle button.

## Fonts

The menu bundles DejaVu Sans under its permissive font license to replace the
pixel-style ImGui default. The font is loaded from the mod's packaged resources.
