// NadoTest - controller input implementation.
#include "pad.h"
#include "common.h"
#include <Xinput.h>
#include <cctype>
#include <cstdio>

typedef DWORD(WINAPI* XInputGetStateFn)(DWORD, XINPUT_STATE*);
static XInputGetStateFn g_xGetState = nullptr;
static int g_xSlot = -1;              // connected XInput slot, -1 = none
static float g_nextSlotScan = 0;      // polling empty slots is slow, so only look for new pads every few seconds

static unsigned g_down = 0, g_prev = 0, g_consumed = 0;
static float g_nextRepeat[PB_COUNT] = {};
static unsigned g_repeatFired = 0;
static float g_lastPadUse = -100, g_lastKeyboardUse = -100;
static bool g_usedXInput = false, g_usedGame = false;
static PadAnalog g_analog;
static bool g_analogOk = false;

static const char* kNames[PB_COUNT] = { "DPAD_UP", "DPAD_DOWN", "DPAD_LEFT", "DPAD_RIGHT", "A", "B", "X", "Y", "LB", "RB", "LS", "RS", "BACK", "START" };
const char* PadButtonName(int b) { return (b >= 0 && b < PB_COUNT) ? kNames[b] : "?"; }

// The game's control actions are joaat hashes of their names (checked against known values: INPUT_JUMP = 0xD9D0E1C0).
static Hash g_gameAction[PB_COUNT];

void PadInit()
{
	const char* dlls[] = { "xinput1_4.dll", "xinput1_3.dll", "xinput9_1_0.dll" };
	for (const char* d : dlls)
	{
		HMODULE m = LoadLibraryA(d);
		if (m && (g_xGetState = (XInputGetStateFn)GetProcAddress(m, "XInputGetState")) != nullptr)
		{
			Log("pad: XInput loaded (%s)", d);
			break;
		}
	}
	if (!g_xGetState)
		Log("pad: XInput not available - using the game's control actions only");
	const char* actions[PB_COUNT] = { "INPUT_FRONTEND_UP", "INPUT_FRONTEND_DOWN", "INPUT_FRONTEND_LEFT", "INPUT_FRONTEND_RIGHT",
		"INPUT_FRONTEND_ACCEPT", "INPUT_FRONTEND_CANCEL", "INPUT_FRONTEND_X", "INPUT_FRONTEND_Y", "INPUT_FRONTEND_LB", "INPUT_FRONTEND_RB",
		"INPUT_FRONTEND_LS", "INPUT_FRONTEND_RS", "INPUT_FRONTEND_SELECT", "INPUT_FRONTEND_PAUSE" };
	for (int i = 0; i < PB_COUNT; i++)
		g_gameAction[i] = Joaat(actions[i]);
}

static unsigned ReadXInput(float now)
{
	if (!g_xGetState)
		return 0;
	XINPUT_STATE st;
	if (g_xSlot >= 0)
	{
		ZeroMemory(&st, sizeof(st));
		if (g_xGetState((DWORD)g_xSlot, &st) != ERROR_SUCCESS)
		{
			Log("pad: XInput slot %d disconnected", g_xSlot);
			g_xSlot = -1;
			g_analogOk = false;
			g_nextSlotScan = now + 1.0f;
			return 0;
		}
	}
	else
	{
		if (now < g_nextSlotScan)
			return 0;
		g_nextSlotScan = now + 3.0f;
		for (DWORD i = 0; i < 4; i++)
		{
			ZeroMemory(&st, sizeof(st));
			if (g_xGetState(i, &st) == ERROR_SUCCESS)
			{
				g_xSlot = (int)i;
				Log("pad: XInput controller found in slot %d", g_xSlot);
				break;
			}
		}
		if (g_xSlot < 0)
			return 0;
	}
	auto stick = [](SHORT v, SHORT dead) { float f = v / 32767.0f; float d = dead / 32767.0f; return fabsf(f) < d ? 0.0f : (f - (f > 0 ? d : -d)) / (1.0f - d); };
	g_analog.lx = stick(st.Gamepad.sThumbLX, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
	g_analog.ly = stick(st.Gamepad.sThumbLY, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
	g_analog.rx = stick(st.Gamepad.sThumbRX, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);
	g_analog.ry = stick(st.Gamepad.sThumbRY, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);
	g_analog.lt = st.Gamepad.bLeftTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD ? st.Gamepad.bLeftTrigger / 255.0f : 0.0f;
	g_analog.rt = st.Gamepad.bRightTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD ? st.Gamepad.bRightTrigger / 255.0f : 0.0f;
	g_analogOk = true;
	WORD w = st.Gamepad.wButtons;
	unsigned b = 0;
	if (w & XINPUT_GAMEPAD_DPAD_UP) b |= 1u << PB_UP;
	if (w & XINPUT_GAMEPAD_DPAD_DOWN) b |= 1u << PB_DOWN;
	if (w & XINPUT_GAMEPAD_DPAD_LEFT) b |= 1u << PB_LEFT;
	if (w & XINPUT_GAMEPAD_DPAD_RIGHT) b |= 1u << PB_RIGHT;
	if (w & XINPUT_GAMEPAD_A) b |= 1u << PB_A;
	if (w & XINPUT_GAMEPAD_B) b |= 1u << PB_B;
	if (w & XINPUT_GAMEPAD_X) b |= 1u << PB_X;
	if (w & XINPUT_GAMEPAD_Y) b |= 1u << PB_Y;
	if (w & XINPUT_GAMEPAD_LEFT_SHOULDER) b |= 1u << PB_LB;
	if (w & XINPUT_GAMEPAD_RIGHT_SHOULDER) b |= 1u << PB_RB;
	if (w & XINPUT_GAMEPAD_LEFT_THUMB) b |= 1u << PB_LS;
	if (w & XINPUT_GAMEPAD_RIGHT_THUMB) b |= 1u << PB_RS;
	if (w & XINPUT_GAMEPAD_BACK) b |= 1u << PB_BACK;
	if (w & XINPUT_GAMEPAD_START) b |= 1u << PB_START;
	return b;
}

static unsigned ReadGameControls()
{
	// Keyboard arrows/Enter also drive the frontend actions; the keyboard hook already handles those.
	if (PAD::IS_USING_KEYBOARD_AND_MOUSE(0))
		return 0;
	unsigned b = 0;
	for (int i = 0; i < PB_COUNT; i++)
		if (PAD::IS_DISABLED_CONTROL_PRESSED(0, g_gameAction[i]))   // reports the state even while we disable it
			b |= 1u << i;
	return b;
}

void PadBeginFrame(float now)
{
	g_prev = g_down;
	g_consumed = 0;
	g_repeatFired = 0;
	unsigned x = ReadXInput(now);
	unsigned game = g_xSlot >= 0 ? 0 : ReadGameControls();   // one source at a time: no double presses
	g_down = x | game;
	if (x && !g_usedXInput) { g_usedXInput = true; Log("pad: first button press seen through XInput"); }
	if (game && !g_usedGame) { g_usedGame = true; Log("pad: first button press seen through the game's control actions"); }
	if (g_down)
		g_lastPadUse = now;
	if (PAD::IS_USING_KEYBOARD_AND_MOUSE(0))   // the game flips this to false as soon as a pad is used
		g_lastKeyboardUse = now;
	for (int i = 0; i < PB_COUNT; i++)
	{
		unsigned m = 1u << i;
		if ((g_down & m) && !(g_prev & m))
			g_nextRepeat[i] = now + 0.38f;
		else if ((g_down & m) && now >= g_nextRepeat[i])
		{
			g_nextRepeat[i] = now + 0.085f;
			g_repeatFired |= m;
		}
	}
}

bool PadHeld(int b) { return b >= 0 && (g_down & (1u << b)) != 0; }

bool PadGetAnalog(PadAnalog* out)
{
	if (!g_analogOk || g_xSlot < 0) { *out = PadAnalog(); return false; }
	*out = g_analog;
	return true;
}

bool PadPressed(int b, bool allowRepeat)
{
	if (b < 0) return false;
	unsigned m = 1u << b;
	if (g_consumed & m) return false;
	if ((g_down & m) && !(g_prev & m)) return true;
	return allowRepeat && b <= PB_RIGHT && (g_repeatFired & m);
}

void PadConsume(int b) { if (b >= 0) g_consumed |= 1u << b; }

bool PadComboPressed(const PadCombo& c)
{
	if (c.press < 0) return false;
	if (c.hold >= 0 && !PadHeld(c.hold)) return false;
	if (!PadPressed(c.press)) return false;
	PadConsume(c.press);
	return true;
}

bool PadRecentlyUsed() { return g_lastPadUse > g_lastKeyboardUse && g_lastPadUse > 0; }

const char* PadSource()
{
	static char b[48];
	if (g_xSlot >= 0) { sprintf_s(b, "XInput slot %d", g_xSlot); return b; }
	return g_usedGame ? "game controls" : "none yet";
}

static int ButtonFromName(const std::string& n)
{
	static const struct { const char* name; int b; } kAlias[] = {
		{ "DPAD_UP", PB_UP }, { "UP", PB_UP }, { "DPAD_DOWN", PB_DOWN }, { "DOWN", PB_DOWN },
		{ "DPAD_LEFT", PB_LEFT }, { "LEFT", PB_LEFT }, { "DPAD_RIGHT", PB_RIGHT }, { "RIGHT", PB_RIGHT },
		{ "A", PB_A }, { "CROSS", PB_A }, { "B", PB_B }, { "CIRCLE", PB_B }, { "X", PB_X }, { "SQUARE", PB_X },
		{ "Y", PB_Y }, { "TRIANGLE", PB_Y }, { "LB", PB_LB }, { "L1", PB_LB }, { "RB", PB_RB }, { "R1", PB_RB },
		{ "LS", PB_LS }, { "L3", PB_LS }, { "RS", PB_RS }, { "R3", PB_RS }, { "BACK", PB_BACK }, { "VIEW", PB_BACK },
		{ "SELECT", PB_BACK }, { "START", PB_START }, { "MENU", PB_START }, { "OPTIONS", PB_START },
	};
	for (auto& a : kAlias)
		if (n == a.name) return a.b;
	return -1;
}

PadCombo ParsePadCombo(const std::string& raw)
{
	PadCombo c;
	c.text = raw;
	std::string s;
	for (char ch : raw)
		if (!isspace((unsigned char)ch)) s += (char)toupper((unsigned char)ch);
	if (s.empty() || s == "NONE" || s == "OFF")
		return c;
	size_t plus = s.find('+');
	if (plus == std::string::npos)
		c.press = ButtonFromName(s);
	else
	{
		c.hold = ButtonFromName(s.substr(0, plus));
		c.press = ButtonFromName(s.substr(plus + 1));
	}
	if (c.press < 0)
		Log("pad: could not read controller combo '%s' (check NadoTest.ini)", raw.c_str());
	return c;
}
