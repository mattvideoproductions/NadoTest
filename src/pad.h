// Tornado Redemption - controller input (v0.4, playtest 4: "I would appreciate straight up controller controls").
// Two sources, whichever sees the pad:
//   1. XInput, read directly (Xbox pads, and most pads routed through Steam Input);
//   2. the game's own control actions (INPUT_FRONTEND_*), for pads RDR2 reads natively (e.g. a DualSense).
// Only one source is used at a time so a press is never counted twice; the log says which one worked.
#pragma once
#include <string>

enum PadButton { PB_UP, PB_DOWN, PB_LEFT, PB_RIGHT, PB_A, PB_B, PB_X, PB_Y, PB_LB, PB_RB, PB_LS, PB_RS, PB_BACK, PB_START, PB_COUNT };

// "RB+DPAD_RIGHT" = hold RB, press D-pad right. A single name ("Y") = just press it.
struct PadCombo { int hold = -1; int press = -1; std::string text; };
PadCombo ParsePadCombo(const std::string& s);

void PadInit();                       // loads XInput if present (no link dependency)
void PadBeginFrame(float now);        // call once per script frame
bool PadPressed(int button, bool allowRepeat = false);   // went down this frame (d-pad auto-repeats when allowed)
bool PadHeld(int button);
bool PadComboPressed(const PadCombo& c);                  // also consumes the press so it isn't reused this frame
void PadConsume(int button);          // ignore this button's press for the rest of the frame
bool PadRecentlyUsed();               // the pad was used more recently than the keyboard (show pad hints)
const char* PadSource();              // "XInput slot N", "game controls" or "none yet"
const char* PadButtonName(int button);
// v1.1 (the jet balloon): analog sticks and triggers from XInput, -1..1 (triggers 0..1). False if no XInput pad.
struct PadAnalog { float lx = 0, ly = 0, rx = 0, ry = 0, lt = 0, rt = 0; };
bool PadGetAnalog(PadAnalog* out);
