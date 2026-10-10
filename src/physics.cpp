#include "Hacks.hpp"

// The previous CatHack hook replaced GJBaseGameLayer::getModifiedDelta(float)
// and returned a changed delta. It did not call or schedule additional
// GJBaseGameLayer::update invocations, so that implementation could not claim
// that a requested TPS was the achieved gameplay-update rate.
//
// The GD 2.2081 bindings expose the layer update and modified-delta methods,
// but no isolated physics-substep API. Calling the full layer update again to
// manufacture ticks could repeat input, triggers, collision checks, and
// postUpdate side effects. Keep Physics TPS unavailable until an engine-safe
// substep path is verified against the game implementation and runtime.
