// Tornado Redemption - menu, controller, world controls, test tools (auto tests, render check, FX Lab, tree tests), storm
// control, drone camera and the main loop. v1.1 adds (in the .inl parts included below): the intro cutscene, the jet balloon
// and Storm chaser, Storm season and the tornado gun.
#include "common.h"
#include "input.h"
#include "pad.h"
#include "tornado.h"
#include "trees.h"
#include "treescan.h"
#include "ui.h"
#include "roar.h"
#include <memory>
#include <functional>
#include <cstdio>
#include <cstdarg>
#include <algorithm>

static const char* kVersion = "Tornado Redemption v1.7.3";
static bool g_menuOpen = false;          // (declared up here: the intro and the balloon close the menu)
static float g_frameDt = 0.016f;
static float UIdt() { return g_frameDt; }
// v1.1 switches shared by the storm, the menu and the new modes
static bool g_manualWeather = false;   // the user locked a weather (World menu): a tornado never replaces it (playtest 10)
static bool g_darkSky = true;          // [Storm] DarkSky: a darker grade while a storm is out (playtest 10: "Storm clouds" never looked dark)
static int g_roarLevel = 1;            // the tornado's roar: 0 off, 1 quiet (v1.4 default), 2 normal, 3 loud
static int g_musicMode = 0;            // mission music: 0 off, 1 on
static int g_uiStyle = 0;              // the menu's look: 0 RDR2, 1 Simple (UI::g_simple follows it)
// v1.2 (voices.inl): the cast's reactions
static int g_voices = 2;               // [Sound] Voices: 0 off, 1 Arthur only, 2 everyone
static int g_chatter = 1;              // [Sound] Chatter: 0 rare, 1 normal, 2 chatty
static bool g_voiceSubs = true;        // [Sound] VoiceSubtitles: ask the game for its own subtitle (the _SUB speech params)
static bool g_stormReport = true;      // [Sound] StormReport: the card when a storm is over
bool IntroRunning();                   // intro.inl
bool IntroActive();
bool IntroOwnsGrade();
bool IntroShieldsPlayer();             // v1.7: the scene, and the first 10 s of the run for the balloon
bool IntroHoldsSky();                  // v1.7: the scene's cloudy sky holds until you're in the balloon
bool BalloonActive();                  // balloon.inl
bool GalleryActive();                  // gallery.inl
static void BalloonRemove(const char* why);
static void LoadBests();               // (after the modes, below)

// ======================= config =======================
struct Keys { Hotkey menu, spawn, despawn, up, down, left, right, select, back, bookmark, cinematic, drone, next; } g_keys;
struct PadKeys { PadCombo menu, spawn, despawn, cinematic, drone, note, next; } g_pad;
static bool g_padOn = true;

static bool FileThere(const std::string& p) { return GetFileAttributesA(p.c_str()) != INVALID_FILE_ATTRIBUTES; }

// v1.7.0: the mod was called NadoTest until the final version. An install that still has only the old NadoTest.ini keeps its
// settings; the best scores and the tree scan are copied over once (the old files stay, untouched).
static std::string IniPath()
{
	std::string now = ModuleDir() + "\\TornadoRedemption.ini", old = ModuleDir() + "\\NadoTest.ini";
	return !FileThere(now) && FileThere(old) ? old : now;
}

static void CarryOverOldFiles()
{
	for (const char* part : { "_best.txt", "_trees.txt" })
	{
		std::string now = ModuleDir() + "\\TornadoRedemption" + part, old = ModuleDir() + "\\NadoTest" + part;
		if (!FileThere(now) && FileThere(old) && CopyFileA(old.c_str(), now.c_str(), TRUE))
			Log("carried NadoTest%s over to TornadoRedemption%s", part, part);
	}
}

static std::string IniStr(const char* sec, const char* key, const char* def)
{
	char buf[256];
	GetPrivateProfileStringA(sec, key, def, buf, sizeof(buf), IniPath().c_str());
	return buf;
}
static float IniFloat(const char* sec, const char* key, float def)
{
	char d[32];
	sprintf_s(d, "%g", def);
	return (float)atof(IniStr(sec, key, d).c_str());
}

static std::string IniUpper(const char* sec, const char* key, const char* def)
{
	std::string v = IniStr(sec, key, def);
	std::string out;
	for (char ch : v) if (ch != ' ' && ch != '_' && ch != '-') out += (char)toupper((unsigned char)ch);
	return out;
}
static bool g_defaultsLoaded = false;
static void IniPick(const char* key, std::vector<std::string> names, int& value)
{
	std::string v = IniUpper("Defaults", key, "");
	for (size_t i = 0; i < names.size(); i++)
		if (v == names[i]) { value = (int)i; return; }
}

static void LoadConfig()
{
	g_keys.menu = ParseHotkey(IniStr("Keys", "Menu", "BACKSLASH"));
	g_keys.spawn = ParseHotkey(IniStr("Keys", "QuickSpawn", "RBRACKET"));
	g_keys.despawn = ParseHotkey(IniStr("Keys", "DespawnAll", "LBRACKET"));
	g_keys.up = ParseHotkey(IniStr("Keys", "MenuUp", "UP"));
	g_keys.down = ParseHotkey(IniStr("Keys", "MenuDown", "DOWN"));
	g_keys.left = ParseHotkey(IniStr("Keys", "MenuLeft", "LEFT"));
	g_keys.right = ParseHotkey(IniStr("Keys", "MenuRight", "RIGHT"));
	g_keys.select = ParseHotkey(IniStr("Keys", "MenuSelect", "ENTER"));
	g_keys.back = ParseHotkey(IniStr("Keys", "MenuBack", "BACKSPACE"));
	g_keys.bookmark = ParseHotkey(IniStr("Keys", "Bookmark", "SEMICOLON"));
	g_keys.cinematic = ParseHotkey(IniStr("Keys", "Cinematic", "QUOTE"));
	g_keys.drone = ParseHotkey(IniStr("Keys", "DroneCam", "PERIOD"));
	g_keys.next = ParseHotkey(IniStr("Keys", "NextStep", "N"));

	g_padOn = IniFloat("Controller", "Enabled", 1) != 0;
	g_pad.menu = ParsePadCombo(IniStr("Controller", "Menu", "RB+DPAD_RIGHT"));
	g_pad.spawn = ParsePadCombo(IniStr("Controller", "QuickSpawn", "RB+DPAD_UP"));
	g_pad.despawn = ParsePadCombo(IniStr("Controller", "DespawnAll", "RB+DPAD_DOWN"));
	g_pad.cinematic = ParsePadCombo(IniStr("Controller", "Cinematic", "RB+DPAD_LEFT"));
	g_pad.drone = ParsePadCombo(IniStr("Controller", "DroneCam", "RB+Y"));
	g_pad.note = ParsePadCombo(IniStr("Controller", "Note", "RB+X"));
	g_pad.next = ParsePadCombo(IniStr("Controller", "NextStep", "RB+A"));
	{
		std::string hud = IniStr("HUD", "Mode", "Tracker");
		for (auto& ch : hud) ch = (char)toupper((unsigned char)ch);
		g_set.hud = hud == "FULL" ? 0 : hud == "OFF" ? 2 : 1;
	}

	g_set.maxEntities = (int)IniFloat("Physics", "MaxEntitiesPerTornado", 150);
	g_set.velocityGain = IniFloat("Physics", "VelocityGain", 2.5f);
	g_set.forceGain = IniFloat("Physics", "ForceGain", 1.0f);
	g_set.windSpeed = IniFloat("Storm", "WindSpeed", 12.0f);
	g_set.shakeName = IniStr("Storm", "CameraShakeName", "HAND_SHAKE");
	g_set.shakeMax = IniFloat("Storm", "CameraShakeMax", 2.5f);
	g_set.ptfxBudget = (int)IniFloat("Visuals", "LoopedEffectBudget", 110);
	g_set.puffsPerSecond = IniFloat("Visuals", "PuffsPerSecond", 70.0f);
	g_set.overlayX = IniFloat("Overlay", "X", 0.33f);
	g_set.streamerPitch = IniFloat("Visuals", "StreamerPitch", -60.0f);
	{
		std::string perf = IniUpper("Performance", "Mode", "BALANCED"), wx = IniUpper("Storm", "Weather", "STORMCLOUDS");
		g_set.perf = perf.rfind("LOW", 0) == 0 ? 2 : perf == "HIGH" ? 0 : 1;
		g_set.weatherMode = (wx == "OFF" || wx == "KEEP") ? 0 : wx == "THUNDERSTORM" ? 2 : wx == "RAIN" ? 3 : 1;
		g_set.roar = IniFloat("Storm", "WindRoar", 1) != 0;
		g_darkSky = IniFloat("Storm", "DarkSky", 1) != 0;
	}
	// v1.1
	{
		std::string ui = IniUpper("UI", "Style", "RDR2");
		g_uiStyle = ui == "SIMPLE" ? 1 : 0;
		UI::g_simple = g_uiStyle == 1;
		std::string roar = IniUpper("Sound", "TornadoRoar", "QUIET");
		g_roarLevel = roar == "OFF" ? 0 : roar == "NORMAL" ? 2 : roar == "LOUD" ? 3 : 1;
		{
			std::string mm = IniUpper("Sound", "MissionMusic", "OFF");
			static const char* kMusicNames[] = { "OFF", "HIDEOUTFIGHT", "BRAITHWAITEBATTLE", "OUTLAWAMBUSH", "HOSTAGERESCUE", "HORSECHASE", "NATIVESONSTORM" };
			g_musicMode = 0;
			for (int i = 0; i < 7; i++) if (mm == kMusicNames[i]) g_musicMode = i;
			if (mm == "1" || mm == "ON") g_musicMode = 1;
		}
		Roar::g_master = Clamp(IniFloat("Sound", "RoarVolume", 0.8f), 0.0f, 1.5f);
		g_set.memeSounds = IniFloat("Intro", "MemeSounds", 1) != 0;
	}
	// v1.2
	{
		std::string v = IniUpper("Sound", "Voices", "EVERYONE");
		g_voices = v == "OFF" || v == "0" ? 0 : v == "ARTHUR" || v == "ARTHURONLY" || v == "1" ? 1 : 2;
		std::string c = IniUpper("Sound", "Chatter", "NORMAL");
		g_chatter = c == "RARE" ? 0 : c == "CHATTY" ? 2 : 1;
		g_voiceSubs = IniFloat("Sound", "VoiceSubtitles", 1) != 0;
		g_stormReport = IniFloat("Sound", "StormReport", 1) != 0;
	}
	g_set.overlayY = IniFloat("Overlay", "Y", 0.015f);
	// v1.0, playtest 8 ("default settings file, optimized defaults"): the [Defaults] section sets what a session starts with.
	if (!g_defaultsLoaded)
	{
		g_defaultsLoaded = true;
		{
			// (audit 3) "Firenado", "Dust Devil", "Ghost Twister", "Multi-vortex", "Junknado" ... are read by their first word
			static const char* kStyleKeys[] = { "VORTEX", "WEDGE", "COLUMN", "ROPE", "TOON", "DUST", "FIRE", "GHOST", "SNOW", "WATER", "MULTI", "JUNK" };
			std::string v = IniUpper("Defaults", "Style", "");
			std::string flat;
			for (char ch : v) if (ch != ' ' && ch != '-' && ch != '_') flat += ch;
			if (flat.rfind("DARK", 0) == 0) flat = flat.substr(4);   // "Dark Vortex", "Dark Column"
			for (int i = 0; i < 12; i++) if (!flat.empty() && flat.rfind(kStyleKeys[i], 0) == 0) { g_set.style = i; break; }
		}
		IniPick("Strength", { "GENTLE", "VIOLENT", "EXTREME" }, g_set.force);
		IniPick("FunnelSize", { "SMALL", "MEDIUM", "LARGE" }, g_set.size);
		IniPick("Speed", { "SLOW", "NORMAL", "FAST" }, g_set.speed);
		IniPick("Movement", { "STATIONARY", "WANDER", "TOWARDYOU", "STRAIGHTLINE" }, g_set.movement);
		IniPick("Reach", { "MEDIUM", "LARGE", "HUGE", "MASSIVE" }, g_set.reach);
		IniPick("Arthur", { "IMMUNE", "TUGGED", "GRABBABLE", "EASYPREY" }, g_set.arthur);
		g_set.flingChase = IniFloat("Defaults", "FlingAndChase", 1) != 0;
		g_set.playerGod = IniFloat("Defaults", "ArthurInvincible", 0) != 0;
		IniPick("Landings", { "SOFT", "MIXED", "REAL" }, g_set.landings);
		g_set.throwAtArthur = IniFloat("Defaults", "ThrowAtArthur", 1) != 0;
		g_set.citySafe = IniFloat("Defaults", "TownSafety", 1) != 0;   // v1.5
		g_set.touchdownCam = IniFloat("Defaults", "TouchdownCamera", 1) != 0;
		g_set.rideCam = IniFloat("Defaults", "RideCamera", 1) != 0;
		g_set.mapTrees = IniFloat("Defaults", "RealTrees", 1) != 0;
		g_set.flatten = IniFloat("Defaults", "FlattenGrass", 1) != 0;
		IniPick("StormSeason", { "OFF", "RARE", "REGULAR", "FREQUENT" }, g_set.season);
		Log("defaults: style %d strength %d size %d speed %d movement %d reach %d arthur %d fling %d invincible %d", g_set.style, g_set.force,
			g_set.size, g_set.speed, g_set.movement, g_set.reach, g_set.arthur, (int)g_set.flingChase, (int)g_set.playerGod);
	}
	g_set.autoLogSeconds = IniFloat("Debug", "AutoLogSeconds", 5.0f);
	g_anchorHideMode = (int)IniFloat("Debug", "AnchorHideMode", 1);
	g_anchorLod = (int)IniFloat("Debug", "AnchorLodDist", 1500);
	LoadBests();   // v1.1 audit 2: the menu's help for Storm chaser / Survive the storm shows your best before the first run

	Log("config: hud=%d next=%s/%s perf=%d weather=%d roar=%d streamerPitch=%.0f", g_set.hud, g_keys.next.text.c_str(), g_pad.next.text.c_str(),
		g_set.perf, g_set.weatherMode, (int)g_set.roar, g_set.streamerPitch);
	Log("config: menu=%s spawn=%s despawn=%s note=%s drone=%s | pad %s menu=%s spawn=%s despawn=%s cinematic=%s drone=%s note=%s | maxEnt=%d velGain=%.2f forceGain=%.2f wind=%.1f shake=%s budget=%d puffs/s=%.0f autolog=%.0fs anchorHide=%d anchorLod=%d",
		g_keys.menu.text.c_str(), g_keys.spawn.text.c_str(), g_keys.despawn.text.c_str(), g_keys.bookmark.text.c_str(), g_keys.drone.text.c_str(),
		g_padOn ? "on" : "OFF", g_pad.menu.text.c_str(), g_pad.spawn.text.c_str(), g_pad.despawn.text.c_str(), g_pad.cinematic.text.c_str(),
		g_pad.drone.text.c_str(), g_pad.note.text.c_str(),
		g_set.maxEntities, g_set.velocityGain, g_set.forceGain, g_set.windSpeed, g_set.shakeName.c_str(),
		g_set.ptfxBudget, g_set.puffsPerSecond, g_set.autoLogSeconds, g_anchorHideMode, g_anchorLod);
}

// ======================= helpers =======================
static V3 PlayerPos() { return ENTITY::GET_ENTITY_COORDS(PLAYER::PLAYER_PED_ID(), TRUE, FALSE); }
static float PlayerHeading() { return ENTITY::GET_ENTITY_HEADING(PLAYER::PLAYER_PED_ID()); }
static float CameraHeading() { return V3(CAMERA::GET_GAMEPLAY_CAM_ROT(2)).z; }
static V3 PointAt(float heading, float dist, float side)
{
	V3 f = HeadingDir(heading), r = HeadingDir(heading - 90.0f);
	V3 p = PlayerPos() + f * dist + r * side;
	p.z = GroundZ(p.x, p.y, p.z + 60.0f, p.z);
	return p;
}
static V3 PointAhead(float dist, float side = 0) { return PointAt(PlayerHeading(), dist, side); }
// Playtest 4 ("Supercell spawned somewhere... it's behind us, we didn't see it"): user spawns go where the CAMERA
// looks - on horseback Arthur and the camera often face different ways.
static V3 PointAheadCam(float dist, float side = 0) { return PointAt(CameraHeading(), dist, side); }
static bool PlayerDead()
{
	Ped me = PLAYER::PLAYER_PED_ID();
	return PLAYER::IS_PLAYER_DEAD(PLAYER::PLAYER_ID()) || PED::IS_PED_DEAD_OR_DYING(me, TRUE);
}

// ======================= tornado list =======================
static std::vector<std::unique_ptr<Tornado>> g_tornadoes;
// v1.1: a remembered tornado. A deleted tornado's address is often reused by the next one, so a stored pointer alone could
// point at a different tornado (e.g. Storm season ending YOUR tornado when its own time was up). The id settles it.
struct TornadoRef
{
	Tornado* p = nullptr;
	int uid = 0;
	TornadoRef() {}
	TornadoRef(Tornado* t) : p(t), uid(t ? t->uid : 0) {}
	Tornado* get() const
	{
		if (!p) return nullptr;
		for (auto& x : g_tornadoes) if (x.get() == p) return p->uid == uid ? p : nullptr;
		return nullptr;
	}
};
static const int kMaxTornadoes = 6;

static int FreeLoops() { return std::max(0, g_set.ptfxBudget - LoopsInUse()); }

// userSpawn (menu + quick keys): touch down where the camera looks and just OUTSIDE the tornado's reach, so you watch
// it come instead of being inside the pull the moment it lands (playtest 4: "It spawned a little close... we're
// already in the tornado"). Tests keep their fixed distances.
static int BigTornadoes()
{
	int n = 0;
	for (auto& tp : g_tornadoes) if (!tp->Mini()) n++;
	return n;
}

// v1.1: a tornado at an exact spot (the intro, Storm season, the tornado gun), sharing the looped-effect budget like any
// other spawn. A mini twister takes a small slice, from the big one if the budget's full.
static Tornado* SpawnTornadoAt(const V3& p, float heading, int style, const std::string& label, const SpawnOpts& opts = SpawnOpts(), bool stationary = false)
{
	if (!opts.mini && BigTornadoes() >= kMaxTornadoes)
	{
		Notify("Max tornadoes reached - despawn some first");
		return nullptr;
	}
	int budget;
	if (opts.mini)
	{
		budget = std::min(14, FreeLoops());   // what's spare; with none it's drawn with puffs (the big one keeps its smoke)
	}
	else
	{
		// v0.7: an even share of the looped-effect budget for every tornado that's out
		int active = 0;
		for (auto& tp : g_tornadoes) if (!tp->Dissipating() && !tp->Mini()) active++;
		int share = g_set.ptfxBudget / (active + 1);
		for (auto& tp : g_tornadoes)
		{
			if (tp->Mini()) continue;
			int keep = tp->Dissipating() ? std::min(tp->loopsRunning, 10) : share;   // one that's dying down keeps a little
			if (tp->loopsRunning > keep)
				tp->TrimLoopsTo(keep);
		}
		budget = std::min(share, FreeLoops());
	}
	g_tornadoes.push_back(std::make_unique<Tornado>(p, heading, style, label, stationary, budget, opts));
	return g_tornadoes.back().get();
}

static Tornado* SpawnTornado(int style, float dist, float side, bool stationary, const std::string& label, int loopShare = -1,
	float headingJitter = 25.0f, bool userSpawn = false)
{
	if (BigTornadoes() >= kMaxTornadoes)
	{
		Notify("Max tornadoes reached - despawn some first");
		return nullptr;
	}
	// v0.6, playtest 6 ("I don't know where any of this stuff is happening" / "where's the wedge?"): every spawn,
	// tests included, goes where the camera looks.
	float look = CameraHeading();
	if (userSpawn)
	{
		const Style& s = GetStyles()[style];
		float sz = g_set.size == 0 ? 0.7f : g_set.size == 2 ? 1.5f : 1.0f;
		float reach = (s.rBase * sz * 1.5f + 4.0f) * ReachMul();
		dist = std::max(dist, reach + 80.0f);   // v1.0: was +40 ("look at how close it spawned to me")
	}
	V3 p = PointAt(look, dist, side);
	float heading = look + 180.0f + RandRange(-headingJitter, headingJitter);
	int budget;
	if (loopShare < 0)
	{
		// v0.7: an even share of the looped-effect budget for every tornado that's out
		int active = 0;
		for (auto& tp : g_tornadoes) if (!tp->Dissipating()) active++;
		int share = g_set.ptfxBudget / (active + 1);
		for (auto& tp : g_tornadoes)
		{
			int keep = tp->Dissipating() ? std::min(tp->loopsRunning, 10) : share;   // one that's dying down keeps a little
			if (tp->loopsRunning > keep)
				tp->TrimLoopsTo(keep);
		}
		budget = std::min(share, FreeLoops());
	}
	else
		budget = std::min(loopShare, FreeLoops());
	g_tornadoes.push_back(std::make_unique<Tornado>(p, heading, style, label, stationary, budget));
	Tornado& t = *g_tornadoes.back();
	char msg[200];
	if (t.loopsFailed)
		sprintf_s(msg, "%s spawned - %d effects running, %d refused by the game", GetStyles()[style].name, t.loopsRunning, t.loopsFailed);
	else if (userSpawn)
		sprintf_s(msg, "%s touching down %.0f m out", GetStyles()[style].name, dist);
	else
		sprintf_s(msg, "%s spawned", GetStyles()[style].name);
	Notify(msg, 3500);
	if (userSpawn && MISC::GET_MISSION_FLAG())
	{
		// Playtest 4: Arthur died in the tornado during a mission, which failed the mission.
		Notify("Heads up: you're in a mission - a tornado can fail it", 6000);
		Log("spawned during a mission (GET_MISSION_FLAG)");
	}
	return &t;
}

static void TouchCamStart(Tornado* tp);
static float g_lastUserSpawn = -100;
static void SaveTreeCache();
static void UserSpawn()
{
	// v0.7, playtest 7: two spawns landed in the same second (17:20:26) - one press must make one tornado
	if (NowSec() - g_lastUserSpawn < 1.0f)
		return;
	g_lastUserSpawn = NowSec();
	// v0.7, playtest 7 ("can we spawn two tornadoes... unless it just automatically deleted the other one... that probably
	// makes the most sense for performance"): unless Multiple tornadoes is on, the old one dies down as the new one lands.
	if (!g_set.multi)
		for (auto& old : g_tornadoes)
			if (!old->Dissipating())
				old->BeginDissipate(NowSec());
	Tornado* tp = SpawnTornado(g_set.style, 90.0f, 0, false, GetStyles()[g_set.style].name, -1, 25.0f, true);
	if (tp) TouchCamStart(tp);
}

static void DespawnAll()
{
	for (auto& t : g_tornadoes)
		t->Destroy();
	g_tornadoes.clear();
	Log("despawned all tornadoes (loops in use now %d)", LoopsInUse());
}

// Removes tornadoes that finished dissipating, ends expired lifetimes, and drops ones that wandered off into
// unloaded land (audit: a wandering tornado used to live forever).
static void TornadoHousekeeping(float t)
{
	static const float kLifetimes[] = { 0.0f, 120.0f, 300.0f, 600.0f };
	V3 pp = ENTITY::GET_ENTITY_COORDS(PLAYER::PLAYER_PED_ID(), TRUE, FALSE);
	for (size_t i = 0; i < g_tornadoes.size();)
	{
		Tornado& tp = *g_tornadoes[i];
		float life = tp.opts.life > 0 ? tp.opts.life : kLifetimes[(g_set.lifetime >= 0 && g_set.lifetime <= 3) ? g_set.lifetime : 0];
		if (tp.Display()) life = 0;   // v1.2: gallery exhibits stay until you leave
		if (life > 0 && tp.Age() > life && !tp.Dissipating())
			tp.BeginDissipate(t);
		float away = (tp.base - pp).len2d();
		if (away > 1200.0f)
		{
			Log("'%s' removed: %.0f m from the player", tp.label.c_str(), away);
			tp.Destroy();
			g_tornadoes.erase(g_tornadoes.begin() + i);
			continue;
		}
		if (tp.Dead())
		{
			g_tornadoes.erase(g_tornadoes.begin() + i);
			continue;
		}
		i++;
	}
}

static Tornado* NearestTornado(float* distOut = nullptr, bool includeMini = false)
{
	V3 pp = PlayerPos();
	Tornado* best = nullptr;
	float bd = 1e9f;
	for (auto& tp : g_tornadoes)
	{
		if (tp->Mini() && !includeMini) continue;   // v1.1: mini twisters aren't "the" tornado (HUD, storm, cameras)
		if (tp->Display()) continue;                // v1.2: nor are the gallery's exhibits (tracker, music, cameras, shake)
		float d = (tp->base - pp).len2d();
		if (d < bd) { bd = d; best = tp.get(); }
	}
	if (distOut) *distOut = bd;
	return best;
}

// ======================= weather lock =======================
// Playtest 2: the storm drifted back to sunny mid-test. Set, override and freeze; re-assert every 15 s.
static Hash g_lockedHash = 0;
static float g_nextWeatherAssert = 0;

static void LockWeather(const char* name, float transition)
{
	Hash h = H(name);
	MISC::SET_WEATHER_TYPE_FROZEN(FALSE);
	MISC::SET_WEATHER_TYPE(h, TRUE, TRUE, TRUE, transition, FALSE);
	MISC::SET_OVERRIDE_WEATHER(h);
	MISC::SET_WEATHER_TYPE_FROZEN(TRUE);
	g_lockedHash = h;
	g_nextWeatherAssert = NowSec() + 15.0f;
	Log("weather locked -> %s", name);
}

static void UnlockWeather()
{
	MISC::SET_WEATHER_TYPE_FROZEN(FALSE);
	MISC::CLEAR_OVERRIDE_WEATHER();
	MISC::CLEAR_WEATHER_TYPE_PERSIST();
	g_lockedHash = 0;
	Log("weather unlocked");
}

static void WeatherAssert(float t)
{
	if (!g_lockedHash || t < g_nextWeatherAssert)
		return;
	g_nextWeatherAssert = t + 15.0f;
	Hash w1 = 0, w2 = 0;
	float pct = 0;
	MISC::GET_CURR_WEATHER_STATE(&w1, &w2, &pct);
	if (w1 != g_lockedHash && w2 != g_lockedHash)
	{
		Log("weather drifted (0x%08X -> 0x%08X %.2f), re-locking", w1, w2, pct);
		MISC::SET_WEATHER_TYPE_FROZEN(FALSE);
		MISC::SET_WEATHER_TYPE(g_lockedHash, TRUE, TRUE, TRUE, 3.0f, FALSE);
		MISC::SET_OVERRIDE_WEATHER(g_lockedHash);
		MISC::SET_WEATHER_TYPE_FROZEN(TRUE);
	}
}

// ======================= storm control =======================
// v1.1 globals the storm, the menu and the new modes share
struct Storm
{
	bool active = false;
	float savedWind = 0;
	int appliedWeather = -1;
	bool shaking = false;
	bool rainForced = false;
	float nextLightning = 0;
	bool godApplied = false;
	bool lockedByStorm = false;
	bool windApplied = false;
	bool seasonLock = false;   // v1.1: Storm season darkened the sky before its tornado touched down
	float dark = 0;            // v1.1: the dark-sky grade, 0..1
	bool darkApplied = false;
	bool rainHeld = false;     // v1.1: Storm clouds keeps the rain off
} g_storm;
// Playtest 4 frames: the style tour turned the storm off and on between styles, so the sky flashed bright and dark
// while the user was judging each funnel. Tests that swap tornadoes hold the storm for their whole run.
static bool g_holdStorm = false;

static void StormUpdate(float t)
{
	// v1.1: mini twisters (the tornado gun) don't bring a storm; the intro holds it
	bool any = false;
	for (auto& tp : g_tornadoes) if (!tp->Mini()) any = true;
	bool want = any || g_holdStorm;
	Player pl = PLAYER::PLAYER_ID();
	// v1.3 (playtest 11: Arthur died with only the gun's minis out - "invincible" waited for a storm that minis never start):
	// it covers every tornado now, minis too
	if (!want)
	{
		bool minis = !g_tornadoes.empty();
		if (minis && g_set.playerGod) { PLAYER::SET_PLAYER_INVINCIBLE(pl, TRUE); g_storm.godApplied = true; }
		else if (g_storm.godApplied && !g_storm.active) { PLAYER::SET_PLAYER_INVINCIBLE(pl, FALSE); g_storm.godApplied = false; }
	}
	if (want && !g_storm.active)
	{
		g_storm.active = true;
		g_storm.savedWind = MISC::GET_WIND_SPEED();
		g_storm.appliedWeather = -1;
		g_storm.nextLightning = t + RandRange(8.0f, 15.0f);
		Log("storm on (wind %.2f)", g_storm.savedWind);
	}
	if (!want && g_storm.active)
	{
		g_storm.active = false;
		if (g_storm.lockedByStorm && !g_manualWeather) UnlockWeather();
		// research (Rockstar's scripts): -1 hands wind and rain back to the weather system (v1.0 pinned the old speed)
		MISC::SET_WIND_SPEED(-1.0f);
		MISC::SET_WIND_DIRECTION(-1.0f);
		if (g_storm.shaking) CAMERA::STOP_GAMEPLAY_CAM_SHAKING(FALSE);
		if (g_storm.rainForced || g_storm.rainHeld) MISC::SET_RAIN(-1.0f);
		// (audit: if the gun's minis are still out, Arthur stays invincible - no one-frame gap)
		bool keepGod = g_storm.godApplied && g_set.playerGod && !g_tornadoes.empty();
		if (g_storm.godApplied && !keepGod) PLAYER::SET_PLAYER_INVINCIBLE(pl, FALSE);
		if (g_storm.darkApplied && !IntroOwnsGrade()) GRAPHICS::CLEAR_TIMECYCLE_MODIFIER();
		bool seasonLock = g_storm.seasonLock;
		g_storm = Storm();
		g_storm.seasonLock = seasonLock;
		g_storm.godApplied = keepGod;
		Log("storm off (weather unlocked, wind and rain handed back)");
		return;
	}
	if (!want)
		return;

	// the user's own locked weather always wins (playtest 10: a spawn replaced his Thunderstorm with sunshine); the intro sets its own
	bool scene = IntroHoldsSky();
	if (scene) {}
	else if (g_set.weather && !g_manualWeather && g_storm.appliedWeather != g_set.weatherType)
	{
		LockWeather(kWeatherTypes[g_set.weatherType], 5.0f);
		g_storm.appliedWeather = g_set.weatherType;
		g_storm.lockedByStorm = true;
	}
	else if ((!g_set.weather || g_manualWeather) && g_storm.appliedWeather >= 0)
	{
		if (!g_manualWeather) UnlockWeather();
		g_storm.appliedWeather = -1;
		g_storm.lockedByStorm = false;
	}
	// Storm clouds = the THUNDER sky with the rain held off (playtest 6: "the rain gets kind of annoying")
	bool holdRain = g_set.weatherMode == 1 && !g_set.heavyRain && !g_manualWeather;
	if (scene) {}
	else if (holdRain) { MISC::SET_RAIN(0.0f); g_storm.rainHeld = true; }
	else if (g_storm.rainHeld && !g_set.heavyRain) { MISC::SET_RAIN(-1.0f); g_storm.rainHeld = false; }
	// v1.1 dark sky (playtest 10: blue sky and sun in every "Storm clouds" shot): Rockstar's own stormy-sky grade, eased in
	// v1.1 audit 2: the intro's grade is left alone until it has faded out after the handoff (it used to be replaced at once,
	// then cleared under the dark sky), and the ease is per second, not per frame
	bool dark = g_darkSky && g_set.weatherMode > 0 && !g_manualWeather && !IntroOwnsGrade();
	float wantDark = dark ? 0.6f : 0.0f;
	if (fabsf(wantDark - g_storm.dark) > 0.001f && !IntroOwnsGrade())
	{
		g_storm.dark += Clamp(wantDark - g_storm.dark, -0.24f * g_frameDt, 0.24f * g_frameDt);   // ~2.5 s
		if (g_storm.dark > 0.001f)
		{
			GRAPHICS::SET_TIMECYCLE_MODIFIER("nbd1_ext_stormydarksky");
			GRAPHICS::SET_TIMECYCLE_MODIFIER_STRENGTH(g_storm.dark);
			g_storm.darkApplied = true;
		}
		else if (g_storm.darkApplied)
		{
			GRAPHICS::CLEAR_TIMECYCLE_MODIFIER();
			g_storm.darkApplied = false;
		}
	}

	float nd;
	Tornado* nearest = NearestTornado(&nd);
	if (g_set.wind && nearest)
	{
		// v1.0, playtest 8: "if we could add some more sound effects... like oh my god we can hear the tornado in the
		// distance". The game's wind sound follows the wind speed, so the wind howls louder the closer the funnel gets.
		float roarK = g_set.roar ? Clamp(1.0f - nd / std::max(1.0f, nearest->reachRadius() * 2.5f), 0.0f, 1.0f) : 0.0f;
		MISC::SET_WIND_SPEED(g_set.windSpeed + roarK * roarK * 22.0f);   // (v1.4: 28 -> 22, playtest 12: "quieter")
		// v1.1: the wind direction is in DEGREES (Rockstar's scripts) - v1.0 passed radians, so the swirl only moved 6 deg
		float wd = g_set.windSwirl ? fmodf(t * 92.0f, 360.0f) : fmodf(nearest->heading, 360.0f);
		if (wd < 0) wd += 360.0f;   // (never negative: -1 hands the wind back to the weather)
		MISC::SET_WIND_DIRECTION(wd);
		g_storm.windApplied = true;
	}
	else if (g_storm.windApplied)
	{
		MISC::SET_WIND_SPEED(-1.0f);
		g_storm.windApplied = false;
	}
	// Playtest 2: "lightning is loud as hell, make it less frequent". Rare = every 25-50 s; near = strikes the funnel.
	if (IntroHoldsSky()) g_storm.nextLightning = std::max(g_storm.nextLightning, t + 20.0f);   // (v1.6 review: not the moment it's over)
	if (g_set.lightning > 0 && t > g_storm.nextLightning && !IntroHoldsSky())   // (v1.6: the intro has its own, quieter)
	{
		g_storm.nextLightning = t + (g_set.lightning == 2 ? RandRange(45.0f, 90.0f) : RandRange(70.0f, 140.0f));
		if (g_set.lightning == 2 && nearest)
		{
			float ang = RandRange(0, 2 * PI);
			V3 p = nearest->base + V3(cosf(ang), sinf(ang), 0) * RandRange(10.0f, 40.0f);
			MISC::FORCE_LIGHTNING_FLASH_AT_COORDS(p.x, p.y, p.z, -1.0f);   // -1: as in all of Rockstar's calls
		}
		else
			MISC::FORCE_LIGHTNING_FLASH();
	}
	if (g_set.heavyRain)
	{
		MISC::SET_RAIN(1.0f);
		g_storm.rainForced = true;
	}
	else if (g_storm.rainForced)
	{
		MISC::SET_RAIN(-1.0f);
		g_storm.rainForced = false;
	}
	if (!g_set.camShake && g_storm.shaking)
	{
		CAMERA::STOP_GAMEPLAY_CAM_SHAKING(FALSE);
		g_storm.shaking = false;
	}
	if (g_set.camShake && nearest && !IntroRunning())
	{
		float k = Clamp(1.0f - nd / (nearest->reachRadius() * 1.2f), 0, 1);
		if (k > 0.01f)
		{
			if (!CAMERA::IS_GAMEPLAY_CAM_SHAKING())
				CAMERA::SHAKE_GAMEPLAY_CAM(g_set.shakeName.c_str(), k * g_set.shakeMax * (t - PlayerLastInWall() < 1.0f ? 1.6f : 1.0f));
			else
				// v1.0, playtest 8 ("I'm in the tornado, the camera's doing a thing... depending on the state you're in, but
				// don't make it abysmal to play"): a stronger shake while it has you
				CAMERA::SET_GAMEPLAY_CAM_SHAKE_AMPLITUDE(k * g_set.shakeMax * (t - PlayerLastInWall() < 1.0f ? 1.6f : 1.0f));
			g_storm.shaking = true;
		}
		else if (g_storm.shaking)
		{
			CAMERA::STOP_GAMEPLAY_CAM_SHAKING(FALSE);
			g_storm.shaking = false;
		}
	}
	if (g_set.playerGod)
	{
		PLAYER::SET_PLAYER_INVINCIBLE(pl, TRUE);
		g_storm.godApplied = true;
	}
	else if (g_storm.godApplied)
	{
		PLAYER::SET_PLAYER_INVINCIBLE(pl, FALSE);
		g_storm.godApplied = false;
	}
}

// ======================= world controls (native-trainer style) =======================
static int g_hour = 12, g_worldWeather = 0, g_teleport = 0;
static bool g_freezeTime = false;
// Playtest 4: "No one needs sandstorm weather, man." Hurricane is a clear sky in RDR2. Both removed.
static const char* kWorldWeathers[] = { "SUNNY", "CLOUDS", "OVERCAST", "OVERCASTDARK", "DRIZZLE", "RAIN", "SHOWER", "THUNDER", "THUNDERSTORM", "FOG", "MISTY", "SNOW", "BLIZZARD" };
static const int kWorldWeatherCount = sizeof(kWorldWeathers) / sizeof(kWorldWeathers[0]);
struct Place { const char* name; V3 pos; };
static const Place kPlaces[] = {
	{ "Rhodes fields (playtest 1)", { 1482.0f, -1465.6f, 73.2f } },
	{ "Rhodes (town)",              { 1282.7f, -1275.7f, 74.9f } },
	{ "Braithwaite Manor",          { 1011.2f, -1661.7f, 45.9f } },
	{ "Caliga Hall fields (pt 4)",  { 1713.4f, -1402.0f, 43.3f } },
	{ "Emerald Ranch",              { 1332.3f, 300.4f, 86.3f } },
	{ "Valentine",                  { -213.2f, 691.8f, 112.4f } },
	{ "Saint Denis",                { 2336.6f, -1106.2f, 44.7f } },
	{ "Blackwater",                 { -798.3f, -1238.9f, 43.5f } },
	{ "Strawberry",                 { -1725.2f, -418.1f, 153.6f } },
	{ "Armadillo",                  { -3622.7f, -2586.6f, -15.4f } },
};
static const int kPlaceCount = sizeof(kPlaces) / sizeof(kPlaces[0]);

static void SetHour(int h)
{
	g_hour = ((h % 24) + 24) % 24;
	CLOCK::SET_CLOCK_TIME(g_hour, 0, 0);
	Log("time -> %02d:00", g_hour);
	char b[40]; sprintf_s(b, "Time set to %02d:00", g_hour); Notify(b, 1500);
}
static void TeleportTo(const V3& p)
{
	if (BalloonActive()) BalloonRemove("teleport");   // v1.1 audit 2: out of the balloon first (it can't come along)
	Ped ped = PLAYER::PLAYER_PED_ID();
	Entity e = PED::IS_PED_ON_MOUNT(ped) ? PED::GET_MOUNT(ped) : ped;
	ENTITY::SET_ENTITY_COORDS(e, p.x, p.y, p.z + 1.0f, FALSE, FALSE, FALSE, FALSE);
	Log("teleport -> (%.1f, %.1f, %.1f)", p.x, p.y, p.z);
}

// ======================= drone camera (v0.4) =======================
// Hands-off B-roll: a scripted camera that orbits the tornado, watches it from ground level, or looks over Arthur's
// shoulder at it. Arthur can still be moved while it runs.
static const char* kDroneModes[] = { "Normal", "Drone: orbit", "Drone: low angle", "Drone: over Arthur's shoulder" };
struct Drone { int mode = 0; int cam = 0; bool active = false; float ang = 0; V3 pos; } g_drone;

static void DroneRelease(bool ease)
{
	if (g_drone.cam && CAMERA::DOES_CAM_EXIST(g_drone.cam))
	{
		CAMERA::SET_CAM_ACTIVE(g_drone.cam, FALSE);
		CAMERA::RENDER_SCRIPT_CAMS(FALSE, ease, ease ? 900 : 0, TRUE, FALSE, 0);
		CAMERA::DESTROY_CAM(g_drone.cam, FALSE);
		Log("drone cam off");
	}
	else if (g_drone.active)
		CAMERA::RENDER_SCRIPT_CAMS(FALSE, FALSE, 0, TRUE, FALSE, 0);
	g_drone.cam = 0;
	g_drone.active = false;
}

static void DroneCycle()
{
	g_drone.mode = (g_drone.mode + 1) % 4;
	Notify(std::string("Drone camera: ") + kDroneModes[g_drone.mode], 2500);
}

static void DroneUpdate(float dt, bool blocked)
{
	if (g_drone.mode == 0 || blocked)
	{
		if (g_drone.active) DroneRelease(!blocked);
		if (blocked && g_drone.mode) g_drone.mode = 0;
		return;
	}
	float nd;
	Tornado* tp = NearestTornado(&nd);
	V3 pp = PlayerPos();
	V3 want, focus;
	float fov = 50.0f;
	if (!tp)
	{
		// v0.5, playtest 5 ("Low angle, is it going to change?... it's just still here"): with no tornado every mode
		// used the same 22 m orbit. Each mode now has its own framing around Arthur.
		V3 fwd = HeadingDir(PlayerHeading());
		if (g_drone.mode == 1)
		{
			g_drone.ang += dt * 0.15f;
			want = pp + V3(cosf(g_drone.ang) * 22.0f, sinf(g_drone.ang) * 22.0f, 7.0f);
			focus = pp + V3(0, 0, 1.2f);
		}
		else if (g_drone.mode == 2)
		{
			g_drone.ang += dt * 0.06f;
			want = pp + V3(cosf(g_drone.ang) * 12.0f, sinf(g_drone.ang) * 12.0f, 1.0f);
			focus = pp + V3(0, 0, 1.6f);
			fov = 62.0f;
		}
		else
		{
			V3 side(-fwd.y, fwd.x, 0);
			want = pp - fwd * 4.0f + side * 0.9f + V3(0, 0, 1.9f);
			focus = pp + fwd * 25.0f + V3(0, 0, 1.2f);
			fov = 55.0f;
		}
	}
	else
	{
		float H = tp->height();
		V3 toMe = pp - tp->base;
		float dm = std::max(toMe.len2d(), 1.0f);
		V3 dir(toMe.x / dm, toMe.y / dm, 0);
		if (g_drone.mode == 1)
		{
			g_drone.ang += dt * 0.09f;
			float R = std::max(150.0f, H * 1.3f);
			want = tp->base + V3(cosf(g_drone.ang) * R, sinf(g_drone.ang) * R, H * 0.22f);
			focus = tp->base + V3(0, 0, H * 0.42f);
		}
		else if (g_drone.mode == 2)
		{
			float R = std::max(70.0f, tp->wallRadius() * 4.5f);
			want = tp->base + dir * R + V3(dir.y * 12.0f, -dir.x * 12.0f, 1.4f);
			focus = tp->base + V3(0, 0, H * 0.55f);
			fov = 62.0f;
		}
		else
		{
			V3 side(-dir.y, dir.x, 0);
			want = pp + dir * 9.0f + side * 2.5f + V3(0, 0, 3.0f);
			focus = tp->base + V3(0, 0, H * 0.3f);
			fov = 55.0f;
		}
	}
	float gz = GroundZ(want.x, want.y, want.z + 60.0f, want.z - 50.0f);
	want.z = std::max(want.z, gz + 1.0f);
	if (!g_drone.active)
	{
		g_drone.pos = want;
		g_drone.cam = CAMERA::CREATE_CAM_WITH_PARAMS("DEFAULT_SCRIPTED_CAMERA", want.x, want.y, want.z, 0, 0, 0, fov, FALSE, 2);
		if (!g_drone.cam)
		{
			Log("drone cam: CREATE_CAM_WITH_PARAMS failed");
			Notify("Drone camera could not start");
			g_drone.mode = 0;
			return;
		}
		CAMERA::SET_CAM_ACTIVE(g_drone.cam, TRUE);
		CAMERA::RENDER_SCRIPT_CAMS(TRUE, TRUE, 1200, TRUE, FALSE, 0);
		g_drone.active = true;
		Log("drone cam on (%s)", kDroneModes[g_drone.mode]);
	}
	else
		g_drone.pos = g_drone.pos + (want - g_drone.pos) * Clamp(dt * 1.6f, 0.0f, 1.0f);
	CAMERA::SET_CAM_COORD(g_drone.cam, g_drone.pos.x, g_drone.pos.y, g_drone.pos.z);
	CAMERA::POINT_CAM_AT_COORD(g_drone.cam, focus.x, focus.y, focus.z);
	CAMERA::SET_CAM_FOV(g_drone.cam, fov);
}

// ======================= FX Lab =======================
static const char* kLabLooped[] = {
	"env_sandstorm", "env_smoke", "env_fog", "env_air_debris", "env_desert_ground_dust", "env_dust_motes",
	"env_wind_debris_city", "env_wind_debris_city_slum", "env_wind_debris_countryside", "env_wind_debris_desert",
	"env_wind_debris_woodland", "env_wind_debris_woodland_pine", "env_wind_debris_mountain",
	"ent_amb_wind_litter_dust_swirl", "ent_amb_wind_litter_swirl", "ent_amb_wind_leaves_swirl", "ent_amb_wind_litter_dust",
	"ent_amb_wind_litter_dust_dir", "ent_amb_wind_leaves", "ent_amb_wind_hay", "ent_amb_wind_grass", "ent_amb_wind_tree_leaves",
	"ent_amb_smoke_stack_l_mainstream", "ent_amb_smoke_stack_m_mainstream", "ent_amb_smoke_stack_m_wind",
	"ent_amb_smoke_stack_s_scatter", "ent_amb_smoke_factory", "ent_amb_smoke_factory_exhaust", "ent_amb_smoke_chimney_long",
	"ent_amb_smoke_mine", "ent_amb_generic_fire_smoke_plume", "ent_amb_generic_roof_smoke_fog",
	"ent_amb_mountain_cloud_01", "ent_amb_mountain_cloud_02", "env_cloud", "ent_amb_fog_ring", "ent_amb_river_mist_gen",
	"ent_amb_rolling_boulder_dust", "ent_amb_falling_tree_leaves", "ent_amb_haystack_debris", "ent_amb_sde_smokegroup_1",
	"ent_amb_ann_smokegroup_01", "exp_grn_cloud",
};
static const char* kLabPuffs[] = {
	"exp_grd_smoke_post", "exp_grd_smoke_post_small", "ent_amb_falling_smoke", "ent_amb_falling_smoke_bam",
	"ent_dst_dust", "ent_brk_dust", "bang_dust", "ent_col_gen_tree_dust", "bang_dirt_dry", "ent_dst_dirt", "ent_brk_dirt",
	"bang_sand", "ent_dst_sand", "ent_amb_falling_dust_debris", "ent_amb_falling_debris", "exp_wood_debris",
	"ent_dst_tower_debris", "ent_brk_tree_leaves", "bang_leaves", "ent_col_tree_leaves_oak", "ent_brk_hay", "exp_smoke_trail",
};
static const int kLabLoopedCount = sizeof(kLabLooped) / sizeof(kLabLooped[0]);
static const int kLabPuffCount = sizeof(kLabPuffs) / sizeof(kLabPuffs[0]);
static const float kLabScales[] = { 0.5f, 1.0f, 2.0f, 3.0f, 5.0f, 8.0f, 12.0f };

struct FxLab
{
	int kind = 0;          // 0 looped, 1 puffs
	int idx = 0, scaleIdx = 2, layout = 1;
	bool on = false, slideshow = false;
	float nextSlide = 0, nextPuff = 0;
	V3 origin;
	std::vector<Object> anchors;
	std::vector<int> fx;
	int ok = 0, failed = 0;
} g_lab;

static int LabCount() { return g_lab.kind == 0 ? kLabLoopedCount : kLabPuffCount; }
static const char* kLabLayouts[] = { "single, still", "single, circling", "column, still", "column, spinning" };

// Anchor i of n. Still and moving layouts share the same count, heights and scale, so the ONLY difference between
// "single, still" and "single, circling" (or the two columns) is motion (Astra: isolate the variable).
static V3 LabAnchorPos(int i, int n, float t)
{
	float hf = n > 1 ? (float)i / (n - 1) : 0.0f;
	float r = n > 1 ? 2.0f + 8.0f * hf * hf : 6.0f;
	float z = n > 1 ? 1.0f + hf * 45.0f : 5.0f;
	bool moving = g_lab.layout == 1 || g_lab.layout == 3;
	float ang = i * 0.9f + (moving ? t * 3.0f * Lerp(1.4f, 0.7f, hf) : 0.0f);
	return g_lab.origin + V3(cosf(ang) * r, sinf(ang) * r, z);
}

static int LabAlive()
{
	int n = 0;
	for (int h : g_lab.fx) if (h && GRAPHICS::DOES_PARTICLE_FX_LOOPED_EXIST(h)) n++;
	return n;
}
static const char* LabName() { return g_lab.kind == 0 ? kLabLooped[g_lab.idx % kLabLoopedCount] : kLabPuffs[g_lab.idx % kLabPuffCount]; }

static void LabStop()
{
	for (int h : g_lab.fx)
		if (h && GRAPHICS::DOES_PARTICLE_FX_LOOPED_EXIST(h))
		{
			GRAPHICS::STOP_PARTICLE_FX_LOOPED(h, FALSE);
			GRAPHICS::REMOVE_PARTICLE_FX(h, FALSE);
		}
	AddLoopsInUse(-g_lab.ok);
	g_lab.ok = g_lab.failed = 0;
	for (Object& o : g_lab.anchors)
		DeleteAnchor(o);
	g_lab.fx.clear();
	g_lab.anchors.clear();
	g_lab.on = false;
}

static void LabStart(bool keepOrigin = false)
{
	LabStop();
	if (!LoadPtfxAsset("core"))
	{
		Notify("FX Lab: 'core' particle library did not load");
		return;
	}
	if (!keepOrigin || g_lab.origin.len() < 1)
		g_lab.origin = PointAheadCam(18.0f);
	g_lab.on = true;
	if (g_lab.kind == 1)
	{
		Log("FX Lab puffs: %s scale %.1f", LabName(), kLabScales[g_lab.scaleIdx]);
		return;   // puffs are sprayed from LabUpdate
	}
	int count = g_lab.layout <= 1 ? 1 : 8;   // 0 single still, 1 single circling, 2 column still, 3 column spinning
	count = std::min(count, FreeLoops());
	for (int i = 0; i < count; i++)
	{
		Object a = SpawnAnchor(LabAnchorPos(i, count, 0.0f));
		g_lab.anchors.push_back(a);
		int h = 0;
		if (a)
		{
			GRAPHICS::USE_PARTICLE_FX_ASSET("core");
			h = GRAPHICS::START_PARTICLE_FX_LOOPED_ON_ENTITY(LabName(), a, 0, 0, 0, 0, 0, 0, kLabScales[g_lab.scaleIdx], FALSE, FALSE, FALSE);
			if (h) GRAPHICS::SET_PARTICLE_FX_LOOPED_FAR_CLIP_DIST(h, 1000.0f);
		}
		g_lab.fx.push_back(h);
		(h ? g_lab.ok : g_lab.failed)++;
	}
	AddLoopsInUse(g_lab.ok);
	Log("FX Lab looped: %s scale %.1f layout %s -> %d running, %d refused", LabName(), kLabScales[g_lab.scaleIdx],
		kLabLayouts[g_lab.layout % 4], g_lab.ok, g_lab.failed);
}

static void LabUpdate(float t)
{
	if (!g_lab.on)
		return;
	if (g_lab.slideshow && t >= g_lab.nextSlide)
	{
		g_lab.nextSlide = t + 6.0f;
		g_lab.idx = (g_lab.idx + 1) % LabCount();
		LabStart(true);
	}
	if (g_lab.kind == 1)
	{
		if (t >= g_lab.nextPuff)
		{
			g_lab.nextPuff = t + 0.25f;
			int n = g_lab.layout == 0 ? 1 : 4;
			for (int i = 0; i < n; i++)
			{
				float hf = n > 1 ? (float)i / (n - 1) : 0;
				float ang = RandRange(0, 2 * PI);
				float r = 2.0f + 8.0f * hf * hf;
				V3 p = g_lab.origin + V3(cosf(ang) * r, sinf(ang) * r, 1.0f + hf * 40.0f);
				GRAPHICS::USE_PARTICLE_FX_ASSET("core");
				GRAPHICS::START_PARTICLE_FX_NON_LOOPED_AT_COORD(LabName(), p.x, p.y, p.z, 0, 0, 0, kLabScales[g_lab.scaleIdx], FALSE, FALSE, FALSE);
			}
		}
		return;
	}
	if (g_lab.layout == 0 || g_lab.layout == 2)
		return;   // still layouts: anchors never move (same count/height/scale as their moving twins)
	for (size_t i = 0; i < g_lab.anchors.size(); i++)
	{
		if (!g_lab.anchors[i] || !ENTITY::DOES_ENTITY_EXIST(g_lab.anchors[i])) continue;
		V3 p = LabAnchorPos((int)i, (int)g_lab.anchors.size(), t);
		ENTITY::SET_ENTITY_COORDS_NO_OFFSET(g_lab.anchors[i], p.x, p.y, p.z, FALSE, FALSE, FALSE);
	}
}

static void LabRate(const char* verdict)
{
	Finding("FXLAB %-18s | %s %s | scale %.1f | layout %s | started %d refused %d alive %d | anchors %s",
		verdict, g_lab.kind ? "PUFF" : "LOOP", LabName(), kLabScales[g_lab.scaleIdx], kLabLayouts[g_lab.layout % 4],
		g_lab.ok, g_lab.failed, LabAlive(), g_anchorHideMode == 0 ? "visible" : g_anchorHideMode == 1 ? "invisible" : "alpha 0");
	Notify(std::string("Saved: ") + verdict + " - " + LabName());
}

// ======================= render check rig (v0.4) =======================
// The invisible/flickering-funnel question, asked one variable at a time with no buttons to press: the user just
// says SOLID / FLICKERS / NOTHING out loud and the recording's transcript is the result (playtest 4: there was no time
// for the FX Lab's manual matrix). Every variant is the same column of 8 dark smoke stacks at scale 3.
static const int kRigVariants = 8;
static const char* kRigNames[kRigVariants] = {
	"apples hidden, still",
	"apples hidden, spinning like a tornado",
	"apples hidden + far draw distance, spinning",
	"apples transparent (alpha 0), spinning",
	"apples VISIBLE, spinning",
	"no apples: world-space smoke, still",
	"no apples: world-space smoke moved each frame (orange lights mark its path)",
	"no looping smoke: one-shot puff spiral only",
};
struct RenderRig
{
	bool on = false;
	int variant = 0;
	float dist = 70;
	V3 origin;
	std::vector<Object> anchors;
	std::vector<int> fx;
	int ok = 0;
	bool spinning = false, world = false, puffsOnly = false, guides = false;
	float nextPuff = 0;
} g_rig;

static V3 RigPos(int i, int n, float t)
{
	float hf = n > 1 ? (float)i / (n - 1) : 0.0f;
	float r = 3.0f + 9.0f * hf;
	float ang = i * 0.9f + (g_rig.spinning ? t * 2.6f * Lerp(1.4f, 0.7f, hf) : 0.0f);
	return g_rig.origin + V3(cosf(ang) * r, sinf(ang) * r, 2.0f + hf * 55.0f);
}

static void RigStop()
{
	if (!g_rig.on)
		return;
	int alive = 0;
	for (int h : g_rig.fx)
		if (h && GRAPHICS::DOES_PARTICLE_FX_LOOPED_EXIST(h))
		{
			alive++;
			GRAPHICS::STOP_PARTICLE_FX_LOOPED(h, FALSE);
			GRAPHICS::REMOVE_PARTICLE_FX(h, FALSE);
		}
	Log("RENDER CHECK variant %d (%s, %.0f m) ended: %d of %d looped handles still alive", g_rig.variant + 1, kRigNames[g_rig.variant], g_rig.dist, alive, g_rig.ok);
	AddLoopsInUse(-g_rig.ok);
	for (Object& o : g_rig.anchors)
		DeleteAnchor(o);
	g_rig.anchors.clear();
	g_rig.fx.clear();
	g_rig.ok = 0;
	g_rig.on = false;
}

static void RigStart(int variant, float dist)
{
	RigStop();
	g_rig.on = true;
	g_rig.variant = variant;
	g_rig.dist = dist;
	g_rig.spinning = variant != 0 && variant != 5;
	g_rig.world = variant == 5 || variant == 6;
	g_rig.puffsOnly = variant == 7;
	g_rig.guides = variant == 6;
	g_rig.origin = PointAheadCam(dist);
	if (g_rig.puffsOnly)
	{
		Log("RENDER CHECK variant %d (%s) at %.0f m: one-shot puffs", variant + 1, kRigNames[variant], dist);
		return;
	}
	int savedHide = g_anchorHideMode, savedLod = g_anchorLod;
	g_anchorHideMode = variant == 3 ? 2 : variant == 4 ? 0 : 1;
	g_anchorLod = variant == 2 ? 1500 : 0;    // only this variant raises the apple's draw distance
	int n = std::min(8, FreeLoops());
	float t = NowSec();
	for (int i = 0; i < n; i++)
	{
		V3 p = RigPos(i, n, t);
		int h = 0;
		GRAPHICS::USE_PARTICLE_FX_ASSET("core");
		if (g_rig.world)
			h = GRAPHICS::START_PARTICLE_FX_LOOPED_AT_COORD("ent_amb_smoke_stack_m_mainstream", p.x, p.y, p.z, 0, 0, 0, 3.0f, FALSE, FALSE, FALSE, FALSE);
		else
		{
			Object a = SpawnAnchor(p);
			g_rig.anchors.push_back(a);
			if (a)
			{
				GRAPHICS::USE_PARTICLE_FX_ASSET("core");
				h = GRAPHICS::START_PARTICLE_FX_LOOPED_ON_ENTITY("ent_amb_smoke_stack_m_mainstream", a, 0, 0, 0, 0, 0, 0, 3.0f, FALSE, FALSE, FALSE);
			}
		}
		if (h)
		{
			GRAPHICS::SET_PARTICLE_FX_LOOPED_FAR_CLIP_DIST(h, 1500.0f);
			if (g_set.darkTint) GRAPHICS::SET_PARTICLE_FX_LOOPED_COLOUR(h, 0.32f, 0.29f, 0.26f, FALSE);
			g_rig.ok++;
		}
		g_rig.fx.push_back(h);
	}
	AddLoopsInUse(g_rig.ok);
	g_anchorHideMode = savedHide;
	g_anchorLod = savedLod;
	Log("RENDER CHECK variant %d (%s) at %.0f m: %d of %d looped effects started", variant + 1, kRigNames[variant], dist, g_rig.ok, n);
}

static void RigUpdate(float t)
{
	if (!g_rig.on)
		return;
	if (g_rig.puffsOnly)
	{
		if (t < g_rig.nextPuff)
			return;
		g_rig.nextPuff = t + 0.05f;
		int arm = rand() % 3;
		float hf = Rand01();
		float ang = arm * 2 * PI / 3 + t * 2.6f * Lerp(1.4f, 0.7f, hf) + hf * 3.0f;
		float r = 3.0f + 9.0f * hf;
		V3 p = g_rig.origin + V3(cosf(ang) * r, sinf(ang) * r, 2.0f + hf * 55.0f);
		GRAPHICS::USE_PARTICLE_FX_ASSET("core");
		if (g_set.darkTint) GRAPHICS::SET_PARTICLE_FX_NON_LOOPED_COLOUR(0.32f, 0.29f, 0.26f);
		GRAPHICS::START_PARTICLE_FX_NON_LOOPED_AT_COORD("exp_grd_smoke_post", p.x, p.y, p.z, 0, 0, RandRange(0, 360), 2.5f, FALSE, FALSE, FALSE);
		return;
	}
	int n = (int)g_rig.fx.size();
	for (int i = 0; i < n; i++)
	{
		V3 p = RigPos(i, n, t);
		if (g_rig.guides)
			GRAPHICS::DRAW_LIGHT_WITH_RANGE(p.x, p.y, p.z, 255, 140, 40, 7.0f, 6.0f);
		if (!g_rig.spinning)
			continue;
		if (g_rig.world)
		{
			if (g_rig.fx[i] && GRAPHICS::DOES_PARTICLE_FX_LOOPED_EXIST(g_rig.fx[i]))
				GRAPHICS::SET_PARTICLE_FX_LOOPED_OFFSETS(g_rig.fx[i], p.x, p.y, p.z, 0, 0, 0);
		}
		else if (i < (int)g_rig.anchors.size() && g_rig.anchors[i] && ENTITY::DOES_ENTITY_EXIST(g_rig.anchors[i]))
			ENTITY::SET_ENTITY_COORDS_NO_OFFSET(g_rig.anchors[i], p.x, p.y, p.z, FALSE, FALSE, FALSE);
	}
}

// ======================= trees =======================
static const char* kDefaultReplacementTrees[] = {
	"p_tree_oak_01_script", "p_tree_w_r_cedar_01_script", "p_tree_fallen_pineprop", "p_tree_hangtreeoak_ropeswing",
	"p_tree_fallen_pine_rope01x", "des_tree_fall_neutral",
};
struct ScannedTree { std::string name; float height; };
static std::vector<ScannedTree> g_spawnableTrees;   // filled by the scan, tallest first
static int g_treeModel = 0, g_hideRadius = 1;
static const float kHideRadii[] = { 4.0f, 8.0f, 15.0f };
struct HidePoint { V3 p; float r; };
static std::vector<HidePoint> g_hidden;
static std::vector<Object> g_testTrees;

struct Scheduled { float at; std::function<void()> fn; };
static std::vector<Scheduled> g_scheduled;
static void After(float sec, std::function<void()> fn) { g_scheduled.push_back({ NowSec() + sec, fn }); }

static int TreeChoiceCount() { return g_spawnableTrees.empty() ? (int)(sizeof(kDefaultReplacementTrees) / sizeof(kDefaultReplacementTrees[0])) : (int)g_spawnableTrees.size(); }
static std::string TreeChoiceName(int i)
{
	if (g_spawnableTrees.empty()) return kDefaultReplacementTrees[i % TreeChoiceCount()];
	return g_spawnableTrees[i % g_spawnableTrees.size()].name;
}
static std::string TreeChoiceLabel(int i)
{
	if (g_spawnableTrees.empty()) return TreeChoiceName(i) + " (run scan)";
	char b[96]; sprintf_s(b, "%s %.0fm", g_spawnableTrees[i % g_spawnableTrees.size()].name.c_str(), g_spawnableTrees[i % g_spawnableTrees.size()].height);
	return b;
}

static void HideTreesAt(const V3& p, float r)
{
	for (auto& tm : kTreeModels)
		ENTITY::CREATE_MODEL_HIDE(p.x, p.y, p.z, r, tm.hash, TRUE);
	g_hidden.push_back({ p, r });
	Log("tree: hid %d tree models within %.0fm of (%.1f, %.1f, %.1f)", (int)(sizeof(kTreeModels) / sizeof(kTreeModels[0])), r, p.x, p.y, p.z);
}

static void RestoreTrees()
{
	for (auto& hp : g_hidden)
		for (auto& tm : kTreeModels)
			ENTITY::REMOVE_MODEL_HIDE(hp.p.x, hp.p.y, hp.p.z, hp.r, tm.hash, FALSE);
	Log("tree: restored %d hidden areas", (int)g_hidden.size());
	Notify("Restored " + std::to_string(g_hidden.size()) + " hidden tree areas");
	g_hidden.clear();
}

static Object SpawnTreeModel(const std::string& name, const V3& ground, bool frozen, bool quiet = false)
{
	Hash model = H(name.c_str());
	if (!LoadModel(model, 1500))
	{
		if (!quiet) { Log("tree: model %s would not load", name.c_str()); Notify("Tree model would not load: " + name); }
		return 0;
	}
	Vector3 mn = {}, mx = {};
	MISC::GET_MODEL_DIMENSIONS(model, &mn, &mx);
	Object o = OBJECT::CREATE_OBJECT(model, ground.x, ground.y, ground.z - mn.z, FALSE, FALSE, TRUE, FALSE, FALSE);
	STREAMING::SET_MODEL_AS_NO_LONGER_NEEDED(model);
	if (!o)
	{
		if (!quiet) { Log("tree: CREATE_OBJECT failed for %s", name.c_str()); Notify("The game refused to spawn " + name); }
		return 0;
	}
	ENTITY::SET_ENTITY_ROTATION(o, 0, 0, RandRange(0, 360), 2, TRUE);
	OBJECT::PLACE_OBJECT_ON_GROUND_PROPERLY(o, FALSE);
	ENTITY::FREEZE_ENTITY_POSITION(o, frozen ? TRUE : FALSE);
	NoteModTree(o);
	if (!quiet)
		Log("tree: spawned %s (handle %d) height %.1fm frozen=%d", name.c_str(), o, mx.z - mn.z, frozen);
	return o;
}

// A planted tree is frozen upright and registered as a priority target: the tornado always sees it and uproots it.
static Object PlantTree(const std::string& name, const V3& ground)
{
	Object o = SpawnTreeModel(name, ground, true);
	if (o)
	{
		g_testTrees.push_back(o);
		g_priority.insert(o);
	}
	return o;
}

static void PlantTreesAhead(int count, float dist, float spacing)
{
	int planted = 0;
	for (int i = 0; i < count; i++)
	{
		std::string name = TreeChoiceName(g_spawnableTrees.empty() ? g_treeModel : i % std::max(1, (int)std::min<size_t>(3, g_spawnableTrees.size())));
		if (PlantTree(name, PointAheadCam(dist, (i - (count - 1) * 0.5f) * spacing)))
			planted++;
	}
	Notify("Planted " + std::to_string(planted) + " tree(s) - send a tornado through them", 4000);
}

static void UprootTest()
{
	V3 p = PointAhead(10.0f);
	HideTreesAt(p, kHideRadii[g_hideRadius]);
	Object o = SpawnTreeModel(TreeChoiceName(g_treeModel), p, true);
	if (!o)
		return;
	g_testTrees.push_back(o);
	Notify("Uprooting in 1.5s...");
	After(1.5f, [o, p]()
	{
		if (!ENTITY::DOES_ENTITY_EXIST(o)) return;
		ENTITY::FREEZE_ENTITY_POSITION(o, FALSE);
		ENTITY::SET_ENTITY_DYNAMIC(o, TRUE);
		PHYSICS::ACTIVATE_PHYSICS(o);
		ENTITY::SET_ENTITY_VELOCITY(o, 0, 0, 14.0f);
		ENTITY::APPLY_FORCE_TO_ENTITY(o, 1, 6.0f, 0, 0, 0, 0, 6.0f, 0, FALSE, TRUE, TRUE, FALSE, TRUE);
		if (LoadPtfxAsset("core", 500))
		{
			GRAPHICS::USE_PARTICLE_FX_ASSET("core");
			GRAPHICS::START_PARTICLE_FX_NON_LOOPED_AT_COORD("ent_brk_dirt", p.x, p.y, p.z + 0.5f, 0, 0, 0, 3.0f, FALSE, FALSE, FALSE);
			GRAPHICS::USE_PARTICLE_FX_ASSET("core");
			GRAPHICS::START_PARTICLE_FX_NON_LOOPED_AT_COORD("bang_dirt_dry", p.x, p.y, p.z + 0.5f, 0, 0, 0, 3.0f, FALSE, FALSE, FALSE);
		}
		Log("tree: uproot test launched handle %d", o);
	});
}

static void DeleteTestTrees()
{
	for (Object& o : g_testTrees)
	{
		g_priority.erase(o);
		DeleteObj(o);
	}
	g_testTrees.clear();
	g_priority.clear();
	Notify("Deleted spawned test trees");
}

static int RayFan(const V3& from, float heading, float range, std::vector<float>* dists)
{
	int hits = 0;
	Ped player = PLAYER::PLAYER_PED_ID();
	for (int i = -3; i <= 3; i++)
	{
		V3 dir = HeadingDir(heading + i * 6.0f);
		V3 to = from + dir * range;
		int h = SHAPETEST::START_EXPENSIVE_SYNCHRONOUS_SHAPE_TEST_LOS_PROBE(from.x, from.y, from.z, to.x, to.y, to.z, -1, player, 0);
		BOOL hit = FALSE;
		Vector3 end = {}, normal = {};
		Entity ent = 0;
		SHAPETEST::GET_SHAPE_TEST_RESULT(h, &hit, &end, &normal, &ent);
		float d = hit ? (V3(end) - from).len() : -1.0f;
		if (hit) hits++;
		if (dists) dists->push_back(d);
	}
	return hits;
}

static void TreeCollisionTest()
{
	V3 from = PlayerPos() + V3(0, 0, 0.8f);
	float heading = PlayerHeading();
	std::vector<float> before;
	int hitsBefore = RayFan(from, heading, 12.0f, &before);
	if (hitsBefore == 0)
	{
		Notify("Collision check: nothing in front within 12 m - face a tree 3-10 m away", 5000);
		Log("tree collision test: no hits before hiding (nothing ahead)");
		return;
	}
	V3 spot = PlayerPos() + HeadingDir(heading) * 7.0f;
	HideTreesAt(spot, 8.0f);
	Notify("Collision check: tree hidden, re-checking in 1s...");
	After(1.0f, [from, heading, before, hitsBefore]()
	{
		std::vector<float> after;
		int hitsAfter = RayFan(from, heading, 12.0f, &after);
		char line[400] = {};
		int off = 0;
		for (size_t i = 0; i < before.size(); i++)
			off += sprintf_s(line + off, sizeof(line) - off, " [%.1f->%.1f]", before[i], i < after.size() ? after[i] : -2.0f);
		const char* verdict = hitsAfter < hitsBefore ? "COLLISION REMOVED with the tree" : "COLLISION STAYED (invisible wall)";
		Finding("TREE COLLISION TEST: rays blocked before %d, after %d -> %s. per-ray metres (-1 = clear):%s", hitsBefore, hitsAfter, verdict, line);
		Notify(std::string("Collision check: ") + verdict + " - does the tree look gone?", 6000);
	});
}

// ======================= v1.1: map tree check =======================
// The open questions about real map trees, answered in one go (research\ui_world_research.md 4.3): face a real tree, and
//   1. every tree model whose art is loaded is "pinned" (PIN_CLOSEST_MAP_ENTITY - Rockstar's way to get a handle on a map
//      object): which ones come back, where, and what model;
//   2. the nearest pinned tree in front is made invisible and collision-less through that handle for 6 s - say whether it
//      vanished;
//   3. GET_CLOSEST_OBJECT_OF_TYPE is asked too (does it ever see map trees?).
// Everything goes to TornadoRedemption_findings.txt (MAPTREE lines).
struct TreePin { int pin; int idx; };
static std::vector<TreePin> g_treePins;
static Entity g_treeChecked = 0;

static void MapTreeCheck()
{
	if (!g_treePins.empty() || g_treeChecked) { Notify("The map tree check is still running"); return; }
	V3 pp = PlayerPos();
	g_treePins.clear();
	int loaded = 0, n = (int)(sizeof(kTreeModels) / sizeof(kTreeModels[0]));
	for (int i = 0; i < n; i++)
	{
		if (!STREAMING::HAS_MODEL_LOADED(kTreeModels[i].hash)) continue;
		loaded++;
		int pin = ENTITY::PIN_CLOSEST_MAP_ENTITY(kTreeModels[i].hash, pp.x, pp.y, pp.z, 9);
		if (pin) g_treePins.push_back({ pin, i });
		Object o = OBJECT::GET_CLOSEST_OBJECT_OF_TYPE(pp.x, pp.y, pp.z, 30.0f, kTreeModels[i].hash, FALSE, FALSE, FALSE);
		if (o) Finding("MAPTREE closest-object found %s (object %d) within 30 m", kTreeModels[i].name, o);
	}
	Finding("MAPTREE check at (%.0f, %.0f): %d tree models loaded here, %d pin requests made", pp.x, pp.y, loaded, (int)g_treePins.size());
	Notify("Map tree check: looking at the trees around you...", 3000);
	After(0.6f, []()
	{
		V3 pp = PlayerPos();
		V3 fwd = HeadingDir(CameraHeading());
		Entity best = 0;
		float bestScore = 1e9f;
		int pinned = 0;
		for (auto& tpn : g_treePins)
		{
			if (!ENTITY::IS_MAP_ENTITY_PINNED(tpn.pin)) continue;
			pinned++;
			Entity e = ENTITY::GET_PINNED_MAP_ENTITY(tpn.pin);
			bool exists = e && ENTITY::DOES_ENTITY_EXIST(e);
			V3 c = exists ? V3(ENTITY::GET_ENTITY_COORDS(e, FALSE, FALSE)) : V3();
			V3 d = c - pp;
			float dist = d.len2d(), ahead = d.x * fwd.x + d.y * fwd.y;
			Finding("MAPTREE pinned %s -> entity %d exists %d type %d model 0x%08X at %.1f m (%.1f m ahead)", kTreeModels[tpn.idx].name, e, (int)exists,
				exists ? ENTITY::GET_ENTITY_TYPE(e) : -1, exists ? ENTITY::GET_ENTITY_MODEL(e) : 0, dist, ahead);
			float score = dist - ahead * 0.5f;
			if (exists && ahead > 1.0f && dist < 30.0f && score < bestScore) { bestScore = score; best = e; }
		}
		Finding("MAPTREE %d of %d pins came back", pinned, (int)g_treePins.size());
		if (!best)
		{
			// v1.1 audit 2: done - let the pins go (they were kept, so the check said "still running" for ever)
			for (auto& tpn : g_treePins) ENTITY::UNPIN_MAP_ENTITY(tpn.pin);
			g_treePins.clear();
			Notify("Map tree check: no pinned tree in front of you (logged)", 6000);
			return;
		}
		g_treeChecked = best;
		ENTITY::SET_ENTITY_VISIBLE(best, FALSE);
		ENTITY::SET_ENTITY_COLLISION(best, FALSE, FALSE);
		Notify("Map tree check: did the tree in front of you VANISH? Say it out loud (back in 6 s)", 6000);
		Finding("MAPTREE hid entity %d through its pin (visible off, collision off) - the user's answer is on the recording", best);
		After(6.0f, []()
		{
			if (g_treeChecked && ENTITY::DOES_ENTITY_EXIST(g_treeChecked))
			{
				ENTITY::SET_ENTITY_VISIBLE(g_treeChecked, TRUE);
				ENTITY::SET_ENTITY_COLLISION(g_treeChecked, TRUE, TRUE);
			}
			for (auto& tpn : g_treePins) ENTITY::UNPIN_MAP_ENTITY(tpn.pin);
			g_treePins.clear();
			g_treeChecked = 0;
			Notify("Map tree check done - logged in TornadoRedemption_findings.txt", 4000);
		});
	});
}

// ======================= auto tests =======================
struct AutoStep { float duration; std::string desc; std::function<void()> start; };
struct AutoTest
{
	std::string name;
	std::vector<AutoStep> steps;
	int idx = -1;
	float stepEnds = 0;
	bool running = false;
	std::function<void()> onEnd;
} g_auto;

static void AutoStop(const char* why)
{
	if (!g_auto.running) return;
	g_auto.running = false;
	if (g_auto.onEnd) g_auto.onEnd();
	Log("AUTOTEST '%s' %s", g_auto.name.c_str(), why);
	Notify("Auto test " + std::string(why));
}

static void AutoRun(const std::string& name, std::vector<AutoStep> steps, std::function<void()> onEnd = nullptr)
{
	// Safety net only - every Auto* entry point already called AutoStop("replaced") before capturing its settings.
	AutoStop("replaced");
	DespawnAll();
	g_auto = AutoTest();
	g_auto.name = name;
	g_auto.steps = std::move(steps);
	g_auto.onEnd = onEnd;
	g_auto.running = true;
	Log("AUTOTEST '%s' started (%d steps)", name.c_str(), (int)g_auto.steps.size());
}

static bool g_autoSkip = false;   // v0.5: next-step key (style tour etc.)
static void AutoUpdate(float t)
{
	if (g_autoSkip && g_auto.running)
	{
		Log("AUTOTEST step %d skipped by the next-step key", g_auto.idx + 1);
		g_auto.stepEnds = t;
	}
	g_autoSkip = false;
	if (!g_auto.running || t < g_auto.stepEnds)
		return;
	for (auto& tp : g_tornadoes) tp->Snapshot("AUTOTEST-END");
	g_auto.idx++;
	if (g_auto.idx >= (int)g_auto.steps.size())
	{
		AutoStop("finished - results in TornadoRedemption.log");
		return;
	}
	AutoStep& s = g_auto.steps[g_auto.idx];
	Log("AUTOTEST step %d/%d: %s", g_auto.idx + 1, (int)g_auto.steps.size(), s.desc.c_str());
	s.start();
	g_auto.stepEnds = NowSec() + s.duration;
}

static std::string AutoStatus()
{
	if (!g_auto.running || g_auto.idx < 0 || g_auto.idx >= (int)g_auto.steps.size()) return "";
	char b[220];
	sprintf_s(b, "TEST %d/%d: %s  (%.0fs)   %s = note", g_auto.idx + 1, (int)g_auto.steps.size(),
		g_auto.steps[g_auto.idx].desc.c_str(), std::max(0.0f, g_auto.stepEnds - NowSec()), g_keys.bookmark.text.c_str());
	return b;
}

static void AutoStyleTour()
{
	AutoStop("replaced");
	std::vector<AutoStep> steps;
	steps.push_back({ 5.0f, "storm rolling in - the weather stays locked for the whole tour", []() { g_holdStorm = true; } });
	// v0.5, playtest 5: "It just says S supercell. I don't know what styles we're observing" / "wait, four? how did I
	// miss so many styles" (the counter counted steps, not styles) / "it is gradual... I get a little overwhelmed".
	// Each style now gets 25 s with its name and number on the card, and the next-step key moves on early.
	int nStyles = (int)GetStyles().size() - 1;
	for (int i = 0; i < nStyles; i++)
	{
		char desc[200];
		sprintf_s(desc, "Style %d of %d:  %s  -  score it out of 10   (%s / pad %s = next)", i + 1, nStyles, GetStyles()[i].name,
			g_keys.next.text.c_str(), g_pad.next.text.c_str());
		steps.push_back({ 25.0f, desc, [i]() { DespawnAll(); SpawnTornado(i, 110.0f, 0, true, GetStyles()[i].name); } });
		steps.push_back({ 2.0f, "next style", []() { DespawnAll(); } });
	}
	AutoRun("style tour", steps, []() { DespawnAll(); g_holdStorm = false; });
}

static void AutoStrengthTour()
{
	AutoStop("replaced");
	static const char* kForceNames[] = { "Gentle", "Violent", "Extreme" };
	int style = g_set.style, savedMove = g_set.movement, savedForce = g_set.force;
	std::vector<AutoStep> steps;
	steps.push_back({ 3.0f, "storm rolling in", []() { g_holdStorm = true; } });
	for (int f = 0; f < 3; f++)
	{
		steps.push_back({ 25.0f, std::string(kForceNames[f]) + " - coming at you", [style, f]()
		{
			DespawnAll();
			g_set.force = f;
			g_set.movement = 2;
			SpawnTornado(style, 110.0f, 0, false, GetStyles()[style].name);
		} });
		steps.push_back({ 3.0f, "clearing", []() { DespawnAll(); } });
	}
	AutoRun("strength tour", steps, [savedMove, savedForce]() { DespawnAll(); g_set.movement = savedMove; g_set.force = savedForce; g_holdStorm = false; });
}

static void AutoRenderCheck()
{
	AutoStop("replaced");
	struct V { int variant; float dist; };
	std::vector<V> plan;
	for (int i = 0; i < kRigVariants; i++) plan.push_back({ i, 70.0f });
	plan.push_back({ 1, 190.0f }); plan.push_back({ 2, 190.0f }); plan.push_back({ 6, 190.0f }); plan.push_back({ 7, 190.0f });
	std::vector<AutoStep> steps;
	steps.push_back({ 6.0f, "storm rolling in - then just watch and SAY what you see", []()
	{
		g_holdStorm = true;
		g_lab.slideshow = false;
		LabStop();
		Finding("RENDER CHECK start %s | anchor LOD default %d, the mod %d | answers are in the recording", kVersion, g_anchorLodDefault, g_anchorLod);
	} });
	int total = (int)plan.size();
	for (int k = 0; k < total; k++)
	{
		V v = plan[k];
		char desc[220];
		sprintf_s(desc, "%d/%d  %s, %.0f m  -  SOLID, FLICKERS or NOTHING?", k + 1, total, kRigNames[v.variant], v.dist);
		steps.push_back({ 10.0f, desc, [v]() { RigStart(v.variant, v.dist); } });
		steps.push_back({ 1.5f, "clearing", []() { RigStop(); } });
	}
	steps.push_back({ 0.5f, "done", []() { Finding("RENDER CHECK done (see the recording's transcript for the answers)"); } });
	AutoRun("render check", steps, []() { RigStop(); g_holdStorm = false; });
}

// Controller check: shows which buttons reach the mod and through which source.
static bool g_padCheck = false;
static std::string g_padSeen;
static void AutoPadCheck()
{
	AutoStop("replaced");
	std::vector<AutoStep> steps;
	steps.push_back({ 20.0f, "press A, B, X, Y, the D-pad, LB and RB one at a time", []() { g_padCheck = true; g_padSeen.clear(); } });
	AutoRun("controller check", steps, []()
	{
		g_padCheck = false;
		Finding("PAD CHECK: source %s | buttons seen:%s", PadSource(), g_padSeen.empty() ? " none" : g_padSeen.c_str());
	});
}

// ======================= self-test (dev tool) =======================
// One button, about a minute: spawns, checks, uproots, dissipates and stress-spawns tornadoes, and logs PASS/FAIL for
// each check to TornadoRedemption.log + TornadoRedemption_findings.txt. Meant to catch leaks and regressions before a real playtest.
struct SelfTest
{
	int pass = 0, fail = 0, skip = 0;
	int baseLoops = 0, baseAnchors = 0, baseObjs = 0;
	bool savedDebris = true, savedTouchdown = true, savedHoverFix = true;
	int savedRender = 2, savedEngine = 0;
	Object injected = 0;
	int healedBefore = 0;
	V3 labPos;
	Object tree = 0;
	float treeZ = 0;
	int uprootedBefore = 0;
	Object floater = 0;
	int wokenBefore = 0;
	int killedFx = 0;             // v0.5: world-space fault injection
	int bHealedBefore = 0;
	Object rester = 0;            // v0.5: a barrel lying on the ground must not be touched
	V3 resterPos;
	float resterOldHag = 0;
	std::vector<Entity> anchors;   // handles captured BEFORE cleanup, verified absent AFTER (Astra)
	std::vector<Entity> orbiters;  // v0.6: the debris cone's props, deleted on despawn
	std::vector<int> fxs;
} g_st;

static void Skip(const char* what, const char* why)
{
	g_st.skip++;
	Finding("SELFTEST SKIP  %-44s %s", what, why);
}

static void Info(const char* fmt, ...)
{
	char b[256] = {};
	va_list args;
	va_start(args, fmt);
	vsnprintf_s(b, sizeof(b), _TRUNCATE, fmt, args);
	va_end(args);
	Finding("SELFTEST info  %s", b);
}

static Tornado* FindTornado(const char* label)
{
	for (auto& tp : g_tornadoes)
		if (tp->label == label) return tp.get();
	return nullptr;
}

static void Check(const char* what, bool ok, const char* detailFmt = "", ...)
{
	char detail[256] = {};
	va_list args;
	va_start(args, detailFmt);
	vsnprintf_s(detail, sizeof(detail), _TRUNCATE, detailFmt, args);
	va_end(args);
	(ok ? g_st.pass : g_st.fail)++;
	Finding("SELFTEST %s  %-44s %s", ok ? "PASS" : "FAIL", what, detail);
}

static int CountObjects()
{
	static int objs[8192];
	return worldGetAllObjects(objs, 8192);
}

static void AutoSelfTest()
{
	AutoStop("replaced");   // Astra: end any running test (and its restore) BEFORE this one captures settings
	std::vector<AutoStep> steps;
	steps.push_back({ 0.5f, "baseline", []()
	{
		g_st = SelfTest();
		g_st.savedDebris = g_set.extraDebris; g_st.savedRender = g_set.render; g_st.savedTouchdown = g_set.touchdown;
		g_st.savedEngine = g_set.engine; g_st.savedHoverFix = g_set.hoverFix;
		g_set.extraDebris = false;   // debris is left behind as wreckage on purpose; keep it out of the leak counts
		g_set.render = 2;            // Astra: "Puffs only" made the loop checks pass as 0 == 0 - force both
		g_set.engine = 1;            // v0.5: tornado A runs the default world-space engine; B runs the legacy anchors
		g_set.touchdown = true;
		g_set.hoverFix = true;
		g_lab.slideshow = false;
		LabStop();                   // the FX Lab is only used in its own controlled step below
		RigStop();
		g_st.baseLoops = LoopsInUse();
		g_st.baseAnchors = AnchorCount();
		g_st.baseObjs = CountObjects();
		Finding("SELFTEST start %s (built %s %s) | module dir %s | game version enum %d | loops %d anchors %d orphans %d objects %d",
			kVersion, __DATE__, __TIME__, ModuleDir().c_str(), (int)getGameVersion(), g_st.baseLoops, g_st.baseAnchors, OrphanCount(), g_st.baseObjs);
		Info("controller source: %s | HUD %s", PadSource(), g_set.hud == 0 ? "Full" : g_set.hud == 1 ? "Tracker" : "Off");
		Check("particle library 'core' loads", LoadPtfxAsset("core"));
		Check("anchor model p_apple01x loads", LoadModel(H("p_apple01x")));
		Check("tree model for the uproot check loads", LoadModel(H(kDefaultReplacementTrees[0]), 2000));
		Check("no orphan anchors at start", OrphanCount() == 0, "(%d)", OrphanCount());
	} });
	steps.push_back({ 9.0f, "tornado A (world-space, the default engine): touchdown 60 m ahead", []()
	{
		g_set.engine = 1;
		SpawnTornado(0, 60.0f, 0, true, "SELFTEST-A");
	} });
	steps.push_back({ 0.5f, "checking tornado A", []()
	{
		Tornado* a = FindTornado("SELFTEST-A");
		Check("tornado A exists", a != nullptr);
		if (!a) return;
		Check("A planned a positive number of looped effects", a->loopsPlanned > 0, "(planned %d)", a->loopsPlanned);
		Check("A started every planned looped effect", a->loopsPlanned > 0 && a->loopsFailed == 0 && a->loopsRunning == a->loopsPlanned,
			"(%d/%d, %d refused)", a->loopsRunning, a->loopsPlanned, a->loopsFailed);
		Check("A's effect handles still exist after 9 s", a->loopsRunning > 0 && a->LoopsAlive() == a->loopsRunning, "(%d/%d)", a->LoopsAlive(), a->loopsRunning);
		std::vector<Entity> anchors; std::vector<int> fxs;
		a->CaptureHandles(anchors, fxs);
		Check("A is world-space: no anchor props at all", anchors.empty() && AnchorCount() == g_st.baseAnchors, "(%d anchors, registry %d vs %d)",
			(int)anchors.size(), AnchorCount(), g_st.baseAnchors);
		Info("handle alive != visibly renders: judge the footage for looks (the Finish-line run asks you out loud)");
		Check("A touched down", a->TouchedDown(), "(growth %.2f)", a->growth);
		if (g_set.debrisCone)
			Check("A carries debris round its cone", a->OrbiterCount() > 0 && a->OrbitersAlive() == a->OrbiterCount(), "(%d props, %d alive)", a->OrbiterCount(), a->OrbitersAlive());
		else
			Skip("debris cone", "switched off");
		float gz = GroundZ(a->base.x, a->base.y, a->base.z + 60.0f, -9999.0f);
		Check("A's base sits on the ground", gz > -9000 && fabsf(a->base.z - gz) < 3.0f, "(base %.1f ground %.1f)", a->base.z, gz);
		Check("A's ground tracking ok", a->groundOk);
		Check("budget accounting matches A", LoopsInUse() == g_st.baseLoops + a->loopsRunning, "(in use %d)", LoopsInUse());
		if (g_set.weather)
		{
			Hash w1 = 0, w2 = 0; float pct = 0;
			MISC::GET_CURR_WEATHER_STATE(&w1, &w2, &pct);
			Hash want = H(kWeatherTypes[g_set.weatherType]);
			Check("storm weather transition requested", w1 == want || w2 == want, "(0x%08X -> 0x%08X at %.2f; a sample, not proof it holds)", w1, w2, pct);
		}
		else
			Skip("storm weather", "weather is switched off");
		// Fault injection (world-space): kill one of A's looped effects behind its back; healing must restart it.
		if (!fxs.empty())
		{
			g_st.killedFx = fxs[0];
			g_st.healedBefore = a->healed;
			GRAPHICS::STOP_PARTICLE_FX_LOOPED(fxs[0], FALSE);
			GRAPHICS::REMOVE_PARTICLE_FX(fxs[0], FALSE);
			Info("fault injection: removed A's looped effect %d", g_st.killedFx);
		}
	} });
	steps.push_back({ 3.5f, "fault injection: waiting for A to heal", []() {} });
	steps.push_back({ 0.5f, "checking the heal", []()
	{
		Tornado* a = FindTornado("SELFTEST-A");
		if (!a || !g_st.killedFx) { Skip("self-heal (world-space)", "no effect to remove"); return; }
		Check("A restarted the removed effect", a->healed > g_st.healedBefore, "(healed %d -> %d, heal-fail %d)", g_st.healedBefore, a->healed, a->healFailed);
		Check("A's effects all alive again", a->LoopsAlive() == a->loopsRunning, "(%d/%d)", a->LoopsAlive(), a->loopsRunning);
	} });
	// v0.4 regression test for playtest 4's "uprooted 0": plant a tree between 1.3x and 1.6x the wall, where the
	// "rip fixed props loose" rule used to claim it first.
	steps.push_back({ 3.5f, "a tree planted at 1.5x A's wall: it should be torn out and carried up", []()
	{
		Tornado* a = FindTornado("SELFTEST-A");
		if (!a) return;
		g_st.uprootedBefore = a->uprooted;
		V3 p = a->base + V3(a->wallRadius() * 1.5f, 0, 0);
		p.z = GroundZ(p.x, p.y, p.z + 40.0f, p.z);
		g_st.tree = PlantTree(kDefaultReplacementTrees[0], p);
		if (g_st.tree)
			g_st.treeZ = V3(ENTITY::GET_ENTITY_COORDS(g_st.tree, FALSE, FALSE)).z;
	} });
	steps.push_back({ 0.5f, "checking the uproot", []()
	{
		Tornado* a = FindTornado("SELFTEST-A");
		if (!a || !g_st.tree) { Skip("scripted uproot", "the test tree did not spawn"); return; }
		Check("planted tree inside the rip ring was uprooted", a->uprooted > g_st.uprootedBefore, "(uprooted %d -> %d)", g_st.uprootedBefore, a->uprooted);
		bool exists = ENTITY::DOES_ENTITY_EXIST(g_st.tree) != 0;
		float rise = exists ? V3(ENTITY::GET_ENTITY_COORDS(g_st.tree, FALSE, FALSE)).z - g_st.treeZ : 0.0f;
		Check("the uprooted tree left the ground", exists && rise > 2.0f, "(rose %.1f m, flights %d)", rise, FlightsActive());
	} });
	steps.push_back({ 4.0f, "tornado B (legacy anchors) + an FX Lab anchor inside A's wall", []()
	{
		Tornado* a = FindTornado("SELFTEST-A");
		g_set.engine = 0;
		SpawnTornado(3, 70.0f, 45.0f, true, "SELFTEST-B");
		g_set.engine = 1;
		if (!a) return;
		g_lab.kind = 0; g_lab.layout = 0; g_lab.idx = 0;
		g_lab.origin = a->base + V3(a->wallRadius() * 0.9f, 0, 0);
		LabStart(true);
		if (!g_lab.anchors.empty() && g_lab.anchors[0])
			g_st.labPos = ENTITY::GET_ENTITY_COORDS(g_lab.anchors[0], FALSE, FALSE);
	} });
	steps.push_back({ 0.5f, "checking anchor exclusion; deleting one of B's anchors", []()
	{
		if (g_lab.anchors.empty() || !g_lab.anchors[0]) { Skip("FX Lab anchor exclusion", "Lab anchor did not spawn"); }
		else
		{
			V3 now = ENTITY::GET_ENTITY_COORDS(g_lab.anchors[0], FALSE, FALSE);
			Check("FX Lab anchor inside A's wall was not moved", (now - g_st.labPos).len() < 0.5f, "(moved %.2f m)", (now - g_st.labPos).len());
		}
		Tornado* b = FindTornado("SELFTEST-B");
		Check("tornado B exists alongside A", b != nullptr);
		if (!b) return;
		std::vector<Entity> anchors; std::vector<int> fxs;
		b->CaptureHandles(anchors, fxs);
		Check("B (legacy) carries its smoke on anchor props", !anchors.empty(), "(%d anchors)", (int)anchors.size());
		if (!anchors.empty())
		{
			Object victim = anchors[0];
			g_st.injected = victim;
			g_st.bHealedBefore = b->healed;
			DeleteObj(victim);
			Info("fault injection: deleted B's anchor %d", g_st.injected);
		}
	} });
	steps.push_back({ 3.5f, "fault injection: waiting for B to re-create its anchor", []() {} });
	steps.push_back({ 0.5f, "checking B's heal, then both die down", []()
	{
		LabStop();
		Tornado* b = FindTornado("SELFTEST-B");
		if (!b || !g_st.injected) Skip("self-heal (anchors)", "no anchor to inject");
		else
		{
			Check("B healed the deleted anchor", b->healed > g_st.bHealedBefore, "(healed %d -> %d, heal-fail %d)", g_st.bHealedBefore, b->healed, b->healFailed);
			Check("B's effects all alive again", b->LoopsAlive() == b->loopsRunning, "(%d/%d)", b->LoopsAlive(), b->loopsRunning);
		}
		// Capture every handle BEFORE cleanup, then dissipate both.
		g_st.anchors.clear(); g_st.fxs.clear();
		for (auto& tp : g_tornadoes) { tp->CaptureHandles(g_st.anchors, g_st.fxs); tp->BeginDissipate(NowSec()); }
		Info("captured %d anchors / %d effects before dissipating A and B", (int)g_st.anchors.size(), (int)g_st.fxs.size());
	} });
	steps.push_back({ 7.5f, "A and B dying down (should fade as they lift)", []() {} });
	steps.push_back({ 1.5f, "checking cleanup", []()
	{
		Check("dissipated tornadoes removed", g_tornadoes.empty(), "(%d left)", (int)g_tornadoes.size());
		int anchorsLeft = 0, fxLeft = 0;
		for (Entity e : g_st.anchors) if (ENTITY::DOES_ENTITY_EXIST(e)) anchorsLeft++;
		for (int f : g_st.fxs) if (GRAPHICS::DOES_PARTICLE_FX_LOOPED_EXIST(f)) fxLeft++;
		Check("every captured anchor is gone (checked by handle)", anchorsLeft == 0, "(%d of %d still exist)", anchorsLeft, (int)g_st.anchors.size());
		Check("every captured effect is gone (checked by handle)", fxLeft == 0, "(%d of %d still exist)", fxLeft, (int)g_st.fxs.size());
		Check("loop budget back to baseline", LoopsInUse() == g_st.baseLoops, "(%d vs %d)", LoopsInUse(), g_st.baseLoops);
		Check("no orphan anchors", OrphanCount() == 0, "(%d)", OrphanCount());
		g_st.anchors.clear(); g_st.fxs.clear();
	} });
	// Sweeper: a barrel hanging 6 m up must be woken and drop; a barrel lying on the ground must be left exactly where
	// it is (v0.5 regression test for playtest 5's 836 teleported props).
	steps.push_back({ 5.0f, "sweeper: a barrel the tornado left 6 m up (should come down), one on the ground (must not move)", []()
	{
		Hash model = H("p_barrel02x");
		if (!LoadModel(model, 1000)) return;
		V3 p = PointAhead(-10.0f);
		g_st.floater = OBJECT::CREATE_OBJECT(model, p.x, p.y, p.z, FALSE, FALSE, TRUE, FALSE, FALSE);
		if (g_st.floater)
		{
			NoteTouched(g_st.floater, NowSec() - 2.0f);           // the tornado found it on the ground 2 s ago...
			ENTITY::SET_ENTITY_COORDS_NO_OFFSET(g_st.floater, p.x, p.y, p.z + 6.0f, FALSE, FALSE, FALSE);
			ENTITY::FREEZE_ENTITY_POSITION(g_st.floater, TRUE);   // ...and left it hanging 6 m up (what playtest 6 saw)
		}
		V3 q = PointAhead(-10.0f, 4.0f);
		g_st.rester = OBJECT::CREATE_OBJECT(model, q.x, q.y, q.z, FALSE, FALSE, TRUE, FALSE, FALSE);
		if (g_st.rester)
		{
			OBJECT::PLACE_OBJECT_ON_GROUND_PROPERLY(g_st.rester, FALSE);
			ENTITY::FREEZE_ENTITY_POSITION(g_st.rester, TRUE);
			g_st.resterPos = ENTITY::GET_ENTITY_COORDS(g_st.rester, FALSE, FALSE);
			g_st.resterOldHag = ENTITY::GET_ENTITY_HEIGHT_ABOVE_GROUND(g_st.rester);
			NoteTouched(g_st.rester, NowSec() - 2.0f);
		}
		g_st.wokenBefore = HoverWoken();
	} });
	steps.push_back({ 0.5f, "checking the two barrels", []()
	{
		if (!g_st.floater || !ENTITY::DOES_ENTITY_EXIST(g_st.floater)) Skip("floating-prop sweeper", "the test barrel did not spawn");
		else
		{
			float h = 99;
			bool known = TrueHeight(ENTITY::GET_ENTITY_COORDS(g_st.floater, FALSE, FALSE), &h);
			Check("the sweeper noticed the floating barrel", HoverWoken() > g_st.wokenBefore, "(woken %d -> %d)", g_st.wokenBefore, HoverWoken());
			Check("the floating barrel came down", known && h < 1.5f, "(%.1f m above the ground now, brought down by gliding %d)", h, HoverGlided());
			DeleteObj(g_st.floater);
		}
		if (!g_st.rester || !ENTITY::DOES_ENTITY_EXIST(g_st.rester)) Skip("resting prop left alone", "the second barrel did not spawn");
		else
		{
			V3 now = ENTITY::GET_ENTITY_COORDS(g_st.rester, FALSE, FALSE);
			float h = 99;
			TrueHeight(now, &h);
			Check("a barrel resting on the ground was not moved", (now - g_st.resterPos).len() < 0.3f, "(moved %.2f m)", (now - g_st.resterPos).len());
			Info("resting barrel: real height %.2f m, GET_ENTITY_HEIGHT_ABOVE_GROUND said %.1f m (v0.4 trusted that number)", h, g_st.resterOldHag);
			DeleteObj(g_st.rester);
		}
	} });
	for (int i = 0; i < 6; i++)
		steps.push_back({ 1.2f, "stress: spawn + despawn " + std::to_string(i + 1) + "/6 (both engines)", [i]()
		{
			for (auto& tp : g_tornadoes) { tp->CaptureHandles(g_st.anchors, g_st.fxs); tp->CaptureOrbiters(g_st.orbiters); }
			DespawnAll();
			g_set.engine = i % 2;
			SpawnTornado(rand() % ((int)GetStyles().size() - 1), 80.0f, RandRange(-30.f, 30.f), true, "SELFTEST-STRESS");
			g_set.engine = 1;
		} });
	steps.push_back({ 1.5f, "stress cleanup", []()
	{
		for (auto& tp : g_tornadoes) { tp->CaptureHandles(g_st.anchors, g_st.fxs); tp->CaptureOrbiters(g_st.orbiters); }
		DespawnAll();
	} });
	steps.push_back({ 0.5f, "summary", []()
	{
		int orbLeft = 0;
		for (Entity e : g_st.orbiters) if (ENTITY::DOES_ENTITY_EXIST(e)) orbLeft++;
		Check("stress: every debris-cone prop deleted on despawn", orbLeft == 0, "(%d of %d still exist)", orbLeft, (int)g_st.orbiters.size());
		int anchorsLeft = 0, fxLeft = 0;
		for (Entity e : g_st.anchors) if (ENTITY::DOES_ENTITY_EXIST(e)) anchorsLeft++;
		for (int f : g_st.fxs) if (GRAPHICS::DOES_PARTICLE_FX_LOOPED_EXIST(f)) fxLeft++;
		Check("stress: every captured anchor gone", anchorsLeft == 0, "(%d of %d still exist)", anchorsLeft, (int)g_st.anchors.size());
		Check("stress: every captured effect gone", fxLeft == 0, "(%d of %d still exist)", fxLeft, (int)g_st.fxs.size());
		Check("stress: loop budget back to baseline", LoopsInUse() == g_st.baseLoops, "(%d vs %d)", LoopsInUse(), g_st.baseLoops);
		if (g_st.tree)
		{
			g_testTrees.erase(std::remove(g_testTrees.begin(), g_testTrees.end(), g_st.tree), g_testTrees.end());
			g_priority.erase(g_st.tree);
			DeleteObj(g_st.tree);
		}
		Info("world object count %d -> %d (telemetry only: things stream in/out)", g_st.baseObjs, CountObjects());
		Finding("SELFTEST done: %d passed, %d failed, %d skipped", g_st.pass, g_st.fail, g_st.skip);
		Notify("Self-test: " + std::to_string(g_st.pass) + " passed, " + std::to_string(g_st.fail) + " failed, " +
			std::to_string(g_st.skip) + " skipped (TornadoRedemption_findings.txt)", 9000);
	} });
	AutoRun("self-test", steps, []()
	{
		g_set.extraDebris = g_st.savedDebris; g_set.render = g_st.savedRender; g_set.touchdown = g_st.savedTouchdown;
		g_set.engine = g_st.savedEngine; g_set.hoverFix = g_st.savedHoverFix;
		LabStop();
	});
}

static void AddTreeScanSteps(std::vector<AutoStep>& steps)
{
	auto results = std::make_shared<std::vector<ScannedTree>>();
	auto tried = std::make_shared<int>(0);
	int n = (int)(sizeof(kTreeScan) / sizeof(kTreeScan[0]));
	for (int i = 0; i < n; i++)
	{
		char desc[128];
		sprintf_s(desc, "scanning tree models %d/%d", i + 1, n);
		steps.push_back({ 0.02f, desc, [i, results, tried]()
		{
			(*tried)++;
			Hash model = kTreeScan[i].hash;
			if (!STREAMING::IS_MODEL_IN_CDIMAGE(model) || !STREAMING::IS_MODEL_VALID(model))
				return;
			Object o = SpawnTreeModel(kTreeScan[i].name, PointAhead(30.0f), true, true);
			if (!o)
				return;
			Vector3 mn = {}, mx = {};
			MISC::GET_MODEL_DIMENSIONS(model, &mn, &mx);
			float h = mx.z - mn.z;
			DeleteObj(o);
			Finding("TREESCAN spawnable: %-40s height %5.1fm  width %4.1fm", kTreeScan[i].name, h, std::max(mx.x - mn.x, mx.y - mn.y));
			// v0.5: playtest 5's tree demo planted douglasfir_snow_05 - white snowy firs in a green meadow.
			if (h >= 5.0f && !strstr(kTreeScan[i].name, "snow"))
				results->push_back({ kTreeScan[i].name, h });
		} });
	}
	steps.push_back({ 0.1f, "scan summary", [results, tried]()
	{
		std::sort(results->begin(), results->end(), [](const ScannedTree& a, const ScannedTree& b) { return a.height > b.height; });
		g_spawnableTrees = *results;
		g_treeModel = 0;
		SaveTreeCache();
		Finding("TREESCAN done: %d models tried, %d spawnable trees taller than 5 m (tallest: %s)", *tried, (int)results->size(),
			results->empty() ? "none" : results->front().name.c_str());
		Notify("Tree scan: " + std::to_string(results->size()) + " full-size trees the game will spawn", 5000);
	} });
}

static void AutoTreeScan()
{
	AutoStop("replaced");
	std::vector<AutoStep> steps;
	AddTreeScanSteps(steps);
	AutoRun("tree scan", steps);
}

// One button: (scan if needed) -> plant the 3 tallest trees ahead in the tornado's path -> it comes straight through.
static void AutoTreeDemo()
{
	AutoStop("replaced");
	std::vector<AutoStep> steps;
	if (g_spawnableTrees.empty())
		AddTreeScanSteps(steps);
	int style = g_set.style, savedMove = g_set.movement, savedSpeed = g_set.speed;
	auto mid = std::make_shared<V3>();
	steps.push_back({ 2.0f, "planting trees 45 m ahead, right in its path", [mid]()
	{
		DeleteTestTrees(); PlantTreesAhead(3, 45.0f, 6.0f); g_holdStorm = true;
		*mid = PointAheadCam(45.0f);
	} });
	// v0.5 (playtest 5: "uprooted 0" - the demo tornado chased Arthur, not the trees): it now travels in a straight
	// line aimed at the middle tree, at normal speed, and the log says how close it got to each tree.
	steps.push_back({ 40.0f, "tornado coming straight through the trees - watch them get torn out", [style, mid]()
	{
		g_set.movement = 3;
		g_set.speed = 1;
		Tornado* tp = SpawnTornado(style, 120.0f, 0, false, GetStyles()[style].name, -1, 0.0f);
		if (tp)
		{
			V3 d = *mid - tp->base;
			tp->heading = atan2f(-d.x, d.y) * 180.0f / PI;
			Log("tree demo: tornado aimed at the middle tree, %.0f m away (heading %.0f)", d.len2d(), tp->heading);
		}
	} });
	steps.push_back({ 0.5f, "tree demo result", [savedSpeed]()
	{
		int up = 0;
		for (auto& tp : g_tornadoes) up += tp->uprooted;
		Finding("TREE DEMO: %d of 3 planted trees uprooted", up);
		Notify("Tree demo: " + std::to_string(up) + " of 3 trees torn out", 5000);
		(void)savedSpeed;
	} });
	AutoRun("tree demo", steps, [savedMove, savedSpeed]() { DespawnAll(); g_set.movement = savedMove; g_set.speed = savedSpeed; g_holdStorm = false; });
}

// ======================= v0.6: touchdown camera =======================
// Playtest 6 (by accident, with the drone left on Orbit): "so it did a camera movement to show you where it spawned. I
// like that... it's kind of cinematic... it could turn the camera from wherever you're at and show you where the tornado
// is spawning, so that could be a feature we have in the menu". Now it is: a camera just behind Arthur swings to the
// touchdown, follows it reaching down from the cloud for ~7 s, then hands back.
struct TouchCam { int cam = 0; bool on = false; float until = 0; TornadoRef tp; } g_tc;

static bool TornadoAlive(Tornado* t)
{
	for (auto& p : g_tornadoes) if (p.get() == t) return true;
	return false;
}

static void TouchCamStop(bool ease)
{
	if (!g_tc.on) return;
	if (g_tc.cam && CAMERA::DOES_CAM_EXIST(g_tc.cam))
	{
		CAMERA::SET_CAM_ACTIVE(g_tc.cam, FALSE);
		CAMERA::RENDER_SCRIPT_CAMS(FALSE, ease, ease ? 900 : 0, TRUE, FALSE, 0);
		CAMERA::DESTROY_CAM(g_tc.cam, FALSE);
	}
	g_tc = TouchCam();
	Log("touchdown cam off");
}

static void TouchCamStart(Tornado* tp)
{
	if (!g_set.touchdownCam || !tp || g_drone.active || g_drone.mode || g_tc.on)
		return;
	V3 pp = PlayerPos();
	V3 d = tp->base - pp;
	float L = std::max(d.len2d(), 1.0f);
	V3 dir(d.x / L, d.y / L, 0), side(-dir.y, dir.x, 0);
	V3 pos = pp - dir * 4.5f + side * 1.6f + V3(0, 0, 2.4f);
	int cam = CAMERA::CREATE_CAM_WITH_PARAMS("DEFAULT_SCRIPTED_CAMERA", pos.x, pos.y, pos.z, 0, 0, 0, 42.0f, FALSE, 2);
	if (!cam)
	{
		Log("touchdown cam: CREATE_CAM_WITH_PARAMS failed");
		return;
	}
	V3 f = tp->base + V3(0, 0, tp->height() * 0.5f);
	CAMERA::POINT_CAM_AT_COORD(cam, f.x, f.y, f.z);
	CAMERA::SET_CAM_ACTIVE(cam, TRUE);
	CAMERA::RENDER_SCRIPT_CAMS(TRUE, TRUE, 900, TRUE, FALSE, 0);
	g_tc.cam = cam; g_tc.on = true; g_tc.until = NowSec() + 7.5f; g_tc.tp = TornadoRef(tp);
	Log("touchdown cam on (%.0f m out)", L);
}

static void TouchCamUpdate(bool blocked)
{
	if (!g_tc.on)
		return;
	if (blocked || NowSec() > g_tc.until || !g_tc.tp.get() || g_drone.mode)
	{
		TouchCamStop(!blocked);
		return;
	}
	Tornado* tp = g_tc.tp.get();
	// follows the funnel as it reaches down: from the cloud base to the middle of the funnel
	V3 f = tp->base + V3(0, 0, tp->height() * (0.75f - 0.4f * tp->growth));
	CAMERA::POINT_CAM_AT_COORD(g_tc.cam, f.x, f.y, f.z);
}

// ======================= v0.7: ride camera =======================
// Playtest 7 (watching the drone orbit): "it'd be cool if we could put this like default into the cinematic camera
// whenever you're sucked up by the tornado... change it to like this so you could see this for like a second and then it
// gives you back. You won't have control anyway". When a tornado lifts Arthur more than 6 m, the camera pulls out to orbit
// him for 5 s, then hands back (at most once every 25 s).
// v1.5 (the user: "maybe the occasional rotating cam when the twister sucks you up, far away wide orbital... let the player
// control it"): about every other ride is a WIDE one instead, yours to steer: look (mouse / right stick) swings it round,
// move forward/back (W/S / left stick) zooms.
// v1.6 (playtest 14: "a little too wide at times, I can't track or see Arthur... less dramatic, can still see Arthur, natural
// transition to wide angle to see the tornado and situation a little and then back, and make sure it disables that camera
// when the tornado throws arthur"): the wide one is built round HIM now - it starts about where the game's camera is, eases out
// to 22-38 m (the funnel behind him, Arthur in the middle of the frame), holds, eases back in and hands back (7 s). Either
// camera hands back the moment the tornado throws him.
struct RideCam { int cam = 0; bool on = false, wide = false; float until = 0, next = 0, ang = 0, elev = 0.35f, dist = 1.0f, low = 0; int rides = 0; TornadoRef tp; float t0 = 0, out = 0; int throws0 = 0; int throwsSeen = -1; float throwAt = -100, baseAng = 0; } g_ride;

static void RideCamStop(bool ease)
{
	if (!g_ride.on) return;
	if (g_ride.cam && CAMERA::DOES_CAM_EXIST(g_ride.cam))
	{
		CAMERA::SET_CAM_ACTIVE(g_ride.cam, FALSE);
		CAMERA::RENDER_SCRIPT_CAMS(FALSE, ease, ease ? 800 : 0, TRUE, FALSE, 0);
		CAMERA::DESTROY_CAM(g_ride.cam, FALSE);
	}
	g_ride.on = false;
	g_ride.cam = 0;
	Log("ride cam off");
}

static void RideCamUpdate(float dt, float t, bool blocked)
{
	// (v1.6 review: "held" is still true for a moment after a throw - no ride camera within 1.5 s of one)
	if (g_ride.throwsSeen < 0) g_ride.throwsSeen = ArthurThrows();
	if (ArthurThrows() != g_ride.throwsSeen) { g_ride.throwsSeen = ArthurThrows(); g_ride.throwAt = t; }
	if (g_ride.on)
	{
		if (blocked || t > g_ride.until || g_drone.mode || g_tc.on || PlayerDead()) { RideCamStop(!blocked); return; }
		if (ArthurThrows() != g_ride.throws0) { Log("ride cam: the tornado threw him - handing back"); RideCamStop(true); return; }   // v1.6
		V3 pp = PlayerPos();
		if (g_ride.wide)
		{
			Tornado* tp = g_ride.tp.get();   // (v1.5 review: the one it started on - not whichever is nearest this frame)
			if (!tp) { RideCamStop(true); return; }
			// back on the ground for a moment: the ride's over
			float h = 99.0f;
			if (TrueHeight(pp, &h) && h < 2.0f) g_ride.low += dt; else g_ride.low = 0;
			if (g_ride.low > 0.8f) { RideCamStop(true); return; }
			float lx = PAD::GET_DISABLED_CONTROL_NORMAL(0, Joaat("INPUT_LOOK_LR")), ly = PAD::GET_DISABLED_CONTROL_NORMAL(0, Joaat("INPUT_LOOK_UD"));
			float my = PAD::GET_DISABLED_CONTROL_NORMAL(0, Joaat("INPUT_MOVE_UD"));
			// (v1.5 review: a mouse's look is how far it moved this frame - not scaled by the frame time; a stick's is a rate)
			bool mouse = PAD::IS_USING_KEYBOARD_AND_MOUSE(0) != 0;
			g_ride.ang += dt * 0.10f - lx * (mouse ? 0.12f : dt * 2.4f);   // (round him, from the funnel's side: see below)
			g_ride.elev = Clamp(g_ride.elev + ly * (mouse ? 0.06f : dt * 1.2f), 0.05f, 0.9f);
			g_ride.dist = Clamp(g_ride.dist + my * dt * 0.9f, 0.6f, 1.5f);
			// out and back: about where the game's camera is (8 m), easing out to the wide one, then back in for the hand-back
			float age = t - g_ride.t0, life = g_ride.until - g_ride.t0;
			float k = Clamp(std::min(age / 1.8f, (life - age) / 1.6f), 0.0f, 1.0f);
			k = k * k * (3.0f - 2.0f * k);
			float wide = Clamp(tp->height() * 0.28f, 22.0f, 38.0f) * g_ride.dist;
			float R = Lerp(8.0f, wide, k);
			g_ride.out = R;
			// the angle is measured from the funnel's axis out through Arthur, so the camera stays outside him with the funnel
			// behind him as he goes round
			V3 rel = pp - tp->base;
			float want = atan2f(rel.y, rel.x), dA = want - g_ride.baseAng;
			while (dA > PI) dA -= 2 * PI;
			while (dA < -PI) dA += 2 * PI;
			g_ride.baseAng += dA * Clamp(dt * 4.0f, 0.0f, 1.0f);   // (v1.6 review: smoothed - he goes round fast)
			float a = g_ride.baseAng + g_ride.ang;
			V3 pos = pp + V3(cosf(a) * cosf(g_ride.elev), sinf(a) * cosf(g_ride.elev), sinf(g_ride.elev)) * R;
			float gz = GroundZ(pos.x, pos.y, pos.z + 50.0f, pos.z - 200.0f);
			if (pos.z < gz + 3.0f) pos.z = gz + 3.0f;
			V3 axis = tp->base + V3(0, 0, pp.z - tp->base.z);
			V3 look = pp + (axis - pp) * (0.2f * k);   // he stays in the middle; the funnel comes in beside him
			CAMERA::SET_CAM_COORD(g_ride.cam, pos.x, pos.y, pos.z);
			CAMERA::POINT_CAM_AT_COORD(g_ride.cam, look.x, look.y, look.z);
			return;
		}
		g_ride.ang += dt * 0.7f;
		V3 pos = pp + V3(cosf(g_ride.ang) * 20.0f, sinf(g_ride.ang) * 20.0f, 4.0f);
		CAMERA::SET_CAM_COORD(g_ride.cam, pos.x, pos.y, pos.z);
		CAMERA::POINT_CAM_AT_COORD(g_ride.cam, pp.x, pp.y, pp.z);
		return;
	}
	if (!g_set.rideCam || blocked || g_drone.mode || g_tc.on || t < g_ride.next || g_tornadoes.empty() || t - g_ride.throwAt < 1.5f)
		return;
	bool held = false;
	for (auto& tp : g_tornadoes)
		if (!tp->Mini() && t - tp->playerLastHeld < 0.3f) held = true;   // (v1.1 audit 2: a gun mini isn't a ride)
	if (!held)
		return;
	float h = 0;
	V3 pp = PlayerPos();
	if (!TrueHeight(pp, &h) || h < 6.0f)
		return;
	g_ride.rides++;
	g_ride.wide = (g_ride.rides % 2 == 0) || Rand01() < 0.25f;   // about every other ride is the wide one
	Tornado* rideT = NearestTornado(nullptr);
	if (!rideT) g_ride.wide = false;
	g_ride.tp = TornadoRef(rideT);
	g_ride.ang = RandRange(0, 2 * PI);
	V3 pos = pp + V3(cosf(g_ride.ang) * 20.0f, sinf(g_ride.ang) * 20.0f, 4.0f);
	if (g_ride.wide)
	{
		g_ride.elev = 0.25f; g_ride.dist = 1.0f; g_ride.low = 0;
		g_ride.ang = RandRange(-0.5f, 0.5f);   // (offset from straight out from the funnel through him)
		V3 d = pp - rideT->base;
		g_ride.baseAng = atan2f(d.y, d.x);
		float a = g_ride.baseAng + g_ride.ang;
		pos = pp + V3(cosf(a) * cosf(0.25f), sinf(a) * cosf(0.25f), sinf(0.25f)) * 8.0f;   // where the game's camera about is
	}
	int cam = CAMERA::CREATE_CAM_WITH_PARAMS("DEFAULT_SCRIPTED_CAMERA", pos.x, pos.y, pos.z, 0, 0, 0, g_ride.wide ? 50.0f : 55.0f, FALSE, 2);
	if (!cam) return;
	CAMERA::POINT_CAM_AT_COORD(cam, pp.x, pp.y, pp.z);
	CAMERA::SET_CAM_ACTIVE(cam, TRUE);
	CAMERA::RENDER_SCRIPT_CAMS(TRUE, TRUE, g_ride.wide ? 1100 : 700, TRUE, FALSE, 0);
	g_ride.cam = cam; g_ride.on = true; g_ride.until = t + (g_ride.wide ? 7.0f : 5.0f); g_ride.next = t + 25.0f;
	g_ride.t0 = t; g_ride.throws0 = ArthurThrows(); g_ride.out = 8.0f;
	if (g_ride.wide) UI::HelpTip("Wide view: look to swing the camera round him, move forward/back to zoom", 4.0f);
	Log("ride cam on (Arthur %.0f m up)%s", h, g_ride.wide ? " - the wide orbit (yours to steer)" : "");
}

// ======================= v1.5: mash to get up =======================
// The user: "when arthur gets knocked down by a twister, not yet grabbed fully, maybe you can quick-time spam A to get back on
// your feet". Down in a heap near a tornado (not in its hands): MASH A / SPACE TO GET UP. Each press fills the bar, which
// drains by itself; full, and he's back on his feet.
struct Mash { bool on = false; float fill = 0, since = 0, upAt = -100; int presses = 0; bool spaceWas = false; int ups = 0; } g_mash;

static void MashUpdate(float dt, float t)
{
	Ped me = PLAYER::PLAYER_PED_ID();
	bool space = (GetAsyncKeyState(VK_SPACE) & 0x8000) != 0;
	bool press = PadPressed(PB_A) || (space && !g_mash.spaceWas);
	g_mash.spaceWas = space;
	bool down = false;
	// (v1.5 review: not again for 1.5 s after getting up - he can still read as ragdolled while he gets up)
	if (!PlayerDead() && !IntroRunning() && !g_menuOpen && t - g_mash.upAt > 1.5f && PED::IS_PED_RAGDOLL(me) && !PED::IS_PED_ON_MOUNT(me) && t - PlayerLastInWall() > 0.6f)
	{
		float h = 9.0f;
		float nd = 0;
		Tornado* tp = NearestTornado(&nd);
		if (tp && nd < tp->reachRadius() * 1.5f + 15.0f && TrueHeight(PlayerPos(), &h) && h < 1.4f)
			down = true;
	}
	if (!down)
	{
		if (g_mash.on && g_mash.presses > 0) Log("mash: he got up by himself (%d presses)", g_mash.presses);
		g_mash.on = false; g_mash.fill = 0; g_mash.presses = 0;
		return;
	}
	if (!g_mash.on) { g_mash.on = true; g_mash.since = t; g_mash.fill = 0; g_mash.presses = 0; }
	if (t - g_mash.since < 0.4f) return;   // (let him land first)
	if (press) { g_mash.fill += 0.17f; g_mash.presses++; }
	g_mash.fill = std::max(0.0f, g_mash.fill - dt * 0.3f);
	UI::PromptMeter(g_padOn && PadRecentlyUsed() ? "MASH  A  TO GET UP" : "MASH  SPACE  TO GET UP", g_mash.fill);
	if (g_mash.fill >= 1.0f)
	{
		PED::SET_PED_TO_RAGDOLL(me, 1, 1, 0, FALSE, FALSE, nullptr);   // (a 1 ms ragdoll: it ends now, and he gets up)
		PED::REQUEST_PED_GETUP_ANIMATION(me, "FRONT");
		g_mash.ups++;
		g_mash.upAt = t;
		Log("mash: back on his feet after %d presses (%.1f s down) - %d times", g_mash.presses, t - g_mash.since, g_mash.ups);
		g_mash.on = false; g_mash.fill = 0; g_mash.presses = 0;
	}
}

// ======================= v0.6: spin check =======================
// Playtest 6: "I was thinking about the train smoke too, is a good one" - and the spin still didn't read. The black
// locomotive exhaust is directional, but which way its emitter points is unknown, so this shows the same ring of six
// smoke streamers circling a column with five different aims, one at a time. Just say which one swirls AROUND the
// column. The answer sets [Visuals] StreamerPitch (and the effect) for the funnels.
// v0.7: playtest 7 answered round one - only the ambient train smoke showed and swirled ("we definitely can see some
// swirling smoke"; the exhaust: "I don't see anything"). Round two compares that smoke's aim and speed, and the wind
// swirl experiment. Best in open country in daylight (in Saint Denis' fog: "we can't see anything in this").
static const int kSpinVariants = 5;
static const float kSpinPitch[kSpinVariants] = { -60.0f, 0.0f, -90.0f, -60.0f, -60.0f };
static const float kSpinSpeed[kSpinVariants] = { 2.2f, 2.2f, 2.2f, 4.5f, 2.2f };
static const bool kSpinWind[kSpinVariants] = { false, false, false, false, true };
static const char* kSpinFx[kSpinVariants] = { "ent_amb_trn4_train_smoke", "ent_amb_trn4_train_smoke", "ent_amb_trn4_train_smoke",
	"ent_amb_trn4_train_smoke", "ent_amb_trn4_train_smoke" };
struct SpinRig { bool on = false; int variant = 0; V3 origin; std::vector<int> fx; int ok = 0; } g_spin;

static V3 SpinPos(int i, int n, float t, float* ang)
{
	float hf = n > 1 ? (float)i / (n - 1) : 0.0f;
	*ang = i * 2 * PI / 3 + t * kSpinSpeed[g_spin.variant];
	float r = 8.0f + 6.0f * hf;
	return g_spin.origin + V3(cosf(*ang) * r, sinf(*ang) * r, 3.0f + hf * 40.0f);
}

static void SpinStop()
{
	if (!g_spin.on) return;
	int alive = 0;
	for (int h : g_spin.fx)
		if (h && GRAPHICS::DOES_PARTICLE_FX_LOOPED_EXIST(h))
		{
			alive++;
			GRAPHICS::STOP_PARTICLE_FX_LOOPED(h, FALSE);
			GRAPHICS::REMOVE_PARTICLE_FX(h, FALSE);
		}
	Log("SPIN CHECK variant %d (%s, pitch %.0f, speed %.1f, wind swirl %d) ended: %d of %d alive", g_spin.variant + 1, kSpinFx[g_spin.variant],
		kSpinPitch[g_spin.variant], kSpinSpeed[g_spin.variant], (int)kSpinWind[g_spin.variant], alive, g_spin.ok);
	if (kSpinWind[g_spin.variant])
		MISC::SET_WIND_SPEED(g_storm.active ? g_storm.savedWind : 1.0f);
	AddLoopsInUse(-g_spin.ok);
	g_spin = SpinRig();
}

static void SpinStart(int variant)
{
	SpinStop();
	g_spin.on = true;
	g_spin.variant = variant;
	g_spin.origin = PointAheadCam(70.0f);
	int n = std::min(6, FreeLoops());
	float t = NowSec();
	for (int i = 0; i < n; i++)
	{
		float ang;
		V3 p = SpinPos(i, n, t, &ang);
		GRAPHICS::USE_PARTICLE_FX_ASSET("core");
		int h = GRAPHICS::START_PARTICLE_FX_LOOPED_AT_COORD(kSpinFx[variant], p.x, p.y, p.z, kSpinPitch[variant], 0, ang * 180.0f / PI, 3.0f, FALSE, FALSE, FALSE, FALSE);
		if (h)
		{
			GRAPHICS::SET_PARTICLE_FX_LOOPED_FAR_CLIP_DIST(h, 1500.0f);
			g_spin.ok++;
		}
		g_spin.fx.push_back(h);
	}
	AddLoopsInUse(g_spin.ok);
	Log("SPIN CHECK variant %d: %s, pitch %.0f - %d of %d streamers started", variant + 1, kSpinFx[variant], kSpinPitch[variant], g_spin.ok, n);
}

static void SpinUpdate(float t)
{
	if (!g_spin.on) return;
	if (kSpinWind[g_spin.variant])
	{
		MISC::SET_WIND_SPEED(18.0f);
		MISC::SET_WIND_DIRECTION(fmodf(t * 1.6f, 2 * PI));
	}
	int n = (int)g_spin.fx.size();
	for (int i = 0; i < n; i++)
	{
		if (!g_spin.fx[i] || !GRAPHICS::DOES_PARTICLE_FX_LOOPED_EXIST(g_spin.fx[i])) continue;
		float ang;
		V3 p = SpinPos(i, n, t, &ang);
		GRAPHICS::SET_PARTICLE_FX_LOOPED_OFFSETS(g_spin.fx[i], p.x, p.y, p.z, kSpinPitch[g_spin.variant], 0, ang * 180.0f / PI);
	}
}

static void AutoSpinCheck()
{
	AutoStop("replaced");
	std::vector<AutoStep> steps;
	steps.push_back({ 5.0f, "storm rolling in - then watch the black smoke and SAY what it does", []()
	{
		g_holdStorm = true;
		Finding("SPIN CHECK start %s | funnels currently use StreamerPitch %.0f | answers are in the recording", kVersion, g_set.streamerPitch);
	} });
	for (int v = 0; v < kSpinVariants; v++)
	{
		char desc[200];
		sprintf_s(desc, "%d/%d: does the smoke SWIRL AROUND the ring?  say YES, A BIT or NO%s", v + 1, kSpinVariants,
			kSpinWind[v] ? "   (the wind turns too)" : (kSpinSpeed[v] > 3 ? "   (faster)" : ""));
		steps.push_back({ 12.0f, desc, [v]() { SpinStart(v); } });
		steps.push_back({ 1.5f, "clearing", []() { SpinStop(); } });
	}
	steps.push_back({ 0.5f, "done", []() { Finding("SPIN CHECK done (see the recording's transcript for the answers)"); } });
	AutoRun("spin check", steps, []() { SpinStop(); g_holdStorm = false; });
}

// ======================= v0.6: tree scan cache =======================
// Playtest 6: the tree demo spent its first 40 s scanning 411 models with nothing to see ("I don't know if it's testing
// anything"). The scan's result is now saved next to the mod and reused.
static std::string TreeCachePath() { return ModuleDir() + "\\TornadoRedemption_trees.txt"; }

static void SaveTreeCache()
{
	FILE* f = nullptr;
	if (fopen_s(&f, TreeCachePath().c_str(), "w") != 0 || !f) return;
	fprintf(f, "# Tornado Redemption tree scan: model height(m). Delete this file to scan again.\n");
	for (auto& t : g_spawnableTrees) fprintf(f, "%s %.1f\n", t.name.c_str(), t.height);
	fclose(f);
}

static void LoadTreeCache()
{
	FILE* f = nullptr;
	if (fopen_s(&f, TreeCachePath().c_str(), "r") != 0 || !f) return;
	char name[128];
	float h;
	char line[256];
	g_spawnableTrees.clear();
	while (fgets(line, sizeof(line), f))
	{
		if (line[0] == '#') continue;
		if (sscanf_s(line, "%127s %f", name, (unsigned)sizeof(name), &h) == 2 && !strstr(name, "snow"))
			g_spawnableTrees.push_back({ name, h });
	}
	fclose(f);
	Log("tree scan cache: %d spawnable trees loaded from %s", (int)g_spawnableTrees.size(), TreeCachePath().c_str());
}

// v1.1 real map trees: the stand-ins a tornado tears out in a map tree's place - natural trees the game lets us spawn (the
// tree scan's results; these defaults are what the scan found in playtest 7, true for every copy of the game).
static void BuildStandIns()
{
	static const StandIn kDefaults[] = {
		{ "p_tree_magnolia_02", 15.2f }, { "p_tree_cedar_s_deep_02_c", 13.5f }, { "p_cs_treestanding01x", 11.6f }, { "p_tree_pine_ponderosa_06", 11.6f },
		{ "p_tree_douglasfir_03", 11.4f }, { "p_tree_hangtreeoak_ropeswing", 9.4f }, { "p_tree_magnolia_01", 8.0f }, { "p_tree_baldcypress_06a", 7.2f },
		{ "p_tree_baldcypress_06b", 6.8f }, { "p_tree_cactus_01e", 6.7f }, { "p_tree_joshua_02d", 6.2f }, { "p_tree_mesquite_01", 5.8f },
		{ "p_tree_douglasfir_04", 5.3f }, { "p_tree_maple_s_04", 5.1f }, { "p_tree_pine_ponderosa_07", 5.0f },
	};
	static const char* kNotTrees[] = { "street", "lamp", "light", "clock", "pole", "cart", "fallen", "log", "des_", "vine", "stump", "rope01x" };
	g_standIns.clear();
	for (auto& t : g_spawnableTrees)
	{
		bool skip = t.height < 4.5f;
		for (const char* k : kNotTrees) if (strstr(t.name.c_str(), k)) skip = true;
		if (!skip) g_standIns.push_back({ t.name, t.height });
	}
	if (g_standIns.empty())
		for (auto& d : kDefaults) g_standIns.push_back(d);
	for (auto& s : g_standIns) s.hash = Joaat(s.name.c_str());   // v1.1 audit 2: hashed once, not in the tree scan's loops
	// (loaded on demand when a trunk needs one - kept loaded, they'd look like trees growing everywhere)
	Log("real trees: %d stand-in tree models", (int)g_standIns.size());
}

// ======================= v0.5 tests =======================
// Drop test: soft landing without needing a tornado. Arthur (and his horse, if he's riding) is lifted 40 m straight up
// and let go; the log records every frame of the fall, the touchdown speed and his health before and after.
struct DropTest { int healthBefore = -1; int landingsBefore = 0; bool savedSoft = true; } g_drop;

static void LiftArthur(float metres)
{
	Ped me = PLAYER::PLAYER_PED_ID();
	Entity body = PED::IS_PED_ON_MOUNT(me) ? PED::GET_MOUNT(me) : me;
	V3 p = ENTITY::GET_ENTITY_COORDS(body, TRUE, FALSE);
	float gz = GroundZ(p.x, p.y, p.z + 2.0f, p.z);
	ENTITY::SET_ENTITY_COORDS(body, p.x, p.y, gz + metres, FALSE, FALSE, FALSE, FALSE);
	ENTITY::SET_ENTITY_VELOCITY(body, 0, 0, -1.0f);
	Log("drop test: lifted %s %.0f m above the ground at (%.0f, %.0f)", body == me ? "Arthur" : "Arthur and his horse", metres, p.x, p.y);
}

static void AutoDropTest()
{
	AutoStop("replaced");
	std::vector<AutoStep> steps;
	steps.push_back({ 3.0f, "soft landing drop test: you'll be lifted 40 m and dropped - hands off", []()
	{
		g_drop = DropTest();
		g_drop.savedSoft = g_set.softLanding;
		g_set.softLanding = true;
		g_drop.healthBefore = ENTITY::GET_ENTITY_HEALTH(PLAYER::PLAYER_PED_ID());
		g_drop.landingsBefore = GetLanding().landings;
	} });
	steps.push_back({ 9.0f, "falling from 40 m - soft landing should catch you", []()
	{
		StartDropTest(NowSec());
		LiftArthur(40.0f);
	} });
	steps.push_back({ 0.5f, "drop test result", []()
	{
		const LandingStats& L = GetLanding();
		bool alive = !PlayerDead();
		bool landed = L.landings > g_drop.landingsBefore;
		bool soft = landed && L.lastImpact > -9.0f;
		int hNow = ENTITY::GET_ENTITY_HEALTH(PLAYER::PLAYER_PED_ID());
		Finding("DROPTEST %s | alive %d, touchdown seen %d, touchdown speed %.1f m/s (soft = slower than 9), health %d -> %d",
			alive && soft ? "PASS" : "FAIL", (int)alive, (int)landed, L.lastImpact, g_drop.healthBefore, hNow);
		char b[160];
		sprintf_s(b, "Drop test: %s - landed at %.1f m/s, health %d -> %d", alive && soft ? "PASSED" : "FAILED", L.lastImpact, g_drop.healthBefore, hNow);
		Notify(b, 7000);
	} });
	AutoRun("drop test", steps, []() { g_set.softLanding = g_drop.savedSoft; });
}

// Finish-line run (hands-free, ~2 min): the whole v0.5 promise in one take, with the log keeping score.
//   1. the storm rolls in; 2. the flagship touches down 170 m out and the drone orbits it (is the whole cone visible?
//   does it turn?); 3. a low angle from the ground; 4. it comes for Arthur with Grab Arthur and Fling and chase on:
//   ride it, get thrown out, land soft, watch it keep coming; 5. it dies down.
struct FinishRun { int savedArthur = 0; bool savedGrab = false, savedFling = true, savedSoft = true, savedGod = true; int savedMove = 2, savedSpeed = 1;
	float savedHoldMin = 15, savedHoldMax = 30; int landingsBefore = 0; } g_fin;

// ======================= v1.0: Survive the storm =======================
// Playtest 8: "If you could have a timed mode, like a timer mode, that's probably something people would want. Run away
// from the tornado for a while." The tornado touches down where you look and comes for you; it gets faster every 30 s.
// The clock stops when it lifts Arthur off the ground (or he dies). Best time is saved next to the mod.
struct Survival
{
	bool on = false;
	float start = 0, best = 0, lastTime = 0, resultUntil = 0;
	int stage = 0;
	bool newBest = false;
	std::string result;
	int savedArthur = 2, savedMove = 2, savedSpeed = 1;
	bool savedFling = true, bestLoaded = false;
} g_surv;

static std::string BestPath() { return ModuleDir() + "\\TornadoRedemption_best.txt"; }

// TornadoRedemption_best.txt (v1.1): one "name value" line per challenge. v1.0 wrote a bare number - and playtest 10's 2:05 was
// inflated (the timer couldn't stop) - so a bare number is ignored.
static float ReadBest(const char* name)
{
	FILE* f = nullptr;
	if (fopen_s(&f, BestPath().c_str(), "r") != 0 || !f) return 0;
	char line[160], key[32];
	float v = 0, found = 0;
	while (fgets(line, sizeof(line), f))
		if (sscanf_s(line, "%31s %f", key, (unsigned)sizeof(key), &v) == 2 && !strcmp(key, name)) found = v;
	fclose(f);
	return found;
}

static void WriteBest(const char* name, float value)
{
	std::vector<std::string> keep;
	FILE* f = nullptr;
	size_t n = strlen(name);
	if (fopen_s(&f, BestPath().c_str(), "r") == 0 && f)
	{
		char line[160];
		while (fgets(line, sizeof(line), f))
			if (isalpha((unsigned char)line[0]) && !(strncmp(line, name, n) == 0 && line[n] == ' ')) keep.push_back(line);
		fclose(f);
	}
	if (fopen_s(&f, BestPath().c_str(), "w") != 0 || !f) return;
	fprintf(f, "# Tornado Redemption best scores (survive = seconds, chaser = points). Delete a line to reset it.\n");
	for (auto& l : keep) fputs(l.c_str(), f);
	fprintf(f, "%s %.2f\n", name, value);
	fclose(f);
}

static void LoadBest()
{
	g_surv.bestLoaded = true;
	g_surv.best = ReadBest("survive");
}

static void SaveBest() { WriteBest("survive", g_surv.best); }

static std::string Clock(float s)
{
	char b[32];
	int m = (int)(s / 60.0f);
	sprintf_s(b, "%d:%04.1f", m, s - m * 60.0f);
	return b;
}

static void SurvivalStop(const char* why)
{
	if (!g_surv.on) return;
	float t = NowSec() - g_surv.start;
	g_surv.on = false;
	g_surv.lastTime = t;
	// v1.1, playtest 10: only a run the storm actually ended counts - not one stopped by hand
	g_surv.newBest = t > g_surv.best && (!strcmp(why, "CAUGHT") || !strcmp(why, "DIED"));
	if (g_surv.newBest)
	{
		g_surv.best = t;
		SaveBest();
	}
	g_set.arthur = g_surv.savedArthur; g_set.movement = g_surv.savedMove; g_set.speed = g_surv.savedSpeed; g_set.flingChase = g_surv.savedFling;
	for (auto& tp : g_tornadoes) tp->BeginDissipate(NowSec());
	g_surv.result = std::string(why) + " after " + Clock(t) + (g_surv.newBest ? "   -   NEW BEST!" : "   (best " + Clock(g_surv.best) + ")");
	g_surv.resultUntil = NowSec() + 8.0f;
	Finding("SURVIVAL %s after %.1f s (best %.1f%s)", why, t, g_surv.best, g_surv.newBest ? ", new best" : "");
}

static void SurvivalStart()
{
	AutoStop("replaced");
	if (g_surv.on) SurvivalStop("STOPPED");
	if (!g_surv.bestLoaded) LoadBest();
	DespawnAll();
	BalloonRemove("survival");   // v1.1 audit: in the balloon it could never catch you
	g_surv.savedArthur = g_set.arthur; g_surv.savedMove = g_set.movement; g_surv.savedSpeed = g_set.speed; g_surv.savedFling = g_set.flingChase;
	g_set.arthur = std::max(2, g_set.arthur);   // it has to be able to catch you
	g_set.movement = 2;
	g_set.speed = 0;
	g_set.flingChase = false;
	g_lastUserSpawn = -100;
	Tornado* tp = SpawnTornado(g_set.style, 90.0f, 0, false, GetStyles()[g_set.style].name, -1, 0.0f, true);
	if (!tp) return;
	TouchCamStart(tp);
	g_surv.on = true;
	g_surv.start = NowSec();
	g_surv.stage = 0;
	g_surv.resultUntil = 0;
	Notify("SURVIVE THE STORM - run! It gets faster every 30 s.", 5000);
	Log("SURVIVAL started (best so far %.1f s)", g_surv.best);
}

static void SurvivalUpdate(float t)
{
	if (!g_surv.on) return;
	float el = t - g_surv.start;
	int stage = (int)(el / 30.0f);
	if (stage != g_surv.stage)
	{
		g_surv.stage = stage;
		g_set.speed = std::min(2, stage);
		if (stage <= 2) Notify(stage == 1 ? "It's picking up speed..." : "It's at full speed now!", 3000);
	}
	if (PlayerDead()) { SurvivalStop("DIED"); return; }
	bool any = false;
	for (auto& tp : g_tornadoes)
	{
		if (tp->Dissipating() || tp->Mini()) continue;   // v1.1 audit 2: a gun mini neither keeps the run alive nor ends it
		any = true;
		float h = 0;
		if (el > 8.0f && t - tp->playerLastHeld < 0.3f && TrueHeight(PlayerPos(), &h) && h > 5.0f)
		{
			SurvivalStop("CAUGHT");
			return;
		}
	}
	if (!any) SurvivalStop("ENDED");
}

static void SurvivalDraw()
{
	const char* line1 = nullptr;
	std::string text;
	if (g_surv.on)
	{
		text = "SURVIVE   " + Clock(NowSec() - g_surv.start) + (g_surv.best > 0 ? "      best " + Clock(g_surv.best) : "");
		line1 = text.c_str();
	}
	else if (NowSec() < g_surv.resultUntil)
	{
		text = g_surv.result;
		line1 = text.c_str();
	}
	if (!line1) return;
	const float w = 0.30f, x0 = 0.5f - w / 2, y0 = 0.080f, h = 0.036f;
	for (int k = 0; k < 4; k++)
		DrawBox(x0, y0 + h * k / 4.0f, w, h / 4.0f + 0.0005f, 30, 8, 6, 205 - k * 18);
	DrawBox(x0, y0, w, 0.003f, 175, 30, 22, 255);
	float tw = strlen(line1) * 0.0052f;
	DrawTextLine(line1, 0.5f - tw / 2, y0 + 0.008f, 0.30f, 245, 228, 195);
}

static void AutoFinishLine()
{
	AutoStop("replaced");
	std::vector<AutoStep> steps;
	steps.push_back({ 6.0f, "storm rolling in - just watch and talk", []()
	{
		g_fin = FinishRun();
		g_fin.savedArthur = g_set.arthur; g_fin.savedGrab = g_set.grabPlayer; g_fin.savedFling = g_set.flingChase; g_fin.savedSoft = g_set.softLanding;
		g_fin.savedGod = g_set.playerGod; g_fin.savedMove = g_set.movement; g_fin.savedSpeed = g_set.speed;
		g_fin.savedHoldMin = g_set.flingHoldMin; g_fin.savedHoldMax = g_set.flingHoldMax;
		g_fin.landingsBefore = GetLanding().landings;
		g_holdStorm = true;
		Finding("FINISH-LINE RUN start %s | style %s | engine %s | HUD %d", kVersion, GetStyles()[g_set.style].name,
			g_set.engine == 1 ? "world-space" : "legacy anchors", g_set.hud);
	} });
	steps.push_back({ 22.0f, "touchdown 170 m out - drone orbit: is the WHOLE cone visible? can you see it turn?", []()
	{
		SpawnTornado(g_set.style, 170.0f, 0, true, GetStyles()[g_set.style].name, -1, 0.0f);
		g_drone.mode = 1;
	} });
	steps.push_back({ 14.0f, "low angle from the ground - how dark, how big, how spinny?", []() { g_drone.mode = 2; } });
	steps.push_back({ 3.0f, "back to Arthur", []() { g_drone.mode = 0; } });
	steps.push_back({ 55.0f, "here it comes - ride it, get thrown out, land soft, then RUN", []()
	{
		g_set.arthur = 2; g_set.grabPlayer = true; g_set.flingChase = true; g_set.softLanding = true;
		g_set.flingHoldMin = 10.0f; g_set.flingHoldMax = 14.0f;   // a shorter ride so the throw happens on camera
		g_set.movement = 2; g_set.speed = 2;
		for (auto& tp : g_tornadoes) tp->stationary = false;
	} });
	steps.push_back({ 9.0f, "letting it die down", []()
	{
		int flings = 0;
		for (auto& tp : g_tornadoes) { flings += tp->flings; tp->BeginDissipate(NowSec()); }
		const LandingStats& L = GetLanding();
		Finding("FINISH-LINE RUN result | flings %d | soft landings %d (last touchdown %.1f m/s, worst %.1f) | Arthur alive %d | puffs x%.2f refused %d",
			flings, L.landings - g_fin.landingsBefore, L.lastImpact, L.worstImpact, (int)!PlayerDead(), PuffRateScale(), PuffsRefused());
	} });
	AutoRun("finish-line run", steps, []()
	{
		DespawnAll();
		g_set.arthur = g_fin.savedArthur; g_set.grabPlayer = g_fin.savedGrab; g_set.flingChase = g_fin.savedFling; g_set.softLanding = g_fin.savedSoft;
		g_set.playerGod = g_fin.savedGod; g_set.movement = g_fin.savedMove; g_set.speed = g_fin.savedSpeed;
		g_set.flingHoldMin = g_fin.savedHoldMin; g_set.flingHoldMax = g_fin.savedHoldMax;
		g_drone.mode = 0;
		g_holdStorm = false;
	});
}

// ======================= v1.1: the new modes (separate files, one translation unit) =======================
#include "balloon.inl"
#include "intro.inl"
#include "season.inl"
#include "gun.inl"
#include "music.inl"
#include "gallery.inl"
#include "voices.inl"

// ======================= menu =======================
// v1.1 (the user: "consolidating and simplifying the menu with good explanations and improving animations and in game ui to
// match rdr2 as close as possible"): five pages instead of nine at the top, every item explained in the footer, drawn with
// the game's own menu textures (Halen84's native menu base: the ink-roller panel, the header banner, the red crafting frame,
// tick boxes, arrows), its fonts ($title / $body) and its menu sounds. [UI] Style=Simple brings back the plain v1.0 look.
struct Item
{
	enum Kind { Action, Toggle, Choice, Page } kind;
	std::string label;
	std::string help;
	std::function<void()> action;
	bool* flag = nullptr;
	int* value = nullptr;
	std::function<std::string(int)> valueText;
	std::function<int()> valueCount;
	std::function<void()> onChange;   // runs when the value changes with left/right
	std::function<void()> onSelect;   // Enter on a choice: runs this instead of cycling
	std::function<std::string()> helpFn;   // dynamic help (e.g. the selected style's description)
	std::function<std::string()> labelFn;  // dynamic label (e.g. "Jet balloon" / "Leave the balloon")
	int page = 0;
};
struct MenuPage { std::string title, sub; std::vector<Item> items; };
static std::vector<MenuPage> g_pages;
static std::vector<std::pair<int, int>> g_menuStack;

static Item MkAction(const std::string& l, const std::string& help, std::function<void()> f) { Item i{ Item::Action, l, help }; i.action = f; return i; }
static Item MkToggle(const std::string& l, const std::string& help, bool* b) { Item i{ Item::Toggle, l, help }; i.flag = b; return i; }
static Item MkPage(const std::string& l, const std::string& help, int page) { Item i{ Item::Page, l, help }; i.page = page; return i; }
static Item MkChoice(const std::string& l, const std::string& help, int* v, std::function<int()> count, std::function<std::string(int)> text,
	std::function<void()> changed = nullptr, std::function<void()> selected = nullptr)
{
	Item i{ Item::Choice, l, help };
	i.value = v; i.valueCount = count; i.valueText = text; i.onChange = changed; i.onSelect = selected;
	return i;
}
static std::function<int()> N(int n) { return [n]() { return n; }; }
static std::function<std::string(int)> Names(std::vector<std::string> names) { return [names](int i) { return names[i % names.size()]; }; }

enum { P_MAIN, P_MODES, P_TORNADO, P_GRAB, P_WEATHER, P_WORLD, P_CAMERA, P_TESTS, P_ADV, P_DEV, P_LAB, P_TREES, P_ARTHUR, P_COUNT };

// Everything the user can make, gone - and the world put back (map trees, grass).
static void ClearEverything()
{
	AutoStop("stopped");
	SurvivalStop("STOPPED");
	if (g_bal.chase) ChaseEnd("STOPPED");
	SeasonCancel("despawn");
	GalleryClose("despawn");
	DespawnAll();
	IntroRemoveAll();
	BalloonRemove("despawn");
	g_gun.minis.clear();
	for (auto& v : g_gun.victims) if (v.p) SetScripted(v.p, false);
	g_gun.victims.clear();
	RestoreRealTrees();
	ClearVegTrail();
	g_lab.slideshow = false;
	LabStop();
	Notify("Everything removed - trees and grass put back");
}

static std::string Keys(const Hotkey& k, const PadCombo& p) { return k.text + " / pad " + p.text; }

#include "checks.inl"

static void BuildMenu()
{
	g_pages.assign(P_COUNT, MenuPage());
	auto styleCount = []() { return (int)GetStyles().size(); };
	auto styleName = [](int i) { return std::string(GetStyles()[i].name); };

	// ---- main: what you change while playing ----
	g_pages[P_MAIN].title = "TORNADO REDEMPTION";
	g_pages[P_MAIN].sub = "Tornadoes for Red Dead Redemption 2";
	auto& m = g_pages[P_MAIN].items;
	m.push_back(MkAction("Spawn tornado", "Touches down where you look, just outside its reach, then comes for you. Quick key: " + Keys(g_keys.spawn, g_pad.spawn) + ".",
		[]() { g_menuOpen = false; UserSpawn(); }));
	{
		Item st = MkChoice("Style", "", &g_set.style, styleCount, styleName, []()
		{
			for (auto& tp : g_tornadoes)
				if (!tp->Dissipating() && !tp->Mini() && !tp->Display())
					tp->Restyle(g_set.style, tp->loopsRunning + FreeLoops());
		});
		st.helpFn = []() { return std::string(GetStyles()[g_set.style % GetStyles().size()].blurb) + " Changes the ones already out, too."; };
		m.push_back(st);
	}
	{
		Item ar = MkPage("Arthur", "His own page: invincible or not, how the tornado treats him, how he lands, the things it throws at him, heal him.", P_ARTHUR);
		ar.labelFn = []()
		{
			static const char* kLv[] = { "immune", "tugged", "grabbable", "easy prey" };
			static const char* kLand[] = { "soft landings", "mixed landings", "real landings" };
			return std::string("Arthur: ") + kLv[g_set.arthur % 4] + ", " + (g_set.playerGod ? "invincible" : kLand[g_set.landings % 3]);
		};
		m.push_back(ar);
	}
	m.push_back(MkChoice("Storm season", "Tornadoes that turn up on their own: the sky darkens, a warning says where, and a wild one touches down out on the map and wanders your way. Rare: every 12-25 min. Frequent: every few minutes.",
		&g_set.season, N(4), Names({ "Off", "Rare", "Regular", "Frequent" })));
	m.push_back(MkPage("Modes & toys", "The intro cutscene, the jet balloon, Storm chaser, Survive the storm, the tornado and texture galleries, the tornado gun.", P_MODES));
	m.push_back(MkPage("The tornado", "Strength, size, speed, how it moves and how far it reaches; real trees, flattened grass; what it grabs and how it treats Arthur.", P_TORNADO));
	m.push_back(MkPage("Weather, time & sound", "The storm sky, lightning, rain and wind; the time of day and the weather right now; the tornado's roar and mission music; the voices and the storm report.", P_WEATHER));
	m.push_back(MkPage("Camera & HUD", "The storm tracker, hands-free drone shots, cinematic mode, the touchdown and ride cameras, the look of this menu.", P_CAMERA));
	m.push_back(MkChoice("Performance", "High: everything. Balanced: a little less smoke, debris and grabbing (the default). Low PC: about half. Applies to new tornadoes.",
		&g_set.perf, N(3), Names({ "High", "Balanced", "Low PC" })));
	m.push_back(MkAction("Let it die down", "The tornadoes pull back up into the clouds and fade out.", []() { for (auto& tp : g_tornadoes) tp->BeginDissipate(NowSec()); }));
	m.push_back(MkAction("Despawn everything", "Removes every tornado, the intro's camp and gang, the balloon and the mini twisters, and puts the map trees and grass back. Quick key: " + Keys(g_keys.despawn, g_pad.despawn) + ".",
		[]() { ClearEverything(); }));
	m.push_back(MkPage("Tests & tools", "The hands-free showcase, self-test, drop test, tree demo, map-tree check, style tours and the developer tools.", P_TESTS));

	// ---- modes & toys ----
	g_pages[P_MODES].title = "MODES & TOYS";
	g_pages[P_MODES].sub = "Things to do with a tornado";
	auto& md = g_pages[P_MODES].items;
	md.push_back(MkAction("The intro: Storm Chasers", "40 s, from anywhere: a sunny morning at the gang's camp, then Arthur, Dutch, Micah and John ride out to watch a storm over Emerald Ranch - and it comes for them. Then run for the balloon. (The first time it takes longer to start.) Backspace / B skips.",
		[]() { IntroStart(); }));
	{
		Item b = MkAction("Jet balloon", "", []()
		{
			g_menuOpen = false;
			if (BalloonActive()) { BalloonRemove("menu"); Notify("Balloon gone"); return; }
			V3 p = PlayerPos() + HeadingDir(PlayerHeading()) * 2.5f;
			p.z = GroundZ(p.x, p.y, p.z + 10.0f, p.z);
			BalloonSpawn(p, PlayerHeading());
		});
		b.labelFn = []() { return std::string(BalloonActive() ? "Leave the jet balloon" : "Jet balloon"); };
		b.helpFn = []() { return std::string("A jet-powered hot air balloon, right here. Fly where you look: W A S D (left stick), SPACE / CTRL up and down (RT / LT), SHIFT (A) for the jets, F (Y) to bail out. The tornado leaves you alone up there."); };
		md.push_back(b);
	}
	{
		Item c = MkAction("Storm chaser (balloon)", "", []() { g_menuOpen = false; ChaseStart(); });
		c.helpFn = []() { char b[300]; sprintf_s(b, "90 seconds in the jet balloon: get as close to the funnel as you dare - the closer, the faster the points. Touch the wall and it has you.%s",
			g_bal.chaseBest > 0 ? (std::string(" Best: ") + std::to_string((int)g_bal.chaseBest) + ".").c_str() : ""); return std::string(b); };
		md.push_back(c);
	}
	{
		Item s = MkAction("Survive the storm (timer)", "", []() { g_menuOpen = false; SurvivalStart(); });
		s.helpFn = []() { return std::string("It touches down where you look and hunts you, faster every 30 s. The clock stops when it lifts you.") +
			(g_surv.best > 0 ? " Best: " + Clock(g_surv.best) + "." : ""); };
		md.push_back(s);
	}
	md.push_back(MkAction("Tornado gallery", "Tornadoes side by side on the horizon, harmless, with their names: the classics, the elements (dust, fire, snow, water), the odd ones, and one tornado built five different ways. Left / right browse, up / down another room, Enter spawns one for real.",
		[]() { GalStylesOpen(); }));
	md.push_back(MkAction("Texture gallery", "The smoke, dust, steam and cloud effects a funnel can be made of, five at a time, each spun into a little twister. Left / right browse, Enter builds style E (Custom) out of one, the note key saves it.",
		[]() { GalTexOpen(); }));
	md.push_back(MkToggle("Tornado gun", "Every shot you fire spawns a pocket-sized twister where it lands, about as tall as a person - it wanders about for 30 s, spinning up whatever's close. Up to three at once. Works with any gun. Shoot someone and they're spun round it, then knocked back.", &g_gun.on));
	md.push_back(MkAction("A wild storm, now", "Starts a Storm season storm right away: the sky darkens, the warning, and a wild one touches down a few hundred metres off.",
		[]() { g_menuOpen = false;
			if (!g_tornadoes.empty()) { Notify("Clear the tornadoes out first (Despawn everything)"); return; }
			if (g_season.stage) { Notify("A storm's already on its way"); return; }
			g_season.manual = true;   // runs even with Storm season Off
			SeasonBegin(NowSec()); }));

	// ---- the tornado ----
	g_pages[P_TORNADO].title = "THE TORNADO";
	g_pages[P_TORNADO].sub = "How it looks, moves and wrecks things";
	auto& to = g_pages[P_TORNADO].items;
	to.push_back(MkChoice("Strength", "How hard it spins, lifts, throws and tumbles things.", &g_set.force, N(3), Names({ "Gentle", "Violent", "Extreme" })));
	to.push_back(MkChoice("Funnel size", "How big it looks. What it lifts follows the funnel.", &g_set.size, N(3), Names({ "Small", "Medium", "Large" })));
	to.push_back(MkChoice("Speed", "How fast it travels: slow 3.5 m/s (you can outrun it on foot), normal 6, fast 10.", &g_set.speed, N(3), Names({ "Slow", "Normal", "Fast" })));
	to.push_back(MkChoice("Movement", "Toward you: it chases Arthur and lingers once it has him. Wander: it drifts about. Straight line: it just keeps going.", &g_set.movement, N(4), Names({ "Stationary", "Wander", "Toward you", "Straight line" })));
	to.push_back(MkChoice("Reach", "How far out it starts pulling things in - gently at the edge, hard near the funnel.", &g_set.reach, N(4), Names({ "Medium", "Large", "Huge", "Massive" })));
	to.push_back(MkChoice("Lifetime", "After this long a tornado dies down on its own.", &g_set.lifetime, N(4), Names({ "Forever", "2 min", "5 min", "10 min" })));
	to.push_back(MkToggle("Multiple tornadoes", "Off: a new tornado replaces the old one (best for performance). On: up to six, sharing the smoke.", &g_set.multi));
	to.push_back(MkToggle("Rips up real trees (new)", "Finds the map's trees in its path, makes the real one vanish and tears out a stand-in. Experimental: the log says how it went.", &g_set.mapTrees));
	to.push_back(MkToggle("Flattens grass", "Grass and bushes are flattened along its track - a trail it leaves behind.", &g_set.flatten));
	to.push_back(MkToggle("Debris cone", "Planks, barrels, wheels and logs carried round the funnel in spiral arms - the spin you can see.", &g_set.debrisCone));
	to.push_back(MkToggle("Impact bursts", "Dust, splinters or smashing glass where something it threw hits.", &g_set.impacts));
	to.push_back(MkToggle("Touchdown animation", "New tornadoes reach down from the clouds before they grab anything.", &g_set.touchdown));
	to.push_back(MkToggle("People flee", "Folks in the outer pull run for it before it reaches them.", &g_set.npcFlee));
	to.push_back(MkPage("What it grabs", "People, animals, wagons, props, fixed props, extra debris. (Arthur has his own page on the main menu.)", P_GRAB));

	g_pages[P_GRAB].title = "WHAT IT GRABS";
	g_pages[P_GRAB].sub = "People, animals and things";
	auto& g = g_pages[P_GRAB].items;
	g.push_back(MkToggle("People", "Townsfolk, lawmen, outlaws - anyone in reach gets pulled in and carried round. Off: people are left alone.", &g_set.grabPeople));
	g.push_back(MkToggle("Animals & horses", "Horses, cattle, deer, dogs. Your own horse too, unless you're riding it with Arthur set to Immune.", &g_set.grabAnimals));
	g.push_back(MkToggle("Wagons", "Wagons, carts and coaches (and whoever's on them). Heavy, so they mostly roll and tumble.", &g_set.grabVehicles));
	g.push_back(MkToggle("Props & items", "Loose things: barrels, crates, chairs, buckets, hay bales. The bulk of what you see flying.", &g_set.grabProps));
	g.push_back(MkToggle("Rip fixed props loose", "Stands, fences, lanterns (lanterns start fires!).", &g_set.ripFixedProps));
	g.push_back(MkToggle("Town safety", "In the big towns (Saint Denis, Valentine, Rhodes, Blackwater...) the street furniture stays put: no lamp posts, poles or fences ripped up and carried off. People, horses, wagons and loose things still fly. Saint Denis crashed the game three times without it.", &g_set.citySafe));
	g.push_back(MkToggle("Extra debris", "Planks, barrels and crates thrown in from the ground around it.", &g_set.extraDebris));
	// (v1.3: Arthur's switches moved to his own page)
	g.push_back(MkToggle("Bring down floating props", "Props it carried and left hanging in mid-air fall back down. Signs and lanterns are left alone.", &g_set.hoverFix));
	g.push_back(MkToggle("Dies down when Arthur dies", "Otherwise it keeps going and can follow him after he respawns.", &g_set.dieWithArthur));

	// ---- Arthur (v1.3) ----
	g_pages[P_ARTHUR].title = "ARTHUR";
	g_pages[P_ARTHUR].sub = "How the tornado treats him";
	auto& ap = g_pages[P_ARTHUR].items;
	ap.push_back(MkToggle("Invincible", "No damage at all while a tornado is out. Off (the default): rough landings and flying debris hurt him.", &g_set.playerGod));
	ap.push_back(MkChoice("In the tornado", "Immune: it never touches him. Tugged: dragged about, escapable. Grabbable: it can carry him off. Easy prey: it takes him high.",
		&g_set.arthur, N(4), Names({ "Immune", "Tugged", "Grabbable", "Easy prey" })));
	ap.push_back(MkChoice("Landings", "Soft: every fall is caught (now and then a long glide down). Mixed: soft or hard landings (a hard one hurts), the stratosphere, sky-high, a far throw, a far glide, slammed down, or a meteor into the ground that doesn't hurt - but it never kills him. Real: no catch at all.",
		&g_set.landings, N(3), Names({ "Soft", "Mixed", "Real" })));
	ap.push_back(MkToggle("Fling and chase", "When it has him, after 10-18 s it throws him out - and keeps coming. Can you get away?", &g_set.flingChase));
	ap.push_back(MkToggle("Things thrown at him", "Now and then it hurls a plank, a barrel or a wheel straight at him (or just short), and more of its debris comes his way.", &g_set.throwAtArthur));
	ap.push_back(MkToggle("Soft landings cost a little", "Even a caught landing costs a bit of health (never below a quarter).", &g_set.landingHurts));
	ap.push_back(MkToggle("Ride camera", "When it lifts him, the camera circles him for 5 s (every other ride it eases out to show the funnel round him, and you can steer it), then hands back. A throw ends it.", &g_set.rideCam));
	ap.push_back(MkAction("Heal Arthur", "Full health, right now.", []()
	{
		Ped me = PLAYER::PLAYER_PED_ID();
		ENTITY::SET_ENTITY_HEALTH(me, ENTITY::GET_ENTITY_MAX_HEALTH(me, FALSE), 0);
		UI::Toast("Arthur", "Patched up.", 2.5f);
	}));

	// ---- weather & sound ----
	g_pages[P_WEATHER].title = "WEATHER, TIME & SOUND";
	g_pages[P_WEATHER].sub = "The sky it comes out of, and how it sounds";
	auto& w = g_pages[P_WEATHER].items;
	w.push_back(MkChoice("Weather override", "While a tornado is out. Storm clouds: a dark thunder sky, no rain (the default). Off: your weather - a sunshine twister. A weather you lock yourself always wins.",
		&g_set.weatherMode, N(4), Names({ "Off (keep yours)", "Storm clouds", "Thunderstorm", "Rain" })));
	w.push_back(MkToggle("Dark sky", "Darkens the light while a storm is out, so the funnel stands out against the sky.", &g_darkSky));
	w.push_back(MkChoice("Lightning", "Rare: every 1-2 min. Near the funnel: every 45-90 s, close to it.", &g_set.lightning, N(3), Names({ "Off", "Rare", "Near the funnel" })));
	w.push_back(MkToggle("Heavy rain", "Forces rain while a tornado is out.", &g_set.heavyRain));
	w.push_back(MkToggle("Strong wind", "The game's wind picks up while a tornado is out.", &g_set.wind));
	w.push_back(MkToggle("Wind howls nearer", "The game's own wind howls louder the closer it gets.", &g_set.roar));
	w.push_back(MkChoice("Tornado roar", "The tornado's own sound: a deep freight-train rumble with gusts and cracks, louder as it gets closer, from the side it's on. Made by the mod (no samples).",
		&g_roarLevel, N(4), Names({ "Off", "Quiet", "Normal", "Loud" })));
	w.push_back(MkChoice("Mission music", "The game's own action music while a tornado is out, like a mission: it builds when one appears and kicks in when it's on you. Pick the score. It's Rockstar's music - it may be claimed on YouTube.",
		&g_musicMode, N(7), Names({ "Off", "Hideout fight", "Braithwaite battle", "Outlaw ambush", "Hostage rescue", "Horse chase", "Native Son storm" })));
	w.push_back(MkChoice("Voices", "Arthur, the gang and passers-by react out loud with the game's own lines, strung into little exchanges: someone screams as it takes them and Arthur says goodbye; it drops him and he thanks it for the lift. Arthur only: just him.",
		&g_voices, N(3), Names({ "Off", "Arthur only", "Everyone" })));
	w.push_back(MkChoice("Chatter", "How often they talk. Rare: now and then. Chatty: a running commentary.", &g_chatter, N(3), Names({ "Rare", "Normal", "Chatty" })));
	w.push_back(MkToggle("Voice subtitles", "Asks the game to show its own subtitle for each line (only the lines that have one).", &g_voiceSubs));
	w.push_back(MkToggle("Storm report", "When a storm is over, a card: how many it took, the trees torn out, and how long and how high Arthur rode it.", &g_stormReport));
	w.push_back(MkPage("Time & weather now", "Set the hour, lock a weather, teleport.", P_WORLD));

	g_pages[P_WORLD].title = "TIME & WEATHER";
	g_pages[P_WORLD].sub = "Right now";
	auto& wo = g_pages[P_WORLD].items;
	wo.push_back(MkChoice("Hour", "Left/right to pick, then select to set it.", &g_hour, N(24), [](int h) { char b[16]; sprintf_s(b, "%02d:00", h); return std::string(b); },
		nullptr, []() { SetHour(g_hour); }));
	wo.push_back(MkToggle("Freeze time", "Stops the clock at the hour it is now, so the light stays the same for a whole recording.", &g_freezeTime));
	wo.push_back(MkChoice("Weather now", "Left/right to pick, then select to apply and lock it. A tornado won't change it.", &g_worldWeather, N(kWorldWeatherCount),
		[](int i) { return std::string(kWorldWeathers[i]); }, nullptr, []() { LockWeather(kWorldWeathers[g_worldWeather], 2.0f); g_manualWeather = true; Notify(std::string("Weather locked: ") + kWorldWeathers[g_worldWeather]); }));
	wo.push_back(MkAction("Unlock weather", "Lets the game's own weather take over again.", []() { UnlockWeather(); g_manualWeather = false; Notify("Weather unlocked"); }));
	wo.push_back(MkChoice("Teleport", "Left/right to pick, then select to go.", &g_teleport, N(kPlaceCount), [](int i) { return std::string(kPlaces[i].name); },
		nullptr, []() { TeleportTo(kPlaces[g_teleport].pos); Notify(std::string("Teleported to ") + kPlaces[g_teleport].name); }));

	// ---- camera & HUD ----
	g_pages[P_CAMERA].title = "CAMERA & HUD";
	g_pages[P_CAMERA].sub = "What you see";
	auto& c = g_pages[P_CAMERA].items;
	c.push_back(MkChoice("HUD", "Tracker: a compass at the top - where it is, how far, closing or not. Full: developer info too. Off: nothing.",
		&g_set.hud, N(3), Names({ "Full (dev)", "Tracker", "Off" })));
	c.push_back(MkChoice("Camera", "Hands-off shots: orbit the tornado, from the ground, or over Arthur's shoulder. Quick key: " + Keys(g_keys.drone, g_pad.drone) + ".",
		&g_drone.mode, N(4), Names({ "Normal", "Drone: orbit", "Drone: low angle", "Drone: over shoulder" })));
	c.push_back(MkToggle("Cinematic mode", "Hides every mod label and the game's HUD for clean footage. Quick key: " + Keys(g_keys.cinematic, g_pad.cinematic) + ".", &g_set.cinematic));
	c.push_back(MkToggle("Touchdown camera", "When you spawn one, the camera swings round to show it touching down, then hands back.", &g_set.touchdownCam));
	c.push_back(MkToggle("Ride camera", "When it lifts Arthur, the camera pulls out and circles him for 5 s, then hands back.", &g_set.rideCam));
	c.push_back(MkToggle("Camera shake", "Stronger the closer you are, strongest when it has you.", &g_set.camShake));
	c.push_back(MkToggle("Map marker", "The storm icon on the map and the minimap.", &g_set.mapBlip));
	c.push_back(MkToggle("Zone banner (HUD Full)", "THE PULL / THE WALL / THE EYE plate with a danger meter.", &g_set.zoneBanner));
	c.push_back(MkChoice("Menu look", "RDR2: the game's own menu art, fonts and sounds. Simple: the plain v1.0 boxes (if anything looks wrong on your setup).",
		&g_uiStyle, N(2), Names({ "RDR2", "Simple" }), []() { UI::g_simple = g_uiStyle == 1; }));

	// ---- tests & tools ----
	g_pages[P_TESTS].title = "TESTS & TOOLS";
	g_pages[P_TESTS].sub = "For playtests and the curious";
	auto& te = g_pages[P_TESTS].items;
	te.push_back(MkAction("Finish-line run (hands-free, ~2 min)", "Touchdown, drone orbit, low angle, then it comes for you: ride, get thrown, land soft. Just talk.",
		[]() { g_menuOpen = false; AutoFinishLine(); }));
	te.push_back(MkAction("Self-test (~1.5 min)", "Spawns, checks, heals, uproots, carries, dissipates and stress-tests tornadoes; logs PASS / FAIL.",
		[]() { g_menuOpen = false; AutoSelfTest(); }));
	te.push_back(MkAction("Drop test (soft landing, 12 s)", "Lifts Arthur 40 m and lets go. The soft landing should put him down gently.", []() { g_menuOpen = false; AutoDropTest(); }));
	te.push_back(MkAction("Tree demo", "Plants 3 tall trees where you look and sends the tornado straight through them.", []() { g_menuOpen = false; AutoTreeDemo(); }));
	te.push_back(MkAction("Systems check (~40 s)", "Hands-free: the jet balloon spawns, seats Arthur and climbs; a mini twister; a Storm season warning; the menu art and stand-in trees; which voice lines Arthur has; a Junknado's prop wall. Logs PASS / FAIL.",
		[]() { g_menuOpen = false; AutoV11Check(); }));
	te.push_back(MkAction("Map tree check (~15 s)", "Face a real tree 5-20 m away. Tries the ways to grab a map tree and asks you what you saw - say it out loud for the recording.",
		[]() { g_menuOpen = false; MapTreeCheck(); }));
	te.push_back(MkAction("Style tour", "Every style for 25 s, named, where you look. Next-step key (" + Keys(g_keys.next, g_pad.next) + ") moves on.", []() { g_menuOpen = false; AutoStyleTour(); }));
	te.push_back(MkAction("Strength tour", "Gentle, Violent, Extreme - each chases you for 25 s.", []() { g_menuOpen = false; AutoStrengthTour(); }));
	te.push_back(MkAction("Plant 3 trees where you look", "Trees the tornado will always tear out when it reaches them.", []() { PlantTreesAhead(3, 25.0f, 9.0f); }));
	te.push_back(MkAction("Stop test", "Stops the test that's running and removes its tornadoes.", []() { AutoStop("stopped"); DespawnAll(); }));
	te.push_back(MkPage("Advanced", "Funnel engine and render mode, wind swirl, push method, snapshots, reload the ini, tree tools.", P_ADV));
	te.push_back(MkPage("Developer tools", "Spin check, render check, controller check, tree scan, tree collision, voice audition, FX Lab.", P_DEV));

	g_pages[P_ADV].title = "ADVANCED";
	g_pages[P_ADV].sub = "Experiments and switches";
	auto& a = g_pages[P_ADV].items;
	a.push_back(MkToggle("Eye of the tornado (experimental)", "A calm centre where nothing is pushed. Hard to see from outside.", &g_set.eye));
	a.push_back(MkToggle("Wind swirl (experimental)", "The wind turns with the funnel so smoke everywhere drifts round. Trees whip about too.", &g_set.windSwirl));
	a.push_back(MkChoice("Funnel render", "Looped: smoke that follows the funnel. Puffs: one-shot bursts in spiral bands.", &g_set.render, N(3), Names({ "Looped only", "Puffs only", "Both" })));
	a.push_back(MkChoice("Funnel engine", "New tornadoes. World-space renders solid (render check, playtest 5); legacy anchors mostly don't.", &g_set.engine, N(2), Names({ "Legacy anchors", "World-space" })));
	a.push_back(MkChoice("Push method", "How it moves things. Hybrid (the default) sets speed for people and forces for props; Velocity and Force are the two halves on their own, for testing.", &g_set.pushMethod, N(3), Names({ "Hybrid", "Velocity", "Force" })));
	a.push_back(MkAction("Write snapshot to log", "Writes every tornado's numbers (effects alive, what it holds, frame time) to TornadoRedemption.log - handy right after something looks wrong.", []() { for (auto& tp : g_tornadoes) tp->Snapshot("MANUAL"); Notify("Snapshot written"); }));
	a.push_back(MkChoice("Anchor hide mode", "Legacy engine only. Visible = shows the apples the effects ride on.", &g_anchorHideMode, N(3), Names({ "Visible", "Invisible", "Alpha 0" })));
	a.push_back(MkAction("Reload TornadoRedemption.ini", "Reads TornadoRedemption.ini again, so you can change a setting there without restarting the game.", []() { LoadConfig(); Notify("Config reloaded"); }));
	a.push_back(MkPage("Tree tools", "Manual hide / restore / spawn / uproot.", P_TREES));

	g_pages[P_DEV].title = "DEVELOPER TOOLS";
	g_pages[P_DEV].sub = "Hands-free checks";
	auto& dv = g_pages[P_DEV].items;
	dv.push_back(MkAction("Spin check (hands-free, ~1.5 min)", "Rings of train smoke aimed differently. Say which one SWIRLS around.", []() { g_menuOpen = false; AutoSpinCheck(); }));
	dv.push_back(MkAction("Render check (hands-free, ~2.5 min)", "Playtest 5's smoke-column check (world-space won). Say: solid, flickers, or nothing.", []() { g_menuOpen = false; AutoRenderCheck(); }));
	dv.push_back(MkAction("Controller check (20 s)", "Press each button: logs which ones reach the mod and how (XInput or the game).", []() { g_menuOpen = false; AutoPadCheck(); }));
	dv.push_back(MkAction("Scan for spawnable trees", "Finds which full-size trees the game lets us spawn (~40 s, once - remembered in TornadoRedemption_trees.txt).", []() { g_menuOpen = false; AutoTreeScan(); }));
	dv.push_back(MkAction("Tree collision check", "Face a map tree 3-10 m away: hides it and checks if you could walk through.", []() { g_menuOpen = false; TreeCollisionTest(); }));
	dv.push_back(MkAction("Memory now", "The game's RAM and video memory right now, and what the mod has out (also logged every 30 s as MEM lines). RDR2 on Vulkan fills a big card's memory as a cache - a high number alone isn't a leak.",
		[]()
		{
			MemInfo mi;
			MemStats(&mi);
			char b[200];
			sprintf_s(b, "RAM %.1f GB. VRAM %.1f of %.1f GB%s. Mod: %d tornadoes, %d smoke effects.", mi.ramGB, mi.vramGB, mi.vramBudgetGB,
				mi.vramKnown ? "" : " (unknown)", (int)g_tornadoes.size(), LoopsInUse());
			UI::Toast("Memory", b, 6.0f);
			Finding("MEMORY NOW %s", b);
		}));
	dv.push_back(MkAction("Voice audition (~4 min)", "Plays every line the reactions use - Arthur's, then the nearest passer-by's - one every 4 s, with its name on screen and the game's subtitle on. Record it: the transcript tells us exactly what each one says.",
		[]() { g_menuOpen = false; AutoVoiceAudition(); }));
	dv.push_back(MkPage("FX Lab", "Browse and rate individual particle effects.", P_LAB));

	g_pages[P_LAB].title = "FX LAB";
	g_pages[P_LAB].sub = "One effect at a time";
	auto& l = g_pages[P_LAB].items;
	auto restartIfOn = []() { if (g_lab.on) LabStart(true); };
	l.push_back(MkChoice("Kind", "Looped: smoke that keeps going (what a funnel is made of). Puffs: one-shot bursts (dust, splinters).", &g_lab.kind, N(2), Names({ "Looped", "Puffs" }), []() { g_lab.idx = 0; if (g_lab.on) LabStart(true); }));
	l.push_back(MkChoice("Effect", "Left / right to browse; it changes live if one is showing. The texture gallery (Modes & toys) shows five at once.", &g_lab.idx, []() { return LabCount(); }, [](int i)
	{
		char b[96]; sprintf_s(b, "%d/%d %s", i + 1, LabCount(), g_lab.kind == 0 ? kLabLooped[i % kLabLoopedCount] : kLabPuffs[i % kLabPuffCount]); return std::string(b);
	}, restartIfOn));
	l.push_back(MkChoice("Scale", "How big the effect is drawn. Funnels use 2-5.", &g_lab.scaleIdx, N(sizeof(kLabScales) / sizeof(kLabScales[0])), [](int i) { char b[16]; sprintf_s(b, "%.1f", kLabScales[i]); return std::string(b); }, restartIfOn));
	l.push_back(MkChoice("Layout", "Still vs moving twins share count, height and scale - only motion differs.", &g_lab.layout, N(4),
		Names({ "Single, still", "Single, circling", "Column, still", "Column, spinning" }), restartIfOn));
	l.push_back(MkAction("Show here", "Shows the effect 18 m ahead.", []() { LabStart(); }));
	l.push_back(MkAction("Slideshow", "Next effect every 6 s. Press the note key on ones you like.", []()
	{
		g_lab.slideshow = true; g_lab.nextSlide = NowSec() + 6.0f; LabStart(); g_menuOpen = false;
	}));
	l.push_back(MkAction("Stop", "Removes the effect.", []() { g_lab.slideshow = false; LabStop(); }));
	l.push_back(MkAction("Rate: VISIBLE", "Logs effect, layout, anchor mode and started/alive counts.", []() { LabRate("VISIBLE"); }));
	l.push_back(MkAction("Rate: NOT visible", "Logs it even though the handle may still be alive.", []() { LabRate("NOT VISIBLE"); }));
	l.push_back(MkAction("Use in style E", "Builds style E (Custom) out of this effect and picks it - spawn one to see it as a tornado.", []()
	{
		SetCustomStyleFx(LabName(), g_lab.kind == 0, kLabScales[g_lab.scaleIdx]);
		g_set.style = (int)GetStyles().size() - 1;
		Notify(std::string("Style E now uses ") + LabName());
	}));

	g_pages[P_TREES].title = "TREE TOOLS";
	g_pages[P_TREES].sub = "By hand";
	auto& tr = g_pages[P_TREES].items;
	tr.push_back(MkChoice("Tree model", "Full-size trees appear here after a scan.", &g_treeModel, []() { return TreeChoiceCount(); }, [](int i) { return TreeChoiceLabel(i); }));
	tr.push_back(MkAction("Plant this tree where you look", "Plants the chosen tree 12 m ahead. The tornado always tears it out when it gets there.", []() { PlantTree(TreeChoiceName(g_treeModel), PointAheadCam(12.0f)); }));
	tr.push_back(MkAction("Uproot test", "Hides map trees ahead, spawns this one, launches it.", []() { UprootTest(); }));
	tr.push_back(MkChoice("Hide radius", "How far round the point ahead 'Hide map trees ahead' reaches.", &g_hideRadius, N(3), Names({ "4 m", "8 m", "15 m" })));
	tr.push_back(MkAction("Hide map trees ahead", "Makes the map's own trees 6 m ahead vanish (the real-trees trick, by hand). Restore brings them back.", []() { HideTreesAt(PointAhead(6.0f), kHideRadii[g_hideRadius]); Notify("Hid trees ahead"); }));
	tr.push_back(MkAction("Restore hidden trees", "Brings back every map tree hidden by hand.", []() { RestoreTrees(); }));
	tr.push_back(MkAction("Delete spawned trees", "Removes the trees planted from this page and the tests.", []() { DeleteTestTrees(); }));
}

// Playtest 4: "I can't move Arthur or anything in the menu" / "I can't move the camera while I'm using the menu".
// Everything the menu might collide with is blocked, then looking around and walking/riding are handed back.
static const char* kKeepWhileMenu[] = {
	"INPUT_LOOK_LR", "INPUT_LOOK_UD", "INPUT_MOVE_LR", "INPUT_MOVE_UD",
	"INPUT_MOVE_LEFT_ONLY", "INPUT_MOVE_RIGHT_ONLY", "INPUT_MOVE_UP_ONLY", "INPUT_MOVE_DOWN_ONLY",
	"INPUT_HORSE_MOVE_LR", "INPUT_HORSE_MOVE_UD", "INPUT_HORSE_MOVE_LEFT_ONLY", "INPUT_HORSE_MOVE_RIGHT_ONLY",
	"INPUT_HORSE_MOVE_UP_ONLY", "INPUT_HORSE_MOVE_DOWN_ONLY",
};
static Hash g_keepHashes[sizeof(kKeepWhileMenu) / sizeof(kKeepWhileMenu[0])];

// menu animation state
struct MenuAnim { float openedAt = -10, pageAt = -10, selY = -1; int lastPage = -1, lastSel = -1; bool wasOpen = false; } g_mAnim;

static void MenuInput()
{
	if (IntroRunning()) { g_menuOpen = false; return; }
	bool toggle = Pressed(g_keys.menu) || (g_padOn && PadComboPressed(g_pad.menu));
	if (toggle)
	{
		g_menuOpen = !g_menuOpen;
		if (g_menuOpen && g_menuStack.empty())
			g_menuStack.push_back({ P_MAIN, 0 });
		if (g_menuOpen)
			g_hour = CLOCK::GET_CLOCK_HOURS();
		UI::Sound(g_menuOpen ? "MENU_ENTER" : "MENU_CLOSE", "HUD_PLAYER_MENU");
	}
	if (!g_menuOpen || g_menuStack.empty())
		return;
	if (!g_keepHashes[0])
		for (size_t i = 0; i < sizeof(kKeepWhileMenu) / sizeof(kKeepWhileMenu[0]); i++)
			g_keepHashes[i] = Joaat(kKeepWhileMenu[i]);
	PAD::DISABLE_ALL_CONTROL_ACTIONS(0);
	for (Hash h : g_keepHashes)
		PAD::ENABLE_CONTROL_ACTION(0, h, TRUE);
	auto& top = g_menuStack.back();
	auto& items = g_pages[top.first].items;
	int n = (int)items.size();
	// While RB (the pad's combo button) is held, the D-pad belongs to the quick combos, not to the menu.
	bool padNav = g_padOn && !PadHeld(g_pad.menu.hold);
	if (Pressed(g_keys.up, true) || (padNav && PadPressed(PB_UP, true))) { top.second = (top.second + n - 1) % n; UI::Sound("NAV_UP", "Ledger_Sounds"); }
	if (Pressed(g_keys.down, true) || (padNav && PadPressed(PB_DOWN, true))) { top.second = (top.second + 1) % n; UI::Sound("NAV_DOWN", "Ledger_Sounds"); }
	Item& it = items[top.second];
	int dir = (Pressed(g_keys.right, true) || (padNav && PadPressed(PB_RIGHT, true))) ? 1
		: ((Pressed(g_keys.left, true) || (padNav && PadPressed(PB_LEFT, true))) ? -1 : 0);
	if (dir && it.kind == Item::Choice)
	{
		int c = std::max(1, it.valueCount());
		*it.value = ((*it.value + dir) % c + c) % c;
		if (it.onChange) it.onChange();
		UI::Sound(dir > 0 ? "NAV_RIGHT" : "NAV_LEFT", "PAUSE_MENU_SOUNDSET");
	}
	if (dir && it.kind == Item::Toggle)
	{
		*it.flag = !*it.flag;
		UI::Sound(dir > 0 ? "NAV_RIGHT" : "NAV_LEFT", "PAUSE_MENU_SOUNDSET");
	}
	if (Pressed(g_keys.select) || (padNav && PadPressed(PB_A)))
	{
		UI::Sound("SELECT", "HUD_SHOP_SOUNDSET");
		switch (it.kind)
		{
		case Item::Action: it.action(); break;
		case Item::Toggle: *it.flag = !*it.flag; break;
		case Item::Choice:
			if (it.onSelect) it.onSelect();
			else { int c = std::max(1, it.valueCount()); *it.value = (*it.value + 1) % c; if (it.onChange) it.onChange(); }
			break;
		case Item::Page: g_menuStack.push_back({ it.page, 0 }); break;
		}
	}
	if (Pressed(g_keys.back) || (padNav && PadPressed(PB_B)))
	{
		UI::Sound("BACK", "HUD_SHOP_SOUNDSET");
		if (g_menuStack.size() > 1) g_menuStack.pop_back();
		else g_menuOpen = false;
	}
}

static const int kHelpLines = 5;   // v1.1 audit 2: the footer's help, up to 5 lines (Simple cut the longer ones off at 3)
static std::vector<std::string> Wrap(const std::string& s, size_t width, int maxLines)
{
	std::vector<std::string> out;
	size_t start = 0;
	while (start < s.size() && (int)out.size() < maxLines)
	{
		if (s.size() - start <= width) { out.push_back(s.substr(start)); break; }
		size_t cut = s.rfind(' ', start + width);
		if (cut == std::string::npos || cut <= start) cut = start + width;
		out.push_back(s.substr(start, cut - start));
		start = cut + 1;
	}
	return out;
}

// v1.0's plain menu (Menu look: Simple)
static void MenuDrawSimple()
{
	auto& top = g_menuStack.back();
	auto& page = g_pages[top.first];
	const float x = 0.02f, y0 = 0.07f, w = 0.34f, lh = 0.029f;
	int n = (int)page.items.size();
	DrawBox(x, y0, w, lh * 1.45f, 135, 22, 18, 240);
	DrawTextLine(page.title.c_str(), x + 0.010f, y0 + 0.007f, 0.40f, 255, 240, 220);
	float by = y0 + lh * 1.45f;
	DrawBox(x, by, w, lh * n + 0.008f, 12, 10, 10, 215);
	for (int i = 0; i < n; i++)
	{
		const Item& it = page.items[i];
		float y = by + 0.004f + lh * i;
		bool sel = i == top.second;
		if (sel) DrawBox(x + 0.003f, y, w - 0.006f, lh - 0.002f, 225, 195, 140, 235);
		int c = sel ? 25 : 232;
		std::string label = it.labelFn ? it.labelFn() : it.label;
		if (it.kind == Item::Page) label += "  >";
		DrawTextLine(label.c_str(), x + 0.012f, y + 0.005f, 0.31f, c, c, c);
		std::string val;
		int vr = sel ? 90 : 240, vg = sel ? 30 : 200, vb = sel ? 20 : 140;
		if (it.kind == Item::Toggle) { val = *it.flag ? "ON" : "OFF"; if (!sel && !*it.flag) { vr = 150; vg = 150; vb = 150; } }
		else if (it.kind == Item::Choice) val = "< " + it.valueText(*it.value) + " >";
		if (!val.empty())
			DrawTextLine(val.c_str(), x + w - 0.012f - 0.0054f * val.size(), y + 0.005f, 0.31f, vr, vg, vb);
	}
	float hy = by + lh * n + 0.012f;
	const Item& sel = page.items[top.second];
	auto lines = Wrap(sel.helpFn ? sel.helpFn() : sel.help, 66, kHelpLines);
	DrawBox(x, hy, w, 0.03f + 0.024f * lines.size(), 12, 10, 10, 200);
	for (size_t k = 0; k < lines.size(); k++)
		DrawTextLine(lines[k].c_str(), x + 0.010f, hy + 0.006f + 0.024f * k, 0.25f, 235, 215, 170);
	std::string hint = PadRecentlyUsed() ? "D-pad move/change   A select   B back" : "Arrows move/change   " + g_keys.select.text + " select   " + g_keys.back.text + " back";
	DrawTextLine(hint.c_str(), x + 0.010f, hy + 0.008f + 0.024f * lines.size(), 0.23f, 160, 160, 160);
}

// The RDR2 look (Halen84's native menu base layout, scaled from his 1920x1080 constants).
static void MenuDraw()
{
	if (!g_menuOpen || g_menuStack.empty())
	{
		g_mAnim.wasOpen = false;
		return;
	}
	float now = NowSec();
	auto& top = g_menuStack.back();
	if (!g_mAnim.wasOpen) { g_mAnim.wasOpen = true; g_mAnim.openedAt = now; g_mAnim.selY = -1; }
	if (top.first != g_mAnim.lastPage) { g_mAnim.lastPage = top.first; g_mAnim.pageAt = now; g_mAnim.selY = -1; }
	if (UI::g_simple) { MenuDrawSimple(); return; }
	auto& page = g_pages[top.first];
	int n = (int)page.items.size();
	float open = Clamp((now - g_mAnim.openedAt) / 0.18f, 0.0f, 1.0f);
	open = 1 - (1 - open) * (1 - open);
	float fade = Clamp((now - g_mAnim.pageAt) / 0.14f, 0.0f, 1.0f);
	int A = (int)(255 * open);
	const float x = 0.016f - (1 - open) * 0.04f, w = 0.27f, cx = x + w / 2;
	const float headH = 0.088f, subH = 0.034f, lh = 0.0415f;
	const int maxRows = 13;
	int first = std::max(0, std::min(top.second - maxRows / 2, n - maxRows));
	int shown = std::min(n, maxRows);
	const Item& selItem = page.items[top.second];
	auto help = Wrap(selItem.helpFn ? selItem.helpFn() : selItem.help, 58, kHelpLines);
	float rowsY = 0.035f + headH + subH;
	float footY = rowsY + lh * shown + 0.008f;
	float totalH = footY - 0.022f + 0.02f + 0.026f * help.size() + 0.04f;
	UI::Panel(x - 0.006f, 0.022f, w + 0.012f, totalH, (int)(232 * open));
	// header banner + title
	UI::Sprite("generic_textures", "menu_header_1a", cx, 0.035f + headH / 2, w * 0.92f, headH, 255, 255, 255, A);
	UI::Text(page.title.c_str(), cx, 0.035f + headH * 0.2f, 0.78f, 245, 242, 232, A, UI::CENTRE, "title", true);
	// sub-header: where you are, and how many
	float sy = 0.035f + headH + 0.006f;
	UI::Text(page.sub.c_str(), x + 0.01f, sy, 0.27f, 205, 195, 175, A, UI::LEFT, "body");
	char cnt[24]; sprintf_s(cnt, "%d / %d", top.second + 1, n);
	UI::Text(cnt, x + w - 0.01f, sy, 0.27f, 160, 152, 138, A, UI::RIGHT, "body");
	if (!UI::Sprite("generic_textures", "menu_bar", cx, sy + 0.026f, w - 0.016f, 0.0018f, 255, 255, 255, A * 2 / 3))
		DrawBox(x + 0.008f, sy + 0.025f, w - 0.016f, 0.0016f, 200, 190, 170, A * 2 / 3);
	// rows
	int sel = top.second - first;
	float wantSelY = rowsY + lh * sel + lh / 2;
	if (g_mAnim.selY < 0) g_mAnim.selY = wantSelY;
	g_mAnim.selY += (wantSelY - g_mAnim.selY) * Clamp(UIdt() * 18.0f, 0.0f, 1.0f);
	int RA = (int)(A * fade);
	for (int r = 0; r < shown; r++)
	{
		int i = first + r;
		const Item& it = page.items[i];
		float cy = rowsY + lh * r + lh / 2;
		bool isSel = i == top.second;
		UI::Sprite("generic_textures", "selection_box_bg_1c", cx, cy, w - 0.012f, lh * 0.92f, 50, 50, 50, isSel ? RA * 3 / 4 : RA * 2 / 5);
		std::string label = it.labelFn ? it.labelFn() : it.label;
		int lc = isSel ? 255 : 212;
		UI::Text(label.c_str(), x + 0.014f, cy - 0.0135f, 0.34f, lc, lc, isSel ? 250 : 205, RA, UI::LEFT, "body");
		float vx = x + w - 0.014f;
		if (it.kind == Item::Toggle)
		{
			float s = 0.016f;
			if (!UI::Sprite("generic_textures", "tick_box", vx - s / 2, cy, s, s * UI::Aspect(), 255, 255, 255, RA))
				UI::Text(*it.flag ? "ON" : "OFF", vx, cy - 0.0135f, 0.32f, 230, 200, 140, RA, UI::RIGHT, "body");
			else if (*it.flag)
				UI::Sprite("generic_textures", "tick", vx - s / 2, cy, s, s * UI::Aspect(), 255, 255, 255, RA);
		}
		else if (it.kind == Item::Choice)
		{
			std::string v = it.valueText(*it.value);
			float colW = 0.105f, ccx = vx - colW / 2;
			UI::Text(v.c_str(), ccx, cy - 0.0135f, 0.32f, isSel ? 255 : 225, isSel ? 245 : 200, isSel ? 230 : 150, RA, UI::CENTRE, "body");
			if (isSel)
			{
				float s = 0.011f;
				if (!UI::Sprite("menu_textures", "selection_arrow_left", vx - colW + s / 2, cy, s, s * UI::Aspect(), 255, 255, 255, RA))
					UI::Text("<", vx - colW, cy - 0.0135f, 0.32f, 255, 255, 255, RA);
				UI::Sprite("menu_textures", "selection_arrow_right", vx - s / 2, cy, s, s * UI::Aspect(), 255, 255, 255, RA);
			}
		}
		else if (it.kind == Item::Page)
		{
			float s = 0.011f;
			if (!UI::Sprite("menu_textures", "selection_arrow_right", vx - s / 2, cy, s, s * UI::Aspect(), 230, 220, 200, RA))
				UI::Text(">", vx, cy - 0.0135f, 0.32f, 230, 220, 200, RA, UI::RIGHT);
		}
	}
	// the red frame round the selected row (it glides between rows)
	{
		float fy = g_mAnim.selY, fw = w - 0.008f, fh = lh * 1.02f, e = 0.006f;
		bool ok = UI::Sprite("menu_textures", "crafting_highlight_t", cx, fy - fh / 2, fw + e, e * 2.2f, 204, 0, 0, A);
		if (ok)
		{
			UI::Sprite("menu_textures", "crafting_highlight_b", cx, fy + fh / 2, fw + e, e * 2.2f, 204, 0, 0, A);
			UI::Sprite("menu_textures", "crafting_highlight_l", cx - fw / 2, fy, e * 1.8f, fh + e, 204, 0, 0, A);
			UI::Sprite("menu_textures", "crafting_highlight_r", cx + fw / 2, fy, e * 1.8f, fh + e, 204, 0, 0, A);
		}
		else
			DrawBox(x + 0.004f, fy - fh / 2, 0.003f, fh, 204, 30, 20, A);
	}
	if (n > maxRows)
	{
		if (first > 0) UI::Text("...", cx, rowsY - 0.016f, 0.3f, 200, 190, 170, A, UI::CENTRE);
		if (first + shown < n) UI::Text("...", cx, rowsY + lh * shown - 0.01f, 0.3f, 200, 190, 170, A, UI::CENTRE);
	}
	// footer: what the selected item does, then the keys
	if (!UI::Sprite("generic_textures", "menu_bar", cx, footY, w - 0.016f, 0.0018f, 255, 255, 255, A * 2 / 3))
		DrawBox(x + 0.008f, footY, w - 0.016f, 0.0016f, 200, 190, 170, A * 2 / 3);
	for (size_t k = 0; k < help.size(); k++)
		UI::Text(help[k].c_str(), x + 0.012f, footY + 0.008f + 0.026f * k, 0.285f, 232, 222, 200, A, UI::LEFT, "body");
	std::string hint = PadRecentlyUsed()
		? "D-pad  move / change      A  select      B  back"
		: "Arrows  move / change      " + g_keys.select.text + "  select      " + g_keys.back.text + "  back";
	UI::Text(hint.c_str(), x + 0.012f, footY + 0.012f + 0.026f * help.size(), 0.25f, 150, 144, 132, A, UI::LEFT, "body");
}

// ======================= overlay & logging =======================
// RDR2-style zone banner (dark red plate, cream serif caps) + a danger meter: which part of the storm you're in.
static void ZoneBannerDraw(Tornado* t, float dist)
{
	if (!g_set.zoneBanner || !t || dist > t->reachRadius())
		return;
	float wall = t->wallRadius(), reach = t->reachRadius();
	const char* title; char sub[96]; int r, g, b;
	float danger;
	if (t->PlayerInEye())
	{
		title = "THE EYE"; sprintf_s(sub, "calm - the only safe place in the storm"); r = 40; g = 90; b = 55; danger = 0.15f;
	}
	else if (dist < wall * 1.6f)
	{
		title = "THE WALL"; sprintf_s(sub, "hold on to your hat"); r = 150; g = 20; b = 16; danger = 1.0f;
	}
	else
	{
		title = "THE PULL"; sprintf_s(sub, "%.0f m to the wall - it's dragging things in", std::max(0.0f, dist - wall * 1.6f)); r = 120; g = 70; b = 20;
		danger = Clamp(1.0f - (dist - wall * 1.6f) / std::max(1.0f, reach - wall * 1.6f), 0.0f, 1.0f) * 0.85f;
	}
	const float x = 0.36f, y = 0.83f, w = 0.28f;
	DrawBox(x, y, w, 0.075f, 18, 12, 10, 200);
	DrawBox(x, y, w, 0.004f, r, g, b, 255);
	DrawTextLine(title, x + 0.012f, y + 0.010f, 0.46f, 240, 225, 195);
	DrawTextLine(sub, x + 0.012f, y + 0.045f, 0.24f, 200, 185, 160);
	// danger meter on the right side of the plate
	float mx = x + w * 0.62f, mw = w * 0.34f, my = y + 0.022f;
	DrawBox(mx, my, mw, 0.012f, 60, 50, 42, 230);
	DrawBox(mx, my, mw * danger, 0.012f, r + 60 > 255 ? 255 : r + 60, g, b, 255);
}

// Playtest 4: "I can't even find it" / "it's behind us, we didn't see it". When the nearest tornado is off screen,
// an arrow at the edge says where it is.
static void TornadoPointer(Tornado* t, float dist)
{
	if (!t || dist < 30.0f || g_drone.active)
		return;
	V3 lp = t->base + V3(0, 0, t->height() * 0.45f);
	float sx, sy;
	if (GRAPHICS::GET_SCREEN_COORD_FROM_WORLD_COORD(lp.x, lp.y, lp.z, &sx, &sy))
		return;
	V3 d = t->base - PlayerPos();
	float bearing = atan2f(-d.x, d.y) * 180.0f / PI;
	float rel = fmodf(bearing - CameraHeading() + 540.0f, 360.0f) - 180.0f;   // + = to the left
	char b[64];
	if (fabsf(rel) > 135.0f)
	{
		sprintf_s(b, "TORNADO BEHIND YOU  -  %.0f m", dist);
		DrawBox(0.40f, 0.765f, 0.20f, 0.034f, 18, 12, 10, 190);
		DrawTextLine(b, 0.41f, 0.770f, 0.28f, 240, 190, 150);
	}
	else if (rel > 0)
	{
		sprintf_s(b, "<  TORNADO  %.0f m", dist);
		DrawBox(0.01f, 0.46f, 0.13f, 0.034f, 18, 12, 10, 190);
		DrawTextLine(b, 0.017f, 0.465f, 0.28f, 240, 190, 150);
	}
	else
	{
		sprintf_s(b, "TORNADO  %.0f m  >", dist);
		DrawBox(0.86f, 0.46f, 0.13f, 0.034f, 18, 12, 10, 190);
		DrawTextLine(b, 0.867f, 0.465f, 0.28f, 240, 190, 150);
	}
}

// v0.5 storm tracker (HUD "Tracker", the new default). Playtest 5: "the added UI is very nice. Could be a little bit
// more subtle though" and "the UI up at the top and the 'C Dark Column' labeling would be gone". Everything you need to
// track the tornado in one slim plate at the top centre, out of the way of the webcam corner:
//   - a compass strip: the marker shows where the tornado is relative to where you're looking (<< / >> when it's behind);
//   - one line: the zone you're in (or TORNADO), the distance, and whether it's closing in, moving away or holding;
//   - the plate's top edge fills up as the danger grows, coloured by zone.
float RelBearing(const V3& from, const V3& to, float camHeading)   // degrees, + = to the left, in (-180, 180]
{
	V3 d = to - from;
	float bearing = atan2f(-d.x, d.y) * 180.0f / PI;
	return fmodf(bearing - camHeading + 720.0f + 180.0f, 360.0f) - 180.0f;
}

static void TrackerDraw(Tornado* t, float dist, float dt)
{
	static int lastUid = 0;   // v1.1 audit 2: the id, not the address (a new tornado can reuse an old one's address)
	static float lastD = 0, closing = 0, shownAt = 0;
	if (!t) { lastUid = 0; return; }
	if (t->uid != lastUid) { lastUid = t->uid; lastD = dist; closing = 0; shownAt = NowSec(); }
	else if (dt > 0)
	{
		closing = Lerp(closing, (lastD - dist) / dt, Clamp(dt * 1.5f, 0.0f, 1.0f));
		lastD = dist;
	}
	float in = Clamp((NowSec() - shownAt) / 0.4f, 0.0f, 1.0f);   // v1.1: fades and drops in
	int A = (int)(255 * in);
	float drop = (1 - in) * -0.02f;
	float wallOuter = t->wallRadius() * 1.6f, reach = t->reachRadius();
	const char* zone = nullptr;
	int zr = 230, zg = 215, zb = 180;
	float danger;
	if (t->PlayerInEye()) { zone = "THE EYE"; zr = 70; zg = 175; zb = 95; danger = 0.15f; }
	else if (dist < wallOuter) { zone = "THE WALL"; zr = 220; zg = 50; zb = 35; danger = 1.0f; }
	else if (dist < reach) { zone = "THE PULL"; zr = 230; zg = 135; zb = 45; danger = 0.35f + 0.6f * Clamp(1.0f - (dist - wallOuter) / std::max(1.0f, reach - wallOuter), 0.0f, 1.0f); }
	else danger = Clamp(0.35f * (1.0f - (dist - reach) / std::max(1.0f, reach * 2.0f)), 0.03f, 0.35f);

	const float cx = 0.5f, w = 0.30f, x0 = cx - w / 2, y0 = 0.010f + drop, h = 0.068f;
	// the game's soft help-text plate (a plain gradient if the texture isn't there)
	if (!UI::Sprite("feeds", "help_text_bg", cx, y0 + h / 2, w * 1.3f, h * 1.45f, 0, 0, 0, A * 4 / 5))
	{
		for (int k = 0; k < 6; k++)
			DrawBox(x0, y0 + h * k / 6.0f, w, h / 6.0f + 0.0005f, 14, 10, 8, (200 - k * 14) * A / 255);
	}
	DrawBox(x0 + 0.01f, y0 + h - 0.0015f, w - 0.02f, 0.0015f, 205, 185, 150, 110 * A / 255);
	DrawBox(cx - w * danger / 2, y0, w * danger, 0.003f, zr, zg, zb, A);   // the danger rule grows from the centre
	// compass strip: ticks at -90, -45, 0, 45, 90 degrees from where you look
	const float half = 0.13f, sy = y0 + 0.013f;
	DrawBox(cx - half, sy + 0.005f, half * 2, 0.0018f, 120, 105, 90, 200 * A / 255);
	for (int k = -2; k <= 2; k++)
		DrawBox(cx + k * half * 0.5f - 0.0008f, sy + (k == 0 ? 0.0f : 0.002f), 0.0016f, k == 0 ? 0.012f : 0.008f, 175, 155, 130, 220 * A / 255);
	float rel = RelBearing(PlayerPos(), t->base, CameraHeading());
	bool behind = fabsf(rel) > 90.0f;
	float mx = cx - Clamp(rel, -90.0f, 90.0f) / 90.0f * half;
	if (!UI::Sprite("BLIPS", "blip_rc_lightning", mx, sy + 0.006f, 0.016f, 0.016f * UI::Aspect(), zr, zg, zb, A))
		DrawBox(mx - 0.004f, sy - 0.003f, 0.008f, 0.018f, zr, zg, zb, A);
	if (behind)
	{
		float ax = rel > 0 ? cx - half - 0.02f : cx + half + 0.02f, s = 0.012f;
		if (!UI::Sprite("menu_textures", rel > 0 ? "selection_arrow_left" : "selection_arrow_right", ax, sy + 0.006f, s, s * UI::Aspect(), zr, zg, zb, A))
			DrawTextLine(rel > 0 ? "<<" : ">>", ax - 0.008f, sy - 0.006f, 0.30f, zr, zg, zb, A);
	}
	// v0.6, playtest 6 ("when it's behind us... make it flashing on the side, the little tornado icon"): out of view, a
	// small funnel icon sits at that edge of the screen with the distance, flashing while it's closing in or has you.
	if (fabsf(rel) > 50.0f)
	{
		bool urgent = closing > 0.6f || dist < reach;
		int a = urgent ? (((int)(NowSec() * 3.0f)) % 2 ? 255 : 70) : 190;
		a = a * A / 255;
		float ix = rel > 0 ? 0.035f : 0.965f, iy = 0.42f;
		static const float kW[5] = { 0.034f, 0.026f, 0.019f, 0.012f, 0.006f };
		for (int k = 0; k < 5; k++)
		{
			float sway = sinf(NowSec() * 4.0f + k) * 0.002f;
			DrawBox(ix - kW[k] / 2 + sway, iy + k * 0.0105f, kW[k], 0.008f, zr, zg, zb, a);
		}
		char dm[24]; sprintf_s(dm, "%.0f m", dist);
		UI::Text(dm, ix, iy + 0.056f, 0.27f, zr, zg, zb, a, UI::CENTRE, "body", true);
	}
	char motion[48];
	if (closing > 0.6f) sprintf_s(motion, "closing %.0f m/s", closing);
	else if (closing < -0.6f) sprintf_s(motion, "moving away");
	else sprintf_s(motion, "holding");
	char line[200];
	if (zone) sprintf_s(line, "%s      %.0f m      %s", zone, dist, motion);
	else sprintf_s(line, "%s      %.0f m      %s%s", t->natural ? "WILD TWISTER" : "TORNADO", dist, motion, behind ? "      BEHIND YOU" : "");
	int big = 0;
	for (auto& tp : g_tornadoes) if (!tp->Mini()) big++;
	if (big > 1) { char more[24]; sprintf_s(more, "      (+%d)", big - 1); strcat_s(line, more); }
	UI::Text(line, cx, y0 + 0.031f, 0.31f, zone ? zr : 238, zone ? zg : 228, zone ? zb : 205, A, UI::CENTRE, "title");
}

static void OverlayDraw(float dt)
{
	if (IntroRunning())
		return;   // the intro draws its own (letterbox, subtitles)
	if (g_set.cinematic)
	{
		HUD::HIDE_HUD_AND_RADAR_THIS_FRAME();
		return;
	}
	bool full = g_set.hud == 0, off = g_set.hud == 2;
	if (full)
		for (auto& tp : g_tornadoes)
		{
			V3 lp = tp->base + V3(0, 0, tp->height() * 0.55f);
			float sx, sy;
			if (GRAPHICS::GET_SCREEN_COORD_FROM_WORLD_COORD(lp.x, lp.y, lp.z, &sx, &sy))
				DrawTextLine(tp->label.c_str(), sx - 0.03f, sy, 0.34f, 255, 230, 120);
		}
	// Playtest 3: "I don't even know what it's testing" - big title card at the top for auto tests (every HUD mode).
	if (g_auto.running && g_auto.idx >= 0 && g_auto.idx < (int)g_auto.steps.size())
	{
		char l1[160], l2[240];
		sprintf_s(l1, "TEST %d / %d  -  %s", g_auto.idx + 1, (int)g_auto.steps.size(), g_auto.name.c_str());
		if (g_padCheck)
			sprintf_s(l2, "source: %s   seen:%s   (%.0fs left)", PadSource(), g_padSeen.empty() ? " nothing yet" : g_padSeen.c_str(), std::max(0.0f, g_auto.stepEnds - NowSec()));
		else
			sprintf_s(l2, "%s   (%.0fs left)", g_auto.steps[g_auto.idx].desc.c_str(), std::max(0.0f, g_auto.stepEnds - NowSec()));
		DrawBox(0.18f, 0.115f, 0.64f, 0.085f, 18, 12, 10, 215);
		DrawBox(0.18f, 0.115f, 0.64f, 0.005f, 150, 20, 16, 255);
		DrawTextLine(l1, 0.195f, 0.127f, 0.42f, 240, 225, 195);
		DrawTextLine(l2, 0.195f, 0.167f, 0.27f, 220, 200, 160);
	}
	if (g_lab.on)
	{
		char b[160];
		sprintf_s(b, "FX LAB %s %d/%d: %s  (scale %.1f, %s)  started %d alive %d%s", g_lab.kind ? "PUFF" : "LOOP", g_lab.idx + 1,
			LabCount(), LabName(), kLabScales[g_lab.scaleIdx], kLabLayouts[g_lab.layout % 4], g_lab.ok, LabAlive(),
			g_lab.slideshow ? "  - slideshow" : "");
		DrawBox(0.22f, 0.75f, 0.56f, 0.045f, 10, 30, 60, 190);
		DrawTextLine(b, 0.23f, 0.758f, 0.32f, 200, 230, 255);
	}
	if (g_drone.active)
	{
		char b[160];
		sprintf_s(b, "CAMERA: %s   -   %s / pad %s: next view (then back to normal)", kDroneModes[g_drone.mode % 4], g_keys.drone.text.c_str(), g_pad.drone.text.c_str());
		DrawBox(0.015f, 0.935f, 0.43f, 0.03f, 18, 12, 10, 170);
		DrawTextLine(b, 0.022f, 0.94f, 0.25f, 235, 215, 170);
	}
	float nd;
	Tornado* nearest = NearestTornado(&nd);
	if (!off && !g_drone.active && !g_tc.on)
		TrackerDraw(nearest, nd, dt);
	if (!g_drone.active)
	{
		SurvivalDraw();
		ChaseDraw();
	}
	if (!full)
		return;
	ZoneBannerDraw(nearest, nd);
	TornadoPointer(nearest, nd);
	static float fps = 60;
	if (dt > 0) fps = Lerp(fps, 1.0f / dt, 0.05f);
	char l1[220], l2[260], l3[260];
	sprintf_s(l1, "%s | tornadoes %d | fps %.0f | effects %d/%d | puffs x%.2f | trees %d (hides %d) | grass %d | roar %.2f | pad %s", kVersion,
		(int)g_tornadoes.size(), fps, LoopsInUse(), g_set.ptfxBudget, PuffRateScale(), RealTreesTotal(), TreeHidesUsed(), VegTrailCount(), Roar::Level(), PadSource());
	if (nearest)
		sprintf_s(l2, "%s %.0fm | grabbing %d | in wall %d, eye %d | reach: %d people, %d wagons, %d props | uprooted %d | flings %d | ground %s",
			nearest->label.c_str(), nd, nearest->grabbedLastFrame, nearest->counts.inWall, nearest->counts.inEye,
			nearest->counts.peds, nearest->counts.vehicles, nearest->counts.objects, nearest->uprooted, nearest->flings, nearest->groundOk ? "ok" : "LOST");
	else
		sprintf_s(l2, "no tornado - %s / pad %s spawns, %s despawns", g_keys.spawn.text.c_str(), g_pad.spawn.text.c_str(), g_keys.despawn.text.c_str());
	static const char* kSpeedNames[] = { "slow", "normal", "fast" };
	const LandingStats& L = GetLanding();
	if (nearest)
		sprintf_s(l3, "%s, %s | eye %.0fm wall %.0fm reach %.0fm | pools p%d v%d o%d (empty %d, top-ups %d) | floaters woken %d | landings %d (%.1f m/s)",
			CurrentForce().name, kSpeedNames[g_set.speed % 3], nearest->eyeRadius(), nearest->wallRadius(), nearest->reachRadius(),
			nearest->rawPeds, nearest->rawVehs, nearest->rawObjs, PoolEmptyReads(), PoolTopUps(), HoverWoken(), L.landings, L.lastImpact);
	else
		sprintf_s(l3, "%s, %s | size %d | reach %d | move %d | landings %d", CurrentForce().name, kSpeedNames[g_set.speed % 3], g_set.size, g_set.reach, g_set.movement, L.landings);
	// v0.5: below the tracker and the test card, right of the menu
	float x = std::max(g_set.overlayX, 0.37f), y = 0.205f;
	DrawBox(x, y, 0.44f, 0.08f, 0, 0, 0, 150);
	DrawTextLine(l1, x + 0.005f, y + 0.004f, 0.25f);
	DrawTextLine(l2, x + 0.005f, y + 0.029f, 0.25f, 255, 220, 160);
	DrawTextLine(l3, x + 0.005f, y + 0.054f, 0.25f, 200, 200, 200);
}

static void SaveNote()
{
	if (g_lab.on)
	{
		LabRate(g_lab.slideshow ? "LIKED (slideshow)" : "LIKED");
		return;
	}
	std::string auto_ = AutoStatus();
	float nd;
	Tornado* nearest = NearestTornado(&nd);
	Finding("NOTE %s | style %s strength %s speed %d size %d reach %d render %d | tornadoes %d, nearest %.0fm grabbing %d",
		auto_.empty() ? "(free play)" : auto_.c_str(), GetStyles()[g_set.style].name, CurrentForce().name, g_set.speed, g_set.size, g_set.reach,
		g_set.render, (int)g_tornadoes.size(), nearest ? nd : -1.0f, nearest ? nearest->grabbedLastFrame : 0);
	for (auto& tp : g_tornadoes) tp->Snapshot("NOTE");
	Notify("Note saved");
}

// Playtest 4: after Arthur died the tornado chased him into Rhodes and the mod's labels stayed on the death screen.
static bool g_wasDead = false;
static bool WatchPlayerDeath(float t)
{
	bool dead = PlayerDead();
	if (dead && !g_wasDead)
	{
		Log("Arthur died (%d tornadoes alive)", (int)g_tornadoes.size());
		if (g_set.dieWithArthur)
			for (auto& tp : g_tornadoes) tp->BeginDissipate(t);
		g_menuOpen = false;
	}
	g_wasDead = dead;
	return dead;
}

// Script Hook restarts the script after a story reload (playtest 4 did one at 13:32) but our statics survive. The
// handles in them belong to the old session and may now point at other entities, so drop them without natives.
static void ResetAfterScriptRestart()
{
	static int starts = 0;
	if (starts++ == 0)
		return;
	Log("script restarted by Script Hook (story reload?) - forgetting %d tornadoes and every handle from the old session", (int)g_tornadoes.size());
	// v1.1 audit: the modes give back the settings they changed, and the global switches go back to the game
	if (g_in.stage >= 1) IntroRestoreSettings();   // (v1.7: the run for the balloon changed Arthur's setting too)
	if (g_bal.chase) g_set.movement = g_bal.savedMove;
	if (g_surv.on) { g_set.arthur = g_surv.savedArthur; g_set.movement = g_surv.savedMove; g_set.speed = g_surv.savedSpeed; g_set.flingChase = g_surv.savedFling; }
	if (g_storm.active || g_in.stage)
	{
		MISC::SET_WIND_SPEED(-1.0f);
		MISC::SET_WIND_DIRECTION(-1.0f);
		MISC::SET_RAIN(-1.0f);
		GRAPHICS::CLEAR_TIMECYCLE_MODIFIER();
		GRAPHICS::ANIMPOSTFX_STOP("DEADEYE");
		if (g_storm.godApplied) PLAYER::SET_PLAYER_INVINCIBLE(PLAYER::PLAYER_ID(), FALSE);
	}
	g_tornadoes.clear();          // ~Tornado() makes no native calls
	ForgetAllAnchors();
	ForgetWorldState();
	g_testTrees.clear();
	g_hidden.clear();
	g_lab = FxLab();
	g_rig = RenderRig();
	g_auto = AutoTest();
	g_scheduled.clear();
	g_holdStorm = false;
	g_padCheck = false;
	bool wasLocked = g_lockedHash != 0;
	g_storm = Storm();
	if (wasLocked) UnlockWeather();
	if (g_drone.active || g_tc.on || g_in.cam || g_in.oldCam) CAMERA::RENDER_SCRIPT_CAMS(FALSE, FALSE, 0, TRUE, FALSE, 0);   // (audit 2: the intro's too)
	g_drone = Drone();
	if (g_ride.on) CAMERA::RENDER_SCRIPT_CAMS(FALSE, FALSE, 0, TRUE, FALSE, 0);
	g_tc = TouchCam();
	g_spin = SpinRig();
	g_ride = RideCam();
	g_surv = Survival();
	g_menuOpen = false;
	g_wasDead = false;
	// v1.1 (no natives on old handles; global switches are fine to reset)
	if (g_in.stage) { CLOCK::PAUSE_CLOCK(FALSE, 0); MISC::SET_TIME_SCALE(1.0f); PLAYER::SET_PLAYER_CONTROL(PLAYER::PLAYER_ID(), TRUE, 0, FALSE); HUD::DISPLAY_HUD(TRUE); MAP::DISPLAY_RADAR(TRUE); AUDIO::SET_AUDIO_FLAG("AllowScriptedSpeechInSlowMo", FALSE); AUDIO::SET_AUDIO_FLAG("DisableAbortConversationForDeathAndInjury", FALSE); }
	// v1.1 audit 2: Arthur himself (a fresh handle) - the intro stops him ragdolling, the balloon sets "don't ragdoll out"
	if (g_in.stage >= 1 && g_in.stage <= 4) PED::SET_PED_CAN_RAGDOLL(PLAYER::PLAYER_PED_ID(), TRUE);
	if (g_bal.on) PED::SET_PED_CONFIG_FLAG(PLAYER::PLAYER_PED_ID(), 15, FALSE);
	g_in = IntroState();
	g_introFx.clear();
	g_introLeft.clear();
	g_bal = BalloonState();
	g_season = SeasonState();
	g_gun.minis.clear();
	g_gun.victims.clear();   // (v1.5: the old session's peds are gone)
	g_manualWeather = false;
	UI::Letterbox(-1);
	if (g_music.fired && !MISC::GET_MISSION_FLAG()) AUDIO::TRIGGER_MUSIC_EVENT("STOP_MUSIC_8S");   // (audit 2: only if ours played)
	g_music = MusicState();
	g_treePins.clear();           // v1.1 audit 2: the map tree check's 6 s timer went with g_scheduled ("still running" for ever)
	g_treeChecked = 0;
	// v1.2: the gallery's camera and Arthur's controls (its exhibits and swatches went with the world state)
	if (g_gal.cam) CAMERA::RENDER_SCRIPT_CAMS(FALSE, FALSE, 0, TRUE, FALSE, 0);
	if (g_gal.mode) PLAYER::SET_PLAYER_CONTROL(PLAYER::PLAYER_ID(), TRUE, 0, FALSE);
	g_gal = Gallery();
	g_vo = VoiceState();
	g_stats = StormStats();
}

// v0.6: the menu's simple choices drive the older switches
static void SyncMenuChoices()
{
	g_set.grabPlayer = g_set.arthur > 0;
	static const int kWeatherFor[] = { 0, 1, 0, 2 };   // keep yours, storm clouds = THUNDER (v1.1; OVERCASTDARK never looked dark), thunderstorm, rain
	g_set.weather = g_set.weatherMode > 0;
	if (g_set.weather) g_set.weatherType = kWeatherFor[g_set.weatherMode % 4];
	g_shieldPlayer = IntroShieldsPlayer() || BalloonActive();   // v1.1: hands off Arthur in the intro and in the balloon (v1.7: and 10 s after it)
	g_set.softLanding = g_set.landings != 2;             // v1.3: Landings: Real = no catch
	UI::g_clean = g_set.cinematic;                         // v1.1 audit 2: UI::Draw() runs every frame, so it hides its labels itself
}

// ======================= v1.1: sound =======================
// The tornado's roar (roar.cpp): the loudest tornado wins - by distance from the camera (what you hear), panned to its side.
static void SoundUpdate(float t)
{
	// v1.4 (playtest 12: "the wind sound... should be quiet and it should be quieter. normal can be quieter too"): Quiet is the
	// default and half what it was; Normal and Loud come down too
	static const float kLevels[] = { 0.0f, 0.22f, 0.45f, 0.75f };
	float vol = 0, pan = 0, inten = 0;
	if (g_roarLevel > 0)
	{
		// v1.1 audit 2: the camera that's actually drawn (the intro's, the drone's), not the gameplay camera behind it
		V3 cam = CAMERA::GET_FINAL_RENDERED_CAM_COORD();
		float camH = V3(CAMERA::GET_FINAL_RENDERED_CAM_ROT(2)).z;
		for (auto& tp : g_tornadoes)
		{
			float d = (tp->base - cam).len2d();
			float k = Clamp(1.0f - std::max(0.0f, d - tp->wallRadius()) / (tp->reachRadius() * 2.5f + 220.0f), 0.0f, 1.0f);
			k = k * k * Clamp(0.25f + 0.75f * tp->growth, 0.0f, 1.0f);
			if (tp->Mini()) k *= tp->Tiny() ? 0.15f : 0.45f;   // (v1.4: a pocket twister is a whisper)
			if (k > vol)
			{
				vol = k;
				float rel = RelBearing(cam, tp->base, camH);   // + = to the left
				pan = -sinf(rel * PI / 180.0f);
				inten = Clamp(0.3f + 0.7f * k, 0.0f, 1.0f) * (g_set.force == 0 ? 0.7f : g_set.force == 2 ? 1.0f : 0.85f);
			}
		}
	}
	if (IntroRunning()) vol *= 0.45f;   // v1.4: the intro's lines come first (playtest 12: the storm drowned them)
	Roar::Update(vol * kLevels[g_roarLevel % 4], pan, inten);
	(void)t;
}

// ======================= main loop =======================
void ScriptMain()
{
	srand(GetTickCount());
	ResetAfterScriptRestart();
	CarryOverOldFiles();
	LoadConfig();
	PadInit();
	LoadTreeCache();
	BuildStandIns();
	BuildMenu();
	Log("%s running. %d hideable tree models, %d scan names, %d looped + %d puff FX Lab effects, %d styles", kVersion,
		(int)(sizeof(kTreeModels) / sizeof(kTreeModels[0])), (int)(sizeof(kTreeScan) / sizeof(kTreeScan[0])),
		kLabLoopedCount, kLabPuffCount, (int)GetStyles().size());
	// Astra: log exactly which build is running and from where (playtest 3 ran an old build by accident).
	Log("BUILD %s compiled %s %s | module dir %s | ini %s", kVersion, __DATE__, __TIME__, ModuleDir().c_str(),
		IniPath().c_str());
	PreloadTornadoAssets();
	UI::Preload();
	Notify(std::string(kVersion) + " loaded - menu: " + g_keys.menu.text + " / pad " + g_pad.menu.text + "   clean footage: " +
		g_keys.cinematic.text + " / pad " + g_pad.cinematic.text, 8000);
	// both would run at once (two menus, two sets of tornadoes): say so until the old one is gone
	if (FileThere(ModuleDir() + "\\NadoTest.asi"))
	{
		Log("OLD NadoTest.asi is still in the game folder - both mods are loaded");
		UI::HelpTip("~COLOR_RED~NadoTest.asi~s~ is still in your game folder. It's the old name of this mod: delete it (and NadoTest.ini) "
			"so only Tornado Redemption runs.", 20.0f);
	}
	float nextSnap = 0, nextOrphanRetry = 0;
	bool clockPaused = false;
	DWORD lastLoopTick = GetTickCount();
	DWORD worstLoopMs = 0;

	while (true)
	{
		SyncMenuChoices();
		InputBeginFrame();
		float dt = MISC::GET_FRAME_TIME();
		float t = NowSec();
		g_frameDt = dt;
		PadBeginFrame(t);
		UI::Frame(dt);
		// v1.1, playtest 9 ("the game almost seems to have crashed... some hitching": one 735 ms frame): each part of the
		// frame is timed, and any frame over 100 ms logs which part took the time
		LARGE_INTEGER qf, q0, q1;
		QueryPerformanceFrequency(&qf);
		double phase[8] = {};
		auto lap = [&](int i) { QueryPerformanceCounter(&q1); phase[i] += (q1.QuadPart - q0.QuadPart) * 1000.0 / qf.QuadPart; q0 = q1; };
		QueryPerformanceCounter(&q0);

		bool dead = WatchPlayerDeath(t);
		bool hideUi = dead || HUD::IS_PAUSE_MENU_ACTIVE() || CAMERA::IS_SCREEN_FADED_OUT() || SCRIPT::IS_LOADING_SCREEN_VISIBLE();

		MenuInput();
		bool scene = IntroRunning() || GalleryActive();   // no quick keys while the intro plays (Backspace / B skips it) or in a gallery
		if (!scene && Pressed(g_keys.spawn))
			UserSpawn();
		if (!scene && Pressed(g_keys.despawn))
			ClearEverything();
		if (Pressed(g_keys.bookmark) && !GalleryActive())   // (the texture gallery saves its own pick)
			SaveNote();
		if (Pressed(g_keys.cinematic))
		{
			g_set.cinematic = !g_set.cinematic;
			if (g_set.cinematic) g_menuOpen = false;
			Log("cinematic mode %s", g_set.cinematic ? "on" : "off");
		}
		if (!scene && Pressed(g_keys.drone))
			DroneCycle();
		if (!g_menuOpen && g_auto.running && Pressed(g_keys.next))
			g_autoSkip = true;
		// v0.6: no quick combos during the controller check (playtest 6: RB then Y switched the drone on by accident)
		if (g_padOn && !g_menuOpen && !g_padCheck && !scene)
		{
			if (PadComboPressed(g_pad.spawn)) UserSpawn();
			if (PadComboPressed(g_pad.despawn)) ClearEverything();
			if (PadComboPressed(g_pad.cinematic))
			{
				g_set.cinematic = !g_set.cinematic;
				Log("cinematic mode %s (pad)", g_set.cinematic ? "on" : "off");
			}
			if (PadComboPressed(g_pad.drone)) DroneCycle();
			if (PadComboPressed(g_pad.note)) SaveNote();
			if (g_auto.running && PadComboPressed(g_pad.next)) g_autoSkip = true;
		}
		if (g_padCheck)
			for (int b = 0; b < PB_COUNT; b++)
				if (PadPressed(b) && g_padSeen.find(PadButtonName(b)) == std::string::npos)
				{
					g_padSeen += std::string(" ") + PadButtonName(b);
					Log("PAD CHECK: %s pressed (%s)", PadButtonName(b), PadSource());
				}
		if (g_freezeTime != clockPaused)
		{
			clockPaused = g_freezeTime;
			CLOCK::PAUSE_CLOCK(clockPaused ? TRUE : FALSE, 0);
		}

		lap(0);
		for (auto& tp : g_tornadoes)
			tp->Update(dt, t);
		lap(1);
		UpdateFlights(dt, t);
		TornadoHousekeeping(t);
		lap(2);
		StormUpdate(t);
		WeatherAssert(t);
		LabUpdate(t);
		RigUpdate(t);
		SpinUpdate(t);
		SurvivalUpdate(t);
		AutoUpdate(t);
		UpdateImpacts(t);
		HoverSweep(t);
		PlayerSafety(dt, t);
		AdaptPuffRate(t);
		lap(3);
		IntroUpdate(dt, t);
		BalloonUpdate(dt, t);
		SeasonUpdate(t);
		GunUpdate(t);
		SoundUpdate(t);
		MusicUpdate(t);
		VoiceUpdate(t);
		GalleryUpdate(dt, t);
		lap(4);
		bool camBlocked = dead || HUD::IS_PAUSE_MENU_ACTIVE() || CAMERA::IS_SCREEN_FADED_OUT() || IntroRunning() || GalleryActive();   // (they have their own cameras)
		DroneUpdate(dt, camBlocked);   // let go for loading screens and cutscene fades too
		TouchCamUpdate(camBlocked);
		RideCamUpdate(dt, t, camBlocked);
		MashUpdate(dt, t);

		for (size_t i = 0; i < g_scheduled.size();)
		{
			if (t >= g_scheduled[i].at)
			{
				auto fn = g_scheduled[i].fn;
				g_scheduled.erase(g_scheduled.begin() + i);
				fn();
			}
			else i++;
		}

		if (t >= nextOrphanRetry)
		{
			nextOrphanRetry = t + 1.0f;
			RetryOrphans();
		}
		// v1.2: the game's memory, from the start of the session, beside what the mod has out - so a climb can be pinned on it or not
		{
			static float nextMem = 0, lastVram = -1, peakVram = 0, peakRam = 0;
			if (t >= nextMem)
			{
				nextMem = t + 30.0f;
				MemInfo mi;
				MemStats(&mi);
				int props = 0;
				for (auto& tp : g_tornadoes) props += tp->OrbiterCount() + tp->JunkAlive();
				peakVram = std::max(peakVram, mi.vramGB); peakRam = std::max(peakRam, mi.ramGB);
				char jump[48] = "";
				if (lastVram >= 0 && mi.vramGB - lastVram > 1.5f) sprintf_s(jump, " (+%.1f GB in 30 s)", mi.vramGB - lastVram);
				lastVram = mi.vramGB;
				Log("MEM game RAM %.1f GB (private %.1f, peak %.1f) | VRAM %s%.1f of %.1f GB budget%s, shared %.1f, peak %.1f | mod: tornadoes %d loops %d props %d%s%s%s",
					mi.ramGB, mi.privateGB, peakRam, mi.vramKnown ? "" : "(unknown) ", mi.vramGB, mi.vramBudgetGB, jump, mi.sharedGB, peakVram,
					(int)g_tornadoes.size(), LoopsInUse(), props, IntroActive() ? " intro" : "", GalleryActive() ? " gallery" : "", BalloonActive() ? " balloon" : "");
			}
		}
		if (g_set.autoLogSeconds > 0 && t >= nextSnap && (!g_tornadoes.empty() || IntroActive() || BalloonActive()))
		{
			nextSnap = t + g_set.autoLogSeconds;
			for (auto& tp : g_tornadoes) tp->Snapshot("SNAP");
			// Script-thread time, measured separately from game fps (a streaming wait would show up here, not in fps).
			// The pause menu stops the script, so a long pause shows up here too.
			int orbs = 0, loops = LoopsInUse();
			for (auto& tp : g_tornadoes) orbs += tp->OrbiterCount();
			Log("PERF worst script frame %lu ms since last snapshot | game fps %.0f | tornadoes %d loops %d cone props %d flights %d puffs x%.2f | perf %s",
				worstLoopMs, dt > 0 ? 1.0f / dt : 0.0f, (int)g_tornadoes.size(), loops, orbs, FlightsActive(), PuffRateScale(), g_set.perf == 2 ? "Low PC" : g_set.perf == 1 ? "Balanced" : "High");
			worstLoopMs = 0;
		}
		DWORD nowTick = GetTickCount();
		worstLoopMs = std::max(worstLoopMs, nowTick - lastLoopTick);
		lastLoopTick = nowTick;

		if (!hideUi)
		{
			OverlayDraw(dt);
			MenuDraw();
			if (!g_set.cinematic && !scene) DrawNotify();
		}
		else if (g_set.cinematic)
			HUD::HIDE_HUD_AND_RADAR_THIS_FRAME();
		if (!dead && !HUD::IS_PAUSE_MENU_ACTIVE())
			UI::Draw();
		lap(5);
		double total = 0;
		for (double ph : phase) total += ph;
		if (total > 100.0)
			Log("SLOW FRAME %.0f ms: input/menu %.0f | tornadoes %.0f (%d) | flights+housekeeping %.0f | storm+sweeper+safety %.0f | intro/balloon/season/gun/sound %.0f | draw %.0f",
				total, phase[0], phase[1], (int)g_tornadoes.size(), phase[2], phase[3], phase[4], phase[5]);
		WAIT(0);
	}
}
