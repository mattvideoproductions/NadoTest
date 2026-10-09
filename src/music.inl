// Tornado Redemption v1.1 - optional mission music while a tornado is out. Part of script.cpp.
// The user: "optional intense music mission style or something... like during a regular nado experience".
// It's the game's own interactive music (research\music_research.md: every name below is a plain string in Rockstar's
// single-player scripts). The pattern is Rockstar's: PREPARE until it's ready, then TRIGGER; a START layer when a tornado
// appears, the ACTION layer once it's down and near you (or, in the intro, at touchdown), the END layer when the last one is
// gone - then STOP_MUSIC_8S, because the music doesn't stop on its own.
// Rockstar's score may be claimed on YouTube, so it's off by default.

struct Score { const char* name; const char* start; const char* action; const char* end; };
static const Score kScores[] = {
	{ "Hideout fight",      "SP_HIDEOUTS_GENERAL_START", "SP_HIDEOUTS_GENERAL_ACTION", "SP_HIDEOUTS_GENERAL_IDLE" },   // nine hideouts share it
	{ "Braithwaite battle", "BRT3_BATTLE",               "BRT3_REINFORCEMENTS",        "BRT3_SCORE_OVERFLOW_STOPPER" },
	{ "Outlaw ambush",      "REOT_START",                "REOT_FIGHT",                 "REOT_END" },
	{ "Hostage rescue",     "RE_HOSTAGE_RESCUE_START",   "RE_HOSTAGE_RESCUE_FIGHT",    "RE_HOSTAGE_RESCUE_END" },
	{ "Horse chase",        "RESR_CHASE_OW",             nullptr,                      "RESR_CHASE_OVER_OW" },
	{ "Native Son storm",   "NTS3_RESTART_6",            "NTS3_RESTART_7",             nullptr },
};
static const int kScoreCount = sizeof(kScores) / sizeof(kScores[0]);

struct MusicState
{
	int layer = 0;             // 0 silent, 1 start, 2 action, 3 ended (waiting to stop)
	int score = -1;            // which score is playing
	const char* pending = nullptr;
	float pendingSince = 0, stopAt = 0;
	bool fired = false;        // one of OUR events actually played (only then does the mod send a stop)
} g_music;

// Rockstar's helper: prepare (every frame until it's ready), then trigger.
static bool MusicFire(const char* ev)
{
	if (!ev) return true;
	if (!AUDIO::PREPARE_MUSIC_EVENT(ev)) return false;
	return AUDIO::TRIGGER_MUSIC_EVENT(ev) != 0;
}

static void MusicGo(const char* ev, float t)
{
	if (g_music.pending && g_music.pending != ev) AUDIO::CANCEL_MUSIC_EVENT(g_music.pending);   // a layer that never fired
	g_music.pending = ev;
	g_music.pendingSince = t;
}

static void MusicStopNow(const char* why)
{
	if (g_music.pending) AUDIO::CANCEL_MUSIC_EVENT(g_music.pending);
	if (g_music.fired && !MISC::GET_MISSION_FLAG()) AUDIO::TRIGGER_MUSIC_EVENT("STOP_MUSIC_8S");   // (it's global: never over a mission's score)
	if (g_music.layer > 0) Log("music: stopped (%s)", why);
	g_music = MusicState();
}

void MusicUpdate(float t)
{
	// what should be playing?
	int want = 0;
	if (g_musicMode > 0)
	{
		if (IntroRunning())
			want = g_in.stage == 4 ? (g_in.clock >= kIntroSpawn + 1.0f ? 2 : g_in.clock >= kIntroThunder ? 1 : 0) : 0;   // (v1.7: a sunny morning has no score)
		else
		{
			float nd;
			Tornado* tp = NearestTornado(&nd);
			bool any = false;
			for (auto& p : g_tornadoes) if (!p->Mini() && !p->Dissipating()) any = true;
			if (any) want = (tp && tp->TouchedDown() && nd < tp->reachRadius() * 2.0f) ? 2 : 1;
			if (g_music.layer == 2 && any) want = 2;   // once it's on, the action layer stays until the storm's over
		}
		if (MISC::GET_MISSION_FLAG()) want = 0;   // never over a story mission's own music
		if (GalleryActive()) want = 0;            // v1.2: a gallery isn't a storm
	}
	int scoreIdx = std::max(0, std::min(kScoreCount - 1, g_musicMode - 1));
	if (g_musicMode == 0 && g_music.layer > 0 && g_music.layer < 3) { MusicStopNow("switched off"); return; }
	if (g_music.score >= 0 && g_music.score != scoreIdx && g_music.layer > 0) { MusicStopNow("score changed"); return; }
	const Score& sc = kScores[scoreIdx];
	if (want >= 1 && g_music.layer == 0) { g_music.score = scoreIdx; g_music.layer = 1; MusicGo(sc.start, t); Log("music: %s - start", sc.name); }
	if (want == 2 && g_music.layer == 1 && sc.action) { g_music.layer = 2; MusicGo(sc.action, t); Log("music: %s - action", sc.name); }
	if (want == 0 && (g_music.layer == 1 || g_music.layer == 2))
	{
		// v1.1 audit 2: nothing of ours ever played (the start was still pending) - no END layer, no stop, just let go
		if (!g_music.fired) { MusicStopNow("it never started"); return; }
		g_music.layer = 3;
		g_music.stopAt = t + 8.0f;
		MusicGo(sc.end, t);
		Log("music: %s - end", sc.name);
	}
	if (g_music.layer == 3 && want >= 1) { g_music.layer = 1; MusicGo(sc.start, t); }   // another storm before it faded
	// keep trying the pending layer (Rockstar does it every frame); give up after 10 s
	if (g_music.pending)
	{
		if (MusicFire(g_music.pending)) { Log("music: %s triggered", g_music.pending); g_music.pending = nullptr; g_music.fired = true; }
		else if (t - g_music.pendingSince > 10.0f) { Log("music: %s never got ready - skipped", g_music.pending); AUDIO::CANCEL_MUSIC_EVENT(g_music.pending); g_music.pending = nullptr; }
	}
	if (g_music.layer == 3 && t > g_music.stopAt && !g_music.pending)
	{
		// v1.1 audit 2: STOP_MUSIC_8S stops ALL music - in a mission that would cut the mission's own score (our END has played)
		if (g_music.fired && !MISC::GET_MISSION_FLAG()) AUDIO::TRIGGER_MUSIC_EVENT("STOP_MUSIC_8S");
		g_music = MusicState();
		Log("music: faded out");
	}
}
