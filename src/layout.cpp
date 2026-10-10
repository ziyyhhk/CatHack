#include "Hacks.hpp"

namespace cat {

State& state() {
    static State s;
    return s;
}

// Windows are placed column by column like the reference menu. Rows with only
// a label are intentionally disabled placeholders until their feature exists.
std::vector<Window> const& layout() {
    static std::vector<Window> windows = [] {
        auto& s = state();
        auto& d = display::settings();
        return std::vector<Window>{
            // ---- column 0
            {"CatHack", 0, Extra::Scale, false, {
                {"Auto-Select"}, {"Auto-Update"}, {"Theme"}, {"Rulesets"},
                {"Sort Interface"}, {"Miscellaneous"},
            }},
            {"Screenshot", 0, Extra::None, true, {
                {"Screenshot"}, {"Mode: Save & copy"},
            }},

            // ---- column 1
            {"Bypass", 1, Extra::None, false, {
                {"Anti-Kick"}, {"Challenge Level"}, {"Keymaster"}, {"Main Levels"},
                {"Music Customiser"}, {"Slider Limit"}, {"Text Length", &s.textLength, "text-length",
                 "Removes the client-side character limit on text boxes. The servers still enforce their own limits."},
                {"Treasure Room"},
                {"Unlock Icons", &s.unlockIcons, "unlock-icons",
                 "Treats every icon and color as unlocked in the garage. Local only, nothing is sent to the servers."},
                {"Unlock Shops"}, {"Unlock Vaults"},
            }},
            {"Speedhack", 1, Extra::Speed, false, {
                {"Enabled", &s.speedEnabled, "speedhack",
                 "Runs the whole game at the speed set above. Marks the run as cheated while you are in a level."},
                {"Speedhack Audio"}, {"Classic Mode"},
            }},

            // ---- column 2
            {"Creator", 2, Extra::None, false, {
                {"Accurate Save"}, {"Copy Hack"}, {"Custom Object Bypass"},
                {"Default Song Bypass"}, {"Editor Extension"}, {"Free Scroll"},
                {"Hide UI"}, {"Level Edit"}, {"Multiple Editor Trails"}, {"No C Mark"},
                {"Place Over"}, {"Smooth Editor Trail"}, {"Toolbox Button Bypass"},
                {"Trigger Value Bypass"},
            }},

            // ---- column 3
            {"Cosmetic", 3, Extra::None, false, {
                {"Accurate Percentage"}, {"Ball Rotation Bug"}, {"Classic Particles"},
                {"Classic Pulse"}, {"Classic Wave Trail"}, {"Coin Shower"},
                {"Frozen Animation"}, {"Hide Attempts"}, {"Hide Pause Button"},
                {"Hide Practice Buttons"}, {"Hide Trail"}, {"No Death Effect", &s.noDeathEffect, "no-death-effect",
                 "Skips the death particle effect when you die."},
                {"No Glow"}, {"No Mirror Force"}, {"No Portal Force"},
                {"No Respawn Flash"}, {"No Wave Pulse"}, {"Practice Music Hack"},
                {"Solid Wave Trail"}, {"Trail Always On"},
            }},

            // ---- column 4
            {"Level", 4, Extra::None, false, {
                {"0% Practice Complete"}, {"Allow Pause Buffering"}, {"All Modes Platformer"},
                {"Auto Clicker"}, {"Auto Deafen"}, {"Auto Kill"}, {"Auto Music Sync"},
                {"Auto Pickup Coins"}, {"Auto Song Download"}, {"Click Between Frames"},
                {"Click on Steps"}, {"Checkpoint Limit Bypass"}, {"Collect Coins in Practice"},
                {"Confirm Exit"}, {"Confirm Full Reset"}, {"Confirm Normal"},
                {"Confirm Practice"}, {"Confirm Reset"}, {"Force Ice"},
                {"Force Platformer"}, {"Frame Step"}, {"Hitbox Multiplier"},
                {"Instant Complete"}, {"Jumpscare"}, {"Jump Hack"},
                {"Noclip", &s.noclip, "noclip",
                 "You cannot die (except to the anticheat spike). Marks the run as cheated."},
                {"Noclip Limits"}, {"No Collision"}, {"No Mirror Portal"},
                {"No Reverse"}, {"No Rotate"}, {"No Wave Collision"},
                {"Pause During Complete"}, {"Practice Bug Fix"}, {"Practice Music"},
                {"Random Seed"}, {"Replay Last Checkpoint"}, {"Respawn Time"},
                {"Show Hitboxes"}, {"Show Hitboxes on Death"}, {"Show Hitboxes Trail"},
                {"Show Layout"}, {"Show Trajectory"}, {"Show Triggers"},
                {"Smart StartPos"}, {"StartPos Switcher"},
            }},

            // ---- column 5
            {"Status", 5, Extra::None, false, {
                {"Field Formatting"}, {"Font: Big Font"}, {"Hide Status"}, {"Message"},
                {"Testmode"}, {"Cheat Indicator"}, {"FPS Counter"}, {"CPS Counter"},
                {"Best Run"}, {"Noclip Accuracy"}, {"Noclip Deaths"}, {"Attempts"},
                {"Jumps"}, {"Percentage"}, {"Level Time"}, {"Session Time"}, {"Clock"},
                {"Frame Counter"}, {"Position"}, {"Velocity"}, {"Dead"}, {"Replay State"},
            }},

            // ---- column 6
            {"Universal", 6, Extra::None, false, {
                {"Allow Low Volume"}, {"Compact Lists"}, {"Custom Background"},
                {"Fast Chests"}, {"Load Audio to Memory"}, {"Lock Cursor"},
                {"Main Menu Play"}, {"No Music Fade Out"}, {"No Transition"},
                {"Pitch Shifter"}, {"Thread Priority"}, {"Transition Customiser"},
                {"Transparent Lists"},
            }},

            // ---- column 7
            {"Cheat Safety", 7, Extra::None, false, {
                {"Disable Cheats"},
                {"Auto Safe Mode", &s.autoSafeMode, "auto-safe-mode",
                 "Turns Safe Mode on by itself once a cheat has touched the level."},
                {"Safe Mode", &s.safeMode, "safe-mode",
                 "Completions are treated as test runs: not saved to your stats and not submitted."},
                {"Safe Mode Popup"},
            }},
            {"Interface", 7, Extra::None, false, {
                {"Hide Endscreen Cheats"}, {"Hide Endscreen Extras"}, {"Hide Menu Snow"},
                {"Hide Iconic on Pause"}, {"Hide RobsVault Shortcut"},
            }},

            // ---- column 8
            {"Display", 8, Extra::Display, false, {
                {"FPS Bypass", &d.fpsBypass, "unlock-fps",
                 "Applies the independent FPS target above through GD's render timer. The observed frame-callback rate is reported separately and is not a hardware-present or monitor-refresh measurement."},
                {"TPS Bypass", &d.tpsBypass, "physics-tps-enabled",
                 "Applies the independent gameplay TPS target by running GJBaseGameLayer::update in bounded fixed-size substeps while PlayLayer is active. A 3000 TPS target is accepted; no 240 cap is used. Requests requiring steps shorter than 1 microsecond or longer than 250 milliseconds are retained but not applied. At most 512 calls run per outer update, so compare observed gameplay updates/s with the requested target. Any applied TPS other than 240 marks the run cheated and may change physics/collisions."},
                {"HZ Bypass", cat::display::kHzBypassSupported ? &d.hzBypass : nullptr,
                 "hz-bypass",
                 cat::display::kHzBypassSupported
                    ? "Calls Geometry Dash's native ForceTimer toggle. The binding establishes that it is a timer control, but its exact platform effect is not confirmed; it does not set the monitor's physical refresh rate and is separate from FPS, TPS, and VSync."
                    : "Unavailable: the GD 2.2081 Android bindings do not expose the native ForceTimer dynamic setter. This feature does not change physical monitor refresh."},
                {"Frame Extrapolation", cat::display::kFrameExtrapolationSupported ? &d.frameExtrapolation : nullptr,
                 "frame-extrapolation",
                 cat::display::kFrameExtrapolationSupported
                    ? "Uses previous PlayerObject simulation positions, simulation-step delta, and monotonic sample timestamps to predict a bounded visual offset while the active PlayLayer subtree is visited for drawing. The base Cocos transform is restored before the visit returns; authoritative player position and collision state are not changed. History resets on death, pause/resume, restart, discontinuities, and scene exit."
                    : "Unavailable on Android: the GD 2.2081 bindings do not expose the GJBaseGameLayer::visit hook needed to apply a render-only offset. The feature is implemented on Windows, macOS, and iOS; it is not substituted with Smooth Fix or a gameplay-position change."},
                {"Vertical Sync", cat::display::kVerticalSyncSupported ? &d.verticalSync : nullptr,
                 "vertical-sync",
                 cat::display::kVerticalSyncSupported
                    ? "Requests Geometry Dash's native VSync setting. VSync is separate from FPS/TPS and may clamp rendering to the display. CatHack reapplies saved preferences after a render-context reset or viewport resize; the Windows 2.2081 loadVideoSettings binding is inline, so a transition that triggers neither may reset the native setting."
                    : "Unavailable: the GD 2.2081 Android bindings do not expose a supported dynamic VSync control."},
                {"Lock Delta"},
                {"Borderless Classic"}, {"Fullscreen"},
            }},
            {"Keybinds", 8, Extra::None, false, {
                {"Choose Hack to Set"}, {"View Keybinds"}, {"Remove Keybinds"},
                {"Disable in Editor"},
            }},

            // ---- column 9
            {"Utility", 9, Extra::None, true, {
                {"Uncomplete Level"}, {"Restart Level"}, {"Practice Mode"},
                {"Settings"}, {"Resources"}, {"AppData"}, {"Toggle DevTools"},
            }},
            {"Replay", 9, Extra::None, false, {
                {"Record"}, {"Play"}, {"Filename"}, {"Auto-save"}, {"Save"},
                {"Clear & New"}, {"Delete"}, {"Gameplay Options"},
                {"Convert (.json, .gdr)"}, {"Open Folder"},
            }},
        };
    }();
    return windows;
}

} // namespace cat
