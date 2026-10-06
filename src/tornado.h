// NadoTest - tornado visuals (looped particles on spinning anchors or in world space + world-space puffs), zoned
// vortex physics, scripted tree uproots, the floating-prop sweeper and Arthur's soft landing.
#pragma once
#include "common.h"
#include <unordered_map>
#include <unordered_set>

// ---------- tunables shared with the menu ----------
struct Settings
{
	int style = 1;          // index into GetStyles(). v1.3: 1 = W: Wedge (playtest 11: "rope, wedge, dark column, any of them can be our
	                        // default ... or decide for yourself" - the wedge reads best from a distance, and playtest 6 called it the best)
	int force = 1;          // 0 gentle, 1 violent, 2 extreme
	int size = 1;           // funnel look (also sets the wall where things get lifted): 0 small, 1 medium, 2 large
	int reach = 2;          // how far it pulls things in: 0 medium, 1 large, 2 huge, 3 massive (playtest 2: bigger = better)
	int speed = 1;          // v0.4, playtest 4 ("make it move a little bit slower, or an adjustable speed"): slow / normal / fast
	int movement = 2;       // 0 stationary, 1 wander, 2 toward player, 3 straight line
	int render = 2;         // 0 looped effects only, 1 puffs only, 2 both
	// v0.5: world-space is the default. Playtest 5's render check proved looped smoke on hidden props doesn't render
	// (variants 1-4: nothing / flickers) while the same smoke placed in the world does (variants 6-7: solid).
	int engine = 1;         // 0 = legacy: looped smoke rides invisible props, 1 = world-space loops moved directly
	bool eye = true;        // calm centre you can stand in
	bool touchdown = true;  // new tornadoes descend from the clouds over ~6 s before they grab anything
	bool npcFlee = true;    // people in the outer pull run away before it reaches them
	int lifetime = 0;       // 0 off, 1 = 2 min, 2 = 5 min, 3 = 10 min, then it dies down on its own
	bool dieWithArthur = true;   // v0.4, playtest 4: after Arthur died the tornado chased him into Rhodes

	bool grabPeople = true;
	bool grabAnimals = true;
	bool grabPlayer = false;     // synced from 'arthur' by the main loop (arthur > 0)
	// v1.0 default: grabbable (playtest 8: "a very very good default"). Invincible + soft landing keep it safe.
	int arthur = 2;              // 0 immune, 1 tugged (heavy, escapable), 2 grabbable, 3 easy prey
	bool grabVehicles = true;
	bool grabProps = true;
	bool ripFixedProps = true;
	bool citySafe = true;        // v1.5: in the big towns, leave the fixed street furniture alone (playtests 12-13 crashed in Saint Denis)
	bool extraDebris = true;
	bool softLanding = true;     // v0.4: slows Arthur just before the ground after the tornado drops him
	bool hoverFix = true;        // v0.4: floating props get their physics woken (v0.5: never teleported, see HoverSweep)
	bool flingChase = true;      // v0.5, playtest 5 ("let go of me... fling me out... as it keeps following you")
	float flingHoldMin = 10.0f, flingHoldMax = 18.0f;   // seconds in the wall before the throw (v0.7: sooner)
	int pushMethod = 0;          // 0 hybrid, 1 velocity, 2 force

	// v0.6, playtest 6: "we don't want to lock it to one weather... it looks fine like this" / "the rain gets kind of
	// annoying, you'd want to be able to disable it super easily". Default: leave the weather alone.
	// v1.0, playtest 8: "I really want that effect of it connecting up into the clouds and it being dark out... we can have
	// it as an override separate for the weather, so we can have a freak tornado with beautiful sunshine". Default: dark
	// storm clouds, no rain.
	int weatherMode = 1;         // 0 off (keep yours), 1 storm clouds, 2 thunderstorm, 3 rain (synced to weather/weatherType)
	bool weather = false;
	int weatherType = 0;         // index into kWeatherTypes (0 = THUNDERSTORM)
	int lightning = 2;           // 0 off, 1 rare, 2 near the funnel
	bool wind = true;
	bool camShake = true;
	bool heavyRain = false;
	bool darkTint = true;
	bool mapBlip = true;

	bool overlay = false;        // v0.5: the dev info box is part of HUD "Full" only
	bool playerGod = false;      // v1.3: off by default - landings and debris can hurt him now (Mixed landings never kill)
	bool cinematic = false;      // hide ALL mod text + game HUD (YouTube B-roll)
	bool zoneBanner = true;      // "THE EYE / THE WALL / THE PULL" banner when you're in reach (HUD "Full")
	int hud = 1;                 // v0.5, playtest 5 ("could be a little bit more subtle"): 0 Full (dev), 1 Tracker, 2 Off
	bool debrisCone = true;      // v0.6: props carried round the cone in spiral arms (the visible spin)
	bool impacts = true;         // v0.6: bursts where thrown things hit something
	bool touchdownCam = true;    // v0.6: the camera swings to show a new tornado touching down
	bool rideCam = true;         // v0.7: when it lifts Arthur, a 5 s orbit shot of the ride (playtest 7's idea)
	bool landingHurts = true;    // v0.7: a soft landing still costs a little health (playtest 7: "for a game")
	// v1.3 (playtest 11: "we want it to be able to hurl around and hurt arthur too... make sure there is some variety"):
	// 0 soft (always caught), 1 mixed (soft / rough / the stratosphere / slammed down - hurts, never kills), 2 real (no catch)
	int landings = 1;
	bool throwAtArthur = true;   // v1.3: now and then it throws something at Arthur, or near him
	bool multi = false;          // v0.7: off = a new tornado replaces the old one (playtest 7: "makes the most sense for performance")
	bool windSwirl = false;      // v0.7 experiment: the wind direction turns with the funnel, so smoke everywhere drifts round
	bool roar = true;            // v1.0: the wind howls louder the closer it gets (playtest 8: "we can hear the tornado")
	// v1.0, playtest 8: "a little bit of lag... and I'm on a 5090... we got to be conscious about the low-end PCs".
	int perf = 1;                // 0 high, 1 balanced (default), 2 low PC
	float streamerPitch = -60.0f;   // [Visuals] StreamerPitch: tilt of the tangent smoke streamers (see the Spin check)
	// v1.1
	bool mapTrees = true;        // real map trees: find trunks in its path, hide the map tree, tear out a stand-in
	int season = 0;              // Storm season: 0 off, 1 rare, 2 regular, 3 frequent - tornadoes that turn up on their own
	bool memeSounds = true;      // [Intro] MemeSounds: the intro's one dramatic "boom" (a synthesized sound, no samples)
	bool flatten = true;         // grass and bushes flattened along its track (the game's vegetation modifiers)

	// from NadoTest.ini
	int maxEntities = 150;
	float velocityGain = 2.5f;
	float forceGain = 1.0f;
	float windSpeed = 12.0f;
	std::string shakeName = "HAND_SHAKE";
	float shakeMax = 2.5f;
	int ptfxBudget = 110;
	float puffsPerSecond = 70.0f;
	float autoLogSeconds = 5.0f;
	float overlayX = 0.33f, overlayY = 0.015f;
};
extern Settings g_set;

extern const char* kWeatherTypes[];
extern const char* kWeatherLabels[];
extern const int kWeatherTypeCount;

// One shell of looped emitters. v0.4 (playtest 4 footage): the funnels that read as tornadoes were the DENSE ones
// (Supercell, Dark Column); thin (Dust Rope) and widely spread (Smoke Wedge) ones washed out against the bright storm
// sky. So styles are now built from layers: a slow, dark, dense core that gives the funnel a solid body, and a faster
// wall around it that shows the rotation.
struct FunnelLayer
{
	std::vector<const char*> fx;
	int rings, perRing;       // rings from the ground up to the cloud base, emitters per ring
	float radiusMul;          // x the funnel radius at that height (core ~0.35, wall 1.0)
	float spinMul;            // x the style's spin
	float scaleBottom, scaleTop;
	float shade;              // x the dark tint: < 1 darker (core), > 1 lighter (outer sheath) - reads as depth
	// v0.5 (playtest 5: "I wish it rotated a little bit more, so being able to see it rotate"): a uniform ring of smoke
	// looks the same at every angle, so its spin is invisible. A layer with twist > 0 is a set of HELIX BANDS instead:
	// each emitter's angle advances with height (radians from the ground to the top), so pale bands wind round the dark
	// funnel and you can watch them travel as it turns.
	float twist = 0;
	// v0.6: emitters point along the direction of spin (tilted by StreamerPitch), so directional smoke - the train
	// exhaust - is thrown round the funnel instead of straight up.
	bool tangent = false;
	// v1.1 Toon Twister: emitters in level RINGS whose height scrolls upward and wraps (stripes climbing a tube)
	bool ringBands = false;
	float scroll = 0;         // fraction of the height per second
	// v1.2: this layer keeps the effect's own colours (fire, steam, snow) even when the style is tinted
	bool natural = false;
};

struct Style
{
	const char* name;
	const char* blurb;        // menu help line
	float height, rBase, rTop, flare, spinDeg;
	std::vector<FunnelLayer> layers;
	std::vector<const char*> groundFx; int groundCount; float groundRadius, groundScale;
	std::vector<const char*> debrisFx; int debrisCount; float debrisRadius, debrisScale;
	std::vector<const char*> topFx; int topCount; float topScale;
	std::vector<const char*> puffFx; float puffScale; int helixArms; float puffDensity;
	std::vector<const char*> skirtFx;
	std::vector<const char*> wallFx;    // dust wall at the edge of the lifting zone
	float collarMul = 0.8f;             // v0.5: top emitters as a wide, slow, dark "wall cloud" ring (x the top radius)
	float collarShade = 1.0f;
	float snake = 0;                    // v0.6: a wave that travels up the funnel (the Rope's "old-timey twister" wiggle)
	float whip = 0;                     // v1.1: the tip whips round in a circle (m)
	float hop = 0;                      // v1.1: the tip hops off the ground (m) - cartoon twisters bounce
	// v1.2 (the user: "new types, animations, methods for creating tornado visually"): new ways to build a funnel
	int tint = 0;                       // 0 the dark tint (Visuals DarkTint), 1 the effects' own colours, 2 the colour below
	float tr = 1, tg = 1, tb = 1;       // tint 2: the smoke's colour
	int lights = 0;                     // glow inside the funnel: lights spiralling up the core (fire, ghost light)
	float lr = 1, lg = 0.5f, lb = 0.2f, lightRange = 16, lightPower = 6, flicker = 0;
	int vortices = 0;                   // a multi-vortex: this many thin funnels circling inside the parent
	float vortexOrbit = 0.6f;           // ...this far out (x the parent's radius)
	int junk = 0;                       // a funnel built from props: planks, barrels and crates form the wall (count at High)
};
std::vector<Style>& GetStyles();       // last entry is "E: Custom (FX Lab pick)"
void SetCustomStyleFx(const char* fx, bool looped, float scale);

struct Emitter
{
	Object anchor = 0;
	int fx = 0;
	int kind = 0;        // 0 funnel, 1 ground ring, 2 low debris, 3 top cloud
	int layer = -1;      // funnel layer index (kind 0)
	bool world = false;  // world-space loop moved with SET_PARTICLE_FX_LOOPED_OFFSETS (no anchor prop)
	float hFrac = 0;
	float angle0 = 0;
	float radiusMul = 1;
	float spinMul = 1;
	float shade = 1;
	const char* fxName = "";
	float scale = 1.0f;   // remembered so a dead effect can be restarted
	bool wanted = false;  // the effect started at least once -> heal it whenever the anchor/effect goes missing
	float retryAt = 0;    // back-off after a failed heal attempt
	bool tangent = false; // v0.6: oriented along the spin (world-space only)
	float curAng = 0;     // its current angle around the funnel (for the orientation)
	bool pending = false; // v1.0: refused at spawn (the game still busy freeing old smoke) - retried every 0.5 s
	int sub = -1;         // v1.2 multi-vortex: which sub-funnel it belongs to
};

struct Caught
{
	float ejectedUntil = -100;
	float lastRagdoll = -100;
	bool ripped = false;
	bool woken = false;
	bool fled = false;
	bool lod = false;     // v0.5: draw distance raised once it reached the wall
	float size = -1;      // largest model dimension (m), looked up once - bigger things tumble harder
	float wallSince = -1; // v0.6: when it entered the wall, and where - a prop that never moves gets carried
	V3 wallStart;
	bool carried = false;
};

struct Tracked { float lastSeen; int type; };   // type 0 ped, 1 vehicle, 2 object
struct TargetCounts { int peds = 0, vehicles = 0, objects = 0, inWall = 0, inEye = 0; };

// v0.5: real height above the ground, from a ground probe just above the point. Playtest 5 showed
// GET_ENTITY_HEIGHT_ABOVE_GROUND returns WORLD Z for many props (836 resting props were "floating" 40-70 m up).
// Returns false if the probe found nothing (then the caller must not act on it).
bool TrueHeight(const V3& p, float* out);

// Entities (planted test trees) that are always targeted and get the scripted uproot when the wall reaches them.
extern std::unordered_set<Entity> g_priority;

class Tornado;
struct StandIn;

// v0.4 scripted uproot (playtest 4: the tree "rotates kind of slowly... if it was spinning and more frantic, even that
// would kind of be funny"). We move the tree ourselves: tear out, ride the wall while tumbling, get thrown, land.
struct Flight
{
	Entity e = 0;
	Tornado* owner = nullptr;   // null once its tornado is gone (the tree is thrown from where it is)
	int phase = 0;              // 0 tearing out, 1 riding the wall, 2 thrown by us, 3 thrown to the game's physics, 4 landed
	float t0 = 0, phaseT = 0;
	V3 pos, vel;
	V3 rot, rotRate;            // pitch, roll, yaw (degrees) and their rates
	float angle = 0, radius = 0, height = 0, release = 0.6f;
	float lieHeight = 0.6f;     // how high the trunk sits when it lands on its side
	V3 physCheckFrom;
	bool tree = true;           // v0.6: false = a carried prop or a floater brought down (lands upright)
};
int FlightsActive();
int MaxFlights();

// v0.6 vortex debris: a prop the tornado carries round its cone (see UpdateOrbiters)
struct Orbiter { Object o = 0; int arm = 0; float ang = 0, h = 0, hTop = 0.8f, climb = 0.05f, rMul = 1, speedMul = 1; V3 rot, rotRate; bool big = false; };

// v1.1: per-spawn overrides (the tornado gun's mini twisters; 0 = use the menu setting)
struct SpawnOpts
{
	float size = 0;        // sizeMul override
	float heightMul = 1;   // x the style's height
	float reach = 0;       // reach override, in multiples of the wall radius
	bool mini = false;     // a mini twister: no weather, no camera, no debris cone of its own, short-lived
	float life = 0;        // seconds until it dies down by itself (0 = the Lifetime setting)
	float grow = 6.0f;     // touchdown seconds
	bool display = false;  // v1.2 the gallery: a harmless exhibit - grabs nothing, no debris, trees, grass, marker or lifetime
	int render = -1;       // v1.2 the gallery's methods room: Funnel render for this one (-1 = the menu setting)
	bool tiny = false;     // v1.4 the gun's pocket twister: person-sized (about 1.7 m), small smoke, no dust wall, no blast
};

class Tornado
{
public:
	Tornado(const V3& groundPos, float headingDeg, int styleIdx, const std::string& label, bool stationary, int loopBudget, const SpawnOpts& opts = SpawnOpts());
	~Tornado();
	void Update(float dt, float t);
	void Destroy();
	void Snapshot(const char* tag);

	V3 base;
	float heading;
	std::string label;
	int styleIdx;
	bool stationary;
	int loopsRunning = 0, loopsFailed = 0, loopsPlanned = 0;
	int grabbedLastFrame = 0;
	TargetCounts counts;
	int rawPeds = 0, rawVehs = 0, rawObjs = 0;
	bool groundOk = true;
	float wallRadius() const;    // where things orbit and get lifted (tied to the visible funnel)
	float eyeRadius() const;
	float reachRadius() const;   // how far the inflow pulls
	float height() const;
	float sizeMul() const;
	float radiusAt(float hFrac) const;
	V3 FunnelPoint(float hFrac, float angle, float radiusScale, float t) const;
	V3 MoveVel() const { return moveVel; }
	bool PlayerInEye() const;
	float Age() const { return NowSec() - bornAt; }
	void BeginDissipate(float t);      // lift back into the clouds while fading out, then remove itself
	void CaptureHandles(std::vector<Entity>& anchors, std::vector<int>& fxs) const;   // for post-cleanup verification
	bool TouchedDown() const { return (1.0f - growth) * height() < 4.0f; }            // lowest funnel point within 4 m of the ground
	int healed = 0, healFailed = 0;
	int statPool = 0, statInRange = 0, statHeightRej = 0, statCapped = 0;   // GatherTargets filter stages
	bool Dissipating() const { return dissipating; }
	bool Dead() const { return dead; }
	float growth = 1.0f;               // 0 = still up in the cloud, 1 = fully touched down
	int LoopsAlive() const;            // looped effects the game still reports as existing
	int uprooted = 0;
	int flings = 0;                    // v0.5 fling-and-chase: times it threw Arthur out
	float playerHeldSince = -1, playerHoldFor = 20, playerLastHeld = -100;
	int carried = 0;                   // v0.6: props with dead physics that it carried itself
	int orbitersEjected = 0;
	int aimedThrows = 0;           // v1.3: pieces it aimed at (or near) Arthur
	int OrbitersAlive() const;
	int OrbiterCount() const { return (int)orbiters.size(); }
	// for the offline harness: (layer, hFrac, radiusMul) of every funnel emitter
	void DebugEmitters(std::vector<V3>& out) const { for (auto& e : emitters) if (e.kind == 0) out.push_back(V3((float)e.layer, e.hFrac, e.radiusMul)); }
	int DebugSubFunnels() const { int mask = 0; for (auto& e : emitters) if (e.sub >= 0 && e.sub < 30) mask |= 1 << e.sub; int n = 0; while (mask) { n += mask & 1; mask >>= 1; } return n; }
	void CaptureOrbiters(std::vector<Entity>& out) const;
	int TrimLoopsTo(int keep);         // v0.7: give effects back so a new tornado gets a fair share
	void Restyle(int newStyle, int budget);   // v0.7: change the look of a tornado that's already out
	float tugTossAt = 0;               // v0.7: "tugged" Arthur gets a low toss every few seconds
	bool touchdownDone = false;        // v0.7: the touchdown blast has fired
	// v1.1: scenes (the intro, Storm season) drive a tornado directly
	float growSeconds = 6.0f;          // how long the touchdown takes (the intro uses a sped-up one)
	bool scripted = false;             // true: it moves at scriptVel and ignores Movement/Speed
	V3 scriptVel;
	bool natural = false;              // a Storm season tornado: wanders, drifting toward Arthur
	SpawnOpts opts;                    // v1.1 (mini twisters)
	int uid = 0;                       // v1.1: unique per tornado - a stored pointer is checked with it (addresses get reused)
	bool Mini() const { return opts.mini; }
	bool Tiny() const { return opts.tiny; }
	int LoopsOfKind(int kind) const { int n = 0; for (auto& e : emitters) if (e.kind == kind) n++; return n; }   // (v1.4: for the checks)
	int ConeProps() const { return (int)orbiters.size(); }
	float BiggestPlume() const { float m = 0; for (auto& e : emitters) m = std::max(m, e.scale); return m; }
	V3 AxisAt(float hFrac, float t) const { return FunnelPoint(hFrac, 0, 0, t); }   // the funnel's centre line (v1.4: for the checks)
	bool Display() const { return opts.display; }
	int Render() const { return opts.render >= 0 ? opts.render : g_set.render; }
	int JunkAlive() const;
	int realTrees = 0;                 // map trees it tore out (v1.1)
	int treeProbes = 0, treeTrunks = 0, treeRejected = 0;

private:
	void BuildVisuals(int loopBudget);
	void UpdateVisuals(float t);
	void UpdatePuffs(float dt, float t);
	void UpdateMovement(float dt, float t);
	void UpdateGround(float dt);
	void GatherTargets(float t);
	void ApplyPhysics(float dt, float t);
	void SpawnDebris(float t);
	void HealEmitters(float t);
	void Uproot(Entity e, float t);
	void Carry(Entity e, float t);
	void Lift(Entity e, float t, bool tree);
	bool StartEmitter(Emitter& e, const V3& at);
	void UpdateOrbiters(float dt, float t);
	void SpawnOrbiter(float t);
	void EjectOrbiter(size_t i, float t, const V3* target = nullptr);
	float nextAimedThrow = 0;          // v1.3: the next thing it throws at Arthur
	V3 spawnPos;                       // v1.3: where it touched down (a mini twister wanders round it)
	void DeleteOrbiters();
	void StopEmitters();
	void TouchdownBlast(float t);
	void RealTreeScan(float t);
	void TearOutTree(const V3& trunk, float trunkDist, bool tall, const StandIn* si, float t);
	bool pendTree = false, pendTall = false;   // a trunk waiting for its stand-in model to stream in
	V3 pendTrunk;
	float pendDist = 0, pendAt = 0;
	std::string pendModel;
	float nextTreeScan = 0;
	V3 lastVeg;
	bool vegStarted = false;
	void FlattenTrail();
	std::vector<V3> treeDone;          // trunks already handled (or rejected) - never probed twice
	float tipAcc = 0;
	std::vector<Orbiter> orbiters;
	struct Junk { Object o = 0; float h = 0, ang = 0, rMul = 1; V3 rot, rotRate; };
	std::vector<Junk> junk;            // v1.2 Junknado: the props the wall is made of
	int junkWant = 0;
	float junkRetry = 0;
	void BuildJunk();
	void TopUpJunk();
	void UpdateJunk(float t);
	void DeleteJunk();
	void DrawLights(float t);
	float nextOrbiter = 0, lastEject = -100;

	std::vector<Emitter> emitters;
	std::unordered_set<Entity> ownEntities;
	std::unordered_set<Entity> anchorSet;
	std::vector<Object> debris;
	std::unordered_map<Entity, Caught> caught;
	std::unordered_map<Entity, Tracked> tracked;
	std::vector<Entity> targets;
	float nextGather = 0, nextDebris = 0;
	float wanderTurn = 0;
	float puffAcc = 0;
	float groundLostFor = 0;
	V3 moveVel;
	V3 lean;
	Blip blip = 0;
	float bornAt = 0;
	float nextHeal = 0;
	float growthAtDissipate = 1.0f;
	bool dissipating = false, dead = false;
	float dissipateStart = 0;
};

struct ForceProfile { const char* name; float vmax, lift, ejectSpeed, tumble; };
const ForceProfile& CurrentForce();
float ReachMul();
float MoveSpeed();                  // metres per second for the current Speed setting

int LoopsInUse();
void AddLoopsInUse(int delta);
int AnchorCount();                  // registered anchor props that still exist, incl. FX Lab + orphans (leak check)
float PoolScanMs();                 // how long the last shared world scan took
int PoolEmptyReads();               // times the game's entity lists came back empty (see RefreshPool)
void PreloadTornadoAssets();        // load models/ptfx once up front so updates never wait on streaming

// ---------- world-level helpers, called from the main loop ----------
void UpdateFlights(float dt, float t);    // scripted uproots (they outlive their tornado)
void HoverSweep(float t);                 // floating props -> wake physics, or set them down
void NoteTouched(Entity e, float t);      // a prop the tornado pushed (the sweeper watches it after release)
int HoverWoken();
int HoverSetDown();
void PlayerSafety(float dt, float t);     // Arthur's soft landing
void NotePlayerInWall(float t);
void NoteModTree(Entity e);               // v1.3 audit: a tree model the mod spawned (the real-tree scan leaves it alone)
void StartDropTest(float t);              // v0.5: soft-landing window opened by the drop test (no tornado needed)
struct LandingStats { int landings = 0; float lastImpact = 0, worstImpact = 0; int healthBefore = -1, healthAfter = -1; bool active = false; };
const LandingStats& GetLanding();
int HoverGaveUp();
int HoverGlided();                        // v0.6: floaters brought down by our own gravity
int HoverNatural();                       // v0.6: "floaters" still where the tornado first found them (signs, lanterns)
int PoolTopUps();
bool IsOrbiter(Entity e);
void NoteFlyer(Entity e, int type, float t);   // v0.6: watch a thrown thing for the moment it hits something
void UpdateImpacts(float t);
int ImpactsShown();
int PerfOrbiters();
float PlayerLastInWall();                 // v1.0: for the camera's "you're in it" shake
int ArthurThrows();                       // v1.6: how many times the tornado has thrown him (the ride camera hands back on a throw)
bool InSaintDenis(const V3& p, float margin);   // v1.6.2: the intro isn't staged there
void SweepAfterTornado(const V3& base, float radius, float t);   // v0.7: bring down what it left hanging
int PerfMaxEntities();
float PerfPuffMul();
void AdaptPuffRate(float t);              // backs the puff rate off if the game starts refusing one-shot effects
float PuffRateScale();
int PuffsRefused();
// After a game reload: forget every handle without touching the game (they may belong to other entities now).
void ForgetWorldState();

// v1.1: entities a scene moves itself (the intro's cast, the balloon) - the tornado's physics, the pool and the sweeper
// leave them alone.
void SetScripted(Entity e, bool on);
bool IsScripted(Entity e);
void ClearScripted();

// v1.1 real map trees (see RealTreeScan): the models the tree scan proved we can spawn, tallest first. script.cpp fills it.
struct StandIn
{
	std::string name; float height;
	Hash hash = 0;                                                            // v1.1 audit 2: cached (BuildStandIns fills it)
	Hash Model() const { return hash ? hash : Joaat(name.c_str()); }
};
extern std::vector<StandIn> g_standIns;
int RealTreesTotal();
int TreeHidesUsed();
void RestoreRealTrees();              // undo every hide the tornadoes made (Despawn everything)
Object TornadoSpawnProp(const char* model, const V3& p, bool frozen);   // non-blocking: 0 if the model isn't loaded yet
void ClearVegTrail();                 // put the flattened grass back (Despawn everything)
int VegTrailCount();
extern bool g_shieldPlayer;           // v1.1: the intro / the balloon: the tornado leaves Arthur alone
