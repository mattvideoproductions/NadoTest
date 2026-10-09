// Tornado Redemption v1.1 - Storm season: tornadoes that turn up on their own. Part of script.cpp.
//
// The user, after v1.0: "can we make a random weather event tornado so it appears on the map and wanders towards the player,
// a fun natural mode too". With Storm season on, every so often (Rare ~12-25 min, Regular ~6-12, Frequent ~2.5-5) the sky
// darkens, a warning comes up with where it is, and a few seconds later a wild tornado touches down 380-520 m away - marked on
// the map - and wanders, drifting toward Arthur. It lives 5-8 minutes, then dies down. Never during a mission, the intro, a
// challenge or a test, and never while another tornado is out.

struct SeasonState
{
	int stage = 0;             // 0 waiting, 1 warning (sky darkening), 2 on the ground
	float nextAt = 0, stageAt = 0, lifeUntil = 0;
	V3 spawnP;
	Blip zone = 0, icon = 0;
	TornadoRef tp;
	int count = 0;
	bool manual = false;       // "A wild storm, now": runs even with Storm season Off
} g_season;

static const float kSeasonGap[4][2] = { { 0, 0 }, { 720, 1500 }, { 360, 720 }, { 150, 300 } };   // seconds between storms

static const char* CompassWord(const V3& from, const V3& to)
{
	static const char* kDir[8] = { "north", "north-west", "west", "south-west", "south", "south-east", "east", "north-east" };
	V3 d = to - from;
	float h = atan2f(-d.x, d.y) * 180.0f / PI;   // RDR2 heading: 0 = north, counter-clockwise
	int i = (int)floorf(fmodf(h + 360.0f + 22.5f, 360.0f) / 45.0f) % 8;
	return kDir[i];
}

static void SeasonClearBlips()
{
	if (g_season.zone && MAP::DOES_BLIP_EXIST(g_season.zone)) MAP::REMOVE_BLIP(&g_season.zone);
	if (g_season.icon && MAP::DOES_BLIP_EXIST(g_season.icon)) MAP::REMOVE_BLIP(&g_season.icon);
	g_season.zone = g_season.icon = 0;
}

static void SeasonSchedule(float t, bool first);

static void SeasonCancel(const char* why)
{
	if (g_season.stage == 0) return;
	SeasonClearBlips();
	if (g_season.stage == 1 && g_storm.seasonLock)
	{
		g_storm.seasonLock = false;
		// v1.1 audit: if a tornado's storm owns the weather now, leave it to the storm
		if (g_storm.active) g_storm.appliedWeather = -1;
		else if (!g_manualWeather) UnlockWeather();
	}
	g_season.stage = 0;
	g_season.tp = TornadoRef();
	g_season.manual = false;
	if (g_set.season > 0) SeasonSchedule(NowSec(), false);   // never straight back in (v1.1 audit: Despawn restarted it)
	else g_season.nextAt = 0;
	Log("SEASON storm cancelled (%s)", why);
}

static bool SeasonQuiet()
{
	return g_tornadoes.empty() && !IntroActive() && !GalleryActive() && !g_surv.on && !g_bal.chase && !g_auto.running && !PlayerDead() && !MISC::GET_MISSION_FLAG();
}

static void SeasonSchedule(float t, bool first)
{
	int s = std::max(1, std::min(3, g_set.season));
	float gap = RandRange(kSeasonGap[s][0], kSeasonGap[s][1]);
	g_season.nextAt = t + (first ? gap * 0.35f : gap);
	Log("SEASON next storm in %.0f s", g_season.nextAt - t);
}

// Starts a storm now (the menu's "Bring one now" too).
static void SeasonBegin(float t)
{
	V3 pp = PlayerPos();
	// upwind-ish: anywhere round Arthur, but not straight ahead of the camera (you should have to turn round to find it)
	float ang = CameraHeading() + RandRange(60.0f, 300.0f);
	float dist = RandRange(380.0f, 520.0f);
	V3 p = pp + HeadingDir(ang) * dist;
	p.z = GroundZ(p.x, p.y, pp.z + 300.0f, pp.z);
	g_season.spawnP = p;
	g_season.stage = 1;
	g_season.stageAt = t;
	if (g_set.weatherMode > 0 && !g_manualWeather)
	{
		LockWeather("THUNDER", 25.0f);   // the sky darkens over the next 25 s
		g_storm.seasonLock = true;
	}
	SeasonClearBlips();
	g_season.zone = MAP::BLIP_ADD_FOR_RADIUS(Joaat("BLIP_STYLE_AVOID_RADIUS"), p.x, p.y, p.z, 140.0f);
	g_season.icon = MAP::BLIP_ADD_FOR_COORDS(Joaat("BLIP_STYLE_DEBUG_RED"), p.x, p.y, p.z);
	if (g_season.icon)
	{
		MAP::SET_BLIP_SPRITE(g_season.icon, Joaat("blip_rc_lightning"), TRUE);
		MAP::BLIP_ADD_MODIFIER(g_season.icon, Joaat("BLIP_MODIFIER_URGENT"));
		MAP::SET_BLIP_NAME(g_season.icon, MISC::VAR_STRING_LITERAL("Storm"));
	}
	char sub[160];
	sprintf_s(sub, "A twister is forming %.0f m to the %s.", dist, CompassWord(pp, p));
	int id = UI::FeedToastIcon("Tornado warning", sub, "TOAST_LOG_BLIPS", "blip_rc_lightning", 7000);
	if (!id) UI::Toast("TORNADO WARNING", sub, 7.0f);
	UI::Sound("Wanted_Spotted", "HUD_Wanted_Sounds");
	g_season.count++;
	Log("SEASON storm %d: warning, tornado due %.0f m to the %s at (%.0f, %.0f) - game toast %d", g_season.count, dist, CompassWord(pp, p), p.x, p.y, id);
}

void SeasonUpdate(float t)
{
	if (g_set.season == 0 && !g_season.manual)
	{
		// v1.1 audit 2: a wild one already on the ground still dies down on time (and still leaves for a mission)
		if (g_season.stage == 1) SeasonCancel("Storm season off");
		if (g_season.stage != 2) { g_season.nextAt = 0; return; }
	}
	if (g_season.stage == 0)
	{
		if (g_set.season == 0) return;
		if (g_season.nextAt <= 0) { SeasonSchedule(t, true); return; }
		if (t < g_season.nextAt) return;
		if (!SeasonQuiet()) { g_season.nextAt = t + 20.0f; return; }   // try again a bit later
		SeasonBegin(t);
		return;
	}
	if (g_season.stage == 1)
	{
		if (!g_tornadoes.empty() || IntroActive() || GalleryActive() || g_surv.on || g_bal.chase || MISC::GET_MISSION_FLAG() || PlayerDead()) { SeasonCancel("something else started (or a mission)"); return; }
		if (t - g_season.stageAt < 22.0f) return;
		V3 p = g_season.spawnP;
		p.z = GroundZ(p.x, p.y, p.z + 200.0f, p.z);
		Tornado* tp = SpawnTornadoAt(p, RandRange(0, 360), g_set.style, "Wild twister");
		if (!tp) { SeasonCancel("spawn failed"); return; }
		tp->natural = true;
		tp->stationary = false;
		g_season.tp = TornadoRef(tp);
		g_season.stage = 2;
		g_season.lifeUntil = t + RandRange(300.0f, 480.0f);
		if (g_season.zone && MAP::DOES_BLIP_EXIST(g_season.zone)) MAP::REMOVE_BLIP(&g_season.zone);
		g_season.zone = 0;
		if (g_season.icon && MAP::DOES_BLIP_EXIST(g_season.icon)) MAP::REMOVE_BLIP(&g_season.icon);   // the tornado has its own marker now
		g_season.icon = 0;
		// v1.1 audit 2: the warning's THUNDER lock is handed to the storm, which lets go when it's over (it used to be dropped
		// here - with Weather override switched Off meanwhile, THUNDER stayed locked for good). Off: your weather at once.
		if (g_storm.seasonLock && !g_manualWeather)
		{
			if (g_set.weatherMode == 0) UnlockWeather();
			else g_storm.lockedByStorm = true;
		}
		g_storm.seasonLock = false;
		float d = (p - PlayerPos()).len2d();
		UI::Shard("TORNADO", (std::string("On the ground, ") + std::to_string((int)d) + " m to the " + CompassWord(PlayerPos(), p)).c_str(), 3.5f);
		Log("SEASON storm %d: touched down %.0f m away, lives %.0f s", g_season.count, d, g_season.lifeUntil - t);
		return;
	}
	// stage 2: it's out there
	Tornado* wild = g_season.tp.get();
	if (!wild)
	{
		g_season.stage = 0;
		g_season.tp = TornadoRef();
		g_season.manual = false;
		if (g_set.season > 0) SeasonSchedule(t, false);
		return;
	}
	if (MISC::GET_MISSION_FLAG() && !wild->Dissipating())
	{
		wild->BeginDissipate(t);   // a story mission started: the wild one leaves
		Log("SEASON storm %d: a mission started - dying down", g_season.count);
	}
	if (t > g_season.lifeUntil && !wild->Dissipating())
	{
		wild->BeginDissipate(t);
		Log("SEASON storm %d: dying down", g_season.count);
	}
}
