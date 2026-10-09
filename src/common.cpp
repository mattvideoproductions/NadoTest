// Tornado Redemption - shared helpers implementation.
#include "common.h"
#include "ui.h"
#include <cstdio>
#include <cstdarg>
#include <ctime>
#include <psapi.h>
#include <dxgi1_4.h>

static std::string g_dir;
static std::string g_notifyText;
static DWORD g_notifyUntil = 0, g_notifyAt = 0;

std::string ModuleDir() { return g_dir; }

void LogInit(HMODULE module)
{
	char path[MAX_PATH] = {};
	GetModuleFileNameA(module, path, MAX_PATH);
	std::string p = path;
	size_t slash = p.find_last_of("\\/");
	g_dir = (slash == std::string::npos) ? "." : p.substr(0, slash);
	FILE* f = nullptr;
	// Append so earlier game sessions survive a crash/relaunch.
	if (fopen_s(&f, (g_dir + "\\TornadoRedemption.log").c_str(), "a") == 0 && f)
	{
		time_t now = time(nullptr);
		char ts[64];
		ctime_s(ts, sizeof(ts), &now);
		fprintf(f, "\n===== Tornado Redemption session started %s", ts);
		fclose(f);
	}
}

static void AppendLine(const char* file, const char* prefix, const char* fmt, va_list args)
{
	char buf[1024];
	vsnprintf_s(buf, sizeof(buf), _TRUNCATE, fmt, args);
	FILE* f = nullptr;
	if (fopen_s(&f, (g_dir + "\\" + file).c_str(), "a") == 0 && f)
	{
		time_t now = time(nullptr);
		tm t;
		localtime_s(&t, &now);
		fprintf(f, "[%02d:%02d:%02d] %s%s\n", t.tm_hour, t.tm_min, t.tm_sec, prefix, buf);
		fclose(f);
	}
}

void Log(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	AppendLine("TornadoRedemption.log", "", fmt, args);
	va_end(args);
}

void Finding(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	AppendLine("TornadoRedemption_findings.txt", "", fmt, args);
	va_end(args);
	va_start(args, fmt);
	AppendLine("TornadoRedemption.log", "FINDING: ", fmt, args);
	va_end(args);
}

void DrawTextLine(const char* text, float x, float y, float scale, int r, int g, int b, int a)
{
	UIDEBUG::BG_SET_TEXT_SCALE(scale, scale);
	UIDEBUG::BG_SET_TEXT_COLOR(r, g, b, a);
	UIDEBUG::BG_DISPLAY_TEXT(MISC::VAR_STRING_LITERAL(text), x, y);
}

void DrawBox(float left, float top, float w, float h, int r, int g, int b, int a)
{
	GRAPHICS::DRAW_RECT(left + w * 0.5f, top + h * 0.5f, w, h, r, g, b, a, FALSE, FALSE);
}

// RAM: the process's working set and private bytes (what Task Manager calls "Memory"). VRAM: DXGI's per-process usage on the
// biggest graphics card - the video memory manager counts every allocation the process makes there, Vulkan included - and the
// budget Windows gives the game on it. "Shared" is what spilled over into system RAM.
bool MemStats(MemInfo* out)
{
	if (!out) return false;
	PROCESS_MEMORY_COUNTERS_EX pmc = {};
	pmc.cb = sizeof(pmc);
	if (K32GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc)))
	{
		out->ramGB = (float)(pmc.WorkingSetSize / 1e9);
		out->privateGB = (float)(pmc.PrivateUsage / 1e9);
	}
	static IDXGIAdapter3* s_adapter = nullptr;
	static bool s_tried = false;
	if (!s_tried)
	{
		s_tried = true;
		typedef HRESULT(WINAPI* CreateFn)(REFIID, void**);
		HMODULE m = LoadLibraryA("dxgi.dll");
		CreateFn create = m ? (CreateFn)GetProcAddress(m, "CreateDXGIFactory1") : nullptr;
		IDXGIFactory1* f = nullptr;
		if (create && SUCCEEDED(create(__uuidof(IDXGIFactory1), (void**)&f)) && f)
		{
			SIZE_T best = 0;
			IDXGIAdapter1* a = nullptr;
			for (UINT i = 0; f->EnumAdapters1(i, &a) != DXGI_ERROR_NOT_FOUND; i++)
			{
				DXGI_ADAPTER_DESC1 d = {};
				a->GetDesc1(&d);
				IDXGIAdapter3* a3 = nullptr;
				if (!(d.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) && d.DedicatedVideoMemory > best && SUCCEEDED(a->QueryInterface(__uuidof(IDXGIAdapter3), (void**)&a3)) && a3)
				{
					if (s_adapter) s_adapter->Release();
					s_adapter = a3;
					best = d.DedicatedVideoMemory;
				}
				a->Release();
			}
			f->Release();
		}
	}
	if (s_adapter)
	{
		DXGI_QUERY_VIDEO_MEMORY_INFO li = {}, nl = {};
		if (SUCCEEDED(s_adapter->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &li)))
		{
			out->vramGB = (float)(li.CurrentUsage / 1e9);
			out->vramBudgetGB = (float)(li.Budget / 1e9);
			out->vramKnown = true;
		}
		if (SUCCEEDED(s_adapter->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_NON_LOCAL, &nl)))
			out->sharedGB = (float)(nl.CurrentUsage / 1e9);
	}
	return true;
}

void Notify(const std::string& text, int ms)
{
	if (text != g_notifyText || GetTickCount() > g_notifyUntil) g_notifyAt = GetTickCount();
	g_notifyText = text;
	g_notifyUntil = GetTickCount() + ms;
	Log("notify: %s", text.c_str());
}

void DrawNotify()
{
	DWORD now = GetTickCount();
	if (now > g_notifyUntil || g_notifyText.empty())
		return;
	if (UI::g_simple)
	{
		DrawBox(0.30f, 0.86f, 0.40f, 0.05f, 0, 0, 0, 170);
		DrawTextLine(g_notifyText.c_str(), 0.31f, 0.868f, 0.32f, 255, 220, 140);
		return;
	}
	// v1.1: the game's soft help-text plate and its body font; it slides up and fades in, and fades out
	float in = Clamp((now - g_notifyAt) / 220.0f, 0.0f, 1.0f), out = Clamp((g_notifyUntil - now) / 400.0f, 0.0f, 1.0f);
	float a = std::min(in, out), slide = (1.0f - in) * 0.02f;
	float w = Clamp(UI::TextWidth(g_notifyText.c_str(), 0.33f) + 0.08f, 0.22f, 0.7f);
	if (!UI::Sprite("feeds", "help_text_bg", 0.5f, 0.885f + slide, w, 0.068f, 0, 0, 0, (int)(215 * a)))
		DrawBox(0.5f - w / 2, 0.86f + slide, w, 0.05f, 0, 0, 0, (int)(170 * a));
	UI::Text(g_notifyText.c_str(), 0.5f, 0.871f + slide, 0.33f, 240, 228, 200, (int)(255 * a), UI::CENTRE, "body");
}

Hash H(const char* s) { return MISC::GET_HASH_KEY(s); }

bool LoadModel(Hash model, int timeoutMs)
{
	if (!STREAMING::IS_MODEL_IN_CDIMAGE(model) || !STREAMING::IS_MODEL_VALID(model))
		return false;
	STREAMING::REQUEST_MODEL(model, FALSE);
	DWORD until = GetTickCount() + timeoutMs;
	while (!STREAMING::HAS_MODEL_LOADED(model))
	{
		if (GetTickCount() > until)
			return false;
		WAIT(0);
	}
	return true;
}

bool LoadPtfxAsset(const char* dict, int timeoutMs)
{
	Hash h = H(dict);
	if (STREAMING::HAS_NAMED_PTFX_ASSET_LOADED(h))
		return true;
	STREAMING::REQUEST_NAMED_PTFX_ASSET(h);
	DWORD until = GetTickCount() + timeoutMs;
	while (!STREAMING::HAS_NAMED_PTFX_ASSET_LOADED(h))
	{
		if (GetTickCount() > until)
			return false;
		WAIT(0);
	}
	return true;
}

int g_anchorHideMode = 1; // 0 = leave visible (debug), 1 = SET_ENTITY_VISIBLE false, 2 = alpha 0
// v0.4: the anchor is a 5 cm apple, so the game may stop drawing it (and maybe its attached smoke) only a few dozen
// metres away. Unproven - the Render check compares boosted and default LOD distances.
int g_anchorLod = 1500;
int g_anchorLodDefault = -1;

// Anchor ownership (Astra audit, v0.5.1): every anchor from every producer (tornadoes AND the FX Lab) is registered
// here when created, and only unregistered once the game confirms it is gone. A delete that doesn't take becomes an
// "orphan": still registered (so physics keeps ignoring it and leak checks still count it) and retried every second.
static std::unordered_set<Entity> g_anchors;
static std::vector<Object> g_orphans;

bool IsAnchor(Entity e) { return g_anchors.count(e) != 0; }

int AnchorsAlive()
{
	int n = 0;
	for (Entity e : g_anchors)
		if (ENTITY::DOES_ENTITY_EXIST(e)) n++;
	return n;
}

int OrphanCount() { return (int)g_orphans.size(); }

Object SpawnAnchor(const V3& pos)
{
	static Hash model = 0;
	if (!model)
		model = H("p_apple01x");
	if (!LoadModel(model))
		return 0;
	Object o = OBJECT::CREATE_OBJECT(model, pos.x, pos.y, pos.z, FALSE, FALSE, FALSE, FALSE, FALSE);
	if (!o)
		return 0;
	g_anchors.insert(o);
	if (g_anchorLodDefault < 0)
	{
		g_anchorLodDefault = ENTITY::GET_ENTITY_LOD_DIST(o);
		Log("anchor prop p_apple01x: game LOD distance %d m (the mod sets %d)", g_anchorLodDefault, g_anchorLod);
	}
	if (g_anchorLod > 0)
		ENTITY::SET_ENTITY_LOD_DIST(o, g_anchorLod);
	ENTITY::SET_ENTITY_COLLISION(o, FALSE, FALSE);
	ENTITY::FREEZE_ENTITY_POSITION(o, TRUE);
	ENTITY::SET_ENTITY_INVINCIBLE(o, TRUE);
	if (g_anchorHideMode == 1)
		ENTITY::SET_ENTITY_VISIBLE(o, FALSE);
	else if (g_anchorHideMode == 2)
		ENTITY::SET_ENTITY_ALPHA(o, 0, FALSE);
	return o;
}

bool DeleteObj(Object& obj)
{
	Object h = obj;
	obj = 0;
	if (!h || !ENTITY::DOES_ENTITY_EXIST(h))
		return true;
	ENTITY::SET_ENTITY_AS_MISSION_ENTITY(h, TRUE, TRUE);
	Object tmp = h;
	OBJECT::DELETE_OBJECT(&tmp);
	return !ENTITY::DOES_ENTITY_EXIST(h);
}

void DeleteAnchor(Object& obj)
{
	Object h = obj;
	if (!h) return;
	if (DeleteObj(obj))
		g_anchors.erase(h);
	else
	{
		g_orphans.push_back(h);   // still registered; retried by RetryOrphans()
		Log("anchor %d did not delete - queued for retry", h);
	}
}

void ForgetAllAnchors()
{
	g_anchors.clear();
	g_orphans.clear();
}

void RetryOrphans()
{
	for (size_t i = 0; i < g_orphans.size();)
	{
		Object h = g_orphans[i];
		Object tmp = h;
		if (DeleteObj(tmp))
		{
			g_anchors.erase(h);
			g_orphans.erase(g_orphans.begin() + i);
			Log("orphan anchor %d finally deleted", h);
		}
		else i++;
	}
	// Anchors the game removed by itself are gone too - drop them from the registry.
	for (auto it = g_anchors.begin(); it != g_anchors.end();)
	{
		bool orphan = std::find(g_orphans.begin(), g_orphans.end(), *it) != g_orphans.end();
		if (!orphan && !ENTITY::DOES_ENTITY_EXIST(*it)) it = g_anchors.erase(it);
		else ++it;
	}
}

float GroundZ(float x, float y, float searchFromZ, float fallback)
{
	float z = 0;
	if (MISC::GET_GROUND_Z_FOR_3D_COORD(x, y, searchFromZ, &z, FALSE))
		return z;
	return fallback;
}
