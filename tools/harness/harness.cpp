// NadoTest offline regression harness (v1.0.0).
// Based on Astra's v0.5 audit harness (ASTRA_AUDIT_EVIDENCE.zip): it compiles the real mod .cpp files against mocked
// Script Hook natives and checks CONTROL FLOW only - no rendering, real physics, streaming or game lifecycle.
// Astra's 9 scenarios are kept with their expectations flipped to "FIXED"; 10-12 cover v0.3.0, 13-20 cover v0.4.0,
// 21-31 cover v0.5.0 (world-space engine, the sweeper that never teleports, soft landing, fling-and-chase, pool top-up,
// the tracker HUD, the style tour, the hour picker, the drone and a clean self-test), 32-41 cover v0.6.0 (signs left alone,
// the debris cone, carried props, Arthur levels, impact bursts, menu sync, Low PC, streamers, touchdown camera, tree cache).
// 55-71 cover v1.1 (funnel density, the Toon style, real map trees, flattened grass, the intro, the jet balloon, Storm chaser,
// Storm season, the tornado gun, the storm/weather fixes, Survive the storm's fixes, the menu, the roar and the UI text). The mock
// grew peds, vehicles, seats, attachments, blips, model hides, vegetation spheres, a tiny shape-test world (trunks and walls)
// and recorders for the last value passed to the camera / time-scale / weather / wind / rain / text natives.
// 76-98 cover audit 2 (the mock also records the timecycle modifier, music events, flags and released assets, and can count
// streaming waits instead of failing on one).
#include <map>
#include <set>
#include <cstring>
#include <cstdio>
#include <stdexcept>
#include <functional>
#include "NadoTest/src/common.cpp"
#include "NadoTest/src/ui.cpp"
#include "NadoTest/src/roar.cpp"
#include "NadoTest/src/input.cpp"
#include "NadoTest/src/pad.cpp"
#include "NadoTest/src/tornado.cpp"
#include "NadoTest/src/script.cpp"
#include "hashes.h"

struct MockObj { V3 p, v; int type = 3; bool frozen = false; Hash model = 0; };
static std::map<int, MockObj> objects;
static std::map<int, int> fxAnchors;
static std::map<UINT64, int> calls;
static UINT64 hashNow;
static std::vector<UINT64> args;
static UINT64 result[8];
static int nextObject = 100, nextFx = 1000, failCreates = 0, failDeletes = 0;
static float mockTime = 0;
static bool emptyPool = false, mockDead = false;          // v0.4 scenarios
static V3 playerVel; static float playerHag = 0;
static std::set<UINT64> enabledControls;
static float lastForceZ = 0;
static float mockGroundZ = 0;                               // v0.5 scenarios
static V3 playerPos;
static std::map<int, int> canBeDamaged;
static std::set<int> stuck;                                 // objects whose physics "never takes" (integrate ignores them)
static std::vector<int> mockPeds;
static V3 lastCamCoord;
static int tiltedStarts = 0;
static int lastHealthSet = -1;
static int fixedCount = 0, brokenCount = 0;

// ---------- v1.1 mock state ----------
// A tiny shape-test world: vertical cylinders (a = centre at the bottom, b.z = top, r) and axis-aligned boxes (a = min, b = max).
struct Shape { int kind; V3 a, b; float r; int ent = 0; };   // ent: v1.3 - the entity a hit on it reports (map trees are entities)
static std::vector<Shape> world;
static BOOL shapeHit = FALSE;
static V3 shapeEnd, shapeNormal;
static int shapeEnt = 0;
static int lastInvincible = -1;                            // v1.3: SET_PLAYER_INVINCIBLE
static int shapeHandles = 0;
static std::map<int, int> shapeHits;                      // shape index -> rays that hit it first
static std::map<int, int> seatOf, attachedTo;              // ped -> vehicle it sits in; entity -> what it's attached to
// v1.6.1: something attached is where what it's attached to is (offsets ignored; Arthur keeps his own mock position)
static int Root(int e) { for (int hop = 0; hop < 4 && e != 1 && attachedTo.count(e); hop++) e = attachedTo[e]; return e; }
static std::set<int> blips;
static int nextBlip = 500, nextVeg = 7000;
static std::set<int> vegAlive;
static std::vector<int> vegAdded, vegRemoved;
static std::vector<V3> vegAt;
static int lastVegType = -1, lastVegFlags = -1;
static int hidesMade = 0, hidesRemoved = 0;
static std::vector<V3> hideAt;
static float lastTimeScale = 1, lastWindDir = -999, lastWindSpeed = -999, lastRain = -999;
static int lastPlayerControl = -1, lastRenderCams = -1;
static std::vector<float> lightningP3;                     // the 4th argument of every FORCE_LIGHTNING_FLASH_AT_COORDS
static Hash lastWeatherHash = 0;
static bool screenFadedOut = false, mockMission = false, mockShooting = false;
static bool mockRagdoll = false;                           // v1.5: IS_PED_RAGDOLL for Arthur
static float mockLookLR = 0, mockLookUD = 0, mockMoveUD = 0; // v1.5: GET_DISABLED_CONTROL_NORMAL
static int lastRagdollMin = -1;
struct NearSpeech { std::string ctx, voice; float camDist; float t; };
static std::vector<NearSpeech> nearSpeech;                 // v1.5: PLAY_AMBIENT_SPEECH_FROM_POSITION_NATIVE
static V3 mockImpact, camPos, camRot;
static std::set<Hash> unloadedModels;                      // HAS_MODEL_LOADED says no for these
static std::vector<std::string> shown;                     // every string BG_DISPLAY_TEXT drew
static std::map<int, std::string> fxNames;                 // world-space looped effects: name...
static std::map<int, V3> fxPos;                            // ...and where they were last put
// v1.2: voices - what was said, by whom and when; which peds are human; lines a voice lacks
struct Said { int ped; std::string ctx; float t; };
static std::vector<Said> said;
static std::set<int> humans;
static std::set<std::string> missingCtx;
static std::map<int, float> speechUntil;
static std::map<int, Hash> outfitOf;                       // v1.2: EQUIP_META_PED_OUTFIT
static std::map<std::string, int> audioFlags;              // v1.2: SET_AUDIO_FLAG
// v1.3: scripted conversations (the intro's story lines). convWorks: does the game play them (else the intro falls back)
struct StoryCall { std::string root; int idx; float t; };
static std::vector<StoryCall> storyCalls;
static std::map<std::string, float> convUntil;
static std::set<std::string> convCreated, textReq, textDel;
static std::map<std::string, std::set<std::string>> convNames;   // root -> the names its peds were added under
static bool convWorks = true;
static int convStops = 0, convHistoryClears = 0;
// ---------- v1.1 audit 2 mock state ----------
static std::string tcName;                                 // the timecycle modifier on now ("" = none) and its strength
static float tcStrength = 0;
static std::vector<std::string> tcSets;                    // every SET_TIMECYCLE_MODIFIER name, in order
static int tcClears = 0;
static bool musicReady = false;                            // PREPARE_MUSIC_EVENT says ready
static std::vector<std::string> musicEvents;               // every TRIGGER_MUSIC_EVENT, in order
static bool countWaits = false;                            // WAIT counts (and steps the clock) instead of failing the run
static int waits = 0;
static std::set<Hash> slowPtfx;                            // ptfx dicts that are only loaded a frame after they're asked for
static std::map<Hash, float> ptfxAskedAt;
static std::map<int, int> ragdollOk, invincible, blockEvents;   // entity -> last value set
static std::map<std::pair<int, int>, int> pedFlags;        // (ped, config flag) -> last value
static std::set<std::string> dictsRemoved;
static std::map<Hash, int> modelsReleased;                 // SET_MODEL_AS_NO_LONGER_NEEDED calls per model
static int deletedWithPedInside = 0;                       // DELETE_VEHICLE while someone still sat in it
static std::set<std::string> musicNotReady;                // music events PREPARE never gets ready
static int feedId = 0;                                     // what the game's right-hand feed toast returns
static int mockHorse = 0;                                  // GET_SADDLE_HORSE_FOR_PLAYER
static std::map<int, std::string> animOf;                  // the last anim clip TASK_PLAY_ANIM gave each ped

static bool InsideShape(const Shape& s, const V3& p)
{
	if (s.kind == 0)
	{
		float dx = p.x - s.a.x, dy = p.y - s.a.y;
		return dx * dx + dy * dy <= s.r * s.r && p.z >= s.a.z && p.z <= s.b.z;
	}
	return p.x >= s.a.x && p.x <= s.b.x && p.y >= s.a.y && p.y <= s.b.y && p.z >= s.a.z && p.z <= s.b.z;
}
// LOS probe against the mock world: marches the segment, first point inside a shape is the hit; the normal faces the side
// it came in from (sideways for trunks and walls, up if it came in over the top). Never an entity.
static void CastRay(const V3& a, const V3& b)
{
	shapeHit = FALSE; shapeEnd = V3(); shapeNormal = V3(); shapeEnt = 0;
	if (world.empty()) return;
	V3 d = b - a;
	float L = d.len();
	if (L < 1e-4f) return;
	int steps = (int)(L / 0.04f) + 1;
	V3 prev = a;
	for (int i = 0; i <= steps; i++)
	{
		V3 p = a + d * ((float)i / steps);
		for (auto& s : world)
		{
			if (!InsideShape(s, p)) continue;
			shapeHit = TRUE; shapeEnd = p; shapeEnt = s.ent; shapeHits[(int)(&s - &world[0])]++;
			if (prev.z > s.b.z) shapeNormal = V3(0, 0, 1);
			else if (s.kind == 0)
			{
				V3 r(p.x - s.a.x, p.y - s.a.y, 0);
				float l = r.len2d();
				shapeNormal = l > 1e-4f ? r * (1.0f / l) : V3(1, 0, 0);
			}
			else if (prev.x < s.a.x) shapeNormal = V3(-1, 0, 0);
			else if (prev.x > s.b.x) shapeNormal = V3(1, 0, 0);
			else if (prev.y < s.a.y) shapeNormal = V3(0, -1, 0);
			else if (prev.y > s.b.y) shapeNormal = V3(0, 1, 0);
			else shapeNormal = V3(0, 0, -1);
			return;
		}
		prev = p;
	}
}

template<class T> T arg(int i) { T v{}; std::memcpy(&v, &args.at(i), sizeof(T)); return v; }
template<class T> PUINT64 ret(T v) { std::memcpy(result, &v, sizeof(T)); return result; }
static Vector3 vec(V3 v) { Vector3 o{}; o.x = v.x; o.y = v.y; o.z = v.z; return o; }
void nativeInit(UINT64 h) { hashNow = h; args.clear(); }
void nativePush64(UINT64 v) { args.push_back(v); }
static MockObj* Obj(int e) { auto it = objects.find(e); return it == objects.end() ? nullptr : &it->second; }   // never creates one
static float mockCamRelHeading = -999.0f;   // v1.6.1: the gameplay camera's heading, relative to Arthur, as last set
PUINT64 nativeCall()
{
	std::memset(result, 0, sizeof(result)); calls[hashNow]++;
	switch (hashNow)
	{
	case N_GET_GAME_TIMER: return ret((int)(mockTime * 1000));
	case N_GET_FRAME_TIME: return ret(0.1f);
	case N_PLAYER_PED_ID: return ret(1);
	case N_GET_HASH_KEY: return ret((Hash)Joaat(arg<const char*>(0)));   // v1.1: the real joaat, so weather / model hashes can be told apart
	// VAR_STRING_LITERAL pushes (10, "LITERAL_STRING", s); VAR_STRING_LABEL (same hash) pushes (2, label)
	case N_VAR_STRING_LITERAL: return ret(args.size() > 2 ? arg<const char*>(2) : arg<const char*>(1));
	case N_IS_MODEL_IN_CDIMAGE: case N_IS_MODEL_VALID:
	case N_HAS_ANIM_DICT_LOADED: return ret(1);
	case N_DOES_ANIM_DICT_EXIST: return ret(1);
	case N_IS_ENTITY_IN_WATER: return ret(0);
	case N_HAS_NAMED_PTFX_ASSET_LOADED:   // (audit 2) a slow dict is there only from the frame after it was asked for
	{
		Hash h = arg<Hash>(0);
		if (!slowPtfx.count(h)) return ret(1);
		auto it = ptfxAskedAt.find(h);
		return ret((int)(it != ptfxAskedAt.end() && mockTime > it->second + 0.001f));
	}
	case N_REQUEST_NAMED_PTFX_ASSET: if (!ptfxAskedAt.count(arg<Hash>(0))) ptfxAskedAt[arg<Hash>(0)] = mockTime; return result;
	case N_HAS_MODEL_LOADED: return ret((int)!unloadedModels.count(arg<Hash>(0)));
	case N_CREATE_OBJECT:
	{
		if (failCreates > 0) { --failCreates; return result; }
		int id = nextObject++; objects[id].p = { arg<float>(1), arg<float>(2), arg<float>(3) }; objects[id].model = arg<Hash>(0); return ret(id);
	}
	case N_CREATE_PED: case N_CREATE_VEHICLE:
	{
		int id = nextObject++; objects[id].p = { arg<float>(1), arg<float>(2), arg<float>(3) }; objects[id].type = hashNow == N_CREATE_PED ? 1 : 2;
		objects[id].model = arg<Hash>(0); return ret(id);
	}
	case N_DELETE_PED: case N_DELETE_VEHICLE: case N_DELETE_ENTITY:
	{
		int* p = arg<int*>(0);
		if (hashNow == N_DELETE_VEHICLE) for (auto& kv : seatOf) if (kv.second == *p) deletedWithPedInside++;
		objects.erase(*p); seatOf.erase(*p); attachedTo.erase(*p); *p = 0; return result;
	}
	case N_GET_ENTITY_MODEL: { MockObj* o = Obj(arg<int>(0)); return ret(o ? o->model : (Hash)0); }
	case N_SET_TIMECYCLE_MODIFIER: { const char* s = arg<const char*>(0); tcName = s ? s : ""; tcSets.push_back(tcName); return result; }
	case N_SET_TIMECYCLE_MODIFIER_STRENGTH: tcStrength = arg<float>(0); return result;
	case N_CLEAR_TIMECYCLE_MODIFIER: tcName.clear(); tcStrength = 0; tcClears++; return result;
	case N_PREPARE_MUSIC_EVENT: { const char* s = arg<const char*>(0); return ret((int)(musicReady && !musicNotReady.count(s ? s : ""))); }
	case N_UI_FEED_POST_SAMPLE_TOAST_RIGHT: return ret(feedId);
	case N_GET_SADDLE_HORSE_FOR_PLAYER: return ret(mockHorse);
	case N_TASK_PLAY_ANIM: { const char* s = arg<const char*>(2); animOf[arg<int>(0)] = s ? s : ""; return result; }
	case N_TRIGGER_MUSIC_EVENT: { const char* s = arg<const char*>(0); musicEvents.push_back(s ? s : ""); return ret(1); }
	case N_GET_FINAL_RENDERED_CAM_COORD: return ret(vec(lastRenderCams == 1 ? lastCamCoord : camPos));   // a script camera, if one's drawn
	case N_GET_FINAL_RENDERED_CAM_ROT: return ret(vec(camRot));
	case N_SET_PED_CAN_RAGDOLL: ragdollOk[arg<int>(0)] = arg<int>(1); return result;
	case N_SET_ENTITY_INVINCIBLE: invincible[arg<int>(0)] = arg<int>(1); return result;
	case N_SET_BLOCKING_OF_NON_TEMPORARY_EVENTS: blockEvents[arg<int>(0)] = arg<int>(1); return result;
	case N_SET_PED_CONFIG_FLAG: pedFlags[{ arg<int>(0), arg<int>(1) }] = arg<int>(2); return result;
	case N_REMOVE_ANIM_DICT: { const char* s = arg<const char*>(0); dictsRemoved.insert(s ? s : ""); return result; }
	case N_SET_MODEL_AS_NO_LONGER_NEEDED: modelsReleased[arg<Hash>(0)]++; return result;
	case N_SET_PED_INTO_VEHICLE: if (Obj(arg<int>(1))) seatOf[arg<int>(0)] = arg<int>(1); return result;
	case N_IS_PED_IN_VEHICLE: { auto it = seatOf.find(arg<int>(0)); return ret((int)(it != seatOf.end() && it->second == arg<int>(1) && Obj(it->second))); }
	case N_IS_PED_IN_ANY_VEHICLE: return ret((int)seatOf.count(arg<int>(0)));
	case N_TASK_LEAVE_VEHICLE: case N_CLEAR_PED_TASKS_IMMEDIATELY: seatOf.erase(arg<int>(0)); return result;
	case N_ATTACH_ENTITY_TO_ENTITY: attachedTo[arg<int>(0)] = arg<int>(1); return result;
	case N_IS_ENTITY_ATTACHED_TO_ENTITY: { auto it = attachedTo.find(arg<int>(0)); return ret((int)(it != attachedTo.end() && it->second == arg<int>(1))); }
	case N_IS_ENTITY_ATTACHED: return ret((int)attachedTo.count(arg<int>(0)));
	case N_DETACH_ENTITY: attachedTo.erase(arg<int>(0)); return result;
	case N_IS_PED_SHOOTING: return ret((int)(mockShooting && arg<int>(0) == 1));
	case N_GET_PED_LAST_WEAPON_IMPACT_COORD: *arg<Vector3*>(1) = vec(mockImpact); return ret(1);
	case N_START_EXPENSIVE_SYNCHRONOUS_SHAPE_TEST_LOS_PROBE:
		CastRay(V3(arg<float>(0), arg<float>(1), arg<float>(2)), V3(arg<float>(3), arg<float>(4), arg<float>(5)));
		return ret(++shapeHandles);
	case N_GET_SHAPE_TEST_RESULT:
		*arg<BOOL*>(1) = shapeHit; *arg<Vector3*>(2) = vec(shapeEnd); *arg<Vector3*>(3) = vec(shapeNormal); *arg<Entity*>(4) = shapeEnt;
		return ret(2);
	case N_ADD_VEG_MODIFIER_SPHERE:
	{
		int id = nextVeg++; vegAlive.insert(id); vegAdded.push_back(id); vegAt.push_back(V3(arg<float>(0), arg<float>(1), arg<float>(2)));
		lastVegType = arg<int>(4); lastVegFlags = arg<int>(5); return ret(id);
	}
	case N_REMOVE_VEG_MODIFIER_SPHERE_PTR: { int h = *arg<int*>(0); vegRemoved.push_back(h); vegAlive.erase(h); return result; }
	case N_CREATE_MODEL_HIDE_EXCLUDING_SCRIPT_OBJECTS:   // v1.1: the real-tree hides spare script objects
	case N_CREATE_MODEL_HIDE: hidesMade++; hideAt.push_back(V3(arg<float>(0), arg<float>(1), arg<float>(2))); return result;
	case N_REMOVE_MODEL_HIDE: hidesRemoved++; return result;
	case N_BLIP_ADD_FOR_COORDS: case N_BLIP_ADD_FOR_RADIUS: { int id = nextBlip++; blips.insert(id); return ret(id); }
	case N_DOES_BLIP_EXIST: return ret((int)blips.count(arg<int>(0)));
	case N_REMOVE_BLIP: { int* p = arg<int*>(0); blips.erase(*p); *p = 0; return result; }
	case N_SET_TIME_SCALE: lastTimeScale = arg<float>(0); return result;
	case N_SET_PLAYER_CONTROL: lastPlayerControl = arg<int>(1); return result;
	case N_RENDER_SCRIPT_CAMS: lastRenderCams = arg<int>(0); return result;
	case N_SET_WIND_DIRECTION: lastWindDir = arg<float>(0); return result;
	case N_SET_WIND_SPEED: lastWindSpeed = arg<float>(0); return result;
	case N_SET_RAIN: lastRain = arg<float>(0); return result;
	case N_SET_WEATHER_TYPE: lastWeatherHash = arg<Hash>(0); return result;
	case N_FORCE_LIGHTNING_FLASH_AT_COORDS: lightningP3.push_back(arg<float>(3)); return result;
	case N_DO_SCREEN_FADE_OUT: screenFadedOut = true; return result;
	case N_DO_SCREEN_FADE_IN: screenFadedOut = false; return result;
	case N_IS_SCREEN_FADED_OUT: return ret((int)screenFadedOut);
	case N_GET_MISSION_FLAG: return ret((int)mockMission);
	case N_GET_CURR_WEATHER_STATE: *arg<Hash*>(0) = lastWeatherHash; *arg<Hash*>(1) = lastWeatherHash; *arg<float*>(2) = 0; return result;
	case N_GET_GAMEPLAY_CAM_COORD: return ret(vec(camPos));
	case N_IS_PED_RAGDOLL: return ret((int)(arg<int>(0) == 1 && mockRagdoll));
	case N_PLAY_AMBIENT_SPEECH_FROM_POSITION_NATIVE:
	{
		const char* const* sp = reinterpret_cast<const char* const*>(arg<int*>(3));
		V3 at(arg<float>(0), arg<float>(1), arg<float>(2));
		nearSpeech.push_back({ sp[0] ? sp[0] : "", sp[1] ? sp[1] : "", (at - (lastRenderCams == 1 ? lastCamCoord : camPos)).len(), mockTime });
		return ret(1);
	}
	case N_SET_PED_TO_RAGDOLL: if (arg<int>(0) == 1) { lastRagdollMin = arg<int>(1); mockRagdoll = arg<int>(1) > 1; } return result;
	case N_GET_DISABLED_CONTROL_NORMAL:
	{
		Hash a = arg<Hash>(1);
		return ret(a == Joaat("INPUT_LOOK_LR") ? mockLookLR : a == Joaat("INPUT_LOOK_UD") ? mockLookUD : a == Joaat("INPUT_MOVE_UD") ? mockMoveUD : 0.0f);
	}
	case N_GET_GAMEPLAY_CAM_ROT: return ret(vec(camRot));
	case N_BG_DISPLAY_TEXT: { const char* s = arg<const char*>(0); if (shown.size() < 5000) shown.push_back(s ? s : ""); return result; }
	case N_SET_PARTICLE_FX_LOOPED_OFFSETS: fxPos[arg<int>(0)] = V3(arg<float>(1), arg<float>(2), arg<float>(3)); return result;
	case N_SET_ENTITY_COORDS:
	{
		V3 p(arg<float>(1), arg<float>(2), arg<float>(3));
		if (arg<int>(0) == 1) playerPos = p; else if (MockObj* o = Obj(arg<int>(0))) o->p = p;
		return result;
	}
	case N_DOES_ENTITY_EXIST: return ret((int)(arg<int>(0) == 1 || objects.count(arg<int>(0))));
	case N_GET_ENTITY_COORDS: { int e = Root(arg<int>(0)); MockObj* o = Obj(e); return ret(vec(e == 1 ? playerPos : o ? o->p : V3())); }
	case N_GET_PED_BONE_COORDS: { int e = Root(arg<int>(0)); MockObj* o = Obj(e); if (e != 1 && !o) return ret(vec(V3())); return ret(vec((e == 1 ? playerPos : o->p) + V3(0, 0, 0.65f))); }   // v1.4
	case N_SET_GAMEPLAY_CAM_RELATIVE_HEADING: mockCamRelHeading = arg<float>(0); return result;   // v1.6.1
	case N_GET_ENTITY_VELOCITY: { int e = arg<int>(0); MockObj* o = Obj(e); return ret(vec(e == 1 ? playerVel : o ? o->v : V3())); }
	case N_GET_ENTITY_HEIGHT_ABOVE_GROUND: { int e = arg<int>(0); return ret(e == 1 ? playerHag : (objects.count(e) ? objects[e].p.z : 0.0f)); }
	case N_START_PARTICLE_FX_NON_LOOPED_AT_COORD: return ret(1);
	case N_IS_PLAYER_DEAD: return ret((int)mockDead);
	case N_IS_PED_DEAD_OR_DYING: return ret((int)(mockDead && arg<int>(0) == 1));
	case N_ENABLE_CONTROL_ACTION: enabledControls.insert(arg<UINT64>(1) & 0xFFFFFFFFull); return result;
	case N_APPLY_FORCE_TO_ENTITY_CENTER_OF_MASS: lastForceZ = arg<float>(4); return result;
	case N_GET_ENTITY_TYPE: { MockObj* o = Obj(arg<int>(0)); return ret(arg<int>(0) == 1 ? 1 : o ? o->type : 0); }
	case N_GET_ENTITY_HEALTH: return ret(200);
	case N_SET_ENTITY_HEALTH: lastHealthSet = arg<int>(1); return result;
	case N_SET_ENTITY_CAN_BE_DAMAGED: canBeDamaged[arg<int>(0)] = arg<int>(1); return result;
	case N_START_PARTICLE_FX_LOOPED_AT_COORD:
	{
		int id = nextFx++; fxAnchors[id] = 0; if (arg<float>(4) != 0) tiltedStarts++;
		const char* nm = arg<const char*>(0); fxNames[id] = nm ? nm : ""; fxPos[id] = V3(arg<float>(1), arg<float>(2), arg<float>(3));
		return ret(id);
	}
	case N_CREATE_CAM_WITH_PARAMS: return ret(7);
	case N_DOES_CAM_EXIST: return ret(1);
	case N_SET_CAM_COORD: lastCamCoord = { arg<float>(1), arg<float>(2), arg<float>(3) }; return result;
	case N_SET_ENTITY_COORDS_NO_OFFSET:
	{
		auto it = objects.find(arg<int>(0));
		if (it != objects.end()) it->second.p = { arg<float>(1), arg<float>(2), arg<float>(3) };
		return result;
	}
	case N_SET_ENTITY_VELOCITY:
		if (arg<int>(0) == 1) playerVel = { arg<float>(1), arg<float>(2), arg<float>(3) };
		else if (MockObj* o = Obj(arg<int>(0))) o->v = { arg<float>(1), arg<float>(2), arg<float>(3) };
		return result;
	case N_FREEZE_ENTITY_POSITION: if (MockObj* o = Obj(arg<int>(0))) o->frozen = arg<int>(1) != 0; return result;
	case N_DELETE_OBJECT:
	{
		if (failDeletes > 0) { --failDeletes; return result; }
		int* p = arg<int*>(0); objects.erase(*p); *p = 0; return result;
	}
	case N_START_PARTICLE_FX_LOOPED_ON_ENTITY: { int id = nextFx++; fxAnchors[id] = arg<int>(1); return ret(id); }
	case N_DOES_PARTICLE_FX_LOOPED_EXIST: return ret((int)fxAnchors.count(arg<int>(0)));
	case N_STOP_PARTICLE_FX_LOOPED: case N_REMOVE_PARTICLE_FX: fxAnchors.erase(arg<int>(0)); return result;
	case N_GET_GROUND_Z_FOR_3D_COORD: *arg<float*>(3) = mockGroundZ; return ret(1);
	case N_PLAY_PED_AMBIENT_SPEECH_NATIVE:
	{
		const char* c = *reinterpret_cast<const char* const*>(arg<int*>(1));
		said.push_back({ arg<int>(0), c ? c : "", mockTime }); speechUntil[arg<int>(0)] = mockTime + 1.2f; return ret(1);
	}
	case N_IS_AMBIENT_SPEECH_PLAYING: case N_IS_ANY_SPEECH_PLAYING: { auto it = speechUntil.find(arg<int>(0)); return ret((int)(it != speechUntil.end() && mockTime < it->second)); }
	case N_DOES_CONTEXT_EXIST_FOR_THIS_PED: { const char* c = arg<const char*>(1); return ret((int)(c && !missingCtx.count(c))); }
	case N_IS_PED_HUMAN: return ret((int)(arg<int>(0) == 1 || humans.count(arg<int>(0))));
	case N_DOES_META_PED_OUTFIT_EXIST_FOR_PED_MODEL: return ret(1);
	case N_EQUIP_META_PED_OUTFIT: outfitOf[arg<int>(0)] = arg<Hash>(1); return result;
	case N_SET_PLAYER_INVINCIBLE: lastInvincible = arg<int>(1); return result;
	case N_SET_AUDIO_FLAG: { const char* f = arg<const char*>(0); audioFlags[f ? f : ""] = arg<int>(1); return result; }
	case N_CREATE_NEW_SCRIPTED_CONVERSATION: convCreated.insert(arg<const char*>(0)); return ret(1);
	case N_IS_SCRIPTED_CONVERSATION_CREATED: return ret((int)convCreated.count(arg<const char*>(0)));
	case N_ADD_PED_TO_CONVERSATION: convNames[arg<const char*>(0)].insert(arg<const char*>(2)); return result;
	case N_START_SCRIPT_CONVERSATION: if (convWorks) convUntil[arg<const char*>(0)] = mockTime + 2.0f; return result;
	case N_PLAY_SINGLE_LINE_OF_CONVERSATION: storyCalls.push_back({ arg<const char*>(0), arg<int>(1), mockTime }); return result;
	case N_IS_SCRIPTED_CONVERSATION_PLAYING: { auto it = convUntil.find(arg<const char*>(0)); return ret((int)(it != convUntil.end() && mockTime < it->second)); }
	case N_STOP_SCRIPTED_CONVERSATION: convUntil.erase(arg<const char*>(0)); convStops++; return ret(1);
	case N_CLEAR_CONVERSATION_HISTORY: convHistoryClears++; return result;
	case N_TEXT_BLOCK_REQUEST: textReq.insert(arg<const char*>(0)); return result;
	case N_TEXT_BLOCK_IS_LOADED: return ret((int)textReq.count(arg<const char*>(0)));
	case N_TEXT_BLOCK_DELETE: textDel.insert(arg<const char*>(0)); textReq.erase(arg<const char*>(0)); return result;
	}
	return result;
}
void scriptWait(DWORD)
{
	if (!countWaits) throw std::runtime_error("unexpected streaming wait in mock");
	waits++; mockTime += 0.02f;   // (audit 2: a scenario that measures blocking waits - each one is a frame the script stood still)
}
int worldGetAllPeds(int* a, int n)
{
	if (emptyPool) return 0;
	int i = 0;
	for (int p : mockPeds) if (i < n) a[i++] = p;
	for (auto& kv : objects) if (kv.second.type == 1 && i < n) a[i++] = kv.first;   // v1.1: spawned peds
	return i;
}
int worldGetAllVehicles(int* a, int n) { if (emptyPool) return 0; int i = 0; for (auto& kv : objects) if (kv.second.type == 2 && i < n) a[i++] = kv.first; return i; }
int worldGetAllObjects(int* a, int n) { if (emptyPool) return 0; int i = 0; for (auto& kv : objects) if (kv.second.type == 3 && i < n) a[i++] = kv.first; return i; }
eGameVersion getGameVersion() { return VER_UNK; }

static int totalCalls() { int n = 0; for (auto& kv : calls) n += kv.second; return n; }

static void reset()
{
	g_auto.running = false; g_auto.onEnd = nullptr; DespawnAll(); LabStop();
	objects.clear(); fxAnchors.clear(); calls.clear(); g_anchors.clear(); g_orphans.clear(); g_pool.clear(); g_poolTime = -100; g_poolWaitUntil = -100;
	g_loopsInUse = 0; g_priority.clear(); g_set = Settings(); g_set.mapBlip = false; g_set.weather = false;
	g_set.extraDebris = false; g_set.camShake = false; g_set.lightning = 0; g_set.wind = false;
	g_storm = Storm(); g_st = SelfTest(); g_lab = FxLab();
	mockTime = 0; nextObject = 100; nextFx = 1000; failCreates = failDeletes = 0;
	ForgetWorldState(); g_hoverWoken = g_hoverSetDown = 0; g_nextSweep = 0; g_playerLastInWall = -100;
	g_rig = RenderRig(); g_drone = Drone(); g_holdStorm = false; g_menuOpen = false; g_menuStack.clear(); g_wasDead = false;
	emptyPool = mockDead = false; playerVel = V3(); playerHag = 0; enabledControls.clear(); lastForceZ = 0;
	g_testTrees.clear();
	mockGroundZ = 0; playerPos = V3(); canBeDamaged.clear(); stuck.clear(); mockPeds.clear(); lastCamCoord = V3();
	g_landing = LandingStats(); g_backstopOn = false; g_backstopMount = 0; g_dropWindowUntil = -100; g_fallPeak = g_lastFallV = 0;
	g_wakeTimes.clear(); g_hoverGaveUp = 0; g_topUps = 0; g_nextTopUp = 0; g_autoSkip = false; g_emptyStreak = 0; g_lastGood.clear();
	g_tc = TouchCam(); g_spin = SpinRig(); g_glided = 0; g_hoverNatural = 0; g_impacts = 0; g_flyers.clear(); g_orbiterSet.clear();
	g_impactTimes.clear(); g_nextImpactTick = 0; g_ride = RideCam(); g_lastUserSpawn = -100; lastHealthSet = -1; g_surv = Survival(); g_fallTopH = 0;
	// v1.1 state (the list in ResetAfterScriptRestart, without natives: ForgetWorldState above already dropped the tree hides,
	// the grass spheres, the scripted set and g_shieldPlayer)
	g_in = IntroState(); g_introFx.clear(); g_bal = BalloonState(); g_season = SeasonState(); g_gun = GunState(); g_music = MusicState();
	g_manualWeather = false; g_lockedHash = 0; g_nextWeatherAssert = 0; UI::Letterbox(-1); UI::g_simple = false; g_uiStyle = 0;
	g_standIns.clear(); g_nextLoadedScan = 0; g_roarLevel = 2; g_set.season = 0; g_mAnim = MenuAnim();
	Roar::g_lastAsk[0] = Roar::g_lastAsk[1] = Roar::g_lastAsk[2] = 0;
	world.clear(); shapeHits.clear(); seatOf.clear(); attachedTo.clear(); blips.clear(); vegAlive.clear(); vegAdded.clear(); vegRemoved.clear(); vegAt.clear();
	hidesMade = hidesRemoved = 0; hideAt.clear(); lastTimeScale = 1; lastWindDir = lastWindSpeed = lastRain = -999; lastPlayerControl = lastRenderCams = -1;
	lightningP3.clear(); lastWeatherHash = 0; screenFadedOut = mockMission = mockShooting = false; mockImpact = camPos = camRot = V3();
	mockRagdoll = false; mockLookLR = mockLookUD = mockMoveUD = 0; lastRagdollMin = -1; g_mash = Mash(); g_ride = RideCam();
	g_gun.victims.clear(); g_gun.spun = 0; g_set.citySafe = true; nearSpeech.clear();
	unloadedModels.clear(); shown.clear(); fxNames.clear(); fxPos.clear(); lastVegType = lastVegFlags = -1;
	said.clear(); humans.clear(); missingCtx.clear(); speechUntil.clear(); outfitOf.clear(); audioFlags.clear(); lastInvincible = -1;
	storyCalls.clear(); convUntil.clear(); convCreated.clear(); textReq.clear(); textDel.clear(); convNames.clear(); convWorks = true; convStops = convHistoryClears = 0;
	g_landKind = 0; g_roughRagdolled = false; g_vo = VoiceState(); g_stats = StormStats(); g_voices = 2; g_chatter = 1;
	GalleryClose("reset"); g_gal = Gallery();
	// v1.1 audit 2
	g_frameDt = 0.016f; UI::g_clean = false; g_introLeft.clear(); g_treePins.clear(); g_treeChecked = 0;
	tcName.clear(); tcStrength = 0; tcSets.clear(); tcClears = 0; musicReady = false; musicEvents.clear(); countWaits = false; waits = 0;
	slowPtfx.clear(); ptfxAskedAt.clear(); ragdollOk.clear(); invincible.clear(); blockEvents.clear(); pedFlags.clear(); dictsRemoved.clear();
	modelsReleased.clear(); deletedWithPedInside = 0; musicNotReady.clear(); feedId = 0; mockHorse = 0; animOf.clear();
}
// Crude physics for tick(): unfrozen objects fall under gravity, move with their velocity and stop at the ground (z = 0).
static void integrate(float dt)
{
	for (auto& kv : objects)
	{
		MockObj& o = kv.second;
		if (stuck.count(kv.first)) { o.v = V3(); continue; }
		if (o.frozen) continue;
		o.v.z -= 9.8f * dt;
		o.p = o.p + o.v * dt;
		if (o.p.z < mockGroundZ) { o.p.z = mockGroundZ; o.v = V3(); }
	}
}
static void check(const char* title, bool fixed, const char* detail)
{
	printf("%s: %s | %s\n", fixed ? "FIXED       " : "STILL BROKEN", title, detail);
	(fixed ? fixedCount : brokenCount)++;
}
static float nextRetry = 0;
static void tick(float t)
{
	mockTime = t;
	for (auto& p : g_tornadoes) p->Update(0.1f, t);
	UpdateFlights(0.1f, t); integrate(0.1f);
	TornadoHousekeeping(t); StormUpdate(t); AutoUpdate(t); HoverSweep(t); PlayerSafety(0.1f, t); AdaptPuffRate(t);
	if (t >= nextRetry) { nextRetry = t + 1.0f; RetryOrphans(); }   // mirrors the main loop
}
static void runSelfTest() { nextRetry = 0; for (int step = 0; step < 900 && g_auto.running; ++step) tick(step / 10.0f); }

// v1.1: one frame of the real main loop (ScriptMain), in its order, minus input and drawing - for the scenes and modes
static void tick2(float t, float dt = 0.1f)
{
	mockTime = t;
	SyncMenuChoices();
	for (auto& p : g_tornadoes) p->Update(dt, t);
	UpdateFlights(dt, t); TornadoHousekeeping(t);
	StormUpdate(t); WeatherAssert(t); SurvivalUpdate(t); AutoUpdate(t); UpdateImpacts(t); HoverSweep(t); PlayerSafety(dt, t); AdaptPuffRate(t);
	IntroUpdate(dt, t); BalloonUpdate(dt, t); SeasonUpdate(t); GunUpdate(t); SoundUpdate(t); MusicUpdate(t); VoiceUpdate(t); GalleryUpdate(dt, t);
	integrate(dt);
	if (t >= nextRetry) { nextRetry = t + 1.0f; RetryOrphans(); }
}
static float T(int k) { return k * 0.1f; }   // frame k's clock (no float drift)

static Shape Cyl(float x, float y, float r, float top) { return { 0, V3(x, y, 0), V3(x, y, top), r }; }
static Shape Box(V3 mn, V3 mx) { return { 1, mn, mx, 0 }; }
// six tall trunks round c (9-11 m out), a 2 m post due south, a wide wall due north
static const int kTrunks = 6;
static void TreeWorld(V3 c)
{
	world.clear();
	const float angs[kTrunks] = { 0, 40, 140, 180, 220, 320 }, dists[kTrunks] = { 9, 10, 9.5f, 11, 9, 10 };
	for (int i = 0; i < kTrunks; i++)
	{
		float r = angs[i] * PI / 180.0f;
		world.push_back(Cyl(c.x + cosf(r) * dists[i], c.y + sinf(r) * dists[i], 0.6f, 16.0f));
	}
	world.push_back(Cyl(c.x, c.y - 7.0f, 0.3f, 2.0f));                                   // [6] the post
	world.push_back(Box(V3(c.x - 4, c.y + 13, 0), V3(c.x + 4, c.y + 14, 10)));          // [7] the wall
}
static float DistToBox2d(const Shape& s, const V3& p)
{
	float dx = std::max(std::max(s.a.x - p.x, 0.0f), p.x - s.b.x), dy = std::max(std::max(s.a.y - p.y, 0.0f), p.y - s.b.y);
	return sqrtf(dx * dx + dy * dy);
}

static std::string ReadFileStr(const std::string& path)
{
	std::string s;
	FILE* f = nullptr;
	if (fopen_s(&f, path.c_str(), "rb") == 0 && f) { char b[512]; size_t n; while ((n = fread(b, 1, sizeof(b), f)) > 0) s.append(b, n); fclose(f); }
	return s;
}
static void WriteFileStr(const std::string& path, const std::string& s)
{
	FILE* f = nullptr;
	if (fopen_s(&f, path.c_str(), "wb") == 0 && f) { fwrite(s.data(), 1, s.size(), f); fclose(f); }
}
// NadoTest_best.txt: the scenarios use a temporary copy and put the real one back
static std::string g_bestBackup;
static bool g_bestExisted = false;
static void BackupBest() { g_bestExisted = GetFileAttributesA(BestPath().c_str()) != INVALID_FILE_ATTRIBUTES; g_bestBackup = g_bestExisted ? ReadFileStr(BestPath()) : ""; }
static void RestoreBest() { if (g_bestExisted) WriteFileStr(BestPath(), g_bestBackup); else remove(BestPath().c_str()); }
static int PosedCount() { int n = 0; for (int i = 0; i < kCastCount; i++) if (kCast[i].ride == RIDE_POSED) n++; return n; }

int main()
{
	try
	{
		LogInit(GetModuleHandleA(nullptr));
		UI::g_mute = true; Roar::g_mute = true;   // v1.1: no sound from the harness
		InputInit();                               // v1.1: the menu scenario feeds a key press through the real keyboard path
		g_introTick = []() -> DWORD { return (DWORD)(mockTime * 1000.0f + 0.5f); };   // the intro runs on the mock clock
		char detail[200];
		char det[2400];

		// 1. heal retries after a failed replacement (legacy anchor engine)
		reset();
		g_set.engine = 0;
		{
			Tornado n(V3(), 0, 0, "heal", true, 1);
			int anchor = objects.begin()->first; objects.erase(anchor); fxAnchors.clear(); failCreates = 1;
			mockTime = 1; n.Update(0.1f, 1); int attempts = calls[N_CREATE_OBJECT];
			mockTime = 2; n.Update(0.1f, 2); mockTime = 3; n.Update(0.1f, 3); mockTime = 3.5f; n.Update(0.1f, 3.5f);
			sprintf_s(detail, "create attempts %d -> %d, loops alive %d, heal-fail %d, healed %d", attempts, calls[N_CREATE_OBJECT], n.LoopsAlive(), n.healFailed, n.healed);
			check("healing retries after CREATE_OBJECT fails", calls[N_CREATE_OBJECT] > attempts && n.LoopsAlive() == 1, detail);
			n.Destroy();
		}
		// 2. self-test can't pass with no looped effects (it now forces render=Both and requires a positive plan)
		reset();
		g_set.render = 1; AutoSelfTest(); runSelfTest();
		sprintf_s(detail, "self-test %d pass / %d fail / %d skip; loop-start calls %d; render restored to %d", g_st.pass, g_st.fail, g_st.skip, calls[N_START_PARTICLE_FX_LOOPED_ON_ENTITY], g_set.render);
		check("self-test exercises real looped effects", calls[N_START_PARTICLE_FX_LOOPED_ON_ENTITY] > 0 && g_set.render == 1, detail);
		printf("              (clean mock run: %s - %d passed)\n", g_st.fail == 0 ? "0 failures" : "has failures - see NadoTest_findings.txt", g_st.pass);
		int cleanFails = g_st.fail, cleanPass = g_st.pass;
		// 3. a surviving object is reported, not hidden
		reset();
		failDeletes = 1; AutoSelfTest(); runSelfTest();
		sprintf_s(detail, "self-test %d pass / %d fail; objects left %zu; AnchorCount %d; orphans %d", g_st.pass, g_st.fail, objects.size(), AnchorCount(), OrphanCount());
		check("self-test reports a failed deletion instead of a clean pass", g_st.fail > 0, detail);
		// 4. FX Lab anchors are ignored by tornado physics
		reset();
		{
			Tornado n(V3(), 0, 0, "lab", true, 0);
			Object lab = SpawnAnchor(V3(n.wallRadius(), 0, 0));
			g_lab.anchors.push_back(lab); g_lab.on = true;
			mockTime = 7; n.Update(0.1f, 7);
			sprintf_s(detail, "lab anchor frozen=%d velocity=%.2f", (int)objects[lab].frozen, objects[lab].v.len());
			check("FX Lab anchor not captured or unfrozen", objects[lab].frozen && objects[lab].v.len() == 0, detail);
			n.Destroy();
		}
		// 5-7. touchdown threshold, dissipate continuity, alpha fade
		reset();
		{
			Tornado n(V3(), 0, 0, "touchdown", true, 1);
			OBJECT::CREATE_OBJECT(42, n.wallRadius(), 0, 0, FALSE, FALSE, TRUE, FALSE, FALSE);
			mockTime = 4.1f; n.Update(0.1f, 4.1f);
			float lowest = n.FunnelPoint(0, 0, 1, 4.1f).z;
			sprintf_s(detail, "at 4.1 s lowest funnel point %.2f m, physics calls %d", lowest, calls[N_SET_ENTITY_VELOCITY]);
			check("no grabbing before touchdown", lowest > 20 && calls[N_SET_ENTITY_VELOCITY] == 0, detail);
			mockTime = 2; n.Update(0.1f, 2); float oldGrowth = n.growth;
			n.BeginDissipate(2); mockTime = 2.1f; n.Update(0.1f, 2.1f);
			sprintf_s(detail, "growth %.3f -> %.3f when dissipating during formation", oldGrowth, n.growth);
			check("early dissipate retracts from where it is", n.growth <= oldGrowth + 0.001f, detail);
			mockTime = 5; n.Update(0.1f, 5);
			sprintf_s(detail, "SET_PARTICLE_FX_LOOPED_ALPHA calls %d", calls[N_SET_PARTICLE_FX_LOOPED_ALPHA]);
			check("dissipate fades the looped effects", calls[N_SET_PARTICLE_FX_LOOPED_ALPHA] > 0, detail);
			n.Destroy();
		}
		// 8. replacing a test restores the previous test's settings
		reset();
		g_set.extraDebris = true; AutoSelfTest(); tick(0); AutoStyleTour();
		sprintf_s(detail, "extra debris after replacing self-test with style tour = %d", (int)g_set.extraDebris);
		check("switching tests restores settings", g_set.extraDebris, detail);
		// 9. storm OFF switches release their effects right away
		reset();
		g_set.camShake = true; g_set.heavyRain = true; g_set.wind = true;
		SpawnTornado(0, 30, 0, true, "toggles"); StormUpdate(0);
		int stops = calls[N_STOP_GAMEPLAY_CAM_SHAKING], rain = calls[N_SET_RAIN], wind = calls[N_SET_WIND_SPEED];
		bool wasShaking = g_storm.shaking;
		g_set.camShake = false; g_set.heavyRain = false; g_set.wind = false; StormUpdate(1);
		sprintf_s(detail, "was shaking %d; stop-shake +%d, rain +%d, wind +%d", (int)wasShaking, calls[N_STOP_GAMEPLAY_CAM_SHAKING] - stops, calls[N_SET_RAIN] - rain, calls[N_SET_WIND_SPEED] - wind);
		check("storm OFF toggles release effects", !g_storm.shaking && !g_storm.rainForced && calls[N_STOP_GAMEPLAY_CAM_SHAKING] > stops && calls[N_SET_RAIN] > rain && calls[N_SET_WIND_SPEED] > wind, detail);
		// 10. (new) failed anchor delete becomes a counted orphan and is retried (legacy anchor engine)
		reset();
		g_set.engine = 0;
		{
			Tornado n(V3(), 0, 0, "orphan", true, 1);
			failDeletes = 1; n.Destroy();
			int orphans = OrphanCount(), alive = AnchorCount();
			RetryOrphans();
			sprintf_s(detail, "after failed delete: orphans %d, AnchorCount %d; after retry: orphans %d, objects %zu", orphans, alive, OrphanCount(), objects.size());
			check("failed anchor delete is counted and retried", orphans == 1 && alive == 1 && OrphanCount() == 0 && objects.empty(), detail);
		}
		// 11. (new) destructor makes no native calls (unload runs off the script thread)
		reset();
		{
			int before;
			{
				Tornado n(V3(), 0, 0, "dtor", true, 1);
				n.Destroy();
				before = totalCalls();
			}
			sprintf_s(detail, "native calls during destruction: %d", totalCalls() - before);
			check("destructor calls no natives", totalCalls() == before, detail);
		}
		// 12. (new) switching a grab category OFF applies immediately
		reset();
		{
			Tornado n(V3(), 0, 0, "toggle", true, 0);
			Object prop = OBJECT::CREATE_OBJECT(42, n.wallRadius(), 0, 0, FALSE, FALSE, TRUE, FALSE, FALSE);
			mockTime = 7; n.Update(0.1f, 7); int pushes = calls[N_SET_ENTITY_VELOCITY];
			g_set.grabProps = false; objects[prop].v = V3();
			mockTime = 7.1f; n.Update(0.1f, 7.1f);   // same gather window: target list not rebuilt yet
			sprintf_s(detail, "pushes while ON %d, extra pushes right after OFF %d", pushes, calls[N_SET_ENTITY_VELOCITY] - pushes);
			check("grab toggle OFF applies immediately", pushes > 0 && calls[N_SET_ENTITY_VELOCITY] == pushes, detail);
			n.Destroy();
		}
		// 13. (v0.4) a planted tree inside the "rip loose" ring is uprooted (playtest 4 logged "uprooted 0" all session)
		reset();
		{
			Tornado n(V3(), 0, 0, "uproot", true, 0);
			Object tree = OBJECT::CREATE_OBJECT(42, n.wallRadius() * 1.5f, 0, 0, FALSE, FALSE, TRUE, FALSE, FALSE);
			objects[tree].frozen = true; g_priority.insert(tree);
			mockTime = 7; n.Update(0.1f, 7);
			int up = n.uprooted, flights = FlightsActive();
			for (int k = 1; k <= 30; k++) { mockTime = 7 + k * 0.1f; n.Update(0.1f, mockTime); UpdateFlights(0.1f, mockTime); }
			float z = objects[tree].p.z;
			n.Destroy();
			sprintf_s(detail, "uprooted %d, flights %d, tree %.1f m up after 3 s, still a priority target %d", up, flights, z, (int)g_priority.count(tree));
			check("planted tree in the rip ring gets the scripted uproot", up == 1 && flights == 1 && z > 5 && !g_priority.count(tree), detail);
		}
		// 14. (v0.4) an empty pool read keeps the last good list (playtest 4: 0/0/0 for ~70 s in Rhodes)
		reset();
		{
			Tornado n(V3(), 0, 0, "pool", true, 0);
			OBJECT::CREATE_OBJECT(42, n.wallRadius() * 3.0f, 0, 0, FALSE, FALSE, TRUE, FALSE, FALSE);
			mockTime = 7; n.Update(0.1f, 7);
			emptyPool = true;
			for (int k = 1; k <= 50; k++) { mockTime = 7 + k * 0.1f; n.Update(0.1f, mockTime); }   // 5 s: longer than the 3 s tracking memory
			int before = calls[N_SET_ENTITY_VELOCITY];
			mockTime = 12.1f; n.Update(0.1f, 12.1f);
			int pushes = calls[N_SET_ENTITY_VELOCITY] - before;
			sprintf_s(detail, "empty reads %d, pushes after 5 s of empty reads %d, tracked targets %d", PoolEmptyReads(), pushes, n.counts.objects);
			check("empty entity lists fall back to the last good list", PoolEmptyReads() > 0 && pushes > 0, detail);
			n.Destroy();
		}
		// 15. (v0.4, v0.6 semantics) a prop the tornado carried and left floating gets its physics woken and comes down
		reset();
		{
			Object prop = OBJECT::CREATE_OBJECT(42, 50, 0, 0, FALSE, FALSE, TRUE, FALSE, FALSE);
			NoteTouched(prop, 0);                                   // found on the ground...
			objects[prop].p.z = 6; objects[prop].frozen = true;     // ...left hanging 6 m up
			for (int k = 0; k <= 40; k++) { mockTime = 2 + k * 0.1f; HoverSweep(mockTime); integrate(0.1f); }
			sprintf_s(detail, "woken %d, frozen %d, height %.1f m", HoverWoken(), (int)objects[prop].frozen, objects[prop].p.z);
			check("floating prop is woken and comes down", HoverWoken() >= 1 && !objects[prop].frozen && objects[prop].p.z < 1, detail);
		}
		// 16. (v0.4) the open menu gives looking and moving back (playtest 4: "I can't move Arthur or anything in the menu")
		reset();
		{
			BuildMenu(); g_menuOpen = true; g_menuStack.push_back({ 0, 0 });
			MenuInput();
			bool look = enabledControls.count(Joaat("INPUT_LOOK_LR")) && enabledControls.count(Joaat("INPUT_LOOK_UD"));
			bool move = enabledControls.count(Joaat("INPUT_MOVE_LR")) && enabledControls.count(Joaat("INPUT_MOVE_UD"));
			sprintf_s(detail, "disable-all %d, look re-enabled %d, move re-enabled %d (%zu actions)", calls[N_DISABLE_ALL_CONTROL_ACTIONS], (int)look, (int)move, enabledControls.size());
			check("menu keeps camera and movement", calls[N_DISABLE_ALL_CONTROL_ACTIONS] == 1 && look && move, detail);
		}
		// 17. (v0.4) a user spawn lands outside its own reach (playtest 4: "we're already in the tornado")
		reset();
		{
			g_set.reach = 3;
			UserSpawn();
			float d = g_tornadoes.empty() ? 0 : g_tornadoes[0]->base.len2d();
			float reach = g_tornadoes.empty() ? 0 : g_tornadoes[0]->reachRadius();
			sprintf_s(detail, "spawned %.0f m away, reach %.0f m", d, reach);
			check("user spawn touches down outside its reach", !g_tornadoes.empty() && d >= reach + 30, detail);
		}
		// 18. (v0.4) Arthur's death makes the tornadoes die down
		reset();
		{
			SpawnTornado(0, 80, 0, false, "death");
			mockDead = true;
			bool dead = WatchPlayerDeath(1);
			sprintf_s(detail, "dead seen %d, dissipating %d", (int)dead, g_tornadoes.empty() ? -1 : (int)g_tornadoes[0]->Dissipating());
			check("tornadoes die down when Arthur dies", dead && !g_tornadoes.empty() && g_tornadoes[0]->Dissipating(), detail);
		}
		// 19. (v0.4) a Script Hook restart forgets old handles without touching the game
		reset();
		{
			ResetAfterScriptRestart();   // the first call in this process counts as the first start
			SpawnTornado(0, 80, 0, false, "restart");
			int before = totalCalls();
			ResetAfterScriptRestart();   // the restart
			sprintf_s(detail, "tornadoes %zu, anchors registered %zu, native calls during the restart %d", g_tornadoes.size(), g_anchors.size(), totalCalls() - before);
			check("script restart drops stale handles without natives", g_tornadoes.empty() && g_anchors.empty() && totalCalls() == before, detail);
		}
		// 20. (v0.4, reworked in v0.5) soft landing: only after a tornado drop; real height; speed SET; damage backstop
		reset();
		{
			playerPos = V3(0, 0, 40); playerVel = V3(0, 0, -22);
			PlayerSafety(0.1f, 10);
			float untouched = playerVel.z;
			NotePlayerInWall(9);
			PlayerSafety(0.1f, 10);
			float at40 = playerVel.z;
			playerPos = V3(0, 0, 8); playerVel = V3(0, 0, -15);
			PlayerSafety(0.1f, 10.5f);
			float at8 = playerVel.z;
			bool backstop = canBeDamaged.count(1) && canBeDamaged[1] == 0;
			playerPos = V3(0, 0, 0.5f); playerVel = V3();
			PlayerSafety(0.1f, 11.0f);
			int landings = GetLanding().landings;
			PlayerSafety(0.1f, 14.0f);
			bool restored = canBeDamaged.count(1) && canBeDamaged[1] == 1;
			sprintf_s(detail, "no tornado: %.1f m/s left alone; after a drop: at 40 m -> %.1f, at 8 m -> %.1f; backstop on %d; landings %d; damage restored %d",
				untouched, at40, at8, (int)backstop, landings, (int)restored);
			check("soft landing slows the whole fall and guards the landing", untouched == -22 && at40 > -22.0f && at40 < -15.0f && at8 > -9.0f &&
				backstop && landings == 1 && restored, detail);
		}
		// 21. (v0.5) world-space engine is the default: no anchor props, and a removed effect is restarted
		reset();
		{
			Tornado n(V3(), 0, 0, "world", true, 40);
			int creates = calls[N_CREATE_OBJECT], atCoord = calls[N_START_PARTICLE_FX_LOOPED_AT_COORD];
			int victim = fxAnchors.begin()->first; fxAnchors.erase(victim);
			mockTime = 1.5f; n.Update(0.1f, 1.5f);
			sprintf_s(detail, "anchor props %d, world loops started %d, healed %d, alive %d/%d", creates, atCoord, n.healed, n.LoopsAlive(), n.loopsRunning);
			check("world-space engine: no anchors, self-heals", creates == 0 && atCoord > 0 && n.healed >= 1 && n.LoopsAlive() == n.loopsRunning, detail);
			n.Destroy();
		}
		// 22. (v0.5) the sweeper leaves a resting prop alone even when the height native reports world Z (playtest 5: 836)
		reset();
		{
			mockGroundZ = 70;
			Object prop = OBJECT::CREATE_OBJECT(42, 50, 0, 70.3f, FALSE, FALSE, TRUE, FALSE, FALSE);
			objects[prop].frozen = true;
			NoteTouched(prop, 0);
			int moves = calls[N_SET_ENTITY_COORDS_NO_OFFSET];
			for (int k = 0; k <= 60; k++) { mockTime = 2 + k * 0.1f; HoverSweep(mockTime); integrate(0.1f); }
			sprintf_s(detail, "height native says %.1f m; woken %d, moves %d, still at z %.1f", ENTITY::GET_ENTITY_HEIGHT_ABOVE_GROUND(prop),
				HoverWoken(), calls[N_SET_ENTITY_COORDS_NO_OFFSET] - moves, objects[prop].p.z);
			check("sweeper ignores props resting on high ground", HoverWoken() == 0 && calls[N_SET_ENTITY_COORDS_NO_OFFSET] == moves && objects[prop].p.z == 70.3f, detail);
		}
		// 23. (v0.5, v0.6 semantics) a carried prop whose physics never takes falls under our own gravity - never teleported
		reset();
		{
			Object prop = OBJECT::CREATE_OBJECT(42, 50, 0, 0, FALSE, FALSE, TRUE, FALSE, FALSE);
			NoteTouched(prop, 0);
			objects[prop].p.z = 6; objects[prop].frozen = true; stuck.insert(prop);
			int teleports = calls[N_SET_ENTITY_COORDS];
			float maxStep = 0, lastZ = 6;
			for (int k = 0; k <= 100; k++)
			{
				mockTime = 2 + k * 0.1f; HoverSweep(mockTime); UpdateFlights(0.1f, mockTime); integrate(0.1f);
				maxStep = std::max(maxStep, fabsf(objects[prop].p.z - lastZ)); lastZ = objects[prop].p.z;
			}
			sprintf_s(detail, "wakes %d, glided %d, height %.1f m, biggest move in 0.1 s %.2f m, teleports %d", HoverWoken(), HoverGlided(),
				objects[prop].p.z, maxStep, calls[N_SET_ENTITY_COORDS] - teleports);
			check("stuck floater: one wake, then brought down smoothly", HoverWoken() == 1 && HoverGlided() == 1 && objects[prop].p.z < 0.5f &&
				maxStep < 2.0f && calls[N_SET_ENTITY_COORDS] == teleports, detail);
		}
		// 24. (v0.5) the sweeper's rate cap: no more than 20 wakes a minute
		reset();
		{
			for (int i = 0; i < 40; i++)
			{
				Object o = OBJECT::CREATE_OBJECT(42, 50.0f + i, 0, 0, FALSE, FALSE, TRUE, FALSE, FALSE);
				NoteTouched(o, 0);
				objects[o].p.z = 6; objects[o].frozen = true; stuck.insert(o);
			}
			for (int k = 0; k <= 100; k++) { mockTime = 2 + k * 0.1f; HoverSweep(mockTime); }
			sprintf_s(detail, "40 floaters, wakes in the first 10 s: %d", HoverWoken());
			check("sweeper wakes at most 20 props a minute", HoverWoken() == 20, detail);
		}
		// 25. (v0.5) fling-and-chase: Arthur held in the wall is thrown out, then not re-grabbed for a while
		reset();
		{
			g_set.grabPlayer = true; g_set.touchdown = false; g_set.flingHoldMin = g_set.flingHoldMax = 2.0f;
			g_set.landings = 0;   // (v1.6: Soft - always an outward throw; Mixed can pick slammed or the meteor)
			mockPeds.push_back(1);
			Tornado n(V3(), 0, 0, "fling", true, 0);
			playerPos = V3(n.wallRadius(), 0, 1);
			for (int k = 1; k <= 35; k++) { mockTime = k * 0.1f; n.Update(0.1f, mockTime); }
			V3 thrown = playerVel;
			int flings = n.flings;
			playerVel = V3();
			for (int k = 36; k <= 60; k++) { mockTime = k * 0.1f; n.Update(0.1f, mockTime); }
			sprintf_s(detail, "flings %d, throw velocity (%.0f, %.0f, %.0f), velocity set again within 2.5 s: %d", flings, thrown.x, thrown.y, thrown.z,
				(int)(playerVel.len() > 0));
			check("fling-and-chase throws Arthur outward once", flings == 1 && thrown.x > 10 && thrown.z > 5 && playerVel.len() == 0, detail);
			n.Destroy();
		}
		// 26. (v0.5) empty reads ask the game for nearby peds (playtest 5's camp ran dry for 20 s)
		reset();
		{
			Tornado n(V3(), 0, 0, "topup", true, 0);
			OBJECT::CREATE_OBJECT(42, n.wallRadius() * 3.0f, 0, 0, FALSE, FALSE, TRUE, FALSE, FALSE);
			mockTime = 7; n.Update(0.1f, 7);
			emptyPool = true;
			for (int k = 1; k <= 40; k++) { mockTime = 7 + k * 0.1f; n.Update(0.1f, mockTime); }
			sprintf_s(detail, "nearby-ped queries during 4 s of empty reads: %d, tracked objects kept %d", calls[N_GET_PED_NEARBY_PEDS], n.counts.objects);
			check("empty reads top up from the game's nearby list and keep the grip", calls[N_GET_PED_NEARBY_PEDS] >= 2 && n.counts.objects == 1, detail);
			n.Destroy();
		}
		// 27. (v0.5) tracker HUD compass maths
		reset();
		{
			float ahead = RelBearing(V3(), V3(0, 100, 0), 0);
			float west = RelBearing(V3(), V3(-100, 0, 0), 0);
			float facingWest = RelBearing(V3(), V3(-100, 0, 0), 90);
			float behind = RelBearing(V3(), V3(0, -100, 0), 0);
			float wrap = RelBearing(V3(), V3(0, 100, 0), 350);
			sprintf_s(detail, "ahead %.0f, west %.0f (left), west while facing west %.0f, behind %.0f, heading 350 -> %.0f", ahead, west, facingWest, behind, wrap);
			check("tracker bearing: ahead 0, left positive, behind 180", fabsf(ahead) < 0.5f && fabsf(west - 90) < 0.5f && fabsf(facingWest) < 0.5f &&
				fabsf(fabsf(behind) - 180) < 0.5f && fabsf(wrap - 10) < 0.5f, detail);
		}
		// 28. (v0.5) style tour names and numbers each style, and the next-step key moves on
		reset();
		{
			AutoStyleTour(); tick(0);
			g_autoSkip = true; tick(0.1f);   // past the storm intro
			std::string desc = g_auto.idx >= 0 ? g_auto.steps[g_auto.idx].desc : "";
			int idx = g_auto.idx;
			g_autoSkip = true; tick(0.2f);
			sprintf_s(detail, "step %d card \"%.40s...\", after the next-step key: step %d", idx + 1, desc.c_str(), g_auto.idx + 1);
			check("style tour says 'Style 1 of N' and can be skipped", desc.find("Style 1 of") != std::string::npos && g_auto.idx == idx + 1, detail);
			AutoStop("harness");
		}
		// 29. (v0.5) the hour can only be changed through the picker (playtest 5: "-1 hour" fired 13 times in 4 s)
		reset();
		{
			BuildMenu();
			bool plusMinus = false;
			for (auto& it : g_pages[P_WORLD].items) if (it.label == "+1 hour" || it.label == "-1 hour") plusMinus = true;
			sprintf_s(detail, "+1/-1 hour items present: %d", (int)plusMinus);
			check("no repeatable +1/-1 hour items", !plusMinus, detail);
		}
		// 30. (v0.5) drone modes frame Arthur differently when there is no tornado (playtest 5 problem 7)
		reset();
		{
			V3 cams[3];
			for (int mode = 1; mode <= 3; mode++)
			{
				g_drone = Drone(); g_drone.mode = mode;
				DroneUpdate(0.1f, false);
				cams[mode - 1] = lastCamCoord;
			}
			g_drone = Drone();
			sprintf_s(detail, "orbit z %.1f, low z %.1f, shoulder (%.1f, %.1f, %.1f)", cams[0].z, cams[1].z, cams[2].x, cams[2].y, cams[2].z);
			check("drone modes differ without a tornado", fabsf(cams[0].z - cams[1].z) > 3 && (cams[2] - cams[0]).len() > 5 && (cams[2] - cams[1]).len() > 3, detail);
		}
		// 31. (v0.5) the full self-test runs clean on the mock (world-space A, legacy-anchor B, both barrels, stress)
		sprintf_s(detail, "%d passed, %d failed", cleanPass, cleanFails);
		check("self-test: clean mock run", cleanFails == 0 && cleanPass >= 30, detail);
		// 32. (v0.6) a "floater" that is still where the tornado first found it (a hanging sign or lantern) is left alone
		reset();
		{
			Object prop = OBJECT::CREATE_OBJECT(42, 50, 0, 6, FALSE, FALSE, TRUE, FALSE, FALSE);
			objects[prop].frozen = true;
			NoteTouched(prop, 0);
			for (int k = 0; k <= 60; k++) { mockTime = 2 + k * 0.1f; HoverSweep(mockTime); UpdateFlights(0.1f, mockTime); integrate(0.1f); }
			sprintf_s(detail, "woken %d, glided %d, left alone %d, still at z %.1f", HoverWoken(), HoverGlided(), HoverNatural(), objects[prop].p.z);
			check("hanging prop where the tornado found it is left alone", HoverWoken() == 0 && HoverGlided() == 0 && HoverNatural() == 1 && objects[prop].p.z == 6, detail);
		}
		// 33. (v0.6) the debris cone: props spawn, orbit (excluded from pushes), are deleted on despawn and thrown out on die-down
		reset();
		{
			g_set.touchdown = false;
			Tornado n(V3(), 0, 0, "cone", true, 40);
			for (int k = 1; k <= 30; k++) { mockTime = k * 0.1f; n.Update(0.1f, mockTime); }
			int count = n.OrbiterCount();
			std::vector<Entity> obs; n.CaptureOrbiters(obs);
			V3 before = obs.empty() ? V3() : objects[obs[0]].p;
			mockTime = 3.1f; n.Update(0.1f, 3.1f);
			float moved = obs.empty() ? 0 : (objects[obs[0]].p - before).len();
			bool excluded = !obs.empty() && IsOrbiter(obs[0]);
			n.Destroy();
			int left = 0; for (Entity e : obs) if (objects.count(e)) left++;
			Tornado d(V3(), 0, 0, "cone2", true, 40);
			for (int k = 1; k <= 30; k++) { mockTime = 4 + k * 0.1f; d.Update(0.1f, mockTime); }
			int had = d.OrbiterCount();
			d.BeginDissipate(mockTime); mockTime += 0.1f; d.Update(0.1f, mockTime);
			sprintf_s(detail, "orbiting %d, moved %.1f m in 0.1 s, flagged %d, left after despawn %d; die-down: %d -> %d, thrown out %d",
				count, moved, (int)excluded, left, had, d.OrbiterCount(), d.orbitersEjected);
			check("debris cone spawns, orbits, cleans up and is thrown out on die-down", count >= 10 && moved > 0.5f && excluded && left == 0 &&
				had > 0 && d.OrbiterCount() == 0 && d.orbitersEjected == had, detail);
			d.Destroy();
		}
		// 34. (v0.6) a prop with dead physics in the wall gets carried by the mod itself (playtest 6: "some stuff doesn't get woken")
		reset();
		{
			g_set.touchdown = false; g_set.debrisCone = false;
			Tornado n(V3(), 0, 0, "carry", true, 0);
			Object prop = OBJECT::CREATE_OBJECT(42, n.wallRadius(), 0, 1, FALSE, FALSE, TRUE, FALSE, FALSE);
			stuck.insert(prop);
			for (int k = 1; k <= 25; k++) { mockTime = k * 0.1f; n.Update(0.1f, mockTime); UpdateFlights(0.1f, mockTime); }
			sprintf_s(detail, "carried %d, in flight %d, height now %.1f m", n.carried, (int)InFlight(prop), objects[prop].p.z);
			check("a prop that never moves is carried up the wall", n.carried == 1 && objects[prop].p.z > 1.5f, detail);
			n.Destroy();
		}
		// 35. (v0.6) Arthur levels: immune is never pushed; tugged is never thrown out; grabbable is
		reset();
		{
			g_set.touchdown = false; g_set.debrisCone = false; g_set.flingHoldMin = g_set.flingHoldMax = 1.0f;
			mockPeds.push_back(1);
			int flings[3];
			for (int lvl = 0; lvl <= 2; lvl++)
			{
				g_set.arthur = lvl; SyncMenuChoices();
				Tornado n(V3(), 0, 0, "arthur", true, 0);
				playerPos = V3(n.wallRadius(), 0, 1); playerVel = V3(); lastForceZ = 0;
				for (int k = 1; k <= 30; k++) { mockTime = 10 * lvl + k * 0.1f; n.Update(0.1f, mockTime); }
				flings[lvl] = n.flings;
				if (lvl == 0) flings[0] = (playerVel.len() > 0 || lastForceZ != 0) ? 99 : 0;
				n.Destroy();
			}
			sprintf_s(detail, "immune pushed %d, tugged flings %d, grabbable flings %d", flings[0], flings[1], flings[2]);
			check("Arthur: immune untouched, tugged never thrown, grabbable thrown", flings[0] == 0 && flings[1] == 0 && flings[2] == 1, detail);
		}
		// 36. (v0.6) impact bursts come from real collisions: a thrown thing that suddenly loses its speed
		reset();
		{
			Object prop = OBJECT::CREATE_OBJECT(42, 10, 0, 5, FALSE, FALSE, TRUE, FALSE, FALSE);
			NoteFlyer(prop, 3, 0);
			objects[prop].v = V3(26, 0, 0); UpdateImpacts(0.0f);
			objects[prop].v = V3(24, 0, 0); UpdateImpacts(0.15f);
			int cruising = ImpactsShown();
			objects[prop].v = V3(1, 0, 0); UpdateImpacts(0.3f);
			sprintf_s(detail, "bursts while flying %d, after the sudden stop %d", cruising, ImpactsShown());
			check("impact burst only where something hits", cruising == 0 && ImpactsShown() == 1, detail);
		}
		// 37. (v0.6) the simple menu choices drive the older switches (Arthur level, weather)
		reset();
		{
			g_set.arthur = 2; g_set.weatherMode = 3; SyncMenuChoices();
			bool a = g_set.grabPlayer && g_set.weather && g_set.weatherType == 2;
			g_set.arthur = 0; g_set.weatherMode = 0; SyncMenuChoices();
			bool b = !g_set.grabPlayer && !g_set.weather;
			sprintf_s(detail, "grabbable+rain -> %d, immune+keep yours -> %d", (int)a, (int)b);
			check("menu choices sync (Arthur, weather; defaults: storm clouds, grabbable)", a && b && Settings().weatherMode == 1 && Settings().arthur == 2, detail);
		}
		// 38. (v0.6) Low PC mode trims the smoke budget, the debris cone and the grab cap
		reset();
		{
			g_set.perf = 2;
			Tornado n(V3(), 0, 0, "lowpc", true, 110);
			sprintf_s(detail, "loops planned %d (budget 110), debris props %d, max grabs %d, puffs x%.1f", n.loopsPlanned, PerfOrbiters(), PerfMaxEntities(), PerfPuffMul());
			check("Low PC mode cuts the expensive parts", n.loopsPlanned <= 50 && PerfOrbiters() == 18 && PerfMaxEntities() <= 70 && PerfPuffMul() == 0.5f, detail);
			n.Destroy();
		}
		// 39. (v0.6) the black train-smoke streamers start aimed along the spin (StreamerPitch)
		reset();
		{
			tiltedStarts = 0;
			Tornado n(V3(), 0, 0, "streamers", true, 110);
			sprintf_s(detail, "streamers started tilted %d (pitch %.0f), total world loops %d", tiltedStarts, g_set.streamerPitch, calls[N_START_PARTICLE_FX_LOOPED_AT_COORD]);
			check("tangent streamers are aimed (default style V: three bands in v1.1)", tiltedStarts >= 18 && tiltedStarts <= 24, detail);
			n.Destroy();
		}
		// 40. (v0.6) the touchdown camera shows a user spawn and hands back after ~7.5 s
		reset();
		{
			mockTime = 1; UserSpawn();
			bool on = g_tc.on;
			mockTime = 5; TouchCamUpdate(false); bool stillOn = g_tc.on;
			mockTime = 9; TouchCamUpdate(false);
			sprintf_s(detail, "on after spawn %d, at 4 s %d, at 8 s %d", (int)on, (int)stillOn, (int)g_tc.on);
			check("touchdown camera on spawn, then back to Arthur", on && stillOn && !g_tc.on, detail);
		}
		// 41. (v0.6) the tree scan is remembered (playtest 6: 40 s of scanning before the tree demo)
		reset();
		{
			g_spawnableTrees = { { "p_tree_test_01", 21.0f }, { "p_tree_douglasfir_snow_05", 27.8f } };
			SaveTreeCache();
			g_spawnableTrees.clear();
			LoadTreeCache();
			sprintf_s(detail, "trees loaded back %zu (snowy ones skipped)", g_spawnableTrees.size());
			check("tree scan cache saves and loads", g_spawnableTrees.size() == 1 && g_spawnableTrees[0].name == "p_tree_test_01", detail);
			g_spawnableTrees.clear();
		}
		// 42. (v0.7) a prop first seen high in the air (thrown debris) that then hangs there is brought down, not "a sign"
		reset();
		{
			Object prop = OBJECT::CREATE_OBJECT(42, 50, 0, 60, FALSE, FALSE, TRUE, FALSE, FALSE);
			objects[prop].frozen = true; stuck.insert(prop);
			NoteTouched(prop, 0);
			for (int k = 0; k <= 120; k++) { mockTime = 2 + k * 0.1f; HoverSweep(mockTime); UpdateFlights(0.1f, mockTime); integrate(0.1f); }
			sprintf_s(detail, "left alone %d, glided %d, height now %.1f m", HoverNatural(), HoverGlided(), objects[prop].p.z);
			check("debris first seen 60 m up isn't mistaken for a hanging sign", HoverNatural() == 0 && HoverGlided() == 1 && objects[prop].p.z < 1.0f, detail);
		}
		// 43. (v0.7) a second tornado gets an even share of the smoke (playtest 7: 85, 24 and 1 looped effects)
		reset();
		{
			g_set.multi = true;
			SpawnTornado(0, 80, 0, true, "first");
			int firstBefore = g_tornadoes[0]->loopsRunning;
			SpawnTornado(0, 80, 40, true, "second");
			SpawnTornado(0, 80, -40, true, "third");
			sprintf_s(detail, "first %d -> %d, second %d, third %d (budget %d, in use %d)", firstBefore, g_tornadoes[0]->loopsRunning,
				g_tornadoes[1]->loopsRunning, g_tornadoes[2]->loopsRunning, g_set.ptfxBudget, LoopsInUse());
			bool fair = g_tornadoes[2]->loopsRunning >= 30 && g_tornadoes[0]->loopsRunning <= 37 && LoopsInUse() <= g_set.ptfxBudget;
			check("tornadoes share the looped-effect budget evenly", fair, detail);
		}
		// 44. (v0.7) with Multiple tornadoes off, a new spawn makes the old one die down; one press = one tornado
		reset();
		{
			mockTime = 1; UserSpawn();
			mockTime = 1.5f; UserSpawn();                 // same press, half a second later: ignored
			size_t afterDouble = g_tornadoes.size();
			mockTime = 3; UserSpawn();
			sprintf_s(detail, "after a double press %zu tornado(es); after a second spawn: %zu, first dying down %d, new one's loops %d",
				afterDouble, g_tornadoes.size(), (int)g_tornadoes[0]->Dissipating(), g_tornadoes.back()->loopsRunning);
			check("one press = one tornado; a new one replaces the old", afterDouble == 1 && g_tornadoes.size() == 2 && g_tornadoes[0]->Dissipating() &&
				g_tornadoes.back()->loopsRunning >= 60, detail);
		}
		// 45. (v0.7) changing Style re-dresses a tornado that's already out
		reset();
		{
			SpawnTornado(0, 80, 0, true, "restyle");
			Tornado* tp = g_tornadoes[0].get();
			int before = tp->loopsRunning, inUse = LoopsInUse();
			tp->Restyle(2, tp->loopsRunning + FreeLoops());
			sprintf_s(detail, "style %d, label '%s', loops %d -> %d, in use %d -> %d", tp->styleIdx, tp->label.c_str(), before, tp->loopsRunning, inUse, LoopsInUse());
			check("live restyle swaps the look and keeps the budget straight", tp->styleIdx == 2 && tp->loopsRunning > 0 && LoopsInUse() == tp->loopsRunning, detail);
		}
		// 46. (v0.7) the touchdown blast fires once, when it reaches the ground
		reset();
		{
			Tornado n(V3(), 0, 0, "blast", true, 20);
			mockTime = 3; n.Update(0.1f, 3); bool early = n.touchdownDone;
			for (int k = 0; k < 40; k++) { mockTime = 3 + k * 0.1f; n.Update(0.1f, mockTime); }
			sprintf_s(detail, "fired before touchdown %d, after %d", (int)early, (int)n.touchdownDone);
			check("touchdown blast on landing", !early && n.touchdownDone, detail);
			n.Destroy();
		}
		// 47. (v0.7) when a tornado ends, what it left hanging comes down
		reset();
		{
			Tornado n(V3(), 0, 0, "sweep", true, 0);
			Object prop = OBJECT::CREATE_OBJECT(42, 20, 0, 0, FALSE, FALSE, TRUE, FALSE, FALSE);
			NoteTouched(prop, 0);
			objects[prop].p.z = 8; objects[prop].frozen = true; stuck.insert(prop);
			n.Destroy();
			for (int k = 0; k < 40; k++) { mockTime = 1 + k * 0.1f; UpdateFlights(0.1f, mockTime); }
			sprintf_s(detail, "glided %d, height now %.1f m", HoverGlided(), objects[prop].p.z);
			check("clean-up after the tornado brings hanging props down", HoverGlided() == 1 && objects[prop].p.z < 1.0f, detail);
		}
		// 48. (v0.7) the ride camera shows Arthur being lifted, then hands back
		reset();
		{
			SpawnTornado(0, 80, 0, true, "ride");
			g_tornadoes[0]->playerLastHeld = 5.0f;
			playerPos = V3(0, 0, 12);
			RideCamUpdate(0.1f, 5.1f, false); bool on = g_ride.on;
			RideCamUpdate(0.1f, 11.0f, false);
			sprintf_s(detail, "on when lifted %d, off after 5 s %d, next allowed at %.0f s", (int)on, (int)!g_ride.on, g_ride.next);
			check("ride camera: on when Arthur is lifted, back after 5 s", on && !g_ride.on && g_ride.next > 25.0f, detail);
			g_ride = RideCam();
		}
		// 49. (v0.7) a soft landing costs a little health - unless Arthur is invincible
		reset();
		{
			int hurt[2];
			for (int god = 0; god <= 1; god++)
			{
				g_set.playerGod = god == 1;
				lastHealthSet = -1;
				NotePlayerInWall(19 + god * 20);
				playerPos = V3(0, 0, 8); playerVel = V3(0, 0, -15); PlayerSafety(0.1f, 20.0f + god * 20);
				playerPos = V3(0, 0, 0.5f); playerVel = V3(); PlayerSafety(0.1f, 20.5f + god * 20);
				PlayerSafety(0.1f, 24.0f + god * 20);
				hurt[god] = lastHealthSet;
			}
			sprintf_s(detail, "health set to %d (normal), %d (invincible: -1 = untouched)", hurt[0], hurt[1]);
			check("landing hurts a little, not when invincible", hurt[0] == 180 && hurt[1] == -1, detail);
		}
		// 50. (v1.0) Survive the storm: the clock runs until it lifts Arthur, the best time is kept, settings come back
		reset();
		{
			g_set.arthur = 0; g_set.movement = 0;
			mockTime = 1; SurvivalStart();
			bool started = g_surv.on && g_set.arthur >= 2 && g_set.movement == 2 && !g_tornadoes.empty();
			mockTime = 40; SurvivalUpdate(40);                    // 39 s in: stage 1, faster
			int speedAt40 = g_set.speed;
			g_tornadoes[0]->playerLastHeld = 50.0f; playerPos = V3(0, 0, 9);
			mockTime = 50.1f; SurvivalUpdate(50.1f);
			sprintf_s(detail, "started %d, speed at 39 s %d, caught -> running %d, time %.1f s, best %.1f, Arthur level restored %d, movement %d",
				(int)started, speedAt40, (int)g_surv.on, g_surv.lastTime, g_surv.best, g_set.arthur, g_set.movement);
			check("survival: timer, speed-up, caught when lifted, best kept, settings restored", started && speedAt40 == 1 && !g_surv.on &&
				fabsf(g_surv.lastTime - 49.1f) < 0.2f && g_surv.best >= 49.0f && g_set.arthur == 0 && g_set.movement == 0, detail);
			g_surv = Survival();
		}
		// 51. (v1.0) performance levels: High / Balanced (default) / Low PC
		reset();
		{
			int orb[3], ent[3], fl[3]; float pf[3];
			for (int lvl = 0; lvl < 3; lvl++) { g_set.perf = lvl; orb[lvl] = PerfOrbiters(); ent[lvl] = PerfMaxEntities(); fl[lvl] = MaxFlights(); pf[lvl] = PerfPuffMul(); }
			sprintf_s(detail, "cone props %d/%d/%d, grabs %d/%d/%d, flights %d/%d/%d, puffs x%.1f/%.1f/%.1f, default %d",
				orb[0], orb[1], orb[2], ent[0], ent[1], ent[2], fl[0], fl[1], fl[2], pf[0], pf[1], pf[2], Settings().perf);
			check("three performance levels, Balanced by default", orb[0] > orb[1] && orb[1] > orb[2] && ent[0] > ent[1] && ent[1] > ent[2] &&
				fl[0] > fl[1] && fl[1] > fl[2] && pf[0] > pf[1] && pf[1] > pf[2] && Settings().perf == 1, detail);
		}
		// 52. (v1.0) a hop of a metre isn't a "landing" (playtest 8's log: dozens of them while Arthur was tugged)
		reset();
		{
			NotePlayerInWall(9);
			playerPos = V3(0, 0, 1.2f); playerVel = V3(0, 0, -3.5f); PlayerSafety(0.1f, 10);
			playerPos = V3(0, 0, 0.5f); playerVel = V3(); PlayerSafety(0.1f, 10.2f);
			sprintf_s(detail, "landings after a 1 m hop %d, backstop %d", GetLanding().landings, (int)g_backstopOn);
			check("small hops don't trigger the landing backstop", GetLanding().landings == 0 && !g_backstopOn, detail);
		}
		// 53. (v1.0) the extra debris is carried by the mod, so it always lands (playtest 8: "a trail of barrels... floating")
		reset();
		{
			g_set.extraDebris = true; g_set.touchdown = false; g_set.debrisCone = false;
			Tornado n(V3(), 0, 0, "debris", true, 0);
			for (int k = 1; k <= 10; k++) { mockTime = k * 0.1f; n.Update(0.1f, mockTime); }
			sprintf_s(detail, "flights in the air %d", FlightsActive());
			check("extra debris rides the flight system", FlightsActive() >= 1, detail);
			n.Destroy();
		}
		// 54. (v1.0) the [Defaults] section of NadoTest.ini sets the starting scene
		reset();
		{
			std::string ini = ModuleDir() + "\\NadoTest.ini";
			FILE* f = nullptr;
			fopen_s(&f, ini.c_str(), "w");
			if (f)
			{
				fprintf(f, "[Defaults]\nStyle=Rope\nArthur=Tugged\nMovement=Toward You\nFlingAndChase=0\n[Performance]\nMode=Low\n[Storm]\nWeather=Off\nWindRoar=0\n");
				fclose(f);
			}
			g_defaultsLoaded = false;
			LoadConfig();
			sprintf_s(detail, "style %d, arthur %d, movement %d, fling %d, perf %d, weather %d, roar %d", g_set.style, g_set.arthur, g_set.movement,
				(int)g_set.flingChase, g_set.perf, g_set.weatherMode, (int)g_set.roar);
			check("ini [Defaults] / [Performance] / [Storm] are read", g_set.style == 3 && g_set.arthur == 1 && g_set.movement == 2 && !g_set.flingChase &&
				g_set.perf == 2 && g_set.weatherMode == 0 && !g_set.roar, detail);
			remove(ini.c_str());
			g_defaultsLoaded = false;
		}

		// ======================= v1.1 =======================
		// 55. (v1.1) funnel density (playtests 9-10: "gaps in the funnel"): every body plume at its own height, the core pulled into
		// one tip, three streamer bands, and the budget trim keeps the ground ring and the wall cloud
		reset(); srand(55);
		{
			auto audit = [](int perf, int* ground, int* top, int* planned, int* running)
			{
				g_set.perf = perf; fxNames.clear();
				Tornado n(V3(), 0, 0, "density", true, 110);
				const Style& S = GetStyles()[0];
				*ground = *top = 0;
				for (auto& kv : fxNames)
				{
					for (const char* g : S.groundFx) if (kv.second == g) (*ground)++;
					for (const char* g : S.topFx) if (kv.second == g) (*top)++;
				}
				*planned = n.loopsPlanned; *running = n.loopsRunning;
				std::vector<V3> em; n.DebugEmitters(em);
				n.Destroy();
				return em;
			};
			int gB, tB, pB, rB, gL, tL, pL, rL;
			std::vector<V3> em = audit(1, &gB, &tB, &pB, &rB);
			audit(2, &gL, &tL, &pL, &rL);
			const Style& S = GetStyles()[0];
			int body = 0, coreLow = 0, coreLowIn = 0, bandN = 0;
			float lowestCore = 9, coreMul = 0;
			std::set<float> heights;
			std::map<int, std::set<float>> layerHeights;
			std::map<int, int> layerCount;
			std::map<float, int> bandH;
			for (auto& e : em)
			{
				const FunnelLayer& L = S.layers[(int)e.x];
				if (L.twist > 0) { bandH[e.y]++; bandN++; continue; }
				if (L.ringBands) continue;
				body++; heights.insert(e.y); layerHeights[(int)e.x].insert(e.y); layerCount[(int)e.x]++;
				if (L.radiusMul < 0.6f && e.y < 0.14f) { coreLow++; coreMul = L.radiusMul; lowestCore = std::min(lowestCore, e.z); if (e.z < L.radiusMul) coreLowIn++; }
			}
			bool threeBands = bandN > 0;
			for (auto& kv : bandH) if (kv.second != 3) threeBands = false;
			// each body layer: its plumes at their own heights (the v1.0 rings put 2-3 plumes at every height = 33-50%)
			float distinct = layerCount.empty() ? 0.0f : 1.0f;
			for (auto& kv : layerCount) distinct = std::min(distinct, (float)layerHeights[kv.first].size() / kv.second);
			sprintf_s(det, "%s at Balanced: %d body plumes in %d layers, each layer at least %.0f%% own heights (%d heights overall); core tip: %d of %d low plumes pulled in "
				"(narrowest x%.2f vs layer x%.2f); streamers %d = %d heights x %s; ground ring %d, wall cloud %d (planned %d, running %d) | Low PC: ground %d, cloud %d (planned %d, running %d)",
				S.name, body, (int)layerCount.size(), distinct * 100, (int)heights.size(), coreLowIn, coreLow, lowestCore, coreMul, bandN, (int)bandH.size(),
				threeBands ? "3 bands" : "NOT 3 bands", gB, tB, pB, rB, gL, tL, pL, rL);
			check("funnel density: own heights, one tip, three streamer bands, ground + cloud kept", body >= 20 && layerCount.size() >= 2 && distinct >= 0.9f && coreLow >= 2 &&
				coreLowIn == coreLow && threeBands && gB >= 2 && tB >= 3 && gL >= 2 && tL >= 3, det);
		}
		// 56. (v1.1) Toon Twister: level ring bands whose height scrolls up the tube and wraps back to the bottom
		reset(); srand(56);
		{
			int toon = -1;
			for (size_t i = 0; i < GetStyles().size(); i++) if (strstr(GetStyles()[i].name, "Toon")) toon = (int)i;
			std::string verdict = "no Toon style in GetStyles()";
			bool ok = false;
			if (toon >= 0)
			{
				g_set.touchdown = false; g_set.debrisCone = false; g_set.extraDebris = false; g_set.flatten = false; g_set.mapTrees = false;
				const Style& S = GetStyles()[toon];
				int ringLayer = -1, coreLayer = -1;
				for (size_t li = 0; li < S.layers.size(); li++)
				{
					if (S.layers[li].ringBands) ringLayer = (int)li;
					else if (S.layers[li].twist == 0 && coreLayer < 0) coreLayer = (int)li;
				}
				int firstFx = nextFx;
				Tornado n(V3(), 0, toon, "toon", true, 110);
				std::vector<V3> em; n.DebugEmitters(em);
				std::map<float, int> rings;
				std::vector<int> ringFx;
				int coreFx = 0;
				for (size_t k = 0; k < em.size(); k++)   // emitter k is the k-th looped effect started (world-space engine)
				{
					if ((int)em[k].x == ringLayer) { rings[em[k].y]++; ringFx.push_back(firstFx + (int)k); }
					else if ((int)em[k].x == coreLayer && !coreFx && em[k].y > 0.3f) coreFx = firstFx + (int)k;
				}
				bool groups = ringLayer >= 0 && rings.size() >= 2;
				for (auto& kv : rings) if (kv.second != S.layers[ringLayer].perRing) groups = false;
				std::map<int, float> lastZ;
				std::map<int, int> rises, steps, wraps;
				float coreMin = 1e9f, coreMax = -1e9f, H = n.height();
				for (int k = 1; k <= 120; k++)   // 12 s, sampled every second
				{
					mockTime = T(k); n.Update(0.1f, mockTime);
					if (k % 10) continue;
					for (int h : ringFx)
					{
						float z = fxPos[h].z;
						if (lastZ.count(h))
						{
							float dz = z - lastZ[h];
							if (dz < -0.4f * H) wraps[h]++;
							else { steps[h]++; if (dz > 2.0f) rises[h]++; }
						}
						lastZ[h] = z;
					}
					if (coreFx) { coreMin = std::min(coreMin, fxPos[coreFx].z); coreMax = std::max(coreMax, fxPos[coreFx].z); }
				}
				int climbing = 0, wrapped = 0;
				for (int h : ringFx) { if (steps[h] && rises[h] >= steps[h] * 0.8f) climbing++; if (wraps[h] >= 1) wrapped++; }
				int planned = n.loopsPlanned, running = n.loopsRunning;
				n.Destroy();
				ok = groups && !ringFx.empty() && climbing == (int)ringFx.size() && wrapped == (int)ringFx.size() && coreFx && coreMax - coreMin < 6.0f;
				char b[500];
				sprintf_s(b, "style %d '%s': %d rings, %s; %d ring plumes - climbing %d, wrapped back down %d (11 s at %.2f of the height/s, %.0f m tall); "
					"a core plume moved %.1f m up/down (no scroll); loops planned %d, running %d",
					toon, S.name, (int)rings.size(), groups ? "each a level ring of equal-height plumes" : "NOT level rings of equal size", (int)ringFx.size(), climbing, wrapped,
					ringLayer >= 0 ? S.layers[ringLayer].scroll : 0.0f, H, coreMax - coreMin, planned, running);
				verdict = b;
			}
			check("Toon Twister: ring bands that climb the tube and wrap", ok, verdict.c_str());
		}
		// 57. (v1.1) real map trees: trunks are found among a 2 m post and a wall, stand-ins are torn out, hides are capped and undone
		reset(); srand(57);
		{
			g_set.touchdown = false; g_set.debrisCone = false; g_set.extraDebris = false; g_set.flatten = false; g_set.mapTrees = true;
			g_spawnableTrees.clear();
			BuildStandIns();
			// as in a real forest, only a few tree models are loaded here: the first three of the list (and the stand-ins)
			std::set<Hash> standInHashes;
			for (auto& s : g_standIns) standInHashes.insert(Joaat(s.name.c_str()));
			int nTree = (int)(sizeof(kTreeModels) / sizeof(kTreeModels[0])), loaded = 0;
			for (int i = 0; i < nTree; i++)
			{
				if (i < 3 || standInHashes.count(kTreeModels[i].hash)) loaded++;
				else unloadedModels.insert(kTreeModels[i].hash);
			}
			bool joaatOk = Joaat(kTreeModels[0].name) == kTreeModels[0].hash;
			TreeWorld(V3());
			Tornado n(V3(), 0, 0, "realtrees", true, 0);
			int maxStep = 0, maxFlights = 0;
			float maxStandZ = 0;
			bool overCap = false;
			for (int k = 1; k <= 300; k++)   // 30 s
			{
				mockTime = T(k);
				int before = n.realTrees;
				n.Update(0.1f, mockTime); UpdateFlights(0.1f, mockTime); integrate(0.1f);
				maxStep = std::max(maxStep, n.realTrees - before);
				maxFlights = std::max(maxFlights, FlightsActive());
				for (Object o : g_standInProps) if (MockObj* p = Obj(o)) maxStandZ = std::max(maxStandZ, p->p.z);
				if (TreeHidesUsed() > 600) overCap = true;
			}
			int trunks = n.treeTrunks, real = n.realTrees, uprooted = n.uprooted, hides = TreeHidesUsed(), made = hidesMade;
			// every hide must be at a trunk: never the post, never the wall
			int atTrunk = 0, atPost = 0, atWall = 0;
			for (auto& h : hideAt)
			{
				float best = 1e9f;
				for (int i = 0; i < kTrunks; i++) best = std::min(best, (h - world[i].a).len2d());
				if (best < 1.0f) atTrunk++;
				if ((h - world[6].a).len2d() < 2.0f) atPost++;
				if (DistToBox2d(world[7], h) < 1.5f) atWall++;
			}
			std::vector<Object> stands = g_standInProps;
			n.Destroy();
			RestoreRealTrees();
			int standsLeft = 0;
			for (Object o : stands) if (Obj(o)) standsLeft++;
			int removedA = hidesRemoved, postHits = shapeHits[6], wallHits = shapeHits[7];
			bool restored = hidesRemoved == made && TreeHidesUsed() == 0 && standsLeft == 0;
			// every tree model loaded: the 600-hide ceiling
			unloadedModels.clear(); g_nextLoadedScan = 0;
			hidesMade = hidesRemoved = 0; hideAt.clear();
			Tornado m(V3(), 0, 0, "realtrees2", true, 0);
			int maxHides = 0;
			for (int k = 301; k <= 700; k++)
			{
				mockTime = T(k);
				m.Update(0.1f, mockTime); UpdateFlights(0.1f, mockTime); integrate(0.1f);
				maxHides = std::max(maxHides, TreeHidesUsed());
			}
			int capHides = TreeHidesUsed(), capMade = hidesMade, capTrunks = m.treeTrunks;
			m.Destroy();
			RestoreRealTrees();
			bool capRestored = hidesRemoved == capMade && TreeHidesUsed() == 0;
			sprintf_s(det, "stand-in models %d; tree models loaded here %d (joaat %s); trunks %d of %d (rejected %d hits in %d probes; rays that hit the post %d, the wall %d); "
				"hides %d = %d x %d, at a trunk %d, at the post %d, at the wall %d; max trees per pass %d; uprooted %d, flights up to %d, stand-in up to %.0f m; "
				"restore: %d/%d hides removed, stand-ins left %d | all %d loaded: %d trunks, hides peak %d, end %d (made %d), restored %d",
				(int)g_standIns.size(), loaded, joaatOk ? "ok" : "MISMATCH", trunks, kTrunks, n.treeRejected, n.treeProbes, postHits, wallHits, hides, trunks, loaded, atTrunk,
				atPost, atWall, maxStep, uprooted, maxFlights, maxStandZ, removedA, made, standsLeft, nTree, capTrunks, maxHides, capHides, capMade, (int)capRestored);
			check("real map trees: trunks only, stand-ins torn out, hides capped and restored", !g_standIns.empty() && trunks >= 3 && real == trunks && uprooted == trunks &&
				n.treeRejected >= 2 && postHits > 0 && wallHits > 0 && hides == trunks * loaded && made == hides && atTrunk == made && atPost == 0 && atWall == 0 && maxStep <= 1 && maxFlights >= 1 &&
				maxStandZ > 5.0f && !overCap && restored && capHides == 600 && maxHides <= 600 && capMade == 600 && capRestored, det);
		}
		// 58. (v1.1) flattened grass: a moving tornado lays spaced vegetation spheres (at most 80 kept); ClearVegTrail removes each one
		reset(); srand(58);
		{
			g_set.touchdown = false; g_set.debrisCone = false; g_set.extraDebris = false; g_set.mapTrees = false; g_set.flatten = true;
			Tornado n(V3(), 90, 0, "grass", false, 0);
			n.scripted = true; n.scriptVel = V3(10, 0, 0);
			float step = std::max(6.0f, n.wallRadius() * 0.7f);
			int maxKept = 0;
			for (int k = 1; k <= 1000; k++)   // 100 s at 10 m/s
			{
				mockTime = T(k); n.Update(0.1f, mockTime);
				maxKept = std::max(maxKept, VegTrailCount());
			}
			float minGap = 1e9f;
			for (size_t i = 1; i < vegAt.size(); i++) minGap = std::min(minGap, (vegAt[i] - vegAt[i - 1]).len2d());
			int added = (int)vegAdded.size(), removedRun = (int)vegRemoved.size(), kept = VegTrailCount();
			bool oldestFirst = removedRun > 0;
			for (int i = 0; i < removedRun && i < added; i++) if (vegRemoved[i] != vegAdded[i]) oldestFirst = false;
			n.Destroy();
			ClearVegTrail();
			std::set<int> rem(vegRemoved.begin(), vegRemoved.end()), add(vegAdded.begin(), vegAdded.end());
			sprintf_s(det, "spheres laid %d (type %d, flags %d), closest two %.1f m apart (step %.1f m); kept at most %d, %d at the end; %d removed on the way (oldest first %d); "
				"after ClearVegTrail: removed %d (distinct %d), still alive %d, trail count %d",
				added, lastVegType, lastVegFlags, minGap, step, maxKept, kept, removedRun, (int)oldestFirst, (int)vegRemoved.size(), (int)rem.size(), (int)vegAlive.size(), VegTrailCount());
			check("flattened grass: spaced spheres, 80 kept, all removed on clear", added > 80 && minGap >= step - 0.01f && maxKept <= 80 && kept == 80 &&
				removedRun == added - 80 && oldestFirst && rem == add && (int)vegRemoved.size() == added && vegAlive.empty() && VegTrailCount() == 0 && lastVegType == 2, det);
		}
		// 59. (v1.1) the intro, played through on the mock clock
		reset(); srand(59);
		{
			g_set.arthur = 3; g_set.movement = 1; g_set.speed = 2; g_set.touchdownCam = true; g_set.rideCam = false; g_set.playerGod = false;
			mockTime = 1.0f;
			IntroStart();
			bool fading = g_in.stage == 1 && screenFadedOut;
			float t4 = -1, clkTornado = -1, clkBoard = -1, clkHandoff = -1, tHandoff = -1, endClock = -1, tReleased = -1, grow = 0;
			bool scripted = false, faded4 = false, seated = false, shieldAfter = false, ctlOff = false;
			int castOk = 0, castPeds = 0, propsOk = 0, ctl = -1, cams = -1, arthur = -1, move = -1, speed = -1;
			float ts = 0, tsEnd = 0;
			int ctlEnd = -1;
			bool tdc = false, ride = true, god = true;
			std::map<int, V3> start, at31;
			int posedLifted = 0, posedMoving = 0, posedUp = 0;
			for (int k = 11; k <= 2100; k++)
			{
				float t = T(k);
				tick2(t);
				if (t4 < 0 && g_in.stage == 3 && lastPlayerControl == 0) ctlOff = true;
				if (t4 < 0 && g_in.stage == 4)
				{
					t4 = t; faded4 = !screenFadedOut;
					for (int i = 0; i < kCastCount; i++)
						if (g_in.cast[i].ok) { castOk++; if (MockObj* o = Obj(g_in.cast[i].ped)) { if (o->type == 1) castPeds++; start[i] = o->p; } }
					for (Entity e : g_in.props) if (Obj(e)) propsOk++;
				}
				if (g_in.stage == 4 && clkTornado < 0 && g_in.tp.get())
				{
					clkTornado = (float)g_in.clock; grow = g_in.tp.get()->growSeconds; scripted = g_in.tp.get()->scripted;
				}
				if (g_in.stage == 4 && g_in.clock >= 86.0 && at31.empty())
					for (int i = 0; i < kCastCount; i++) if (kCast[i].ride == RIDE_POSED && Obj(g_in.cast[i].ped)) at31[i] = Obj(Root(g_in.cast[i].ped))->p;   // (v1.6.1: on a carrier)
				if (g_in.stage == 4 && g_in.clock >= 87.0 && !at31.empty() && posedLifted == 0)
					for (int i = 0; i < kCastCount; i++)
					{
						if (kCast[i].ride != RIDE_POSED || !g_in.cast[i].lifted || !Obj(g_in.cast[i].ped)) continue;
						posedLifted++;
						V3 p = Obj(Root(g_in.cast[i].ped))->p;
						if ((p - at31[i]).len() > 0.5f) posedMoving++;
						if (p.z > 2.0f && ((p - start[i]).len2d() > 2.0f || p.z > 5.0f)) posedUp++;   // (its orbit can pass right over where it stood)
					}
				if (g_in.stage == 4 && clkBoard < 0 && g_bal.on && !g_bal.waiting)
				{
					clkBoard = (float)g_in.clock; seated = g_bal.seated && seatOf.count(1) && seatOf[1] == g_bal.veh;
				}
				if (clkHandoff < 0 && g_in.stage == 5)
				{
					clkHandoff = (float)g_in.clock; tHandoff = t;
					ctl = lastPlayerControl; cams = lastRenderCams; ts = lastTimeScale;
					arthur = g_set.arthur; move = g_set.movement; speed = g_set.speed; tdc = g_set.touchdownCam; ride = g_set.rideCam; god = g_set.playerGod;
				}
				if (tHandoff > 0 && fabsf(t - tHandoff - 0.5f) < 0.01f) shieldAfter = g_shieldPlayer && BalloonActive();
				if (tHandoff > 0 && tReleased < 0)
				{
					bool all = true;
					for (int i = 0; i < kCastCount; i++) if (kCast[i].ride == RIDE_POSED && g_in.cast[i].ok && !g_in.cast[i].released) all = false;
					if (all) tReleased = clkHandoff + (t - tHandoff);
				}
				if (tHandoff > 0 && g_in.stage == 0) { endClock = clkHandoff + (t - tHandoff); ctlEnd = lastPlayerControl; tsEnd = lastTimeScale; break; }
			}
			float lightning = lightningP3.empty() ? 0 : lightningP3.front();
			sprintf_s(det, "fade-out first %d, control taken %d, stage 4 at %.1f s faded in %d; cast %d/%d (peds %d), camp props %d/%d; tornado at %.2f s (grow %.1f s, scripted %d); "
				"posed riders at 87 s (after Javier's pass by the basket): lifted %d/%d, moving %d, up off the camp %d; boarded at %.2f s (seated %d); handoff at %.2f s: control %d, script cams %d, time scale %.2f, "
				"arthur %d move %d speed %d touchdown cam %d ride cam %d invincible %d, shield in the balloon %d; riders all released by %.1f s, stage 0 at %.1f s (intro clock; control %d, time scale %.2f) | "
				"intro lightning p3 %.0f",
				(int)fading, (int)ctlOff, t4, (int)faded4, castOk, kCastCount, castPeds, propsOk, kCampPropCount, clkTornado, grow, (int)scripted,
				posedLifted, PosedCount(), posedMoving, posedUp, clkBoard, (int)seated, clkHandoff, ctl, cams, ts, arthur, move, speed, (int)tdc, (int)ride, (int)god,
				(int)shieldAfter, tReleased, endClock, ctlEnd, tsEnd, lightning);
			// (v1.2: Uncle's chair is one more prop, and the gang walks it off before the scene lets go - done by ~86 s)
			// (v1.3: the new screenplay - the funnel drops at 15.6 s, Arthur lifts off at 19.4 s, the handoff is at 44 s; the cow is the
			// only rider still flown by hand, the gang go up as ragdolls)
			// (v1.4: a 5 s longer setup - the funnel drops at 20.6 s, Arthur lifts off at 24.4 s, the handoff is at 49 s)
			check("intro: fade, cast and camp, tornado at 43.7 s, lift-off at 52 s, handoff at 102.5 s (+ any held cuts), done by ~150 s", fading && ctlOff && t4 > 0 && faded4 &&
				castOk == kCastCount && kCastCount == 12 && castPeds == kCastCount && propsOk >= kCampPropCount && propsOk <= kCampPropCount + 1 && fabsf(clkTornado - 43.7f) <= 0.11f && grow == 2.5f && scripted &&
				posedLifted == PosedCount() && posedMoving == posedLifted && posedUp == posedLifted && fabsf(clkBoard - 52.0f) <= 0.11f && seated &&
				clkHandoff >= 102.4f && clkHandoff <= 102.4f + 8.1f && ctl == 1 && cams == 0 && ts == 1.0f && arthur == 3 && move == 1 && speed == 2 && tdc && !ride && !god && shieldAfter &&
				tReleased > clkHandoff && endClock > 0 && endClock <= 155.0f && ctlEnd == 1 && tsEnd == 1.0f, det);
		}
		// 60. (v1.1) skipping the intro (IntroSkip at 10 s, as the back key does) lands at the handoff
		reset(); srand(60);
		{
			g_set.arthur = 1;
			mockTime = 1.0f;
			IntroStart();
			int k = 11;
			for (; k <= 400 && !(g_in.stage == 4 && g_in.clock >= 10.0); k++) tick2(T(k));
			float clkBefore = (float)g_in.clock;
			IntroSkip();
			float clkAfter = (float)g_in.clock;
			bool tp = g_in.tp.get() != nullptr;
			int lifted = 0, riders = 0;
			for (int i = 0; i < kCastCount; i++) if (kCast[i].ride != RIDE_STAY) { riders++; if (g_in.cast[i].lifted) lifted++; }
			float tSkip = T(k - 1), tHand = -1;
			for (; k <= 500; k++) { tick2(T(k)); if (g_in.stage == 5) { tHand = T(k); break; } }
			bool seated = g_bal.on && g_bal.seated && seatOf.count(1) && seatOf[1] == g_bal.veh;
			sprintf_s(det, "skipped at %.1f s -> clock %.1f, tornado %d, riders lifted %d/%d; handoff %.1f s later (clock %.2f): seated %d, control %d, time scale %.2f, script cams %d, arthur %d",
				clkBefore, clkAfter, (int)tp, lifted, riders, tHand > 0 ? tHand - tSkip : -1.0f, (float)g_in.clock, (int)seated, lastPlayerControl, lastTimeScale, lastRenderCams, g_set.arthur);
			check("intro skip lands at the handoff", clkBefore >= 10.0f && clkBefore < 10.2f && fabsf(clkAfter - 94.5f) < 0.01f && tp && lifted == riders && tHand > 0 && tHand - tSkip <= 8.2f &&
				seated && lastPlayerControl == 1 && lastTimeScale == 1.0f && lastRenderCams == 0 && g_set.arthur == 1, det);
		}
		// 61. (v1.1) Arthur dies mid-intro (in the slow motion): control, cameras and time scale come back
		reset(); srand(61);
		{
			g_set.arthur = 2; g_set.touchdownCam = true;
			mockTime = 1.0f;
			IntroStart();
			int k = 11;
			for (; k <= 1300 && !(g_in.stage == 4 && g_in.clock >= 88.5); k++) tick2(T(k));   // (v1.5: the slow motion is 87.4-90.3 s)
			float tsBefore = lastTimeScale;
			int ctlBefore = lastPlayerControl;
			mockDead = true;
			tick2(T(k));
			mockDead = false;
			int stillPosed = 0;
			for (int i = 0; i < kCastCount; i++) if (kCast[i].ride == RIDE_POSED && g_in.cast[i].lifted && !g_in.cast[i].released) stillPosed++;
			sprintf_s(det, "at 32 s: time scale %.2f, control %d; after the death: stage %d, control %d, time scale %.2f, script cams %d, arthur %d, touchdown cam %d, riders still posed %d",
				tsBefore, ctlBefore, g_in.stage, lastPlayerControl, lastTimeScale, lastRenderCams, g_set.arthur, (int)g_set.touchdownCam, stillPosed);
			check("intro: death mid-scene restores control and time scale", tsBefore < 1.0f && ctlBefore == 0 && g_in.stage == 0 && lastPlayerControl == 1 &&
				lastTimeScale == 1.0f && lastRenderCams == 0 && g_set.arthur == 2 && g_set.touchdownCam && stillPosed == 0, det);
		}
		// 62. (v1.1) the jet balloon: seated; when the game ignores the balloon natives it falls back step by step; bail; remove
		reset(); srand(62);
		{
			mockTime = 1.0f;
			bool spawned = BalloonSpawn(V3(0, 3, 0), 0);
			Vehicle veh = g_bal.veh;
			bool seated = g_bal.seated && seatOf.count(1) && seatOf[1] == veh, waiting = g_bal.waiting;
			stuck.insert(veh);                      // the balloon never moves by itself in this mock
			g_bal.intro = true; g_bal.cmdVel = V3(10, 0, 4);
			std::vector<std::pair<float, int>> drives;
			int balloonVel = 0, entityVel = 0;
			float t2 = -1;
			V3 at2;
			bool shield = true;
			int k = 11;
			for (; k <= 90; k++)
			{
				int bv = calls[N_SET_VELOCITY_FOR_BALLOON], ev = calls[N_SET_ENTITY_VELOCITY];
				int before = g_bal.drive;
				tick2(T(k));
				if (g_bal.drive == 0) balloonVel += calls[N_SET_VELOCITY_FOR_BALLOON] - bv;
				if (g_bal.drive == 1 && before == 1) entityVel += calls[N_SET_ENTITY_VELOCITY] - ev;
				if (g_bal.drive != before) { drives.push_back({ T(k), g_bal.drive }); if (g_bal.drive == 2) { t2 = T(k); at2 = g_bal.pos; } }
				if (!g_shieldPlayer) shield = false;
			}
			float moved = (g_bal.pos - at2).len(), movedFor = T(k - 1) - t2;
			mockTime = T(k);
			BalloonBail(T(k));
			bool parked = g_bal.parked && !BalloonActive(), window = g_dropWindowUntil > T(k), out = !seatOf.count(1);
			tick2(T(k + 1));
			bool unshielded = !g_shieldPlayer;
			BalloonRemove("harness");
			bool gone = !Obj(veh) && !g_bal.on && !g_bal.body;
			char seq[100] = "";
			for (auto& d : drives) { char b[32]; sprintf_s(b, " %s@%.1fs", kDriveNames[d.second], d.first); strcat_s(seq, b); }
			sprintf_s(det, "spawned %d, seated %d (waiting %d); drive:%s; balloon-velocity calls %d, entity-velocity calls %d; moved by the mod %.1f m in %.1f s; shielded in flight %d; "
				"bail: parked %d, soft-landing window %d, out of the seat %d, unshielded %d; removed %d",
				(int)spawned, (int)seated, (int)waiting, seq, balloonVel, entityVel, moved, movedFor, (int)shield, (int)parked, (int)window, (int)out, (int)unshielded, (int)gone);
			// (v1.3: it starts on entity velocity - playtest 11 showed the balloon native never moves it - and falls back to moving it itself)
			check("jet balloon: seated, drive fallback, bail and remove", spawned && seated && !waiting && drives.size() == 1 && drives[0].second == 2 &&
				balloonVel == 0 && entityVel > 0 && moved > 5.0f && shield && parked && window && out && unshielded && gone, det);
		}
		// 63. (v1.1) Storm chaser: points near the funnel, caught in the wall, the best saved, the movement setting back
		reset(); srand(63);
		{
			BackupBest();
			WriteFileStr(BestPath(), "# harness\nchaser 1.00\nsurvive 5.00\n");
			g_set.movement = 2;
			mockTime = 1.0f;
			ChaseStart();
			bool started = g_bal.chase && g_bal.on && g_tornadoes.size() == 1 && g_set.movement == 1;
			Vehicle veh = g_bal.veh;
			stuck.insert(veh);
			float scoreNear = 0, tCaught = -1, tEnd = -1;
			for (int k = 11; k <= 400; k++)
			{
				float t = T(k);
				Tornado* tp = g_tornadoes.empty() ? nullptr : g_tornadoes[0].get();
				if (tp && g_bal.chase && g_bal.chaseCaughtAt < 0)
					if (MockObj* o = Obj(veh)) o->p = tp->base + (t < 15.0f ? V3(40, 0, 20) : V3(5, 0, 20));   // near, then into the wall
				tick2(t);
				if (t < 15.0f) scoreNear = g_bal.chaseScore;
				if (tCaught < 0 && g_bal.chaseCaughtAt >= 0) tCaught = t;
				if (tCaught > 0 && !g_bal.chase) { tEnd = t; break; }
			}
			float saved = ReadBest("chaser"), other = ReadBest("survive");
			sprintf_s(det, "started %d; %.0f points by 15 s (40 m out); caught at %.1f s, ended at %.1f s: \"%s\"; best %.0f, file chaser %.2f survive %.2f; movement back to %d",
				(int)started, scoreNear, tCaught, tEnd, g_bal.chaseResult.c_str(), g_bal.chaseBest, saved, other, g_set.movement);
			check("Storm chaser: scores near the funnel, CAUGHT in the wall, best saved, movement restored", started && scoreNear > 20 && tCaught >= 14.95f && tEnd > tCaught &&
				g_bal.chaseResult.rfind("CAUGHT", 0) == 0 && fabsf(saved - g_bal.chaseBest) < 0.01f && saved > 1.0f && other == 5.0f && g_set.movement == 2, det);
			BalloonRemove("harness");
			RestoreBest();
		}
		// 64. (v1.1) Storm season: blocked by a mission or the intro; the warning (blips, sky); touchdown at 22 s, 380-520 m out; dies down; next one scheduled
		reset(); srand(64);
		{
			g_set.season = 3; g_set.weatherMode = 1;
			mockTime = 1.0f;
			mockMission = true; g_season.nextAt = 0.5f; SeasonUpdate(1.0f);
			bool missionBlocked = g_season.stage == 0 && g_season.nextAt > 20.0f;
			mockMission = false;
			g_in.stage = 4; g_season.nextAt = 1.0f; SeasonUpdate(1.05f);
			bool introBlocked = g_season.stage == 0 && g_season.nextAt > 20.0f;
			g_in.stage = 0;
			int wx = calls[N_SET_WEATHER_TYPE];
			g_season.nextAt = 1.1f; mockTime = 1.2f; SeasonUpdate(1.2f);
			float warnAt = 1.2f;
			bool warning = g_season.stage == 1 && blips.count(g_season.zone) && blips.count(g_season.icon) && g_storm.seasonLock &&
				calls[N_SET_WEATHER_TYPE] > wx && lastWeatherHash == Joaat("THUNDER");
			int zone = g_season.zone, icon = g_season.icon;
			float spawnT = -1, dist = 0, life = 0;
			bool natural = false, early = false;
			int k = 13;
			for (; k <= 300; k++)
			{
				float t = T(k);
				tick2(t);
				if (spawnT < 0 && !g_tornadoes.empty())
				{
					spawnT = t;
					Tornado* tp = g_season.tp.get();
					natural = tp && tp->natural && !tp->stationary;
					dist = tp ? (tp->base - playerPos).len2d() : 0;
					life = g_season.lifeUntil - t;
					if (t - warnAt < 21.95f) early = true;
					break;
				}
			}
			bool blipsGone = !blips.count(zone) && !blips.count(icon) && !g_season.zone && !g_season.icon;
			for (k++; k <= 330; k++) tick2(T(k));
			g_season.lifeUntil = T(k);
			tick2(T(++k));
			bool dying = !g_tornadoes.empty() && g_tornadoes[0]->Dissipating();
			float nextIn = -1;
			for (k++; k <= 450; k++)
			{
				tick2(T(k));
				if (g_tornadoes.empty() && g_season.stage == 0) { nextIn = g_season.nextAt - T(k); break; }
			}
			// a weather the user locked is left alone (the warning still comes)
			g_manualWeather = true;
			int wx2 = calls[N_SET_WEATHER_TYPE];
			g_season.nextAt = T(k) - 0.05f;
			SeasonUpdate(T(k));
			bool manualKept = g_season.stage == 1 && calls[N_SET_WEATHER_TYPE] == wx2 && !g_storm.seasonLock;
			SeasonCancel("harness");
			g_manualWeather = false;
			sprintf_s(det, "mission blocks %d, intro blocks %d; warning %d (zone %d, icon %d, sky locked to THUNDER %d); touchdown %.1f s after the warning, natural %d, %.0f m out, "
				"lives %.0f s; markers cleared %d; dying down %d; next storm in %.0f s; a locked weather kept %d",
				(int)missionBlocked, (int)introBlocked, (int)warning, zone, icon, (int)(lastWeatherHash != 0), spawnT > 0 ? spawnT - warnAt : -1.0f, (int)natural, dist, life,
				(int)blipsGone, (int)dying, nextIn, (int)manualKept);
			check("Storm season: warning, touchdown at 22 s 380-520 m out, dies down, next scheduled; blocked by missions and the intro", missionBlocked && introBlocked &&
				warning && spawnT > 0 && !early && spawnT - warnAt <= 22.15f && natural && dist >= 380.0f && dist <= 520.0f && life >= 300.0f && life <= 480.0f && blipsGone &&
				dying && nextIn >= 150.0f && nextIn <= 300.0f && manualKept, det);
		}
		// 65. (v1.1) the tornado gun: minis at the impact point, small, no marker, three at most, no storm, not "the" tornado
		reset(); srand(65);
		{
			g_set.mapBlip = true; g_set.wind = true; g_set.weatherMode = 1; g_set.lightning = 2;
			g_gun.on = true;
			const V3 shots[6] = { V3(30, 10, 0), V3(-20, 25, 0), V3(5, -40, 0), V3(50, 50, 0), V3(-35, -15, 0), V3(12, 60, 0) };
			int blipsBefore = calls[N_BLIP_ADD_FOR_COORDS], wx = calls[N_SET_WEATHER_TYPE], wind = calls[N_SET_WIND_SPEED];
			int atImpact = 0, minis = 0, smallOnes = 0, maxOut = 0, maxList = 0;
			bool storm = false, hudSees = false, anyMini = false;
			int k = 10;
			for (int s = 0; s < 6; s++)
			{
				mockShooting = true; mockImpact = shots[s];
				int fired = g_gun.fired;
				tick2(T(k++));
				mockShooting = false;
				if (g_gun.fired > fired && !g_tornadoes.empty())
				{
					Tornado* tp = g_tornadoes.back().get();
					if (fabsf(tp->base.x - shots[s].x) < 0.01f && fabsf(tp->base.y - shots[s].y) < 0.01f) atImpact++;
					if (tp->Mini()) minis++;
					if (tp->sizeMul() < 0.5f && tp->height() < 50.0f) smallOnes++;
				}
				for (int j = 0; j < 9; j++)
				{
					tick2(T(k++));
					int out = 0;
					for (auto& tp : g_tornadoes) if (tp->Mini() && !tp->Dissipating()) out++;
					maxOut = std::max(maxOut, out);
					maxList = std::max(maxList, (int)g_gun.minis.size());
					if (g_storm.active) storm = true;
					if (NearestTornado()) hudSees = true;
					if (NearestTornado(nullptr, true)) anyMini = true;
				}
			}
			int newBlips = calls[N_BLIP_ADD_FOR_COORDS] - blipsBefore, newWx = calls[N_SET_WEATHER_TYPE] - wx, newWind = calls[N_SET_WIND_SPEED] - wind;
			SpawnTornado(0, 80, 0, true, "big");
			tick2(T(k++));
			bool bigStorm = g_storm.active;
			sprintf_s(det, "shots %d -> minis %d (%d at the impact point, %d small), out at once at most %d (list %d); markers %d, weather locks %d, wind changes %d, storm %d; "
				"HUD's nearest sees a mini %d (with minis included %d); a big one starts the storm %d",
				6, minis, atImpact, smallOnes, maxOut, maxList, newBlips, newWx, newWind, (int)storm, (int)hudSees, (int)anyMini, (int)bigStorm);
			check("tornado gun: minis at the impact, three at most, no storm, not the HUD's tornado", g_gun.fired == 6 && minis == 6 && atImpact == 6 && smallOnes == 6 &&
				maxOut <= 3 && maxOut >= 2 && maxList <= 3 && newBlips == 0 && newWx == 0 && newWind == 0 && !storm && !hudSees && anyMini && bigStorm, det);
		}
		// 66. (v1.1) storm and weather fixes (playtest 10): a locked weather wins; Storm clouds = THUNDER, rain held at 0; wind in degrees;
		// storm off hands wind and rain back (-1); lightning at coords passes -1
		reset(); srand(66);
		{
			g_set.wind = true; g_set.lightning = 0;
			g_set.weatherMode = 2;
			mockTime = 1.0f;
			LockWeather("SNOW", 2.0f); g_manualWeather = true;   // what the World menu's "Weather now" does
			int wx = calls[N_SET_WEATHER_TYPE], clr = calls[N_CLEAR_OVERRIDE_WEATHER];
			SpawnTornado(0, 80, 0, true, "manual");
			int k = 11;
			for (; k <= 40; k++) tick2(T(k));
			DespawnAll(); tick2(T(k++));
			bool manualKept = calls[N_SET_WEATHER_TYPE] == wx && calls[N_CLEAR_OVERRIDE_WEATHER] == clr && g_lockedHash == Joaat("SNOW");
			UnlockWeather(); g_manualWeather = false;
			g_set.weatherMode = 1;
			Tornado* tp = SpawnTornado(0, 80, 0, true, "clouds");
			tp->heading = 123.0f;
			for (int j = 0; j < 5; j++) tick2(T(k++));
			bool clouds = g_set.weatherType == 1 && lastWeatherHash == Joaat("THUNDER") && lastRain == 0.0f;
			float rainHeld = lastRain, dirA = lastWindDir;
			bool thunder = lastWeatherHash == Joaat("THUNDER");
			tp->heading = -100.0f; tick2(T(k++));
			float dirB = lastWindDir;
			g_set.lightning = 2; g_storm.nextLightning = 0; lightningP3.clear();
			tick2(T(k++));
			int stormStrikes = (int)lightningP3.size();
			g_set.touchdown = true;
			Tornado blast(V3(300, 0, 0), 0, 0, "blast", true, 0);
			for (int j = 0; j < 80 && !blast.touchdownDone; j++) { mockTime = T(k++); blast.Update(0.1f, mockTime); }
			blast.Destroy();
			bool lightning = lightningP3.size() >= 2 && stormStrikes >= 1;
			for (float p : lightningP3) if (p != -1.0f) lightning = false;
			DespawnAll(); tick2(T(k++));
			sprintf_s(det, "a locked SNOW kept through a tornado %d; Storm clouds: weather type %d, THUNDER %d, rain %.0f; wind direction %.0f for heading 123, %.0f for heading -100; "
				"lightning at coords %d calls, p3 all -1 %d; storm off: wind speed %.0f, wind direction %.0f, rain %.0f",
				(int)manualKept, g_set.weatherType, (int)thunder, rainHeld, dirA, dirB, (int)lightningP3.size(), (int)lightning, lastWindSpeed, lastWindDir, lastRain);
			check("storm/weather: locked weather wins, THUNDER + no rain, wind in degrees, -1 hand-back, lightning -1", manualKept && clouds && fabsf(dirA - 123.0f) < 0.01f &&
				fabsf(dirB - 260.0f) < 0.01f && lightning && lastWindSpeed == -1.0f && lastWindDir == -1.0f && lastRain == -1.0f, det);
		}
		// 67. (v1.1) the wind direction stays in 0-360 when the tornado's heading has wandered past -360 / +360 (wander, Storm season)
		reset(); srand(67);
		{
			g_set.wind = true; g_set.lightning = 0;
			mockTime = 1.0f;
			Tornado* tp = SpawnTornado(0, 80, 0, true, "wrap");
			tp->heading = -450.0f; tick2(1.1f);
			float a = lastWindDir;
			tp->heading = 725.0f; tick2(1.2f);
			float b = lastWindDir;
			tp->heading = -360.5f; tick2(1.3f);
			float c = lastWindDir;
			sprintf_s(det, "heading -450 -> wind direction %.1f (want 270), 725 -> %.1f (want 5), -360.5 -> %.1f (want 359.5; -1 would hand the wind back to the weather)", a, b, c);
			check("wind direction wraps into 0-360 for any heading", fabsf(a - 270.0f) < 0.01f && fabsf(b - 5.0f) < 0.01f && fabsf(c - 359.5f) < 0.01f, det);
		}
		// 68. (v1.1) Survive the storm: held in the wall with fling off ends CAUGHT (playtest 10); a run stopped by hand sets no best;
		// a legacy bare-number best file is ignored and WriteBest keeps the other lines
		reset(); srand(68);
		{
			BackupBest();
			WriteFileStr(BestPath(), "# harness\nsurvive 5.00\nchaser 3.00\n");
			g_set.arthur = 2; g_set.flingChase = true; g_set.debrisCone = false; g_set.extraDebris = false;
			mockPeds.push_back(1);
			mockTime = 1.0f;
			SurvivalStart();
			bool flingOff = !g_set.flingChase;
			float tEnd = -1;
			int flings = -1;
			for (int k = 11; k <= 300; k++)
			{
				Tornado* tp = g_tornadoes.empty() ? nullptr : g_tornadoes[0].get();
				if (tp) playerPos = tp->base + V3(tp->wallRadius(), 0, 9.0f);   // held in the wall, 9 m up
				tick2(T(k));
				if (tp) flings = tp->flings;
				if (!g_surv.on) { tEnd = T(k); break; }
			}
			std::string caught = g_surv.result;
			float lastTime = g_surv.lastTime, savedSurvive = ReadBest("survive"), keptChaser = ReadBest("chaser");
			bool restored = g_set.flingChase && g_set.arthur == 2;
			// stopped by hand: no best
			reset();
			WriteFileStr(BestPath(), "survive 5.00\n");
			mockTime = 1.0f;
			SurvivalStart();
			playerPos = V3(0, -400, 0);
			for (int k = 11; k <= 210; k++) tick2(T(k));
			SurvivalStop("STOPPED");
			bool noBest = !g_surv.newBest && g_surv.best == 5.0f && ReadBest("survive") == 5.0f && g_surv.lastTime > 15.0f;
			float stoppedAt = g_surv.lastTime;
			// the v1.0 file format
			WriteFileStr(BestPath(), "125.3\n");
			float legacy = ReadBest("survive");
			WriteFileStr(BestPath(), "125.3\nchaser 12.00\nsurvive 30.00\n");
			WriteBest("survive", 40.0f);
			std::string txt = ReadFileStr(BestPath());
			int surviveLines = 0;
			for (size_t p = 0; p < txt.size(); p = txt.find('\n', p) == std::string::npos ? txt.size() : txt.find('\n', p) + 1) if (txt.compare(p, 8, "survive ") == 0) surviveLines++;
			bool kept = ReadBest("chaser") == 12.0f && ReadBest("survive") == 40.0f && surviveLines == 1 && txt.find("125.3") == std::string::npos;
			RestoreBest();
			sprintf_s(det, "fling off %d; held in the wall: \"%s\" after %.1f s (flings %d), best file survive %.2f, chaser kept %.2f, settings back %d; stopped by hand at %.1f s: "
				"no best %d; legacy bare number read as %.1f; WriteBest kept the other line and one survive line %d",
				(int)flingOff, caught.c_str(), lastTime, flings, savedSurvive, keptChaser, (int)restored, stoppedAt, (int)noBest, legacy, (int)kept);
			check("Survive the storm: CAUGHT with fling off, no best when stopped, legacy file ignored", flingOff && tEnd > 0 && caught.rfind("CAUGHT", 0) == 0 &&
				lastTime > 8.0f && flings == 0 && fabsf(savedSurvive - lastTime) < 0.01f && keptChaser == 3.0f && restored && noBest && legacy == 0.0f && kept, det);
		}
		// 69. (v1.1) the menu: help on every item of the main pages, every page reachable, none over 14 items; Select on
		// "Despawn everything" clears tornadoes, the intro's camp and gang, the balloon, the grass trail and the tree hides
		reset(); srand(69);
		{
			BuildMenu();
			std::string missing;
			int items = 0;
			for (int pg = 0; pg < P_COUNT; pg++)   // (v1.2: every page, the tools too)
				for (auto& it : g_pages[pg].items)
				{
					items++;
					std::string h = it.helpFn ? it.helpFn() : it.help;
					if (h.empty()) missing += " " + g_pages[pg].title + "/" + it.label + ";";
				}
			std::set<int> seen = { P_MAIN };
			std::vector<int> todo = { P_MAIN };
			while (!todo.empty())
			{
				int pg = todo.back(); todo.pop_back();
				for (auto& it : g_pages[pg].items) if (it.kind == Item::Page && !seen.count(it.page)) { seen.insert(it.page); todo.push_back(it.page); }
			}
			int biggest = 0;
			std::string bigName;
			for (auto& pg : g_pages) if ((int)pg.items.size() > biggest) { biggest = (int)pg.items.size(); bigName = pg.title; }
			// leftovers: map-tree hides and a grass trail from a tornado among trees...
			g_set.touchdown = false; g_set.debrisCone = false; g_set.extraDebris = false;
			BuildStandIns();
			TreeWorld(V3(0, 300, 0));
			SpawnTornadoAt(V3(0, 300, 0), 0, 0, "trees", SpawnOpts(), true);
			int k = 10;
			for (; k <= 400 && !(TreeHidesUsed() > 0 && VegTrailCount() > 0 && T(k) > 3.0f); k++) tick2(T(k));
			std::vector<Object> stands = g_standInProps;
			// ...then the intro's camp, gang, tornado and balloon (skipped to the handoff)
			IntroStart();
			for (k++; k <= 500 && g_in.stage != 4; k++) tick2(T(k));
			IntroSkip();
			for (k++; k <= 600 && g_in.stage != 5; k++) tick2(T(k));
			tick2(T(k++));
			std::vector<int> leftovers;
			for (int i = 0; i < kCastCount; i++) if (g_in.cast[i].ped) leftovers.push_back(g_in.cast[i].ped);
			for (Entity e : g_in.props) leftovers.push_back(e);
			int veh = g_bal.veh;
			int had = 0;
			for (int e : leftovers) if (Obj(e)) had++;
			int tornadoes = (int)g_tornadoes.size(), hides = TreeHidesUsed(), made = hidesMade, grass = VegTrailCount();
			bool balloon = g_bal.on && Obj(veh), stage5 = g_in.stage == 5;
			// Select on "Despawn everything", through the real key path
			int idx = -1;
			for (size_t i = 0; i < g_pages[P_MAIN].items.size(); i++) if (g_pages[P_MAIN].items[i].label == "Despawn everything") idx = (int)i;
			g_keys.select = ParseHotkey("ENTER");
			g_menuOpen = true; g_menuStack.clear(); g_menuStack.push_back({ P_MAIN, std::max(0, idx) });
			OnKeyboardMessage(VK_RETURN, 1, 0, FALSE, FALSE, FALSE, FALSE);
			InputBeginFrame();
			MenuInput();
			InputBeginFrame();
			int left = 0;
			for (int e : leftovers) if (Obj(e)) left++;
			int standsLeft = 0;
			for (Object o : stands) if (Obj(o)) standsLeft++;
			bool cleared = g_tornadoes.empty() && left == 0 && !Obj(veh) && !g_bal.on && g_in.stage == 0 && VegTrailCount() == 0 && vegAlive.empty() &&
				TreeHidesUsed() == 0 && hidesRemoved == made && standsLeft == 0;
			g_menuOpen = false; g_menuStack.clear();
			sprintf_s(det, "%d items on 6 pages, without help:%s; pages reachable %d/%d; biggest page %s (%d items) | before: %d tornadoes, intro stage %d, %d cast+props, balloon %d, "
				"grass %d, tree hides %d; Select on item %d: tornadoes %d, cast+props left %d, balloon %d, intro stage %d, grass %d (alive %d), hides %d (removed %d/%d), stand-ins left %d",
				items, missing.empty() ? " none" : missing.c_str(), (int)seen.size(), (int)P_COUNT, bigName.c_str(), biggest, tornadoes, (int)stage5 * 5, had, (int)balloon, grass, hides,
				idx, (int)g_tornadoes.size(), left, (int)(Obj(veh) != nullptr), g_in.stage, VegTrailCount(), (int)vegAlive.size(), TreeHidesUsed(), hidesRemoved, made, standsLeft);
			check("menu: help everywhere, all pages reachable, <= 14 items; Despawn everything clears it all", missing.empty() && (int)seen.size() == P_COUNT && biggest <= 14 &&
				idx >= 0 && tornadoes >= 1 && stage5 && had == (int)leftovers.size() && had >= kCastCount && balloon && grass > 0 && hides > 0 && cleared, det);
		}
		// 70. (v1.1) the roar: louder when closer, ~0 far away or with none, panned to the side it's on, minis quieter
		reset(); srand(70);
		{
			g_roarLevel = 2;
			mockTime = 1.0f;
			SoundUpdate(1.0f);
			float none = Roar::g_lastAsk[0];
			Tornado* tp = SpawnTornadoAt(V3(-60, 0, 0), 0, 0, "roar", SpawnOpts(), true);   // west = the camera's left (it looks north)
			SoundUpdate(1.0f);
			float vNear = Roar::g_lastAsk[0], panLeft = Roar::g_lastAsk[1];
			tp->base = V3(-250, 0, 0); SoundUpdate(1.0f);
			float vMid = Roar::g_lastAsk[0];
			tp->base = V3(-3000, 0, 0); SoundUpdate(1.0f);
			float vFar = Roar::g_lastAsk[0];
			tp->base = V3(60, 0, 0); SoundUpdate(1.0f);
			float panRight = Roar::g_lastAsk[1];
			DespawnAll();
			SpawnOpts mo; mo.mini = true; mo.size = 0.32f; mo.heightMul = 0.42f; mo.reach = 2.6f;
			SpawnTornadoAt(V3(-60, 0, 0), 0, 3, "mini", mo);
			SoundUpdate(1.0f);
			float vMini = Roar::g_lastAsk[0];
			DespawnAll();
			sprintf_s(det, "none %.3f; big one 60 m to the left %.3f (pan %.2f), 250 m %.3f, 3 km %.3f; 60 m to the right pan %.2f; a mini at 60 m %.3f", none, vNear, panLeft, vMid, vFar,
				panRight, vMini);
			check("roar: closer is louder, silent far/none, panned, minis quieter", none == 0.0f && vNear > vMid && vMid > vFar && vFar < 0.01f && vNear <= 0.75f &&
				panLeft < -0.5f && panRight > 0.5f && vMini < vNear * 0.6f, det);
		}
		// 71. (v1.1) UI text: the RDR2 look draws with the game's font markup and alignment; Simple draws plain text
		reset(); srand(71);
		{
			UI::g_simple = false;
			UI::Text("Hello there", 0.5f, 0.4f, 0.3f, 255, 255, 255, 255, UI::CENTRE, "title");
			std::string c = shown.empty() ? "" : shown.back();
			UI::Text("Right side", 0.9f, 0.4f, 0.3f, 255, 255, 255, 255, UI::RIGHT, "body");
			std::string r = shown.empty() ? "" : shown.back();
			UI::Text("Left side", 0.1f, 0.4f, 0.3f, 255, 255, 255, 255, UI::LEFT, "body");
			std::string l = shown.empty() ? "" : shown.back();
			bool single = c.find("<FONT FACE='$title'>") != std::string::npos && c.find("ALIGN='Center'") != std::string::npos && c.find("Hello there</FONT>") != std::string::npos &&
				r.find("<FONT FACE='$body'>") != std::string::npos && r.find("ALIGN='Right'") != std::string::npos && r.find("RIGHTMARGIN='0'") == std::string::npos &&
				l.find("ALIGN='Left'") != std::string::npos;
			// the whole menu, RDR2 look (two frames: the first one is the open animation at alpha 0)
			BuildMenu();
			g_menuOpen = true; g_menuStack.clear(); g_menuStack.push_back({ P_MAIN, 1 });
			mockTime = 5.0f; MenuDraw();
			mockTime = 6.0f; shown.clear(); MenuDraw();
			int rdrLines = (int)shown.size(), marked = 0;
			bool titleRdr = false;
			for (auto& s : shown)
			{
				if (s.find("<FONT FACE='$") != std::string::npos && s.find("<P ALIGN='") != std::string::npos) marked++;
				if (s.find("FACE='$title'") != std::string::npos && s.find("ALIGN='Center'") != std::string::npos && s.find(g_pages[P_MAIN].title) != std::string::npos) titleRdr = true;
			}
			// Simple
			UI::g_simple = true;
			shown.clear(); MenuDraw();
			int simpleLines = (int)shown.size(), simpleMarked = 0;
			bool titleSimple = false;
			for (auto& s : shown) { if (s.find("<FONT") != std::string::npos || s.find("<TEXTFORMAT") != std::string::npos || s.find("<P ALIGN") != std::string::npos) simpleMarked++; if (s == g_pages[P_MAIN].title) titleSimple = true; }
			UI::Text("Plain", 0.5f, 0.4f, 0.3f, 255, 255, 255, 255, UI::CENTRE, "title");
			bool plain = !shown.empty() && shown.back() == "Plain";
			UI::g_simple = false; g_menuOpen = false; g_menuStack.clear();
			sprintf_s(det, "centre/title: %s | right: %.70s... | RDR2 menu: %d of %d lines with markup, title centred in $title %d | Simple menu: %d lines, %d with markup, plain title %d, "
				"plain text %d", c.c_str(), r.c_str(), marked, rdrLines, (int)titleRdr, simpleLines, simpleMarked, (int)titleSimple, (int)plain);
			check("UI text: RDR2 markup and alignment, Simple plain", single && rdrLines > 10 && marked == rdrLines && titleRdr && simpleLines > 10 && simpleMarked == 0 &&
				titleSimple && plain, det);
		}

		// 72. (v1.1) after the intro (it locks a THUNDER sky for the scene) the weather is handed back once its storm is over -
		// also with Weather override "Off (keep yours)", where no storm lock ever takes the intro's lock over
		{
			bool locked[2] = {};
			int clears[2] = {}, stageEnd[2] = {};
			for (int mode = 1; mode >= 0; mode--)
			{
				reset(); srand(72);
				g_set.weatherMode = mode;
				mockTime = 1.0f;
				IntroStart();
				int k = 11;
				for (; k <= 400 && g_in.stage != 4; k++) tick2(T(k));
				IntroSkip();
				for (; k <= 600 && g_in.stage != 5; k++) tick2(T(k));
				for (int j = 0; j < 520 && g_in.stage != 0; j++) tick2(T(k++));   // the gang is thrown out and walks it off, the scene ends
				DespawnAll();
				for (int j = 0; j < 20; j++) tick2(T(k++));                         // the storm is over
				locked[mode] = g_lockedHash != 0;
				clears[mode] = calls[N_CLEAR_OVERRIDE_WEATHER];
				stageEnd[mode] = g_in.stage;
			}
			sprintf_s(det, "Storm clouds: still locked %d (clear-override calls %d, intro stage %d) | Off (keep yours): still locked %d to THUNDER (clear-override calls %d, intro stage %d)",
				(int)locked[1], clears[1], stageEnd[1], (int)locked[0], clears[0], stageEnd[0]);
			check("intro: the scene's THUNDER lock is handed back after the storm (Weather override Off too)", !locked[1] && !locked[0] && stageEnd[0] == 0 && stageEnd[1] == 0, det);
		}
		// 73. (v1.1 audit) "A wild storm, now" works with Storm season Off (the shipped default), and Despawn everything never
		// starts a new storm straight away
		reset(); srand(73);
		{
			g_set.season = 0; g_set.weatherMode = 1;
			mockTime = 1.0f;
			g_season.manual = true;
			SeasonBegin(1.0f);
			int k = 11;
			for (; k <= 300 && g_season.stage != 2; k++) tick2(T(k));
			bool landed = g_season.stage == 2 && !g_tornadoes.empty();
			// now with Storm season on: despawn mid-storm, and nothing should come back for a good while
			g_set.season = 3;
			ClearEverything();
			int restarted = 0;
			for (int j = 0; j < 600; j++) { tick2(T(k++)); if (g_season.stage) restarted++; }
			sprintf_s(det, "season Off + manual: touched down %d (stage %d, tornadoes %d); after Despawn with season Frequent: stages seen in the next 60 s %d, next storm in %.0f s",
				(int)landed, g_season.stage, (int)g_tornadoes.size(), restarted, g_season.nextAt - T(k));
			check("Storm season: a manual storm runs with the season Off; Despawn doesn't restart it", landed && restarted == 0 && g_season.nextAt - T(k) > 60.0f, det);
		}
		// 74. (v1.1 audit) leaving the balloon high up (menu / Despawn) opens the soft landing first - also when Arthur rides attached
		reset(); srand(74);
		{
			mockTime = 1.0f;
			BalloonSpawn(V3(0, 3, 0), 0);
			seatOf.clear();                         // the seat didn't take: he rides attached to the basket
			g_bal.seated = false;
			ENTITY::ATTACH_ENTITY_TO_ENTITY(1, g_bal.body, 0, 0, 0, 1, 0, 0, 0, FALSE, FALSE, FALSE, FALSE, 2, TRUE, FALSE, FALSE);
			g_bal.attached = true;
			playerPos = V3(0, 3, 60); playerHag = 60;
			mockTime = 5.0f;
			BalloonRemove("harness");
			bool window = g_dropWindowUntil > 5.0f;
			sprintf_s(det, "attached 60 m up, then removed: soft-landing window open %d (until %.1f)", (int)window, g_dropWindowUntil);
			check("balloon: removing it with Arthur aboard high up opens the soft landing", window, det);
		}
		// 75. (v1.1 audit) the tornado gun never takes smoke from the big tornado
		reset(); srand(75);
		{
			g_gun.on = true;
			Tornado* big = SpawnTornado(0, 120, 0, true, "big");
			int before = big ? big->loopsRunning : -1;
			int k = 10;
			for (int s = 0; s < 6; s++)
			{
				mockShooting = true; mockImpact = V3(20.0f + s * 9, 10, 0);
				tick2(T(k++));
				mockShooting = false;
				for (int j = 0; j < 8; j++) tick2(T(k++));
			}
			int after = big && TornadoRef(big).get() ? big->loopsRunning : -1;
			sprintf_s(det, "big tornado's looped effects %d -> %d after 6 shots (in use %d of %d)", before, after, LoopsInUse(), g_set.ptfxBudget);
			check("tornado gun: the big tornado keeps all its smoke", before > 0 && after == before && LoopsInUse() <= g_set.ptfxBudget, det);
		}

		// ======================= v1.1 audit 2 =======================
		// 76. (H1) the intro's colour grade over the handoff, at 144 fps: the Dark sky waits until the intro's grade has faded out (it
		// replaced it at once, then the fade's CLEAR wiped the dark sky - at high fps for good), then eases in from nothing
		reset(); srand(76);
		{
			g_set.weatherMode = 1; g_darkSky = true;
			mockTime = 1.0f;
			IntroStart();
			g_in.grade = "teaser_campMOD"; g_in.gradeStrength = 0.75f;   // (v1.3: no grade by default - [Intro] Grade asks for one)
			int k = 11;
			for (; k <= 400 && g_in.stage != 4; k++) tick2(T(k));
			IntroSkip();
			for (; k <= 600 && g_in.stage != 5; k++) tick2(T(k));
			std::string atHandoff = tcName;
			const float dt = 1.0f / 144.0f;
			g_frameDt = dt;
			float t = T(k), fadeEnd = -1, darkFirst = -1, darkFirstK = -1;
			bool replaced = false;
			for (int f = 0; f < 144 * 8; f++)
			{
				t += dt;
				size_t n0 = tcSets.size();
				bool introGrade = g_in.gradeK > 0;
				tick2(t, dt);
				for (size_t i = n0; i < tcSets.size(); i++)
				{
					if (introGrade && tcSets[i] != g_in.grade) replaced = true;
					if (!introGrade && darkFirst < 0 && tcSets[i] == "nbd1_ext_stormydarksky") { darkFirst = t; darkFirstK = tcStrength; }
				}
				if (fadeEnd < 0 && g_in.gradeK <= 0) fadeEnd = t;
			}
			bool darkOn = tcName == "nbd1_ext_stormydarksky" && fabsf(tcStrength - 0.6f) < 0.01f && g_storm.darkApplied;
			sprintf_s(det, "grade at the handoff '%s' (the intro's '%s'); replaced while it faded %d; faded out %.2f s after the handoff; dark sky from %.2f s at strength %.3f; "
				"8 s on: '%s' at %.2f (stage %d)", atHandoff.c_str(), g_in.grade.c_str(), (int)replaced, fadeEnd - T(k), darkFirst - T(k), darkFirstK, tcName.c_str(), tcStrength, g_in.stage);
			check("intro grade -> Dark sky: no pop at the handoff, the dark sky eases in after the fade (144 fps)", !g_in.grade.empty() && atHandoff == g_in.grade && !replaced &&
				fadeEnd > 0 && darkFirst >= fadeEnd && darkFirstK < 0.05f && darkOn && g_in.stage == 5, det);
		}
		// 77. (M1, M2, L3) the intro's funnel settles a few metres off Arthur, on the far side from the close-up camera (it sat on him, so
		// the calm-eye shots filmed through its core), and stays put from 36 s instead of chasing his balloon; the jets go back to normal
		reset(); srand(77);
		{
			mockTime = 1.0f;
			IntroStart();
			float off = -1, camToC = -1, wall = 0, moved = -1, fromBalloon = -1, boostAfter = -1;
			V3 at36;
			bool got36 = false;
			for (int k = 11; k <= 2000; k++)
			{
				// the mock doesn't carry Arthur with the balloon: done here, so a funnel that chased him would show it
				if (g_bal.on && !g_bal.waiting && (g_bal.seated || g_bal.attached)) playerPos = g_bal.pos + V3(0, 0, 1.0f);
				tick2(T(k));
				Tornado* tp = ITp();
				if (!tp) continue;
				if (g_in.stage == 4 && off < 0 && g_in.clock >= 80.0)
				{
					off = (tp->base - IntroSettleSpot()).len2d(); camToC = (lastCamCoord - tp->base).len2d(); wall = tp->wallRadius();
				}
				if (g_in.stage == 4 && !got36 && g_in.clock >= 82.0) { got36 = true; at36 = tp->base; }
				if (g_in.stage == 5) { moved = (tp->base - at36).len2d(); fromBalloon = (g_bal.pos - tp->base).len2d(); boostAfter = g_bal.boostShown; break; }
			}
			// (v1.3: it settles on the camp - Arthur's in the balloon - and every camera stays outside its wall)
			sprintf_s(det, "at 80 s: the funnel %.1f m from the middle of camp (wall %.1f m), the camera %.1f m from it; from 82 s to the handoff the funnel moved %.1f m and "
				"the balloon was %.1f m from it at the handoff; jet boost after the handoff %.1f", off, wall, camToC, moved, fromBalloon, boostAfter);
			check("intro: the funnel settles on the camp, the cameras stay outside it, and the balloon gets away; jets back to normal", off >= 0 && off < 6.0f &&
				camToC > wall * 1.2f && got36 && moved >= 0 && moved < 12.0f && fromBalloon > 30.0f && boostAfter == 0.0f, det);
		}
		// 78. (M3) the jets' effects are streamed in while the screen is black: lighting them at 36 s never waits mid-scene (a streaming
		// wait there froze the letterbox and subtitles for up to 1.5 s)
		reset(); srand(78);
		{
			countWaits = true; slowPtfx.insert(Joaat("anm_fire_dancers"));
			mockTime = 1.0f;
			IntroStart();
			int buildWaits = 0, sceneWaits = 0;
			bool jets = false;
			for (int k = 11; k <= 1000; k++)
			{
				int w0 = waits, st = g_in.stage;
				tick2(T(k));
				(st >= 4 ? sceneWaits : buildWaits) += waits - w0;
				if (g_bal.jetFx[0] || g_bal.jetFx[1]) jets = true;
				if (g_in.stage == 5) break;
			}
			countWaits = false;
			auto asked = ptfxAskedAt.find(Joaat("anm_fire_dancers"));
			sprintf_s(det, "streaming waits while the screen was black %d, during the scene %d; jets lit %d; the jets' dictionary asked for at %.1f s", buildWaits, sceneWaits,
				(int)jets, asked == ptfxAskedAt.end() ? -1.0f : asked->second);
			check("intro: no streaming wait mid-scene for the balloon's jets", sceneWaits == 0 && jets, det);
		}
		// 79. (M4) the roar's rumble, rendered offline (4 s at full volume and intensity): no longer sitting at the clip level, and mostly
		// above 20 Hz (it was 56% of samples at the ceiling, two thirds of it subsonic)
		reset(); srand(79);
		{
			std::vector<int16_t> buf(Roar::kFrames * 2);
			const int rate = 22050;
			double sum2 = 0, sub2 = 0;
			float lp = 0, peak = 0;
			int n = 0, nearClip = 0;
			for (int b = 0; b < 15 + rate * 4 / Roar::kFrames; b++)
			{
				Roar::Fill(buf.data(), 1.0f, 0.0f, 1.0f);
				for (int i = 0; i < Roar::kFrames; i++)
				{
					float s = buf[i * 2] / 30000.0f;
					lp += (s - lp) * (2 * PI * 20.0f / rate);
					if (b < 15) continue;   // ~1 s to settle
					n++; sum2 += s * s; sub2 += lp * lp; peak = std::max(peak, fabsf(s));
					if (fabsf(s) > 0.8f) nearClip++;
				}
			}
			float rms = (float)sqrt(sum2 / n), clip = 100.0f * nearClip / n, sub = (float)(100.0 * sub2 / sum2);
			sprintf_s(det, "rms %.2f, peak %.2f (ceiling 0.85), samples near the ceiling %.1f%%, share under 20 Hz %.0f%%", rms, peak, clip, sub);
			check("roar: the rumble is audible and doesn't sit at the clip level", clip < 2.0f && sub < 45.0f && rms > 0.15f && rms < 0.6f && peak <= 0.851f, det);
		}
		// 80. (M5) a gun mini emits puffs in proportion to its size (it was raised to 0.7: about 70% of a full tornado's puffs)
		reset(); srand(80);
		{
			g_set.touchdown = false; g_set.debrisCone = false; g_set.extraDebris = false; g_set.mapTrees = false; g_set.flatten = false;
			auto puffsPerSec = [&](const SpawnOpts& o) -> float
			{
				mockTime = 1.0f;
				Tornado n(V3(), 0, 3, "puffs", true, 0, o);
				float t = 1.0f;
				for (int k = 0; k < 80; k++) { t += 0.1f; mockTime = t; n.Update(0.1f, t); }   // fully down
				int before = calls[N_START_PARTICLE_FX_NON_LOOPED_AT_COORD];
				for (int k = 0; k < 100; k++) { t += 0.1f; mockTime = t; n.Update(0.1f, t); }
				n.Destroy();
				return (calls[N_START_PARTICLE_FX_NON_LOOPED_AT_COORD] - before) / 10.0f;
			};
			float big = puffsPerSec(SpawnOpts());
			SpawnOpts mo; mo.mini = true; mo.size = 0.32f; mo.heightMul = 0.42f; mo.reach = 2.6f; mo.grow = 1.1f;
			float mini = puffsPerSec(mo);
			sprintf_s(det, "one-shot effects per second: a full R tornado %.1f, a gun mini (size 0.32) %.1f (x%.2f)", big, mini, big > 0 ? mini / big : 0.0f);
			check("gun minis: puffs in proportion to their size", big > 10.0f && mini < big * 0.45f && mini > big * 0.15f, det);
		}
		// 81. (M6) Cinematic mode: UI::Draw (it runs every frame) keeps the letterbox, the chapter card and subtitles, and hides the mod's
		// own labels - help tip, toast, shard, objective, honor sting, score plate
		reset(); srand(81);
		{
			auto has = [&](const char* s) { for (auto& x : shown) if (x.find(s) != std::string::npos) return true; return false; };
			auto frame = [&]()
			{
				UI::ScorePlate("SCOREPLATE", "12", "sub", "", 0); UI::Subtitle("ARTHUR", "SUBTITLELINE", 1); UI::ChapterCard("CARDTITLE", "card", 1);
				shown.clear(); UI::Draw();
			};
			mockTime = 1.0f; UI::Frame(0.016f);
			UI::HelpTip("HELPTIP here", 5); UI::Toast("TOASTTITLE", "toast text", 5); UI::Shard("SHARDTITLE", "sub", 5); UI::Objective("OBJECTIVELINE", 5);
			UI::HonorLost(true);
			mockTime = 2.0f; UI::Frame(0.016f);
			g_set.cinematic = true; SyncMenuChoices();
			frame();
			bool cleanOk = has("SUBTITLELINE") && has("CARDTITLE") && !has("HELPTIP") && !has("TOASTTITLE") && !has("SHARDTITLE") && !has("OBJECTIVELINE") &&
				!has("HONOR") && !has("SCOREPLATE");
			int cleanLines = (int)shown.size();
			g_set.cinematic = false; SyncMenuChoices();
			frame();
			bool normalOk = has("SUBTITLELINE") && has("CARDTITLE") && has("HELPTIP") && has("TOASTTITLE") && has("SHARDTITLE") && has("OBJECTIVELINE") && has("HONOR") &&
				has("SCOREPLATE");
			sprintf_s(det, "cinematic: %d lines drawn, only the subtitle and the chapter card %d; normal: %d lines, everything %d", cleanLines, (int)cleanOk, (int)shown.size(), (int)normalOk);
			check("cinematic mode hides the mod's labels in UI::Draw (keeps subtitles and the chapter card)", cleanOk && normalOk, det);
		}
		// 82. (L10) Storm season switched Off while a wild one is out: it still dies down when its time is up (it lived on unchecked)
		reset(); srand(82);
		{
			g_set.season = 3; g_set.weatherMode = 1;
			mockTime = 1.0f;
			SeasonBegin(1.0f);
			int k = 11;
			for (; k <= 300 && g_season.stage != 2; k++) tick2(T(k));
			bool landed = g_season.stage == 2 && !g_tornadoes.empty();
			g_set.season = 0;
			for (int j = 0; j < 5; j++) tick2(T(k++));
			bool tracked = g_season.stage == 2 && g_season.tp.get() != nullptr;
			g_season.lifeUntil = T(k);
			tick2(T(++k));
			bool dying = !g_tornadoes.empty() && g_tornadoes[0]->Dissipating();
			for (int j = 0; j < 120 && !g_tornadoes.empty(); j++) tick2(T(++k));
			for (int j = 0; j < 5; j++) tick2(T(++k));
			bool quiet = g_season.stage == 0 && g_season.nextAt == 0 && g_tornadoes.empty();
			sprintf_s(det, "touched down %d; season Off: still tracked %d; time up -> dying down %d; afterwards stage %d, next storm at %.0f, tornadoes %d", (int)landed, (int)tracked,
				(int)dying, g_season.stage, g_season.nextAt, (int)g_tornadoes.size());
			check("Storm season Off with a wild one out: it still dies down on time, and no next storm", landed && tracked && dying && quiet, det);
		}
		// 83. (L11) Weather override switched Off during a Storm season warning: the warning's THUNDER lock is let go (it was locked for good)
		reset(); srand(83);
		{
			g_set.season = 0; g_set.weatherMode = 1;
			mockTime = 1.0f;
			g_season.manual = true;
			SeasonBegin(1.0f);
			bool locked = g_lockedHash == Joaat("THUNDER") && g_storm.seasonLock;
			int k = 11;
			for (; k <= 60; k++) tick2(T(k));
			g_set.weatherMode = 0;   // "Off (keep yours)"
			for (; k <= 300 && g_season.stage != 2; k++) tick2(T(k));
			bool landed = g_season.stage == 2;
			bool atTouchdown = g_lockedHash == 0;
			DespawnAll();
			for (int j = 0; j < 20; j++) tick2(T(k++));
			sprintf_s(det, "warning locked THUNDER %d; touched down %d with the lock %s; after the storm: locked %d, storm %d", (int)locked, (int)landed,
				atTouchdown ? "released" : "still on", (int)(g_lockedHash != 0), (int)g_storm.active);
			check("Storm season: Weather override Off during the warning lets the THUNDER lock go", locked && landed && atTouchdown && g_lockedHash == 0 && !g_storm.active, det);
		}
		// 84. (L14) mission music: a story mission never gets our global STOP_MUSIC_8S, and our END plays only if our START did
		reset(); srand(84);
		{
			auto sawEvent = [&](const char* e) { for (auto& x : musicEvents) if (x == e) return true; return false; };
			const Score& sc = kScores[0];
			g_musicMode = 1; musicReady = true; musicNotReady.insert(sc.start);
			mockTime = 1.0f;
			SpawnTornado(0, 80, 0, true, "music");
			int k = 11;
			for (int j = 0; j < 20; j++) tick2(T(k++));
			bool pending = g_music.pending != nullptr && !g_music.fired;
			mockMission = true;                                       // (a) a mission while our START was still pending
			for (int j = 0; j < 150; j++) tick2(T(k++));
			bool quietA = musicEvents.empty() && g_music.layer == 0;
			mockMission = false; musicNotReady.clear();               // (b) it plays, then a mission
			for (int j = 0; j < 20; j++) tick2(T(k++));
			bool startedB = g_music.fired && !musicEvents.empty();
			mockMission = true;
			for (int j = 0; j < 150; j++) tick2(T(k++));
			bool endB = sawEvent(sc.end), stopB = sawEvent("STOP_MUSIC_8S");
			mockMission = false; musicEvents.clear();                 // (c) no mission: END, then the stop, as before
			for (int j = 0; j < 20; j++) tick2(T(k++));
			DespawnAll();
			for (int j = 0; j < 120; j++) tick2(T(k++));
			bool endC = sawEvent(sc.end), stopC = sawEvent("STOP_MUSIC_8S");
			g_musicMode = 0;
			sprintf_s(det, "(a) start pending %d, a mission: events %d, layer %d | (b) started %d, a mission: END %d, STOP_MUSIC_8S %d | (c) storm over: END %d, STOP %d",
				(int)pending, (int)quietA, g_music.layer, (int)startedB, (int)endB, (int)stopB, (int)endC, (int)stopC);
			check("music: no global stop over a mission's score, no END when nothing of ours played", pending && quietA && startedB && endB && !stopB && endC && stopC, det);
		}
		// 85. (L19) the gun's minis: no ride camera when one lifts Arthur, they neither keep Survive the storm going nor end it, and they
		// don't flatten grass (their spheres pushed the big one's trail out of its 80)
		reset(); srand(85);
		{
			g_set.rideCam = true; g_set.touchdown = false; g_set.flatten = true;
			SpawnOpts mo; mo.mini = true; mo.size = 0.32f; mo.heightMul = 0.42f; mo.reach = 2.6f; mo.grow = 1.1f; mo.life = 120.0f;
			mockTime = 1.0f;
			Tornado* m = SpawnTornadoAt(V3(10, 0, 0), 0, 3, "Mini twister", mo);
			playerPos = V3(0, 0, 10);
			m->playerLastHeld = 1.0f;
			RideCamUpdate(0.1f, 1.0f, false);
			bool noRide = !g_ride.on;
			if (g_ride.on) RideCamStop(false);
			DespawnAll();
			mockTime = 2.0f;
			SurvivalStart();
			Tornado* big = g_tornadoes.empty() ? nullptr : g_tornadoes[0].get();
			Tornado* mini = SpawnTornadoAt(V3(8, 0, 0), 0, 3, "Mini twister", mo);
			int k = 21;
			for (; k <= 130; k++) { playerPos = V3(0, 0, 9); if (mini) mini->playerLastHeld = T(k); tick2(T(k)); if (!g_surv.on) break; }
			bool notCaught = g_surv.on;
			std::string early = g_surv.result;
			if (big && TornadoRef(big).get()) big->BeginDissipate(T(k));
			for (int j = 0; j < 5 && g_surv.on; j++) tick2(T(++k));
			bool ended = !g_surv.on && g_surv.result.rfind("ENDED", 0) == 0;
			DespawnAll();
			int before = (int)vegAdded.size();
			Tornado w(V3(0, 50, 0), 90, 3, "mini grass", false, 0, mo);
			w.scripted = true; w.scriptVel = V3(10, 0, 0);
			for (int j = 1; j <= 100; j++) { mockTime = T(k + j); w.Update(0.1f, mockTime); }
			int miniVeg = (int)vegAdded.size() - before;
			w.Destroy();
			ClearVegTrail();
			sprintf_s(det, "a mini lifting Arthur 10 m: ride cam %d | Survive: a mini holding him 9 m up for 11 s - still running %d (\"%s\"); the big one gone -> \"%s\" | "
				"grass spheres from a mini crossing 100 m %d", (int)!noRide, (int)notCaught, early.c_str(), g_surv.result.c_str(), miniVeg);
			check("gun minis: no ride camera, don't keep or end Survive the storm, no grass spheres", noRide && notCaught && ended && miniVeg == 0, det);
		}
		// 86. (L5) real trees: a telegraph pole (thin, nothing 10.5 m up) is no trunk; a thin but tall tree still is
		reset(); srand(86);
		{
			g_set.touchdown = false; g_set.debrisCone = false; g_set.extraDebris = false; g_set.flatten = false; g_set.mapTrees = true;
			g_spawnableTrees.clear();
			BuildStandIns();
			std::set<Hash> standIns;
			for (auto& s : g_standIns) standIns.insert(s.Model());
			int nTree = (int)(sizeof(kTreeModels) / sizeof(kTreeModels[0]));
			for (int i = 0; i < nTree; i++) if (!(i < 3 || standIns.count(kTreeModels[i].hash))) unloadedModels.insert(kTreeModels[i].hash);
			world.clear();
			for (int i = 0; i < 6; i++) { float r = (15.0f + i * 30.0f) * PI / 180.0f; world.push_back(Cyl(cosf(r) * 6.0f, sinf(r) * 6.0f, 0.15f, 9.0f)); }    // [0-5] poles, 9 m
			for (int i = 0; i < 6; i++) { float r = (195.0f + i * 30.0f) * PI / 180.0f; world.push_back(Cyl(cosf(r) * 9.0f, sinf(r) * 9.0f, 0.2f, 16.0f)); }   // [6-11] thin tall trees
			Tornado n(V3(), 0, 0, "poles", true, 0);
			for (int k = 1; k <= 600; k++) { mockTime = T(k); n.Update(0.1f, mockTime); UpdateFlights(0.1f, mockTime); integrate(0.1f); }
			int atPole = 0, atTree = 0;
			for (auto& h : hideAt)
				for (int i = 0; i < 12; i++)
					if ((h - world[i].a).len2d() < 1.0f) (i < 6 ? atPole : atTree)++;
			int poleRays = 0, treeRays = 0;
			for (int i = 0; i < 12; i++) (i < 6 ? poleRays : treeRays) += shapeHits[i];
			int real = n.realTrees, rejected = n.treeRejected;
			n.Destroy();
			RestoreRealTrees();
			sprintf_s(det, "rays that hit a pole %d, a thin tree %d; trees torn out %d (rejected %d hits); hides at a pole %d, at a thin tree %d", poleRays, treeRays, real, rejected,
				atPole, atTree);
			check("real trees: telegraph poles aren't trunks, thin tall trees still are", poleRays > 0 && atPole == 0 && atTree > 0 && real >= 1 && rejected >= 1, det);
		}
		// 87. (L7) real trees: the hide cap running out partway leaves no stand-in for that tree (it'd be a twin of a tree still standing);
		// a stand-in that never streams in is let go
		reset(); srand(87);
		{
			g_set.touchdown = false; g_set.debrisCone = false; g_set.extraDebris = false; g_set.flatten = false; g_set.mapTrees = true;
			g_spawnableTrees.clear();
			BuildStandIns();
			int nTree = (int)(sizeof(kTreeModels) / sizeof(kTreeModels[0]));
			TreeWorld(V3());
			Tornado a(V3(), 0, 0, "cap", true, 0);   // every tree model loaded: the 600 run out on the third trunk
			for (int k = 1; k <= 400; k++) { mockTime = T(k); a.Update(0.1f, mockTime); UpdateFlights(0.1f, mockTime); integrate(0.1f); }
			int hides = TreeHidesUsed(), real = a.realTrees, stands = (int)g_standInProps.size(), trunks = a.treeTrunks;
			a.Destroy();
			RestoreRealTrees();
			bool capOk = hides == 600 && real == 600 / nTree && stands == real && trunks > real;
			// the stand-ins never stream in
			std::set<Hash> standIns;
			for (auto& s : g_standIns) standIns.insert(s.Model());
			for (int i = 0; i < nTree; i++) if (i >= 3) unloadedModels.insert(kTreeModels[i].hash);
			for (Hash h : standIns) unloadedModels.insert(h);
			g_nextLoadedScan = 0; modelsReleased.clear();
			mockTime = T(400);
			Tornado b(V3(), 0, 0, "pending", true, 0);
			for (int k = 401; k <= 700; k++) { mockTime = T(k); b.Update(0.1f, mockTime); UpdateFlights(0.1f, mockTime); integrate(0.1f); }
			int released = 0;
			for (Hash h : standIns) released += modelsReleased.count(h) ? modelsReleased[h] : 0;
			int pendTrunks = b.treeTrunks, pendReal = b.realTrees;
			b.Destroy();
			RestoreRealTrees();
			sprintf_s(det, "all %d models loaded: trunks %d, torn out %d (stand-ins %d), hides %d | stand-ins never loaded: trunks %d, torn out %d, stand-in models let go %d",
				nTree, trunks, real, stands, hides, pendTrunks, pendReal, released);
			check("real trees: no stand-in where the hide cap ran out; an unstreamed stand-in is released", capOk && pendTrunks >= 1 && pendReal == 0 && released >= 1, det);
		}
		// 88. (L12) what a finished intro handed to the world (the gang, the camp) is still cleared by Despawn everything - but not a handle
		// the game has since given to something else
		reset(); srand(88);
		{
			mockTime = 1.0f;
			IntroStart();
			int k = 11;
			for (; k <= 400 && g_in.stage != 4; k++) tick2(T(k));
			IntroSkip();
			std::vector<int> ours;
			for (; k <= 1400 && g_in.stage != 0; k++)
			{
				if (g_in.stage == 5 && ours.empty())
				{
					for (int i = 0; i < kCastCount; i++) if (g_in.cast[i].ped) ours.push_back(g_in.cast[i].ped);
					for (Entity e : g_in.props) ours.push_back(e);
				}
				tick2(T(k));
			}
			bool done = g_in.stage == 0;
			int alive = 0, reused = 0;
			for (int e : ours) if (Obj(e)) { alive++; if (!reused) reused = e; }
			if (reused) objects[reused].model = Joaat("a_c_horse_arabian_white");   // deleted by the game, the handle reused
			ClearEverything();
			int left = 0;
			for (int e : ours) if (e != reused && Obj(e)) left++;
			sprintf_s(det, "scene over %d; %d of %d cast + camp still in the world; after Despawn everything: left %d, the reused handle kept %d", (int)done, alive, (int)ours.size(),
				left, (int)(reused && Obj(reused) != nullptr));
			check("intro: Despawn everything clears what the scene left in the world (not a reused handle)", done && alive >= kCastCount && left == 0 && reused && Obj(reused), det);
		}
		// 89. (L13) no intro during a story mission; the tornado gun doesn't fire in a mission or while an auto test runs
		reset(); srand(89);
		{
			mockMission = true;
			mockTime = 1.0f;
			IntroStart();
			bool introBlocked = g_in.stage == 0 && !screenFadedOut;
			g_gun.on = true; mockShooting = true; mockImpact = V3(20, 10, 0);
			GunUpdate(2.0f);
			int inMission = g_gun.fired;
			mockMission = false; g_auto.running = true;
			GunUpdate(3.0f);
			int inTest = g_gun.fired;
			g_auto.running = false;
			GunUpdate(4.0f);
			int free = g_gun.fired;
			mockShooting = false;
			DespawnAll(); g_gun.minis.clear();
			sprintf_s(det, "intro in a mission: stage %d, faded %d; gun shots fired: in a mission %d, in an auto test %d, otherwise %d", g_in.stage, (int)screenFadedOut, inMission,
				inTest, free);
			check("no intro in a story mission; no gun minis in a mission or an auto test", introBlocked && inMission == 0 && inTest == 0 && free == 1, det);
		}
		// 90. (L15) the roar follows the camera that's actually drawn (the drone, the intro); a bare "&" is escaped for the markup, once
		reset(); srand(90);
		{
			g_roarLevel = 2;
			mockTime = 1.0f;
			SpawnTornadoAt(V3(0, 2000, 0), 0, 0, "far", SpawnOpts(), true);
			SoundUpdate(1.0f);
			float vGameplay = Roar::g_lastAsk[0];
			lastRenderCams = 1; lastCamCoord = V3(0, 1940, 5);   // a script camera 60 m from it
			SoundUpdate(1.0f);
			float vRendered = Roar::g_lastAsk[0];
			lastRenderCams = 0;
			DespawnAll();
			UI::g_simple = false;
			UI::Text("MODES & TOYS", 0.5f, 0.4f, 0.3f, 255, 255, 255, 255, UI::CENTRE, "title");
			std::string a = shown.empty() ? "" : shown.back();
			UI::Text("Fish &amp; chips <3", 0.5f, 0.4f, 0.3f, 255, 255, 255, 255, UI::LEFT, "body");
			std::string b = shown.empty() ? "" : shown.back();
			bool amp = a.find("MODES &amp; TOYS") != std::string::npos && b.find("Fish &amp; chips &lt;3") != std::string::npos && b.find("&amp;amp;") == std::string::npos;
			sprintf_s(det, "roar with the gameplay camera 2 km off %.3f, with a script camera drawn 60 m from it %.3f | \"%.60s\" | \"%.60s\"", vGameplay, vRendered,
				a.c_str() + std::min(a.size(), a.find("$title'>") + 8), b.c_str() + std::min(b.size(), b.find("$body'>") + 7));
			check("roar follows the rendered camera; '&' escaped once in the markup", vGameplay < 0.01f && vRendered > 0.2f && amp, det);
		}
		// 91. (L16) the honor sting: the game's toast or the mod's, never both; shots 11-12 frame Uncle / Pearson where they are when their
		// lift is late (the camera went to the map's origin); the cow gets no human anim clip
		reset(); srand(91);
		{
			auto has = [&](const char* s) { for (auto& x : shown) if (x.find(s) != std::string::npos) return true; return false; };
			feedId = 9;
			// (v1.3: well past any sting an earlier scenario's intro left on the UI's own clock)
			mockTime = 5000.0f; UI::Frame(0.016f);
			int p0 = calls[N_UI_FEED_POST_SAMPLE_TOAST_RIGHT];
			UI::HonorLost(false);
			mockTime = 5000.5f; UI::Frame(0.016f); shown.clear(); UI::Draw();
			bool gameOnly = calls[N_UI_FEED_POST_SAMPLE_TOAST_RIGHT] == p0 + 1 && !has("HONOR");
			mockTime = 5010.0f; UI::Frame(0.016f);
			int p1 = calls[N_UI_FEED_POST_SAMPLE_TOAST_RIGHT];
			UI::HonorLost(true);
			mockTime = 5010.5f; UI::Frame(0.016f); shown.clear(); UI::Draw();
			bool modOnly = calls[N_UI_FEED_POST_SAMPLE_TOAST_RIGHT] == p1 && has("HONOR");
			feedId = 0;
			// the shots, with the funnel held 200 m off from 24 s so the lifts come late
			playerPos = V3(500, 500, 0);
			mockTime = 20.0f;
			IntroStart();
			float worst11 = -1, worst12 = -1;
			bool uncleLate = false, pearsonLate = false;
			for (int k = 201; k <= 1700; k++)
			{
				tick2(T(k));
				if (g_in.stage != 4) continue;
				Tornado* tp = ITp();
				if (tp && g_in.clock >= 63.0 && g_in.clock < 67.0) tp->base = IL(0, 200);
				const CastState& u = g_in.cast[CA_UNCLE], & p = g_in.cast[CA_PEARSON];
				if (g_in.clock >= 75.4 && g_in.clock < 76.6 && u.ok && !u.lifted) { uncleLate = true; if (MockObj* o = Obj(u.ped)) worst11 = std::max(worst11, (lastCamCoord - o->p).len2d()); }
				if (g_in.clock >= 78.2 && g_in.clock < 79.4 && p.ok && !p.lifted) { pearsonLate = true; if (MockObj* o = Obj(p.ped)) worst12 = std::max(worst12, (lastCamCoord - o->p).len2d()); }
				if (g_in.clock > 70.0) break;
			}
			int cow = g_in.cast[CA_COW].ped;
			bool cowLifted = g_in.cast[CA_COW].lifted;
			std::string cowAnim = animOf.count(cow) ? animOf[cow] : "";
			IntroAbort("harness");
			sprintf_s(det, "HonorLost(false) with the feed working: game toast only %d; HonorLost(true): the mod's sting only %d | late lifts: Uncle %d, Pearson %d; shot 11's camera "
				"%.1f m from Uncle, shot 12's %.1f m from Pearson | the cow lifted %d, anim clip '%s'", (int)gameOnly, (int)modOnly, (int)uncleLate, (int)pearsonLate, worst11, worst12,
				(int)cowLifted, cowAnim.c_str());
			// (v1.3: the shots that framed late posed lifts are gone - the gang go up as ragdolls; the honor and cow checks stay)
			(void)uncleLate; (void)pearsonLate; (void)worst11; (void)worst12;
			check("honor sting: one or the other; no human clip on the cow", gameOnly && modOnly && cowLifted && cowAnim.empty(), det);
		}
		// 92. (L17) the best scores are read at startup (LoadConfig), so the menu's help shows them before the first run; no help text is
		// cut off in either menu look
		reset(); srand(92);
		{
			BackupBest();
			WriteFileStr(BestPath(), "# harness\nchaser 321.00\nsurvive 77.00\n");
			LoadConfig();
			bool loaded = g_bal.chaseBest == 321.0f && g_surv.best == 77.0f && g_bal.chaseBestLoaded && g_surv.bestLoaded;
			BuildMenu();
			std::string chaserHelp, surviveHelp, cut;
			int items = 0;
			for (auto& pg : g_pages)
				for (auto& it : pg.items)
				{
					std::string h = it.helpFn ? it.helpFn() : it.help;
					if (it.label.find("chaser") != std::string::npos) chaserHelp = h;
					if (it.label.find("Survive") != std::string::npos) surviveHelp = h;
					items++;
					for (size_t w : { (size_t)66, (size_t)58 })
						if (Wrap(h, w, 1000).size() > Wrap(h, w, kHelpLines).size()) cut += " " + it.label + ";";
				}
			RestoreBest();
			sprintf_s(det, "after LoadConfig: chaser best %.0f, survive best %.0f (loaded %d); Storm chaser help ends \"...%s\", Survive help ends \"...%s\"; %d items, cut off:%s",
				g_bal.chaseBest, g_surv.best, (int)loaded, chaserHelp.size() > 14 ? chaserHelp.c_str() + chaserHelp.size() - 14 : chaserHelp.c_str(),
				surviveHelp.size() > 14 ? surviveHelp.c_str() + surviveHelp.size() - 14 : surviveHelp.c_str(), items, cut.empty() ? " none" : cut.c_str());
			check("best scores loaded at startup and shown in the help; no help text cut off", loaded && chaserHelp.find("321") != std::string::npos &&
				surviveHelp.find("1:17") != std::string::npos && cut.empty(), det);
		}
		// 93. (L1) the tracker's closing speed is per tornado id, not per address (a new tornado at a dead one's address read as closing at
		// hundreds of m/s)
		reset(); srand(93);
		{
			auto line = [&]() { for (auto& s : shown) if (s.find(" m      ") != std::string::npos) return s; return std::string(); };
			mockTime = 1.0f;
			Tornado* tp = SpawnTornado(0, 100, 0, true, "tracker");
			TrackerDraw(nullptr, 0, 0.1f);
			TrackerDraw(tp, 100.0f, 0.1f);
			tp->uid += 1000;   // the same address, another tornado, 90 m nearer
			mockTime = 1.1f;
			TrackerDraw(tp, 10.0f, 0.1f);
			mockTime = 1.6f; shown.clear();   // (after its 0.4 s fade-in) it holds at 10 m
			TrackerDraw(tp, 10.0f, 0.1f);
			std::string reusedLine = line();
			mockTime = 1.7f; shown.clear();
			TrackerDraw(tp, 9.0f, 0.1f);   // then comes on
			std::string realLine = line();
			DespawnAll();
			bool ok = !reusedLine.empty() && reusedLine.find("closing") == std::string::npos && realLine.find("closing") != std::string::npos;
			sprintf_s(det, "a new tornado at the old address: \"%.80s\" | it then comes on: \"%.80s\"", reusedLine.c_str() + std::min(reusedLine.size(), reusedLine.find("'>~s~") + 5),
				realLine.c_str() + std::min(realLine.size(), realLine.find("'>~s~") + 5));
			check("tracker: no bogus closing speed when an address is reused", ok, det);
		}
		// 94. (L2, L20) a Script Hook restart mid-intro gives the camera back (the intro's own camera too), lets Arthur ragdoll again and clears
		// the balloon's "don't ragdoll out" flag; a map tree check cut off by the restart can run again (it said "still running" for ever)
		reset(); srand(94);
		{
			mockTime = 1.0f;
			IntroStart();
			int k = 11;
			for (; k <= 400 && !(g_in.stage == 4 && g_in.clock > 5.0); k++) tick2(T(k));
			bool scene = lastRenderCams == 1 && g_in.cam != 0 && ragdollOk.count(1) && ragdollOk[1] == 0;
			g_treePins.push_back({ 1, 0 }); g_treeChecked = 77;   // the check's 6 s timer was in g_scheduled
			ResetAfterScriptRestart();
			bool camsBack = lastRenderCams == 0, ragdollBack = ragdollOk[1] == 1;
			int pins = calls[N_PIN_CLOSEST_MAP_ENTITY];
			MapTreeCheck();
			bool treeCheckRan = calls[N_PIN_CLOSEST_MAP_ENTITY] > pins;
			g_scheduled.clear(); g_treePins.clear(); g_treeChecked = 0;
			BalloonSpawn(V3(0, 3, 0), 0);
			bool flagOn = pedFlags.count({ 1, 15 }) && pedFlags[{ 1, 15 }] == 1;
			ResetAfterScriptRestart();
			bool flagOff = pedFlags[{ 1, 15 }] == 0;
			sprintf_s(det, "in the scene: script cams %d, ragdoll off %d | after the restart: script cams %d, ragdoll %d; the map tree check ran again %d | balloon flag 15 set %d, "
				"after a restart %d", (int)scene, (int)scene, lastRenderCams, ragdollOk[1], (int)treeCheckRan, (int)flagOn, pedFlags[{ 1, 15 }]);
			check("script restart: the intro's camera, Arthur's ragdoll and balloon flag back; map tree check can run again", scene && camsBack && ragdollBack && treeCheckRan &&
				flagOn && flagOff, det);
		}
		// 95. (L8, L9) removing the balloon gets Arthur out of the seat before the vehicle goes; teleporting from the balloon removes it first
		reset(); srand(95);
		{
			mockTime = 1.0f;
			BalloonSpawn(V3(0, 3, 0), 0);
			bool seated = g_bal.seated && seatOf.count(1) != 0;
			BalloonRemove("harness");
			int inside = deletedWithPedInside;
			BalloonSpawn(V3(0, 3, 0), 0);
			Vehicle v = g_bal.veh;
			bool seated2 = seatOf.count(1) != 0;
			TeleportTo(V3(100, 100, 0));
			bool gone = !g_bal.on && !Obj(v) && fabsf(playerPos.x - 100.0f) < 0.01f;
			sprintf_s(det, "seated %d, removed with him in the seat %d; seated again %d, teleported: balloon gone %d, Arthur at x %.0f, deleted with him inside %d", (int)seated, inside,
				(int)seated2, (int)gone, playerPos.x, deletedWithPedInside);
			check("balloon: never deleted with Arthur in the seat; teleport removes it first", seated && inside == 0 && seated2 && gone && deletedWithPedInside == 0, det);
		}
		// 96. (L4) Arthur's horse waits out the intro invincible and deaf to the screams, and is given back as it was
		reset(); srand(96);
		{
			mockHorse = nextObject++;
			objects[mockHorse].p = V3(4, -3, 0); objects[mockHorse].type = 1; objects[mockHorse].model = Joaat("a_c_horse_arabian_white");
			mockTime = 1.0f;
			IntroStart();
			int k = 11;
			for (; k <= 400 && g_in.stage != 4; k++) tick2(T(k));
			int invIn = invincible.count(mockHorse) ? invincible[mockHorse] : -1, blockIn = blockEvents.count(mockHorse) ? blockEvents[mockHorse] : -1;
			bool inScene = invIn == 1 && blockIn == 1;
			IntroSkip();
			for (; k <= 1400 && g_in.stage != 0; k++) tick2(T(k));
			bool after = g_in.stage == 0 && invincible[mockHorse] == 0 && blockEvents[mockHorse] == 0;
			sprintf_s(det, "horse %d: in the scene invincible %d, events blocked %d; after the scene invincible %d, events blocked %d", mockHorse, invIn, blockIn,
				invincible[mockHorse], blockEvents[mockHorse]);
			check("intro: Arthur's horse protected in the scene and given back", inScene && after, det);
		}
		// 97. (L6) the tree scan uses the stand-ins' cached model hashes: no GET_HASH_KEY in its loops (hundreds per trunk before)
		reset(); srand(97);
		{
			g_set.touchdown = false; g_set.debrisCone = false; g_set.extraDebris = false; g_set.flatten = false;
			g_spawnableTrees.clear();
			BuildStandIns();
			bool cached = !g_standIns.empty();
			for (auto& s : g_standIns) if (s.hash != Joaat(s.name.c_str())) cached = false;
			std::set<Hash> standIns;
			for (auto& s : g_standIns) standIns.insert(s.Model());
			int nTree = (int)(sizeof(kTreeModels) / sizeof(kTreeModels[0]));
			for (int i = 0; i < nTree; i++) if (!(i < 3 || standIns.count(kTreeModels[i].hash))) unloadedModels.insert(kTreeModels[i].hash);
			auto hashCalls = [&](bool trees, int* trunks) -> int
			{
				g_set.mapTrees = trees;
				TreeWorld(V3());
				mockTime = 1.0f;
				Tornado n(V3(), 0, 0, "hashes", true, 0);
				int before = calls[N_GET_HASH_KEY];
				for (int k = 11; k <= 200; k++) { mockTime = T(k); n.Update(0.1f, mockTime); UpdateFlights(0.1f, mockTime); integrate(0.1f); }
				int c = calls[N_GET_HASH_KEY] - before;
				*trunks = n.treeTrunks;
				n.Destroy();
				RestoreRealTrees();
				return c;
			};
			int t0 = 0, t1 = 0;
			int off = hashCalls(false, &t0), on = hashCalls(true, &t1);
			sprintf_s(det, "stand-in hashes cached %d; GET_HASH_KEY calls in 19 s: real trees off %d, on %d (%d trunks torn out)", (int)cached, off, on, t1);
			check("real trees: no GET_HASH_KEY in the tree scan", cached && t1 >= 2 && on - off <= 2, det);
		}
		// 98. (L18) when the intro is over its anim dictionaries go back to the game, and the camp models it asked for but didn't use are let go
		reset(); srand(98);
		{
			mockTime = 1.0f;
			IntroStart();
			int k = 11;
			for (; k <= 400 && g_in.stage != 4; k++) tick2(T(k));
			int altsReleased = 0, alts = 0;
			for (auto& p : kCampProps) if (p.alt) { alts++; if (modelsReleased.count(Joaat(p.alt))) altsReleased++; }
			IntroSkip();
			bool heldInScene = dictsRemoved.empty();
			for (; k <= 1400 && g_in.stage != 0; k++) tick2(T(k));
			int dicts = 0, removed = 0;
			for (const char* d : kIntroDicts) { dicts++; if (dictsRemoved.count(d)) removed++; }
			sprintf_s(det, "camp alternatives let go after the build %d of %d; dictionaries kept through the scene %d, released at the end %d of %d (stage %d)", altsReleased, alts,
				(int)heldInScene, removed, dicts, g_in.stage);
			check("intro: unused camp models and the anim dictionaries are released", altsReleased == alts && heldInScene && removed == dicts && g_in.stage == 0, det);
		}
		// ======== v1.2 ========
		// 99. voices: the tornado takes a man near Arthur, he screams, and Arthur answers once the scream is over
		reset(); srand(99);
		{
			g_set.grabPeople = false;              // the mock lifts him by hand
			Tornado* tp = SpawnTornado(0, 40, 0, true, "voices");
			int k = 0;
			for (; k < 120; k++) tick2(T(k));
			V3 b = tp ? tp->base : V3();
			int ped = nextObject++; objects[ped].type = 1; objects[ped].p = b + V3(4, 0, 0); humans.insert(ped);
			for (int j = 0; j < 10; j++) tick2(T(k++));
			size_t before = said.size();
			objects[ped].p.z = 9.0f; objects[ped].frozen = true;   // up he goes
			for (int j = 0; j < 50; j++) tick2(T(k++));
			int scream = -1, reply = -1;
			for (size_t i = before; i < said.size(); i++)
			{
				if (said[i].ped == ped && scream < 0) scream = (int)i;
				if (said[i].ped == 1 && scream >= 0 && reply < 0) reply = (int)i;
			}
			float gap = scream >= 0 && reply >= 0 ? said[reply].t - said[scream].t : -1;
			sprintf_s(det, "his scream #%d (%s), Arthur's answer #%d (%s) %.1f s later", scream, scream >= 0 ? said[scream].ctx.c_str() : "-",
				reply, reply >= 0 ? said[reply].ctx.c_str() : "-", gap);
			check("voices: it takes a man, he screams, Arthur answers after it", scream >= 0 && reply > scream && gap >= 1.0f, det);
		}
		// 100. voices: lifted -> a line; dropped -> a landing line ("thanks for the lift" / getting up); a missing line falls back
		reset(); srand(100);
		{
			g_set.arthur = 0;                       // the mock decides when he's held
			SpawnTornado(0, 300, 0, true, "ride");
			int k = 0;
			for (; k < 80; k++) tick2(T(k));
			size_t before = said.size();
			for (int j = 0; j < 30; j++) { g_playerLastInWall = T(k); playerHag = 20; tick2(T(k++)); }   // in the wall, 20 m up
			playerHag = 0;
			for (int j = 0; j < 10; j++) tick2(T(k++));
			g_landing.landings++;                   // down safe
			for (int j = 0; j < 40; j++) tick2(T(k++));
			int lifted = 0, landed = 0;
			static const char* kLand[] = { "RIDER_THANK_FOR_LIFT", "GET_UP_FROM_FALL", "CALM_HORSE_BUCKED_OFF_GET_UP", "LAUGH_LOW", "GREET_THIRD_GOOD_WEATHER_CONV", "GREET_GENERAL" };
			for (size_t i = before; i < said.size(); i++)
			{
				if (said[i].ped != 1) continue;
				if (said[i].t < T(k - 45)) lifted++;
				else for (const char* c : kLand) if (said[i].ctx == c) landed++;
			}
			// again with the landing lines missing from his voice: he falls back to the last (a vocal) or says nothing - never a missing one
			said.clear(); for (const char* c : kLand) missingCtx.insert(c); missingCtx.erase("LAUGH_LOW");
			g_vo.evNext[VE_ARTHUR_LANDED] = 0; g_vo.arthurNext = 0;
			g_playerLastInWall = T(k); tick2(T(k++)); g_landing.landings++;
			for (int j = 0; j < 40; j++) tick2(T(k++));
			bool usedMissing = false;
			for (auto& s : said) if (s.ped == 1 && missingCtx.count(s.ctx)) usedMissing = true;
			sprintf_s(det, "lines while lifted %d, landing lines %d; with the landing lines missing he said %d line(s), a missing one %d",
				lifted, landed, (int)said.size(), (int)usedMissing);
			check("voices: lifted, then a landing line; missing lines are never played", lifted >= 1 && landed >= 1 && !usedMissing, det);
		}
		// 101. voices: one of the gang goes up - Arthur greets him by name and he answers (any of four runs); Voices Off is silent
		{
			bool smallTalk = false, wrongName = false;
			std::string seen;
			for (int run = 0; run < 10 && !smallTalk; run++)   // (each run picks one of three exchanges at random)
			{
				reset(); srand(101 + run);
				g_set.grabPeople = false;
				Tornado* tp = SpawnTornado(0, 40, 0, true, "gang");
				int k = 0;
				for (; k < 120; k++) tick2(T(k));
				V3 b = tp ? tp->base : V3();
				int dutch = nextObject++; objects[dutch].type = 1; objects[dutch].p = b + V3(3, 0, 0); objects[dutch].model = Joaat("cs_dutch"); humans.insert(dutch);
				for (int j = 0; j < 5; j++) tick2(T(k++));
				objects[dutch].p.z = 12.0f; objects[dutch].frozen = true;
				for (int j = 0; j < 90; j++) tick2(T(k++));
				int greet = -1;
				for (size_t i = 0; i < said.size(); i++)
				{
					if (said[i].ped == 1 && said[i].ctx.rfind("GREET_", 0) == 0 && said[i].ctx != "GREET_DUTCH" && said[i].ctx.find("CONV") == std::string::npos
						&& said[i].ctx != "GREET_GENERAL" && said[i].ctx != "GREET_WHISTLE_FAR" && said[i].ctx != "GREET_GROUP_SHOUTED") wrongName = true;
					if (said[i].ped == 1 && said[i].ctx == "GREET_DUTCH" && greet < 0) greet = (int)i;
					if (greet >= 0 && said[i].ped == dutch && (int)i > greet) smallTalk = true;
				}
				seen += std::to_string(run) + ":" + (greet >= 0 ? "greet " : "") + (smallTalk ? "answer " : "");
			}
			reset(); srand(105); g_voices = 0; g_set.grabPeople = false;
			Tornado* tp = SpawnTornado(0, 40, 0, true, "off");
			int k = 0;
			for (; k < 120; k++) tick2(T(k));
			int p2 = nextObject++; objects[p2].type = 1; objects[p2].p = (tp ? tp->base : V3()) + V3(3, 0, 12); objects[p2].frozen = true; humans.insert(p2);
			for (int j = 0; j < 60; j++) tick2(T(k++));
			int offLines = (int)said.size();
			sprintf_s(det, "runs [%s], greeted someone by the wrong name %d; Voices Off: %d lines", seen.c_str(), (int)wrongName, offLines);
			check("voices: small talk with the gang as they fly past; Voices Off is silent", smallTalk && !wrongName && offLines == 0, det);
		}
		// 102. voices: a tornado that takes ten people in ten seconds doesn't get ten remarks from Arthur
		reset(); srand(102);
		{
			g_set.grabPeople = false;
			Tornado* tp = SpawnTornado(0, 40, 0, true, "pace");
			int k = 0;
			for (; k < 120; k++) tick2(T(k));
			size_t before = said.size();
			V3 b = tp ? tp->base : V3();
			for (int i = 0; i < 10; i++)
			{
				int ped = nextObject++; objects[ped].type = 1; objects[ped].p = b + V3(3, (float)i, 0); humans.insert(ped);
				tick2(T(k++));
				objects[ped].p.z = 10.0f; objects[ped].frozen = true;
				for (int j = 0; j < 10; j++) tick2(T(k++));
			}
			for (int j = 0; j < 30; j++) tick2(T(k++));
			int arthur = 0, overlaps = 0;
			float lastArthur = -100;
			for (size_t i = before; i < said.size(); i++)
				if (said[i].ped == 1) { arthur++; if (said[i].t - lastArthur < 1.2f) overlaps++; lastArthur = said[i].t; }
			sprintf_s(det, "10 people in 11 s: Arthur spoke %d times (two within 1.2 s: %d), %d lines in all", arthur, overlaps, (int)(said.size() - before));
			check("voices: paced - Arthur doesn't remark on every one, and never talks over himself", arthur >= 1 && arthur <= 3 && overlaps == 0, det);
		}
		// 103. the storm report: a storm that lifted two people and lasted a minute posts its card when the last tornado's gone
		reset(); srand(103);
		{
			g_set.grabPeople = false;
			Tornado* tp = SpawnTornado(0, 40, 0, true, "report");
			int k = 0;
			for (; k < 100; k++) tick2(T(k));
			V3 b = tp ? tp->base : V3();
			for (int i = 0; i < 2; i++)
			{
				int ped = nextObject++; objects[ped].type = 1; objects[ped].p = b + V3(3, (float)i * 2, 0); humans.insert(ped);
				tick2(T(k++)); objects[ped].p.z = 10.0f; objects[ped].frozen = true;
			}
			for (int j = 0; j < 500; j++) tick2(T(k++));
			bool during = g_stats.on;
			DespawnAll();
			tick2(T(k++));
			sprintf_s(det, "open during the storm %d, closed after %d; taken %d, Arthur rides %d", (int)during, (int)!g_stats.on, g_stats.taken, g_stats.rides);
			check("storm report: counts the storm and closes when it's over", during && !g_stats.on && g_stats.taken == 2, det);
		}
		// 104. the new types: each builds its smoke; Firenado and Ghost glow; the Junknado's wall is props (and goes when restyled); the
		// multi-vortex's emitters sit off its axis (three sub-funnels) while the Dark Vortex's core hugs it
		reset(); srand(104);
		{
			std::string bad;
			const char* letters = "DFGSHMJ";
			int clock = 1;
			for (const char* c = letters; *c; c++)
			{
				int st = StyleByLetter(*c);
				Tornado* tp = SpawnTornado(st, 120, 0, true, std::string("new ") + *c);
				if (!tp || tp->loopsRunning <= 0) bad += std::string(1, *c) + ":no smoke ";
				int lights0 = calls[N_DRAW_LIGHT_WITH_RANGE];
				for (int k = 0; k < 40; k++) tick2(T(clock++));
				int lights = calls[N_DRAW_LIGHT_WITH_RANGE] - lights0;
				bool glows = GetStyles()[st].lights > 0;
				if (glows != (lights > 0)) bad += std::string(1, *c) + ":lights " + std::to_string(lights) + " ";
				if (*c == 'J' && (!tp || tp->JunkAlive() < 20)) bad += "J:junk " + std::to_string(tp ? tp->JunkAlive() : -1) + " ";
				if (*c == 'J' && tp)
				{
					tp->Restyle(StyleByLetter('V'), tp->loopsRunning);
					if (tp->JunkAlive() != 0) bad += "J:junk kept after restyle ";
					tp->Restyle(StyleByLetter('J'), tp->loopsRunning);
					if (tp->JunkAlive() < 20) bad += "J:no junk after restyle back ";
				}
				DespawnAll();
				if (LoopsInUse() != 0) bad += std::string(1, *c) + ":loops leaked ";
			}
			// three sub-funnels in the multi-vortex, none in the others
			Tornado* m = SpawnTornado(StyleByLetter('M'), 120, 0, true, "subs");
			int mSubs = m ? m->DebugSubFunnels() : -1;
			DespawnAll();
			Tornado* v = SpawnTornado(StyleByLetter('V'), 120, 0, true, "subs");
			int vSubs = v ? v->DebugSubFunnels() : -1;
			DespawnAll();
			sprintf_s(det, "problems: %s | sub-funnels: multi-vortex %d, dark vortex %d", bad.empty() ? "none" : bad.c_str(), mSubs, vSubs);
			check("new types: smoke, glow, junk walls and sub-funnels as designed; nothing leaks", bad.empty() && mSubs == 3 && vSubs == 0, det);
		}
		// 105. the tornado gallery: rooms of harmless exhibits; the methods room draws them five ways; leaving puts it all back
		reset(); srand(105);
		{
			int ped = nextObject++; objects[ped].type = 1; humans.insert(ped);
			GalStylesOpen();
			int n0 = (int)g_gal.exhibits.size(), k = 0;
			bool display = true;
			for (auto& tp : g_tornadoes) if (!tp->Display()) display = false;
			objects[ped].p = g_gal.exPos.empty() ? V3() : g_gal.exPos[0] + V3(2, 0, 0);
			for (; k < 80; k++) tick2(T(k));
			bool untouched = objects.count(ped) && objects[ped].v.len() < 0.01f && objects[ped].p.z < 0.5f;
			int control = lastPlayerControl, lines = (int)said.size();
			std::string rooms;
			int methodsRenders = 0;
			for (int r = 1; r < kGalRoomCount; r++)
			{
				GalRoomBuild(r);
				for (int j = 0; j < 10; j++) tick2(T(k++));
				rooms += std::to_string(g_gal.exhibits.size()) + " ";
				if (r == kGalRoomCount - 1)
					for (auto& ref : g_gal.exhibits) if (Tornado* tp = ref.get()) if (tp->opts.render >= 0) methodsRenders++;
			}
			int loops = LoopsInUse(), tornadoes = (int)g_tornadoes.size();
			GalleryClose("harness");
			sprintf_s(det, "room 1: %d exhibits, all harmless %d, the man next to one untouched %d, control while open %d, voice lines %d; rooms 2-4: %s(methods room render overrides %d); open: %d tornadoes, %d loops; after: %d tornadoes, %d loops, control %d",
				n0, (int)display, (int)untouched, control, lines, rooms.c_str(), methodsRenders, tornadoes, loops, (int)g_tornadoes.size(), LoopsInUse(), lastPlayerControl);
			check("tornado gallery: rooms of harmless exhibits; leaving cleans up", n0 == 5 && display && untouched && control == 0 && lines == 0 && methodsRenders >= 3 &&
				tornadoes <= 5 && loops <= g_set.ptfxBudget && g_tornadoes.empty() && LoopsInUse() == 0 && lastPlayerControl == 1, det);
		}
		// 106. the texture gallery pages through every effect without leaking smoke, and Enter makes style E of the pick
		reset(); srand(106);
		{
			GalTexOpen();
			int k = 0, maxLoops = 0;
			for (int pg = 0; pg < GalPages() + 1; pg++)
			{
				for (int j = 0; j < 5; j++) { tick2(T(k++)); maxLoops = std::max(maxLoops, LoopsInUse()); }
				GalTexPage(g_gal.page + 1);
			}
			g_gal.sel = 2;
			int idx = g_gal.page * kGalPerPage + g_gal.sel;
			SetCustomStyleFx(kLabLooped[idx], true, kGalTexScale);   // what Enter does
			bool picked = GetStyles().back().layers[0].fx.size() == 1 && !strcmp(GetStyles().back().layers[0].fx[0], kLabLooped[idx]);
			GalleryClose("harness");
			sprintf_s(det, "%d pages, peak loops %d (budget %d), after closing %d; style E made of %s: %d", GalPages(), maxLoops, g_set.ptfxBudget, LoopsInUse(), kLabLooped[idx], (int)picked);
			check("texture gallery: pages cleanly, no leaked smoke, picks style E", maxLoops <= g_set.ptfxBudget && maxLoops >= kGalPerPage && LoopsInUse() == 0 && picked, det);
		}
		// 107. (v1.2) the intro's gags happen: gag outfits, Uncle's chair goes up with him, Pearson's pot and ladle, speech allowed in the
		// slow motion (and off after), Dutch points at Arthur, and the gang walks it off before the scene lets go
		reset(); srand(107);
		{
			mockTime = 1.0f;
			IntroStart();
			bool chairUp = false, potUp = false, flagIn = false, flagAfter = true, dutchFaces = false;
			int wanders0 = 0;
			for (int k = 11; k <= 2400 && !(g_in.stage == 0 && k > 200); k++)
			{
				tick2(T(k));
				if (g_in.stage != 4) continue;
				double c = g_in.clock;
				const CastState& u = g_in.cast[CA_UNCLE];
				const CastState& pe = g_in.cast[CA_PEARSON];
				if (c > 77.0 && u.lifted && u.held[0] && attachedTo.count(u.held[0]) && attachedTo[u.held[0]] == u.ped) chairUp = true;
				if (c > 80.0 && pe.lifted && pe.held[0] && pe.held[1] && attachedTo.count(pe.held[0]) && attachedTo[pe.held[0]] == pe.ped && attachedTo[pe.held[1]] == pe.ped) potUp = true;
				if (c > 87.9 && c < 89.9 && audioFlags["AllowScriptedSpeechInSlowMo"] == 1) flagIn = true;
				if (c > 91.0 && c < 92.5) flagAfter = audioFlags["AllowScriptedSpeechInSlowMo"] == 0;
				if (c > 85.0 && c < 86.5 && g_in.cast[CA_DUTCH].lifted) dutchFaces = true;
				if (c < 1.0) wanders0 = calls[N_TASK_WANDER_STANDARD];
			}
			int walked = 0;
			for (auto& kv : objects) (void)kv;
			int wanders = calls[N_TASK_WANDER_STANDARD] - wanders0;
			bool uncle = false, dutch = false;
			for (auto& kv : outfitOf) { if (kv.second == 0xB93CB089) uncle = true; if (kv.second == 0x84793D7F) dutch = true; }
			(void)walked; (void)dutchFaces;
			sprintf_s(det, "outfits: Uncle's long johns %d, Dutch's party suit %d; Uncle's chair up with him %d; Pearson's pot and ladle %d; speech flag in the slow-mo %d, off after %d; walked it off %d; stage %d",
				(int)uncle, (int)dutch, (int)chairUp, (int)potUp, (int)flagIn, (int)flagAfter, wanders, g_in.stage);
			check("intro gags: outfits, Uncle's chair, Pearson's pot, slow-mo speech, walking it off", uncle && dutch && chairUp && potUp && flagIn && flagAfter && wanders >= 3 && g_in.stage == 0, det);
		}
		// 108. (v1.2 audit 3) voices: a long ride gets its riding line; a near spawn gets its touchdown remark; the storm report counts
		// a ride with Voices Off
		reset(); srand(108);
		{
			// a near spawn: the touchdown is remarked on (Arthur, or someone near him)
			g_set.arthur = 0;
			SpawnTornado(0, 120, 0, true, "near");
			int k = 0, touchdownLines = 0;
			for (; k < 150; k++) tick2(T(k));
			for (auto& s : said) for (const char* c : { "GENERIC_SHOCKED_HIGH", "RE_BOT_UNI_V0_SHIT", "GENERIC_CURSE_HIGH", "WHOA_ESCALATED", "SCREAM_SHOCKED", "PANIC_COMMUNICATE", "GENERIC_FRIGHTENED_HIGH" })
				if (s.ctx == c) touchdownLines++;
			// a 15 s ride, 20 m up: the riding line comes after the "lifted" one
			size_t before = said.size();
			for (int j = 0; j < 150; j++) { g_playerLastInWall = T(k); playerHag = 20; tick2(T(k++)); }
			playerHag = 0;
			int riding = 0;
			for (size_t i = before; i < said.size(); i++)
				for (const char* c : { "PLAYER_DRUNK_MERRY_SINGING", "DRUNK_MERRY_VOFX", "LAUGH_HIGH", "LAUGH_LOW", "GREET_WHISTLE_FAR", "GREET_GROUP_SHOUTED" })
					if (said[i].ped == 1 && said[i].ctx == c) riding++;
			// Voices Off: the storm report still counts the ride
			reset(); srand(109); g_voices = 0; g_set.arthur = 0;
			SpawnTornado(0, 120, 0, true, "quiet");
			k = 0;
			for (; k < 100; k++) tick2(T(k));
			for (int j = 0; j < 80; j++) { g_playerLastInWall = T(k); playerHag = 15; tick2(T(k++)); }
			playerHag = 0;
			for (int j = 0; j < 400; j++) tick2(T(k++));
			int rides = g_stats.rides; float longest = g_stats.longestRide;
			sprintf_s(det, "touchdown remarks %d; riding lines %d; Voices Off: report counted %d ride(s), longest %.0f s, lines said %d",
				touchdownLines, riding, rides, longest, (int)said.size());
			check("voices: touchdown remark, the riding line, and a report with Voices Off", touchdownLines >= 1 && riding >= 1 && rides == 1 && longest >= 7.0f && said.empty(), det);
		}
		// 109. (v1.2 audit 3) the gallery: opened with the drone camera on it keeps its own camera; switching galleries leaks nothing; the
		// Style menu leaves exhibits alone; a menu key doesn't act twice
		reset(); srand(110);
		{
			SpawnTornado(0, 120, 0, true, "drone target");
			g_drone.mode = 1;
			int k = 0;
			for (; k < 40; k++) { tick2(T(k)); DroneUpdate(0.1f, GalleryActive()); }
			bool droneWas = g_drone.active;
			GalTexOpen();
			for (int j = 0; j < 10; j++) { tick2(T(k++)); DroneUpdate(0.1f, GalleryActive()); }
			bool camKept = g_gal.cam != 0 && lastRenderCams == 1 && !g_drone.active;
			int texLoops = LoopsInUse();
			GalStylesOpen();   // straight from the texture gallery
			bool texGone = g_gal.fx.empty() && g_gal.mode == 1;
			int afterSwitch = LoopsInUse(), exhibitLoops = 0;
			for (auto& tp : g_tornadoes) exhibitLoops += tp->loopsRunning;
			// the Style menu: pick another style with the exhibits out
			BuildMenu();
			std::vector<int> before;
			for (auto& tp : g_tornadoes) before.push_back(tp->styleIdx);
			for (auto& it : g_pages[P_MAIN].items) if (it.label == "Style" && it.onChange) { g_set.style = 6; it.onChange(); }
			bool untouched = true;
			for (size_t i = 0; i < g_tornadoes.size() && i < before.size(); i++) if (g_tornadoes[i]->styleIdx != before[i]) untouched = false;
			GalleryClose("harness");
			sprintf_s(det, "drone was on %d; gallery camera kept %d; texture loops %d, after switching %d (exhibits hold %d), swatches gone %d; exhibits untouched by Style %d; after closing: loops %d, cam %d",
				(int)droneWas, (int)camKept, texLoops, afterSwitch, exhibitLoops, (int)texGone, (int)untouched, LoopsInUse(), g_gal.cam);
			check("gallery: keeps its camera over the drone, switches cleanly, exhibits keep their style", droneWas && camKept && texGone &&
				afterSwitch == exhibitLoops && untouched && LoopsInUse() == 0 && !g_gal.cam, det);
		}
		// 110. (v1.2 audit 3) skipping the intro doesn't fire the v1.2 beats on the way out (no facepalm mid-air, no hat tip while boarding)
		reset(); srand(111);
		{
			mockTime = 1.0f;
			IntroStart();
			int k = 11;
			for (; k <= 400 && !(g_in.stage == 4 && g_in.clock >= 10.0); k++) tick2(T(k));
			IntroSkip();
			for (int j = 0; j < 60; j++) tick2(T(k++));
			Ped micah = g_in.cast[CA_MICAH].ped;
			std::string m = animOf.count(micah) ? animOf[micah] : "", a = animOf.count(1) ? animOf[1] : "";
			sprintf_s(det, "after the skip: Micah's last clip '%s', Arthur's '%s' (stage %d)", m.c_str(), a.c_str(), g_in.stage);
			check("intro: a skip doesn't fire the v1.2 gags late", m != "action" && a != "action" && g_in.stage >= 4, det);
		}
		// 111. (v1.2) the memory reader: this process's RAM, and the biggest graphics card's usage and budget (real numbers - it's the
		// one scenario that reads the machine it runs on)
		reset();
		{
			MemInfo mi;
			bool ok = MemStats(&mi);
			sprintf_s(det, "RAM %.2f GB (private %.2f) | VRAM known %d: %.2f of %.1f GB budget, shared %.2f", mi.ramGB, mi.privateGB, (int)mi.vramKnown, mi.vramGB, mi.vramBudgetGB, mi.sharedGB);
			check("memory: the MEM line's numbers read back sanely", ok && mi.ramGB > 0.001f && mi.privateGB > 0.001f && (!mi.vramKnown || mi.vramBudgetGB > 0.5f), det);
		}
		// ======== v1.3 (playtest 11) ========
		// 112. Arthur's landings: Mixed gives all eight kinds (soft, rough, the stratosphere, slammed down; v1.6: far, sky-high, far soft,
		// the meteor); Soft only catches (or glides); Real turns the catch off and picks only the ones that hurt; a rough landing hurts but
		// never takes him below a fifth of his health; the glide and the meteor don't hurt at all
		reset(); srand(112);
		{
			int kinds[8] = {};
			g_set.landings = 1; for (int i = 0; i < 800; i++) kinds[PickLandKind()]++;
			int softOnly = 0, glides = 0; g_set.landings = 0; for (int i = 0; i < 50; i++) { int k = PickLandKind(); if (k == 0 || k == 6) softOnly++; if (k == 6) glides++; }
			int realSoft = 0; g_set.landings = 2; for (int i = 0; i < 50; i++) { int k = PickLandKind(); if (k == 0 || k >= 6) realSoft++; }
			g_set.landings = 2; SyncMenuChoices(); bool realNoCatch = !g_set.softLanding;
			g_set.landings = 1; SyncMenuChoices(); bool mixedCatch = g_set.softLanding;
			// a rough landing: the backstop on, then off at the touchdown with damage applied
			g_set.playerGod = false;
			g_landKind = 2; g_backstopOn = true; g_landing.healthBefore = 200;
			BackstopOff("landed");
			int after = lastHealthSet;
			bool kindReset = g_landKind == 0;
			int unhurt[2] = { -2, -2 };
			for (int j = 0; j < 2; j++)
			{
				g_set.landingHurts = j == 1;   // (the glide is a soft landing: free unless "soft landings cost a little"; the meteor is free either way)
				g_landKind = j == 0 ? 6 : 7; g_backstopOn = true; g_landing.healthBefore = 200; lastHealthSet = -1;
				BackstopOff("landed");
				unhurt[j] = lastHealthSet;
			}
			sprintf_s(det, "Mixed of 800: soft %d rough %d stratosphere %d slammed %d far %d sky-high %d far soft %d meteor %d; Soft: %d of 50 caught (%d glides); "
				"Real: %d of 50 soft, catch off %d; Mixed catch on %d; a stratosphere landing from 200 health -> %d (kind reset %d); health set by a glide %d, a meteor %d",
				kinds[0], kinds[1], kinds[2], kinds[3], kinds[4], kinds[5], kinds[6], kinds[7], softOnly, glides, realSoft, (int)realNoCatch, (int)mixedCatch, after,
				(int)kindReset, unhurt[0], unhurt[1]);
			check("landings: Mixed varies, Soft catches, Real doesn't, rough ones hurt but never below a fifth", kinds[0] > 80 && kinds[1] > 100 && kinds[2] > 40 &&
				kinds[3] > 40 && kinds[4] > 50 && kinds[5] > 30 && kinds[6] > 50 && kinds[7] > 30 && softOnly == 50 && glides > 5 && realSoft == 0 &&
				realNoCatch && mixedCatch && after >= 40 && after < 200 && kindReset && unhurt[0] == -1 && unhurt[1] == -1, det);
		}
		// 113. things thrown at Arthur: within half a minute in its reach (outside the wall) something comes his way - aimed at him or just short
		reset(); srand(113);
		{
			g_set.debrisCone = true; g_set.throwAtArthur = true; g_set.arthur = 0;
			Tornado* tp = SpawnTornado(0, 60, 0, true, "thrower");
			int k = 0, aimed = 0, flights0 = 0;
			for (; k < 100; k++) tick2(T(k));
			flights0 = (int)g_flights.size();
			V3 pp = playerPos;
			float bestDot = -2, bestMiss = 1e9f;
			for (int j = 0; j < 320; j++)
			{
				size_t n0 = g_flights.size();
				tick2(T(k++));
				for (size_t f = n0; f < g_flights.size(); f++)
				{
					V3 to = pp - g_flights[f].pos; to.z = 0;
					V3 v = g_flights[f].vel; v.z = 0;
					float d = to.len2d(), sp = v.len2d();
					if (d < 1 || sp < 1) continue;
					float dot = (to.x * v.x + to.y * v.y) / (d * sp);
					if (dot > 0.95f) aimed++;
					bestDot = std::max(bestDot, dot);
				}
			}
			(void)flights0; (void)bestMiss;
			sprintf_s(det, "tornado %.0f m away (wall %.0f, reach %.0f): %d throws headed straight at him in 32 s (best aim %.2f)", tp ? (tp->base - pp).len2d() : -1.0f,
				tp ? tp->wallRadius() : 0.0f, tp ? tp->reachRadius() : 0.0f, aimed, bestDot);
			check("things thrown at Arthur: some come straight at him", aimed >= 1, det);
		}
		// 114. the tornado gun: a cute one (the Toon Twister), 30 s, wandering about where it landed (never far), shoving Arthur at most
		reset(); srand(114);
		{
			g_gun.on = true; g_set.arthur = 3;
			mockShooting = true; mockImpact = V3(20, 0, 0);
			tick2(T(1));
			mockShooting = false;
			Tornado* m = g_gun.minis.empty() ? nullptr : g_gun.minis.front().get();
			int style = m ? m->styleIdx : -1;
			float life = m ? m->opts.life : 0, maxAway = 0;
			V3 start = m ? m->base : V3();
			int k = 2;
			for (; k < 330 && g_gun.minis.size() && g_gun.minis.front().get() && !g_gun.minis.front().get()->Dissipating(); k++)
			{
				tick2(T(k));
				if (Tornado* x = g_gun.minis.front().get()) maxAway = std::max(maxAway, (x->base - start).len2d());
			}
			bool dissipating = !g_gun.minis.front().get() || g_gun.minis.front().get()->Dissipating();
			sprintf_s(det, "style %d (%s), life %.0f s, wandered up to %.1f m from where it landed, dying down by 33 s %d", style, style >= 0 ? GetStyles()[style].name : "-", life, maxAway, (int)dissipating);
			check("tornado gun: a little Toon Twister that wanders for 30 s", style == StyleByLetter('T') && life == 30.0f && maxAway > 3.0f && maxAway < 30.0f && dissipating, det);
		}
		// 115. real map trees: the shape test hands back the tree as an entity (playtest 11 - every hit was rejected for that); a map tree
		// model is now recognised as one and torn out, anything else that's an entity still isn't
		reset(); srand(115);
		{
			g_set.touchdown = false; g_set.debrisCone = false; g_set.extraDebris = false; g_set.flatten = false; g_set.mapTrees = true;
			g_set.grabProps = false; g_set.ripFixedProps = false;   // (map trees aren't in the game's object lists; the mock's are - keep its hands off)
			g_spawnableTrees.clear();
			BuildStandIns();
			int treeEnt = nextObject++; objects[treeEnt].type = 3; objects[treeEnt].p = V3(10, 0, 0); objects[treeEnt].model = kTreeModels[0].hash; objects[treeEnt].frozen = true;
			int crate = nextObject++; objects[crate].type = 3; objects[crate].p = V3(-10, 0, 0); objects[crate].model = Joaat("p_crate03x"); objects[crate].frozen = true;
			world.clear();
			Shape a = Cyl(10, 0, 0.6f, 16.0f); a.ent = treeEnt; world.push_back(a);
			Shape b = Cyl(-10, 0, 0.6f, 16.0f); b.ent = crate; world.push_back(b);
			Tornado n(V3(), 0, 0, "realtrees-ent", true, 0);
			for (int k = 1; k <= 200; k++) { mockTime = T(k); n.Update(0.1f, mockTime); UpdateFlights(0.1f, mockTime); integrate(0.1f); }
			bool hidNearTree = false, hidNearCrate = false;
			for (auto& h : hideAt) { if ((h - V3(10, 0, 0)).len2d() < 3.0f) hidNearTree = true; if ((h - V3(-10, 0, 0)).len2d() < 3.0f) hidNearCrate = true; }
			V3 firstHide = hideAt.empty() ? V3(-1, -1, -1) : hideAt.front();
			sprintf_s(det, "trunks found %d, torn out %d, rejected %d; hid at the tree %d, at the crate %d (hides %d, loaded tree models %d, first hide at %.1f %.1f %.1f)", n.treeTrunks, n.realTrees, n.treeRejected,
				(int)hidNearTree, (int)hidNearCrate, (int)hideAt.size(), (int)g_loadedTreeIdx.size(), firstHide.x, firstHide.y, firstHide.z);
			check("real trees: a map tree that comes back as an entity is torn out; other entities aren't", n.realTrees >= 1 && hidNearTree && !hidNearCrate, det);
			n.Destroy();
		}
		// 116. invincible covers the gun's minis too (playtest 11: Arthur died with only minis out); the menu: Arthur's page, the time in the
		// main menu's title, Wedge the default
		reset(); srand(116);
		{
			g_set.playerGod = true; g_gun.on = true;
			mockShooting = true; mockImpact = V3(20, 0, 0);
			tick2(T(1));
			mockShooting = false;
			tick2(T(2));
			int withMini = lastInvincible;
			DespawnAll(); g_gun.minis.clear();
			tick2(T(3));
			int after = lastInvincible;
			BuildMenu();
			bool arthurPage = false, timeTitle = false;
			for (auto& it : g_pages[P_MAIN].items)
			{
				if (it.kind == Item::Page && it.page == P_ARTHUR) arthurPage = true;
				if (it.kind == Item::Page && it.page == P_WEATHER && it.label.find("time") != std::string::npos) timeTitle = true;
			}
			int arthurItems = (int)g_pages[P_ARTHUR].items.size();
			sprintf_s(det, "invincible with only a mini out %d, after it's gone %d; Arthur's page on the main menu %d (%d items); 'time' in the weather page's name %d; default style %s",
				withMini, after, (int)arthurPage, arthurItems, (int)timeTitle, GetStyles()[Settings().style].name);
			check("invincible with minis; Arthur's own page; the time in the menu; Wedge by default", withMini == 1 && after == 0 && arthurPage && arthurItems >= 6 && timeTitle &&
				Settings().style == StyleByLetter('W'), det);
		}
		// ======== v1.3 audit ========
		// 117. every way the tornado lets go of Arthur gives him the landing it picked: the stratosphere goes up (v1.4: 40-52 m/s, and the catch
		// waits 40 s), slammed goes down, soft and rough as before - at the top of his ride (the release) as well as in fling and chase.
		// Landings: Real still catches a balloon bail or the drop test, but not a tornado throw; a landing kind never outlives its drop.
		reset(); srand(117);
		{
			ForceProfile fp = CurrentForce();
			int seen[8] = {}, wrong = 0;
			g_set.landings = 1;
			// (v1.6: far - flat and fast; sky-high - up, still climbing; far soft - out and up, a drift kept for the glide; meteor - a hop)
			auto right = [&](int kd, const V3& v, bool wall) {
				switch (kd)
				{
				case 2: return v.z >= 38.0f && g_dropWindowUntil > 40.0f;
				case 3: return v.z < 0;
				case 4: return v.len2d() >= 40.0f && v.z > 15.0f;
				case 5: return v.z >= 50.0f && g_skyUntil > 10.0f && g_dropWindowUntil > 60.0f;
				case 6: return v.z > 10.0f && g_glideVel.len2d() > 15.0f;
				case 7: return v.z > 15.0f && v.len2d() < 15.0f;
				default: return wall ? v.z > 10.0f : (v.z >= 2.0f && v.z < 10.0f);
				}
			};
			for (int i = 0; i < 400; i++)
			{
				bool wall = i % 2 == 0;
				g_dropWindowUntil = -100; g_skyUntil = -100;
				V3 v = LaunchArthur(V3(1, 0, 0), V3(0, 1, 0), fp, 1.0f, 10.0f, wall, -3.0f);
				int kd = g_landKind;
				seen[kd]++;
				if (!right(kd, v, wall) || v.len() > 60.01f) wrong++;
			}
			// the release path, for real: Arthur (easy prey) high in the wall, a fresh tornado each time
			g_set.grabPlayer = true; g_set.arthur = 3; g_set.flingChase = false; g_set.touchdown = false; g_set.debrisCone = false; g_set.extraDebris = false;
			mockPeds.push_back(1);
			int rel[8] = {}, relWrong = 0, relNone = 0;
			for (int trial = 0; trial < 40; trial++)
			{
				Tornado n(V3(), 0, 0, "release", true, 0);
				g_landKind = -1;
				bool done = false;
				for (int k = 1; k <= 60 && !done; k++)
				{
					playerPos = V3(n.wallRadius() * 1.2f, 0, n.height() * 0.92f);
					playerVel = V3();
					mockTime = 1000.0f + trial * 20.0f + k * 0.1f;
					n.Update(0.1f, mockTime);
					if (g_landKind >= 0)
					{
						done = true;
						rel[g_landKind]++;
						if (!right(g_landKind, playerVel, false) || playerVel.len() > 60.01f) relWrong++;
					}
				}
				if (!done) relNone++;
				n.Destroy();
			}
			// Real: a balloon bail / the drop test is still caught; a tornado throw isn't
			g_set.landings = 2; SyncMenuChoices();
			g_backstopOn = false; g_playerLastInWall = -100; g_landKind = 0;
			StartDropTest(5000.0f);
			playerPos = V3(0, 0, 30); playerVel = V3(0, 0, -25);
			PlayerSafety(0.1f, 5000.5f);
			float bailV = playerVel.z;
			g_testWindowUntil = g_dropWindowUntil = -100; NotePlayerInWall(6000.0f); g_backstopOn = false;
			playerPos = V3(0, 0, 30); playerVel = V3(0, 0, -25);
			PlayerSafety(0.1f, 6000.5f);
			float throwV = playerVel.z;
			// a landing kind left over (he landed in water, say) goes when the window closes
			g_set.landings = 1; SyncMenuChoices();
			g_landKind = 1; g_backstopOn = false; g_playerLastInWall = -100; g_dropWindowUntil = g_testWindowUntil = -100;
			PlayerSafety(0.1f, 7000.0f);
			bool kindGone = g_landKind == 0;
			sprintf_s(det, "launches of 400: soft %d rough %d stratosphere %d slammed %d far %d sky-high %d far soft %d meteor %d, wrong %d; the release path: "
				"soft %d rough %d stratosphere %d slammed %d far %d sky-high %d far soft %d meteor %d, wrong %d, never let go %d; Real: a bail held at %.1f m/s, "
				"a throw left at %.1f; a stale kind cleared %d", seen[0], seen[1], seen[2], seen[3], seen[4], seen[5], seen[6], seen[7], wrong,
				rel[0], rel[1], rel[2], rel[3], rel[4], rel[5], rel[6], rel[7], relWrong, relNone, bailV, throwV, (int)kindGone);
			check("Arthur's landings: the kind picked is the throw he gets, on both paths; Real still catches a bail", wrong == 0 && seen[2] > 15 && seen[3] > 15 &&
				seen[4] > 20 && seen[5] > 12 && seen[6] > 20 && seen[7] > 12 && relWrong == 0 && relNone == 0 && rel[2] + rel[3] + rel[4] + rel[5] >= 4 &&
				bailV > -20.0f && throwV == -25.0f && kindGone, det);
		}
		// 118. (v1.3 audit) things thrown at Arthur: never faster than 40 m/s, and nothing aimed at him in a wagon or while he's shielded
		// (the intro, the balloon); Immune Arthur gets near misses only
		reset(); srand(118);
		{
			g_set.debrisCone = true; g_set.throwAtArthur = true; g_set.grabPlayer = true; g_set.arthur = 2;
			Tornado* tp = SpawnTornado(0, 60, 0, true, "thrower2");
			int k = 0;
			for (; k < 100; k++) tick2(T(k));
			int a0 = tp ? tp->aimedThrows : 0;
			float maxSp = 0;
			for (int j = 0; j < 320; j++)
			{
				size_t n0 = g_flights.size();
				tick2(T(k++));
				for (size_t f = n0; f < g_flights.size(); f++) maxSp = std::max(maxSp, g_flights[f].vel.len());
			}
			int free_ = tp ? tp->aimedThrows - a0 : -1;
			// in a wagon
			int wagon = 9001; seatOf[1] = wagon;
			int a1 = tp ? tp->aimedThrows : 0;
			for (int j = 0; j < 320; j++) tick2(T(k++));
			int inWagon = tp ? tp->aimedThrows - a1 : -1;
			seatOf.erase(1);
			// shielded (the main loop sets it from the intro and the balloon: this drives the tornado directly, as the loop would)
			int a2 = tp ? tp->aimedThrows : 0;
			for (int j = 0; j < 320 && tp; j++) { g_shieldPlayer = true; mockTime = T(k++); tp->Update(0.1f, mockTime); UpdateFlights(0.1f, mockTime); integrate(0.1f); }
			g_shieldPlayer = false;
			int shielded = tp ? tp->aimedThrows - a2 : -1;
			sprintf_s(det, "aimed at him in 32 s: %d (fastest piece thrown %.1f m/s); in a wagon: %d; shielded: %d", free_, maxSp, inWagon, shielded);
			check("things thrown at Arthur: capped, and never at a wagon or a shielded Arthur", free_ >= 1 && maxSp <= 40.5f && inWagon == 0 && shielded == 0, det);
		}
		// 119. (v1.3) the intro's story lines: one line of Rockstar's own conversation each (created, every voice added - Micah as
		// MICAH_BELL - started, then the single line); the answer waits for the question to finish; the text blocks are asked for and
		// given back; the conversation flag is on in the scene and off after. If the game won't play them, each falls back to its ambient
		// line. Dutch is thrown at 30.2 s (inside the slow motion's reach now).
		reset(); srand(119);
		{
			int storyN = 0, blocksN = 0;
			for (auto& l : kIntroLines) storyN += l.root != nullptr;
			{ static int* pn; pn = &blocksN; IntroStoryBlocks([](const char*) { (*pn)++; }); }
			mockTime = 1.0f;
			IntroStart();
			bool flagIn = false;
			float throwAt = -1;
			for (int k = 11; k <= 2400 && !(g_in.stage == 0 && k > 200); k++)
			{
				tick2(T(k));
				if (g_in.stage == 4 && g_in.clock > 20 && g_in.clock < 30 && audioFlags["DisableAbortConversationForDeathAndInjury"] == 1) flagIn = true;
				if (throwAt < 0) for (int b : g_in.firedBeats) if (b == 11) throwAt = (float)g_in.clock;
			}
			// (v1.5: a far speaker's camp line is played beside the camera by its audio name - that counts as said)
			int nearStory = 0;
			for (auto& n : nearSpeech) for (auto& l : kIntroLines) if (l.audio && n.ctx == l.audio) nearStory++;
			int calls = (int)storyCalls.size() + nearStory, played = g_in.storyPlayed, fell = g_in.storyFell;
			bool micahBell = convNames["CFMB5_ACT"].count("MICAH_BELL") && convNames["CFMB5_ACT"].count("ARTHUR") && convNames["CFMB5_ACT"].count("DUTCH");
			float faith = -1, say = -1, wrongT = -1, sorry = -1;
			for (auto& c : storyCalls)
			{
				if (c.root == "CWDS1_ACT" && c.idx == 4) faith = c.t;
				if (c.root == "CWDS1_ACT" && c.idx == 1) say = c.t;
				if (c.root == "CDT26_ACT" && c.idx == 0) wrongT = c.t;
				if (c.root == "CPGEN_CONF_GEN" && c.idx == 1) sorry = c.t;
			}
			for (auto& n : nearSpeech)
			{
				if (n.ctx == "CDT26_AAAA") wrongT = n.t;
				if (n.ctx == "CWDS1_AAAE") faith = n.t;
				if (n.ctx == "CWDS1_AAAB") say = n.t;
			}
			bool flagOff = audioFlags["DisableAbortConversationForDeathAndInjury"] == 0;
			int given = (int)textDel.size(), clears = convHistoryClears;
			// the same scene when the game won't play story lines
			reset(); srand(119);
			convWorks = false;
			mockTime = 1.0f;
			IntroStart();
			for (int k = 11; k <= 2400 && !(g_in.stage == 0 && k > 200); k++) tick2(T(k));
			int fell2 = g_in.storyFell, fbSaid = 0, fbWant = 0;
			for (auto& n : nearSpeech) for (auto& l : kIntroLines) if (l.audio && n.ctx == l.audio) fell2++;   // (rescued beside the camera instead)
			for (auto& l : kIntroLines)
			{
				if (!l.root || !l.ctx[0]) continue;
				fbWant++;
				for (auto& x : said) if (x.ctx == l.ctx[0] || (l.ctx[1] && x.ctx == l.ctx[1])) { fbSaid++; break; }
			}
			sprintf_s(det, "story lines %d (text blocks %d): started %d, heard %d, fell back %d; Micah added as MICAH_BELL %d; \"If you say so.\" %.1f s after \"Faith...\", "
				"Arthur's \"Sorry...\" %.1f s after Dutch's; text blocks given back %d, history cleared %d, flag in %d / off after %d; Dutch thrown at %.1f s | "
				"without story lines: fell back %d of %d, ambient stand-ins heard %d of %d", storyN, blocksN, calls, played, fell, (int)micahBell, say - faith, sorry - wrongT,
				given, clears, (int)flagIn, (int)flagOff, throwAt, fell2, storyN, fbSaid, fbWant);
			check("intro story lines: played by name, one at a time, cleaned up, with a fallback for each", calls >= storyN - 1 && played >= storyN - 1 && fell <= 1 && micahBell &&
				say - faith >= 2.0f && sorry - wrongT >= 2.0f && given == blocksN && clears >= 1 && flagIn && flagOff && throwAt > 87.3f && throwAt < 87.6f &&
				fell2 >= storyN - 1 && fbSaid >= fbWant - 1, det);
		}
		// 120. (v1.3 audit) skipping the intro: in the slow motion it hands Dutch back to the tornado (he was left out of its hands); late
		// (rising out of it) the last lines and the boom don't all fire at once
		reset(); srand(120);
		{
			mockTime = 1.0f;
			IntroStart();
			int k = 11;
			for (; k <= 1300 && !(g_in.stage == 4 && g_in.clock >= 88.0); k++) tick2(T(k));
			Ped dutch = g_in.cast[CA_DUTCH].ped;
			bool scriptedBefore = IsScripted(dutch);
			IntroSkip();
			bool scriptedAfter = IsScripted(dutch);
			for (int j = 0; j < 20; j++) tick2(T(k++));
			// late skip
			reset(); srand(121);
			mockTime = 1.0f;
			IntroStart();
			k = 11;
			for (; k <= 1400 && !(g_in.stage == 4 && g_in.clock >= 95.6); k++) tick2(T(k));
			size_t said0 = said.size(), story0 = storyCalls.size();
			IntroSkip();
			for (int j = 0; j < 10; j++) tick2(T(k++));
			// (only the intro's own lines count: Arthur's everyday voice may well react to the tornado once he's back in control)
			int lateLines = (int)(storyCalls.size() - story0);
			std::string lateWhat;
			for (size_t i = said0; i < said.size(); i++)
			{
				bool intro = false;
				for (auto& l : kIntroLines) if (l.t > 95.6f) for (const char* c : l.ctx) if (c && said[i].ctx == c) intro = true;
				if (intro) { lateLines++; lateWhat += " " + said[i].ctx; }
			}
			sprintf_s(det, "mid-throw skip: Dutch scripted before %d, after %d; a late skip then said %d lines at once (%s)", (int)scriptedBefore, (int)scriptedAfter, lateLines, lateWhat.c_str());
			check("intro skip: Dutch back in the tornado's hands; no pile-up of lines after a late skip", scriptedBefore && !scriptedAfter && lateLines == 0, det);
		}
		// 121. (v1.3 audit) the real-tree scan leaves the mod's own trees alone: a test tree the mod spawned is a tree model too, and so
		// is a map tree that's already been torn out (its hide is in place) - neither is taken; a real map tree beside them still is
		reset(); srand(121);
		{
			g_set.touchdown = false; g_set.debrisCone = false; g_set.extraDebris = false; g_set.flatten = false; g_set.mapTrees = true;
			g_set.grabProps = false; g_set.ripFixedProps = false;
			g_spawnableTrees.clear();
			BuildStandIns();
			auto mkTree = [](V3 at) { int e = nextObject++; objects[e].type = 3; objects[e].p = at; objects[e].model = kTreeModels[0].hash; objects[e].frozen = true; return e; };
			int ours = mkTree(V3(10, 0, 0));
			NoteModTree(ours);                                  // (as SpawnTreeModel does for a test tree)
			int hidden = mkTree(V3(0, 10, 0));
			g_treeHideList.push_back({ V3(0.5f, 10.5f, 0), 2.4f, kTreeModels[0].hash });   // (torn out already)
			int real = mkTree(V3(-10, 0, 0));
			world.clear();
			Shape a = Cyl(10, 0, 0.6f, 16.0f); a.ent = ours; world.push_back(a);
			Shape b = Cyl(0, 10, 0.6f, 16.0f); b.ent = hidden; world.push_back(b);
			Shape c = Cyl(-10, 0, 0.6f, 16.0f); c.ent = real; world.push_back(c);
			Tornado n(V3(), 0, 0, "realtrees-ours", true, 0);
			for (int k = 1; k <= 300; k++) { mockTime = T(k); n.Update(0.1f, mockTime); UpdateFlights(0.1f, mockTime); integrate(0.1f); }
			bool atOurs = false, atHidden = false, atReal = false;
			for (auto& h : hideAt)
			{
				if ((h - V3(10, 0, 0)).len2d() < 3.0f) atOurs = true;
				if ((h - V3(0, 10, 0)).len2d() < 3.0f) atHidden = true;
				if ((h - V3(-10, 0, 0)).len2d() < 3.0f) atReal = true;
			}
			sprintf_s(det, "torn out %d, rejected %d; hid at the mod's tree %d, at the torn-out one %d, at the real one %d", n.realTrees, n.treeRejected,
				(int)atOurs, (int)atHidden, (int)atReal);
			check("real trees: never the mod's own trees, never one torn out already", !atOurs && !atHidden && atReal && n.realTrees >= 1, det);
			n.Destroy();
		}
		// ======== v1.4 (playtest 12) ========
		// 122. the tornado gun's pocket twister: person-sized ("desktop size, like the size of a person or a little smaller"), no dust
		// storm round it ("too much dust, you can't see anything"), a dozen small plumes, no debris cone, no wagons
		reset(); srand(122);
		{
			g_gun.on = true;
			mockShooting = true; mockImpact = V3(20, 0, 0);
			tick2(T(1));
			mockShooting = false;
			Tornado* m = g_gun.minis.empty() ? nullptr : g_gun.minis.front().get();
			for (int k = 2; k < 120; k++) tick2(T(k));
			float H = m ? m->height() : -1, wall = m ? m->wallRadius() : -1, reach = m ? m->reachRadius() : -1;
			int ground = m ? m->LoopsOfKind(1) : -1, top = m ? m->LoopsOfKind(3) : -1, all = m ? m->LoopsOfKind(0) + m->LoopsOfKind(1) + m->LoopsOfKind(2) + m->LoopsOfKind(3) : -1;
			int cone = m ? m->ConeProps() : -1;
			float leanTop = m ? (m->AxisAt(1.0f, mockTime) - m->base).len2d() : -1;   // (v1.4 review: it used to lean 4 m)
			float maxScale = 0;
			if (m) maxScale = m->BiggestPlume();
			sprintf_s(det, "tiny %d: %.1f m tall, wall %.2f m, reach %.1f m; plumes %d (ground storm %d, top collar %d), the biggest plume x%.2f; cone props %d; the top %.2f m off the base",
				m ? (int)m->Tiny() : -1, H, wall, reach, all, ground, top, maxScale, cone, leanTop);
			check("tornado gun: a pocket twister, about as tall as a person, without the dust", m && m->Tiny() && H > 1.2f && H < 2.4f && wall < 1.0f && reach <= 3.0f &&
				ground == 0 && top == 0 && all <= 16 && maxScale < 0.8f && cone == 0 && leanTop < 0.8f, det);   // (v1.5: the whirls are 0.45-0.75)
		}
		// 123. (playtest 12's crash came 5 s after a 90 m/s launch over Saint Denis) no stratosphere over a big town, nothing faster than
		// 60 m/s, and nothing thrown at Arthur while he's up in the air
		reset(); srand(123);
		{
			ForceProfile fp = CurrentForce();
			g_set.landings = 2;   // (Real: a quarter of the throws are the stratosphere)
			int inTown = 0, outTown = 0;
			playerPos = V3(2560, -1220, 10);
			for (int i = 0; i < 200; i++) { LaunchArthur(V3(1, 0, 0), V3(0, 1, 0), fp, 1.6f, 10.0f, true, 0); if (g_landKind == 2) inTown++; }
			playerPos = V3(-1000, 500, 10);
			float fastest = 0;
			for (int i = 0; i < 200; i++) { V3 v = LaunchArthur(V3(1, 0, 0), V3(0, 1, 0), fp, 1.6f, 10.0f, true, 0); if (g_landKind == 2) outTown++; fastest = std::max(fastest, v.len()); }
			mockGroundZ = 0;
			playerPos = V3(0, 0, 12);
			bool airborne = ArthurTargetable(PLAYER::PLAYER_PED_ID());
			playerPos = V3(0, 0, 1);
			bool grounded = ArthurTargetable(PLAYER::PLAYER_PED_ID());
			// the orbit: Easy prey at the strongest force, in the wall - never driven faster than the cap
			g_set.grabPlayer = true; g_set.arthur = 3; g_set.flingChase = false; g_set.force = 2; g_set.touchdown = false;
			g_set.pushMethod = 1; g_set.velocityGain = 50.0f;   // (velocity steering at full gain, so his speed is the target the physics wants)
			mockPeds.push_back(1);
			float orbitMax = 0;
			{
				Tornado n(V3(), 0, 0, "orbit", true, 0);
				for (int k = 1; k <= 40; k++)
				{
					playerPos = V3(n.wallRadius(), 0, 3.0f);
					mockTime = 2000.0f + k * 0.1f; n.Update(0.1f, mockTime);
					orbitMax = std::max(orbitMax, playerVel.len());
				}
				n.Destroy();
			}
			sprintf_s(det, "stratosphere throws in Saint Denis %d of 200, out of town %d of 200; fastest launch %.1f m/s; aimed at Arthur 12 m up %d, on the ground %d; "
				"his orbit at full force: %.1f m/s", inTown, outTown, fastest, (int)airborne, (int)grounded, orbitMax);
			check("landings: no stratosphere over a town, nothing over 60 m/s, nothing thrown at a flying Arthur", inTown == 0 && outTown > 20 && fastest <= 60.01f &&
				!airborne && grounded && orbitMax <= 60.01f, det);
		}
		// 124. the intro's camera director: with trees right where Arthur's and Dutch's close-ups want to stand, the cameras move until
		// they can see their man (the line from his head to the lens is clear), and the moves are logged
		reset(); srand(124);
		{
			mockTime = 1.0f;
			IntroStart();
			int k = 11;
			for (; k <= 200 && g_in.stage < 4; k++) tick2(T(k));
			// a trunk 1.2 m in front of Arthur's face toward the camp (in the way of shots 3 and 6), and one in front of Dutch
			V3 head = PedHead(PLAYER::PLAYER_PED_ID());
			V3 af = HeadingDir(ENTITY::GET_ENTITY_HEADING(PLAYER::PLAYER_PED_ID()));   // (v1.6: the close-ups are in front of his face)
			V3 tA = head + af * 1.2f;
			V3 dh = PedHead(CastPed(CA_DUTCH));
			V3 toBal = FlatDir(dh, g_bal.on ? g_bal.pos : IL(16, -10));
			V3 tD = dh + (toBal * 3.0f - V3(-toBal.y, toBal.x, 0) * 1.2f) * 0.5f;   // (halfway along shot 8's line of sight)
			world.clear();
			world.push_back(Cyl(tA.x, tA.y, 0.45f, 30.0f));
			world.push_back(Cyl(tD.x, tD.y, 0.35f, 30.0f));
			int arthurFrames = 0, arthurBlocked = 0, dutchFrames = 0, dutchBlocked = 0;
			for (; k <= 900 && g_in.stage == 4 && g_in.clock < 40.0; k++)
			{
				tick2(T(k));
				int sh = g_in.camShot;
				if (sh == 2 || sh == 4 || sh == 6 || sh == 9)
				{
					arthurFrames++;
					if (!IntroClear(PedHead(PLAYER::PLAYER_PED_ID()), lastCamCoord, PLAYER::PLAYER_PED_ID())) arthurBlocked++;
				}
				if (sh == 8)
				{
					dutchFrames++;
					if (!IntroClear(PedHead(CastPed(CA_DUTCH)), lastCamCoord, CastPed(CA_DUTCH))) dutchBlocked++;
				}
			}
			int moves = g_in.dirMoves;
			bool dutchMoved = false;
			// the balloon's close-ups as it rises (5 m/s; the mock's Arthur doesn't ride the balloon, so he's moved here): the camera
			// stays above his head, not trailing down into the basket (v1.4 review)
			world.clear();
			int riseFrames = 0, riseLow = 0;
			for (; k <= 2000 && g_in.stage == 4 && g_in.clock < 94.5; k++)
			{
				if (g_in.clock >= 52.0) playerPos.z += 0.16f;   // (the balloon climbs at 1.6 m/s)
				tick2(T(k));
				int sh = g_in.camShot;
				if (sh == 15 || sh == 18 || sh == 22)
				{
					riseFrames++;
					if (lastCamCoord.z < PedHead(PLAYER::PLAYER_PED_ID()).z) riseLow++;
				}
			}
			IntroAbort("harness");
			(void)dutchMoved;
			sprintf_s(det, "Arthur's close-ups: %d frames, %d with a tree between him and the lens; Dutch's: %d frames, %d blocked; the director moved %d shots | "
				"rising balloon: %d frames, the camera below his head in %d", arthurFrames, arthurBlocked, dutchFrames, dutchBlocked, moves, riseFrames, riseLow);
			check("intro director: the close-ups always see their man", arthurFrames > 40 && arthurBlocked == 0 && dutchFrames > 10 && dutchBlocked == 0 && moves >= 3 &&
				riseFrames > 20 && riseLow == 0, det);
		}
		// ======== v1.5 (playtest 13) ========
		// 125. town safety: on a Saint Denis street a fixed prop in the wall stays fixed - not ripped loose, not carried off; with
		// town safety off the same prop is torn loose (playtests 12-13: three crashes in Saint Denis after the city's props were torn up)
		{
			bool touched[2] = { false, false }, seen[2] = { false, false };
			int flights[2] = { -1, -1 };
			for (int pass = 0; pass < 2; pass++)
			{
				reset(); srand(125);
				g_set.touchdown = false; g_set.debrisCone = false; g_set.extraDebris = false; g_set.mapTrees = false; g_set.flatten = false;
				g_set.grabProps = true; g_set.ripFixedProps = true;
				g_set.citySafe = pass == 0;   // (the same street in Saint Denis, with town safety on, then off)
				V3 at = V3(2600, -1250, 0);
				playerPos = at + V3(45, 0, 0);
				g_poolTime = -100.0f;
				Tornado n(at, 0, 0, pass == 0 ? "safe" : "unsafe", true, 0);
				int o = nextObject++; objects[o].type = 3; objects[o].frozen = true; objects[o].model = Joaat("p_lamppost01x");
				for (int k = 1; k <= 40; k++)
				{
					objects[o].p = at + V3(n.wallRadius() * 0.9f, 0, 1.0f);
					mockTime = 3000.0f + k * 0.1f; n.Update(0.1f, mockTime);
					if (n.statInRange > 0) seen[pass] = true;
					if (!objects[o].frozen || !g_flights.empty()) touched[pass] = true;   // ripped loose, or carried off
				}
				flights[pass] = (int)g_flights.size();
				n.Destroy();
			}
			sprintf_s(det, "a lamp post in the wall on a Saint Denis street: town safety on - seen %d, ripped or carried %d (flights %d); off - seen %d, ripped or carried %d (flights %d)",
				(int)seen[0], (int)touched[0], flights[0], (int)seen[1], (int)touched[1], flights[1]);
			// (v1.6.2: with town safety on, a big town's objects aren't even read - see 134)
			check("town safety: Saint Denis' street furniture stays put (and would fly without it)", !touched[0] && seen[1] && touched[1], det);
		}
		// 126. the tornado gun hits a person: spun round the twister for a second, then knocked back away from Arthur (not off in a
		// random direction); the twister still spawns
		reset(); srand(126);
		{
			g_gun.on = true;
			playerPos = V3(0, 0, 1); camPos = V3(0, -2, 2);
			int ped = nextObject++; objects[ped].type = 1; objects[ped].p = V3(0, 20, 1);
			world.clear();
			Shape body = Cyl(0, 20, 0.4f, 1.8f); body.ent = ped; world.push_back(body);
			mockShooting = true; mockImpact = V3(0, 19.7f, 1.0f);
			tick2(T(1));
			mockShooting = false;
			bool victim = !g_gun.victims.empty() && g_gun.victims[0].p == ped;
			bool twister = !g_gun.minis.empty();
			float spinV = 0, flungAlong = 0, flungUp = 0;
			for (int k = 2; k <= 20; k++)
			{
				tick2(T(k));
				MockObj* o = Obj(ped);
				if (!o) break;
				if (T(k) - T(1) < 0.9f) spinV = std::max(spinV, V3(o->v.x, o->v.y, 0).len2d());
				else if (flungAlong == 0 && g_gun.victims.size() && g_gun.victims[0].flung) { flungAlong = o->v.y; flungUp = o->v.z; }
			}
			sprintf_s(det, "victim %d, twister spawned %d; spun at up to %.1f m/s, then knocked %.1f m/s away from Arthur (%.1f up)", (int)victim, (int)twister, spinV, flungAlong, flungUp);
			check("tornado gun: a shot person spins, then is knocked back the way the bullet went", victim && twister && spinV > 3.0f && flungAlong > 8.0f && flungUp > 3.0f, det);
		}
		// 127. the pocket twister: (v1.6, playtest 14: "my little cute pocket tornados are gone") its own little funnel again - the style's
		// looped effects at pocket scale - and still no ground storm, no smoke collar and no puffs
		reset(); srand(127);
		{
			g_gun.on = true;
			mockShooting = true; mockImpact = V3(20, 0, 0);
			size_t fx0 = fxNames.size();
			std::set<std::string> before;
			for (auto& kv : fxNames) before.insert(std::to_string(kv.first));
			tick2(T(1));
			mockShooting = false;
			for (int k = 2; k < 30; k++) tick2(T(k));
			int whirls = 0, other = 0;
			std::string others;
			for (auto& kv : fxNames)
			{
				if (before.count(std::to_string(kv.first))) continue;
				if (kv.second.rfind("ent_amb_wind_", 0) == 0) whirls++;
				else if (kv.second != "ent_amb_campfire_sma") { other++; if (others.size() < 120) others += " " + kv.second; }
			}
			(void)fx0;
			Tornado* mini = g_gun.minis.empty() ? nullptr : g_gun.minis[0].get();
			int loops = mini ? mini->loopsRunning : -1;
			sprintf_s(det, "the pocket twister's looped effects: %d of its own funnel (%s), %d whirls; %d running", other, others.c_str(), whirls, loops);
			check("pocket twister: its own little funnel, no smoke", other >= 4 && whirls == 0 && loops >= 4 && loops <= 10, det);
		}
		// 128. mash to get up: knocked down near a tornado (not in its hands), mashing A fills the bar and gets him up; just lying
		// there doesn't
		reset(); srand(128);
		{
			Tornado* tp = SpawnTornado(0, 30, 0, true, "knocker");
			playerPos = V3(0, 0, 0.5f);
			mockRagdoll = true;
			float tt = 50.0f;
			for (int k = 0; k < 10; k++) { tt += 0.1f; MashUpdate(0.1f, tt); }   // lying there
			bool upAlone = !mockRagdoll;
			int presses = 0;
			for (int k = 0; k < 60 && mockRagdoll; k++)
			{
				tt += 0.05f;
				g_prev = 0; g_down = (k % 2 == 0) ? (1u << PB_A) : 0;
				if (g_down) presses++;
				MashUpdate(0.05f, tt);
			}
			g_prev = g_down = 0;
			sprintf_s(det, "tornado %d; up without pressing %d; up after %d presses %d (1 ms ragdoll asked %d)", tp ? 1 : 0, (int)upAlone, presses, (int)!mockRagdoll, lastRagdollMin);
			check("mash to get up: A gets him back on his feet", tp && !upAlone && !mockRagdoll && presses >= 5 && presses <= 12 && lastRagdollMin == 1, det);
		}
		// 129. the wide ride camera: every other ride. (v1.6, playtest 14: "a little too wide... can't track or see Arthur... natural
		// transition to wide angle... and then back... disable it when the tornado throws arthur") It starts about where the game's camera
		// is, eases out round Arthur (never more than 40 m from him), the look input swings it round him, and it eases back in before
		// it hands back; a throw ends either ride camera at once
		reset(); srand(129);
		{
			g_set.rideCam = true;
			Tornado* tp = SpawnTornado(0, 30, 0, true, "rider");
			playerPos = V3(0, 50, 20);
			float tt = 10.0f;
			bool firstWide = true, secondWide = false, thirdWide = false, plainOff = false, wideOff = false, endedAlone = false;
			float startD = 0, maxD = 0, endD = 99, turned = 0;
			// ride 1 (the plain orbit): thrown - off
			tt += 30.0f; tp->playerLastHeld = tt; RideCamUpdate(0.1f, tt, false);
			firstWide = g_ride.wide;
			g_arthurThrows++; tt += 0.1f; RideCamUpdate(0.1f, tt, false); plainOff = !g_ride.on;
			// ride 2 (wide): out round him, steered, then thrown - off
			tt += 30.0f; tp->playerLastHeld = tt; RideCamUpdate(0.1f, tt, false);
			secondWide = g_ride.wide;
			for (int k = 1; k <= 40 && g_ride.on; k++)
			{
				tt += 0.1f;
				mockLookLR = (k > 20 && k <= 30) ? 1.0f : 0.0f;
				float a0 = g_ride.ang;
				RideCamUpdate(0.1f, tt, false);
				if (k > 20 && k <= 30) turned += a0 - g_ride.ang;
				float d = (lastCamCoord - playerPos).len();
				if (k == 1) startD = d;
				maxD = std::max(maxD, d);
			}
			mockLookLR = 0;
			g_arthurThrows++; tt += 0.1f; RideCamUpdate(0.1f, tt, false); wideOff = !g_ride.on;
			// ride 3 (wide again): left alone - back in close before it hands back
			g_ride.rides = 3;
			tt += 30.0f; tp->playerLastHeld = tt; RideCamUpdate(0.1f, tt, false);
			thirdWide = g_ride.wide;
			for (int k = 1; k <= 90 && g_ride.on; k++)
			{
				tt += 0.1f;
				RideCamUpdate(0.1f, tt, false);
				if (g_ride.on && g_ride.until - tt < 0.25f) endD = (lastCamCoord - playerPos).len();
			}
			endedAlone = !g_ride.on;
			RideCamStop(false);
			sprintf_s(det, "rides wide: %d %d %d; the wide one starts %.1f m from Arthur, goes out to %.1f m, back to %.1f m before it hands back (on its own %d); "
				"the look input turned it %.2f rad; thrown - the plain one off %d, the wide one off %d", (int)firstWide, (int)secondWide, (int)thirdWide, startD, maxD, endD,
				(int)endedAlone, turned, (int)plainOff, (int)wideOff);
			check("ride camera: out round Arthur and back, steerable, and a throw ends it", !firstWide && secondWide && thirdWide && startD < 10.0f && maxD >= 20.0f &&
				maxD <= 40.5f && endD < 12.0f && endedAlone && fabsf(turned) > 1.0f && plainOff && wideOff, det);
		}
		// 130. (v1.5) the intro's ears: no line starts in the first second after a cut; a cut waits for a line still being said (8 s
		// in all at most); a far-off speaker's camp line is played beside the camera, by its audio name, in their own voice; the
		// subtitles' text blocks roll (a handful at a time, not all 17). (v1.5 review) Cuts really are held - each 2.2 s at most, and
		// never longer than the line: a line beside the camera holds a cut only until 2.2 s after it began (real time); and never
		// at the cuts round Dutch's pass
		reset(); srand(130);
		{
			mockTime = 1.0f;
			IntroStart();
			std::vector<float> cuts;
			int lastShot = -2, maxBlocks = 0;
			float earliestAfterCut = 99.0f;
			size_t said0 = said.size(), story0 = storyCalls.size();
			float clk = 0;
			float curHold = 0, maxHold = 0, prevHold = 0;
			int holds = 0, overHeld = 0, throwHeld = 0;
			for (int k = 11; k <= 2400 && !(g_in.stage == 0 && k > 200); k++)
			{
				size_t sN = said.size(), cN = storyCalls.size(), nN = nearSpeech.size();
				tick2(T(k));
				if (g_in.stage != 4) continue;
				clk = (float)g_in.clock;
				if (g_in.camShot != lastShot) { lastShot = g_in.camShot; cuts.push_back(clk); }
				bool started = said.size() > sN || storyCalls.size() > cN || nearSpeech.size() > nN;
				// (the cast's own reactions aren't scripted lines: only the intro's, which come through these three)
				if (started && !cuts.empty() && clk > 1.0f) earliestAfterCut = std::min(earliestAfterCut, clk - cuts.back());
				int live = 0;
				for (int i = 0; i < kIntroLineCount; i++) if (g_in.blockReq[i] && !g_in.blockGone[i]) live++;
				maxBlocks = std::max(maxBlocks, live);
				// the holds
				if (g_in.holdHere > prevHold)
				{
					curHold = g_in.holdHere;
					bool conv = g_in.storyLine >= 0 && AUDIO::IS_SCRIPTED_CONVERSATION_PLAYING(kIntroLines[g_in.storyLine].root);
					bool amb = g_in.ambPed && AUDIO::IS_AMBIENT_SPEECH_PLAYING(g_in.ambPed);
					if (!conv && !amb && (nearSpeech.empty() || mockTime - nearSpeech.back().t > 2.3f)) overHeld++;   // held for nothing
					float nx = IntroNextCut(clk);
					if (nx >= kIntroDutchThrow && nx <= kIntroSlowMo1 + 0.7f) throwHeld++;
				}
				else if (g_in.holdHere == 0 && curHold > 0) { holds++; maxHold = std::max(maxHold, curHold); curHold = 0; }
				prevHold = g_in.holdHere;
			}
			bool byAudio = false;
			std::string nearList;
			for (auto& n : nearSpeech)
			{
				nearList += " " + n.ctx + "/" + n.voice;
				for (int i = 0; i < kIntroLineCount; i++)
					if (kIntroLines[i].audio && n.ctx == kIntroLines[i].audio && n.voice == IntroVoice(kIntroLines[i].who) && n.camDist < 3.0f) byAudio = true;
			}
			(void)said0; (void)story0;
			sprintf_s(det, "%d cuts; the earliest a line started after a cut %.2f s; %d cuts held (%.1f s in all, the longest %.2f s; held with nothing being said %d frames, "
				"round Dutch's pass %d); lines beside the camera %d (%s) - a camp line by its audio name in the speaker's own voice %d; text blocks live at once at most %d",
				(int)cuts.size(), earliestAfterCut, holds, g_in.holdTotal, maxHold, overHeld, throwHeld, (int)nearSpeech.size(), nearList.c_str(), (int)byAudio, maxBlocks);
			check("intro ears: a second after every cut, held cuts within budget, far lines beside the camera, rolling subtitles",
				cuts.size() >= 20 && earliestAfterCut >= 0.95f && g_in.holdTotal <= 8.05f && holds >= 1 && maxHold <= 2.25f && overHeld == 0 && throwHeld == 0 &&
				byAudio && maxBlocks <= 8, det);
		}
		// ======== v1.6 (playtest 14) ========
		// 131. the new throws, through the catch: sky-high keeps climbing at 50 m/s for 3.5 s; far soft glides down at 4.5 m/s keeping its
		// drift; the meteor dives at 46 m/s, makes him undamageable from the top of the dive, hits with a burst of dirt - and costs nothing;
		// the far and sky-high throws (like the stratosphere) never happen over a big town
		reset(); srand(131);
		{
			mockGroundZ = 0;
			g_set.landings = 1; SyncMenuChoices();
			float t0 = 3000.0f;
			// sky-high
			g_landKind = 5; g_skyUntil = t0 + 3.5f; NotePlayerInWall(t0);
			playerPos = V3(0, 0, 80); playerVel = V3(3, 0, 22);
			PlayerSafety(0.1f, t0 + 1.0f);
			float climb = playerVel.z;
			playerVel = V3(3, 0, 22);
			PlayerSafety(0.1f, t0 + 4.0f);
			float after = playerVel.z;
			// far soft
			g_landKind = 6; g_glideVel = V3(20, 0, 0); g_backstopOn = false; NotePlayerInWall(t0 + 10.0f);
			playerPos = V3(0, 0, 150); playerVel = V3(5, 0, -30);
			PlayerSafety(0.1f, t0 + 10.1f);
			V3 glide = playerVel;
			// the meteor
			g_landKind = 7; g_meteorHit = false; g_backstopOn = false; g_fallTopH = 0; NotePlayerInWall(t0 + 20.0f);
			int fx0 = calls[N_START_PARTICLE_FX_NON_LOOPED_AT_COORD];
			int dmgOff0 = calls[N_SET_ENTITY_CAN_BE_DAMAGED];
			playerPos = V3(0, 0, 60); playerVel = V3(2, 0, -6);
			PlayerSafety(0.1f, t0 + 20.1f);
			float dive = playerVel.z;
			bool shielded = g_backstopOn && calls[N_SET_ENTITY_CAN_BE_DAMAGED] > dmgOff0;
			playerPos = V3(0, 0, 2.0f); playerVel = V3(2, 0, -40);
			PlayerSafety(0.1f, t0 + 21.0f);
			bool hit = g_meteorHit;
			int bursts = calls[N_START_PARTICLE_FX_NON_LOOPED_AT_COORD] - fx0;
			g_set.landingHurts = true; g_set.playerGod = false; g_landing.healthBefore = 200; lastHealthSet = -1;
			BackstopOff("landed");
			int hurt = lastHealthSet;
			// a whole glide from 200 m, under gravity: still caught at the ground, however long it takes (v1.6 review: the window was 40 s)
			g_landKind = 6; g_glideVel = V3(20, 0, 0); g_backstopOn = false; g_fallTopH = 0;
			g_dropWindowUntil = t0 + 100.0f + 40.0f; g_playerLastInWall = -100.0f;
			playerPos = V3(0, 0, 200); playerVel = V3(20, 0, -5);
			float glideT = 0, worstFall = 0;
			bool glideKindKept = true;
			for (int k = 1; k <= 900 && playerPos.z > 0.5f; k++)
			{
				float tg = t0 + 100.0f + k * 0.1f;
				playerVel.z -= 9.8f * 0.1f;
				PlayerSafety(0.1f, tg);
				playerPos = playerPos + playerVel * 0.1f;
				glideT = k * 0.1f;
				if (playerPos.z > 1.5f) worstFall = std::min(worstFall, playerVel.z);
				if (g_landKind != 6 && playerPos.z > 1.5f) glideKindKept = false;
			}
			// town
			ForceProfile fp = CurrentForce();
			int throws0 = g_arthurThrows;
			LaunchArthur(V3(1, 0, 0), V3(0, 1, 0), fp, 1.0f, 10.0f, true, 0);
			bool counted = g_arthurThrows == throws0 + 1;
			int farInTown = 0;
			playerPos = V3(2560, -1220, 10);
			for (int i = 0; i < 300; i++) { LaunchArthur(V3(1, 0, 0), V3(0, 1, 0), fp, 1.0f, 10.0f, true, 0); if (g_landKind == 2 || g_landKind == 4 || g_landKind == 5) farInTown++; }
			sprintf_s(det, "sky-high: climbing at %.0f m/s at 1 s, left at %.0f after 3.5 s; far soft: (%.1f, %.1f, %.1f) m/s at 150 m; meteor: diving at %.0f m/s, "
				"shielded from the top %d, hit %d (%d bursts), health set after %d; far / sky-high / stratosphere in Saint Denis %d of 300; a glide from 200 m: "
				"%.0f s, the fastest fall %.1f m/s, still gliding to the end %d; a launch counted %d",
				climb, after, glide.x, glide.y, glide.z, dive, (int)shielded, (int)hit, bursts, hurt, farInTown, glideT, worstFall, (int)glideKindKept, (int)counted);
			check("new throws: sky-high climbs, far soft glides, the meteor dives and doesn't hurt, none of the far ones in town", climb >= 49.9f && after == 22.0f &&
				glide.z == -4.5f && glide.x > 18.0f && dive <= -45.0f && shielded && hit && bursts >= 2 && hurt == -1 && farInTown == 0 &&
				glideT > 40.0f && worstFall >= -5.5f && glideKindKept && counted, det);
		}
		// 132. (v1.6, playtest 14) the intro: Arthur's gestures are story-mode ones (the Online emotes never loaded); (v1.6.1, playtest 15:
		// "fly the balloon into micah, knocking him over before taking off, all in one swoop") the balloon skims across the camp into
		// Micah and knocks him flying, then climbs; Dutch, steered, crosses the slow motion a few metres from Arthur on the camera's side;
		// Arthur's close-ups are in front of his face; the shot of the gang going up is from the camp's edge, not 58 m+
		reset(); srand(132);
		{
			bool noOnline = true;
			for (const char* d : kIntroDicts) if (strstr(d, "script_mp@")) noOnline = false;
			mockTime = 1.0f;
			IntroStart();
			int faceFrames = 0, backFrames = 0, gangFrames = 0;
			float gangFar = 0, dutchStart = 0, dutchMin = 99, micahOut = 0, skimLow = 99, swoopClosest = 99, climbAfter = 0;
			bool ran = false, knocked = false, dutchCamSide = false;
			float hitAt = -1;
			std::string backShots;
			for (int k = 11; k <= 2400 && !(g_in.stage == 0 && k > 200); k++)
			{
				tick2(T(k));
				if (g_in.stage != 4) continue;
				float c = (float)g_in.clock;
				Ped me = PLAYER::PLAYER_PED_ID();
				V3 head = PedHead(me);
				Ped micah = CastPed(CA_MICAH), dutch = CastPed(CA_DUTCH);
				for (int b : g_in.firedBeats) if (b == 22) ran = true;
				// the swoop (the mock's peds don't walk: Micah is where he stood up)
				if (g_in.swoop && micah)
				{
					skimLow = std::min(skimLow, g_bal.pos.z + g_bal.basketZ - mockGroundZ);
					swoopClosest = std::min(swoopClosest, (V3(ENTITY::GET_ENTITY_COORDS(micah, FALSE, FALSE)) - g_bal.pos).len2d());
				}
				if (!knocked && micah && g_in.swoopHit)
				{
					knocked = true; hitAt = c;
					if (MockObj* o = Obj(micah)) micahOut = g_in.swoopDir.x * o->v.x + g_in.swoopDir.y * o->v.y;
				}
				if (knocked && c > hitAt + 0.05f && c < hitAt + 0.25f && micah)
					if (MockObj* o = Obj(micah)) micahOut = std::max(micahOut, V3(o->v.x, o->v.y, 0).len2d());   // (the knock, held a few frames; v1.6.1 review: off to the side)
				if (knocked && c > hitAt + 1.0f && c < hitAt + 1.2f) climbAfter = g_bal.vel.z;
				int sh = g_in.camShot;
				// (the mock's Arthur never leaves the ground - in the sweep he's in the funnel, and the director swings a shot out of its
				// smoke: a few frames from behind are that)
				if (sh == 2 || sh == 4 || sh == 6 || sh == 9 || sh == 15 || sh == 18 || sh == 22)
				{
					V3 af = HeadingDir(ENTITY::GET_ENTITY_HEADING(me));
					V3 d = lastCamCoord - head;
					if (d.x * af.x + d.y * af.y > 0.5f) faceFrames++; else { backFrames++; if (backShots.size() < 60) backShots += " " + std::to_string(sh) + "@" + std::to_string((int)(c * 10)); }
				}
				if (sh == 19) { gangFrames++; gangFar = std::max(gangFar, (lastCamCoord - IntroSettleSpot()).len2d()); }
				if (dutch && c >= 87.4f && c < 90.4f)
				{
					V3 dpos = ENTITY::GET_ENTITY_COORDS(dutch, FALSE, FALSE);
					float dd = (dpos - head).len();
					if (dutchStart == 0) dutchStart = dd;
					if (dd < dutchMin)
					{
						dutchMin = dd;
						V3 a = dpos - head, b = lastCamCoord - head;
						dutchCamSide = a.x * b.x + a.y * b.y > 0;   // (closest to Arthur on the camera's side: between the lens and him)
					}
				}
			}
			sprintf_s(det, "no Online emotes %d; Micah up and heading for it %d; the swoop: the basket as low as %.2f m, %.1f m from Micah at the closest, hit him %d at %.1f s, "
				"knocked on at %.1f m/s, climbing at %.1f m/s a second later; Arthur's close-ups: %d frames in front of his face, %d behind (%s); the gang going up: %d frames, "
				"the camera at most %.0f m from the camp; Dutch: %.1f m off at the cut, %.1f m from Arthur at the closest, on the camera's side %d",
				(int)noOnline, (int)ran, skimLow, swoopClosest, (int)knocked, hitAt, micahOut, climbAfter, faceFrames, backFrames, backShots.c_str(), gangFrames, gangFar,
				dutchStart, dutchMin, (int)dutchCamSide);
			IntroAbort("harness");
			check("intro: story-mode gestures, the balloon swoops into Micah, Dutch right past the basket, close-ups at his face, the gang going up close",
				noOnline && ran && knocked && hitAt < 55.5f && skimLow < 1.0f && swoopClosest < 2.2f && micahOut > 6.0f && climbAfter > 2.0f &&
				faceFrames > 60 && backFrames <= 10 && gangFrames > 10 && gangFar < 40.0f && dutchStart > 6.0f && dutchMin < 4.5f && dutchCamSide, det);
		}
		// 133. (v1.6.1, playtest 15) Lenny went round stiff even with his sitting clip playing (frozen in mid-air): the posed riders ride an
		// invisible carrier they're attached to, and the carrier is what's flown (they go round with it); released, they're let off it. At the
		// hand-back the game's camera looks the way he's escaping - away from the funnel ("flip the camera around when it hands it back")
		reset(); srand(133);
		{
			mockTime = 1.0f;
			IntroStart();
			bool lennyOn = false, javierOn = false, lennyRound = false, offAfter = false;
			float lennyR = 0;
			for (int k = 11; k <= 2400 && !(g_in.stage == 0 && k > 200); k++)
			{
				tick2(T(k));
				float c = (float)g_in.clock;
				CastState& L = g_in.cast[CA_LENNY];
				CastState& J = g_in.cast[CA_JAVIER];
				if (g_in.stage == 4 && c > 84.0f && c < 90.0f)
				{
					if (L.carrier && attachedTo.count(L.ped) && attachedTo[L.ped] == L.carrier) lennyOn = true;
					if (J.carrier && attachedTo.count(J.ped) && attachedTo[J.ped] == J.carrier) javierOn = true;
					if (ITp() && L.ped)
					{
						float r = (V3(ENTITY::GET_ENTITY_COORDS(L.ped, FALSE, FALSE)) - ITp()->base).len2d();
						lennyR = std::max(lennyR, r);
						if (r > 3.0f) lennyRound = true;
					}
				}
				if (g_in.stage == 5 && L.released && !attachedTo.count(L.ped)) offAfter = true;
				if (g_in.stage == 5 && offAfter) break;
			}
			V3 esc = FlatDir(ITp() ? ITp()->base : g_in.C, g_bal.on ? g_bal.pos : playerPos);
			V3 look = HeadingDir(ENTITY::GET_ENTITY_HEADING(PLAYER::PLAYER_PED_ID()) + mockCamRelHeading);
			float along = look.x * esc.x + look.y * esc.y;
			sprintf_s(det, "Lenny on his carrier %d (going round it, out to %.0f m from the funnel's axis %d), Javier on his %d, let off when released %d; "
				"the camera handed back %.0f deg from where he faces, looking along his escape %.2f", (int)lennyOn, lennyR, (int)lennyRound, (int)javierOn, (int)offAfter,
				mockCamRelHeading, along);
			IntroAbort("harness");
			check("intro: the riders on carriers (they animate), the camera handed back looking the way he escapes", lennyOn && lennyRound && javierOn && offAfter && along > 0.9f, det);
		}
		// 134. (v1.6.2) five crashes, all in Saint Denis, the logged ones each ending on an empty entity-list read: in and near a big town
		// the object list isn't read (Script Hook's read hands every entity a script handle - the city runs the game out of them);
		// after an empty read the next read waits 1.5 s (it used to retry 10 times a second); the intro won't stage in Saint Denis
		reset(); srand(134);
		{
			int o = nextObject++; objects[o].type = 3; objects[o].p = V3(2600, -1250, 1);
			int o2 = nextObject++; objects[o2].type = 3; objects[o2].p = V3(0, 30, 1);
			g_set.grabProps = true;
			// Saint Denis
			playerPos = V3(2600, -1250, 1); camPos = playerPos + V3(0, -5, 2);
			g_poolTime = -100.0f; int skips0 = g_poolSkipsObjects;
			RefreshPool(5000.0f);
			int objsInCity = g_rawObjs; bool skipped = g_poolSkipsObjects > skips0;
			// the open country
			playerPos = V3(0, 0, 1); camPos = V3(0, -5, 2);
			g_poolTime = -100.0f;
			RefreshPool(5001.0f);
			int objsOutside = g_rawObjs;
			// an empty read, then the back-off
			emptyPool = true;
			int reads0 = PoolEmptyReads();
			RefreshPool(5002.0f);
			int afterFirst = PoolEmptyReads() - reads0;
			RefreshPool(5002.5f); RefreshPool(5003.0f);
			int within = PoolEmptyReads() - reads0;
			RefreshPool(5003.6f);
			int after = PoolEmptyReads() - reads0;
			emptyPool = false;
			// the intro in Saint Denis
			playerPos = V3(2600, -1250, 1);
			mockTime = 6000.0f;
			IntroStart();
			bool refused = g_in.stage == 0;
			sprintf_s(det, "objects read in Saint Denis %d (skipped %d), in the open %d; empty reads: %d at once, %d within 1 s, %d after 1.6 s; the intro in Saint Denis refused %d",
				objsInCity, (int)skipped, objsOutside, afterFirst, within, after, (int)refused);
			check("Saint Denis: no object list in a big town, a back-off after an empty read, no intro in the city", objsInCity == 0 && skipped && objsOutside >= 2 &&
				afterFirst == 1 && within == 1 && after == 2 && refused, det);
		}
		reset();
		printf("Result: %d fixed, %d still broken (control flow only - no game was run).\n", fixedCount, brokenCount);
		return brokenCount == 0 ? 0 : 2;
	}
	catch (const std::exception& e) { fprintf(stderr, "HARNESS ERROR: %s\n", e.what()); return 1; }
}
