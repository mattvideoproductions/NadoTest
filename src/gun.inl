// NadoTest v1.1 - the tornado gun. Part of script.cpp. The user: "can we make an optional mini tornado gun we can use :))".
// With it on, every shot Arthur fires (any gun) spawns a mini twister where the bullet hit: a pocket-sized Toon Twister (v1.4:
// about as tall as a person) that wanders about where it landed for 30 s, spinning up whatever's in its little reach, and dies down. Up to three at once (the oldest goes when a fourth lands); they never touch
// the weather and don't count as "the" tornado for the HUD, the camera or the challenges.

// v1.5 (playtest 13: "if you shoot someone it makes them spin and then sends them absolutely careening in a random direction"):
// someone the bullet hits is spun round the little twister for a second, then knocked back the way the bullet was going - away
// from Arthur - like a real hit. The twister still spawns where they stood.
struct GunVictim { Ped p = 0; V3 home, dir; float t0 = 0; bool flung = false; };

struct GunState
{
	bool on = false;
	float next = 0;
	V3 lastImpact;
	int fired = 0;
	std::vector<TornadoRef> minis;
	bool spinPeople = true;
	std::vector<GunVictim> victims;
	int spun = 0;
} g_gun;

// the person the shot hit, if it hit one: a probe from the camera through the impact point
static Ped GunVictimAt(const V3& at)
{
	Ped me = PLAYER::PLAYER_PED_ID();
	V3 from = CAMERA::GET_GAMEPLAY_CAM_COORD();
	V3 d = at - from;
	float L = d.len();
	if (L < 0.5f || L > 250.0f) return 0;
	V3 to = at + d * (0.8f / L);
	int h = SHAPETEST::START_EXPENSIVE_SYNCHRONOUS_SHAPE_TEST_LOS_PROBE(from.x, from.y, from.z, to.x, to.y, to.z, -1, me, 0);
	BOOL hit = FALSE;
	Vector3 end = {}, normal = {};
	Entity e = 0;
	SHAPETEST::GET_SHAPE_TEST_RESULT(h, &hit, &end, &normal, &e);
	if (!hit || !e || e == me || ENTITY::GET_ENTITY_TYPE(e) != 1) return 0;
	if (PED::IS_PED_ON_MOUNT(me) && e == PED::GET_MOUNT(me)) return 0;
	if (IsScripted(e)) return 0;
	return e;
}

static void GunVictims(float t)
{
	for (size_t i = 0; i < g_gun.victims.size();)
	{
		GunVictim& v = g_gun.victims[i];
		if (!v.p || !ENTITY::DOES_ENTITY_EXIST(v.p) || t - v.t0 > 3.0f)
		{
			if (v.p) SetScripted(v.p, false);   // the twister may have them now, like anyone
			g_gun.victims.erase(g_gun.victims.begin() + i);
			continue;
		}
		V3 p = ENTITY::GET_ENTITY_COORDS(v.p, FALSE, FALSE);
		if (t - v.t0 < 1.0f)
		{
			// round the twister, 0.7 m out, lifting to about chest height
			V3 rel = p - v.home; rel.z = 0;
			float r = std::max(0.2f, rel.len2d());
			V3 out(rel.x / r, rel.y / r, 0), tang(-out.y, out.x, 0);
			V3 vel = tang * 7.5f + out * ((0.7f - r) * 4.0f);   // (v1.6, playtest 14: "a little strength boost")
			vel.z = p.z < v.home.z + 1.2f ? 2.2f : 0.3f;
			ENTITY::SET_ENTITY_VELOCITY(v.p, vel.x, vel.y, vel.z);
		}
		else if (!v.flung)
		{
			v.flung = true;
			V3 vel = v.dir * 14.0f + V3(0, 0, 6.5f);   // knocked back, the way the bullet was going (v1.6: harder)
			ENTITY::SET_ENTITY_VELOCITY(v.p, vel.x, vel.y, vel.z);
		}
		i++;
	}
}

static void GunFire(const V3& at)
{
	// tidy the list, and make room
	for (size_t i = 0; i < g_gun.minis.size();)
	{
		Tornado* m = g_gun.minis[i].get();
		if (!m || m->Dissipating()) g_gun.minis.erase(g_gun.minis.begin() + i); else i++;
	}
	if (g_gun.minis.size() >= 3)
	{
		if (Tornado* oldest = g_gun.minis.front().get()) oldest->BeginDissipate(NowSec());
		g_gun.minis.erase(g_gun.minis.begin());
	}
	SpawnOpts o;
	o.mini = true;
	// v1.4 (playtest 12: "you want them to be desktop size, like the size of a person or a little smaller"): a pocket twister,
	// about 1.7 m tall (105 m x sqrt(0.03) x 0.094), a 0.5 m wall and a 2.5 m reach - small but still violent (the physics are
	// boosted for minis), with small smoke and no dust wall
	o.tiny = true;
	o.size = 0.03f;
	o.heightMul = 0.094f;
	o.reach = 5.0f;
	o.grow = 0.6f;
	o.life = 30.0f;            // "they should die down after 30 seconds"
	V3 p = at;
	p.z = GroundZ(p.x, p.y, p.z + 3.0f, p.z);
	if (g_gun.spinPeople && g_gun.victims.size() < 4)
		if (Ped v = GunVictimAt(at))
		{
			GunVictim gv;
			gv.p = v; gv.home = p; gv.t0 = NowSec();
			gv.dir = FlatDir(PlayerPos(), at);
			SetScripted(v, true);   // (the twister's own physics leaves them to this)
			PED::SET_PED_TO_RAGDOLL(v, 3500, 5000, 0, FALSE, FALSE, nullptr);
			g_gun.victims.push_back(gv);
			g_gun.spun++;
			if (g_gun.spun <= 5 || g_gun.spun % 20 == 0) Log("GUN hit a person (%d): spun, then knocked back away from Arthur - %d so far", v, g_gun.spun);
		}
	// v1.3 (playtest 11: "cute little mini tornados that wander around"): the Toon Twister, small - a cartoon tube that hops
	Tornado* tp = SpawnTornadoAt(p, RandRange(0, 360), 4 /* T: Toon Twister */, "Mini twister", o);
	if (!tp) return;
	g_gun.minis.push_back(TornadoRef(tp));
	g_gun.fired++;
	if (LoadPtfxAsset("core", 0))
	{
		GRAPHICS::USE_PARTICLE_FX_ASSET("core");
		GRAPHICS::START_PARTICLE_FX_NON_LOOPED_AT_COORD("bang_dirt_dry", p.x, p.y, p.z + 0.3f, 0, 0, 0, 0.5f, FALSE, FALSE, FALSE);
	}
	if (g_gun.fired <= 5 || g_gun.fired % 20 == 0) Log("GUN mini twister %d at (%.0f, %.0f, %.0f), %d out", g_gun.fired, p.x, p.y, p.z, (int)g_gun.minis.size());
}

void GunUpdate(float t)
{
	// v1.1 audit 2: not in a story mission (a twister can fail it) or while an auto test runs
	GunVictims(t);
	if (!g_gun.on || IntroRunning() || PlayerDead() || g_auto.running || MISC::GET_MISSION_FLAG()) return;
	Ped me = PLAYER::PLAYER_PED_ID();
	if (t < g_gun.next || !PED::IS_PED_SHOOTING(me)) return;
	Vector3 c = {};
	V3 at;
	if (WEAPON::GET_PED_LAST_WEAPON_IMPACT_COORD(me, &c) && (V3(c) - g_gun.lastImpact).len() > 0.5f)
		at = V3(c);
	else
	{
		// into the sky (no impact): where the camera's looking, up to 120 m away
		V3 from = CAMERA::GET_GAMEPLAY_CAM_COORD();
		V3 rot = CAMERA::GET_GAMEPLAY_CAM_ROT(2);
		float pitch = rot.x * PI / 180.0f;
		V3 dir = HeadingDir(rot.z) * cosf(pitch) + V3(0, 0, sinf(pitch));
		V3 to = from + dir * 120.0f;
		int h = SHAPETEST::START_EXPENSIVE_SYNCHRONOUS_SHAPE_TEST_LOS_PROBE(from.x, from.y, from.z, to.x, to.y, to.z, -1, me, 0);
		BOOL hit = FALSE;
		Vector3 end = {}, normal = {};
		Entity ent = 0;
		SHAPETEST::GET_SHAPE_TEST_RESULT(h, &hit, &end, &normal, &ent);
		at = hit ? V3(end) : from + HeadingDir(rot.z) * 60.0f;
	}
	g_gun.lastImpact = at;
	g_gun.next = t + 0.7f;
	GunFire(at);
}
