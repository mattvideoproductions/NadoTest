// Tornado Redemption v1.1 - the jet balloon and the Storm chaser challenge. Part of script.cpp (included there, so it can use the
// tornado list, the storm and the camera helpers); kept in its own file to stay readable.
//
// The user, after v1.0: "set up hot air balloon intro tornado mode or maybe it can be its own mini challenge or setting in
// the mod - a jet powered hot air balloon to ride around on and see the nado, easy to control please".
//
// How it flies (research\cutscene_research.md 5): the game's balloon is a vehicle ("hotAirBalloon01", the basket) plus a
// cloth envelope the game creates with it. Rockstar's own balloon mission steers it every frame with the balloon natives,
// so that's what this does: _SET_VELOCITY_FOR_BALLOON to where you're steering (a jet, not the wind), _SET_BALLOON_FACE_COORD
// to turn it. If the game ignores that (measured: it isn't moving the way it's told), it falls back to plain entity velocity,
// then to moving it ourselves - each switch is logged. Controls are relative to where the camera looks, like walking:
//   keyboard: W A S D fly, SPACE up, CTRL down, SHIFT jet boost, F bail out
//   pad:      left stick fly, RT up, LT down, A jet boost, Y bail out (the right stick still turns the camera)
// The tornado leaves the balloon and Arthur alone while he's in it - except in Storm chaser, where the wall catches you.

static const char* kBalloonVehicle = "hotAirBalloon01";
// fallback: a static balloon we move ourselves. v1.1 audit 2: p_hotairballoon01x is only the cloth envelope (research 5.1),
// not something to stand in, so it's no longer tried; s_hotairballoon01x is the story mission's static balloon (unseen).
static const char* kBalloonProps[] = { "s_hotairballoon01x" };
static const char* kJetDict = "anm_fire_dancers";
static const char* kJetFx = "ent_anim_fire_breathe_loop";      // the fire-breather's flame (femga's example blows it from a head)
static const char* kBurnerFx = "veh_exhaust_hot_air_balloon";  // core: the balloon's own burner
static const char* kTrailFx = "ent_amb_exhaust_thick";         // core

static void BalloonRemove(const char* why);
void ChaseEnd(const char* why);

struct JetAim { float x, y, z; const char* name; };
// The flame's own axis is unknown, so in flight the next-step key cycles the aim and the log records which one was used
// ([Balloon] JetAim picks the default). femga's example rotates it -90 on X to blow it forward from a ped's head.
static const JetAim kJetAims[] = {
	{ 0, 0, 180, "backward (yaw 180)" }, { -90, 0, 180, "pitch -90, yaw 180" }, { 90, 0, 0, "pitch +90" }, { 0, 0, 0, "no rotation" },
};
static const int kJetAimCount = sizeof(kJetAims) / sizeof(kJetAims[0]);
static const char* kDriveNames[] = { "balloon velocity", "entity velocity", "moved by the mod" };

struct BalloonState
{
	bool on = false;
	bool waiting = false;      // spawned and parked, nobody aboard yet (the intro's balloon at the edge of camp)
	bool intro = false;        // the intro flies it (cmdVel) until the handoff
	bool skim = false;         // v1.6.1: the intro's swoop - it may fly the basket just off the grass (the 3 m floor is off)
	bool parked = false;       // Arthur bailed out: it's left to drift down
	Vehicle veh = 0;
	Object prop = 0;
	Entity body = 0, envelope = 0;
	bool seated = false, attached = false;
	int reseats = 0;
	float basketZ = 0;
	V3 pos, vel, cmdVel;
	float heading = 0;
	int drive = 1;             // see kDriveNames. v1.3: starts on entity velocity - playtest 11 showed the balloon native never moves it
	                           // (the intro: 2.6 m of 36; the systems check: 0 m), and entity velocity flew it every time
	V3 checkFrom;
	float checkAt = 0, checkCmd = 0;
	float boost = 0, boostShown = 0;
	bool boostHeld = false;    // v1.2: the jets were on last frame (a fresh press pulls the burner and whooshes)
	float whooshAt = -100;
	int jetFx[2] = { 0, 0 }, burnerFx = 0, trailFx = 0;
	int fxCounted = 0;         // looped effects it holds from the shared budget (the game runs out at ~126 in total)
	int aim = 0;
	float startedAt = 0, seatCheckAt = 0, parkedAt = 0, envelopeCheckAt = 0;
	int driveBad = 0;          // failed movement checks in a row (two drop it to the next drive mode)
	bool helpShown = false;
	// Storm chaser
	bool chase = false;
	float chaseStart = 0, chaseScore = 0, chaseBest = 0, chaseMult = 0, chaseCaughtAt = -1, chaseResultUntil = 0;
	std::string chaseResult;
	bool chaseBestLoaded = false;
	float caughtAng = 0, caughtR = 0, caughtH = 0;
	int savedMove = 2;
} g_bal;

bool BalloonActive() { return g_bal.on && !g_bal.parked && !g_bal.waiting; }

static void BalloonStopFx()
{
	AddLoopsInUse(-g_bal.fxCounted);
	g_bal.fxCounted = 0;
	int* all[] = { &g_bal.jetFx[0], &g_bal.jetFx[1], &g_bal.burnerFx, &g_bal.trailFx };
	for (int* h : all)
	{
		if (*h && GRAPHICS::DOES_PARTICLE_FX_LOOPED_EXIST(*h))
		{
			GRAPHICS::STOP_PARTICLE_FX_LOOPED(*h, FALSE);
			GRAPHICS::REMOVE_PARTICLE_FX(*h, FALSE);
		}
		*h = 0;
	}
}

static void BalloonStartFx()
{
	BalloonStopFx();
	if (!g_bal.body) return;
	const JetAim& a = kJetAims[g_bal.aim % kJetAimCount];
	float z = g_bal.basketZ + 0.45f;
	if (LoadPtfxAsset(kJetDict, g_bal.intro ? 0 : 1500))   // (the intro asked for it while the screen was black: no wait mid-scene)
		for (int i = 0; i < 2; i++)
		{
			GRAPHICS::USE_PARTICLE_FX_ASSET(kJetDict);
			g_bal.jetFx[i] = GRAPHICS::START_PARTICLE_FX_LOOPED_ON_ENTITY(kJetFx, g_bal.body, i ? 0.5f : -0.5f, -1.05f, z, a.x, a.y, a.z, 1.4f, FALSE, FALSE, FALSE);
		}
	GRAPHICS::USE_PARTICLE_FX_ASSET("core");
	g_bal.burnerFx = GRAPHICS::START_PARTICLE_FX_LOOPED_ON_ENTITY(kBurnerFx, g_bal.body, 0, 0, g_bal.basketZ + 2.4f, 0, 0, 0, 1.3f, FALSE, FALSE, FALSE);
	if (!g_bal.burnerFx)
	{
		GRAPHICS::USE_PARTICLE_FX_ASSET("core");
		g_bal.burnerFx = GRAPHICS::START_PARTICLE_FX_LOOPED_ON_ENTITY("ent_amb_torch_fire", g_bal.body, 0, 0, g_bal.basketZ + 2.4f, 0, 0, 0, 1.6f, FALSE, FALSE, FALSE);
	}
	GRAPHICS::USE_PARTICLE_FX_ASSET("core");
	g_bal.trailFx = GRAPHICS::START_PARTICLE_FX_LOOPED_ON_ENTITY(kTrailFx, g_bal.body, 0, -1.3f, z, 0, 0, 0, 1.0f, FALSE, FALSE, FALSE);
	g_bal.fxCounted = (g_bal.jetFx[0] != 0) + (g_bal.jetFx[1] != 0) + (g_bal.burnerFx != 0) + (g_bal.trailFx != 0);
	AddLoopsInUse(g_bal.fxCounted);
	Log("balloon: jets lit (aim %s): jets %d/%d, burner %d, trail %d", a.name, g_bal.jetFx[0], g_bal.jetFx[1], g_bal.burnerFx, g_bal.trailFx);
}

// Spawns the balloon at a spot on the ground and parks it there (frozen, nobody aboard). Returns false if nothing spawned.
static bool BalloonSpawnParked(const V3& ground, float heading)
{
	if (g_bal.on || g_bal.body) BalloonRemove("replaced by a new one");   // v1.1 audit: never orphan the old one
	float best = g_bal.chaseBest;
	bool loaded = g_bal.chaseBestLoaded;
	g_bal = BalloonState();
	g_bal.chaseBest = best; g_bal.chaseBestLoaded = loaded;
	g_bal.aim = ((int)IniFloat("Balloon", "JetAim", 0) % kJetAimCount + kJetAimCount) % kJetAimCount;
	STREAMING::REQUEST_NAMED_PTFX_ASSET(H(kJetDict));   // v1.1 audit 2: asked for now, so lighting the jets later doesn't wait
	STREAMING::REQUEST_ANIM_DICT("script_story@gng2@ig@ig_2_balloon_control");   // v1.2: Arthur's burner pull
	Hash vm = H(kBalloonVehicle);
	Vector3 mn = {}, mx = {};
	if (LoadModel(vm, 4000))
	{
		MISC::GET_MODEL_DIMENSIONS(vm, &mn, &mx);
		g_bal.veh = VEHICLE::CREATE_VEHICLE(vm, ground.x, ground.y, ground.z - mn.z + 0.05f, heading, FALSE, FALSE, TRUE, FALSE);
		STREAMING::SET_MODEL_AS_NO_LONGER_NEEDED(vm);
	}
	if (g_bal.veh)
		g_bal.body = g_bal.veh;
	else
	{
		for (const char* name : kBalloonProps)
		{
			Hash pm = H(name);
			if (!LoadModel(pm, 3000)) continue;
			MISC::GET_MODEL_DIMENSIONS(pm, &mn, &mx);
			g_bal.prop = OBJECT::CREATE_OBJECT(pm, ground.x, ground.y, ground.z - mn.z + 0.05f, FALSE, FALSE, FALSE, FALSE, FALSE);
			STREAMING::SET_MODEL_AS_NO_LONGER_NEEDED(pm);
			if (g_bal.prop) { Log("balloon: the vehicle wouldn't spawn - using the prop %s (moved by the mod)", name); break; }
		}
		g_bal.body = g_bal.prop;
		g_bal.drive = 2;
	}
	if (!g_bal.body)
	{
		Log("balloon: no balloon model spawned (vehicle %s, prop %s)", kBalloonVehicle, kBalloonProps[0]);
		Notify("The balloon wouldn't spawn here - try again in the open");
		return false;
	}
	g_bal.basketZ = mn.z;   // the model's lowest point: the bottom of the basket
	SetScripted(g_bal.body, true);
	ENTITY::SET_ENTITY_AS_MISSION_ENTITY(g_bal.body, TRUE, TRUE);
	ENTITY::SET_ENTITY_HEADING(g_bal.body, heading);
	ENTITY::SET_ENTITY_INVINCIBLE(g_bal.body, TRUE);
	ENTITY::SET_ENTITY_LOD_DIST(g_bal.body, 1500);
	ENTITY::FREEZE_ENTITY_POSITION(g_bal.body, TRUE);
	g_bal.pos = V3(ENTITY::GET_ENTITY_COORDS(g_bal.body, FALSE, FALSE));
	g_bal.heading = heading;
	g_bal.on = true;
	g_bal.waiting = true;
	g_bal.startedAt = NowSec();
	g_bal.envelopeCheckAt = NowSec() + 0.5f;
	Log("balloon: spawned %s (handle %d) at (%.1f, %.1f, %.1f), size %.1f x %.1f x %.1f m",
		g_bal.veh ? kBalloonVehicle : "prop", g_bal.body, g_bal.pos.x, g_bal.pos.y, g_bal.pos.z, mx.x - mn.x, mx.y - mn.y, mx.z - mn.z);
	return true;
}

static bool BalloonAboard()
{
	Ped me = PLAYER::PLAYER_PED_ID();
	if (g_bal.attached) return ENTITY::IS_ENTITY_ATTACHED_TO_ENTITY(me, g_bal.body) != 0;
	return g_bal.veh && PED::IS_PED_IN_VEHICLE(me, g_bal.veh, FALSE) != 0;
}

static void BalloonSeat()
{
	Ped me = PLAYER::PLAYER_PED_ID();
	if (PED::IS_PED_ON_MOUNT(me) || (PED::IS_PED_IN_ANY_VEHICLE(me, FALSE) && !(g_bal.veh && PED::IS_PED_IN_VEHICLE(me, g_bal.veh, FALSE))))
		TASK::CLEAR_PED_TASKS_IMMEDIATELY(me, FALSE, TRUE);
	g_bal.seated = g_bal.attached = false;
	if (g_bal.veh)
	{
		PED::SET_PED_INTO_VEHICLE(me, g_bal.veh, -1);   // the pilot's seat (Rockstar: gang2, the balloon mission)
		g_bal.seated = PED::IS_PED_IN_VEHICLE(me, g_bal.veh, FALSE) != 0;
	}
	if (!g_bal.seated)
	{
		// Rockstar's passengers ride attached, not seated (gang2.c): the same, in the middle of the basket
		ENTITY::ATTACH_ENTITY_TO_ENTITY(me, g_bal.body, 0, 0.0f, -0.2f, g_bal.basketZ + 1.1f, 0, 0, 0, FALSE, FALSE, FALSE, FALSE, 2, TRUE, FALSE, FALSE);
		g_bal.attached = true;
	}
	PED::SET_PED_CONFIG_FLAG(me, 15, TRUE);   // don't ragdoll out from bumps (Rockstar sets it on the balloon pilot)
}

// Arthur climbs in and the jets light. intro = the intro flies it until the handoff.
static void BalloonBoard(bool intro)
{
	if (!g_bal.on || !g_bal.body) return;
	BalloonSeat();
	g_bal.waiting = false;
	g_bal.intro = intro;
	g_bal.startedAt = NowSec();
	g_bal.seatCheckAt = NowSec() + 0.6f;
	if (g_bal.drive < 2) ENTITY::FREEZE_ENTITY_POSITION(g_bal.body, FALSE);
	g_bal.vel = V3(0, 0, 3.0f);
	g_bal.checkAt = NowSec() + 1.5f;
	g_bal.checkFrom = g_bal.pos;
	BalloonStartFx();
	Log("balloon: Arthur %s, flying by %s", g_bal.seated ? "in the pilot's seat" : "attached in the basket", kDriveNames[g_bal.drive]);
}

static bool BalloonSpawn(const V3& ground, float heading)
{
	if (!BalloonSpawnParked(ground, heading)) return false;
	BalloonBoard(false);
	return true;
}

static void BalloonRemove(const char* why)
{
	if (!g_bal.on && !g_bal.body) return;
	if (g_bal.chase) ChaseEnd("STOPPED");   // v1.1 audit: give the movement setting back
	Ped me = PLAYER::PLAYER_PED_ID();
	BalloonStopFx();
	// v1.1 audit: was he aboard, and how high? (asked BEFORE he's detached - after it, "aboard" is always false)
	float h = 0;
	bool aboard = g_bal.body && ENTITY::DOES_ENTITY_EXIST(g_bal.body) && !g_bal.waiting && !g_bal.parked && BalloonAboard();
	if (aboard && TrueHeight(ENTITY::GET_ENTITY_COORDS(me, TRUE, FALSE), &h) && h > 3.0f)
		StartDropTest(NowSec());   // Arthur was up there: the soft landing catches him
	if (g_bal.attached && ENTITY::DOES_ENTITY_EXIST(me))
		ENTITY::DETACH_ENTITY(me, TRUE, TRUE);
	PED::SET_PED_CONFIG_FLAG(me, 15, FALSE);
	if (g_bal.envelope) SetScripted(g_bal.envelope, false);
	if (g_bal.body && ENTITY::DOES_ENTITY_EXIST(g_bal.body))
	{
		SetScripted(g_bal.body, false);
		if (g_bal.veh)
		{
			Vehicle v = g_bal.veh;
			// v1.1 audit 2: out of the seat first - never delete the vehicle he's sitting in
			if (PED::IS_PED_IN_VEHICLE(me, v, FALSE)) TASK::CLEAR_PED_TASKS_IMMEDIATELY(me, FALSE, TRUE);
			ENTITY::SET_ENTITY_AS_MISSION_ENTITY(v, TRUE, TRUE);
			VEHICLE::DELETE_VEHICLE(&v);
		}
		else
		{
			Object o = g_bal.prop;
			DeleteObj(o);
		}
	}
	Log("balloon: removed (%s)", why);
	std::string result = g_bal.chaseResult;
	float until = g_bal.chaseResultUntil, best = g_bal.chaseBest;
	bool loaded = g_bal.chaseBestLoaded;
	g_bal = BalloonState();
	g_bal.chaseResult = result; g_bal.chaseResultUntil = until;
	g_bal.chaseBest = best; g_bal.chaseBestLoaded = loaded;
}

void ChaseEnd(const char* why);

// Arthur jumps out (F / Y): the soft landing catches him; the balloon is left to drift down and stays where it lands.
static void BalloonBail(float t)
{
	if (!g_bal.on || g_bal.parked || g_bal.waiting) return;
	Ped me = PLAYER::PLAYER_PED_ID();
	if (g_bal.attached)
		ENTITY::DETACH_ENTITY(me, TRUE, TRUE);
	else if (g_bal.seated && g_bal.veh && PED::IS_PED_IN_VEHICLE(me, g_bal.veh, FALSE))
		TASK::TASK_LEAVE_VEHICLE(me, g_bal.veh, 4096, 0);   // jump out (Halen84's eEnterExitVehicleFlags; 16 is a warp)
	PED::SET_PED_CONFIG_FLAG(me, 15, FALSE);
	g_bal.seated = g_bal.attached = false;
	g_bal.parked = true;
	g_bal.parkedAt = t;
	StartDropTest(t);
	BalloonStopFx();
	if (g_bal.chase) ChaseEnd("BAILED OUT");
	float h = 0;
	TrueHeight(g_bal.pos, &h);
	Log("balloon: Arthur bailed out %.0f m up", h);
}

static void BalloonControls(float dt, float t, V3* wantVel)
{
	float camH = CameraHeading();
	V3 fwd = HeadingDir(camH), right = HeadingDir(camH - 90.0f);
	float mx = 0, my = 0, up = 0, boost = 0;
	// keys only count while the game's window has focus (GetAsyncKeyState sees the whole desktop)
	DWORD fgPid = 0;
	GetWindowThreadProcessId(GetForegroundWindow(), &fgPid);
	bool focused = fgPid == GetCurrentProcessId();
	auto key = [focused](int vk) { return focused && (GetAsyncKeyState(vk) & 0x8000) != 0; };
	if (key('W')) my += 1;
	if (key('S')) my -= 1;
	if (key('D')) mx += 1;
	if (key('A')) mx -= 1;
	if (key(VK_SPACE)) up += 1;
	if (key(VK_CONTROL) || key('C')) up -= 1;
	if (key(VK_SHIFT)) boost = 1;
	PadAnalog pa;
	if (PadGetAnalog(&pa))
	{
		mx += pa.lx; my += pa.ly;
		up += pa.rt - pa.lt;
		if (PadHeld(PB_A)) boost = 1;
	}
	else if (!PAD::IS_USING_KEYBOARD_AND_MOUSE(0))
	{
		// a pad the game reads itself (e.g. a DualSense): its move stick and triggers
		mx += PAD::GET_DISABLED_CONTROL_NORMAL(0, Joaat("INPUT_MOVE_LR"));
		my -= PAD::GET_DISABLED_CONTROL_NORMAL(0, Joaat("INPUT_MOVE_UD"));
		up += PAD::GET_DISABLED_CONTROL_NORMAL(0, Joaat("INPUT_VEH_ACCELERATE")) - PAD::GET_DISABLED_CONTROL_NORMAL(0, Joaat("INPUT_VEH_BRAKE"));
		if (PAD::IS_DISABLED_CONTROL_PRESSED(0, Joaat("INPUT_FRONTEND_ACCEPT"))) boost = 1;
	}
	mx = Clamp(mx, -1, 1); my = Clamp(my, -1, 1); up = Clamp(up, -1, 1);
	g_bal.boost = Lerp(g_bal.boost, boost, Clamp(dt * 3.0f, 0, 1));
	// v1.2: a fresh press - Arthur yanks the burner line (Rockstar's own balloon clip) and the jets whoosh
	if (boost > 0.5f && !g_bal.boostHeld && t - g_bal.whooshAt > 2.5f)
	{
		g_bal.whooshAt = t;
		Ped me = PLAYER::PLAYER_PED_ID();
		const char* dict = "script_story@gng2@ig@ig_2_balloon_control";
		if (STREAMING::HAS_ANIM_DICT_LOADED(dict))
			TASK::TASK_PLAY_ANIM(me, dict, "pull_burner_arthur", 4.0f, -4.0f, -1, 16 | 8 | 4, 0.0f, FALSE, 0, FALSE, nullptr, FALSE);
		else
			STREAMING::REQUEST_ANIM_DICT(dict);
		UI::PlayWhoosh();
	}
	g_bal.boostHeld = boost > 0.5f;
	float speed = Lerp(13.0f, 38.0f, g_bal.boost);   // a drift, or a jet
	*wantVel = (fwd * my + right * mx) * speed;
	wantVel->z = up * Lerp(7.0f, 13.0f, g_bal.boost);
	// while the balloon is ours, the game mustn't drive, shoot, jump or throw Arthur out with these; the camera stays free
	static const char* kBlock[] = { "INPUT_VEH_ACCELERATE", "INPUT_VEH_BRAKE", "INPUT_VEH_MOVE_LR", "INPUT_VEH_MOVE_UD", "INPUT_VEH_FLY_THROTTLE_UP",
		"INPUT_VEH_FLY_THROTTLE_DOWN", "INPUT_JUMP", "INPUT_DUCK", "INPUT_SPRINT", "INPUT_MOVE_LR", "INPUT_MOVE_UD", "INPUT_ATTACK", "INPUT_AIM",
		"INPUT_VEH_EXIT", "INPUT_ENTER", "INPUT_COVER" };
	for (const char* a : kBlock) PAD::DISABLE_CONTROL_ACTION(0, Joaat(a), TRUE);
	(void)t;
}

static void ChaseUpdate(float dt, float t);

static void BalloonApply(float dt)
{
	V3 v = g_bal.vel;
	V3 hv(v.x, v.y, 0);
	if (hv.len() > 1.5f)
	{
		float want = atan2f(-hv.x, hv.y) * 180.0f / PI;
		float d = fmodf(want - g_bal.heading + 540.0f, 360.0f) - 180.0f;
		g_bal.heading += Clamp(d, -80.0f * dt, 80.0f * dt);
	}
	V3 face = g_bal.pos + HeadingDir(g_bal.heading) * 40.0f;
	if (g_bal.drive == 0 && g_bal.veh)
	{
		VEHICLE::SET_VELOCITY_FOR_BALLOON(g_bal.veh, v.x, v.y, v.z);
		VEHICLE::SET_BALLOON_FACE_COORD(g_bal.veh, face.x, face.y, face.z);
	}
	else if (g_bal.drive == 1)
	{
		ENTITY::SET_ENTITY_VELOCITY(g_bal.body, v.x, v.y, v.z);
		ENTITY::SET_ENTITY_HEADING(g_bal.body, g_bal.heading);
	}
	else
	{
		ENTITY::FREEZE_ENTITY_POSITION(g_bal.body, TRUE);
		g_bal.pos = g_bal.pos + v * dt;
		ENTITY::SET_ENTITY_COORDS_NO_OFFSET(g_bal.body, g_bal.pos.x, g_bal.pos.y, g_bal.pos.z, FALSE, FALSE, FALSE);
		ENTITY::SET_ENTITY_ROTATION(g_bal.body, -Clamp(hv.len() * 0.18f, 0.0f, 7.0f), 0, g_bal.heading, 2, TRUE);
	}
}

void BalloonUpdate(float dt, float t)
{
	if (!g_bal.on) return;
	if (!g_bal.body || !ENTITY::DOES_ENTITY_EXIST(g_bal.body)) { BalloonRemove("the balloon is gone"); return; }
	if (g_bal.drive < 2) g_bal.pos = V3(ENTITY::GET_ENTITY_COORDS(g_bal.body, FALSE, FALSE));
	if (g_bal.veh && t > g_bal.envelopeCheckAt && !g_bal.envelope)
	{
		// the cloth envelope is a separate object the game makes; Rockstar waits for it before using the balloon
		g_bal.envelopeCheckAt = t + 1.0f;
		g_bal.envelope = VEHICLE::GET_BALLOON_OBJECT_FROM_VEHICLE(g_bal.veh);
		if (g_bal.envelope && ENTITY::DOES_ENTITY_EXIST(g_bal.envelope))
		{
			SetScripted(g_bal.envelope, true);
			Log("balloon: envelope %d is up (%.1f s after spawning)", g_bal.envelope, t - g_bal.startedAt);
		}
		else
		{
			g_bal.envelope = 0;
			if (t - g_bal.startedAt > 6.0f && t - g_bal.startedAt < 7.1f) Log("balloon: no envelope after 6 s (a known RDR2 quirk) - flying the basket anyway");
		}
	}
	if (g_bal.waiting) return;
	if (g_bal.parked)
	{
		// left behind: let it settle; removed after 3 minutes, or 400 m away
		if (g_bal.drive == 2)
		{
			float gz = GroundZ(g_bal.pos.x, g_bal.pos.y, g_bal.pos.z + 5.0f, g_bal.pos.z - 50.0f);
			if (g_bal.pos.z + g_bal.basketZ > gz + 0.1f) g_bal.pos.z -= 1.5f * dt;
			ENTITY::SET_ENTITY_COORDS_NO_OFFSET(g_bal.body, g_bal.pos.x, g_bal.pos.y, g_bal.pos.z, FALSE, FALSE, FALSE);
		}
		else if (g_bal.veh)
			VEHICLE::SET_VELOCITY_FOR_BALLOON(g_bal.veh, 0, 0, -1.5f);
		// still in the seat a second after bailing (the leave task didn't take): out at once - the soft landing has him
		Ped me = PLAYER::PLAYER_PED_ID();
		if (t - g_bal.parkedAt > 1.0f && t - g_bal.parkedAt < 1.2f && g_bal.veh && PED::IS_PED_IN_VEHICLE(me, g_bal.veh, FALSE))
		{
			TASK::CLEAR_PED_TASKS_IMMEDIATELY(me, FALSE, TRUE);
			Log("balloon: the leave task didn't take - Arthur pulled out");
		}
		if (t - g_bal.parkedAt > 180.0f || (PlayerPos() - g_bal.pos).len2d() > 400.0f) BalloonRemove("left behind");
		return;
	}
	if (PlayerDead()) { BalloonBail(t); return; }
	if (t > g_bal.seatCheckAt)
	{
		g_bal.seatCheckAt = t + 0.5f;
		if (!BalloonAboard())
		{
			// knocked out of it (the known RDR2 balloon problem): put him back, a few times
			if (g_bal.reseats < 4) { g_bal.reseats++; BalloonSeat(); Log("balloon: Arthur fell out - put back (%d)", g_bal.reseats); }
			else { Log("balloon: Arthur keeps falling out - letting him go"); BalloonBail(t); return; }
		}
	}
	V3 want;
	if (g_bal.intro)
		want = g_bal.cmdVel;
	else
	{
		BalloonControls(dt, t, &want);
		DWORD fgPid = 0;
		GetWindowThreadProcessId(GetForegroundWindow(), &fgPid);
		bool bail = (fgPid == GetCurrentProcessId() && (GetAsyncKeyState('F') & 0x8000) != 0) || PadPressed(PB_Y);
		if (bail && t - g_bal.startedAt > 1.5f) { BalloonBail(t); return; }
		if (!g_auto.running && !g_menuOpen && (Pressed(g_keys.next) || (g_padOn && PadComboPressed(g_pad.next))))
		{
			g_bal.aim = (g_bal.aim + 1) % kJetAimCount;
			BalloonStartFx();
			Notify(std::string("Jet flame aim ") + std::to_string(g_bal.aim + 1) + "/" + std::to_string(kJetAimCount) + ": " + kJetAims[g_bal.aim].name, 3000);
			Finding("BALLOON jet aim -> %d %s", g_bal.aim, kJetAims[g_bal.aim].name);
		}
	}
	if (g_bal.chase)
	{
		ChaseUpdate(dt, t);
		if (g_bal.chaseCaughtAt >= 0) return;   // the tornado has it (ChaseUpdate steers it)
	}
	g_bal.vel = g_bal.vel + (want - g_bal.vel) * Clamp(dt * (g_bal.intro ? 3.0f : 1.7f), 0.0f, 1.0f);
	float h = 0;
	if (TrueHeight(g_bal.pos + V3(0, 0, g_bal.basketZ), &h))
	{
		if (!(g_bal.intro && g_bal.skim) && h < 3.0f && g_bal.vel.z < 2.0f) g_bal.vel.z = 2.0f + (3.0f - h);   // never scrape along the ground
		if (h > 450.0f && g_bal.vel.z > 0) g_bal.vel.z = 0;
	}
	if (!(g_bal.intro && g_bal.skim)) g_bal.vel.z += sinf(t * 0.9f) * 0.3f;   // a gentle bob (v1.6.1 review: not in the swoop - it swamped the height hold)
	BalloonApply(dt);
	// is the game doing what it's told? (measured over 1.5 s while it's asked to move)
	float cmd = g_bal.vel.len();
	if (t > g_bal.checkAt)
	{
		float moved = (g_bal.pos - g_bal.checkFrom).len();
		float expected = g_bal.checkCmd * 1.5f;
		bool bad = g_bal.drive < 2 && expected > 9.0f && moved < expected * 0.3f;
		g_bal.driveBad = bad ? g_bal.driveBad + 1 : 0;
		if (bad && g_bal.driveBad >= 2)   // two checks in a row (one could just be a cliff in the way)
		{
			g_bal.driveBad = 0;
			g_bal.drive++;
			Log("balloon: moved %.1f m of %.1f expected - switching to %s", moved, expected, kDriveNames[g_bal.drive]);
			Finding("BALLOON drive -> %s (moved %.1f of %.1f m)", kDriveNames[g_bal.drive], moved, expected);
			ENTITY::FREEZE_ENTITY_POSITION(g_bal.body, g_bal.drive == 2 ? TRUE : FALSE);
		}
		g_bal.checkAt = t + 1.5f;
		g_bal.checkFrom = g_bal.pos;
		g_bal.checkCmd = cmd;
	}
	else
		g_bal.checkCmd = std::max(g_bal.checkCmd * 0.98f, cmd);
	// the jets roar with the boost
	float boost = std::max(g_bal.boost, g_bal.boostShown);
	float k = 0.6f + 1.6f * boost + Clamp(cmd / 40.0f, 0.0f, 0.6f);
	if (g_bal.intro) k *= 0.45f;   // v1.6 (playtest 14: in the intro's close-ups the jets were walls of fire either side of Arthur)
	for (int fx : g_bal.jetFx) if (fx) GRAPHICS::SET_PARTICLE_FX_LOOPED_SCALE(fx, k);
	if (g_bal.trailFx) GRAPHICS::SET_PARTICLE_FX_LOOPED_SCALE(g_bal.trailFx, 0.6f + 1.4f * boost);
	if (g_bal.burnerFx) GRAPHICS::SET_PARTICLE_FX_LOOPED_SCALE(g_bal.burnerFx, 1.1f + (g_bal.vel.z > 1.0f ? 1.0f : 0.0f));
	if (!g_bal.intro && !g_bal.helpShown && t - g_bal.startedAt > 1.2f)
	{
		g_bal.helpShown = true;
		UI::HelpTip(PadRecentlyUsed()
			? "JET BALLOON\nLeft stick  fly where you look\nRT / LT  up / down\nA  jet boost      Y  bail out"
			: "JET BALLOON\nW A S D  fly where you look\nSPACE / CTRL  up / down\nSHIFT  jet boost      F  bail out", 10.0f);
	}
}

// ======================= Storm chaser (balloon challenge) =======================
// Fly as close to the funnel as you dare: points come faster the closer you are (x1 at the edge of the pull, x10 at the wall),
// and touching the wall gets you caught - it spins the balloon round and throws it out. 90 seconds; the best score is saved.
// (the best scores live in TornadoRedemption_best.txt - ReadBest / WriteBest in script.cpp)

void ChaseEnd(const char* why)
{
	if (!g_bal.chase) return;
	g_bal.chase = false;
	bool best = g_bal.chaseScore > g_bal.chaseBest && g_bal.chaseScore > 0;
	if (best) { g_bal.chaseBest = g_bal.chaseScore; WriteBest("chaser", g_bal.chaseBest); }
	char b[160];
	sprintf_s(b, "%s  -  %d points%s", why, (int)g_bal.chaseScore, best ? "   NEW BEST!" : "");
	g_bal.chaseResult = b;
	g_bal.chaseResultUntil = NowSec() + 8.0f;
	g_set.movement = g_bal.savedMove;
	Finding("STORM CHASER %s score %.0f (best %.0f)", why, g_bal.chaseScore, g_bal.chaseBest);
	if (best) UI::Shard("NEW BEST", (std::to_string((int)g_bal.chaseScore) + " points").c_str(), 4.0f);
}

static void ChaseStart()
{
	AutoStop("replaced");
	SurvivalStop("STOPPED");
	if (!g_bal.chaseBestLoaded) { g_bal.chaseBest = ReadBest("chaser"); g_bal.chaseBestLoaded = true; }
	DespawnAll();
	BalloonRemove("new chase");
	V3 p = PlayerPos();
	if (!BalloonSpawn(p, CameraHeading())) return;
	g_bal.savedMove = g_set.movement;
	g_set.movement = 1;   // it wanders: you go to it
	Tornado* tp = SpawnTornado(g_set.style, 230.0f, 0, false, GetStyles()[g_set.style].name, -1, 0.0f);
	if (tp) TouchCamStart(tp);
	g_bal.cmdVel = V3(0, 0, 8.0f);
	g_bal.chase = true;
	g_bal.chaseStart = NowSec() + 8.0f;   // the clock starts once it has touched down
	g_bal.chaseScore = 0;
	g_bal.chaseMult = 0;
	g_bal.chaseCaughtAt = -1;
	UI::Shard("STORM CHASER", "Get as close as you dare. Touch the wall and it has you.", 4.5f);
	Log("STORM CHASER started (best %.0f)", g_bal.chaseBest);
}

static void ChaseUpdate(float dt, float t)
{
	float nd;
	Tornado* tp = NearestTornado(&nd);
	if (!tp) { ChaseEnd("THE STORM PASSED"); return; }
	nd = (g_bal.pos - tp->base).len2d();
	if (t < g_bal.chaseStart) return;
	float el = t - g_bal.chaseStart;
	if (g_bal.chaseCaughtAt >= 0)
	{
		// caught: round the funnel, climbing, then thrown out
		float ct = t - g_bal.chaseCaughtAt;
		g_bal.caughtAng += dt * 2.0f;
		g_bal.caughtH += dt * 8.0f;
		V3 want = tp->base + V3(cosf(g_bal.caughtAng) * g_bal.caughtR, sinf(g_bal.caughtAng) * g_bal.caughtR, g_bal.caughtH);
		g_bal.vel = (want - g_bal.pos) * 3.0f;
		if (g_bal.vel.len() > 70.0f) g_bal.vel = g_bal.vel * (70.0f / g_bal.vel.len());
		BalloonApply(dt);
		if (ct > 4.5f)
		{
			V3 out(cosf(g_bal.caughtAng), sinf(g_bal.caughtAng), 0);
			g_bal.vel = out * 40.0f + V3(0, 0, 6.0f);
			g_bal.chaseCaughtAt = -1;
			ChaseEnd("CAUGHT");
		}
		return;
	}
	float wall = tp->wallRadius(), wallOuter = wall * 1.6f, band = std::max(40.0f, tp->reachRadius() * 0.7f);
	float h = g_bal.pos.z - tp->base.z;
	if (nd < wall * 1.25f && h < tp->height() * 0.85f)
	{
		g_bal.chaseCaughtAt = t;
		V3 rel = g_bal.pos - tp->base;
		g_bal.caughtAng = atan2f(rel.y, rel.x);
		g_bal.caughtR = std::max(wall, rel.len2d());
		g_bal.caughtH = std::max(5.0f, h);
		UI::Shard("CAUGHT", "", 2.0f);
		Log("STORM CHASER: caught %.0f m from the centre, %.0f m up, score %.0f", nd, h, g_bal.chaseScore);
		return;
	}
	float close = Clamp(1.0f - (nd - wallOuter) / band, 0.0f, 1.0f);
	g_bal.chaseMult = close > 0 ? 1.0f + 9.0f * close * close : 0.0f;
	if (h > 3.0f) g_bal.chaseScore += g_bal.chaseMult * 10.0f * dt;
	if (el > 90.0f) ChaseEnd("TIME");
}

static void ChaseDraw()
{
	if (g_bal.chase && NowSec() >= g_bal.chaseStart)
	{
		float left = std::max(0.0f, 90.0f - (NowSec() - g_bal.chaseStart));
		char score[32], sub[96], best[48] = "";
		sprintf_s(score, "%d", (int)g_bal.chaseScore);
		if (g_bal.chaseCaughtAt >= 0) sprintf_s(sub, "IT HAS YOU");
		else if (g_bal.chaseMult > 0) sprintf_s(sub, "x%.1f      %.0f s left", g_bal.chaseMult, left);
		else sprintf_s(sub, "get closer      %.0f s left", left);
		if (g_bal.chaseBest > 0) sprintf_s(best, "best %d", (int)g_bal.chaseBest);
		UI::ScorePlate("STORM CHASER", score, sub, best, Clamp(g_bal.chaseMult / 10.0f, 0.0f, 1.0f));
	}
	else if (g_bal.chase)
		UI::ScorePlate("STORM CHASER", "", "it's touching down...", "", 0);
	else if (NowSec() < g_bal.chaseResultUntil)
		UI::ScorePlate("STORM CHASER", "", g_bal.chaseResult.c_str(), "", 0);
}

// v1.1 audit 2: both best scores, read at startup (LoadConfig) - they used to wait for the first run, so the menu's help left them out
static void LoadBests()
{
	LoadBest();
	g_bal.chaseBest = ReadBest("chaser");
	g_bal.chaseBestLoaded = true;
}
