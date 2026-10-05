#include "Hacks.hpp"

namespace cat {

State& state() {
    static State s;
    return s;
}

// Windows are placed column by column like the screenshot. Rows with only a
// label are placeholders. To make one real, give it a bool in State, add a
// pointer + save id here, and read the bool from a hook in hooks.cpp.
std::vector<Window> const& layout() {
    static std::vector<Window> windows = [] {
        auto& s = state();
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
                 "Removes the client-side character limit on text boxes. The servers still enforce their own limits."}, {"Treasure Room"},
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
            {"Player", 4, Extra::None, false, {
                {"Noclip", &s.noclip, "noclip",
                 "You cannot die (except to the anticheat spike). Marks the run as cheated."},
                {"No Clip Accuracy"}, {"No Clip Deaths"}, {"No Mirror Portal"},
                {"No Reverse"}, {"No Rotate"}, {"No Wave Collision"},
                {"Show Hitboxes"}, {"Show Trajectory"},
            }},
            {"Bot", 4, Extra::None, false, {
                {"Enabled"}, {"Record"}, {"Play"}, {"Save"}, {"Load"},
            }},

            // ---- column 5
            {"Labels", 5, Extra::None, false, {
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
            {"Display", 8, Extra::Fps, false, {
                {"Unlock FPS", &s.fpsEnabled, "unlock-fps",
                 "Uses the FPS value above instead of the default 60."},
                {"Frame Extrapolation"}, {"Vertical Sync"}, {"Lock Delta"},
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
        };
    }();
    return windows;
}

} // namespace cat
