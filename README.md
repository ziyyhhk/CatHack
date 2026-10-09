# CatHack Menu

Mega Hack style mod menu for Geometry Dash 2.2081, built with Geode + ImGui.
Press **Tab** to open or close it. Windows are draggable and remember their
position and your settings.

> **Status: untested.** This was written without a Geode toolchain available,
> so expect possible binding/signature fixes on first build.

## Working hacks

| Category | Hacks | Notes |
| --- | --- | --- |
| Bypass | Text Length, Unlock Icons | local only |
| Speedhack | Enabled + speed slider | marks run as cheated |
| Cosmetic | No Death Effect | |
| Player | Noclip | respects anticheat spike |
| Cheat Safety | Safe Mode, Auto Safe Mode | sets `m_isTestMode` on completion so the run isn't saved |
| Display | Unlock FPS + FPS box | `CCApplication::setAnimationInterval` |
| CatHack | Interface Scale | ImGui font scale |

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
- Windows have a soft shadow, are draggable, and remember their position and
  collapsed state.
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
- If Tab does nothing, imgui-cocos may not be forwarding the key. Hook
  `CCKeyboardDispatcher::dispatchKeyboardMSG` and call `setOpen()` yourself.
- Desktop only for now (Tab). Mobile would need an on-screen button.

## Fonts

The default ImGui font is used. To get the Mega Hack look, load a TTF in the
`setup` callback in `src/menu.cpp` with `io.Fonts->AddFontFromFileTTF(...)`.
