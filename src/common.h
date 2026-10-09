// Tornado Redemption - shared helpers: math, logging, timing, model/ptfx loading.
#pragma once

#include <windows.h>
#include <cmath>
#include <string>
#include <vector>
#include <unordered_set>
#include <algorithm>
#include "nat.h"
#include <main.h>              // Script Hook RDR2 SDK (build.bat adds its inc folder)

// ---------- math ----------
struct V3
{
	float x = 0, y = 0, z = 0;
	V3() {}
	V3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}
	V3(const Vector3& v) : x(v.x), y(v.y), z(v.z) {}
	V3 operator+(const V3& o) const { return { x + o.x, y + o.y, z + o.z }; }
	V3 operator-(const V3& o) const { return { x - o.x, y - o.y, z - o.z }; }
	V3 operator*(float s) const { return { x * s, y * s, z * s }; }
	V3& operator+=(const V3& o) { x += o.x; y += o.y; z += o.z; return *this; }
	float len() const { return sqrtf(x * x + y * y + z * z); }
	float len2d() const { return sqrtf(x * x + y * y); }
};

const float PI = 3.14159265f;
inline float Clamp(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }
inline float Lerp(float a, float b, float t) { return a + (b - a) * t; }
inline float Rand01() { return (float)rand() / (float)RAND_MAX; }
inline float RandRange(float a, float b) { return a + (b - a) * Rand01(); }
inline V3 HeadingDir(float headingDeg)
{
	// RDR2 heading: 0 = north (+Y), increases counter-clockwise.
	float r = headingDeg * PI / 180.0f;
	return { -sinf(r), cosf(r), 0.0f };
}

// ---------- time ----------
inline float NowSec() { return MISC::GET_GAME_TIMER() / 1000.0f; }

// ---------- logging (TornadoRedemption.log next to the .asi) ----------
void LogInit(HMODULE module);
void Log(const char* fmt, ...);
void Finding(const char* fmt, ...);   // appends to TornadoRedemption_findings.txt (things the user rated / bookmarked)
std::string ModuleDir();

// ---------- on-screen text ----------
void DrawTextLine(const char* text, float x, float y, float scale, int r = 255, int g = 255, int b = 255, int a = 255);
void DrawBox(float left, float top, float w, float h, int r, int g, int b, int a);
void Notify(const std::string& text, int ms = 3000);   // short status line at the bottom of the screen
// v1.2: the game's own memory (the whole RDR2 process, not just the mod), for the MEM log line - the user saw 12 GB of RAM and
// 31 GB of GPU memory before even spawning a tornado, so every session now logs both from the start
struct MemInfo { float ramGB = 0, privateGB = 0, vramGB = 0, vramBudgetGB = 0, sharedGB = 0; bool vramKnown = false; };
bool MemStats(MemInfo* out);
void DrawNotify();

// ---------- streaming ----------
bool LoadModel(Hash model, int timeoutMs = 3000);
bool LoadPtfxAsset(const char* dict, int timeoutMs = 3000);
Hash H(const char* s);
// joaat, the game's string hash, computed locally (no native) - control actions are hashed names.
inline Hash Joaat(const char* s)
{
	Hash h = 0;
	for (; *s; ++s)
	{
		h += (Hash)((*s >= 'A' && *s <= 'Z') ? *s + 32 : *s);
		h += h << 10;
		h ^= h >> 6;
	}
	h += h << 3;
	h ^= h >> 11;
	h += h << 15;
	return h;
}

// Spawns an invisible, collision-less, frozen prop used as a moving particle anchor.
Object SpawnAnchor(const V3& pos);        // registers the anchor (see IsAnchor)
bool DeleteObj(Object& obj);              // clears the handle; returns true only if the game confirms it is gone
void DeleteAnchor(Object& obj);           // DeleteObj + unregister, or keep as an orphan to retry
void RetryOrphans();                      // call about once a second from the script thread
bool IsAnchor(Entity e);
int AnchorsAlive();                       // registered anchors that still exist (includes orphans) - leak check
int OrphanCount();
// After a game reload Script Hook restarts the script but our statics survive: the old handles may now belong to
// other entities, so forget them WITHOUT calling any natives on them.
void ForgetAllAnchors();
extern int g_anchorHideMode;              // 0 visible (debug), 1 SET_ENTITY_VISIBLE false, 2 alpha 0
extern int g_anchorLod;                   // SET_ENTITY_LOD_DIST for new anchors (0 = game default) - render experiment
extern int g_anchorLodDefault;            // the game's own LOD distance for the anchor prop, logged once

// Ground height under a point (searches from above). Returns fallback if not found.
float GroundZ(float x, float y, float searchFromZ, float fallback);
