// ======================= galleries (v1.2) =======================
// Two showrooms, for footage and for choosing a look (the user: "different tornado comparisons, some new types, animations,
// methods for creating tornado visually"):
//  - The tornado gallery: rooms of tornadoes side by side along the horizon, smaller and harmless (they grab nothing), each
//    with its name over it - the classics, the elements, the odd ones, and one tornado built five different ways. Left /
//    right glides the camera from one to the next, up / down changes room, Enter spawns that one for real.
//  - The texture gallery: the smoke, dust and cloud effects a funnel can be made of, five at a time on a shelf, each spun
//    into a little twister so you see it as tornado material. Left / right picks (and turns the page at the ends), Enter
//    makes style E out of the picked one, the note key saves it to NadoTest_findings.txt.

struct Gallery
{
	int mode = 0;                       // 0 off, 1 the tornado gallery, 2 the texture gallery
	int sel = 0, page = 0;
	V3 origin, F, R;                    // where it's laid out: forward (away from the camera) and right
	float heading = 0;
	std::vector<TornadoRef> exhibits;
	std::vector<int> exStyle;
	std::vector<V3> exPos;
	int room = 0;
	int cam = 0;
	V3 camPos, camLook;
	bool camSnap = true;
	float openedAt = 0;
	// the texture shelf
	std::vector<int> fx;                // 4 loops per swatch
	std::vector<int> fxName;            // index into kLabLooped
	int fxCounted = 0;
	bool hidHud = false;
} g_gal;

static const int kGalPerPage = 5, kGalLoops = 4;
static const float kGalTexScale = 2.0f;
static int GalPages() { return (kLabLoopedCount + kGalPerPage - 1) / kGalPerPage; }
bool GalleryActive() { return g_gal.mode != 0; }

static void GalCamStart()
{
	if (g_gal.cam) return;
	// (audit 3: the drone / touchdown / ride cameras let go when the gallery blocks them - and that switches off every script
	// camera, ours too. So they're stopped first, the way the intro does it.)
	g_drone.mode = 0;
	DroneRelease(false);
	TouchCamStop(false);
	RideCamStop(false);
	g_gal.cam = CAMERA::CREATE_CAM_WITH_PARAMS("DEFAULT_SCRIPTED_CAMERA", g_gal.camPos.x, g_gal.camPos.y, g_gal.camPos.z, 0, 0, 0, 50.0f, FALSE, 2);
	if (!g_gal.cam) { Log("GALLERY: CREATE_CAM_WITH_PARAMS failed"); return; }
	CAMERA::SET_CAM_ACTIVE(g_gal.cam, TRUE);
	CAMERA::RENDER_SCRIPT_CAMS(TRUE, TRUE, 900, TRUE, FALSE, 0);
	g_gal.camSnap = true;
}
static void GalCamStop()
{
	if (!g_gal.cam) return;
	CAMERA::RENDER_SCRIPT_CAMS(FALSE, TRUE, 900, TRUE, FALSE, 0);
	CAMERA::SET_CAM_ACTIVE(g_gal.cam, FALSE);
	CAMERA::DESTROY_CAM(g_gal.cam, FALSE);
	g_gal.cam = 0;
}

// the camera glides toward where it wants to be
static void GalCamMove(const V3& pos, const V3& look, float dt)
{
	if (!g_gal.cam) return;
	float k = g_gal.camSnap ? 1.0f : Clamp(dt * 2.2f, 0.0f, 1.0f);
	g_gal.camSnap = false;
	g_gal.camPos = g_gal.camPos + (pos - g_gal.camPos) * k;
	g_gal.camLook = g_gal.camLook + (look - g_gal.camLook) * k;
	CAMERA::SET_CAM_COORD(g_gal.cam, g_gal.camPos.x, g_gal.camPos.y, g_gal.camPos.z);
	CAMERA::POINT_CAM_AT_COORD(g_gal.cam, g_gal.camLook.x, g_gal.camLook.y, g_gal.camLook.z);
}

static void GalLayout(float dist)
{
	g_gal.heading = CameraHeading();
	g_gal.F = HeadingDir(g_gal.heading);
	g_gal.R = V3(g_gal.F.y, -g_gal.F.x, 0);
	g_gal.origin = PlayerPos() + g_gal.F * dist;
	g_gal.origin.z = GroundZ(g_gal.origin.x, g_gal.origin.y, g_gal.origin.z + 40.0f, PlayerPos().z);
}

// ---------- the tornado gallery ----------
// A room is up to five exhibits: a style (by its letter) and how it's drawn (-1 = as it comes, 0 smoke loops only, 1 puffs only).
struct GalExhibit { char letter; int render; const char* method; };
struct GalRoom { const char* name; const char* sub; GalExhibit ex[5]; int n; };
static const GalRoom kGalRooms[] = {
	{ "The classics", "The five looks from playtests 1 to 10", { { 'V', -1, nullptr }, { 'W', -1, nullptr }, { 'C', -1, nullptr }, { 'R', -1, nullptr }, { 'T', -1, nullptr } }, 5 },
	{ "The elements", "Dust, fire, snow and water - new in v1.2", { { 'D', -1, nullptr }, { 'F', -1, nullptr }, { 'S', -1, nullptr }, { 'H', -1, nullptr } }, 4 },
	{ "The odd ones", "A ghost, three funnels in one, a funnel of junk, and your FX Lab pick", { { 'G', -1, nullptr }, { 'M', -1, nullptr }, { 'J', -1, nullptr }, { 'E', -1, nullptr } }, 4 },
	{ "Five ways to build one", "The same idea drawn five different ways - the mod's whole toolbox", {
		{ 'V', 0, "Smoke loops: plumes that follow the funnel round" },
		{ 'V', 1, "One-shot puffs: bursts sprayed in spiral bands" },
		{ 'T', 0, "Rings that climb: level stripes scrolling up a tube" },
		{ 'J', -1, "Props: the wall is made of flying junk" },
		{ 'F', -1, "Glow: lights spiralling up inside the smoke" } }, 5 },
};
static const int kGalRoomCount = sizeof(kGalRooms) / sizeof(kGalRooms[0]);

static int StyleByLetter(char c)
{
	auto& st = GetStyles();
	for (size_t i = 0; i < st.size(); i++) if (st[i].name[0] == c) return (int)i;
	return 0;
}

static void GalClearExhibits()
{
	for (auto& r : g_gal.exhibits)
		if (Tornado* tp = r.get())
			for (size_t i = 0; i < g_tornadoes.size(); i++)
				if (g_tornadoes[i].get() == tp) { tp->Destroy(); g_tornadoes.erase(g_tornadoes.begin() + i); break; }
	g_gal.exhibits.clear(); g_gal.exStyle.clear(); g_gal.exPos.clear();
}

static void GalRoomBuild(int room)
{
	GalClearExhibits();
	g_gal.room = (room + kGalRoomCount) % kGalRoomCount;
	g_gal.sel = 0;
	g_gal.openedAt = NowSec();
	const GalRoom& R = kGalRooms[g_gal.room];
	float spacing = 46.0f;
	for (int i = 0; i < R.n; i++)
	{
		float x = (i - (R.n - 1) * 0.5f) * spacing;
		V3 p = g_gal.origin + g_gal.R * x + g_gal.F * (fabsf(x) * 0.18f);   // a shallow arc, so the ends face you too
		p.z = GroundZ(p.x, p.y, p.z + 40.0f, g_gal.origin.z);
		SpawnOpts o;
		o.size = 0.55f; o.heightMul = 0.62f; o.reach = 1.1f;
		o.grow = 2.5f + i * 0.7f;        // they touch down one after another, left to right
		o.display = true;
		o.render = R.ex[i].render;
		int style = StyleByLetter(R.ex[i].letter);
		Tornado* tp = SpawnTornadoAt(p, g_gal.heading + 180.0f, style, std::string("Gallery ") + GetStyles()[style].name, o, true);
		if (!tp) continue;
		g_gal.exhibits.push_back(TornadoRef(tp));
		g_gal.exStyle.push_back(i);       // the exhibit's index in the room
		g_gal.exPos.push_back(p);
	}
	Log("GALLERY room '%s': %d exhibits (loops in use %d of %d)", R.name, (int)g_gal.exhibits.size(), LoopsInUse(), g_set.ptfxBudget);
	UI::Shard(R.name, R.sub, 3.0f);
}

static int GalExStyle(int i) { return StyleByLetter(kGalRooms[g_gal.room].ex[g_gal.exStyle[i]].letter); }

static void GalleryClose(const char* why);
static void GalTexStop();
// (audit 3) not over the intro (its handoff is still running), not from the balloon; other modes stop; another gallery closes
static bool GalCanOpen()
{
	if (IntroActive()) { UI::Toast("Gallery", "After the intro, please - it's still handing over.", 3.0f); return false; }
	if (BalloonActive()) { UI::Toast("Gallery", "Land the balloon first.", 3.0f); return false; }
	GalleryClose("switching");
	SurvivalStop("STOPPED");
	if (g_bal.chase) ChaseEnd("STOPPED");
	return true;
}

static void GalStylesOpen(bool fromCheck = false)
{
	if (!GalCanOpen()) return;
	if (!g_tornadoes.empty()) { DespawnAll(); UI::Toast("Gallery", "Cleared the tornadoes out for the gallery.", 3.0f); }
	if (!fromCheck) AutoStop("gallery");   // (the systems check opens it from inside a running test)
	if (g_season.stage) SeasonCancel("gallery");
	GalLayout(150.0f);
	g_gal.mode = 1;
	GalRoomBuild(0);
	PLAYER::SET_PLAYER_CONTROL(PLAYER::PLAYER_ID(), FALSE, 0, FALSE);
	g_gal.camPos = PlayerPos() + g_gal.F * -6.0f + V3(0, 0, 14.0f);
	g_gal.camLook = g_gal.origin + V3(0, 0, 30.0f);
	GalCamStart();
	g_menuOpen = false;
	UI::Sound("MENU_ENTER", "HUD_PLAYER_MENU");
}

static void GalStylesUpdate(float dt, float t)
{
	int n = (int)g_gal.exhibits.size();
	if (!n) return;
	bool wide = t - g_gal.openedAt < 5.0f;   // the establishing shot: all of them touching down
	int s = (int)Clamp((float)g_gal.sel, 0.0f, (float)(n - 1));
	V3 at = g_gal.exPos[s];
	Tornado* tp = g_gal.exhibits[s].get();
	float h = tp ? tp->height() : 60.0f;
	if (wide)
		GalCamMove(PlayerPos() + g_gal.F * -6.0f + V3(0, 0, 16.0f), g_gal.origin + V3(0, 0, h * 0.45f), dt);
	else
		GalCamMove(at - g_gal.F * (h * 0.9f + 30.0f) + g_gal.R * 10.0f + V3(0, 0, 8.0f), at + V3(0, 0, h * 0.42f), dt);
	// names over them
	for (int i = 0; i < n; i++)
	{
		Tornado* e = g_gal.exhibits[i].get();
		if (!e) continue;
		V3 lp = g_gal.exPos[i] + V3(0, 0, e->height() * 1.02f + 6.0f);
		float sx, sy;
		if (!GRAPHICS::GET_SCREEN_COORD_FROM_WORLD_COORD(lp.x, lp.y, lp.z, &sx, &sy)) continue;
		bool on = i == s && !wide;
		UI::Text(GetStyles()[GalExStyle(i)].name, sx, sy - 0.03f, on ? 0.62f : 0.42f, 255, 255, 255, on ? 255 : 150, UI::CENTRE, "title", true);
	}
	if (!wide)
	{
		const GalRoom& R = kGalRooms[g_gal.room];
		const GalExhibit& ex = R.ex[g_gal.exStyle[s]];
		const Style& st = GetStyles()[GalExStyle(s)];
		UI::Panel(0.25f, 0.785f, 0.5f, 0.125f, 200);
		char head[128];
		sprintf_s(head, "%s   %d / %d   %s", R.name, s + 1, n, st.name);
		UI::Text(head, 0.5f, 0.792f, 0.46f, 255, 255, 255, 255, UI::CENTRE, "title");
		UI::Text(ex.method ? ex.method : st.blurb, 0.5f, 0.832f, 0.3f, 220, 220, 220, 255, UI::CENTRE);
		if (!ex.method) UI::Text("UP / DOWN  another room", 0.5f, 0.872f, 0.26f, 190, 190, 190, 200, UI::CENTRE);
	}
}

// ---------- the texture gallery ----------
static void GalTexStop()
{
	for (int h : g_gal.fx)
		if (h && GRAPHICS::DOES_PARTICLE_FX_LOOPED_EXIST(h))
		{
			GRAPHICS::STOP_PARTICLE_FX_LOOPED(h, FALSE);
			GRAPHICS::REMOVE_PARTICLE_FX(h, FALSE);
		}
	AddLoopsInUse(-g_gal.fxCounted);
	g_gal.fxCounted = 0;
	g_gal.fx.clear();
	g_gal.fxName.clear();
}

static V3 g_galBase[kGalPerPage];
static bool g_galBaseOk[kGalPerPage] = {};
static V3 GalSwatchBase(int i)
{
	i = std::max(0, std::min(kGalPerPage - 1, i));
	if (!g_galBaseOk[i])   // (audit 3: one ground probe per swatch per page, not 26 a frame)
	{
		float x = (i - (kGalPerPage - 1) * 0.5f) * 11.0f;
		V3 p = g_gal.origin + g_gal.R * x;
		p.z = GroundZ(p.x, p.y, p.z + 30.0f, g_gal.origin.z);
		g_galBase[i] = p;
		g_galBaseOk[i] = true;
	}
	return g_galBase[i];
}
static V3 GalSwatchPoint(int i, int k, float t)
{
	float ang = t * 2.4f + k * 1.9f;
	float r = 0.7f + k * 0.55f;
	return GalSwatchBase(i) + V3(cosf(ang) * r, sinf(ang) * r, 1.0f + k * 3.6f);
}

static void GalTexPage(int page)
{
	GalTexStop();
	for (bool& b : g_galBaseOk) b = false;
	g_gal.page = (page + GalPages()) % GalPages();
	if (!LoadPtfxAsset("core")) { UI::Toast("Texture gallery", "The game's 'core' effects didn't load.", 4.0f); return; }
	int perSwatch = std::min(kGalLoops, std::max(1, FreeLoops() / kGalPerPage));
	float t = NowSec();
	for (int i = 0; i < kGalPerPage; i++)
	{
		int idx = g_gal.page * kGalPerPage + i;
		if (idx >= kLabLoopedCount) break;
		for (int k = 0; k < kGalLoops; k++)
		{
			int h = 0;
			if (k < perSwatch && FreeLoops() > 0)
			{
				V3 p = GalSwatchPoint(i, k, t);
				GRAPHICS::USE_PARTICLE_FX_ASSET("core");
				h = GRAPHICS::START_PARTICLE_FX_LOOPED_AT_COORD(kLabLooped[idx], p.x, p.y, p.z, 0, 0, 0, kGalTexScale, FALSE, FALSE, FALSE, FALSE);
				if (h) { GRAPHICS::SET_PARTICLE_FX_LOOPED_FAR_CLIP_DIST(h, 600.0f); AddLoopsInUse(1); g_gal.fxCounted++; }
			}
			g_gal.fx.push_back(h);
			g_gal.fxName.push_back(idx);
		}
	}
	Log("GALLERY textures page %d of %d: %d loops running", g_gal.page + 1, GalPages(), g_gal.fxCounted);
}

static int GalTexCount()   // swatches on this page
{
	return std::min(kGalPerPage, kLabLoopedCount - g_gal.page * kGalPerPage);
}

static void GalTexOpen()
{
	if (!GalCanOpen()) return;
	if (!g_tornadoes.empty()) { DespawnAll(); UI::Toast("Gallery", "Cleared the tornadoes out for the gallery.", 3.0f); }
	AutoStop("gallery");
	g_lab.slideshow = false; LabStop();
	GalLayout(34.0f);
	g_gal.mode = 2;
	g_gal.sel = 0;
	g_gal.openedAt = NowSec();
	GalTexPage(0);
	PLAYER::SET_PLAYER_CONTROL(PLAYER::PLAYER_ID(), FALSE, 0, FALSE);
	g_gal.camPos = PlayerPos() + g_gal.F * 4.0f + V3(0, 0, 3.0f);
	g_gal.camLook = g_gal.origin + V3(0, 0, 7.0f);
	GalCamStart();
	g_menuOpen = false;
	UI::Sound("MENU_ENTER", "HUD_PLAYER_MENU");
}

static void GalTexUpdate(float dt, float t)
{
	for (size_t j = 0; j < g_gal.fx.size(); j++)
	{
		int h = g_gal.fx[j];
		if (!h) continue;
		V3 p = GalSwatchPoint((int)(j / kGalLoops), (int)(j % kGalLoops), t);   // world-space loops take world coordinates
		GRAPHICS::SET_PARTICLE_FX_LOOPED_OFFSETS(h, p.x, p.y, p.z, 0, 0, 0);
	}
	int s = (int)Clamp((float)g_gal.sel, 0.0f, (float)(GalTexCount() - 1));
	V3 b = GalSwatchBase(s);
	GalCamMove(b - g_gal.F * 24.0f + V3(0, 0, 4.5f), b + V3(0, 0, 7.0f), dt);
	GRAPHICS::DRAW_LIGHT_WITH_RANGE(b.x, b.y, b.z + 0.6f, 255, 214, 150, 5.0f, 4.0f);   // a lamp under the picked one
	for (int i = 0; i < GalTexCount(); i++)
	{
		V3 lp = GalSwatchBase(i) + V3(0, 0, 17.5f);
		float sx, sy;
		if (!GRAPHICS::GET_SCREEN_COORD_FROM_WORLD_COORD(lp.x, lp.y, lp.z, &sx, &sy)) continue;
		int idx = g_gal.page * kGalPerPage + i;
		char b2[16];
		sprintf_s(b2, "%d", idx + 1);
		bool on = i == s;
		UI::Text(b2, sx, sy - 0.02f, on ? 0.6f : 0.4f, 255, 255, 255, on ? 255 : 140, UI::CENTRE, "title", true);
	}
	int idx = g_gal.page * kGalPerPage + s;
	UI::Panel(0.27f, 0.80f, 0.46f, 0.105f, 200);
	char head[128];
	sprintf_s(head, "%d / %d   %s", idx + 1, kLabLoopedCount, kLabLooped[idx]);
	UI::Text(head, 0.5f, 0.808f, 0.44f, 255, 255, 255, 255, UI::CENTRE, "title");
	char sub[160];
	sprintf_s(sub, "Page %d of %d.  Enter: make style E (Custom) out of this one.  %s: save it to the findings.", g_gal.page + 1, GalPages(), g_keys.bookmark.text.c_str());
	UI::Text(sub, 0.5f, 0.846f, 0.3f, 220, 220, 220, 255, UI::CENTRE);
}

// ---------- both ----------
static void GalleryClose(const char* why)
{
	if (!g_gal.mode) return;
	GalClearExhibits();
	GalTexStop();
	GalCamStop();
	PLAYER::SET_PLAYER_CONTROL(PLAYER::PLAYER_ID(), TRUE, 0, FALSE);
	Log("GALLERY closed (%s), loops in use %d", why, LoopsInUse());
	g_gal.mode = 0;
	UI::Sound("MENU_CLOSE", "HUD_PLAYER_MENU");
}

static void GalleryUpdate(float dt, float t)
{
	if (!g_gal.mode) return;
	if (PlayerDead() || IntroActive() || MISC::GET_MISSION_FLAG()) { GalleryClose("interrupted"); return; }
	HUD::HIDE_HUD_AND_RADAR_THIS_FRAME();
	int count = g_gal.mode == 1 ? (int)g_gal.exhibits.size() : GalTexCount();
	static bool menuWas = false;
	bool menuBusy = g_menuOpen || menuWas;   // (audit 3: the key that closed the menu is still "pressed" this frame)
	menuWas = g_menuOpen;
	if (!menuBusy && t - g_gal.openedAt > 0.4f)
	{
		bool right = Pressed(g_keys.right, true) || PadPressed(PB_RIGHT, true);
		bool left = Pressed(g_keys.left, true) || PadPressed(PB_LEFT, true);
		if (right || left)
		{
			int d = right ? 1 : -1;
			int s = g_gal.sel + d;
			if (g_gal.mode == 2 && (s < 0 || s >= count))
			{
				GalTexPage(g_gal.page + d);
				g_gal.sel = d > 0 ? 0 : GalTexCount() - 1;   // (audit 3: the new page's own count - the last page has 4)
			}
			else
				g_gal.sel = (s + count) % std::max(1, count);
			UI::Sound(d > 0 ? "NAV_RIGHT" : "NAV_LEFT", "PAUSE_MENU_SOUNDSET");
		}
		if (Pressed(g_keys.select) || PadPressed(PB_A))
		{
			if (g_gal.mode == 1 && g_gal.sel < (int)g_gal.exStyle.size())
			{
				int st = GalExStyle(g_gal.sel);
				GalleryClose("picked a style");
				g_set.style = st;
				UserSpawn();
				return;
			}
			if (g_gal.mode == 2)
			{
				int idx = g_gal.page * kGalPerPage + g_gal.sel;
				SetCustomStyleFx(kLabLooped[idx], true, kGalTexScale);
				g_set.style = (int)GetStyles().size() - 1;
				Finding("TEXTURE GALLERY: style E now uses %s (scale %.1f)", kLabLooped[idx], kGalTexScale);
				UI::Toast("Style E", (std::string("Now made of ") + kLabLooped[idx]).c_str(), 3.0f);
				UI::Sound("SELECT", "HUD_SHOP_SOUNDSET");
			}
		}
		if (g_gal.mode == 1 && (Pressed(g_keys.up, true) || PadPressed(PB_UP, true) || Pressed(g_keys.down, true) || PadPressed(PB_DOWN, true)))
		{
			int d = (Pressed(g_keys.down, true) || PadPressed(PB_DOWN, true)) ? 1 : -1;
			GalRoomBuild(g_gal.room + d);
			UI::Sound(d > 0 ? "NAV_DOWN" : "NAV_UP", "Ledger_Sounds");
		}
		if (g_gal.mode == 2 && Pressed(g_keys.bookmark))
		{
			int idx = g_gal.page * kGalPerPage + g_gal.sel;
			Finding("TEXTURE GALLERY liked: %s", kLabLooped[idx]);
			UI::Toast("Saved", kLabLooped[idx], 2.5f);
		}
		if (Pressed(g_keys.back) || PadPressed(PB_B)) { GalleryClose("left"); return; }
	}
	if (g_gal.mode == 1) GalStylesUpdate(dt, t);
	else GalTexUpdate(dt, t);
	static float nextTip = 0;
	if (t > nextTip)
	{
		nextTip = t + 6.0f;
		UI::HelpTip(g_gal.mode == 1 ? "LEFT / RIGHT  browse      UP / DOWN  another room      ENTER  spawn this one for real      BACKSPACE  leave"
			: "LEFT / RIGHT  browse      ENTER  use it for style E      BACKSPACE  leave", 6.5f);
	}
}
