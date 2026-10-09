// Tornado Redemption v1.1 - the RDR2-style UI kit. See ui.h.
#include "ui.h"
#include <cstdio>
#include <cstdint>
#include <vector>
#include <string>
#include <cstring>

namespace UI
{
bool g_simple = false;
bool g_clean = false;
bool g_mute = false;

// ---------- screen ----------
static int g_screenW = 1920, g_screenH = 1080;
static float g_nextScreenCheck = 0;
static BOOL CALLBACK FindGameWindow(HWND w, LPARAM lp)
{
	DWORD pid = 0;
	GetWindowThreadProcessId(w, &pid);
	if (pid != GetCurrentProcessId() || !IsWindowVisible(w)) return TRUE;
	RECT rc;
	if (GetClientRect(w, &rc) && rc.right * rc.bottom > 0)
	{
		int* best = (int*)lp;
		if (rc.right * rc.bottom > best[0] * best[1]) { best[0] = rc.right; best[1] = rc.bottom; }
	}
	return TRUE;
}
static void CheckScreen(float t)
{
	if (t < g_nextScreenCheck) return;
	g_nextScreenCheck = t + 5.0f;
	int best[2] = { 0, 0 };
	EnumWindows(FindGameWindow, (LPARAM)best);
	if (best[0] > 320 && best[1] > 200) { g_screenW = best[0]; g_screenH = best[1]; }
	else { g_screenW = GetSystemMetrics(SM_CXSCREEN); g_screenH = GetSystemMetrics(SM_CYSCREEN); }
	if (g_screenW <= 0 || g_screenH <= 0) { g_screenW = 1920; g_screenH = 1080; }
}
float Aspect() { return (float)g_screenW / (float)std::max(1, g_screenH); }

// ---------- text ----------
// Rockstar's menus pick the face and the alignment with markup inside the string (Halen84's native menu base):
// <TEXTFORMAT RIGHTMARGIN='px'><P ALIGN='Left|Center|Right'><FONT FACE='$face'>~s~text</FONT></P><TEXTFORMAT>
void Text(const char* s, float x, float y, float scale, int r, int g, int b, int a, Align al, const char* face, bool shadow)
{
	if (!s || !*s || a <= 0) return;
	if (g_simple)
	{
		float w = al == LEFT ? 0 : TextWidth(s, scale);
		DrawTextLine(s, al == LEFT ? x : al == CENTRE ? x - w / 2 : x - w, y, scale, r, g, b, a);
		return;
	}
	// v1.1 audit: a "<" or ">" in the text would be read as markup. Audit 2: so would a bare "&" ("MODES & TOYS"); one that
	// already starts an entity (&amp; &lt; &gt; &quot; &apos; &#...;) is left as it is, so nothing is escaped twice.
	static const char* kEntities[] = { "&amp;", "&lt;", "&gt;", "&quot;", "&apos;", "&#" };
	std::string esc;
	for (const char* c = s; *c; c++)
	{
		if (*c == '<') esc += "&lt;";
		else if (*c == '>') esc += "&gt;";
		else if (*c == '&')
		{
			bool entity = false;
			for (const char* e : kEntities) if (!strncmp(c, e, strlen(e))) entity = true;
			esc += entity ? "&" : "&amp;";
		}
		else esc += *c;
	}
	s = esc.c_str();
	float bx = x;
	int rightPx = 0;
	const char* p = "Left";
	if (al == CENTRE) { bx = 2.0f * x - 1.0f; p = "Center"; }
	else if (al == RIGHT) { bx = 0.0f; rightPx = (int)((1.0f - x) * g_screenW); p = "Right"; }
	char buf[1400];
	sprintf_s(buf, "<TEXTFORMAT RIGHTMARGIN='%d'><P ALIGN='%s'><FONT FACE='$%s'>~s~%s</FONT></P><TEXTFORMAT>", rightPx, p, face, s);
	if (shadow)
	{
		UIDEBUG::BG_SET_TEXT_SCALE(scale, scale);
		UIDEBUG::BG_SET_TEXT_COLOR(0, 0, 0, a * 3 / 4);
		UIDEBUG::BG_DISPLAY_TEXT(MISC::VAR_STRING_LITERAL(buf), bx + 0.0012f, y + 0.0016f);
	}
	UIDEBUG::BG_SET_TEXT_SCALE(scale, scale);
	UIDEBUG::BG_SET_TEXT_COLOR(r, g, b, a);
	UIDEBUG::BG_DISPLAY_TEXT(MISC::VAR_STRING_LITERAL(buf), bx, y);
}

float TextWidth(const char* s, float scale)
{
	// measured on v1.0's HUD: about 0.0174 x scale per character in the debug face; markup tags don't count
	int n = 0;
	for (const char* c = s; *c; c++)
	{
		if (*c == '~') { const char* e = strchr(c + 1, '~'); if (e) { c = e; continue; } }
		n++;
	}
	return n * 0.0174f * scale;
}

// ---------- sprites ----------
bool Sprite(const char* dict, const char* tex, float cx, float cy, float w, float h, int r, int g, int b, int a, float rot)
{
	if (g_simple || a <= 0) return false;
	if (!TXD::HAS_STREAMED_TEXTURE_DICT_LOADED(dict))
	{
		TXD::REQUEST_STREAMED_TEXTURE_DICT(dict, FALSE);
		return false;
	}
	GRAPHICS::SET_SCRIPT_GFX_DRAW_ORDER(0);
	GRAPHICS::DRAW_SPRITE(dict, tex, cx, cy, w, h, rot, r, g, b, a, FALSE);
	return true;
}

void Panel(float x, float y, float w, float h, int a)
{
	if (!Sprite("generic_textures", "inkroller_1a", x + w / 2, y + h / 2, w, h, 0, 0, 0, a))
		DrawBox(x, y, w, h, 10, 8, 7, a * 9 / 10);
}

static void SoftBox(float cx, float cy, float w, float h, int a)
{
	// the soft-edged dark plate the game puts behind help text (feeds/help_text_bg); a plain box if it isn't there
	if (!Sprite("feeds", "help_text_bg", cx, cy, w, h, 0, 0, 0, a))
		DrawBox(cx - w / 2, cy - h / 2, w, h, 8, 6, 5, a * 3 / 4);
}

void Sound(const char* name, const char* set)
{
	if (g_mute) return;
	AUDIO::STOP_SOUND_WITH_NAME(name, set);
	AUDIO::PLAY_SOUND_FRONTEND(name, set, TRUE, 0);
}

// ---------- timed widgets ----------
struct Timed { std::string a, b; float until = 0, start = 0, len = 0; };
static float g_now = 0, g_dt = 0;
static float g_letter = 0, g_letterWant = 0;
static Timed g_objective, g_help, g_shard, g_toast, g_honor;

static float Fade(const Timed& w, float in = 0.35f, float out = 0.5f)
{
	if (g_now >= w.until) return 0;
	float a = Clamp((g_now - w.start) / in, 0, 1), b = Clamp((w.until - g_now) / out, 0, 1);
	return std::min(a, b);
}
static void Set(Timed& w, const char* a, const char* b, float seconds)
{
	w.a = a ? a : ""; w.b = b ? b : "";
	w.start = g_now; w.until = (a && *a) ? g_now + seconds : 0; w.len = seconds;
}

void Preload()
{
	static const char* kDicts[] = { "generic_textures", "menu_textures", "feeds", "BLIPS", "TOAST_LOG_BLIPS" };
	for (const char* d : kDicts) TXD::REQUEST_STREAMED_TEXTURE_DICT(d, FALSE);
}

void Frame(float dt)
{
	g_dt = dt;
	g_now = NowSec();
	CheckScreen(g_now);
	g_letter += Clamp(g_letterWant - g_letter, -dt * 1.6f, dt * 1.6f);
}

void Letterbox(float amount) { if (amount < 0) g_letter = 0; g_letterWant = Clamp(amount, 0, 1); }   // < 0: gone at once
void Objective(const char* text, float seconds) { Set(g_objective, text, nullptr, seconds); }
void HelpTip(const char* text, float seconds) { Set(g_help, text, nullptr, seconds); }
void Toast(const char* title, const char* text, float seconds) { Set(g_toast, title, text, seconds); }
static bool g_shardGood = false;   // v1.7: a success (cream), not a failure (red)
void Shard(const char* title, const char* sub, float seconds, bool good) { Set(g_shard, title, sub, seconds); g_shardGood = good; }

// Subtitles, the chapter card and the score plate are drawn in Draw(), after the letterbox, so they sit on top of it.
struct FrameText { std::string a, b, c, d; float alpha = 0, meter = 0; bool on = false; };
static FrameText g_sub, g_card, g_plate, g_place, g_prompt;

void Subtitle(const char* speaker, const char* text, float alpha)
{
	g_sub.on = true; g_sub.a = speaker ? speaker : ""; g_sub.b = text ? text : ""; g_sub.alpha = alpha;
}
static void DrawSubtitle(const char* speaker, const char* text, float alpha)
{
	int a = (int)(255 * Clamp(alpha, 0, 1));
	if (a <= 0) return;
	// on the letterbox, like a film: the speaker in gold, the line in white
	char b[600];
	if (speaker && *speaker) sprintf_s(b, "~COLOR_GOLD~%s~s~   %s", speaker, text);
	else sprintf_s(b, "%s", text);
	float y = g_letter > 0.5f ? 0.905f : 0.84f;
	if (g_simple) { Text(b, 0.5f, y, 0.42f, 240, 236, 225, a, CENTRE); return; }
	Text(b, 0.5f, y, 0.42f, 240, 236, 225, a, CENTRE, "body", true);
}

void ChapterCard(const char* title, const char* sub, float alpha)
{
	g_card.on = true; g_card.a = title ? title : ""; g_card.b = sub ? sub : ""; g_card.alpha = alpha;
}
void PromptMeter(const char* text, float fill)
{
	g_prompt.on = true; g_prompt.a = text ? text : ""; g_prompt.meter = fill;
}
// v1.5: "MASH A / SPACE TO GET UP" over a bar that fills as you press
static void DrawPromptMeter(const char* text, float fill)
{
	float pulse = 0.9f + 0.1f * sinf(g_now * 12.0f);
	SoftBox(0.5f, 0.74f, 0.30f, 0.075f, 200);
	Text(text, 0.5f, 0.716f, 0.42f * pulse, 245, 240, 225, 255, CENTRE, "title", true);
	DrawBox(0.4f, 0.756f, 0.2f, 0.007f, 60, 50, 42, 220);
	DrawBox(0.4f, 0.756f, 0.2f * Clamp(fill, 0, 1), 0.007f, 200, 40, 30, 255);
}
void PlaceCard(const char* place, const char* sub, float alpha, float scale)
{
	g_place.meter = scale;   // (v1.7.1: the size, in the spare slot)
	g_place.on = true; g_place.a = place ? place : ""; g_place.b = sub ? sub : ""; g_place.alpha = alpha;
}
// v1.4: where and when, bottom left above the letterbox, like a film's opening card
static void DrawPlaceCard(const char* place, const char* sub, float alpha, float k = 1.0f)
{
	int a = (int)(255 * Clamp(alpha, 0, 1));
	if (a <= 0) return;
	float y = (g_letter > 0.5f ? 0.765f : 0.80f) - 0.07f * (k - 1.0f);   // (v1.7.1: a bigger card sits higher, clear of the letterbox)
	Text(place, 0.06f, y, 0.62f * k, 236, 230, 214, a, LEFT, "title", true);
	DrawBox(0.06f, y + 0.043f * k, 0.13f * k * Clamp(alpha * 1.3f, 0, 1), 0.0018f * k, 200, 40, 30, a * 3 / 4);
	Text(sub, 0.06f, y + 0.05f * k, 0.36f * (1.0f + (k - 1.0f) * 0.6f), 236, 230, 214, a, LEFT, "body", true);
}
static void DrawChapterCard(const char* title, const char* sub, float alpha)
{
	int a = (int)(255 * Clamp(alpha, 0, 1));
	if (a <= 0) return;
	Text(title, 0.5f, 0.39f, 1.15f, 236, 230, 214, a, CENTRE, "title", true);
	if (!Sprite("generic_textures", "menu_bar", 0.5f, 0.468f, 0.16f * alpha, 0.0022f, 236, 230, 214, a * 3 / 4))
		DrawBox(0.5f - 0.08f * alpha, 0.467f, 0.16f * alpha, 0.0018f, 236, 230, 214, a * 3 / 4);
	Text(sub, 0.5f, 0.478f, 0.62f, 236, 230, 214, a, CENTRE, "body", true);
}

void ScorePlate(const char* title, const char* big, const char* sub, const char* right, float meter)
{
	g_plate.on = true; g_plate.a = title ? title : ""; g_plate.b = big ? big : ""; g_plate.c = sub ? sub : ""; g_plate.d = right ? right : "";
	g_plate.meter = meter;
}
static void DrawScorePlate(const char* title, const char* big, const char* sub, const char* right, float meter)
{
	const float cx = 0.5f, y0 = 0.012f, w = 0.27f, h = big && *big ? 0.098f : 0.06f;
	SoftBox(cx, y0 + h / 2, w * 1.25f, h * 1.25f, 215);
	Text(title, cx, y0 + 0.004f, 0.32f, 220, 200, 160, 255, CENTRE, "title");
	float y = y0 + 0.027f;
	if (big && *big) { Text(big, cx, y, 0.72f, 245, 240, 225, 255, CENTRE, "title", true); y += 0.047f; }
	Text(sub, cx, y, 0.3f, 230, 215, 185, 255, CENTRE, "body");
	if (right && *right) Text(right, cx + w / 2, y0 + 0.004f, 0.27f, 170, 160, 140, 255, RIGHT, "body");
	if (meter > 0)
	{
		DrawBox(cx - w / 2, y0 + h - 0.004f, w, 0.004f, 60, 50, 42, 200);
		DrawBox(cx - w / 2, y0 + h - 0.004f, w * Clamp(meter, 0, 1), 0.004f, 200, 40, 30, 255);
	}
}

int FeedToastIcon(const char* title, const char* sub, const char* dict, const char* tex, int ms)
{
	if (g_simple) return 0;
	// the game's own toast (13-slot config + 8-slot data, every field in its own 8-byte slot - Rockstar's layout)
	struct Cfg { int64_t duration; const char* soundSet; const char* sound; int64_t rest[10]; } c{};
	struct Data { int64_t f0; const char* title; const char* sub; int64_t f3; uint64_t dictHash, texHash, colour; int64_t f7, pad[8]; } d{};   // (zeroed spare slots: audit 2)
	if (!TXD::HAS_STREAMED_TEXTURE_DICT_LOADED(dict)) { TXD::REQUEST_STREAMED_TEXTURE_DICT(dict, FALSE); return 0; }
	c.duration = ms;
	d.title = MISC::VAR_STRING_LITERAL(title);
	d.sub = MISC::VAR_STRING_LITERAL(sub);
	d.dictHash = (uint32_t)Joaat(dict);
	d.texHash = (uint32_t)Joaat(tex);
	d.colour = (uint32_t)Joaat("COLOR_WHITE");
	d.f7 = 1;
	return UIFEED::UI_FEED_POST_SAMPLE_TOAST(reinterpret_cast<int*>(&c), reinterpret_cast<int*>(&d), TRUE, TRUE);
}

void HonorLost(bool hudHidden)
{
	// the game's real "honor decreased" toast and sound (as Shtivi's Duels mod posts it) - it doesn't touch Arthur's honor.
	// v1.1 audit 2: one or the other, never both. With the HUD hidden (the intro) the game's feed probably doesn't show
	// (research: ui_world_research 2.1), so the mod's own sting is drawn instead of posting the toast.
	int id = 0;
	if (!g_simple && !hudHidden)
	{
		struct Cfg { int64_t duration; const char* soundSet; const char* sound; int64_t rest[10]; } c{};
		struct Data { int64_t f0; const char* text; const char* dict; uint64_t icon; int64_t one; uint64_t colour; int64_t quality, pad[9]; } d{};   // (zeroed spare slots: audit 2)
		c.duration = 2500;
		c.soundSet = "Honor_Display_Sounds";
		c.sound = "Honor_Decrease_Big";
		d.text = MISC::VAR_STRING_LABEL("PLAYER_HONOR_CHANGE_NEG");
		d.dict = "ITEMTYPE_TEXTURES";
		d.icon = (uint32_t)Joaat("TRANSACTION_HONOR_BAD");
		d.one = 1;
		d.colour = 859817522u;
		id = UIFEED::UI_FEED_POST_SAMPLE_TOAST_RIGHT(reinterpret_cast<int*>(&c), reinterpret_cast<int*>(&d), TRUE);
	}
	Log("UI honor-lost toast -> %d%s", id, id ? "" : " (drawn by the mod instead)");
	if (!id)
	{
		Set(g_honor, "HONOR", nullptr, 2.6f);
		Sound("Honor_Decrease_Big", "Honor_Display_Sounds");
	}
}

static void DrawLines(const std::string& s, float x, float y, float lh, float scale, int r, int g, int b, int a, const char* face)
{
	size_t start = 0;
	int line = 0;
	while (start <= s.size())
	{
		size_t nl = s.find('\n', start);
		std::string part = s.substr(start, nl == std::string::npos ? std::string::npos : nl - start);
		bool head = line == 0 && s.find('\n') != std::string::npos;
		Text(part.c_str(), x, y + line * lh, head ? scale * 1.05f : scale, head ? 225 : r, head ? 190 : g, head ? 120 : b, a, LEFT, head ? "title" : face);
		line++;
		if (nl == std::string::npos) break;
		start = nl + 1;
	}
}

void Draw()
{
	// letterbox (2.39:1 on a 16:9 screen is ~12.8% top and bottom)
	if (g_letter > 0.001f)
	{
		float e = g_letter * g_letter * (3 - 2 * g_letter), hb = 0.128f * e;
		DrawBox(0, 0, 1, hb, 0, 0, 0, 255);
		DrawBox(0, 1 - hb, 1, hb, 0, 0, 0, 255);
	}
	float f;
	// v1.1 audit 2: Cinematic mode (clean footage) - the film parts stay, the mod's own labels go
	if (g_clean)
	{
		if (g_card.on) DrawChapterCard(g_card.a.c_str(), g_card.b.c_str(), g_card.alpha);
		if (g_place.on) DrawPlaceCard(g_place.a.c_str(), g_place.b.c_str(), g_place.alpha, g_place.meter > 0 ? g_place.meter : 1.0f);
		if (g_sub.on) DrawSubtitle(g_sub.a.c_str(), g_sub.b.c_str(), g_sub.alpha);
		if (g_prompt.on) DrawPromptMeter(g_prompt.a.c_str(), g_prompt.meter);
		g_plate.on = g_card.on = g_sub.on = g_place.on = g_prompt.on = false;
		return;
	}
	if ((f = Fade(g_help)) > 0)
	{
		int lines = 1;
		for (char c : g_help.a) if (c == '\n') lines++;
		float h = 0.022f + lines * 0.027f;
		SoftBox(0.155f, 0.06f + h / 2 - 0.012f, 0.30f, h * 1.3f, (int)(220 * f));
		DrawLines(g_help.a, 0.025f, 0.055f, 0.027f, 0.3f, 240, 236, 225, (int)(255 * f), "body");
	}
	if ((f = Fade(g_toast)) > 0)
	{
		float slide = (1 - Clamp((g_now - g_toast.start) / 0.25f, 0, 1)) * 0.03f;
		SoftBox(0.5f, 0.86f + slide, 0.44f, 0.07f, (int)(210 * f));
		if (!g_toast.b.empty())
		{
			Text(g_toast.a.c_str(), 0.5f, 0.838f + slide, 0.28f, 225, 190, 120, (int)(255 * f), CENTRE, "title");
			Text(g_toast.b.c_str(), 0.5f, 0.86f + slide, 0.32f, 240, 236, 225, (int)(255 * f), CENTRE, "body");
		}
		else
			Text(g_toast.a.c_str(), 0.5f, 0.85f + slide, 0.33f, 240, 236, 225, (int)(255 * f), CENTRE, "body");
	}
	if ((f = Fade(g_shard, 0.25f, 0.6f)) > 0)
	{
		float grow = 0.9f + 0.1f * Clamp((g_now - g_shard.start) / 0.3f, 0, 1);
		SoftBox(0.5f, 0.25f, 0.5f * grow, 0.15f, (int)(200 * f));
		Text(g_shard.a.c_str(), 0.5f, 0.205f, 1.0f * grow, g_shardGood ? 240 : 230, g_shardGood ? 228 : 45, g_shardGood ? 200 : 35, (int)(255 * f), CENTRE, "title", true);
		if (!g_shard.b.empty()) Text(g_shard.b.c_str(), 0.5f, 0.277f, 0.36f, 240, 236, 225, (int)(255 * f), CENTRE, "body", true);
	}
	if ((f = Fade(g_objective, 0.6f, 0.8f)) > 0)
		Text(g_objective.a.c_str(), 0.5f, g_letter > 0.3f ? 0.83f : 0.88f, 0.42f, 240, 236, 225, (int)(255 * f), CENTRE, "body", true);
	if ((f = Fade(g_honor, 0.15f, 0.6f)) > 0)
	{
		// fallback honor sting: a red down-arrow and HONOR, bottom right like the game's
		float pulse = 0.85f + 0.15f * sinf(g_now * 14.0f);
		if (!Sprite("menu_textures", "selection_arrow_right", 0.905f, 0.80f, 0.018f, 0.018f * Aspect(), 210, 30, 25, (int)(255 * f), 90.0f))
			DrawBox(0.898f, 0.79f, 0.014f, 0.02f, 210, 30, 25, (int)(255 * f));
		Text("HONOR", 0.92f, 0.787f, 0.36f * pulse, 230, 60, 45, (int)(255 * f), LEFT, "title", true);
	}
	if (g_plate.on) DrawScorePlate(g_plate.a.c_str(), g_plate.b.c_str(), g_plate.c.c_str(), g_plate.d.c_str(), g_plate.meter);
	if (g_card.on) DrawChapterCard(g_card.a.c_str(), g_card.b.c_str(), g_card.alpha);
	if (g_place.on) DrawPlaceCard(g_place.a.c_str(), g_place.b.c_str(), g_place.alpha, g_place.meter > 0 ? g_place.meter : 1.0f);
	if (g_sub.on) DrawSubtitle(g_sub.a.c_str(), g_sub.b.c_str(), g_sub.alpha);
	if (g_prompt.on) DrawPromptMeter(g_prompt.a.c_str(), g_prompt.meter);
	g_plate.on = g_card.on = g_sub.on = g_place.on = g_prompt.on = false;
}

// ---------- sounds of our own ----------
typedef BOOL(WINAPI* PlaySoundAFn)(LPCSTR, HMODULE, DWORD);
typedef DWORD(WINAPI* MciSendStringAFn)(LPCSTR, LPSTR, UINT, HWND);
static PlaySoundAFn g_playSound = nullptr;
static MciSendStringAFn g_mci = nullptr;
static bool g_winmmTried = false;
static void LoadWinmm()
{
	if (g_winmmTried) return;
	g_winmmTried = true;
	HMODULE m = LoadLibraryA("winmm.dll");
	if (m)
	{
		g_playSound = (PlaySoundAFn)GetProcAddress(m, "PlaySoundA");
		g_mci = (MciSendStringAFn)GetProcAddress(m, "mciSendStringA");
	}
	Log("UI audio: winmm %s", g_playSound ? "loaded" : "NOT available");
}

static std::string g_boomPath, g_boomSoftPath, g_whooshSoftPath;
// v1.6.1: a 16-bit mono .wav, written next to the mod
static bool WriteWav(const std::string& path, const std::vector<int16_t>& pcm, int rate)
{
	FILE* fp = nullptr;
	if (fopen_s(&fp, path.c_str(), "wb") != 0 || !fp) return false;
	uint32_t n = (uint32_t)pcm.size(), dataBytes = n * 2, riff = 36 + dataBytes, fmtLen = 16, byteRate = rate * 2;
	uint16_t pcmTag = 1, ch = 1, align = 2, bits = 16;
	uint32_t r32 = rate;
	fwrite("RIFF", 1, 4, fp); fwrite(&riff, 4, 1, fp); fwrite("WAVEfmt ", 1, 8, fp); fwrite(&fmtLen, 4, 1, fp);
	fwrite(&pcmTag, 2, 1, fp); fwrite(&ch, 2, 1, fp); fwrite(&r32, 4, 1, fp); fwrite(&byteRate, 4, 1, fp); fwrite(&align, 2, 1, fp); fwrite(&bits, 2, 1, fp);
	fwrite("data", 1, 4, fp); fwrite(&dataBytes, 4, 1, fp); fwrite(pcm.data(), 2, n, fp);
	fclose(fp);
	return true;
}
// v1.6.1 (playtest 15: in the intro the title's boom was "the thunder... too loud" - 12-17 dB over the voices, nearly all of it
// under 150 Hz - and the lift-off whoosh 10 dB over): a soft copy of each, for the intro
static bool MakeSoftCopy(const std::string& src, std::string& dst, const char* name, float gain, int rate)
{
	if (!dst.empty() && GetFileAttributesA(dst.c_str()) != INVALID_FILE_ATTRIBUTES) return true;   // (review: gone mid-session - made again)
	dst.clear();
	FILE* fp = nullptr;
	if (fopen_s(&fp, src.c_str(), "rb") != 0 || !fp) return false;
	fseek(fp, 0, SEEK_END); long len = ftell(fp); fseek(fp, 44, SEEK_SET);
	std::vector<int16_t> pcm(len > 44 ? (len - 44) / 2 : 0);
	size_t got = pcm.empty() ? 0 : fread(pcm.data(), 2, pcm.size(), fp);
	fclose(fp);
	pcm.resize(got);
	if (pcm.empty()) return false;
	for (auto& v : pcm) v = (int16_t)(v * gain);
	std::string path = ModuleDir() + "\\" + name;
	if (!WriteWav(path, pcm, rate)) return false;
	dst = path;
	return true;
}
// A "dramatic boom" made from scratch at load (no sample from anywhere): a falling sine thump with a soft click and a long
// tail, written once to a .wav next to the mod and played through Windows (it mixes with the game's own sound).
static bool MakeBoom()
{
	if (!g_boomPath.empty()) return true;
	const int rate = 22050, n = (int)(rate * 1.3f);
	std::vector<int16_t> pcm(n);
	double ph = 0;
	for (int i = 0; i < n; i++)
	{
		double t = (double)i / rate;
		double f = 34.0 + 46.0 * exp(-t * 9.0);           // 80 Hz falling to 34
		ph += 2 * 3.14159265358979 * f / rate;
		double env = (1 - exp(-t * 160.0)) * exp(-t * 2.6);
		double s = sin(ph) * env + 0.35 * sin(ph * 2.01) * env * exp(-t * 6.0);
		s += (t < 0.012 ? (((i * 7919) % 200) / 100.0 - 1.0) * (1 - t / 0.012) * 0.25 : 0.0);   // the click
		s = tanh(s * 1.8) * 0.92;                            // a little warmth
		pcm[i] = (int16_t)(s * 32000);
	}
	g_boomPath = ModuleDir() + "\\TornadoRedemption_boom.wav";
	if (!WriteWav(g_boomPath, pcm, rate)) { g_boomPath.clear(); return false; }
	return true;
}

// v1.2: the jet burners lighting (the previs had it, the game didn't): a hiss of air rising into a roar, then a low burn.
static std::string g_whooshPath;
static bool MakeWhoosh()
{
	if (!g_whooshPath.empty()) return true;
	const int rate = 22050, n = (int)(rate * 1.5f);
	std::vector<int16_t> pcm(n);
	uint32_t seed = 12345;
	double lp = 0, lp2 = 0, bp = 0;
	for (int i = 0; i < n; i++)
	{
		double t = (double)i / rate;
		seed = seed * 1664525u + 1013904223u;
		double w = ((seed >> 9) & 0xFFFF) / 32768.0 - 1.0;
		double cut = 0.02 + 0.22 * (1 - exp(-t * 7.0)) * exp(-t * 1.4);   // the band opens up, then settles
		lp += (w - lp) * cut;
		lp2 += (lp - lp2) * 0.04;
		bp = lp - lp2;                                                      // a moving band of noise: the whoosh
		double roar = lp2 * 3.2;                                            // and the burn underneath it
		double env = (1 - exp(-t * 30.0)) * (0.55 * exp(-t * 2.2) + 0.45 * exp(-t * 0.9));
		double sm = (bp * 1.6 + roar) * env;
		sm = tanh(sm * 1.5) * 0.85;
		pcm[i] = (int16_t)(sm * 30000);
	}
	g_whooshPath = ModuleDir() + "\\TornadoRedemption_whoosh.wav";
	if (!WriteWav(g_whooshPath, pcm, rate)) { g_whooshPath.clear(); return false; }
	return true;
}

void PlayWhoosh(bool soft)
{
	if (g_mute) return;
	LoadWinmm();
	if (!g_mci || !MakeWhoosh()) return;
	const std::string& path = soft && MakeSoftCopy(g_whooshPath, g_whooshSoftPath, "TornadoRedemption_whoosh_soft.wav", 0.4f, 22050) ? g_whooshSoftPath : g_whooshPath;
	g_mci("close nadowhoosh", nullptr, 0, nullptr);
	std::string open = "open \"" + path + "\" type waveaudio alias nadowhoosh";
	DWORD e = g_mci(open.c_str(), nullptr, 0, nullptr);
	if (!e) e = g_mci("play nadowhoosh from 0", nullptr, 0, nullptr);
	Log("UI whoosh%s %s", soft ? " (soft)" : "", e ? "failed (MCI)" : "played");
}

void PlayBoom(bool soft)
{
	if (g_mute) return;
	LoadWinmm();
	if (!g_mci || !MakeBoom()) return;
	const std::string& path = soft && MakeSoftCopy(g_boomPath, g_boomSoftPath, "TornadoRedemption_boom_soft.wav", 0.2f, 22050) ? g_boomSoftPath : g_boomPath;
	// MCI, so it can play over a voice line that PlaySound is playing
	g_mci("close nadoboom", nullptr, 0, nullptr);
	std::string open = "open \"" + path + "\" type waveaudio alias nadoboom";
	DWORD e = g_mci(open.c_str(), nullptr, 0, nullptr);
	if (!e) e = g_mci("play nadoboom from 0", nullptr, 0, nullptr);
	Log("UI boom%s %s", soft ? " (soft)" : "", e ? "failed (MCI)" : "played");
}

void PlayVoiceFile(const char* id)
{
	if (g_mute || !id) return;
	std::string path = ModuleDir() + "\\TornadoRedemption_intro\\" + id + ".wav";
	if (GetFileAttributesA(path.c_str()) == INVALID_FILE_ATTRIBUTES) return;
	LoadWinmm();
	if (g_playSound && g_playSound(path.c_str(), nullptr, SND_FILENAME | SND_ASYNC | SND_NODEFAULT))
		Log("UI voice line %s played", id);
}
}
