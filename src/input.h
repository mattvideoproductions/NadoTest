// Tornado Redemption - keyboard input with rebindable hotkeys (65% keyboards: no F-row / numpad needed).
#pragma once
#include <windows.h>
#include <string>

struct Hotkey
{
	int vk = 0;
	bool ctrl = false, shift = false, alt = false;
	std::string text;
};

Hotkey ParseHotkey(const std::string& s);
void OnKeyboardMessage(DWORD key, WORD repeats, BYTE scanCode, BOOL isExtended, BOOL isWithAlt, BOOL wasDownBefore, BOOL isUpNow);
void InputInit();

// Call once per script frame; returns true if the hotkey was pressed since the last frame.
// allowRepeat lets held keys auto-repeat (used for menu navigation).
void InputBeginFrame();
bool Pressed(const Hotkey& hk, bool allowRepeat = false);
