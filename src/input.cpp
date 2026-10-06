// NadoTest - keyboard input.
#include "input.h"
#include <vector>
#include <algorithm>
#include <cctype>

struct KeyEvent { int vk; bool ctrl, shift, alt, repeat; };

static CRITICAL_SECTION g_lock;
static std::vector<KeyEvent> g_pending;   // filled by the keyboard hook thread
static std::vector<KeyEvent> g_frame;     // events for the current script frame

void InputInit() { InitializeCriticalSection(&g_lock); }

void OnKeyboardMessage(DWORD key, WORD, BYTE, BOOL, BOOL isWithAlt, BOOL wasDownBefore, BOOL isUpNow)
{
	if (isUpNow)
		return;
	KeyEvent e;
	e.vk = (int)key;
	e.ctrl = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
	e.shift = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
	e.alt = isWithAlt != 0;
	e.repeat = wasDownBefore != 0;
	EnterCriticalSection(&g_lock);
	if (g_pending.size() < 64)
		g_pending.push_back(e);
	LeaveCriticalSection(&g_lock);
}

void InputBeginFrame()
{
	EnterCriticalSection(&g_lock);
	g_frame.swap(g_pending);
	g_pending.clear();
	LeaveCriticalSection(&g_lock);
}

bool Pressed(const Hotkey& hk, bool allowRepeat)
{
	if (!hk.vk)
		return false;
	for (auto& e : g_frame)
		if (e.vk == hk.vk && e.ctrl == hk.ctrl && e.shift == hk.shift && e.alt == hk.alt && (allowRepeat || !e.repeat))
			return true;
	return false;
}

static int NamedKey(const std::string& n)
{
	static const struct { const char* name; int vk; } kNames[] = {
		{ "BACKSLASH", VK_OEM_5 }, { "LBRACKET", VK_OEM_4 }, { "RBRACKET", VK_OEM_6 }, { "SEMICOLON", VK_OEM_1 },
		{ "QUOTE", VK_OEM_7 }, { "COMMA", VK_OEM_COMMA }, { "PERIOD", VK_OEM_PERIOD }, { "SLASH", VK_OEM_2 },
		{ "MINUS", VK_OEM_MINUS }, { "EQUALS", VK_OEM_PLUS }, { "GRAVE", VK_OEM_3 },
		{ "UP", VK_UP }, { "DOWN", VK_DOWN }, { "LEFT", VK_LEFT }, { "RIGHT", VK_RIGHT },
		{ "ENTER", VK_RETURN }, { "BACKSPACE", VK_BACK }, { "DELETE", VK_DELETE }, { "INSERT", VK_INSERT },
		{ "HOME", VK_HOME }, { "END", VK_END }, { "PGUP", VK_PRIOR }, { "PGDN", VK_NEXT },
		{ "SPACE", VK_SPACE }, { "TAB", VK_TAB }, { "ESC", VK_ESCAPE },
	};
	for (auto& k : kNames)
		if (n == k.name)
			return k.vk;
	if (n.size() == 1 && (isalnum((unsigned char)n[0])))
		return toupper((unsigned char)n[0]);
	if (n.size() >= 2 && n[0] == 'F' && isdigit((unsigned char)n[1]))
		return VK_F1 + atoi(n.c_str() + 1) - 1;
	if (n.rfind("NUMPAD", 0) == 0 && n.size() == 7)
		return VK_NUMPAD0 + (n[6] - '0');
	if (n.rfind("0X", 0) == 0)
		return (int)strtol(n.c_str(), nullptr, 16);
	return 0;
}

Hotkey ParseHotkey(const std::string& raw)
{
	Hotkey hk;
	hk.text = raw;
	std::string s;
	for (char c : raw)
		if (!isspace((unsigned char)c))
			s += (char)toupper((unsigned char)c);
	size_t start = 0;
	while (start <= s.size())
	{
		size_t plus = s.find('+', start);
		std::string part = s.substr(start, plus == std::string::npos ? std::string::npos : plus - start);
		if (part == "CTRL") hk.ctrl = true;
		else if (part == "SHIFT") hk.shift = true;
		else if (part == "ALT") hk.alt = true;
		else if (!part.empty()) hk.vk = NamedKey(part);
		if (plus == std::string::npos) break;
		start = plus + 1;
	}
	return hk;
}
