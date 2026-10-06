// NadoTest v1.1 - the v1.1 systems check (Tests & tools, hands-free, ~30 s). Part of script.cpp. Checks in the real game what
// the offline harness can't: the balloon spawns, seats Arthur, gets its envelope and climbs; a mini twister spawns and stays
// out of the storm; a Storm season warning comes up and cleans up; the menu's textures load; the stand-in trees stream in.
// Every line goes to NadoTest_findings.txt as SELFTEST PASS / FAIL (same as the self-test).

struct V11Check { float z0 = 0; int stormWas = 0; TornadoRef mini; bool savedGun = false; int season = 0; } g_v11;

static void AutoV11Check()
{
	AutoStop("replaced");
	ClearEverything();
	g_v11.season = g_set.season;   // (captured up front: stopping the check early must put it back)
	std::vector<AutoStep> steps;
	steps.push_back({ 2.5f, "Systems check: the jet balloon - spawning here, Arthur climbs in", []()
	{
		g_st = SelfTest();
		Finding("V11CHECK start %s", kVersion);
		V3 p = PlayerPos() + HeadingDir(PlayerHeading()) * 2.5f;
		p.z = GroundZ(p.x, p.y, p.z + 10.0f, p.z);
		Check("balloon: something spawned", BalloonSpawn(p, PlayerHeading()), "(%s)", g_bal.veh ? "the game's balloon vehicle" : g_bal.prop ? "a balloon prop" : "nothing");
	} });
	steps.push_back({ 3.5f, "Systems check: is Arthur aboard, is the envelope up? climbing 8 m/s for 3 s", []()
	{
		Check("balloon: Arthur aboard", g_bal.on && BalloonAboard(), "(%s)", g_bal.seated ? "pilot seat" : g_bal.attached ? "attached to the basket" : "no");
		Check("balloon: the cloth envelope exists", !g_bal.veh || (g_bal.envelope && ENTITY::DOES_ENTITY_EXIST(g_bal.envelope)), "(envelope %d)", g_bal.envelope);
		Check("balloon: jet flames started", g_bal.jetFx[0] || g_bal.jetFx[1] || g_bal.burnerFx, "(jets %d/%d burner %d trail %d)", g_bal.jetFx[0], g_bal.jetFx[1], g_bal.burnerFx, g_bal.trailFx);
		g_v11.z0 = g_bal.pos.z;
		g_bal.intro = true;               // fly it by command for the check
		g_bal.cmdVel = V3(0, 0, 8.0f);
	} });
	steps.push_back({ 0.5f, "Systems check: did it climb?", []()
	{
		float rose = g_bal.pos.z - g_v11.z0;
		Check("balloon: climbs when told", rose > 8.0f, "(rose %.1f m in 3.5 s, flying by %s)", rose, kDriveNames[g_bal.drive % 3]);
		g_bal.cmdVel = V3();
		g_bal.intro = false;
		BalloonRemove("systems check");
		Check("balloon: removed cleanly", !g_bal.on && !g_bal.body, "");
	} });
	steps.push_back({ 4.0f, "Systems check: a mini twister 25 m ahead (the tornado gun)", []()
	{
		V3 p = PointAheadCam(25.0f);
		size_t before = g_tornadoes.size();
		GunFire(p);
		g_v11.mini = TornadoRef(g_tornadoes.size() > before ? g_tornadoes.back().get() : nullptr);
		Check("gun: a mini twister spawned", g_v11.mini.get() && g_v11.mini.get()->Mini(), "(%d tornadoes)", (int)g_tornadoes.size());
	} });
	steps.push_back({ 0.5f, "Systems check: the mini twister", []()
	{
		Tornado* m = g_v11.mini.get();
		Check("gun: it has smoke", m && (m->loopsRunning > 0 || g_set.render == 1), "(%d looped effects)", m ? m->loopsRunning : -1);
		Check("gun: it's pocket-sized", m && m->height() < 3.0f, "(%.1f m tall)", m ? m->height() : 0.0f);   // (v1.4: about a person)
		Check("gun: no storm for a mini twister", !g_storm.active, "(storm %s)", g_storm.active ? "ON" : "off");
		Check("gun: the HUD ignores it", NearestTornado() == nullptr, "");
		DespawnAll();
		g_gun.minis.clear();
	} });
	steps.push_back({ 3.0f, "Systems check: Storm season - a warning (the sky darkens, a marker on the map)", []()
	{
		g_set.season = 3;
		SeasonBegin(NowSec());
		Check("season: warning stage", g_season.stage == 1, "(stage %d)", g_season.stage);
		Check("season: map markers", g_season.zone || g_season.icon, "(zone %d icon %d)", g_season.zone, g_season.icon);
	} });
	steps.push_back({ 0.5f, "Systems check: Storm season cleans up", []()
	{
		SeasonCancel("systems check");
		Check("season: cancelled cleanly", g_season.stage == 0 && !g_season.zone && !g_season.icon, "");
		g_set.season = g_v11.season;
		g_season.nextAt = 0;
	} });
	steps.push_back({ 1.0f, "Systems check: menu art, fonts and the stand-in trees", []()
	{
		static const char* kDicts[] = { "generic_textures", "menu_textures", "feeds", "BLIPS" };
		for (const char* d : kDicts)
		{
			TXD::REQUEST_STREAMED_TEXTURE_DICT(d, FALSE);
			Check((std::string("ui: texture dictionary ") + d).c_str(), TXD::HAS_STREAMED_TEXTURE_DICT_LOADED(d) != 0, "");
		}
		int known = 0;
		for (auto& si : g_standIns) if (STREAMING::IS_MODEL_IN_CDIMAGE(si.Model()) && STREAMING::IS_MODEL_VALID(si.Model())) known++;
		Check("real trees: the stand-in tree models exist", known > 0, "(%d of %d)", known, (int)g_standIns.size());
		int treesHere = 0, n = (int)(sizeof(kTreeModels) / sizeof(kTreeModels[0]));
		for (int i = 0; i < n; i++) if (STREAMING::HAS_MODEL_LOADED(kTreeModels[i].hash)) treesHere++;
		Info("real trees: %d map tree models are loaded around you (the ones a tornado here could make vanish)", treesHere);
	} });
	// v1.2: the voices - which of the lines the reactions use Arthur's voice actually has
	steps.push_back({ 0.5f, "Systems check: Arthur's voice lines", []()
	{
		Ped me = PLAYER::PLAYER_PED_ID();
		std::vector<std::string> all = VoContextsFor(true);
		int has = 0, checked = 0;
		std::string missing;
		for (auto& x : all)
		{
			if (x[0] == '~') continue;   // vocals live in the vocal banks: not checkable this way
			checked++;
			if (AUDIO::DOES_CONTEXT_EXIST_FOR_THIS_PED(me, x.c_str(), FALSE)) has++;
			else if (missing.size() < 300) missing += " " + x;
		}
		Check("voices: Arthur has most of his reaction lines", has * 2 > checked, "(%d of %d;%s%s)", has, checked, missing.empty() ? " none missing" : " missing:", missing.c_str());
		Check("voices: 'thanks for the lift' or 'getting up'", AUDIO::DOES_CONTEXT_EXIST_FOR_THIS_PED(me, "RIDER_THANK_FOR_LIFT", FALSE) || AUDIO::DOES_CONTEXT_EXIST_FOR_THIS_PED(me, "GET_UP_FROM_FALL", FALSE), "");
	} });
	// v1.2: the new ways to build one - a Junknado's prop wall, 120 m ahead
	steps.push_back({ 4.0f, "Systems check: a Junknado 120 m ahead (its wall is props)", []()
	{
		SpawnTornado(StyleByLetter('J'), 120.0f, 0, true, "check junk");
	} });
	steps.push_back({ 0.5f, "Systems check: the Junknado", []()
	{
		Tornado* j = g_tornadoes.empty() ? nullptr : g_tornadoes.back().get();
		Check("junknado: the prop wall is up", j && j->JunkAlive() >= 20, "(%d props)", j ? j->JunkAlive() : -1);
		DespawnAll();
		Check("junknado: removed cleanly", g_tornadoes.empty() && LoopsInUse() == 0, "(loops in use %d)", LoopsInUse());
	} });
	// v1.2: the tornado gallery opens, shows its first room harmlessly, and gives everything back when it closes
	steps.push_back({ 4.0f, "Systems check: the tornado gallery (the classics room)", []() { GalStylesOpen(true); } });
	steps.push_back({ 0.5f, "Systems check: the tornado gallery closes", []()
	{
		int n = (int)g_gal.exhibits.size(), loops = LoopsInUse();
		bool harmless = true;
		for (auto& tp : g_tornadoes) if (!tp->Display()) harmless = false;
		Check("gallery: the classics room is out", n == 5 && harmless, "(%d exhibits, %d looped effects)", n, loops);
		GalleryClose("systems check");
		Check("gallery: closed cleanly", !GalleryActive() && g_tornadoes.empty() && LoopsInUse() == 0 && !g_gal.cam, "(loops in use %d)", LoopsInUse());
	} });
	steps.push_back({ 0.5f, "Systems check: summary", []()
	{
		Finding("V11CHECK %s: %d pass, %d fail", g_st.fail ? "FAIL" : "PASS", g_st.pass, g_st.fail);
		char b[160];
		sprintf_s(b, "Systems check: %d passed, %d failed (NadoTest_findings.txt)", g_st.pass, g_st.fail);
		Notify(b, 8000);
	} });
	AutoRun("v1.1 systems check", steps, []() { BalloonRemove("check ended"); g_set.season = g_v11.season; });
}
