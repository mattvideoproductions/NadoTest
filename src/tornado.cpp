// Tornado Redemption - tornado implementation.
#include "tornado.h"
#include <algorithm>
#include <cstring>

Settings g_set;
std::unordered_set<Entity> g_priority;
static std::unordered_set<Entity> g_scripted;
bool g_shieldPlayer = false;   // v1.1: the intro and the balloon keep the tornado's hands off Arthur
void SetScripted(Entity e, bool on) { if (!e) return; if (on) g_scripted.insert(e); else g_scripted.erase(e); }
bool IsScripted(Entity e) { return g_scripted.count(e) != 0; }
void ClearScripted() { g_scripted.clear(); }

// Playtest 2: HURRICANE renders as a clear blue sky in RDR2. Playtest 4: "no one needs sandstorm weather". Both gone.
const char* kWeatherTypes[] = { "THUNDERSTORM", "THUNDER", "RAIN", "OVERCASTDARK" };
const char* kWeatherLabels[] = { "Thunderstorm", "Thunder", "Rain", "Overcast dark" };
const int kWeatherTypeCount = sizeof(kWeatherTypes) / sizeof(kWeatherTypes[0]);

static const ForceProfile kForces[] = {
	// name       vmax  lift  eject tumble
	{ "Gentle",   20.f,  7.f, 14.f, 0.6f },
	{ "Violent",  38.f, 14.f, 26.f, 1.0f },
	{ "Extreme",  60.f, 22.f, 40.f, 1.4f },
};
const ForceProfile& CurrentForce() { return kForces[(g_set.force >= 0 && g_set.force <= 2) ? g_set.force : 1]; }

static const float kSizeMul[] = { 0.7f, 1.0f, 1.5f };
static const float kTinyFxScale = 0.085f;      // v1.4: the pocket twister's smoke, x the style's plume sizes (2-6 -> 0.2-0.5)
static const float kMaxArthurLaunch = 60.0f;   // v1.4: m/s - the fastest the tornado ever throws Arthur (playtest 12's crash came after 90)
static const float kReachMul[] = { 4.0f, 6.0f, 8.0f, 11.0f };   // multiples of the wall radius
float ReachMul() { return kReachMul[(g_set.reach >= 0 && g_set.reach <= 3) ? g_set.reach : 2]; }
// Playtest 4: "maybe make it move a little bit slower, or we can have an adjustable movement speed". v0.3 was 6-8 m/s.
static const float kSpeeds[] = { 3.5f, 6.0f, 10.0f };
float MoveSpeed() { return kSpeeds[(g_set.speed >= 0 && g_set.speed <= 2) ? g_set.speed : 1]; }

// ======================= shared world pool =======================
// One shared read of the world's peds / wagons / props (with positions), refreshed at most every 0.2 s, so several
// tornadoes don't each scan up to 8192 objects four times a second.
struct PoolEntry { Entity e; int type; V3 pos; bool isPlayer; };
static std::vector<PoolEntry> g_pool;
static float g_poolTime = -100.0f;
static float g_poolMs = 0;
float PoolScanMs() { return g_poolMs; }
static int g_rawPeds = 0, g_rawVehs = 0, g_rawObjs = 0;
// Playtest 4 settled it: after Arthur respawned in Rhodes the game's own lists read 0 / 0 / 0 for ~70 s (the new
// per-stage SNAP log showed the RAW read was empty, not our filters). So an empty read is treated as a failed read:
// keep the last good list (positions refreshed, dead handles dropped) and try again soon.
static std::vector<PoolEntry> g_lastGood;
static float g_lastGoodT = -100.0f;
static int g_emptyStreak = 0, g_emptyReads = 0;
static float g_emptySince = 0;
int PoolEmptyReads() { return g_emptyReads; }
static int g_topUps = 0;
static float g_nextTopUp = 0;
int PoolTopUps() { return g_topUps; }

// v0.5, playtest 5 camp: during a long empty-read stretch the camp respawned its people and horses with NEW handles,
// so the stale list drained and the tornado grabbed nothing for ~20 s (41:40-41:55). While reads are empty, ask the
// game for the peds near Arthur (its own nearby list, a different source from Script Hook's pool read) and add any
// the stale list doesn't know. Every handle is checked (exists + is a ped) before it is used.
static void TopUpNearbyPeds(float t)
{
	if (t < g_nextTopUp)
		return;
	g_nextTopUp = t + 1.0f;
	const int kMax = 64;
	static int buf[2 * kMax + 2];   // GTA-style layout: [0] = capacity, then 8-byte slots
	memset(buf, 0, sizeof(buf));
	buf[0] = kMax;
	Ped player = PLAYER::PLAYER_PED_ID();
	int found = PED::GET_PED_NEARBY_PEDS(player, buf, -1, 0);
	if (found <= 0)
		return;
	found = std::min(found, kMax);
	std::unordered_set<Entity> known;
	for (auto& pe : g_pool) known.insert(pe.e);
	int added = 0;
	for (int i = 0; i < found; i++)
	{
		Ped p = buf[i * 2 + 2];
		if (!p || known.count(p) || IsScripted(p) || !ENTITY::DOES_ENTITY_EXIST(p) || ENTITY::GET_ENTITY_TYPE(p) != 1 || p == player)
			continue;
		bool human = PED::IS_PED_HUMAN(p) != 0;
		if (!((human && g_set.grabPeople) || (!human && g_set.grabAnimals)))
			continue;
		g_pool.push_back({ p, 0, ENTITY::GET_ENTITY_COORDS(p, FALSE, FALSE), false });
		g_lastGood.push_back(g_pool.back());
		known.insert(p);
		added++;
	}
	if (added)
	{
		g_topUps += added;
		static float lastLog = -100;
		if (t - lastLog > 5.0f) { lastLog = t; Log("POOL fallback topped up %d nearby peds (total %d)", added, g_topUps); }
	}
}
bool PoolStale() { return g_emptyStreak > 0; }

bool InBigTown(const V3& p, float margin);
int g_poolSkipsObjects = 0;   // v1.6.2: reads made without the object list (in or near a big town)
static float g_poolWaitUntil = -100.0f;   // v1.6.2: after an empty read, no read until then
static void RefreshPool(float t)
{
	if (t < g_poolWaitUntil && g_poolWaitUntil - t < 3.0f)   // (the second test: a clock that went back doesn't wedge it)
		return;
	if (t - g_poolTime < 0.2f && t >= g_poolTime)
		return;
	g_poolTime = t;
	DWORD t0 = GetTickCount();
	static int peds[2048], vehs[2048], objs[8192];
	// v1.6.2 (five crashes, ALL in Saint Denis - and the four that left a log each END on "POOL EMPTY ... using the last good
	// list (199-658 entities)": the game's entity lists read empty, and the game froze for 12-21 s and gave up). Script Hook's
	// list read gives every entity it returns a script handle, and the game has only so many; Saint Denis - its hundreds of
	// props, more still from the air - is where they run out: first our reads come back empty, then the game's own scripts
	// can't get one. In and near the big towns (with Town safety on) the object list isn't read at all - there's nothing there
	// it may rip loose anyway - and only people and wagons are.
	V3 camAt = CAMERA::GET_FINAL_RENDERED_CAM_COORD();
	bool crowded = g_set.citySafe && (InBigTown(ENTITY::GET_ENTITY_COORDS(PLAYER::PLAYER_PED_ID(), TRUE, FALSE), 300.0f) || InBigTown(camAt, 300.0f));
	g_rawPeds = worldGetAllPeds(peds, 2048);
	g_rawVehs = worldGetAllVehicles(vehs, 2048);
	g_rawObjs = crowded ? 0 : worldGetAllObjects(objs, 8192);
	if (crowded) g_poolSkipsObjects++;
	bool empty = g_rawPeds + g_rawVehs + g_rawObjs == 0;
	if (empty && !g_lastGood.empty() && t - g_lastGoodT < 45.0f)
	{
		g_emptyReads++;
		if (g_emptyStreak++ == 0)
		{
			g_emptySince = t;
			// (v1.6.2: no second, diagnostic read - an empty read is the warning sign; another only adds to the load)
			Log("POOL EMPTY: the game's entity lists read 0/0/0 - using the last good list from %.1f s ago (%d entities); next read in 1.5 s",
				t - g_lastGoodT, (int)g_lastGood.size());
		}
		g_pool.clear();
		for (const PoolEntry& pe : g_lastGood)
			if (ENTITY::DOES_ENTITY_EXIST(pe.e))
				g_pool.push_back({ pe.e, pe.type, ENTITY::GET_ENTITY_COORDS(pe.e, FALSE, FALSE), pe.isPlayer });
		if (g_set.grabPeople || g_set.grabAnimals)
			TopUpNearbyPeds(t);
		g_poolWaitUntil = t + 1.5f;   // v1.6.2: back off (it used to retry sooner, 10 times a second - right into the crashes)
		g_poolMs = (float)(GetTickCount() - t0);
		return;
	}
	if (!empty && g_emptyStreak)
	{
		Log("POOL back after %d empty reads (%.1f s)", g_emptyStreak, t - g_emptySince);
		g_emptyStreak = 0;
	}
	g_pool.clear();
	Ped player = PLAYER::PLAYER_PED_ID();
	Ped playerMount = PED::IS_PED_ON_MOUNT(player) ? PED::GET_MOUNT(player) : 0;
	for (int i = 0; i < g_rawPeds; i++)
	{
		Ped p = peds[i];
		if (IsScripted(p))
			continue;   // v1.1: a scene is moving it
		bool isPlayer = p == player || (playerMount && p == playerMount);
		if (!isPlayer)
		{
			bool human = PED::IS_PED_HUMAN(p) != 0;
			if (!((human && g_set.grabPeople) || (!human && g_set.grabAnimals)))
				continue;
		}
		g_pool.push_back({ p, 0, ENTITY::GET_ENTITY_COORDS(p, FALSE, FALSE), isPlayer });
	}
	if (g_set.grabVehicles)
		for (int i = 0; i < g_rawVehs; i++)
			if (!IsScripted(vehs[i]) && !VEHICLE::IS_THIS_MODEL_A_TRAIN(ENTITY::GET_ENTITY_MODEL(vehs[i])))
				g_pool.push_back({ vehs[i], 1, ENTITY::GET_ENTITY_COORDS(vehs[i], FALSE, FALSE), false });
	if (g_set.grabProps)
		for (int i = 0; i < g_rawObjs; i++)
			if (!IsAnchor(objs[i]) && !IsOrbiter(objs[i]) && !IsScripted(objs[i]) && !ENTITY::IS_ENTITY_ATTACHED(objs[i]))
				g_pool.push_back({ objs[i], 2, ENTITY::GET_ENTITY_COORDS(objs[i], FALSE, FALSE), false });
	if (!empty)
	{
		g_lastGood = g_pool;
		g_lastGoodT = t;
	}
	g_poolMs = (float)(GetTickCount() - t0);
}

static const char* kDebrisModels[] = {
	"p_woodplank01x", "p_woodplank02x", "p_woodplank03x", "p_barrel02x", "p_crate01x", "p_crate03x",
	"p_haybale01x", "p_bucket01x", "p_chair02x", "p_wagonwheel01", "p_door01x", "p_barrelhalf01x"
};

// Astra: LoadModel/LoadPtfxAsset WAIT(0) inside the update, which stalls the whole mod (menu, other tornadoes).
// Load everything once at startup; later calls then return immediately.
void PreloadTornadoAssets()
{
	LoadPtfxAsset("core", 5000);
	LoadModel(H("p_apple01x"), 5000);
	for (const char* m : kDebrisModels) LoadModel(H(m), 3000);
	LoadModel(H("p_tree_fallen_pineprop"), 3000);
	LoadModel(H("p_cs_treefallen01x"), 3000);
	Log("assets preloaded");
}

int AnchorCount() { return AnchorsAlive(); }

int Tornado::LoopsAlive() const
{
	int n = 0;
	for (auto& e : emitters)
		if (e.fx && GRAPHICS::DOES_PARTICLE_FX_LOOPED_EXIST(e.fx)) n++;
	return n;
}

static int g_loopsInUse = 0;
int LoopsInUse() { return g_loopsInUse; }
void AddLoopsInUse(int delta) { g_loopsInUse = std::max(0, g_loopsInUse + delta); }

// ======================= styles =======================
std::vector<Style>& GetStyles()
{
	// Proven looped effects only (playtest 3: sandstorm-in-the-funnel, dust swirl, factory smoke and air debris don't
	// draw on moving anchors). Styles differ by SHAPE, LAYERS and PUFFS.
	// v0.6 (playtest 6): "I don't like the white smoke combined with the black smoke... more black smoke only", so no
	// layer is tinted lighter than the body any more. S+ and S are gone ("can barely see it", "not that attractive").
	static const std::vector<const char*> kCore = { "ent_amb_smoke_stack_m_mainstream", "env_smoke" };
	static const std::vector<const char*> kWall = { "ent_amb_smoke_stack_m_mainstream", "env_smoke", "ent_amb_generic_fire_smoke_plume" };
	static const std::vector<const char*> kWedgeWall = { "ent_amb_generic_fire_smoke_plume", "ent_amb_smoke_stack_m_mainstream", "env_smoke" };
	static const std::vector<const char*> kSkirt = { "bang_dirt_dry", "ent_dst_dust", "ent_dst_dirt" };
	static const std::vector<const char*> kDustWall = { "ent_dst_dust", "bang_dust" };
	// v0.6 (playtest 6: "I was thinking about the train smoke too, is a good one"): the locomotive's thick black exhaust,
	// aimed along the direction of spin (tangent streamers) so the smoke itself is thrown round the funnel. Experimental:
	// the emitter's axis is unknown, so the tilt is TornadoRedemption.ini [Visuals] StreamerPitch and the Spin check compares four.
	// v0.7, playtest 7's Spin check: the locomotive exhaust never showed ("I don't see anything", variants 1-4); the ambient
	// train smoke did and "we definitely can see some swirling smoke" (variant 5). So the streamers use that one.
	static const std::vector<const char*> kTrain = { "ent_amb_trn4_train_smoke" };
	const float kTurn = 2 * PI;
	static std::vector<Style> styles = {
		// v0.5 flagship ("V dark vortex actually looks pretty good"), now black all through: the pale bands are black
		// tangent streamers instead.
		{ "V: Dark Vortex", "v0.7 default: tall and near-black, swirling train-smoke streamers, debris spiralling up it, a wall cloud on top.",
			135.f, 5.f, 32.f, 1.8f, 150.f,
			{ { kCore, 12, 2, 0.32f, 0.55f, 2.6f, 6.8f, 0.62f },
			  { kWall, 11, 3, 1.0f, 1.0f, 2.3f, 6.0f, 0.8f },
			  { kTrain, 8, 3, 1.1f, 1.15f, 1.7f, 4.0f, 0.62f, 2 * kTurn, true } },   // v1.1: three bands (was 7 x 2)
			{ "env_sandstorm" }, 5, 14.f, 3.2f,
			{ "env_wind_debris_woodland" }, 2, 10.f, 1.5f,
			{ "ent_amb_smoke_stack_l_mainstream" }, 6, 9.f,
			{ "exp_grd_smoke_post", "ent_amb_falling_smoke" }, 3.0f, 2, 1.2f,
			kSkirt, kDustWall, 1.45f, 0.7f },
		// Playtest 6: "the wedge is probably the best looking one... if we could get that one spinning more, have it a
		// little bit more tight, like a tight little cone that comes up, it could be the best". The default now.
		{ "W: Wedge", "A wide, dark wedge pulled into a tight cone, with swirling streamers and debris spiralling up it.",
			115.f, 7.f, 40.f, 1.6f, 140.f,
			{ { kCore, 10, 2, 0.4f, 0.55f, 3.2f, 7.0f, 0.65f },
			  { kWedgeWall, 9, 3, 1.0f, 1.0f, 2.8f, 6.2f, 0.8f },
			  { kTrain, 7, 3, 1.05f, 1.1f, 1.7f, 3.7f, 0.6f, 2 * kTurn, true } },
			{ "env_sandstorm" }, 5, 16.f, 3.0f,
			{ "env_wind_debris_woodland" }, 2, 12.f, 1.5f,
			{ "ent_amb_smoke_stack_l_mainstream" }, 6, 8.f,
			{ "exp_grd_smoke_post", "ent_amb_falling_smoke" }, 3.2f, 3, 1.2f,
			kSkirt, kDustWall, 1.4f, 0.7f },
		// Playtest 6: "this one's actually pretty good... I can see some pieces rotating around it, it's quite large".
		{ "C: Dark Column", "The classic: tall, dark and steady.",
			125.f, 6.f, 28.f, 1.6f, 150.f,
			// v1.1: a dark core inside the column (it was a single shell of 30 plumes)
			{ { kCore, 8, 2, 0.4f, 0.6f, 2.4f, 5.6f, 0.7f },
			  { kWall, 12, 3, 1.0f, 1.0f, 2.1f, 5.7f, 0.95f } },
			{ "env_sandstorm" }, 4, 12.f, 3.0f,
			{ "env_wind_debris_woodland" }, 2, 8.f, 1.5f,
			{ "ent_amb_smoke_stack_l_mainstream" }, 3, 7.f,
			{ "exp_grd_smoke_post" }, 3.0f, 4, 0.9f,
			{ "bang_dirt_dry", "ent_dst_dirt" }, { "ent_dst_dust" } },
		// Playtest 6: "the rope does not look amazing. It's just not dense enough. And it's not animated like a twister...
		// like an old-timey twister, you could see a lot of rotation on the rope because it's thinner". Denser, black, and
		// it snakes: a wave travels up the funnel.
		{ "R: Rope", "An old-timey twister: thin, dense and snaking, with black streamers winding up it.",
			120.f, 3.f, 14.f, 2.4f, 220.f,
			{ { kCore, 16, 2, 0.35f, 0.6f, 2.4f, 5.2f, 0.6f },
			  { kCore, 12, 2, 1.0f, 1.0f, 2.0f, 4.4f, 0.78f },
			  { kTrain, 8, 3, 1.1f, 1.1f, 1.5f, 3.2f, 0.6f, 3 * kTurn, true } },
			{ "env_sandstorm" }, 3, 9.f, 2.5f,
			{ "env_wind_debris_woodland" }, 2, 6.f, 1.2f,
			{ "ent_amb_smoke_stack_l_mainstream" }, 3, 6.f,
			{ "exp_grd_smoke_post", "ent_dst_dust" }, 2.4f, 2, 1.1f,
			{ "bang_dirt_dry", "ent_dst_dirt" }, { "ent_dst_dust" }, 0.8f, 0.8f, 2.5f },
		// v1.1, the user: "comic animated twister variant like those tubular moving twisters... an exaggerated one".
		// A cartoon tube: fast spin, a big S-shaped wiggle, a tip that whips round and hops, and level stripes of smoke that
		// climb the tube and wrap back to the bottom - motion you can read from any distance, no volume needed.
		{ "T: Toon Twister", "A cartoon twister: a wobbling tube with stripes climbing it, a whipping, hopping tip. Exaggerated on purpose.",
			105.f, 3.5f, 20.f, 1.15f, 290.f,
			{ { kCore, 12, 2, 0.55f, 0.6f, 2.6f, 6.0f, 0.9f },
			  { kCore, 6, 6, 1.0f, 1.7f, 2.0f, 4.6f, 0.72f, 0, false, true, 0.11f },
			  { kTrain, 6, 3, 1.15f, 1.6f, 1.5f, 3.0f, 0.7f, 2 * kTurn, true } },
			{ "env_sandstorm" }, 4, 10.f, 2.8f,
			{ "env_wind_debris_woodland" }, 2, 7.f, 1.3f,
			{ "ent_amb_smoke_stack_l_mainstream" }, 4, 7.f,
			{ "exp_grd_smoke_post", "ent_dst_dust" }, 2.6f, 3, 1.1f,
			{ "bang_dirt_dry", "ent_dst_dirt" }, { "ent_dst_dust" }, 0.9f, 0.9f, 3.4f, 6.0f, 2.6f },
		// ---- v1.2: new types (the user: "some new types, animations, methods for creating tornado visually"). Each is built a
		// different way: own colours instead of the dark tint, glow from inside, sub-funnels, a wall made of props. They use
		// effects from the game's "core" library that playtests haven't tried in a funnel yet - the gallery is where to judge them.
		{ "D: Dust Devil", "A desert dust devil: short, tan and fast, whipping about. Made for New Austin's plains.",
			55.f, 2.2f, 10.f, 1.3f, 330.f,
			{ { kCore, 10, 2, 0.45f, 0.7f, 1.8f, 3.8f, 1.0f },
			  { { "env_smoke", "ent_amb_wind_litter_dust_swirl" }, 9, 3, 1.0f, 1.2f, 1.6f, 3.6f, 1.05f },
			  { { "ent_amb_wind_litter_dust_swirl", "env_smoke" }, 6, 3, 1.1f, 1.4f, 1.4f, 3.0f, 1.1f, 2 * kTurn } },
			{ "env_sandstorm" }, 4, 7.f, 2.2f,
			{ "env_wind_debris_desert" }, 2, 5.f, 1.2f,
			{ "env_smoke" }, 2, 4.f,
			{ "ent_dst_sand", "bang_sand", "ent_dst_dust" }, 2.0f, 3, 1.3f,
			{ "bang_sand", "ent_dst_dust" }, { "ent_dst_dust", "bang_sand" }, 0.7f, 1.0f, 1.2f, 2.5f, 0.6f,
			2, 0.80f, 0.64f, 0.44f },
		{ "F: Firenado", "A fire whirl: a core of flame wrapped in black smoke, glowing from inside and throwing embers. Best at night.",
			95.f, 3.5f, 18.f, 1.5f, 240.f,
			{ { { "ent_amb_generic_fire_plume" }, 9, 2, 0.35f, 0.8f, 1.6f, 3.2f, 1.0f, 0, false, false, 0, true },
			  { kWall, 10, 3, 1.0f, 1.0f, 2.2f, 5.0f, 0.55f },
			  { { "ent_amb_generic_fire_smoke_plume" }, 6, 3, 1.1f, 1.2f, 1.6f, 3.4f, 0.5f, 2 * kTurn, true } },
			{ "env_sandstorm" }, 3, 9.f, 2.4f,
			{ "ent_amb_falling_embers" }, 2, 6.f, 1.4f,
			{ "ent_amb_smoke_stack_l_mainstream" }, 4, 7.f,
			{ "exp_grd_smoke_post", "ent_dst_embers", "ent_brk_embers" }, 2.6f, 3, 1.1f,
			kSkirt, kDustWall, 1.2f, 0.6f, 1.4f, 1.5f, 0.0f,
			0, 1, 1, 1,
			7, 1.0f, 0.42f, 0.1f, 22.f, 9.f, 0.45f },
		{ "G: Ghost Twister", "Pale and slow-turning, lit cold blue from inside, made of the ghost train's own steam. Spooky after dark.",
			120.f, 4.f, 22.f, 1.6f, 120.f,
			{ { { "ent_amb_ghost_train_steam" }, 10, 2, 0.4f, 0.6f, 2.0f, 4.6f, 1.0f, 0, false, false, 0, true },
			  { { "env_smoke", "ent_amb_smoke_stack_m_mainstream" }, 10, 3, 1.0f, 0.8f, 2.2f, 5.4f, 1.0f },
			  { kTrain, 6, 3, 1.1f, 0.9f, 1.5f, 3.2f, 1.0f, 2 * kTurn, true } },
			{ "ent_amb_river_mist_gen" }, 4, 10.f, 2.6f,
			{ "env_wind_debris_woodland" }, 2, 7.f, 1.2f,
			{ "env_fog" }, 3, 7.f,
			{ "ent_amb_falling_smoke", "exp_grd_smoke_post" }, 2.6f, 3, 0.9f,
			{ "ent_dst_dust" }, { "ent_dst_dust" }, 1.3f, 1.0f, 2.2f, 1.5f, 0.0f,
			2, 0.62f, 0.86f, 0.9f,
			6, 0.35f, 0.85f, 1.0f, 20.f, 7.f, 0.15f },
		{ "S: Snow Devil", "A blizzard twister for the mountains: white, wind-blown snow spun into a column. Try it up at Colter.",
			90.f, 3.f, 16.f, 1.5f, 260.f,
			{ { { "ent_amb_snow_mountain_drifts_swirl", "ent_amb_snow_mist_base" }, 10, 2, 0.45f, 0.7f, 2.0f, 4.4f, 1.0f, 0, false, false, 0, true },
			  { { "env_smoke", "ent_amb_snow_mist_upper" }, 10, 3, 1.0f, 1.1f, 2.0f, 4.8f, 1.0f },
			  { { "ent_amb_snow_mountain_drifts_swirl" }, 6, 3, 1.1f, 1.3f, 1.5f, 3.0f, 1.0f, 2 * kTurn, false, false, 0, true } },
			{ "ent_amb_snow_mountain_drifts" }, 4, 9.f, 2.4f,
			{ "ent_amb_snow_mist_base" }, 2, 6.f, 1.4f,
			{ "env_smoke" }, 3, 6.f,
			{ "ent_dst_snow_lrg", "bang_snow", "ent_dst_snow" }, 2.4f, 3, 1.2f,
			{ "bang_snow", "ent_dst_snow" }, { "ent_dst_snow" }, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,
			2, 0.92f, 0.95f, 1.0f },
		{ "H: Waterspout", "A tall, thin spout of mist and spray, the kind that comes off a lake. Grey-white and snaking.",
			140.f, 2.5f, 13.f, 2.0f, 210.f,
			{ { { "ent_amb_wf_mist_lg", "ent_amb_rapid_area_spray_hvy" }, 12, 2, 0.4f, 0.7f, 1.8f, 4.0f, 1.0f, 0, false, false, 0, true },
			  { { "env_smoke", "ent_amb_wf_mist_md" }, 12, 3, 1.0f, 1.0f, 2.0f, 4.6f, 1.0f },
			  { kTrain, 6, 2, 1.1f, 1.1f, 1.4f, 2.8f, 1.0f, 3 * kTurn, true } },
			{ "ent_amb_wf_close_spray" }, 5, 8.f, 2.6f,
			{ "ent_amb_rapid_area_spray" }, 2, 5.f, 1.4f,
			{ "env_rain_mist" }, 3, 6.f,
			{ "ent_dst_gen_water_spray", "liquid_splash_water", "exp_grd_smoke_post" }, 2.2f, 2, 1.1f,
			{ "liquid_splash_water", "ent_dst_gen_water_spray" }, { "ent_dst_gen_water_spray" }, 0.8f, 1.0f, 2.2f, 0.0f, 0.0f,
			2, 0.78f, 0.82f, 0.86f },
		{ "M: Multi-vortex", "Three thin funnels circling each other inside one storm, like the real big ones. Wide and dark.",
			120.f, 12.f, 36.f, 1.4f, 120.f,
			{ { kCore, 14, 2, 1.0f, 2.2f, 2.0f, 4.6f, 0.62f },
			  { kWall, 12, 3, 1.0f, 2.0f, 2.0f, 4.8f, 0.75f },
			  { kTrain, 6, 3, 1.15f, 0.6f, 1.6f, 3.6f, 0.65f, 1.5f * kTurn, true } },
			{ "env_sandstorm" }, 6, 18.f, 3.2f,
			{ "env_wind_debris_woodland" }, 2, 12.f, 1.5f,
			{ "ent_amb_smoke_stack_l_mainstream" }, 6, 9.f,
			{ "exp_grd_smoke_post", "ent_amb_falling_smoke" }, 3.0f, 3, 1.0f,
			kSkirt, kDustWall, 1.3f, 0.7f, 0.0f, 0.0f, 0.0f,
			0, 1, 1, 1,
			0, 1, 0.5f, 0.2f, 16, 6, 0,
			3, 0.6f },
		{ "J: Junknado", "A funnel made of junk: planks, barrels, crates, wheels and doors climbing round it, with only a little dust.",
			85.f, 4.f, 20.f, 1.4f, 200.f,
			{ { kCore, 6, 2, 0.4f, 0.6f, 1.6f, 3.4f, 1.0f } },
			{ "env_sandstorm" }, 3, 9.f, 2.4f,
			{ "env_wind_debris_woodland" }, 2, 7.f, 1.4f,
			{ "ent_amb_smoke_stack_l_mainstream" }, 2, 6.f,
			{ "ent_dst_dust", "bang_dust" }, 2.2f, 3, 0.8f,
			kSkirt, kDustWall, 1.0f, 1.0f, 0.8f, 1.0f, 0.0f,
			2, 0.62f, 0.57f, 0.5f,
			0, 1, 0.5f, 0.2f, 16, 6, 0,
			0, 0.6f, 110 },
		{ "E: Custom (FX Lab pick)", "Uses the effect the FX Lab's 'Use in style E' picked.",
			120.f, 6.f, 25.f, 1.5f, 170.f,
			{ { { "ent_amb_smoke_stack_m_mainstream" }, 10, 3, 1.0f, 1.0f, 1.5f, 5.0f, 1.0f } },
			{ "env_sandstorm" }, 4, 12.f, 3.0f,
			{ "env_wind_debris_woodland" }, 2, 9.f, 1.5f,
			{ "ent_amb_smoke_stack_l_mainstream" }, 3, 6.f,
			{ "exp_grd_smoke_post" }, 2.5f, 4, 1.0f,
			{ "bang_dirt_dry" }, { "ent_dst_dust" } },
	};
	return styles;
}

void SetCustomStyleFx(const char* fx, bool looped, float scale)
{
	Style& s = GetStyles().back();
	if (looped)
	{
		s.layers[0].fx = { fx };
		s.layers[0].scaleBottom = scale;
		s.layers[0].scaleTop = scale * 2.5f;
	}
	else
	{
		s.puffFx = { fx };
		s.puffScale = scale;
	}
}

float Tornado::sizeMul() const { return opts.size > 0 ? opts.size : kSizeMul[(g_set.size >= 0 && g_set.size <= 2) ? g_set.size : 1]; }
float Tornado::height() const { return GetStyles()[styleIdx].height * sqrtf(sizeMul()) * opts.heightMul; }
// Playtest 2: the grab ring was up to 5x wider than the visible funnel ("it affects things where the tornado isn't").
// The lifting wall now sits just outside the visible funnel base; Reach only controls the gentler inflow beyond it.
float Tornado::wallRadius() const
{
	if (opts.tiny) return std::max(0.45f, GetStyles()[styleIdx].rBase * sizeMul() * 1.5f + 0.35f);   // v1.4: a person-sized one
	return GetStyles()[styleIdx].rBase * sizeMul() * 1.5f + 4.0f;
}
float Tornado::eyeRadius() const { return wallRadius() * 0.45f; }
float Tornado::reachRadius() const { return wallRadius() * (opts.reach > 0 ? opts.reach : ReachMul()); }
float Tornado::radiusAt(float hFrac) const
{
	const Style& s = GetStyles()[styleIdx];
	return (s.rBase + (s.rTop - s.rBase) * powf(Clamp(hFrac, 0, 1), s.flare)) * sizeMul();
}

bool Tornado::PlayerInEye() const
{
	if (!g_set.eye) return false;
	V3 pp = ENTITY::GET_ENTITY_COORDS(PLAYER::PLAYER_PED_ID(), TRUE, FALSE);
	return (pp - base).len2d() < eyeRadius() && pp.z - base.z < height() * 0.9f;
}

Tornado::Tornado(const V3& groundPos, float headingDeg, int style, const std::string& lbl, bool stat, int loopBudget, const SpawnOpts& o)
	: base(groundPos), heading(headingDeg), label(lbl), styleIdx(style), stationary(stat), opts(o)
{
	bornAt = NowSec();
	spawnPos = groundPos;
	growSeconds = o.grow;
	static int s_uid = 0;
	uid = ++s_uid;
	BuildVisuals(loopBudget);
	BuildJunk();
	if (g_set.mapBlip && !opts.mini && !opts.display)
	{
		blip = MAP::BLIP_ADD_FOR_COORDS(H("BLIP_STYLE_DEBUG_RED"), base.x, base.y, base.z);
		if (blip)
		{
			MAP::SET_BLIP_SPRITE(blip, H("blip_rc_lightning"), TRUE);   // v1.1: the storm icon Rockstar uses (184 times)
			MAP::BLIP_ADD_MODIFIER(blip, H("BLIP_MODIFIER_URGENT"));
			MAP::SET_BLIP_SCALE(blip, 1.3f);
			MAP::SET_BLIP_NAME(blip, MISC::VAR_STRING_LITERAL("Tornado"));
		}
	}
	Log("Spawned '%s' at (%.1f, %.1f, %.1f): loops planned %d running %d refused %d (budget %d, in use %d, engine %s) | wall %.0fm eye %.0fm reach %.0fm height %.0fm speed %.1f m/s",
		label.c_str(), base.x, base.y, base.z, loopsPlanned, loopsRunning, loopsFailed, loopBudget, LoopsInUse(),
		emitters.empty() ? "none" : (emitters[0].world ? "world-space" : "anchors"), wallRadius(), eyeRadius(), reachRadius(), height(), MoveSpeed());
}

// Astra: the static tornado list is destroyed at DLL unload, off the script thread, where natives must not run.
// Cleanup is always explicit (Destroy() on the script thread); the destructor does nothing.
Tornado::~Tornado() {}

// v1.2: the smoke's colour by style - the dark tint (the default look), the effects' own colours (fire, steam, snow), or the
// style's own colour (a tan dust devil, a pale ghost). A layer marked natural keeps its own colours inside a tinted style.
static void PuffTint(const Style& s, float shade, const char* fx = nullptr)
{
	if (s.tint == 1) return;
	if (fx && strstr(fx, "ember")) return;   // (audit 3: embers glow - never darkened)
	if (s.tint == 2) { shade = Lerp(1.0f, shade, 0.3f); GRAPHICS::SET_PARTICLE_FX_NON_LOOPED_COLOUR(std::min(1.0f, s.tr * shade), std::min(1.0f, s.tg * shade), std::min(1.0f, s.tb * shade)); }
	else if (g_set.darkTint) GRAPHICS::SET_PARTICLE_FX_NON_LOOPED_COLOUR(0.32f * shade, 0.29f * shade, 0.26f * shade);
}

V3 Tornado::FunnelPoint(float hFrac, float ang, float radiusScale, float t) const
{
	float sz = sizeMul();
	float r = radiusAt(hFrac) * radiusScale;
	V3 sway(sinf(t * 0.35f + hFrac * 2.1f) * 6.0f * sz * hFrac, cosf(t * 0.27f + hFrac * 1.6f) * 6.0f * sz * hFrac, 0);
	float tinyK = opts.tiny ? 0.35f : 1.0f;   // v1.4 review: a pocket twister's wiggle, whip and hop, scaled to its 1.7 m
	const Style& st = GetStyles()[styleIdx];
	float snake = st.snake;
	if (snake > 0)   // a wave travelling up the funnel
		sway += V3(sinf(t * 1.1f - hFrac * 4.0f), cosf(t * 0.9f - hFrac * 3.3f), 0) * (snake * 4.0f * sz * hFrac * tinyK);
	float low = (1.0f - hFrac) * (1.0f - hFrac);
	if (st.whip > 0)   // v1.1: the tip whips round
		sway += V3(cosf(t * 2.3f), sinf(t * 2.3f), 0) * (st.whip * sz * low * tinyK);
	if (st.hop > 0)    // v1.1: and hops
		sway.z += fabsf(sinf(t * 2.7f)) * st.hop * sqrtf(sz) * low * low * tinyK;
	// growth < 1: the funnel hangs from the cloud and reaches down (touchdown), or pulls back up (dissipating)
	float zFrac = 1.0f - (1.0f - hFrac) * growth;
	return base + sway + lean * hFrac + V3(cosf(ang) * r, sinf(ang) * r, zFrac * height());
}

// Starts (or restarts) one looped effect: on its anchor prop, or - engine "world-space" - directly at a coordinate.
bool Tornado::StartEmitter(Emitter& e, const V3& at)
{
	GRAPHICS::USE_PARTICLE_FX_ASSET("core");
	if (e.world)
	{
		float rx = e.tangent ? g_set.streamerPitch : 0.0f, rz = e.tangent ? e.curAng * 180.0f / PI : 0.0f;
		e.fx = GRAPHICS::START_PARTICLE_FX_LOOPED_AT_COORD(e.fxName, at.x, at.y, at.z, rx, 0, rz, e.scale, FALSE, FALSE, FALSE, FALSE);
	}
	else
	{
		if (!e.anchor)
		{
			e.anchor = SpawnAnchor(at);
			if (!e.anchor)
				return false;
			ownEntities.insert(e.anchor);
			anchorSet.insert(e.anchor);
		}
		e.fx = GRAPHICS::START_PARTICLE_FX_LOOPED_ON_ENTITY(e.fxName, e.anchor, 0, 0, 0, 0, 0, 0, e.scale, FALSE, FALSE, FALSE);
	}
	if (!e.fx)
		return false;
	GRAPHICS::SET_PARTICLE_FX_LOOPED_FAR_CLIP_DIST(e.fx, 1500.0f);
	const Style& s = GetStyles()[styleIdx];
	bool natural = s.tint == 1 || (e.kind == 0 && e.layer >= 0 && e.layer < (int)s.layers.size() && s.layers[e.layer].natural)
		|| (e.fxName && strstr(e.fxName, "ember"));   // (audit 3: the Firenado's falling embers glow)
	if (natural)
		;   // fire, steam, snow: their own colours
	else if (s.tint == 2)
		GRAPHICS::SET_PARTICLE_FX_LOOPED_COLOUR(e.fx, std::min(1.0f, s.tr * e.shade), std::min(1.0f, s.tg * e.shade), std::min(1.0f, s.tb * e.shade), FALSE);
	else if (g_set.darkTint)
		GRAPHICS::SET_PARTICLE_FX_LOOPED_COLOUR(e.fx, std::min(1.0f, 0.32f * e.shade), std::min(1.0f, 0.29f * e.shade), std::min(1.0f, 0.26f * e.shade), FALSE);
	return true;
}

void Tornado::BuildVisuals(int loopBudget)
{
	if (!LoadPtfxAsset("core"))
	{
		Log("ERROR: ptfx dictionary 'core' did not load");
		return;
	}
	if (Render() == 1 && !opts.tiny)   // (v1.5 review: a pocket twister is all whirls - "Puffs only" would leave it invisible)
		return;
	const Style& s = GetStyles()[styleIdx];
	float sz = sqrtf(sizeMul());
	int subNext = 0;

	// Fit the style into the free looped-effect budget. Trim what's least visible first: low debris, then extra
	// ground/top emitters, then every funnel layer in proportion (v1.1: it used to strip one layer's rings at a time).
	std::vector<int> plan, orig;
	for (auto& L : s.layers) { plan.push_back(L.rings * L.perRing); orig.push_back(L.rings * L.perRing); }
	int ng = s.groundCount, nd = s.debrisCount, nt = s.topCount;
	// v1.4 (playtest 12: the gun's twisters "make too much dust, you can't see anything"): a pocket twister has no ground
	// storm and no smoke collar, a couple of leaf swirls, and a dozen small plumes at the most
	if (opts.tiny) { ng = 0; nt = 0; nd = 0; loopBudget = std::min(loopBudget, 10); }
	auto total = [&]() { int n = ng + nd + nt; for (int c : plan) n += c; return n; };
	int budget = std::max(0, g_set.perf == 2 ? std::min(loopBudget, 50) : g_set.perf == 1 ? std::min(loopBudget, 80) : loopBudget);
	while (total() > budget)
	{
		if (nd > 0) { nd--; continue; }
		// everything else gives up emitters in proportion: whichever still has the largest share of its own count. (The
		// ground ring and the wall cloud on top keep a few - the cloud is what the funnel "connects up into".)
		int best = -1;
		float bestShare = 0;
		for (size_t i = 0; i < plan.size(); i++)
		{
			const FunnelLayer& L = s.layers[i];
			int floor = L.twist > 0 ? L.perRing * 2 : 4;   // a band layer keeps at least two emitters per band
			float share = (float)plan[i] / std::max(1, orig[i]);
			if (plan[i] > floor && share > bestShare) { bestShare = share; best = (int)i; }
		}
		float gShare = (float)ng / std::max(1, s.groundCount), tShare = (float)nt / std::max(1, s.topCount);
		if (ng > 2 && gShare > bestShare) { ng--; continue; }
		if (nt > 3 && tShare > bestShare) { nt--; continue; }
		if (best >= 0) { plan[best]--; continue; }
		if (ng > 0) { ng--; continue; }
		if (nt > 0) { nt--; continue; }
		if (plan.size() > 1) { plan.pop_back(); continue; }
		if (!plan.empty() && plan[0] > 0) { plan[0]--; continue; }
		break;
	}
	{
		int spare = 0;
		for (size_t i = 0; i < plan.size(); i++)
		{
			const FunnelLayer& L = s.layers[i];
			if ((L.twist > 0 || L.ringBands) && L.perRing > 1) { int r = plan[i] % L.perRing; plan[i] -= r; spare += r; }
		}
		for (size_t i = 0; i < plan.size() && spare > 0; i++)
			if (!(s.layers[i].twist > 0 || s.layers[i].ringBands)) { plan[i] += spare; spare = 0; }
	}
	loopsPlanned = total();

	auto add = [&](int kind, int layer, float hFrac, float angle0, float radiusMul, float spinMul, const char* fx, float scale, float shade)
	{
		Emitter e;
		e.kind = kind; e.layer = layer; e.hFrac = hFrac; e.angle0 = angle0; e.radiusMul = radiusMul; e.spinMul = spinMul;
		e.tangent = kind == 0 && layer >= 0 && s.layers[layer].tangent; e.curAng = angle0;
		if (kind == 0 && s.vortices > 0 && s.layers[layer].twist <= 0)
			e.sub = subNext++ % s.vortices;   // v1.2: dealt round the sub-funnels (the streamer bands stay on the parent)
		e.fxName = fx; e.scale = scale * (opts.tiny ? kTinyFxScale : sz); e.shade = shade;
		// (v1.6, playtest 14: "my little cute pocket tornados are gone" - v1.5 drew them with the game's ambient whirls, which
		// barely showed. Its own little funnel is back; the smoke that built into a cloud was the puffs, and those stay off)
		e.world = g_set.engine == 1;
		if (StartEmitter(e, base + V3(0, 0, 1 + hFrac * 50)))
		{
			e.wanted = true;
			loopsRunning++;
			AddLoopsInUse(1);
		}
		else
		{
			loopsFailed++;
			if (!e.world && !e.anchor)
				return;   // the anchor itself failed: nothing to keep
			// v1.0, playtest 8: a tornado that replaced another got 4 of its 85 effects - the game hadn't freed the old
			// tornado's smoke yet that frame - and refused ones were never tried again. Now they are, as room frees up.
			e.pending = true;
			e.retryAt = NowSec() + 0.5f;
		}
		emitters.push_back(e);
	};

	// v1.1, playtests 9 and 10 ("leaving lots of gaps in the tornado's form"; the frames showed the tip splitting into two
	// prongs, the column as a stack of lumps 13.5 m apart and the Rope as separate strands). Body layers are no longer rings
	// of 2-3 emitters at the same height: every emitter gets its OWN height, and each one sits a golden angle (137.5 deg)
	// round from the one below, so the same number of plumes covers twice the heights with no two lining up into a strand.
	// The wall is given some thickness (each plume a little in or out), and the core's lowest plumes pull in to a single tip.
	const float kGolden = 2.39996323f;
	static const float kThick[4] = { 1.0f, 0.86f, 1.07f, 0.93f };
	for (size_t li = 0; li < plan.size(); li++)
	{
		const FunnelLayer& L = s.layers[li];
		int n = plan[li];
		if (n <= 0)
			continue;
		if (L.twist > 0)
		{
			// streamer bands: arms x heights, each band a line winding up the funnel (the spin you can watch).
			// v1.1: three bands where the style allows, so the train smoke's chuffs overlap into a line.
			int arms = std::max(1, L.perRing), per = std::max(1, n / arms);
			for (int a = 0; a < arms; a++)
				for (int i = 0; i < per; i++)
				{
					float hFrac = per > 1 ? 0.06f + 0.9f * (float)i / (per - 1) : 0.4f;
					add(0, (int)li, hFrac, a * 2 * PI / arms + hFrac * L.twist + li * 0.9f, L.radiusMul, L.spinMul,
						L.fx[(a + i) % L.fx.size()], Lerp(L.scaleBottom, L.scaleTop, hFrac), L.shade);
				}
			continue;
		}
		if (L.ringBands)
		{
			// v1.1 Toon: level rings, evenly spaced, that climb the tube (UpdateVisuals scrolls them)
			int per = std::max(1, L.perRing), nr = std::max(1, n / per);
			for (int i = 0; i < nr; i++)
				for (int j = 0; j < per; j++)
				{
					float hFrac = (float)i / nr;
					add(0, (int)li, hFrac, j * 2 * PI / per + i * 0.5f, L.radiusMul, L.spinMul, L.fx[(i + j) % L.fx.size()], Lerp(L.scaleBottom, L.scaleTop, hFrac), L.shade);
				}
			continue;
		}
		bool core = L.radiusMul < 0.6f;
		for (int k = 0; k < n; k++)
		{
			float u = n > 1 ? (float)k / (n - 1) : 0.0f;
			float hFrac = powf(u, 1.12f);          // a touch denser low down, where you look
			float rMul = L.radiusMul * kThick[k % 4];
			if (core && hFrac < 0.14f)
				rMul *= Lerp(0.35f, 1.0f, hFrac / 0.14f);   // one tip, not two prongs
			add(0, (int)li, hFrac, k * kGolden + li * 0.9f, rMul, L.spinMul * Lerp(1.4f, 0.7f, hFrac),
				L.fx[k % L.fx.size()], Lerp(L.scaleBottom, L.scaleTop, hFrac) * (core ? 1.0f : 1.08f), L.shade);
		}
	}
	for (int j = 0; j < ng; j++)
		add(1, -1, 0.f, j * 2 * PI / ng, s.groundRadius, 1.6f, s.groundFx[j % s.groundFx.size()], s.groundScale, 1.1f);
	for (int j = 0; j < nd; j++)
		add(2, -1, 0.03f, j * 2 * PI / nd, s.debrisRadius, 2.0f, s.debrisFx[j % s.debrisFx.size()], s.debrisScale, 1.0f);
	for (int j = 0; j < nt; j++)
		add(3, -1, 0.97f, j * 2 * PI / nt, s.collarMul, 0.4f, s.topFx[j % s.topFx.size()], s.topScale, s.collarShade);
}

void Tornado::UpdateVisuals(float t)
{
	const Style& s = GetStyles()[styleIdx];
	float sz = sizeMul();
	float spin = s.spinDeg * PI / 180.0f;
	for (auto& e : emitters)
	{
		bool live = e.world ? (e.fx && GRAPHICS::DOES_PARTICLE_FX_LOOPED_EXIST(e.fx)) : (e.anchor && ENTITY::DOES_ENTITY_EXIST(e.anchor));
		if (!live)
			continue;
		float ang = e.angle0 + spin * e.spinMul * t;
		e.curAng = ang;
		V3 p;
		float hf = e.hFrac;
		if (e.kind == 0 && e.layer >= 0 && e.layer < (int)s.layers.size() && s.layers[e.layer].scroll > 0)
			hf = fmodf(e.hFrac + s.layers[e.layer].scroll * t, 1.0f);   // v1.1 Toon: the stripes climb and wrap
		switch (e.kind)
		{
		case 0:
			if (e.sub >= 0 && s.vortices > 0)
			{
				// v1.2 multi-vortex: each emitter belongs to a thin sub-funnel; the sub-funnels circle inside the parent
				float subAng = spin * 0.55f * t + e.sub * 2 * PI / s.vortices;
				V3 c = FunnelPoint(hf, subAng, s.vortexOrbit, t);
				float r = radiusAt(hf) * 0.3f * e.radiusMul;
				p = c + V3(cosf(ang * 1.6f) * r, sinf(ang * 1.6f) * r, 0);
			}
			else
				p = FunnelPoint(hf, ang, e.radiusMul * (s.layers[e.layer].ringBands ? (0.9f + 0.25f * hf) : 1.0f), t);
			break;
		case 1: p = base + V3(cosf(ang) * e.radiusMul * sz, sinf(ang) * e.radiusMul * sz, 0.5f + (1.0f - growth) * height()); break;
		case 2: p = base + V3(cosf(ang) * e.radiusMul * sz, sinf(ang) * e.radiusMul * sz, (opts.tiny ? 0.1f : 1.0f) * (3.0f + 3.0f * sinf(t * 1.3f + e.angle0)) + (1.0f - growth) * height()); break;
		default: p = FunnelPoint(e.hFrac, ang, e.radiusMul, t); break;
		}
		if (e.world)
			GRAPHICS::SET_PARTICLE_FX_LOOPED_OFFSETS(e.fx, p.x, p.y, p.z, e.tangent ? g_set.streamerPitch : 0.0f, 0, e.tangent ? ang * 180.0f / PI : 0.0f);
		else
			ENTITY::SET_ENTITY_COORDS_NO_OFFSET(e.anchor, p.x, p.y, p.z, FALSE, FALSE, FALSE);
	}
	// Map marker follows the switch live (Astra: it used to only apply at spawn).
	if (g_set.mapBlip && !blip && !dissipating && !opts.mini && !opts.display)
	{
		blip = MAP::BLIP_ADD_FOR_COORDS(H("BLIP_STYLE_DEBUG_RED"), base.x, base.y, base.z);
		if (blip)
		{
			MAP::SET_BLIP_SPRITE(blip, H("blip_rc_lightning"), TRUE);   // v1.1: the storm icon Rockstar uses (184 times)
			MAP::BLIP_ADD_MODIFIER(blip, H("BLIP_MODIFIER_URGENT"));
			MAP::SET_BLIP_SCALE(blip, 1.3f);
			MAP::SET_BLIP_NAME(blip, MISC::VAR_STRING_LITERAL("Tornado"));
		}
	}
	else if ((!g_set.mapBlip || dissipating) && blip)
	{
		if (MAP::DOES_BLIP_EXIST(blip)) MAP::REMOVE_BLIP(&blip);
		blip = 0;
	}
	if (blip)
		MAP::SET_BLIP_COORDS(blip, base.x, base.y, base.z);
}

// ======================= puffs =======================
// One-shot bursts in world space. They don't use the looped budget and they are the one thing proven to render at
// any distance ("I like these puffing effects"), so v0.4 leans on them for the rotating-cone look ("if only we could
// get it all rotating in a nice, neat tornado cone-like fashion"): 3-4 tight helix bands that climb and turn with the
// funnel, a dust wall at the lifting radius, and the touchdown skirt. The game can refuse one-shots when too many are
// alive, so the rate adapts to what it accepts.
static int g_puffOk = 0, g_puffFail = 0, g_puffRefusedTotal = 0, g_puffOkTotal = 0;
static float g_puffScale = 1.0f, g_nextPuffAdapt = 0;
float PuffRateScale() { return g_puffScale; }
int PuffsRefused() { return g_puffRefusedTotal; }

void AdaptPuffRate(float t)
{
	if (t < g_nextPuffAdapt)
		return;
	g_nextPuffAdapt = t + 1.0f;
	int n = g_puffOk + g_puffFail;
	g_puffOkTotal += g_puffOk;
	// The return value is unverified in RDR2: if the game has never said "started", the flag means nothing - don't
	// throttle the one effect type that is proven to render.
	if (g_puffOkTotal == 0)
	{
		g_puffOk = g_puffFail = 0;
		return;
	}
	if (n >= 6 && g_puffFail > n * 0.15f)
	{
		float old = g_puffScale;
		g_puffScale = std::max(0.25f, g_puffScale * 0.85f);
		if (old - g_puffScale > 0.01f)
			Log("puffs: the game refused %d of %d one-shot effects last second - rate x%.2f", g_puffFail, n, g_puffScale);
	}
	else if (g_puffFail == 0)
		g_puffScale = std::min(1.0f, g_puffScale * 1.04f);
	g_puffOk = g_puffFail = 0;
}

void Tornado::UpdatePuffs(float dt, float t)
{
	if (Render() == 0 || opts.tiny)   // (v1.5: a pocket twister is all whirls - no smoke puffs)
		return;
	const Style& s = GetStyles()[styleIdx];
	if (s.puffFx.empty())
		return;
	float sz = sizeMul();
	float spin = s.spinDeg * PI / 180.0f;
	float wall = wallRadius();
	int arms = std::max(1, s.helixArms);
	// v1.1 audit 2: a mini twister (size 0.32) isn't raised to 0.7 - it was emitting ~70% of a full tornado's puffs
	float rateSz = opts.mini ? sz : Clamp(sz, 0.7f, 1.4f);
	puffAcc += g_set.puffsPerSecond * PerfPuffMul() * s.puffDensity * g_puffScale * dt * rateSz * (dissipating ? growth : 0.3f + 0.7f * growth);
	puffAcc = std::min(puffAcc, 12.0f);   // audit fix: a long frame hitch no longer queues hundreds of puffs
	int n = 0;
	while (puffAcc >= 1.0f && n < 12)
	{
		puffAcc -= 1.0f;
		n++;
		float roll = Rand01();
		if (opts.tiny) roll *= 0.62f;   // v1.4: a pocket twister puffs only on its own funnel - no skirt, dust wall or debris ring
		V3 p;
		const char* fx;
		float scale;
		float shade = 1.0f;
		if (roll >= 0.30f && roll < 0.62f && growth > 0.5f)
		{
			// v1.1 fill (playtests 9/10: sky showing between the plumes): dark puffs anywhere on the funnel's surface, more of
			// them where it's wider, so the gaps between the looped plumes keep getting plugged.
			float hFrac = Clamp(powf(Rand01(), 0.7f), 0.0f, 0.92f);
			p = FunnelPoint(hFrac, RandRange(0, 2 * PI), RandRange(0.78f, 1.02f), t);
			fx = s.puffFx[rand() % s.puffFx.size()];
			scale = s.puffScale * Lerp(1.0f, 2.4f, hFrac) * sqrtf(sz);
			shade = 0.55f;
		}
		else if (roll < 0.62f)   // (fill falls back to helix bands while the funnel is still reaching down)
		{
			// Helix bands: each arm is a line that winds up the cone and turns with the spin. v0.5: the bands match the
			// looped helix (same twist, turning as one piece) and are darker, so the corkscrew reads against the sky.
			int arm = rand() % arms;
			float hFrac = powf(Rand01(), 1.3f);
			// With a banded style the dark puff bands sit exactly BETWEEN the pale looped bands and turn with them:
			// alternating dark and pale stripes, like a barber's pole, which is what makes a rotation readable.
			float ang;
			int bl = -1;
			for (size_t li = 0; li < s.layers.size(); li++) if (s.layers[li].twist > 0) bl = (int)li;
			if (bl >= 0)
			{
				const FunnelLayer& L = s.layers[bl];
				ang = arm * 2 * PI / arms + PI / arms + hFrac * L.twist + bl * 0.9f + spin * L.spinMul * t + RandRange(-0.1f, 0.1f);
			}
			else
				ang = arm * 2 * PI / arms + spin * t * Lerp(1.3f, 0.75f, hFrac) + hFrac * 3.0f + RandRange(-0.12f, 0.12f);
			p = FunnelPoint(hFrac, ang, RandRange(0.92f, 1.05f), t);
			fx = s.puffFx[rand() % s.puffFx.size()];
			scale = s.puffScale * Lerp(0.85f, 2.1f, hFrac) * sqrtf(sz);
			shade = 0.6f;   // v0.7: darker (playtest 7: "white cotton ball" puffs)
		}
		else if (roll < 0.70f && TouchedDown() && !dissipating)
		{
			// v0.5, playtest 5 ("you can't really see it picking stuff up this far away in the distance"): a debris
			// ring - dark dirt and splinter bursts circling 15-80 m up just outside the funnel, sized to read from far off.
			// v0.6: no wood splinters ("the wood splintering, cracking open is kind of artificial")
			static const char* kDebrisFx[] = { "ent_brk_dirt", "bang_dirt_dry" };
			float hm = RandRange(15.0f, 80.0f) * sqrtf(sz);
			float hFrac = Clamp(hm / height(), 0.0f, 0.8f);
			float ang = RandRange(0, 2 * PI) + spin * t;
			p = FunnelPoint(hFrac, ang, RandRange(1.05f, 1.35f), t);
			fx = kDebrisFx[rand() % 2];
			scale = RandRange(2.4f, 3.6f) * sqrtf(sz);
			shade = 0.6f;
		}
		else if (roll < 0.86f && !s.wallFx.empty() && growth > 0.8f)
		{
			float ang = RandRange(0, 2 * PI);
			float r = wall * RandRange(0.9f, 1.15f);
			p = base + V3(cosf(ang) * r, sinf(ang) * r, RandRange(0.3f, 18.0f));
			fx = s.wallFx[rand() % s.wallFx.size()];
			scale = 2.2f * sqrtf(sz);
			shade = 0.7f;   // brown-grey dust, not white
		}
		else
		{
			if (dissipating) continue;   // no fresh ground dust while it is lifting away
			float ang = RandRange(0, 2 * PI);
			float r = s.rBase * sz * RandRange(1.2f, 2.6f);
			p = base + V3(cosf(ang) * r, sinf(ang) * r, 0.3f);
			fx = s.skirtFx[rand() % s.skirtFx.size()];
			scale = 2.6f * sqrtf(sz);
			shade = 0.7f;
		}
		GRAPHICS::USE_PARTICLE_FX_ASSET("core");
		PuffTint(s, shade, fx);
		if (opts.tiny) scale *= 0.45f;   // (v1.4: pocket-sized puffs)
		if (GRAPHICS::START_PARTICLE_FX_NON_LOOPED_AT_COORD(fx, p.x, p.y, p.z, 0, 0, RandRange(0, 360), scale, FALSE, FALSE, FALSE))
			g_puffOk++;
		else
		{
			g_puffFail++;
			g_puffRefusedTotal++;
		}
	}
}

// ======================= ground & movement =======================
void Tornado::UpdateGround(float dt)
{
	float z = 0;
	bool ok = MISC::GET_GROUND_Z_FOR_3D_COORD(base.x, base.y, base.z + 60.0f, &z, FALSE)
		|| MISC::GET_GROUND_Z_FOR_3D_COORD(base.x, base.y, 1500.0f, &z, FALSE);
	if (ok)
	{
		if (!groundOk)
			Log("'%s' ground found again at z=%.1f (base was %.1f)", label.c_str(), z, base.z);
		groundOk = true;
		groundLostFor = 0;
		// v0.5 (playtest 5: "ground found again at z=69.2 (base was 44.4)"): a big step is eased over ~1-2 s.
		float gap = fabsf(z - base.z);
		float rate = gap > 6.0f ? 12.0f : ((z > base.z) ? 15.0f : 30.0f);
		base.z += Clamp(z - base.z, -rate * dt, rate * dt);
		return;
	}
	groundLostFor += dt;
	if (groundLostFor > 0.5f && groundOk)
	{
		groundOk = false;
		Log("'%s' ground unknown at (%.0f, %.0f) - following player height", label.c_str(), base.x, base.y);
	}
	if (!groundOk)
	{
		V3 pp = ENTITY::GET_ENTITY_COORDS(PLAYER::PLAYER_PED_ID(), TRUE, FALSE);
		if ((pp - base).len2d() < 600.0f)
			base.z += Clamp(pp.z - base.z, -4.0f * dt, 4.0f * dt);
	}
}

void Tornado::UpdateMovement(float dt, float t)
{
	if (scripted)
	{
		// v1.1: a scene steers it (the intro drives it through the camp)
		moveVel = scriptVel;
		if (scriptVel.len2d() > 0.1f)
			heading = atan2f(-scriptVel.x, scriptVel.y) * 180.0f / PI;
	}
	else if (natural)
	{
		// v1.1 Storm season: a wild tornado wanders, but its wandering leans toward Arthur, so sooner or later it finds him.
		// Slower than a summoned one, and it doesn't linger on him.
		V3 pp = ENTITY::GET_ENTITY_COORDS(PLAYER::PLAYER_PED_ID(), TRUE, FALSE);
		V3 d = pp - base;
		float want = atan2f(-d.x, d.y) * 180.0f / PI;
		float diff = fmodf(want - heading + 540.0f, 360.0f) - 180.0f;
		wanderTurn = Clamp(wanderTurn + RandRange(-30.f, 30.f) * dt, -14.f, 14.f);
		heading += wanderTurn * dt + Clamp(diff, -6.f * dt, 6.f * dt);
		moveVel = HeadingDir(heading) * (MoveSpeed() * 0.75f);
	}
	else if (opts.mini)
	{
		// a mini twister wanders about where it landed (v1.3: livelier, and it turns back before it strays far)
		wanderTurn = Clamp(wanderTurn + RandRange(-60.f, 60.f) * dt, -45.f, 45.f);
		heading += wanderTurn * dt;
		V3 home = spawnPos - base; home.z = 0;
		if (home.len2d() > (opts.tiny ? 10.0f : 22.0f))
		{
			float want = atan2f(-home.x, home.y) * 180.0f / PI;
			heading += Clamp(fmodf(want - heading + 540.0f, 360.0f) - 180.0f, -70.f * dt, 70.f * dt);
		}
		moveVel = HeadingDir(heading) * (opts.tiny ? 2.2f : 3.4f);   // (v1.4: a pocket twister scurries)
	}
	else if (stationary || g_set.movement == 0)
	{
		moveVel = V3();
	}
	else
	{
		float speed = MoveSpeed();
		if (g_set.movement == 1)
		{
			wanderTurn = Clamp(wanderTurn + RandRange(-25.f, 25.f) * dt, -15.f, 15.f);
			heading += wanderTurn * dt;
			speed *= 0.8f;
		}
		else if (g_set.movement == 2)
		{
			V3 pp = ENTITY::GET_ENTITY_COORDS(PLAYER::PLAYER_PED_ID(), TRUE, FALSE);
			V3 d = pp - base;
			float want = atan2f(-d.x, d.y) * 180.0f / PI;
			float diff = fmodf(want - heading + 540.0f, 360.0f) - 180.0f;
			heading += Clamp(diff, -20.f * dt, 20.f * dt);
			// Linger once it has you (playtest 4: "it keeps you in the tornado for a while... about the right time").
			if (d.len2d() < wallRadius() * 1.5f)
				speed *= 0.55f;
		}
		else
		{
			speed *= 1.2f;
		}
		moveVel = HeadingDir(heading) * speed;
	}
	heading = fmodf(heading, 360.0f);
	if (heading < 0) heading += 360.0f;   // v1.1: wandering used to drift it without limit (the wind direction went negative)
	base.x += moveVel.x * dt;
	base.y += moveVel.y * dt;
	UpdateGround(dt);
	V3 wantLean = moveVel * (opts.tiny ? -0.15f : -1.8f);   // (v1.4 review: 4 m of lean laid a 1.7 m pocket twister flat)
	lean = lean + (wantLean - lean) * Clamp(dt * 0.5f, 0, 1);
	(void)t;
}

// ======================= targets =======================
static std::vector<Flight> g_flights;
static bool InFlight(Entity e)
{
	for (auto& f : g_flights)
		if (f.e == e) return true;
	return false;
}
int FlightsActive() { return (int)g_flights.size(); }

// Playtest 2: pool reads came back empty for 15-20 s at a time in Rhodes, so the tornado grabbed in bursts.
// Entities are remembered for a few seconds after they were last seen, and every filter stage is logged.
void Tornado::GatherTargets(float t)
{
	RefreshPool(t);
	rawPeds = g_rawPeds; rawVehs = g_rawVehs; rawObjs = g_rawObjs;
	float rReach = reachRadius();
	float trackRadius = rReach * 1.2f;
	statPool = (int)g_pool.size();
	statInRange = statHeightRej = statCapped = 0;
	for (const PoolEntry& pe : g_pool)
	{
		if (pe.isPlayer && (!g_set.grabPlayer || g_shieldPlayer))
			continue;
		if ((pe.pos - base).len2d() <= trackRadius)
			tracked[pe.e] = { t, pe.type };
	}
	for (Entity e : g_priority)
		if (ENTITY::DOES_ENTITY_EXIST(e) && (V3(ENTITY::GET_ENTITY_COORDS(e, FALSE, FALSE)) - base).len2d() <= trackRadius)
			tracked[e] = { t, 2 };
	// Settings can change between reads (e.g. "Grab Arthur" switched off) - drop the player if so.
	if (!g_set.grabPlayer || g_shieldPlayer)
	{
		Ped player = PLAYER::PLAYER_PED_ID();
		tracked.erase(player);
		if (PED::IS_PED_ON_MOUNT(player)) tracked.erase(PED::GET_MOUNT(player));
	}

	struct Found { float d; Entity e; int type; bool prio; };
	std::vector<Found> found;
	float H = height();
	for (auto it = tracked.begin(); it != tracked.end();)
	{
		Entity e = it->first;
		// v0.5: while the game's lists read empty, nothing is "seen" - keep what still exists (up to 30 s) instead of
		// dropping it after 3 s, so the things already circling don't fall out of the grip.
		float memory = PoolStale() ? 30.0f : 3.0f;
		if (t - it->second.lastSeen > memory || !ENTITY::DOES_ENTITY_EXIST(e) || InFlight(e))
		{
			it = tracked.erase(it);
			continue;
		}
		V3 rel = V3(ENTITY::GET_ENTITY_COORDS(e, FALSE, FALSE)) - base;
		float d = rel.len2d();
		if (d <= rReach)
		{
			statInRange++;
			if (rel.z > -30.0f && rel.z < H + 40.0f)
				found.push_back({ d, e, it->second.type, g_priority.count(e) != 0 });
			else
				statHeightRej++;
		}
		++it;
	}
	// Priority trees first, then people/animals and wagons, then props; nearest first within each group.
	std::sort(found.begin(), found.end(), [](const Found& a, const Found& b)
	{
		if (a.prio != b.prio) return a.prio;
		bool ap = a.type < 2, bp = b.type < 2;
		if (ap != bp) return ap;
		return a.d < b.d;
	});
	targets.clear();
	counts = TargetCounts();
	int cap = PerfMaxEntities();
	statCapped = std::max(0, (int)found.size() - cap);
	float wall = wallRadius(), eye = eyeRadius();
	for (size_t i = 0; i < found.size() && (int)i < cap; i++)
	{
		targets.push_back(found[i].e);
		if (found[i].type == 0) counts.peds++;
		else if (found[i].type == 1) counts.vehicles++;
		else counts.objects++;
		if (found[i].d < eye) counts.inEye++;
		else if (found[i].d < wall * 1.6f) counts.inWall++;
	}
}

// ======================= scripted uproot / carry =======================
static void Burst(const char* fx, const V3& p, float scale)
{
	GRAPHICS::USE_PARTICLE_FX_ASSET("core");
	GRAPHICS::START_PARTICLE_FX_NON_LOOPED_AT_COORD(fx, p.x, p.y, p.z, 0, 0, RandRange(0, 360), scale, FALSE, FALSE, FALSE);
}

int MaxFlights() { return g_set.perf == 2 ? 10 : g_set.perf == 1 ? 16 : 24; }

// Takes an entity out of the game's physics and flies it ourselves: trees (planted, uprooted with dirt) and, since
// v0.6, any prop whose physics won't wake up (it's carried up the wall, thrown and laid down on the ground).
void Tornado::Lift(Entity e, float t, bool tree)
{
	V3 p = ENTITY::GET_ENTITY_COORDS(e, FALSE, FALSE);
	Vector3 mn = {}, mx = {};
	MISC::GET_MODEL_DIMENSIONS(ENTITY::GET_ENTITY_MODEL(e), &mn, &mx);
	Flight f;
	f.e = e; f.owner = this; f.t0 = f.phaseT = t; f.pos = p; f.tree = tree;
	f.rot = ENTITY::GET_ENTITY_ROTATION(e, 2);
	const ForceProfile& fp = CurrentForce();
	auto sgn = []() { return Rand01() < 0.5f ? -1.0f : 1.0f; };
	// "Frantic but not super crazy": a few turns a second, scaled by Strength.
	f.rotRate = V3(sgn() * RandRange(70.f, 150.f), sgn() * RandRange(40.f, 100.f), sgn() * RandRange(120.f, 240.f)) * fp.tumble;
	f.release = RandRange(0.45f, 0.7f);
	f.lieHeight = tree ? Clamp(std::min(mx.x - mn.x, mx.y - mn.y) * 0.18f, 0.4f, 2.5f) : 0.3f;
	ENTITY::FREEZE_ENTITY_POSITION(e, TRUE);   // we move it ourselves until it is thrown
	g_priority.erase(e);
	g_flights.push_back(f);
	if (tree)
	{
		Burst("ent_brk_dirt", p + V3(0, 0, 0.5f), 3.5f);
		Burst("bang_dirt_dry", p + V3(0, 0, 0.3f), 3.0f);
		Burst("ent_col_gen_tree_dust", p + V3(0, 0, (mx.z - mn.z) * 0.5f), 3.0f);
	}
	else
		Burst("ent_brk_dust", p, 1.6f);
}

void Tornado::Uproot(Entity e, float t)
{
	Lift(e, t, true);
	uprooted++;
	Vector3 mn = {}, mx = {};
	MISC::GET_MODEL_DIMENSIONS(ENTITY::GET_ENTITY_MODEL(e), &mn, &mx);
	Log("'%s' UPROOTED entity %d (model 0x%08X, %.0f m tall) - total %d", label.c_str(), e, ENTITY::GET_ENTITY_MODEL(e), mx.z - mn.z, uprooted);
}

void Tornado::Carry(Entity e, float t)
{
	Lift(e, t, false);
	carried++;
	if (carried <= 5 || carried % 20 == 0)
		Log("'%s' CARRIED a prop whose physics wouldn't wake (entity %d, model 0x%08X) - total %d", label.c_str(), e, ENTITY::GET_ENTITY_MODEL(e), carried);
}

// Thrown: try the game's physics first (a real tumble with collisions). Many models have no live physics, so if it
// hasn't moved shortly after, we fly it ourselves (phase 2) and lay it down where it lands.
static void Throw(Flight& f, float t, const V3& vel)
{
	f.vel = vel;
	f.phaseT = t;
	f.phase = 3;
	ENTITY::FREEZE_ENTITY_POSITION(f.e, FALSE);
	ENTITY::SET_ENTITY_DYNAMIC(f.e, TRUE);
	ENTITY::SET_ENTITY_HAS_GRAVITY(f.e, TRUE);
	PHYSICS::ACTIVATE_PHYSICS(f.e);
	ENTITY::SET_ENTITY_VELOCITY(f.e, vel.x, vel.y, vel.z);
	f.physCheckFrom = ENTITY::GET_ENTITY_COORDS(f.e, FALSE, FALSE);
}

// v0.6: a prop left hanging in the air after the tornado carried it is brought down the same way - it falls under our
// own gravity and is laid on the ground (playtest 6: "if we could touch them and then have them actually fall to the
// ground at least"). Never an instant jump: it moves a few centimetres a frame like anything falling.
static int g_glided = 0;
int HoverGlided() { return g_glided; }
static bool StartGlide(Entity e, float t)
{
	if (InFlight(e) || FlightsActive() >= MaxFlights())
		return false;
	Flight f;
	f.e = e; f.owner = nullptr; f.t0 = f.phaseT = t; f.tree = false; f.phase = 2;
	f.pos = ENTITY::GET_ENTITY_COORDS(e, FALSE, FALSE);
	f.rot = ENTITY::GET_ENTITY_ROTATION(e, 2);
	f.rotRate = V3(RandRange(-40.f, 40.f), RandRange(-40.f, 40.f), RandRange(-60.f, 60.f));
	f.vel = V3(0, 0, -1.0f);
	f.lieHeight = 0.3f;
	ENTITY::FREEZE_ENTITY_POSITION(e, TRUE);
	g_flights.push_back(f);
	g_glided++;
	return true;
}

// A tornado that is removed mid-flight throws what it is carrying instead of leaving it hanging in the sky.
static void ReleaseFlights(Tornado* owner, float t)
{
	for (auto& f : g_flights)
	{
		if (f.owner != owner)
			continue;
		f.owner = nullptr;
		if (f.phase <= 1 && ENTITY::DOES_ENTITY_EXIST(f.e))
		{
			V3 rel = f.pos - owner->base;
			float d = std::max(rel.len2d(), 1.0f);
			Throw(f, t, V3(rel.x / d * 12.0f, rel.y / d * 12.0f, 3.0f));
		}
	}
}

void UpdateFlights(float dt, float t)
{
	const ForceProfile& fp = CurrentForce();
	for (size_t i = 0; i < g_flights.size();)
	{
		Flight& f = g_flights[i];
		if (!ENTITY::DOES_ENTITY_EXIST(f.e))
		{
			g_flights.erase(g_flights.begin() + i);
			continue;
		}
		float age = t - f.phaseT;
		Tornado* o = f.owner;
		V3 target = f.pos;
		if (f.phase == 0)
		{
			// Tearing out: it heaves up and leans for a moment while the dirt bursts.
			if (!o) { Throw(f, t, V3(0, 0, 6.0f)); i++; continue; }
			float k = Clamp(age / 0.7f, 0.0f, 1.0f);
			target = f.pos + V3(RandRange(-0.08f, 0.08f), RandRange(-0.08f, 0.08f), 1.8f * dt / 0.7f);
			f.rot.x += (f.tree ? 30.0f : 60.0f) * dt / 0.7f;
			if (k >= 1.0f)
			{
				V3 rel = target - o->base;
				f.angle = atan2f(rel.y, rel.x);
				f.radius = std::max(rel.len2d(), 2.0f);
				f.height = rel.z;
				f.phase = 1;
				f.phaseT = t;
			}
		}
		else if (f.phase == 1)
		{
			// Riding the wall: spiral in to the funnel's cone, climb, tumble.
			if (!o || o->Dissipating()) { if (o) f.owner = nullptr; Throw(f, t, V3(0, 0, 4.0f)); i++; continue; }
			float want = std::max(o->wallRadius() * 0.8f, o->radiusAt(Clamp(f.height / o->height(), 0.0f, 1.0f)) * 0.95f + 2.0f);
			f.radius += Clamp(want - f.radius, -6.0f * dt, 6.0f * dt);
			float vt = fp.vmax * 0.7f;
			f.angle += vt / std::max(f.radius, 3.0f) * dt;
			f.height += fp.lift * 1.2f * dt;
			target = o->base + V3(cosf(f.angle) * f.radius, sinf(f.angle) * f.radius, f.height);
			f.rot = f.rot + f.rotRate * dt;
			if (f.height >= f.release * o->height())
			{
				V3 out(cosf(f.angle), sinf(f.angle), 0), tang(-sinf(f.angle), cosf(f.angle), 0);
				V3 v = out * fp.ejectSpeed + tang * (vt * 0.8f) + V3(0, 0, 4.0f) + o->MoveVel();
				f.pos = target;
				ENTITY::SET_ENTITY_COORDS_NO_OFFSET(f.e, target.x, target.y, target.z, FALSE, FALSE, FALSE);
				Throw(f, t, v);
				if (f.tree) Log("uprooted entity %d thrown at %.0f m up", f.e, f.height);
				i++;
				continue;
			}
		}
		else if (f.phase == 3)
		{
			// Handed to the game's physics: did it actually move?
			if (age >= 0.4f)
			{
				V3 now = ENTITY::GET_ENTITY_COORDS(f.e, FALSE, FALSE);
				float expected = f.vel.len() * age;
				if ((now - f.physCheckFrom).len() < expected * 0.3f)
				{
					ENTITY::FREEZE_ENTITY_POSITION(f.e, TRUE);
					f.pos = now;
					f.phase = 2;
					f.phaseT = t;
					if (f.tree) Log("uprooted entity %d: the game's physics didn't take it - flying it ourselves", f.e);
				}
				else
				{
					NoteTouched(f.e, t);   // the hover sweeper watches it from here
					NoteFlyer(f.e, 3, t);
					g_flights.erase(g_flights.begin() + i);
					continue;
				}
			}
			i++;
			continue;
		}
		else if (f.phase == 2)
		{
			// Our own arc: gravity, a little drag, a slower tumble, then it lands - a tree on its side, a prop upright.
			f.vel.z -= 9.8f * dt;
			f.vel = f.vel * (1.0f - 0.12f * dt);
			target = f.pos + f.vel * dt;
			f.rot = f.rot + f.rotRate * (0.6f * dt);
			float gz = GroundZ(target.x, target.y, target.z + 5.0f, -10000.0f);
			if (gz > -9000.0f && target.z <= gz + f.lieHeight)
			{
				target.z = gz + f.lieHeight;
				f.rot = f.tree ? V3(90.0f, 0.0f, f.rot.z) : V3(0.0f, 0.0f, f.rot.z);
				ENTITY::SET_ENTITY_ROTATION(f.e, f.rot.x, f.rot.y, f.rot.z, 2, TRUE);
				ENTITY::SET_ENTITY_COORDS_NO_OFFSET(f.e, target.x, target.y, target.z, FALSE, FALSE, FALSE);
				Burst("bang_dirt_dry", target, f.tree ? 3.0f : 1.4f);
				if (f.tree)
				{
					Burst("ent_dst_dust", target, 3.0f);
					Log("uprooted entity %d landed", f.e);
				}
				else
				{
					OBJECT::PLACE_OBJECT_ON_GROUND_PROPERLY(f.e, FALSE);
					ENTITY::FREEZE_ENTITY_POSITION(f.e, FALSE);
				}
				g_flights.erase(g_flights.begin() + i);
				continue;
			}
		}
		ENTITY::SET_ENTITY_ROTATION(f.e, f.rot.x, f.rot.y, f.rot.z, 2, TRUE);
		ENTITY::SET_ENTITY_COORDS_NO_OFFSET(f.e, target.x, target.y, target.z, FALSE, FALSE, FALSE);
		f.pos = target;
		i++;
	}
}

// ======================= vortex debris (v0.6) =======================
// Playtest 6: "you want to see much more stuff moving around that circle. That creates the cone. That's how a real cone
// is created in real life" / "it's very hard to tell which way it's rotating... if you can make a spiral on purpose".
// Smoke particles don't move once they're emitted, so smoke alone can't show the spin. These are real objects - planks,
// barrels, crates, wheels, doors, hay, the odd whole log - that the tornado carries round its own cone in three spiral
// arms, fast near the ground and slower as they climb (so the arms wind into a spiral), tumbling as they go. Most drop
// back into the base at the top; now and then one is flung out with real physics. They're the mod's own props:
// deleted on despawn, thrown out as wreckage when it dies down.
static const char* kOrbitModels[] = {
	"p_woodplank01x", "p_woodplank02x", "p_woodplank03x", "p_barrel02x", "p_crate01x", "p_crate03x",
	"p_haybale01x", "p_wagonwheel01", "p_door01x", "p_barrelhalf01x", "p_chair02x", "p_bucket01x",
};
static const char* kOrbitBig[] = { "p_tree_fallen_pineprop", "p_cs_treefallen01x" };
static std::unordered_set<Entity> g_orbiterSet;
bool IsOrbiter(Entity e) { return g_orbiterSet.count(e) != 0; }
// v1.1 ("swirl... not frequent enough"): a fuller debris cone - 60 / 44 / 18 (was 48 / 34 / 14)
int PerfOrbiters() { return !g_set.debrisCone ? 0 : (g_set.perf == 2 ? 18 : g_set.perf == 1 ? 44 : 60); }
int PerfMaxEntities() { return g_set.perf == 2 ? std::min(g_set.maxEntities, 70) : g_set.perf == 1 ? std::min(g_set.maxEntities, 110) : g_set.maxEntities; }
float PerfPuffMul() { return g_set.perf == 2 ? 0.5f : g_set.perf == 1 ? 0.8f : 1.0f; }

int Tornado::OrbitersAlive() const
{
	int n = 0;
	for (auto& ob : orbiters) if (ob.o && ENTITY::DOES_ENTITY_EXIST(ob.o)) n++;
	return n;
}

void Tornado::CaptureOrbiters(std::vector<Entity>& out) const
{
	for (auto& ob : orbiters) if (ob.o) out.push_back(ob.o);
}

void Tornado::SpawnOrbiter(float t)
{
	bool big = Rand01() < 0.06f;   // v0.7: fewer logs ("maybe make them a little bit smaller... spin more randomly")
	const char* name = big ? kOrbitBig[rand() % 2] : kOrbitModels[rand() % (sizeof(kOrbitModels) / sizeof(kOrbitModels[0]))];
	Hash model = H(name);
	if (!STREAMING::HAS_MODEL_LOADED(model))
	{
		STREAMING::REQUEST_MODEL(model, FALSE);   // never wait inside an update: try another model next time
		return;
	}
	Orbiter ob;
	ob.arm = rand() % 3;
	ob.h = RandRange(0.02f, 0.08f);
	ob.hTop = RandRange(0.55f, 0.92f);
	ob.climb = RandRange(0.03f, 0.07f);
	ob.rMul = RandRange(0.9f, 1.25f);
	ob.speedMul = RandRange(0.75f, 1.0f);
	ob.ang = ob.arm * 2 * PI / 3 + t * 1.5f + RandRange(-0.15f, 0.15f);
	auto sgn = []() { return Rand01() < 0.5f ? -1.0f : 1.0f; };
	ob.rot = V3(RandRange(0, 360), RandRange(0, 360), RandRange(0, 360));
	ob.rotRate = V3(sgn() * RandRange(90.f, 300.f), sgn() * RandRange(60.f, 220.f), sgn() * RandRange(90.f, 300.f)) * (big ? 0.8f : 1.0f);
	ob.big = big;
	V3 p = FunnelPoint(ob.h, ob.ang, ob.rMul, t);
	ob.o = OBJECT::CREATE_OBJECT(model, p.x, p.y, p.z, FALSE, FALSE, TRUE, FALSE, FALSE);
	if (!ob.o)
		return;
	ENTITY::SET_ENTITY_COLLISION(ob.o, FALSE, FALSE);   // it flies through things (and Arthur) while it orbits
	ENTITY::SET_ENTITY_HAS_GRAVITY(ob.o, FALSE);
	ENTITY::SET_ENTITY_INVINCIBLE(ob.o, TRUE);
	if (g_set.perf < 2)
		ENTITY::SET_ENTITY_LOD_DIST(ob.o, 1500);
	g_orbiterSet.insert(ob.o);
	ownEntities.insert(ob.o);
	orbiters.push_back(ob);
}

// v1.3 audit: whether the tornado may aim things at Arthur right now. v1.4: not while he's in the air (playtest 12 threw one at him
// as he flew up past 100 m)
bool TrueHeight(const V3& p, float* out);
static bool ArthurTargetable(Ped me)
{
	if (g_shieldPlayer || PED::IS_PED_IN_ANY_VEHICLE(me, FALSE) || PED::IS_PED_DEAD_OR_DYING(me, TRUE))
		return false;
	float h = 0;
	return TrueHeight(ENTITY::GET_ENTITY_COORDS(me, TRUE, FALSE), &h) && h < 3.0f;   // (no ground found: he's somewhere odd - no)
}

void Tornado::EjectOrbiter(size_t i, float t, const V3* target)
{
	Orbiter& ob = orbiters[i];
	if (ob.o && ENTITY::DOES_ENTITY_EXIST(ob.o))
	{
		const ForceProfile& fp = CurrentForce();
		V3 p = ENTITY::GET_ENTITY_COORDS(ob.o, FALSE, FALSE);
		V3 rel = p - base;
		float d = std::max(rel.len2d(), 1.0f);
		V3 out(rel.x / d, rel.y / d, 0), tang(-rel.y / d, rel.x / d, 0);
		V3 vel = out * fp.ejectSpeed * 0.8f + tang * (fp.vmax * 0.6f) + V3(0, 0, 3.0f);
		if (target)
		{
			// v1.3: a ballistic throw at a point - flat and fast up close, lobbed from further out
			V3 to = *target - p;
			float D = std::max(1.0f, to.len2d());
			float vh = Clamp(D / 1.5f, 14.0f, 34.0f);
			// (audit: from high up it mostly falls - the flight time is at least the drop's - so it never comes down like a bullet)
			float T = std::max(D / vh, sqrtf(2.0f * std::max(0.0f, -to.z) / 9.8f));
			vel = V3(to.x / T, to.y / T, (to.z + 0.5f * 9.8f * T * T) / T);
			float sp = vel.len();
			if (sp > 40.0f) vel = vel * (40.0f / sp);
		}
		ENTITY::SET_ENTITY_INVINCIBLE(ob.o, FALSE);
		ENTITY::SET_ENTITY_COLLISION(ob.o, TRUE, TRUE);
		// v0.7, playtest 7 ("we have trees frozen in the sky"): thrown cone props now go through the flight system - the
		// game's physics if it takes, otherwise our own arc - so they always land.
		Flight f;
		f.e = ob.o; f.owner = nullptr; f.t0 = f.phaseT = t; f.tree = false; f.pos = p; f.lieHeight = 0.3f;
		f.rot = ob.rot; f.rotRate = ob.rotRate * 0.5f;
		Throw(f, t, vel);
		g_flights.push_back(f);
		NoteFlyer(ob.o, 3, t);
		ownEntities.erase(ob.o);
		g_orbiterSet.erase(ob.o);
		Object o = ob.o;
		ENTITY::SET_ENTITY_AS_NO_LONGER_NEEDED(&o);   // wreckage: the game cleans it up when it's far away
		orbitersEjected++;
	}
	else if (ob.o)
		g_orbiterSet.erase(ob.o);
	orbiters.erase(orbiters.begin() + i);
}

void Tornado::UpdateOrbiters(float dt, float t)
{
	if (dissipating)
	{
		while (!orbiters.empty()) EjectOrbiter(orbiters.size() - 1, t);
		return;
	}
	if (!TouchedDown())
		return;
	int coneWant = opts.tiny ? 0 : opts.mini ? std::min(8, PerfOrbiters()) : PerfOrbiters();
	if (opts.display) coneWant = std::min(coneWant, 10);                  // (audit 3: a room of five ran 220 cone props)
	if (GetStyles()[styleIdx].junk > 0) coneWant /= 3;                    // the junk wall is the debris already
	if ((int)orbiters.size() < coneWant && t >= nextOrbiter)
	{
		nextOrbiter = t + 0.12f;
		SpawnOrbiter(t);
	}
	const ForceProfile& fp = CurrentForce();
	// v1.3 (playtest 11: "occasionally, rarely, throw something directly at arthur or near him"): every 7-16 s, while he's in
	// its reach but not in its hands, one piece of the cone comes straight at him (or lands just short)
	if (g_set.throwAtArthur && !opts.display && !orbiters.empty() && t >= nextAimedThrow)
	{
		Ped me = PLAYER::PLAYER_PED_ID();
		V3 pp = ENTITY::GET_ENTITY_COORDS(me, TRUE, FALSE);
		float d = (pp - base).len2d();
		bool held = t - PlayerLastInWall() < 1.0f;
		// v1.3 audit: never in the intro or the balloon (g_shieldPlayer), never at a wagon or a train, never at a dead man; and it's
		// thrown from low on the cone, so it flies at him instead of falling out of the sky
		int pick = -1;
		for (size_t k = 0; k < orbiters.size(); k++)
			if (orbiters[k].h < 0.35f && (pick < 0 || orbiters[k].h > orbiters[pick].h)) pick = (int)k;
		bool ok = ArthurTargetable(me);
		if (ok && pick >= 0 && !held && d > wallRadius() * 1.6f && d < reachRadius() * 1.6f)
		{
			nextAimedThrow = t + RandRange(7.0f, 16.0f) * (opts.mini ? 1.6f : 1.0f);
			bool direct = g_set.grabPlayer && Rand01() < 0.35f;   // (Immune Arthur only gets near misses)
			float a = RandRange(0, 2 * PI), r = direct ? RandRange(0.0f, 0.8f) : RandRange(2.0f, 5.5f);
			V3 at = pp + V3(cosf(a) * r, sinf(a) * r, direct ? 0.9f : 0.0f);
			Log("'%s' throws something at Arthur (%s, %.0f m out)", label.c_str(), direct ? "right at him" : "just short", d);
			aimedThrows++;
			EjectOrbiter((size_t)pick, t, &at);
		}
		else
			nextAimedThrow = t + 1.0f;
	}
	for (size_t i = 0; i < orbiters.size();)
	{
		Orbiter& ob = orbiters[i];
		if (!ob.o || !ENTITY::DOES_ENTITY_EXIST(ob.o))
		{
			if (ob.o) g_orbiterSet.erase(ob.o);
			ownEntities.erase(ob.o);
			orbiters.erase(orbiters.begin() + i);
			continue;
		}
		float r = std::max(2.0f, radiusAt(ob.h) * ob.rMul);
		ob.ang += std::min(fp.vmax * 1.25f * ob.speedMul / r, 6.0f) * dt;   // v0.7: a bit faster - the debris is what reads as spin (v1.1: capped, for minis)
		ob.h += ob.climb * dt;
		if (ob.h >= ob.hTop)
		{
			// at the top: one in five is flung out with real physics (at most one every 3 s); the rest drop back into
			// the base and climb again (the drop happens inside the wall cloud, where you can't see it)
			if (!opts.display && Rand01() < 0.2f && t - lastEject > 3.0f)   // (exhibits keep theirs)
			{
				lastEject = t;
				// v1.3 (playtest 11: "a few more things being thrown out from the nado at his general direction"): half of them
				// head his way, landing somewhere round him
				V3 pp = ENTITY::GET_ENTITY_COORDS(PLAYER::PLAYER_PED_ID(), TRUE, FALSE);
				if (g_set.throwAtArthur && (pp - base).len2d() < reachRadius() * 2.0f && Rand01() < 0.5f && ArthurTargetable(PLAYER::PLAYER_PED_ID()))
				{
					float a = RandRange(0, 2 * PI), r = RandRange(5.0f, 14.0f);
					V3 spot = pp + V3(cosf(a) * r, sinf(a) * r, 0);
					aimedThrows++;
					EjectOrbiter(i, t, &spot);
				}
				else
					EjectOrbiter(i, t);
				continue;
			}
			ob.h = RandRange(0.02f, 0.08f);
		}
		ob.rot = ob.rot + ob.rotRate * dt;
		V3 p = FunnelPoint(ob.h, ob.ang, ob.rMul, t);
		ENTITY::SET_ENTITY_COORDS_NO_OFFSET(ob.o, p.x, p.y, p.z, FALSE, FALSE, FALSE);
		ENTITY::SET_ENTITY_ROTATION(ob.o, ob.rot.x, ob.rot.y, ob.rot.z, 2, TRUE);
		i++;
	}
}

// ======================= v1.2: lights and junk =======================
// Glow from inside: point lights spiralling up the core, flickering for fire. Nothing to load or count - they're drawn each
// frame - so they show even where the smoke is thin, and at night a Firenado lights the ground round it.
void Tornado::DrawLights(float t)
{
	const Style& s = GetStyles()[styleIdx];
	if (s.lights <= 0 || dead)
		return;
	float spin = s.spinDeg * PI / 180.0f, sz = sqrtf(sizeMul());
	float fade = dissipating ? Clamp(1.0f - (t - dissipateStart) / 6.0f, 0.0f, 1.0f) : growth;
	for (int i = 0; i < s.lights; i++)
	{
		float h = (i + 0.5f) / s.lights * 0.85f;
		V3 p = FunnelPoint(h, i * 2.39996f + spin * 0.8f * t, 0.3f, t);
		float k = 1.0f - s.flicker * (0.5f + 0.5f * sinf(t * 13.0f + i * 1.7f)) * (0.6f + 0.4f * sinf(t * 7.3f + i));
		GRAPHICS::DRAW_LIGHT_WITH_RANGE(p.x, p.y, p.z, (int)(s.lr * 255), (int)(s.lg * 255), (int)(s.lb * 255),
			s.lightRange * sz, s.lightPower * k * fade);
	}
}

// Junknado: the wall is MADE of props - planks, barrels, crates, wheels, doors - climbing a spiral and wrapping back to the
// bottom, tumbling as they go. A way to draw a funnel that doesn't depend on smoke at all.
static const char* kJunkModels[] = {
	"p_woodplank01x", "p_woodplank02x", "p_woodplank03x", "p_barrel02x", "p_crate01x", "p_crate03x",
	"p_haybale01x", "p_wagonwheel01", "p_door01x", "p_barrelhalf01x", "p_chair02x", "p_bucket01x",
};
static const int kJunkModelCount = sizeof(kJunkModels) / sizeof(kJunkModels[0]);

int Tornado::JunkAlive() const
{
	int n = 0;
	for (auto& j : junk) if (j.o && ENTITY::DOES_ENTITY_EXIST(j.o)) n++;
	return n;
}

void Tornado::BuildJunk()
{
	const Style& s = GetStyles()[styleIdx];
	junkWant = 0;
	if (s.junk <= 0)
		return;
	junkWant = g_set.perf == 2 ? s.junk * 2 / 5 : g_set.perf == 1 ? s.junk * 7 / 10 : s.junk;
	if (opts.mini) junkWant = std::min(junkWant, 18);
	TopUpJunk();
}

// makes the props still missing (a model that hasn't streamed in yet is asked for and tried again a second later)
void Tornado::TopUpJunk()
{
	int made = 0, missing = 0;
	for (int i = (int)junk.size(); i < junkWant; i++)
	{
		Hash model = H(kJunkModels[i % kJunkModelCount]);
		if (!STREAMING::HAS_MODEL_LOADED(model)) { STREAMING::REQUEST_MODEL(model, FALSE); missing++; continue; }
		Junk j;
		j.h = fmodf(i * 0.6180339f, 1.0f);
		j.ang = i * 2.39996f;
		j.rMul = RandRange(0.85f, 1.1f);
		auto sgn = []() { return Rand01() < 0.5f ? -1.0f : 1.0f; };
		j.rot = V3(RandRange(0, 360), RandRange(0, 360), RandRange(0, 360));
		j.rotRate = V3(sgn() * RandRange(60.f, 240.f), sgn() * RandRange(40.f, 180.f), sgn() * RandRange(60.f, 240.f));
		V3 p = FunnelPoint(j.h, j.ang, j.rMul, NowSec());
		j.o = OBJECT::CREATE_OBJECT(model, p.x, p.y, p.z, FALSE, FALSE, TRUE, FALSE, FALSE);
		if (!j.o) continue;
		ENTITY::SET_ENTITY_COLLISION(j.o, FALSE, FALSE);
		ENTITY::SET_ENTITY_HAS_GRAVITY(j.o, FALSE);
		ENTITY::SET_ENTITY_INVINCIBLE(j.o, TRUE);
		ENTITY::FREEZE_ENTITY_POSITION(j.o, TRUE);
		if (g_set.perf < 2) ENTITY::SET_ENTITY_LOD_DIST(j.o, 1500);
		g_orbiterSet.insert(j.o);   // never grabbed, never swept
		ownEntities.insert(j.o);
		junk.push_back(j);
		made++;
	}
	if (made || missing)
		Log("'%s' junk wall: %d props made, %d of %d now (%d waiting for their model)", label.c_str(), made, (int)junk.size(), junkWant, missing);
}

void Tornado::UpdateJunk(float t)
{
	if ((int)junk.size() < junkWant && t >= junkRetry && !dissipating)
	{
		junkRetry = t + 1.0f;
		TopUpJunk();
	}
	if (junk.empty())
		return;
	const Style& s = GetStyles()[styleIdx];
	float spin = s.spinDeg * PI / 180.0f, dt = Clamp(MISC::GET_FRAME_TIME(), 0.0f, 0.1f);
	float fade = dissipating ? Clamp(1.0f - (t - dissipateStart) / 6.0f, 0.0f, 1.0f) : 1.0f;
	for (auto& j : junk)
	{
		if (!j.o || !ENTITY::DOES_ENTITY_EXIST(j.o)) continue;
		float h = fmodf(j.h + 0.045f * t, 1.0f);                     // they climb and wrap round to the bottom
		float ang = j.ang + spin * Lerp(1.3f, 0.7f, h) * t;
		V3 p = FunnelPoint(h, ang, j.rMul * Lerp(0.75f, 1.0f, fade), t);
		j.rot = j.rot + j.rotRate * dt;
		ENTITY::SET_ENTITY_COORDS_NO_OFFSET(j.o, p.x, p.y, p.z, FALSE, FALSE, FALSE);
		ENTITY::SET_ENTITY_ROTATION(j.o, j.rot.x, j.rot.y, j.rot.z, 2, TRUE);
	}
}

void Tornado::DeleteJunk()
{
	bool had = !junk.empty();
	for (auto& j : junk)
		if (j.o)
		{
			g_orbiterSet.erase(j.o);
			ownEntities.erase(j.o);
			DeleteObj(j.o);
		}
	junk.clear();
	junkWant = 0;
	if (had)
		for (const char* m : kJunkModels) STREAMING::SET_MODEL_AS_NO_LONGER_NEEDED(H(m));   // (the debris cone asks again for the ones it uses)
}

void Tornado::DeleteOrbiters()
{
	for (auto& ob : orbiters)
		if (ob.o)
		{
			g_orbiterSet.erase(ob.o);
			ownEntities.erase(ob.o);
			DeleteObj(ob.o);
		}
	orbiters.clear();
}

// ======================= impacts (v0.6) =======================
// Playtest 6: "the wood sprays look like collisions" (random splinter bursts in the air - removed) and "use the effects
// wisely, you want things to be interactive and be like, oh, that collided and specifically caused that" / "random
// explosions, bottles... it should create some random chaos". So effects now come from real collisions: anything the
// tornado threw that suddenly loses most of its speed hit something, and gets a burst where it hit - dust for people and
// animals, splintering planks for wagons, smashing glass or dust for props.
struct Flyer { float lastSpeed = 0; float until = 0; float lastFx = -100; int type = 3; };
static std::unordered_map<Entity, Flyer> g_flyers;
static float g_nextImpactTick = 0;
static int g_impacts = 0;
static std::vector<float> g_impactTimes;
int ImpactsShown() { return g_impacts; }

void NoteFlyer(Entity e, int type, float t)
{
	if (!g_set.impacts || g_set.perf == 2)
		return;
	auto it = g_flyers.find(e);
	if (it == g_flyers.end())
	{
		if (g_flyers.size() >= 250)
			return;
		it = g_flyers.emplace(e, Flyer()).first;
		it->second.type = type;
	}
	it->second.until = t + 6.0f;
}

void UpdateImpacts(float t)
{
	if (t < g_nextImpactTick)
		return;
	g_nextImpactTick = t + 0.1f;
	while (!g_impactTimes.empty() && t - g_impactTimes.front() > 1.0f)
		g_impactTimes.erase(g_impactTimes.begin());
	for (auto it = g_flyers.begin(); it != g_flyers.end();)
	{
		Flyer& f = it->second;
		if (t > f.until || !ENTITY::DOES_ENTITY_EXIST(it->first))
		{
			it = g_flyers.erase(it);
			continue;
		}
		float speed = V3(ENTITY::GET_ENTITY_VELOCITY(it->first, 0)).len();
		if (f.lastSpeed > 18.0f && speed < f.lastSpeed * 0.3f && t - f.lastFx > 2.0f && g_impactTimes.size() < 4)
		{
			f.lastFx = t;
			g_impactTimes.push_back(t);
			g_impacts++;
			V3 p = ENTITY::GET_ENTITY_COORDS(it->first, FALSE, FALSE);
			float k = Clamp(f.lastSpeed / 25.0f, 0.7f, 1.6f);
			if (f.type == 1)
			{
				Burst("ent_dst_dust", p, 1.6f * k);
				Burst("bang_dirt_dry", p, 1.3f * k);
			}
			else if (f.type == 2)
			{
				Burst("ent_brk_wood_planks", p, 1.4f * k);
				Burst("ent_dst_dust", p, 2.0f * k);
			}
			else
				Burst(Rand01() < 0.35f ? "ent_brk_glass_bottle" : "ent_brk_dust", p, 1.3f * k);
			if (g_impacts <= 5 || g_impacts % 50 == 0)
				Log("impact: entity %d (type %d) hit something at %.0f m/s - burst %d", it->first, f.type, f.lastSpeed, g_impacts);
		}
		f.lastSpeed = speed;
		++it;
	}
}

// ======================= floating-prop sweeper =======================
// Playtest 2 and 4: "another floating barrel", "this mod might leave permanent random floating objects in your game".
// Every prop the tornado pushes is remembered; once it has been left alone for a moment, one that hangs in mid-air
// (high above the ground, not moving, nothing under it) gets its physics woken.
// v0.5, playtest 5: the v0.4 sweeper read height with GET_ENTITY_HEIGHT_ABOVE_GROUND, which returns WORLD Z for many
// props - so 836 props lying on the ground looked 40-70 m up and were TELEPORTED ("set it down"). Now:
//   - height comes from a ground probe just above the prop (TrueHeight); no probe result -> leave it alone;
//   - it never moves a prop: at most two physics wakes, then it gives up (and logs it);
//   - at most 20 wakes a minute in total, each one logged.
struct Touch { float lastPush = 0; float stillFor = 0; float lastCheck = -1; int seen = 0; int wakes = 0; V3 origin; bool airborne = false; bool gaveUpOnce = false; };
static std::unordered_map<Entity, Touch> g_touched;
static std::vector<Entity> g_sweepOrder;   // the sweeper works through this a batch at a time (no frame spikes)
static size_t g_sweepPos = 0;
static float g_nextSweep = 0;
static int g_hoverWoken = 0, g_hoverSetDown = 0, g_hoverGaveUp = 0, g_hoverNatural = 0;
int HoverNatural() { return g_hoverNatural; }
static std::vector<float> g_wakeTimes;     // rate cap
int HoverWoken() { return g_hoverWoken; }
int HoverSetDown() { return g_hoverSetDown; }   // v0.5: always 0 - nothing is teleported any more (kept for the logs)
int HoverGaveUp() { return g_hoverGaveUp; }

bool TrueHeight(const V3& p, float* out)
{
	float gz = 0;
	if (!MISC::GET_GROUND_Z_FOR_3D_COORD(p.x, p.y, p.z + 1.0f, &gz, FALSE))
		return false;
	*out = p.z - gz;
	return true;
}

void NoteTouched(Entity e, float t)
{
	if (IsAnchor(e) || IsScripted(e))
		return;
	auto it = g_touched.find(e);
	if (it == g_touched.end())
	{
		if (g_touched.size() >= 3000)
			return;
		it = g_touched.emplace(e, Touch()).first;
		it->second.origin = ENTITY::GET_ENTITY_COORDS(e, FALSE, FALSE);   // where the tornado first found it
		// v0.7, playtest 7: thrown debris and props flung off the debris cone were first seen 45-95 m up, and if they then
		// hung there the sweeper took them for signs ("left alone" 310 times). Anything first seen more than 12 m up was
		// already flying - it is never treated as a hanging sign.
		float h0 = 0;
		it->second.airborne = TrueHeight(it->second.origin, &h0) && h0 > 12.0f;
	}
	it->second.lastPush = t;
}

static bool RestingOnSomething(Entity e, const V3& p, float h)
{
	int sh = SHAPETEST::START_EXPENSIVE_SYNCHRONOUS_SHAPE_TEST_LOS_PROBE(p.x, p.y, p.z, p.x, p.y, p.z - h - 0.5f, -1, e, 0);
	BOOL hit = FALSE;
	Vector3 end = {}, normal = {};
	Entity other = 0;
	SHAPETEST::GET_SHAPE_TEST_RESULT(sh, &hit, &end, &normal, &other);
	return hit && p.z - end.z < 1.0f;   // a table, a roof, a wagon bed...
}

void HoverSweep(float t)
{
	if (t < g_nextSweep)
		return;
	g_nextSweep = t + 0.25f;
	if (!g_set.hoverFix)
		return;
	if (g_sweepPos >= g_sweepOrder.size())
	{
		g_sweepOrder.clear();
		for (auto& kv : g_touched) g_sweepOrder.push_back(kv.first);
		g_sweepPos = 0;
	}
	while (!g_wakeTimes.empty() && t - g_wakeTimes.front() > 60.0f)
		g_wakeTimes.erase(g_wakeTimes.begin());
	int budget = 24;   // expensive checks per quarter second (v0.7: was 60 - performance)
	while (g_sweepPos < g_sweepOrder.size() && budget > 0)
	{
		Entity e = g_sweepOrder[g_sweepPos++];
		auto it = g_touched.find(e);
		if (it == g_touched.end())
			continue;
		Touch& tc = it->second;
		float since = tc.lastCheck < 0 ? 0.25f : t - tc.lastCheck;
		tc.lastCheck = t;
		// v1.1: a known floater (it already had its wake) is kept for 5 min, not 90 s - playtest 9 left a bucket 67 m up
		if (!ENTITY::DOES_ENTITY_EXIST(e) || t - tc.lastPush > (tc.wakes > 0 ? 300.0f : 90.0f) || InFlight(e))
		{
			g_touched.erase(it);
			continue;
		}
		if (t - tc.lastPush < 1.5f)
			continue;   // still in a tornado's grip
		budget--;
		V3 p = ENTITY::GET_ENTITY_COORDS(e, FALSE, FALSE);
		float h = 0;
		if (!TrueHeight(p, &h))
			continue;   // no ground under it that we can find: never act on a guess
		float speed = V3(ENTITY::GET_ENTITY_VELOCITY(e, 0)).len();
		if (h > 1.2f && speed < 0.35f)
		{
			if (RestingOnSomething(e, p, h))
			{
				g_touched.erase(it);
				continue;
			}
			// Two separate sightings a second apart, so a prop at the top of its arc isn't mistaken for a floater.
			tc.seen++;
			tc.stillFor += since;
			if (tc.seen >= 2 && tc.stillFor >= 1.0f)
			{
				tc.seen = 0;
				tc.stillFor = 0;
				// v0.6: playtest 6's 93 "gave up" props were mostly hanging signs and lanterns still where the tornado
				// found them. Those are left alone. Something the tornado actually carried and left in mid-air is brought
				// down: one physics wake (which rarely works in RDR2), then it falls under our own gravity.
				if ((p - tc.origin).len() < 3.0f && !tc.airborne)
				{
					g_hoverNatural++;
					if (g_hoverNatural <= 5 || g_hoverNatural % 25 == 0)
						Log("hover: prop %d (model 0x%08X) %.1f m up is where the tornado found it (a sign or lantern?) - left alone (%d)", e, ENTITY::GET_ENTITY_MODEL(e), h, g_hoverNatural);
					g_touched.erase(it);
					continue;
				}
				if (tc.wakes >= 1)
				{
					if (StartGlide(e, t))
					{
						Log("hover: prop %d (model 0x%08X) still %.1f m up after a wake - bringing it down (glided %d)", e, ENTITY::GET_ENTITY_MODEL(e), h, HoverGlided());
						g_touched.erase(it);
					}
					else
					{
						// v1.1, playtest 9 ("gave up 4", a bucket left 67 m up): the flight slots were full. Keep it and try
						// again on a later pass instead of abandoning it.
						if (!tc.gaveUpOnce) { tc.gaveUpOnce = true; g_hoverGaveUp++; }
					}
					continue;
				}
				if ((int)g_wakeTimes.size() >= 20)
					continue;   // rate cap: try again on a later pass
				tc.wakes++;
				g_wakeTimes.push_back(t);
				ENTITY::FREEZE_ENTITY_POSITION(e, FALSE);
				ENTITY::SET_ENTITY_DYNAMIC(e, TRUE);
				ENTITY::SET_ENTITY_HAS_GRAVITY(e, TRUE);
				PHYSICS::ACTIVATE_PHYSICS(e);
				ENTITY::SET_ENTITY_VELOCITY(e, 0, 0, -2.0f);
				g_hoverWoken++;
				Log("hover: woke floating prop %d (model 0x%08X, %.1f m above the ground at z %.1f, wake %d) - total %d",
					e, ENTITY::GET_ENTITY_MODEL(e), h, p.z, tc.wakes, g_hoverWoken);
			}
		}
		else
		{
			tc.seen = 0;
			tc.stillFor = 0;
			if (h < 0.8f && speed < 0.2f && t - tc.lastPush > 6.0f)
				g_touched.erase(it);   // came to rest on the ground
		}
	}
}

// v0.7, playtest 7: "if it cleans itself up after the tornado goes away, that's fine too. But we don't want to leave it."
// When a tornado ends, everything it touched within its reach that's still hanging in the air is brought down at once
// (hanging signs and lanterns still where it found them are left alone).
void SweepAfterTornado(const V3& base, float radius, float t)
{
	int glided = 0;
	for (auto it = g_touched.begin(); it != g_touched.end();)
	{
		Entity e = it->first;
		Touch& tc = it->second;
		if (!ENTITY::DOES_ENTITY_EXIST(e) || InFlight(e)) { it = g_touched.erase(it); continue; }
		V3 p = ENTITY::GET_ENTITY_COORDS(e, FALSE, FALSE);
		float h = 0;
		if ((p - base).len2d() > radius || !TrueHeight(p, &h) || h <= 1.2f || V3(ENTITY::GET_ENTITY_VELOCITY(e, 0)).len() > 0.35f)
		{
			++it;
			continue;
		}
		if ((p - tc.origin).len() < 3.0f && !tc.airborne) { ++it; continue; }
		if (!StartGlide(e, t))
			break;   // the flight cap is full: the regular sweeper gets the rest
		glided++;
		it = g_touched.erase(it);
	}
	if (glided)
		Log("after the tornado: brought down %d props it left hanging", glided);
}

// ======================= Arthur's soft landing =======================
// Playtest 4: riding the tornado was "the best tornado test we've had yet" - and the drop killed Arthur every time
// he wasn't invincible. Only for a few seconds after the tornado had him, so ordinary falls are left alone.
// v0.5, playtest 5: Arthur still died (log 17:18:24: "falling -16.2 m/s at 15.8 m", then dead). v0.4 only started
// below 16 m, used the broken height native, and nudged a ragdoll with a tiny force. Now:
//   - real height from a ground probe, starting from 45 m up;
//   - the fall speed is SET every frame to a safe profile (about -24 m/s at 45 m down to -5 m/s at the ground);
//   - below 12 m Arthur (and his horse) can't be damaged until 2.5 s after he stops falling - the backstop;
//   - every landing is logged: height, speed, and health before and after.
static float g_playerLastInWall = -100.0f, g_lastSoftLog = -100.0f;
static float g_dropWindowUntil = -100.0f;
// v1.3: how this drop ends - 0 soft (the catch), 1 rough (a hard landing: ragdoll, hurts), 2 the stratosphere, 3 slammed down
// v1.6 (playtest 14: "more throw options - an even farther one, a stratosphere higher in the sky one, and a far soft landing, plus
// one more dramatic fast into the ground but this one doesn't hurt you"): 4 far (a long, flat throw, a rough landing), 5 sky-high
// (straight up, still climbing for 3.5 s: 300 m+), 6 far soft (out and up, then a long gentle glide down), 7 meteor (a hop, then
// into the ground at 46 m/s - a crater of dust, and not a scratch)
static int g_landKind = 0;
static bool g_roughRagdolled = false;
static const char* kLandKinds[] = { "soft", "rough", "stratosphere", "slammed", "far", "sky-high", "far soft", "meteor" };
static float g_skyUntil = -100.0f;   // sky-high: it keeps climbing until then
static V3 g_glideVel;                // far soft: the drift it keeps on the way down
static bool g_meteorHit = false;
static int g_arthurThrows = 0;       // every throw (the ride camera hands back when this changes)
int ArthurThrows() { return g_arthurThrows; }
static bool LandHurts(int k) { return k >= 1 && k <= 5; }   // (soft, the glide and the meteor don't)
static int PickLandKind()
{
	float r = Rand01();
	if (g_set.landings == 0) return r < 0.3f ? 6 : 0;   // Soft: caught - or a long glide down
	if (g_set.landings == 2) return r < 0.35f ? 1 : r < 0.5f ? 2 : r < 0.65f ? 3 : r < 0.85f ? 4 : 5;   // Real: no catch, so only the ones that hurt
	static const float kW[8] = { 0.18f, 0.22f, 0.10f, 0.10f, 0.12f, 0.08f, 0.12f, 0.08f };
	float acc = 0;
	for (int k = 0; k < 8; k++) { acc += kW[k]; if (r < acc) return k; }
	return 1;
}
// v1.4 (playtest 12 ended in a crash: the game froze 5 s after the first stratosphere throw - 90 m/s straight up over Saint
// Denis - and gave up 21 s later with ERROR FFFFFFFF). The big towns, where the game streams the most: no stratosphere there.
struct TownCircle { float x, y, r; };
static const TownCircle kBigTowns[] = {
	{ 2580.f, -1230.f, 520.f },   // Saint Denis
	{ -800.f, -1300.f, 260.f },   // Blackwater
	{ -280.f, 760.f, 230.f },     // Valentine
	{ 1300.f, -1300.f, 230.f },   // Rhodes
	{ 2930.f, 1300.f, 230.f },    // Annesburg
	{ -1800.f, -400.f, 200.f },   // Strawberry
	{ -3650.f, -2600.f, 200.f },  // Armadillo
	{ -5500.f, -2950.f, 200.f },  // Tumbleweed
	{ 2950.f, 560.f, 160.f },     // Van Horn
};
bool InSaintDenis(const V3& p, float margin) { V3 c(kBigTowns[0].x, kBigTowns[0].y, 0); return (V3(p.x, p.y, 0) - c).len2d() < kBigTowns[0].r + margin; }
bool InBigTown(const V3& p, float margin = 0.0f)
{
	for (auto& c : kBigTowns)
		if ((p.x - c.x) * (p.x - c.x) + (p.y - c.y) * (p.y - c.y) < (c.r + margin) * (c.r + margin)) return true;
	return false;
}
// v1.3 audit: every way the tornado lets go of Arthur goes through here, so the kind it picks is the throw he gets. fromWall is
// fling and chase (hurled out of the wall); otherwise he's let go at the top of his ride.
static V3 LaunchArthur(const V3& out, const V3& tang, const ForceProfile& fp, float mul, float t, bool fromWall, float vzNow)
{
	g_arthurThrows++;
	g_landKind = PickLandKind();
	g_roughRagdolled = false;
	// (v1.6 review: as far as the throw can carry him - a far throw from just outside Saint Denis lands in it)
	if ((g_landKind == 2 || g_landKind == 4 || g_landKind == 5) &&
		InBigTown(ENTITY::GET_ENTITY_COORDS(PLAYER::PLAYER_PED_ID(), TRUE, FALSE), g_landKind == 4 ? 450.0f : 150.0f))
	{
		Log("landings: no %s throw over a town - a rough one instead", kLandKinds[g_landKind]);
		g_landKind = 1;   // (v1.4: not over a town - a hard landing instead; v1.6: nor the far and the sky-high ones)
	}
	V3 v;
	if (g_landKind == 4)
	{
		g_dropWindowUntil = t + 30.0f;
		v = out * 50.0f + tang * (fp.vmax * 0.15f) + V3(0, 0, 30.0f);   // far: the longest throw (300 m or so - it keeps its speed through the catch)
	}
	else if (g_landKind == 5)
	{
		g_dropWindowUntil = t + 60.0f;
		g_skyUntil = t + 3.5f;   // (PlayerSafety keeps him climbing at 50 m/s till then)
		v = out * 6.0f + V3(0, 0, 55.0f);   // sky-high
	}
	else if (g_landKind == 6)
	{
		g_dropWindowUntil = t + 40.0f;
		v = out * 28.0f + tang * (fp.vmax * 0.2f) + V3(0, 0, 18.0f);   // far soft: out and up...
		g_glideVel = V3(v.x, v.y, 0) * 0.8f;                            // ...and this drift all the way down
	}
	else if (g_landKind == 7)
	{
		g_dropWindowUntil = t + 20.0f;
		g_meteorHit = false;
		v = out * 10.0f + V3(0, 0, 20.0f);   // meteor: a hop up first
	}
	else if (g_landKind == 2)
	{
		g_dropWindowUntil = t + 40.0f;   // (a long way down: the catch waits for him)
		// v1.4: 40-52 m/s (was 70-95): still a 100 m+ trip, but nothing the game's streaming hasn't seen from a balloon
		v = out * (fp.ejectSpeed * 0.7f) + tang * (fp.vmax * 0.3f) + V3(0, 0, RandRange(40.0f, 52.0f));   // the stratosphere
	}
	else if (g_landKind == 3)
		v = out * (fp.ejectSpeed * 2.6f * mul) + tang * (fp.vmax * 0.4f) + V3(0, 0, -RandRange(4.0f, 10.0f));   // slammed down
	else if (fromWall)
		v = out * (fp.ejectSpeed * 2.0f * mul) + tang * (fp.vmax * 0.6f) + V3(0, 0, 15.0f * mul);
	else
	{
		v = out * fp.ejectSpeed + tang * (fp.vmax * 0.5f);
		v.z = std::max(vzNow, 2.0f);
	}
	float sp = v.len();
	if (sp > kMaxArthurLaunch) v = v * (kMaxArthurLaunch / sp);   // (v1.4: never faster than this, whatever the force)
	return v;
}
static float g_backstopUntil = -100.0f;
static bool g_backstopOn = false;
static Entity g_backstopMount = 0;
static float g_fallPeak = 0, g_lastFallV = 0, g_fallTopH = 0;
float PlayerLastInWall() { return g_playerLastInWall; }
static LandingStats g_landing;
const LandingStats& GetLanding() { return g_landing; }
void NotePlayerInWall(float t) { g_playerLastInWall = t; }
// v1.3 audit: a balloon bail or the drop test is always caught soft, even on Landings: Real (Real is about the tornado's throws)
static float g_testWindowUntil = -100.0f;
void StartDropTest(float t) { g_dropWindowUntil = g_testWindowUntil = t + 20.0f; g_landKind = 0; g_roughRagdolled = false; }

static void BackstopOff(const char* why)
{
	if (!g_backstopOn)
		return;
	Ped me = PLAYER::PLAYER_PED_ID();
	ENTITY::SET_ENTITY_CAN_BE_DAMAGED(me, TRUE);
	if (g_backstopMount && ENTITY::DOES_ENTITY_EXIST(g_backstopMount))
		ENTITY::SET_ENTITY_CAN_BE_DAMAGED(g_backstopMount, TRUE);
	g_backstopMount = 0;
	g_backstopOn = false;
	g_landing.active = false;
	g_landing.healthAfter = ENTITY::GET_ENTITY_HEALTH(me);
	// v0.7, playtest 7: "whether or not it hurts you, or can just at least damage you a little bit each time, might make
	// sense for a game". Not when Arthur is invincible, and never below a quarter of his health.
	if (LandHurts(g_landKind) && !g_set.playerGod && strcmp(why, "landed") == 0)
	{
		// v1.3: a rough landing hurts - a quarter to nearly half his health - but never below a fifth of what he had
		float frac = (g_landKind == 2 || g_landKind == 5) ? 0.45f : g_landKind == 3 ? 0.35f : g_landKind == 4 ? 0.3f : 0.25f;
		int floorH = std::max(40, (int)(g_landing.healthBefore * 0.2f));
		int to = std::max(floorH, g_landing.healthAfter - (int)(g_landing.healthBefore * frac));
		if (to < g_landing.healthAfter) { ENTITY::SET_ENTITY_HEALTH(me, to, 0); g_landing.healthAfter = to; }
	}
	else if (g_set.landingHurts && g_landKind != 7 && !g_set.playerGod && strcmp(why, "landed") == 0 && g_landing.healthAfter > 120)
	{
		ENTITY::SET_ENTITY_HEALTH(me, g_landing.healthAfter - 20, 0);
		g_landing.healthAfter -= 20;
	}
	if (strcmp(why, "landed") == 0) { g_landKind = 0; g_roughRagdolled = false; }
	Log("soft landing: backstop off (%s) | touched down at %.1f m/s, health %d -> %d, landings %d", why, g_landing.lastImpact,
		g_landing.healthBefore, g_landing.healthAfter, g_landing.landings);
}

void PlayerSafety(float dt, float t)
{
	// v1.6 sky-high: he keeps climbing for 3.5 s (at 50 m/s - under the cap)
	if (g_landKind == 5 && t < g_skyUntil)
	{
		Ped me0 = PLAYER::PLAYER_PED_ID();
		Entity b0 = PED::IS_PED_ON_MOUNT(me0) ? PED::GET_MOUNT(me0) : me0;
		V3 v0 = ENTITY::GET_ENTITY_VELOCITY(b0, 0);
		if (v0.z < 50.0f)
		{
			ENTITY::SET_ENTITY_VELOCITY(b0, v0.x * 0.98f, v0.y * 0.98f, 50.0f);
			if (b0 != me0) ENTITY::SET_ENTITY_VELOCITY(me0, v0.x * 0.98f, v0.y * 0.98f, 50.0f);
		}
	}
	else if (g_landKind == 2 || g_landKind == 5)
	{
		// (and on the way down from up there, never faster than 55 m/s - under the launch cap; the streaming was the worry in v1.4)
		Ped me0 = PLAYER::PLAYER_PED_ID();
		Entity b0 = PED::IS_PED_ON_MOUNT(me0) ? PED::GET_MOUNT(me0) : me0;
		V3 v0 = ENTITY::GET_ENTITY_VELOCITY(b0, 0);
		if (v0.z < -55.0f)
		{
			ENTITY::SET_ENTITY_VELOCITY(b0, v0.x, v0.y, -55.0f);
			if (b0 != me0) ENTITY::SET_ENTITY_VELOCITY(me0, v0.x, v0.y, -55.0f);
		}
	}
	bool window = t - g_playerLastInWall <= 15.0f || t < g_dropWindowUntil;
	bool test = t < g_testWindowUntil;
	if (!window || ((!g_set.softLanding || g_set.landings == 2) && !test))
	{
		if (g_backstopOn && t > g_backstopUntil) BackstopOff("window closed");
		if (!window && !g_backstopOn && g_landKind != 0) { g_landKind = 0; g_roughRagdolled = false; }   // (audit: a kind never outlives its drop)
		return;
	}
	if (test && t - g_playerLastInWall > 15.0f) g_landKind = 0;   // a bail, not a throw: soft
	Ped me = PLAYER::PLAYER_PED_ID();
	Entity body = PED::IS_PED_ON_MOUNT(me) ? PED::GET_MOUNT(me) : me;
	V3 p = ENTITY::GET_ENTITY_COORDS(body, TRUE, FALSE);
	V3 v = ENTITY::GET_ENTITY_VELOCITY(body, 0);
	float h = 0;
	bool known = TrueHeight(p, &h);
	bool falling = v.z < -3.0f;
	if (g_landKind == 7 && ENTITY::IS_ENTITY_IN_WATER(body))
	{
		g_landKind = 0;   // (v1.6 review: the ground probe finds the river bed - no driving him to the bottom)
		Log("landings: the meteor came down in water - a soft one instead");
	}
	float top = g_landKind == 6 ? 220.0f : g_landKind == 7 ? 90.0f : 45.0f;   // (v1.6: the glide and the meteor take over higher up)
	if (known && falling && h < top && h > 0.3f)
	{
		g_fallPeak = std::min(g_fallPeak, v.z);
		g_lastFallV = v.z;
		g_fallTopH = std::max(g_fallTopH, h);
		float want = -(5.0f + std::max(0.0f, h - 3.0f) * 0.45f);   // -5 m/s near the ground, about -24 m/s at 45 m
		bool rough = LandHurts(g_landKind);
		if (rough) want = -(13.0f + std::max(0.0f, h - 3.0f) * 0.55f);   // v1.3: a hard landing - fast, but it can't kill
		if (rough && h < 2.8f && !g_roughRagdolled && body == me)
		{
			g_roughRagdolled = true;   // he hits the ground in a heap
			PED::SET_PED_TO_RAGDOLL(me, 2500, 4500, 0, FALSE, FALSE, nullptr);
		}
		if (g_landKind == 6)
		{
			// v1.6 far soft: a long, gentle glide down - the drift kept (fading slowly), the fall held at 4.5 m/s
			g_glideVel = g_glideVel * std::max(0.0f, 1.0f - 0.12f * dt);
			g_dropWindowUntil = std::max(g_dropWindowUntil, t + 3.0f);   // (v1.6 review: a glide from 200 m+ outlasts a fixed window)
			ENTITY::SET_ENTITY_VELOCITY(body, g_glideVel.x, g_glideVel.y, std::max(v.z, -4.5f));
			if (body != me) ENTITY::SET_ENTITY_VELOCITY(me, g_glideVel.x, g_glideVel.y, std::max(v.z, -4.5f));
		}
		else if (g_landKind == 7)
		{
			// v1.6 meteor: straight into the ground, fast - and he can't be hurt (from the top of the dive, not the last 12 m)
			float down = h > 6.0f ? -46.0f : -28.0f;
			ENTITY::SET_ENTITY_VELOCITY(body, v.x * 0.9f, v.y * 0.9f, down);
			if (body != me) ENTITY::SET_ENTITY_VELOCITY(me, v.x * 0.9f, v.y * 0.9f, down);
			if (h < 2.6f && !g_meteorHit)
			{
				g_meteorHit = true;
				V3 g = p - V3(0, 0, h);
				Burst("bang_dirt_dry", g + V3(0, 0, 0.3f), 4.0f);
				Burst("ent_dst_dust", g + V3(0, 0, 0.5f), 4.5f);
				Burst("ent_brk_dirt", g + V3(0, 0, 0.4f), 3.5f);
				CAMERA::SHAKE_GAMEPLAY_CAM("MEDIUM_EXPLOSION_SHAKE", 0.8f);
				if (body == me) PED::SET_PED_TO_RAGDOLL(me, 2500, 4000, 0, FALSE, FALSE, nullptr);
				Log("landings: METEOR - into the ground at %.0f m/s, and not a scratch", -down);
			}
		}
		else if (v.z < want)
		{
			float keep = g_landKind == 4 ? 1.0f : 0.9f;   // (v1.6: the far throw keeps going while it's caught)
			ENTITY::SET_ENTITY_VELOCITY(body, v.x * keep, v.y * keep, want);
			if (body != me)
				ENTITY::SET_ENTITY_VELOCITY(me, v.x * keep, v.y * keep, want);
		}
		if ((h < 12.0f || g_landKind == 7 || g_landKind == 4) && g_fallTopH > 3.0f)   // (v1.6 review: the far throw too - 50 m/s sideways into a hillside)
		{
			if (!g_backstopOn)
			{
				g_backstopOn = true;
				g_landing.active = true;
				g_landing.healthBefore = ENTITY::GET_ENTITY_HEALTH(me);
				ENTITY::SET_ENTITY_CAN_BE_DAMAGED(me, FALSE);
				if (body != me)
				{
					g_backstopMount = body;
					ENTITY::SET_ENTITY_CAN_BE_DAMAGED(body, FALSE);
				}
				Log("soft landing: backstop on at %.1f m (falling %.1f m/s, health %d)", h, v.z, g_landing.healthBefore);
			}
			g_backstopUntil = t + 2.5f;
		}
		if (t - g_lastSoftLog > 0.25f && g_fallTopH > 3.0f)
		{
			g_lastSoftLog = t;
			Log("soft landing: %.1f m up, falling %.1f m/s -> held at %.1f", h, v.z, std::max(v.z, want));
		}
	}
	else if (known && h <= 1.5f && fabsf(v.z) < 2.0f)
	{
		// touched down: count it once (only a real drop, from more than 3 m), with the speed it had on the last stretch
		if (g_backstopOn && g_fallPeak < 0 && g_fallTopH > 3.0f)
		{
			g_landing.landings++;
			g_landing.lastImpact = g_lastFallV;
			g_landing.worstImpact = std::min(g_landing.worstImpact, g_lastFallV);
			Log("soft landing: TOUCHDOWN at %.1f m/s (fastest fall this drop %.1f m/s, from %.0f m) - a %s landing", g_lastFallV, g_fallPeak, g_fallTopH, kLandKinds[g_landKind % 8]);
		}
		g_fallPeak = 0;
		g_lastFallV = 0;
		g_fallTopH = 0;
	}
	if (g_backstopOn && t > g_backstopUntil)
		BackstopOff("landed");
	(void)dt;
}

// ======================= physics =======================
// Zones (playtest 2: "suck them in gradually, swirl them around the core, then fling them", plus the eye idea):
//   eye   (< eyeRadius)          calm: nothing is pushed
//   wall  (eye .. 1.6 x wall)     orbit at the wall radius, spiral upward, thrown out at each entity's release height
//   inflow(1.6 x wall .. reach)   horizontal pull inward that grows smoothly toward the wall; gravity left alone
void Tornado::ApplyPhysics(float dt, float t)
{
	ForceProfile fp = CurrentForce();
	if (opts.mini)
	{
		// v1.3 (playtest 11: "spin everything around it, small radius but still violent")
		fp.vmax *= 1.35f; fp.lift *= 1.6f; fp.ejectSpeed *= 1.5f; fp.tumble *= 1.6f;
		if (opts.tiny) { fp.vmax = std::min(fp.vmax * 0.58f, 24.0f); fp.ejectSpeed = std::min(fp.ejectSpeed * 0.45f, 18.0f); }   // v1.4 review: violent for its size - not 40-60 m/s throws (v1.6: a bit stronger, capped)
	}
	float wall = wallRadius(), eye = eyeRadius(), rReach = reachRadius(), H = height();
	float wallOuter = wall * 1.6f;
	int grabbed = 0;
	static int rippedTotal = 0;
	Ped me = PLAYER::PLAYER_PED_ID();
	Ped myMount = PED::IS_PED_ON_MOUNT(me) ? PED::GET_MOUNT(me) : 0;
	// v0.6, playtest 6 ("a mode for Grab Arthur... it just tugs Arthur every so often... Arthur ate a thousand
	// cheeseburgers... or easy to grab... bias it for Arthur independently of everybody else"): 1 tugged, 2 grabbable,
	// 3 easy prey. (grabPlayer without a level = grabbable.)
	int arthur = g_set.grabPlayer ? (g_set.arthur > 0 ? g_set.arthur : 2) : 0;
	if (opts.mini && arthur > 1) arthur = 1;   // v1.3: the gun's minis only shove Arthur about (he's the one shooting at the ground)
	// v1.5 (three crashes, all in Saint Denis: each came seconds after Arthur suddenly moved far - thrown 90 m/s, a death and
	// respawn, a 104 m drop - while the tornado had ripped loose and carried off hundreds of the city's fixed props, many of them
	// wired poles and lamps). In a big town the street furniture stays put: no ripping it loose, no carrying it off, no long draw
	// distance on map objects. People, horses, wagons and loose props still fly.
	// (v1.5 review: anywhere its reach overlaps a town - a funnel just outside the circle still reaches the street inside it)
	bool town = g_set.citySafe && InBigTown(base, reachRadius());

	for (Entity e : targets)
	{
		if (!ENTITY::DOES_ENTITY_EXIST(e) || IsAnchor(e) || IsOrbiter(e) || InFlight(e) || IsScripted(e))
			continue;
		int type = ENTITY::GET_ENTITY_TYPE(e);
		bool prio = g_priority.count(e) != 0;
		bool isMe = type == 1 && (e == me || (myMount && e == myMount));
		{
			if (isMe && (arthur == 0 || g_shieldPlayer)) continue;   // v1.1: in the intro / the balloon
			if (type == 1 && !isMe && !(PED::IS_PED_HUMAN(e) ? g_set.grabPeople : g_set.grabAnimals)) continue;
			if (type == 2 && (!g_set.grabVehicles || opts.tiny)) continue;   // (v1.4: a pocket twister doesn't throw wagons)
			if (type == 3 && !g_set.grabProps && !prio) continue;
		}
		Caught& c = caught[e];
		if (t < c.ejectedUntil)
			continue;

		V3 p = ENTITY::GET_ENTITY_COORDS(e, FALSE, FALSE);
		V3 rel = p - base;
		float dist = std::max(rel.len2d(), 0.5f);
		if (dist > rReach)
			continue;
		// v0.4 fix (playtest 4 log: "uprooted 0" all session): the "rip fixed props loose" rule reached planted trees
		// first, at 1.6 x the wall, so the uproot (1.3 x) never ran. Planted trees now go first - even inside the eye.
		if (prio && !c.ripped && dist < wallOuter && !opts.tiny)   // (v1.4 review: a pocket twister doesn't uproot trees)
		{
			c.ripped = true;
			Uproot(e, t);
			continue;
		}
		float hF = rel.z / H;
		if (g_set.eye && dist < eye && hF < 0.9f)
			continue;   // the eye: calm

		V3 out(rel.x / dist, rel.y / dist, 0);
		V3 inward = out * -1.0f;
		V3 tang(-rel.y / dist, rel.x / dist, 0);   // counter-clockwise seen from above
		bool isPed = type == 1, isObj = type == 3;
		V3 v = ENTITY::GET_ENTITY_VELOCITY(e, 0);
		V3 target;
		float gain;
		// v0.6, playtest 6 ("you want to see much more stuff moving around that circle. That creates the cone... pull them
		// a little bit more tightly towards the centre"): things orbit on the funnel's own cone - tight near the ground,
		// wider as they climb - instead of on one fixed ring, so everything caught traces the cone.
		float rCone = std::max(wall * 0.7f, radiusAt(Clamp(hF, 0.0f, 1.0f)) * 0.95f + (opts.tiny ? 0.3f : 2.0f));   // (v1.4 review: not a 2 m ring round a pocket twister)
		bool inWall = dist < std::max(wallOuter, rCone * 1.25f);

		if (inWall)
		{
			if (!c.lod && g_set.perf < 2 && !(town && isObj && !ownEntities.count(e)))
			{
				c.lod = true;
				ENTITY::SET_ENTITY_LOD_DIST(e, 1000);
			}
			if (isMe)
			{
				NotePlayerInWall(t);
				// v1.1, playtest 10: "held" used to be recorded only inside the fling branch, and Survive the storm turns fling
				// off - so it could never catch you (or show the ride camera). It's recorded for every level now.
				if (playerHeldSince < 0 || t - playerLastHeld > 2.0f)
				{
					playerHeldSince = t;
					playerHoldFor = RandRange(g_set.flingHoldMin, std::max(g_set.flingHoldMin, g_set.flingHoldMax));
				}
				playerLastHeld = t;
				// v0.5 fling-and-chase (playtest 5, 53:57). After 15-30 s in the wall Arthur (and his horse) are thrown
				// outward; soft landing catches the fall, and the tornado keeps coming. Not for "tugged" Arthur.
				if (g_set.flingChase && arthur >= 2)
				{
					if (t - playerHeldSince > playerHoldFor)
					{
						// v0.7, playtest 7: "eventually hurling us away, throwing us so far... it can be a little bit of a
						// cartoonish tornado". Soft landing catches it.
						// v1.0, playtest 8: "it should throw you even farther than that too sometimes"
						float throwMul = RandRange(1.0f, 1.6f);
						// v1.3 (playtest 11: "fling to the stratosphere, towards the ground... some variety")
						V3 fl = LaunchArthur(out, tang, fp, throwMul, t, true, v.z);
						Log("'%s' throws Arthur: a %s one", label.c_str(), kLandKinds[g_landKind]);
						Entity bodies[2] = { me, myMount };
						for (Entity b : bodies)
							if (b && ENTITY::DOES_ENTITY_EXIST(b))
							{
								ENTITY::SET_ENTITY_VELOCITY(b, fl.x, fl.y, fl.z);
								caught[b].ejectedUntil = t + 10.0f;
							}
						flings++;
						Log("'%s' FLING-AND-CHASE: threw Arthur out after %.0f s in the wall (%.0f m up, %.0f m/s) - fling %d",
							label.c_str(), t - playerHeldSince, rel.z, fl.len(), flings);
						playerHeldSince = -1;
						grabbed++;
						continue;
					}
				}
			}
			// Orbit the cone and climb.
			float release = 0.35f + 0.45f * (float)((unsigned)e * 2654435761u % 1000u) / 1000.0f;
			if (isMe && arthur == 3) release = 0.85f;   // easy prey rides high
			if (hF > release && !(isMe && arthur == 1))
			{
				if (isMe)
				{
					// v1.3 audit: Arthur and his horse are let go together, and the landing kind is the throw he really gets
					V3 fl = LaunchArthur(out, tang, fp, 1.0f, t, false, v.z);
					Log("'%s' lets go of Arthur at %.0f m up: a %s one", label.c_str(), rel.z, kLandKinds[g_landKind]);
					Entity bodies[2] = { me, myMount };
					for (Entity b : bodies)
						if (b && ENTITY::DOES_ENTITY_EXIST(b))
						{
							ENTITY::SET_ENTITY_VELOCITY(b, fl.x, fl.y, fl.z);
							caught[b].ejectedUntil = t + (g_landKind == 5 ? 9.0f : 3.0f);   // (v1.6 review: sky-high climbs for 3.5 s)
						}
					NoteFlyer(e, type, t);
					grabbed++;
					continue;
				}
				V3 fling = out * fp.ejectSpeed + tang * (fp.vmax * 0.5f);
				ENTITY::SET_ENTITY_VELOCITY(e, fling.x, fling.y, std::max(v.z, 2.0f));
				c.ejectedUntil = t + 3.0f;
				if (isObj) NoteTouched(e, t);
				NoteFlyer(e, type, t);
				grabbed++;
				continue;
			}
			float vt = fp.vmax * (dist < rCone ? Clamp(dist / rCone, 0.4f, 1.0f) : powf(rCone / dist, 0.7f));
			if (isMe) vt *= 1.4f;   // v0.7, playtest 7: "increase that spin rate a little bit... keep us moving around the circle"
			float vr = Clamp((rCone - dist) * 1.2f, -15.0f, 15.0f);   // pulls toward the cone from either side
			target = tang * vt + out * vr + moveVel;
			target.z = fp.lift * Clamp(1.0f - hF, 0.2f, 1.0f);
			gain = g_set.velocityGain;
			if (isMe && arthur == 1 && t >= tugTossAt)
			{
				// v0.7, playtest 7 (tugged: "just dragging me around on the ground more... throw me more"): a low toss
				// every 6-10 s, never more than a few metres up
				if (tugTossAt > 0)
				{
					V3 toss = out * 14.0f + tang * 10.0f + V3(0, 0, 4.0f);
					ENTITY::SET_ENTITY_VELOCITY(e, toss.x, toss.y, toss.z);
					PED::SET_PED_TO_RAGDOLL(e, 2500, 4000, 0, FALSE, FALSE, nullptr);
					NotePlayerInWall(t);
					tugTossAt = t + RandRange(6.0f, 10.0f);
					grabbed++;
					continue;
				}
				tugTossAt = t + RandRange(6.0f, 10.0f);
			}
			if (isMe && arthur == 1)
			{
				// tugged: dragged and spun about at ground level, never carried off
				target.z = rel.z > 4.0f ? -3.0f : std::min(target.z * 0.25f, 2.0f);
				gain *= 0.25f;   // v1.0, playtest 8: "you can't really escape it... make it an escapable tuggable"
			}
			else if (isMe && arthur == 3)
			{
				target.z *= 1.3f;
				gain *= 1.5f;
			}
			grabbed++;

			if (isObj && g_set.ripFixedProps && !c.ripped && !prio && !ownEntities.count(e) && !town)
			{
				c.ripped = true;
				ENTITY::FREEZE_ENTITY_POSITION(e, FALSE);
				ENTITY::SET_ENTITY_DYNAMIC(e, TRUE);
				rippedTotal++;
				if (rippedTotal % 25 == 1)
					Log("ripped loose fixed prop model 0x%08X (total %d)", ENTITY::GET_ENTITY_MODEL(e), rippedTotal);
			}
			// v0.6, playtest 6 ("some stuff doesn't immediately get woken up by the tornado, like this carriage with some
			// stuff in it... if we can make that stuff move, that would be cool"): a prop that hasn't budged after 1.2 s of
			// being pushed in the wall has dead physics - the mod carries it up the wall itself (like the trees), throws
			// it, and lays it down on the ground where it lands.
			if (isObj && !prio && !c.carried && !(town && !ownEntities.count(e)))   // (v1.5: a town's map props aren't carried off either)
			{
				if (c.wallSince < 0) { c.wallSince = t; c.wallStart = p; }
				else if (t - c.wallSince > 1.2f && (p - c.wallStart).len() < 0.6f && FlightsActive() < MaxFlights())
				{
					if (c.size < 0)
					{
						Vector3 mn = {}, mx = {};
						MISC::GET_MODEL_DIMENSIONS(ENTITY::GET_ENTITY_MODEL(e), &mn, &mx);
						c.size = std::max(std::max(mx.x - mn.x, mx.y - mn.y), mx.z - mn.z);
					}
					c.carried = true;
					if (c.size < 9.0f)
					{
						Carry(e, t);
						continue;
					}
				}
			}
			if (isPed && !isMe && t - c.lastRagdoll > 2.0f)
			{
				// "Living creatures can resist the tornado a little bit... throw them in a ragdoll and keep them flying
				// around that tornado, the funnier the better" (playtest 6)
				c.lastRagdoll = t;
				if (PED::IS_PED_ON_MOUNT(e) || PED::IS_PED_IN_ANY_VEHICLE(e, FALSE))
					PED::KNOCK_PED_OFF_VEHICLE(e);
				PED::SET_PED_TO_RAGDOLL(e, 4000, 6000, 0, FALSE, FALSE, nullptr);
			}
			else if (isMe && t - c.lastRagdoll > (arthur == 1 ? 6.0f : 2.5f))
			{
				c.lastRagdoll = t;
				if (arthur != 1 && (PED::IS_PED_ON_MOUNT(e) || PED::IS_PED_IN_ANY_VEHICLE(e, FALSE)))
					PED::KNOCK_PED_OFF_VEHICLE(e);
				PED::SET_PED_TO_RAGDOLL(e, 3000, 5000, 0, FALSE, FALSE, nullptr);
			}
			NoteFlyer(e, type, t);
		}
		else
		{
			c.wallSince = -1;
			// Inflow: grows smoothly from the reach edge (k=0) to the wall (k=1). Vertical speed untouched.
			float k = Clamp(1.0f - (dist - wallOuter) / std::max(1.0f, rReach - wallOuter), 0, 1);
			float vin = fp.vmax * 0.5f * (0.2f + 0.8f * k * k);
			target = inward * vin + tang * (vin * 0.6f) + moveVel * k;
			target.z = v.z;
			gain = g_set.velocityGain * (0.25f + 0.75f * k);
			if (isMe && arthur == 1) gain *= 0.3f;
			if (isPed)
			{
				if (k < 0.6f)
				{
					// people far out keep their footing (no more sudden "gusts") - but they run for it
					bool isMyHorse = myMount && myMount == e;   // never make Arthur's horse bolt
					if (g_set.npcFlee && !c.fled && !isMyHorse && !PED::IS_PED_A_PLAYER(e) && !PED::IS_PED_DEAD_OR_DYING(e, TRUE))
					{
						c.fled = true;
						TASK::TASK_SMART_FLEE_COORD(e, base.x, base.y, base.z, rReach * 1.5f, 20000, 0, 3.0f);
					}
					continue;
				}
				if (t - c.lastRagdoll > 2.5f)
				{
					c.lastRagdoll = t;
					if (PED::IS_PED_ON_MOUNT(e) || PED::IS_PED_IN_ANY_VEHICLE(e, FALSE))
						PED::KNOCK_PED_OFF_VEHICLE(e);
					PED::SET_PED_TO_RAGDOLL(e, 2500, 4000, 0, FALSE, FALSE, nullptr);
				}
				grabbed++;
			}
		}

		// Playtest 2: boxes and horseshoes hovered because we moved props whose physics was asleep.
		if (isObj && !c.woken)
		{
			c.woken = true;
			PHYSICS::ACTIVATE_PHYSICS(e);
		}
		if (isObj)
			NoteTouched(e, t);

		if (isMe)
		{
			float L = target.len();
			if (L > kMaxArthurLaunch) target = target * (kMaxArthurLaunch / L);   // (v1.4 review: the orbit reached 84 m/s on Extreme)
		}
		// v0.6: NPCs in the wall are steered by velocity (a ragdoll shrugs off force), so they keep flying round it.
		int method = g_set.pushMethod == 0 ? ((isPed && (!inWall || isMe)) ? 2 : 1) : g_set.pushMethod;
		float a = Clamp(gain * dt, 0, 1);
		V3 dv = (target - v) * a;
		if (method == 1)
			ENTITY::SET_ENTITY_VELOCITY(e, v.x + dv.x, v.y + dv.y, v.z + dv.z);
		else
			ENTITY::APPLY_FORCE_TO_ENTITY_CENTER_OF_MASS(e, 1, dv.x * g_set.forceGain, dv.y * g_set.forceGain, dv.z * g_set.forceGain, 0, FALSE, TRUE, TRUE);

		// Tumble: off-centre kicks so things spin as they fly. v0.4: scaled by the object's size and by Strength
		// (playtest 4: a lifted tree "rotates kind of slowly... more frantic would kind of be funny").
		if (isObj && inWall && Rand01() < 0.35f)
		{
			if (c.size < 0)
			{
				Vector3 mn = {}, mx = {};
				MISC::GET_MODEL_DIMENSIONS(ENTITY::GET_ENTITY_MODEL(e), &mn, &mx);
				c.size = std::max(std::max(mx.x - mn.x, mx.y - mn.y), mx.z - mn.z);
			}
			float s = Clamp(c.size, 0.5f, 12.0f);
			float kick = (1.5f + s * 0.9f) * fp.tumble;
			ENTITY::APPLY_FORCE_TO_ENTITY(e, 1, 0, 0, RandRange(-kick, kick), s * 0.35f, 0, 0, 0, FALSE, TRUE, TRUE, FALSE, TRUE);
			if (Rand01() < 0.5f)
				ENTITY::APPLY_FORCE_TO_ENTITY(e, 1, RandRange(-kick, kick), 0, 0, 0, 0, s * 0.35f, 0, FALSE, TRUE, TRUE, FALSE, TRUE);
		}
	}
	grabbedLastFrame = grabbed;
	if (caught.size() > 1500)
		caught.clear();
}

void Tornado::SpawnDebris(float t)
{
	for (size_t i = 0; i < debris.size();)
	{
		Object o = debris[i];
		bool gone = !ENTITY::DOES_ENTITY_EXIST(o) || (V3(ENTITY::GET_ENTITY_COORDS(o, FALSE, FALSE)) - base).len2d() > reachRadius() * 1.5f;
		if (gone)
		{
			ownEntities.erase(o);
			if (ENTITY::DOES_ENTITY_EXIST(o))
				ENTITY::SET_ENTITY_AS_NO_LONGER_NEEDED(&o);
			debris.erase(debris.begin() + i);
		}
		else i++;
	}
	if (!g_set.extraDebris || t < nextDebris || debris.size() >= 30)
		return;
	nextDebris = t + 0.6f;
	Hash model = H(kDebrisModels[rand() % (sizeof(kDebrisModels) / sizeof(kDebrisModels[0]))]);
	if (!STREAMING::HAS_MODEL_LOADED(model))
	{
		STREAMING::REQUEST_MODEL(model, FALSE);   // Astra: never WAIT inside an update - try again next time
		return;
	}
	if (FlightsActive() >= MaxFlights())
		return;
	float ang = RandRange(0, 2 * PI);
	float r = wallRadius() * RandRange(0.9f, 1.2f);
	V3 p = base + V3(cosf(ang) * r, sinf(ang) * r, 0);
	p.z = GroundZ(p.x, p.y, base.z + 30.0f, base.z) + 0.4f;
	Object o = OBJECT::CREATE_OBJECT(model, p.x, p.y, p.z, FALSE, FALSE, TRUE, FALSE, FALSE);
	if (!o)
		return;
	// v1.0, playtest 8 ("a trail of barrels over there that it left that are floating"): the extra debris used to be
	// spawned 1 m up with an upward kick - when the game's physics didn't take it just hung there. It's carried up the wall
	// by the mod now, thrown, and laid down where it lands, like everything else that won't move on its own.
	ownEntities.insert(o);
	debris.push_back(o);
	Lift(o, t, false);
}

// Self-healing (v0.5, reworked after Astra's audit). An emitter that ever started keeps its *intent* (wanted,
// effect name, scale) even when its handles are gone, so a failed replacement is retried with a back-off instead of
// being dropped forever. A surviving old effect is removed before a new anchor is attached. Emitters whose effect was
// refused at spawn are NOT retried (that would silently exceed the looped-effect budget).
void Tornado::HealEmitters(float t)
{
	if (t < nextHeal)
		return;
	int pending = 0, recovered = 0;
	for (auto& e : emitters)
	{
		if (e.pending)
		{
			if (t < e.retryAt || LoopsInUse() >= g_set.ptfxBudget || t - bornAt > 30.0f)
			{
				pending++;
				continue;
			}
			if (StartEmitter(e, FunnelPoint(e.hFrac, e.curAng, e.radiusMul, t)))
			{
				e.pending = false;
				e.wanted = true;
				loopsRunning++;
				AddLoopsInUse(1);
				recovered++;
			}
			else
			{
				e.retryAt = t + 0.5f;
				pending++;
			}
			continue;
		}
		if (!e.wanted || t < e.retryAt)
			continue;
		bool anchorOk = e.world || (e.anchor && ENTITY::DOES_ENTITY_EXIST(e.anchor));
		bool fxOk = e.fx && GRAPHICS::DOES_PARTICLE_FX_LOOPED_EXIST(e.fx);
		if (anchorOk && fxOk)
			continue;
		if (!anchorOk)
		{
			if (fxOk)
			{
				GRAPHICS::STOP_PARTICLE_FX_LOOPED(e.fx, FALSE);
				GRAPHICS::REMOVE_PARTICLE_FX(e.fx, FALSE);
			}
			e.fx = 0;
			if (e.anchor)
			{
				anchorSet.erase(e.anchor); ownEntities.erase(e.anchor);
				DeleteAnchor(e.anchor);   // gone already -> just unregistered; still there -> orphan retry
			}
		}
		V3 at = FunnelPoint(e.hFrac, e.curAng, e.radiusMul, t);
		if (!StartEmitter(e, at))
		{
			e.retryAt = t + 2.0f;
			healFailed++;
			if (!e.world && !e.anchor)
				Log("'%s' heal: anchor re-create failed for %s - retrying in 2 s (failures %d)", label.c_str(), e.fxName, healFailed);
			continue;
		}
		healed++;
		if (healed <= 5 || healed % 25 == 0)
			Log("'%s' healed emitter (%s, %s) - total %d", label.c_str(), e.fxName, e.world ? "world-space" : anchorOk ? "anchor ok" : "anchor re-created", healed);
	}
	if (recovered)
		Log("'%s' started %d effects the game had refused at spawn (now %d running, %d still waiting)", label.c_str(), recovered, loopsRunning, pending);
	nextHeal = t + (pending ? 0.5f : 1.0f);
}

void Tornado::CaptureHandles(std::vector<Entity>& anchors, std::vector<int>& fxs) const
{
	for (auto& e : emitters)
	{
		if (e.anchor) anchors.push_back(e.anchor);
		if (e.fx) fxs.push_back(e.fx);
	}
}

// v0.7, playtest 7: three tornadoes spawned within 12 s got 85, 24 and 1 looped effects - the third was just puffs.
// A new tornado now takes an even share: the others give back effects from the end of their list (top cloud, debris,
// ground ring, then the outer layers), never their core.
int Tornado::TrimLoopsTo(int keep)
{
	int freed = 0;
	while (loopsRunning > keep && !emitters.empty())
	{
		Emitter& e = emitters.back();
		if (e.fx && GRAPHICS::DOES_PARTICLE_FX_LOOPED_EXIST(e.fx))
		{
			GRAPHICS::STOP_PARTICLE_FX_LOOPED(e.fx, FALSE);
			GRAPHICS::REMOVE_PARTICLE_FX(e.fx, FALSE);
		}
		if (e.anchor)
		{
			anchorSet.erase(e.anchor); ownEntities.erase(e.anchor);
			DeleteAnchor(e.anchor);
		}
		if (e.wanted)
		{
			loopsRunning--;
			AddLoopsInUse(-1);
			freed++;
		}
		emitters.pop_back();
	}
	if (freed)
		Log("'%s' gave back %d looped effects to share with a new tornado (now %d)", label.c_str(), freed, loopsRunning);
	return freed;
}

// v0.7, playtest 7: "the touchdown wasn't all that impressive" / "it wants like a super dramatic entrance... a really nice
// stark swirling tornado". While it reaches down, dark smoke boils at the tip so you see it coming down; when it touches
// the ground, a ring of dust and dirt blasts outward, with a lightning strike at the base.
void Tornado::TouchdownBlast(float t)
{
	touchdownDone = true;
	float sz = sizeMul(), wall = wallRadius();
	if (opts.tiny)
	{
		// v1.4: a pocket twister lands with a little poof, not a dust explosion
		GRAPHICS::USE_PARTICLE_FX_ASSET("core");
		for (int i = 0; i < 3; i++)
			GRAPHICS::START_PARTICLE_FX_NON_LOOPED_AT_COORD("ent_dst_dust", base.x + RandRange(-0.4f, 0.4f), base.y + RandRange(-0.4f, 0.4f), base.z + 0.2f,
				0, 0, RandRange(0, 360), 0.35f, FALSE, FALSE, FALSE);
		return;
	}
	for (int i = 0; i < 16; i++)
	{
		float ang = i * 2 * PI / 16 + RandRange(-0.1f, 0.1f);
		float r = wall * RandRange(0.6f, 1.6f);
		V3 p = base + V3(cosf(ang) * r, sinf(ang) * r, 0);
		p.z = GroundZ(p.x, p.y, base.z + 20.0f, base.z) + 0.5f;
		GRAPHICS::USE_PARTICLE_FX_ASSET("core");
		PuffTint(GetStyles()[styleIdx], 0.8f);
		GRAPHICS::START_PARTICLE_FX_NON_LOOPED_AT_COORD(i % 3 == 0 ? "exp_grd_smoke_post" : (i % 3 == 1 ? "bang_dirt_dry" : "ent_dst_dust"),
			p.x, p.y, p.z, 0, 0, RandRange(0, 360), 3.5f * sqrtf(sz), FALSE, FALSE, FALSE);
	}
	if (g_set.lightning > 0 && !opts.mini && !opts.display && !IntroHoldsSky())   // (v1.7: the intro has its one flash, far off)
		MISC::FORCE_LIGHTNING_FLASH_AT_COORDS(base.x + RandRange(-8.f, 8.f), base.y + RandRange(-8.f, 8.f), base.z, -1.0f);   // -1: as in all of Rockstar's calls
	Log("'%s' TOUCHDOWN blast", label.c_str());
}

void Tornado::StopEmitters()
{
	for (auto& e : emitters)
	{
		if (e.fx && GRAPHICS::DOES_PARTICLE_FX_LOOPED_EXIST(e.fx))
		{
			GRAPHICS::STOP_PARTICLE_FX_LOOPED(e.fx, FALSE);
			GRAPHICS::REMOVE_PARTICLE_FX(e.fx, FALSE);
		}
		e.fx = 0;
		if (e.anchor)
		{
			anchorSet.erase(e.anchor); ownEntities.erase(e.anchor);
			DeleteAnchor(e.anchor);
		}
	}
	AddLoopsInUse(-loopsRunning);
	loopsRunning = loopsFailed = loopsPlanned = 0;
	emitters.clear();
}

// v0.7, playtest 7: "can I change the tornado style in real time? ... doesn't really look all that different" - it couldn't.
// Now changing Style in the menu re-dresses the tornadoes that are already out.
void Tornado::Restyle(int newStyle, int budget)
{
	if (newStyle == styleIdx || dissipating)
		return;
	StopEmitters();
	DeleteJunk();
	styleIdx = newStyle;
	label = GetStyles()[newStyle].name;
	BuildVisuals(budget);
	BuildJunk();
	Log("'%s' restyled live: %d looped effects (budget %d)", label.c_str(), loopsRunning, budget);
}

void Tornado::BeginDissipate(float t)
{
	if (dissipating) return;
	dissipating = true;
	dissipateStart = t;
	growthAtDissipate = growth;   // Astra: start retracting from wherever it is now (no snap back to the ground)
	Log("'%s' dissipating (age %.0fs, growth %.2f)", label.c_str(), Age(), growth);
}

// ======================= real map trees (v1.1) =======================
// The user, after v1.0: "we know it can rip up placed trees, can we now try real trees?". Map trees aren't entities - nothing
// can grab them - but they can be HIDDEN (CREATE_MODEL_HIDE, by model and radius) and a tree prop we spawn in the same spot
// can be torn out with the uproot the planted trees use. So:
//   1. find trunks: from the funnel's centre, rays outward 1.3 m above the ground; a hit counts only if the same ray hits at
//      the same distance 4.5 m up (vertical), rays 1.7 m to either side go past it (narrow - not a wall or a rock face), the
//      surface faces sideways (not the ground) and it isn't an entity; a third ray 8.5 m up says whether it's a tall one;
//   2. which tree is it? Only tree models whose art is LOADED right now can be standing nearby, so only those are hidden,
//      and only within 2.4 m of the trunk (a few model hides per tree, not 219);
//   3. the stand-in: the same model if it's one the game lets us spawn, else one of the same family (pine, leafy, swamp,
//      desert) and about the same height. It's torn out, rides the wall and is thrown like the planted trees.
// Every step is logged (REALTREE lines) - whether the hidden map tree really disappears is the thing to watch next playtest.
#include "trees.h"
std::vector<StandIn> g_standIns;
static int g_realTreesTotal = 0, g_treeHides = 0;
struct TreeHide { V3 p; float r; Hash model; };
static std::vector<TreeHide> g_treeHideList;
static std::vector<Object> g_standInProps;
static std::vector<int> g_loadedTreeIdx;   // kTreeModels entries whose art is loaded (refreshed every 2 s)
static float g_nextLoadedScan = 0;
int RealTreesTotal() { return g_realTreesTotal; }
int TreeHidesUsed() { return g_treeHides; }
static const int kMaxTreeHides = 600;      // a ceiling on model hides per session (the game's own limit is unknown)

Object TornadoSpawnProp(const char* model, const V3& p, bool frozen)
{
	Hash m = H(model);
	if (!STREAMING::HAS_MODEL_LOADED(m)) { STREAMING::REQUEST_MODEL(m, FALSE); return 0; }
	Object o = OBJECT::CREATE_OBJECT(m, p.x, p.y, p.z, FALSE, FALSE, TRUE, FALSE, FALSE);
	if (o) { ENTITY::FREEZE_ENTITY_POSITION(o, frozen ? TRUE : FALSE); NoteModTree(o); }
	return o;
}

void RestoreRealTrees()
{
	for (auto& h : g_treeHideList)
		ENTITY::REMOVE_MODEL_HIDE(h.p.x, h.p.y, h.p.z, h.r, h.model, FALSE);
	int props = (int)g_standInProps.size();
	for (Object& o : g_standInProps)
		DeleteObj(o);
	if (!g_treeHideList.empty() || props)
		Log("REALTREE restored %d model hides, removed %d stand-in trees", (int)g_treeHideList.size(), props);
	g_treeHideList.clear();
	g_standInProps.clear();
	g_treeHides = 0;
}

static void RefreshLoadedTrees(float t)
{
	if (t < g_nextLoadedScan) return;
	g_nextLoadedScan = t + 2.0f;
	g_loadedTreeIdx.clear();
	int n = (int)(sizeof(kTreeModels) / sizeof(kTreeModels[0]));
	for (int i = 0; i < n; i++)
		if (STREAMING::HAS_MODEL_LOADED(kTreeModels[i].hash))
			g_loadedTreeIdx.push_back(i);
}

// a ray from a to b: distance to the hit (or -1), the surface normal's z and the entity it hit
static float Ray(const V3& a, const V3& b, float* nz = nullptr, Entity* ent = nullptr)
{
	int h = SHAPETEST::START_EXPENSIVE_SYNCHRONOUS_SHAPE_TEST_LOS_PROBE(a.x, a.y, a.z, b.x, b.y, b.z, -1, PLAYER::PLAYER_PED_ID(), 0);
	BOOL hit = FALSE;
	Vector3 end = {}, normal = {};
	Entity e = 0;
	SHAPETEST::GET_SHAPE_TEST_RESULT(h, &hit, &end, &normal, &e);
	if (nz) *nz = normal.z;
	if (ent) *ent = e;
	return hit ? (V3(end) - a).len() : -1.0f;
}

static const char* TreeFamily(const char* model)
{
	static const struct { const char* key; const char* fam; } kFam[] = {
		{ "joshua", "desert" }, { "cactus", "desert" }, { "mesquite", "desert" }, { "juniper", "desert" }, { "riodel", "desert" },
		{ "cypress", "swamp" }, { "mangrove", "swamp" }, { "banyan", "swamp" }, { "magnolia", "swamp" }, { "palm", "swamp" },
		{ "fir", "pine" }, { "pine", "pine" }, { "lodgepole", "pine" }, { "cedar", "pine" }, { "redwood", "pine" },
	};
	for (auto& f : kFam) if (strstr(model, f.key)) return f.fam;
	return "leafy";
}

// The stand-ins are only loaded when a trunk needs one (and let go after), so "loaded" really means "growing round here".
static bool IsStandInModel(Hash h)
{
	for (auto& s : g_standIns) if (s.Model() == h) return true;
	return false;
}

static const StandIn* PickStandIn(float trunkHeight)
{
	if (g_standIns.empty()) return nullptr;
	// the family most of the trees growing here belong to (stand-ins of ours that happen to be loaded don't vote), nearest in height
	int votes[4] = {};
	static const char* kF[4] = { "desert", "swamp", "pine", "leafy" };
	for (int i : g_loadedTreeIdx)
	{
		if (IsStandInModel(kTreeModels[i].hash)) continue;
		for (int f = 0; f < 4; f++) if (!strcmp(TreeFamily(kTreeModels[i].name), kF[f])) votes[f]++;
	}
	int fam = 3;
	for (int f = 0; f < 4; f++) if (votes[f] > votes[fam]) fam = f;
	const StandIn* best = nullptr;
	float bd = 1e9f;
	for (auto& s : g_standIns)
	{
		float d = fabsf(s.height - trunkHeight) + (strcmp(TreeFamily(s.name.c_str()), kF[fam]) ? 25.0f : 0.0f);
		if (d < bd) { bd = d; best = &s; }
	}
	return best;
}

// Hide the map trees at a trunk, then tear out a stand-in in their place.
void Tornado::TearOutTree(const V3& trunk, float trunkDist, bool tall, const StandIn* si, float t)
{
	// the hides first, and EXCLUDING script objects - so the stand-in (one of ours, maybe the same model) can't be hidden too
	int hid = 0;
	for (int i : g_loadedTreeIdx)
	{
		if (g_treeHides >= kMaxTreeHides) break;
		ENTITY::CREATE_MODEL_HIDE_EXCLUDING_SCRIPT_OBJECTS(trunk.x, trunk.y, trunk.z, 2.4f, kTreeModels[i].hash, TRUE);
		g_treeHideList.push_back({ trunk, 2.4f, kTreeModels[i].hash });
		g_treeHides++;
		hid++;
	}
	Hash m = si->Model();
	if (hid < (int)g_loadedTreeIdx.size())
	{
		// v1.1 audit 2: the hide cap ran out partway - the real tree may still be standing, so no stand-in (it would be a twin)
		STREAMING::SET_MODEL_AS_NO_LONGER_NEEDED(m);
		Log("REALTREE '%s' hide cap reached (%d): hid %d of %d tree models here - no stand-in for this one", label.c_str(), g_treeHides, hid, (int)g_loadedTreeIdx.size());
		return;
	}
	Vector3 mn = {}, mx = {};
	MISC::GET_MODEL_DIMENSIONS(m, &mn, &mx);
	Object o = OBJECT::CREATE_OBJECT(m, trunk.x, trunk.y, trunk.z - mn.z, FALSE, FALSE, TRUE, FALSE, FALSE);
	STREAMING::SET_MODEL_AS_NO_LONGER_NEEDED(m);
	if (!o)
	{
		Log("REALTREE '%s' CREATE_OBJECT refused the stand-in %s", label.c_str(), si->name.c_str());
		return;
	}
	ENTITY::FREEZE_ENTITY_POSITION(o, TRUE);
	ENTITY::SET_ENTITY_ROTATION(o, 0, 0, RandRange(0, 360), 2, TRUE);
	g_standInProps.push_back(o);
	ownEntities.insert(o);
	realTrees++;
	g_realTreesTotal++;
	Uproot(o, t);
	Log("REALTREE '%s' trunk %.1f m out at (%.1f, %.1f, %.1f)%s: hid %d loaded tree models within 2.4 m (hides used %d), stand-in %s %.0f m",
		label.c_str(), trunkDist, trunk.x, trunk.y, trunk.z, tall ? " tall" : "", hid, g_treeHides, si->name.c_str(), si->height);
}

// v1.3: one of the map's tree models (trees.h)
static bool IsTreeModel(Hash m)
{
	static std::unordered_set<unsigned int> s;
	if (s.empty()) for (auto& tm : kTreeModels) s.insert(tm.hash);
	return s.count(m) != 0;
}
// v1.3 audit: every tree model the mod spawns (test trees, stand-ins, gallery props), so the real-tree scan never takes one
std::unordered_set<Entity> g_modTrees;
void NoteModTree(Entity e) { if (e && IsTreeModel(ENTITY::GET_ENTITY_MODEL(e))) g_modTrees.insert(e); }

void Tornado::RealTreeScan(float t)
{
	if (!g_set.mapTrees || t < nextTreeScan || !TouchedDown() || dissipating)
		return;
	nextTreeScan = t + (g_set.perf == 2 ? 0.7f : 0.4f);
	RefreshLoadedTrees(t);
	if (realTrees >= 40 || g_treeHides >= kMaxTreeHides || FlightsActive() >= MaxFlights())
		return;
	// a trunk found earlier whose stand-in was still streaming in
	if (pendTree)
	{
		const StandIn* si = nullptr;
		for (auto& s : g_standIns) if (s.name == pendModel) si = &s;
		Hash m = si ? si->Model() : Joaat(pendModel.c_str());
		if (STREAMING::HAS_MODEL_LOADED(m))
		{
			pendTree = false;
			if (si) TearOutTree(pendTrunk, pendDist, pendTall, si, t);
			else STREAMING::SET_MODEL_AS_NO_LONGER_NEEDED(m);
		}
		else if (t - pendAt > 4.0f)
		{
			pendTree = false;
			STREAMING::SET_MODEL_AS_NO_LONGER_NEEDED(m);   // v1.1 audit 2: the request isn't left behind
			Log("REALTREE '%s' the stand-in %s never streamed in - skipped a trunk", label.c_str(), pendModel.c_str());
		}
		else
			STREAMING::REQUEST_MODEL(m, FALSE);
		return;
	}
	if (g_loadedTreeIdx.empty())
		return;
	float reachT = wallRadius() * 1.7f;
	int rays = g_set.perf == 2 ? 3 : 5;
	for (int r = 0; r < rays; r++)
	{
		float ang = RandRange(0, 2 * PI);
		V3 dir(cosf(ang), sinf(ang), 0), side(-dir.y, dir.x, 0);
		float gz0 = GroundZ(base.x, base.y, base.z + 30.0f, base.z);
		V3 a = V3(base.x, base.y, gz0 + 1.3f);
		treeProbes++;
		float nz = 1;
		Entity ent = 0;
		float d = Ray(a, a + dir * reachT, &nz, &ent);
		if (d < 0) continue;
		V3 hitP = a + dir * d;
		bool seen = false;
		for (auto& q : treeDone) if ((q - hitP).len2d() < 3.0f) { seen = true; break; }
		if (seen) continue;
		auto reject = [&](const char* why)
		{
			treeRejected++;
			treeDone.push_back(hitP);
			if (treeRejected <= 8 || treeRejected % 40 == 0) Log("REALTREE '%s' rejected a hit %.1f m out: %s (rejected %d)", label.c_str(), d, why, treeRejected);
		};
		if (ent != 0)
		{
			// v1.3 (playtest 11's log: all 160 hits rejected as "an entity"): the map's trees ARE entities to the shape test. If
			// it's one of the tree models, that's a tree - and now we know exactly which model and where its trunk stands.
			Hash em = ENTITY::GET_ENTITY_MODEL(ent);
			if (!IsTreeModel(em))
			{
				char why[80];
				sprintf_s(why, "an entity that isn't a map tree (model 0x%08X)", em);
				reject(why);
				continue;
			}
			V3 tp = ENTITY::GET_ENTITY_COORDS(ent, FALSE, FALSE);
			// v1.3 audit: the mod's own trees are tree models too - a stand-in, a test tree, anything flying - and so is a map tree
			// already torn out (its hide is in place)
			bool ours = IsOrbiter(ent) || ownEntities.count(ent) || g_priority.count(ent) || g_modTrees.count(ent) || InFlight(ent)
				|| std::find(g_standInProps.begin(), g_standInProps.end(), (Object)ent) != g_standInProps.end();
			for (auto& h : g_treeHideList) if (!ours && (h.p - tp).len2d() < 2.4f) ours = true;
			if (ours) { reject("one of the mod's own trees, or one already torn out"); continue; }
			Vector3 mn = {}, mx = {};
			MISC::GET_MODEL_DIMENSIONS(em, &mn, &mx);
			bool tallTree = (mx.z - mn.z) > 9.0f;
			const StandIn* si = PickStandIn(tallTree ? 11.0f : 6.0f);
			if (!si) { reject("no stand-in tree model available"); continue; }
			treeTrunks++;
			treeDone.push_back(hitP);
			V3 trunk(tp.x, tp.y, GroundZ(tp.x, tp.y, tp.z + 3.0f, tp.z));
			Log("REALTREE '%s' found a map tree (model 0x%08X, %.0f m tall) %.1f m out", label.c_str(), em, mx.z - mn.z, d);
			if (STREAMING::HAS_MODEL_LOADED(si->Model()))
				TearOutTree(trunk, d, tallTree, si, t);
			else
			{
				STREAMING::REQUEST_MODEL(si->Model(), FALSE);
				pendTree = true; pendTrunk = trunk; pendDist = d; pendTall = tallTree; pendModel = si->name; pendAt = t;
			}
			break;
		}
		if (fabsf(nz) > 0.6f) { reject("faces up - ground or a roof"); continue; }
		float gzHit = GroundZ(hitP.x, hitP.y, hitP.z + 3.0f, hitP.z - 3.0f);
		if (hitP.z - gzHit < 0.6f) { reject("the ground"); continue; }
		float d2 = Ray(a + V3(0, 0, 3.2f), a + V3(0, 0, 3.2f) + dir * reachT);
		if (d2 < 0 || fabsf(d2 - d) > 1.8f) { reject("not vertical at 4.5 m"); continue; }
		float d3 = Ray(a + V3(0, 0, 7.2f), a + V3(0, 0, 7.2f) + dir * reachT);
		bool tall = d3 >= 0 && fabsf(d3 - d) < 2.2f;
		float l = Ray(a + side * 1.7f, a + side * 1.7f + dir * reachT), rr = Ray(a - side * 1.7f, a - side * 1.7f + dir * reachT);
		if ((l >= 0 && l < d + 1.2f) || (rr >= 0 && rr < d + 1.2f)) { reject("too wide - a wall, a rock or a building"); continue; }
		// v1.1 audit 2: a telegraph pole or a tall post passes all of that. Thin at both heights (rays 35 cm either side both
		// miss it - always true for a pole up to ~35 cm thick, never for a trunk over 70 cm) AND nothing 10.5 m up (no more
		// trunk, no branches): not a tree.
		bool thin = true;
		for (float up : { 0.0f, 3.2f })
		{
			V3 o = a + V3(0, 0, up);
			float sl = Ray(o + side * 0.35f, o + side * 0.35f + dir * reachT), sr = Ray(o - side * 0.35f, o - side * 0.35f + dir * reachT);
			if ((sl >= 0 && fabsf(sl - d) < 1.5f) || (sr >= 0 && fabsf(sr - d) < 1.5f)) thin = false;
		}
		if (thin)
		{
			bool above = false;
			V3 o = a + V3(0, 0, 9.2f);
			for (float s : { 0.0f, 1.5f, -1.5f })
				if (Ray(o + side * s, o + side * s + dir * (d + 3.0f)) >= 0) above = true;
			if (!above) { reject("thin, with nothing 10.5 m up - a pole or a post"); continue; }
		}
		// a trunk
		V3 trunk = hitP + dir * 0.45f;
		trunk.z = gzHit;
		const StandIn* si = PickStandIn(tall ? 11.0f : 6.0f);
		if (!si) { reject("no stand-in tree model available"); continue; }
		treeTrunks++;
		treeDone.push_back(hitP);
		Hash m = si->Model();
		if (STREAMING::HAS_MODEL_LOADED(m))
			TearOutTree(trunk, d, tall, si, t);
		else
		{
			STREAMING::REQUEST_MODEL(m, FALSE);
			pendTree = true; pendTrunk = trunk; pendDist = d; pendTall = tall; pendModel = si->name; pendAt = t;
		}
		break;   // one tree per pass
	}
}

// ======================= flattened grass (v1.1) =======================
// The ground it crosses: grass, bushes and long grass are flattened along its track (ADD_VEG_MODIFIER_SPHERE, type 2
// "flatten", flags grass 2 + bush 4 + long grass 256 - the kind Rockstar uses). They stay after it's gone, a trail, until
// Despawn everything; at most 80 (the oldest go first).
static std::vector<int> g_vegMods;
int VegTrailCount() { return (int)g_vegMods.size(); }
static void VegRemove(int& h)
{
	// Rockstar passes the handle by reference (research: REMOVE_VEG_MODIFIER_SPHERE(&handle, 1)) - so does this
	if (h) GRAPHICS::REMOVE_VEG_MODIFIER_SPHERE_PTR(&h, 1);
	h = 0;
}
void ClearVegTrail()
{
	for (int& h : g_vegMods) VegRemove(h);
	if (!g_vegMods.empty()) Log("flattened-grass trail cleared (%d spheres)", (int)g_vegMods.size());
	g_vegMods.clear();
}
void Tornado::FlattenTrail()
{
	if (!g_set.flatten || Mini()) return;   // v1.1 audit 2: the gun's minis would push the big one's trail out of the 80
	float step = std::max(6.0f, wallRadius() * 0.7f);
	if (vegStarted && (base - lastVeg).len2d() < step) return;
	vegStarted = true;
	lastVeg = base;
	int h = GRAPHICS::ADD_VEG_MODIFIER_SPHERE(base.x, base.y, base.z, wallRadius() * 1.15f, 2, 2 | 4 | 256, 0);
	if (!h) return;
	g_vegMods.push_back(h);
	if (g_vegMods.size() == 1 || g_vegMods.size() % 20 == 0) Log("'%s' flattening the grass (%d spheres, handle %d)", label.c_str(), (int)g_vegMods.size(), h);
	if (g_vegMods.size() > 80) { VegRemove(g_vegMods.front()); g_vegMods.erase(g_vegMods.begin()); }
}

void Tornado::Update(float dt, float t)
{
	if (dead)
		return;
	// Touchdown: reach down from the cloud over ~6 s. Dissipate: pull back up over ~6 s while fading, then remove.
	float fade = 1.0f;
	if (dissipating)
	{
		float d = Clamp((t - dissipateStart) / 6.0f, 0.0f, 1.0f);
		d = d * d * (3.0f - 2.0f * d);
		growth = growthAtDissipate * (1.0f - d);
		fade = 1.0f - d;
		if (d >= 1.0f)
		{
			Destroy();
			dead = true;
			Log("'%s' finished dissipating", label.c_str());
			return;
		}
		// Astra: v0.5 claimed a fade but never set any alpha. Fade the looped effects with the retraction.
		for (auto& e : emitters)
			if (e.fx && GRAPHICS::DOES_PARTICLE_FX_LOOPED_EXIST(e.fx))
				GRAPHICS::SET_PARTICLE_FX_LOOPED_ALPHA(e.fx, fade);
	}
	else
	{
		float g = g_set.touchdown ? Clamp((t - bornAt) / std::max(0.5f, growSeconds), 0.0f, 1.0f) : 1.0f;
		growth = g * g * (3.0f - 2.0f * g);
	}

	UpdateMovement(dt, t);
	HealEmitters(t);
	UpdateVisuals(t);
	UpdatePuffs(dt, t);
	UpdateJunk(t);
	DrawLights(t);
	if (!dissipating && Render() != 0)
	{
		if (growth < 0.98f && !opts.tiny)   // (v1.4 review: a pocket twister made a smoke cloud every shot)
		{
			// the tip boils as it reaches down
			tipAcc = std::min(tipAcc + 22.0f * PerfPuffMul() * dt, 4.0f);
			while (tipAcc >= 1.0f)
			{
				tipAcc -= 1.0f;
				V3 p = FunnelPoint(RandRange(0.0f, 0.08f), RandRange(0, 2 * PI), RandRange(0.4f, 1.0f), t);
				GRAPHICS::USE_PARTICLE_FX_ASSET("core");
				PuffTint(GetStyles()[styleIdx], 0.62f);
				GRAPHICS::START_PARTICLE_FX_NON_LOOPED_AT_COORD("exp_grd_smoke_post", p.x, p.y, p.z, 0, 0, RandRange(0, 360), 3.2f * sqrtf(sizeMul()), FALSE, FALSE, FALSE);
			}
		}
		if (!touchdownDone && TouchedDown() && g_set.touchdown)
			TouchdownBlast(t);
	}
	if (opts.display)
	{
		// v1.2 a gallery exhibit: all looks, no hands
		grabbedLastFrame = 0;
		UpdateOrbiters(dt, t);
		return;
	}
	if (t >= nextGather)
	{
		nextGather = t + 0.25f;
		GatherTargets(t);
	}
	// Astra: v0.5 grabbed at growth > .75 while the funnel was still ~28 m up. Only grab once the funnel's lowest
	// point is within 4 m of the ground, and not while it is lifting away.
	if (TouchedDown() && !dissipating)
		ApplyPhysics(dt, t);
	else
		grabbedLastFrame = 0;
	if (TouchedDown() && !dissipating && !opts.mini)
		SpawnDebris(t);
	if (TouchedDown() && !dissipating)
	{
		if (!opts.mini && !(g_set.citySafe && InBigTown(base, reachRadius()))) RealTreeScan(t);   // (v1.5: not in a town)
		FlattenTrail();
	}
	UpdateOrbiters(dt, t);
}

void Tornado::Snapshot(const char* tag)
{
	V3 pp = ENTITY::GET_ENTITY_COORDS(PLAYER::PLAYER_PED_ID(), TRUE, FALSE);
	Log("%s '%s' age %.0fs pos (%.0f, %.0f, %.1f) ground %s growth %.2f%s | player %.0fm dz %.1f%s | pool raw p%d v%d o%d (%.0f ms, empty reads %d) kept %d tracked %d in-range %d height-rej %d capped %d | reach peds %d veh %d obj %d (wall %d eye %d) grabbing %d uprooted %d flights %d | loops %d/%d healed %d heal-fail %d | anchors %d orphans %d | puffs x%.2f refused %d | hover woke %d gave up %d | pool top-ups %d | flings %d landings %d (last %.1f m/s) | orbiters %d ejected %d carried %d impacts %d glided %d natural %d | arthur %d perf %d | force %s size %d reach %d speed %d render %d engine %d style %s",
		tag, label.c_str(), NowSec() - bornAt, base.x, base.y, base.z, groundOk ? "ok" : "LOST", growth, dissipating ? " DISSIPATING" : "",
		(pp - base).len2d(), pp.z - base.z, PlayerInEye() ? " IN EYE" : "",
		rawPeds, rawVehs, rawObjs, PoolScanMs(), PoolEmptyReads(), statPool, (int)tracked.size(), statInRange, statHeightRej, statCapped,
		counts.peds, counts.vehicles, counts.objects, counts.inWall, counts.inEye, grabbedLastFrame, uprooted, FlightsActive(),
		LoopsAlive(), loopsRunning, healed, healFailed, AnchorCount(), OrphanCount(), PuffRateScale(), PuffsRefused(),
		HoverWoken(), HoverGaveUp(), PoolTopUps(), flings, GetLanding().landings, GetLanding().lastImpact,
		OrbitersAlive(), orbitersEjected, carried, ImpactsShown(), HoverGlided(), HoverNatural(), g_set.arthur, g_set.perf,
		CurrentForce().name, g_set.size, g_set.reach, g_set.speed, g_set.render, g_set.engine, GetStyles()[styleIdx].name);
}

void Tornado::Destroy()
{
	ReleaseFlights(this, NowSec());
	DeleteOrbiters();
	DeleteJunk();
	SweepAfterTornado(base, reachRadius(), NowSec());
	for (auto& e : emitters)
	{
		if (e.fx && GRAPHICS::DOES_PARTICLE_FX_LOOPED_EXIST(e.fx))
		{
			GRAPHICS::STOP_PARTICLE_FX_LOOPED(e.fx, FALSE);
			GRAPHICS::REMOVE_PARTICLE_FX(e.fx, FALSE);
		}
		e.fx = 0;
		if (e.anchor)
			DeleteAnchor(e.anchor);   // unregistered only if the game confirms deletion; otherwise kept as an orphan
	}
	AddLoopsInUse(-loopsRunning);
	loopsRunning = 0;
	emitters.clear();
	for (Object o : debris)
		if (ENTITY::DOES_ENTITY_EXIST(o))
			ENTITY::SET_ENTITY_AS_NO_LONGER_NEEDED(&o);   // wreckage stays in the world on purpose (the sweeper still watches it)
	debris.clear();
	if (blip && MAP::DOES_BLIP_EXIST(blip))
		MAP::REMOVE_BLIP(&blip);
	blip = 0;
	ownEntities.clear();
	anchorSet.clear();
	caught.clear();
	tracked.clear();
}

void ForgetWorldState()
{
	g_pool.clear();
	g_lastGood.clear();
	g_poolTime = -100.0f;
	g_poolWaitUntil = -100.0f;   // v1.6.2
	g_emptyStreak = 0;
	g_touched.clear();
	g_sweepOrder.clear();
	g_sweepPos = 0;
	g_flights.clear();
	g_priority.clear();
	g_loopsInUse = 0;
	g_wakeTimes.clear();
	g_nextTopUp = 0;
	g_orbiterSet.clear();
	g_flyers.clear();
	g_impactTimes.clear();
	g_backstopOn = false;       // the old session's ped handle is gone; CAN_BE_DAMAGED belongs to the dead/old ped
	g_backstopMount = 0;
	g_dropWindowUntil = g_playerLastInWall = g_testWindowUntil = -100.0f;
	g_fallTopH = 0;
	g_landKind = 0;              // v1.3 audit: a landing kind never carries over a reload
	g_roughRagdolled = false;
	g_skyUntil = -100.0f; g_meteorHit = false;   // v1.6
	g_modTrees.clear();
	g_scripted.clear();          // v1.1
	g_vegMods.clear();
	g_shieldPlayer = false;
	g_treeHideList.clear();      // (the game drops model hides on a reload)
	g_standInProps.clear();
	g_loadedTreeIdx.clear();
	g_treeHides = 0;
	g_nextLoadedScan = 0;
}
