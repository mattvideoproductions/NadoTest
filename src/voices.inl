// ======================= voices (v1.2) =======================
// Arthur, the gang and passers-by react to the tornado out loud, with the game's own lines (research\cutscene_research.md §2,
// research\speech_reactions_research.md), strung into little exchanges:
//   a townsman screams for help as it takes him ... Arthur: "Goodbye."
//   Dutch flies past ... Arthur: "Dutch." ... Dutch: "Going badly!"
//   it drops Arthur ... Arthur: "Thanks for the lift."
// Every line is checked with DOES_CONTEXT_EXIST_FOR_THIS_PED first and falls back to the next one in its list, and every
// attempt is logged ("VOICE ..."), so a playtest shows which ones the game plays.

enum VoRole { VR_ARTHUR, VR_SUBJECT, VR_BYSTANDER, VR_GANG, VR_ROLES };
enum VoEvent
{
	VE_SIGHTED, VE_TOUCHDOWN, VE_CLOSE, VE_NPC_LIFTED, VE_GANG_LIFTED, VE_HORSE_LIFTED, VE_ARTHUR_LIFTED, VE_ARTHUR_RIDING,
	VE_ARTHUR_THROWN, VE_ARTHUR_LANDED, VE_NEAR_MISS, VE_DIES_DOWN, VE_GUN_HIT, VE_BLAMED, VE_BALLOON_BOOST, VE_BALLOON_WATCH, VE_COUNT
};
static const char* kVoEventNames[VE_COUNT] = {
	"sighted", "touchdown", "close", "npc lifted", "gang lifted", "horse lifted", "arthur lifted", "arthur riding",
	"arthur thrown", "arthur landed", "near miss", "dies down", "gun hit", "blamed", "balloon boost", "balloon watch"
};
// how soon the same event may start another exchange (s, at Chatter Normal), and how much it matters (a higher one cuts in)
static const float kVoEventGap[VE_COUNT] = { 60, 45, 40, 7, 6, 30, 10, 999, 12, 10, 25, 60, 9, 40, 30, 50 };
static const int kVoEventPrio[VE_COUNT] = { 1, 2, 2, 3, 3, 3, 5, 4, 4, 4, 2, 1, 3, 2, 1, 1 };
static const bool kVoArthurFirst[VE_COUNT] = { true, false, true, false, false, true, true, true, true, true, true, true, false, false, true, true };

// A step: who speaks, the lines to try (the first that speaker's voice has), how long after the previous line ends, shouted or not.
//   "GREET_*"  - greet this gang member by name (Arthur has GREET_DUTCH, GREET_MICAH ...)
//   "~NAME"    - a vocal sound (gasp, laugh, fall), played without the existence check (those live in the vocal banks)
// Lines and who has them: research\speech_reactions_research.md §9 (the reaction table) and §2-3 (coverage).
struct VoStep { int role; const char* ctx[6]; float gap; bool shout; };
struct VoExchange { int ev; int weight; int n; VoStep s[3]; };
#define VA VR_ARTHUR
#define VS VR_SUBJECT
#define VB VR_BYSTANDER
#define VG VR_GANG
static const VoExchange kVoExchanges[] = {
	// ---- it shows up far off
	{ VE_SIGHTED, 3, 1, { { VA, { "GENERIC_SHOCKED_MED", "RESSH_PRE_ATTACK_WHAT_A", "RE_BOT_UNI_V0_WTH", "GENERIC_SHOCKED_HIGH" }, 0, false } } },
	{ VE_SIGHTED, 3, 2, { { VB, { "CHAT_BAD_WEATHER", "GENERIC_SHOCKING_EVENT_COMMENT", "COME_SEE_THIS", "GENERIC_SHOCKED_HIGH" }, 0, false },
	                      { VA, { "GREET_THIRD_BAD_WEATHER_CONV", "GENERIC_SHOCKED_MED" }, 0.4f, false } } },   // the deadpan weather remark
	{ VE_SIGHTED, 2, 2, { { VG, { "GENERIC_SHOCKED_DISBELIEF", "GENERIC_SHOCKED_MED" }, 0, false },
	                      { VA, { "GENERIC_SHOCKED_MED", "GENERIC_CURSE_MED" }, 0.4f, false } } },
	// ---- it touches down
	{ VE_TOUCHDOWN, 3, 2, { { VB, { "SCREAM_SHOCKED", "GENERIC_SHOCKED_HIGH", "PANIC_COMMUNICATE", "GENERIC_FRIGHTENED_HIGH" }, 0, true },
	                        { VA, { "GENERIC_CURSE_HIGH", "RE_BOT_UNI_V0_SHIT", "GENERIC_SHOCKED_HIGH" }, 0.3f, false } } },
	{ VE_TOUCHDOWN, 2, 1, { { VA, { "GENERIC_SHOCKED_HIGH", "RE_BOT_UNI_V0_SHIT", "GENERIC_CURSE_HIGH" }, 0, false } } },
	{ VE_TOUCHDOWN, 2, 2, { { VG, { "WHOA_ESCALATED", "GENERIC_SHOCKED_HIGH", "GENERIC_CURSE_HIGH" }, 0, true },
	                        { VA, { "PLAYER_RESPONSE_TO_ALLY_NEAR", "GENERIC_CURSE_HIGH" }, 0.3f, false } } },
	// ---- it's coming
	{ VE_CLOSE, 3, 2, { { VB, { "PANIC_HELP", "SCARED_HELP", "IM_OUTTA_HERE", "GENERIC_FRIGHTENED_HIGH" }, 0, true },
	                    { VA, { "PLAYER_GOADS_FLEE", "GENERIC_CURSE_HIGH" }, 0.3f, true } } },
	{ VE_CLOSE, 2, 1, { { VA, { "PLAYER_TAUNT_ENEMY", "PLAYER_TAUNT_SINGLE_ENEMY_POS_NEAR", "PLAYER_UNDER_FIRE_NEAR", "GENERIC_CURSE_HIGH" }, 0, true } } },   // he squares up to the weather
	{ VE_CLOSE, 2, 2, { { VG, { "MOVE_IT_NEAR", "GENERIC_FRIGHTENED_HIGH", "KEEP_UP" }, 0, true },
	                    { VA, { "SPURS_HORSE_URGENT", "PLAYER_UNDER_FIRE_NEAR", "GENERIC_CURSE_HIGH" }, 0.3f, true } } },
	// ---- it takes someone (the victim's scream has already played - the first line answers it)
	{ VE_NPC_LIFTED, 4, 1, { { VA, { "GENERIC_GOODBYE", "GREET_THIRD_FAREWELL_GENERAL_CONV" }, 0.3f, false } } },
	{ VE_NPC_LIFTED, 2, 1, { { VA, { "GREET_THIRD_SORRY_TO_HEAR_THAT_CONV", "GENERIC_GOODBYE" }, 0.3f, false } } },
	{ VE_NPC_LIFTED, 2, 2, { { VB, { "WAS_THIS_YOU", "SICK_BASTARD", "SHAME_ON_YOU", "GENERIC_SHOCKED_HIGH" }, 0.3f, true },
	                         { VA, { "PLAYER_REACTION_CAUGHT_OUT", "DEFUSE_ARGUMENT_TOUGH", "GENERIC_GOODBYE" }, 0.3f, false } } },   // they've worked out who brings the weather
	{ VE_NPC_LIFTED, 1, 1, { { VA, { "~LAUGH_LOW", "GENERIC_SHOCKED_MED" }, 0.4f, false } } },
	// ---- it takes one of the gang: small talk as they go round
	{ VE_GANG_LIFTED, 4, 3, { { VA, { "GREET_*", "GREET_GENERAL" }, 0.4f, false },
	                          { VS, { "GOING_BADLY", "GENERIC_ANGRY_REACTION", "GENERIC_CURSE_HIGH" }, 0.2f, true },
	                          { VA, { "CAMP_GREET_THIRD_BADLY_RESPONSE", "GREET_THIRD_SORRY_TO_HEAR_THAT_CONV", "GENERIC_GOODBYE" }, 0.2f, false } } },
	{ VE_GANG_LIFTED, 2, 2, { { VS, { "WHATS_YOUR_PROBLEM", "CUT_THAT_OUT", "GENERIC_ANGRY_REACTION" }, 0.3f, true },
	                          { VA, { "GENERIC_GOODBYE" }, 0.3f, false } } },
	{ VE_GANG_LIFTED, 1, 2, { { VS, { "GENERIC_INSULT_HIGH", "GENERIC_CURSE_HIGH" }, 0.3f, true },
	                          { VA, { "~LAUGH_HIGH", "~LAUGH_LOW" }, 0.3f, false } } },
	// ---- it takes his horse
	{ VE_HORSE_LIFTED, 2, 2, { { VA, { "HORSE_CALLOUT_ON_FLEE", "GENERIC_CURSE_HIGH" }, 0.2f, true },
	                           { VG, { "PLAYER_LOSES_HORSE", "GENERIC_SHOCKED_HIGH" }, 0.3f, false } } },
	{ VE_HORSE_LIFTED, 2, 2, { { VA, { "CALLING_HORSE", "HORSE_CALLOUT_ON_FLEE" }, 1.5f, true },
	                           { VA, { "HORSE_NO_SHOW", "GENERIC_CURSE_MED" }, 2.5f, false } } },   // he whistles; nothing comes
	{ VE_HORSE_LIFTED, 1, 1, { { VA, { "GREET_THIRD_FAREWELL_HORSE_CONV", "GENERIC_GOODBYE" }, 0.6f, false } } },
	// ---- it takes Arthur
	{ VE_ARTHUR_LIFTED, 2, 1, { { VA, { "~GASP_SURPRISED", "RE_BOT_UNI_V0_WTH", "GENERIC_CURSE_HIGH" }, 0, true } } },
	{ VE_ARTHUR_LIFTED, 2, 1, { { VA, { "GENERIC_CURSE_HIGH", "RE_BOT_UNI_V0_SHIT", "GENERIC_SHOCKED_HIGH" }, 0, true } } },
	{ VE_ARTHUR_LIFTED, 1, 2, { { VA, { "~GASP_SURPRISED", "GENERIC_SHOCKED_HIGH" }, 0, true },
	                            { VG, { "WHOA_ESCALATED", "GENERIC_SHOCKED_HIGH" }, 0.2f, true } } },
	// ---- ...and he settles in for the ride
	{ VE_ARTHUR_RIDING, 3, 1, { { VA, { "PLAYER_DRUNK_MERRY_SINGING", "~DRUNK_MERRY_VOFX", "~LAUGH_HIGH" }, 0, false } } },
	{ VE_ARTHUR_RIDING, 1, 1, { { VA, { "~LAUGH_HIGH", "~LAUGH_LOW" }, 0, false } } },
	{ VE_ARTHUR_RIDING, 2, 2, { { VA, { "GREET_WHISTLE_FAR", "GREET_GROUP_SHOUTED" }, 0, true },
	                            { VB, { "PLAYER_ACTING_WEIRD", "GENERIC_SHOCKED_DISBELIEF", "WHATS_YOUR_PROBLEM" }, 0.3f, true } } },   // waves to folks below
	{ VE_ARTHUR_THROWN, 2, 1, { { VA, { "~HIGH_FALL", "GENERIC_CURSE_HIGH" }, 0, true } } },
	{ VE_ARTHUR_THROWN, 1, 2, { { VA, { "~HIGH_FALL", "GENERIC_CURSE_HIGH" }, 0, true },
	                            { VG, { "PLAYER_THROWN", "GENERIC_SHOCKED_HIGH" }, 0.3f, true } } },
	// ---- ...and lands on his feet
	{ VE_ARTHUR_LANDED, 3, 1, { { VA, { "RIDER_THANK_FOR_LIFT", "GET_UP_FROM_FALL", "~LAUGH_LOW" }, 0.8f, false } } },
	{ VE_ARTHUR_LANDED, 3, 1, { { VA, { "GET_UP_FROM_FALL", "CALM_HORSE_BUCKED_OFF_GET_UP", "RIDER_THANK_FOR_LIFT" }, 0.8f, false } } },
	{ VE_ARTHUR_LANDED, 2, 2, { { VG, { "PLAYER_THROWN", "RELIEVED_REACT", "PLAYER_ACTING_WEIRD" }, 0.8f, false },
	                            { VA, { "GET_UP_FROM_FALL", "RIDER_THANK_FOR_LIFT" }, 0.3f, false } } },
	{ VE_ARTHUR_LANDED, 2, 2, { { VB, { "GREET_MUDDY", "GENERIC_SHOCKED_DISBELIEF", "WHATS_YOUR_PROBLEM" }, 0.8f, false },
	                            { VA, { "GREET_THIRD_GOOD_WEATHER_CONV", "RIDER_THANK_FOR_LIFT", "GREET_GENERAL" }, 0.3f, false } } },   // "lovely weather"
	// ---- near miss
	{ VE_NEAR_MISS, 2, 1, { { VA, { "PLAYER_NEAR_MISS", "~PAIN_NEAR_MISS", "GENERIC_CURSE_MED" }, 0, false } } },
	{ VE_NEAR_MISS, 1, 2, { { VA, { "~PAIN_NEAR_MISS", "PLAYER_NEAR_MISS" }, 0, false },
	                        { VA, { "PLAYER_TAUNT_SINGLE_ENEMY_POS_NEAR", "PLAYER_TAUNT_ENEMY" }, 0.6f, true } } },   // and then gets brave
	{ VE_NEAR_MISS, 1, 2, { { VG, { "ALLY_COMMENT_PLAYER_NEAR_MISS", "WHOA" }, 0, false },
	                        { VA, { "PLAYER_RESPONSE_TO_ALLY_NEAR", "PLAYER_NEAR_MISS" }, 0.3f, false } } },
	// ---- it goes back up
	{ VE_DIES_DOWN, 2, 1, { { VA, { "WON_FIGHT", "PLAYER_TAUNT_ENEMY", "GENERIC_GOODBYE" }, 0, true } } },   // brave once it's leaving
	{ VE_DIES_DOWN, 2, 1, { { VA, { "GREET_THIRD_GOOD_WEATHER_CONV", "GENERIC_GOODBYE" }, 0, false } } },
	{ VE_DIES_DOWN, 2, 2, { { VB, { "VICTIMIZED_REACT", "RELIEVED_REACT", "GENERIC_THANKS" }, 0, false },
	                        { VA, { "GREET_THIRD_SORRY_TO_HEAR_THAT_CONV", "GENERIC_GOODBYE" }, 0.4f, false } } },
	{ VE_DIES_DOWN, 1, 2, { { VB, { "CHAT_GOOD_WEATHER", "GENERIC_CHEER" }, 0, false },
	                        { VA, { "GREET_THIRD_GOOD_WEATHER_CONV", "GENERIC_GOODBYE" }, 0.4f, false } } },
	{ VE_DIES_DOWN, 2, 3, { { VA, { "CAMP_GREET_SECOND_HOWS_IT_GOING", "GREET_GENERAL" }, 0, false },
	                        { VG, { "GOING_BADLY", "GENERIC_ANGRY_REACTION" }, 0.3f, false },
	                        { VA, { "CAMP_GREET_THIRD_BADLY_RESPONSE", "GREET_THIRD_SORRY_TO_HEAR_THAT_CONV" }, 0.3f, false } } },
	// ---- the tornado gun goes off next to someone
	{ VE_GUN_HIT, 3, 2, { { VS, { "CUT_THAT_OUT", "GET_AWAY_FROM_ME", "WHATS_YOUR_PROBLEM", "GENERIC_CURSE_HIGH" }, 0.3f, true },
	                      { VA, { "~LAUGH_HIGH", "PLAYER_TAUNT_SINGLE_ENEMY_POS_NEAR" }, 0.2f, false } } },
	{ VE_GUN_HIT, 2, 2, { { VS, { "WRONGED_BY_PLAYER", "EXPLAIN_YOURSELF", "PLAYER_SHOOTS_PED", "GENERIC_ANGRY_REACTION" }, 0.3f, true },
	                      { VA, { "PLAYER_REACTION_CAUGHT_OUT", "DEFUSE_ARGUMENT_TOUGH" }, 0.3f, false } } },
	{ VE_GUN_HIT, 1, 2, { { VB, { "GET_THE_LAW", "SICK_BASTARD", "GENERIC_SHOCKED_HIGH" }, 0.4f, true },
	                      { VA, { "TELLS_PED_TO_SHUT_UP", "~LAUGH_LOW" }, 0.3f, false } } },
	// ---- folks have worked out who brings the weather
	{ VE_BLAMED, 3, 2, { { VS, { "WAS_THIS_YOU", "PLAYER_ACTING_WEIRD", "EXPLAIN_YOURSELF", "SICK_BASTARD", "GET_AWAY_FROM_ME" }, 0, false },
	                     { VA, { "PLAYER_REACTION_CAUGHT_OUT", "DEFUSE_ARGUMENT", "DEFUSE_ARGUMENT_TOUGH" }, 0.3f, false } } },
	{ VE_BLAMED, 1, 2, { { VS, { "GENERIC_SHOCKED_DISBELIEF", "PLAYER_LOOKING_WEIRD", "WHATS_YOUR_PROBLEM" }, 0, false },
	                     { VA, { "GREET_GENERAL", "PLAYER_ACKNOWLEDGEMENT" }, 0.3f, false } } },
	// ---- the jet balloon: he spurs it like a horse; shouts down at folks
	{ VE_BALLOON_BOOST, 1, 1, { { VA, { "SPURS_HORSE", "SPURS_HORSE_URGENT", "~LAUGH_HIGH" }, 0, true } } },
	{ VE_BALLOON_WATCH, 2, 1, { { VA, { "~LAUGH_HIGH", "PLAYER_TAUNT_MULTIPLE_ENEMIES_POS_FAR", "GENERIC_SHOCKED_MED" }, 0, true } } },
	{ VE_BALLOON_WATCH, 1, 1, { { VA, { "PLAYER_DRUNK_MERRY_SINGING", "~LAUGH_HIGH" }, 0, false } } },
	{ VE_BALLOON_WATCH, 2, 2, { { VA, { "GREET_GROUP_SHOUTED", "GREET_WHISTLE_FAR" }, 0, true },
	                            { VB, { "GREET_SHOUTED", "COME_SEE_THIS", "GENERIC_EXCITING_EVENT_COMMENT", "GENERIC_SHOCKED_DISBELIEF" }, 0.3f, true } } },
};
#undef VA
#undef VS
#undef VB
#undef VG
static const int kVoExchangeCount = sizeof(kVoExchanges) / sizeof(kVoExchanges[0]);

// The gang, by model, with the line Arthur greets them by (research: speech_reactions_research.md §gang models)
struct VoGang { const char* model; const char* greet; };
static const VoGang kVoGang[] = {
	{ "cs_dutch", "GREET_DUTCH" }, { "cs_micahbell", "GREET_MICAH" }, { "cs_billwilliamson", "GREET_BILL" },
	{ "cs_javierescuella", "GREET_JAVIER" }, { "cs_uncle", "GREET_UNCLE" }, { "cs_mrpearson", "GREET_PEARSON" },
	{ "cs_leostrauss", "GREET_STRAUSS" }, { "cs_hoseamatthews", "GREET_HOSEA" }, { "cs_lenny", "GREET_LENNY" },
	{ "cs_charlessmith", "GREET_CHARLES" }, { "cs_sean", "GREET_SEAN" }, { "cs_kieran", "GREET_KIERAN" },
	{ "cs_johnmarston", "GREET_JOHN" }, { "cs_mrsadler", "GREET_SADIE" }, { "cs_karen", "GREET_KAREN" },
	{ "cs_abigailroberts", "GREET_ABIGAIL" }, { "cs_susangrimshaw", "GREET_GRIMSHAW" }, { "cs_mollyoshea", "GREET_MOLLY" },
	{ "cs_tilly", "GREET_TILLY" }, { "cs_revswanson", "GREET_SWANSON" }, { "cs_josiahtrelawny", "GREET_TRELAWNY" },
	{ "cs_jackmarston", "GREET_JACK" }, { "cs_marybeth", "GREET_MARYBETH" },
};
static const int kVoGangCount = sizeof(kVoGang) / sizeof(kVoGang[0]);
static Hash g_voGangHash[kVoGangCount] = {};
static int VoGangIndex(Ped p)
{
	if (!g_voGangHash[0]) for (int i = 0; i < kVoGangCount; i++) g_voGangHash[i] = Joaat(kVoGang[i].model);
	Hash m = ENTITY::GET_ENTITY_MODEL(p);
	for (int i = 0; i < kVoGangCount; i++) if (g_voGangHash[i] == m) return i;
	return -1;
}


struct VoPed { bool up = false; float upAt = 0, downFor = 0, nextScream = 0, lastSaid = -100; };
struct VoTornado { int uid = 0; bool sighted = false, touchdown = false, close = false, diesDown = false, gun = false, nearArmed = false; float nearArmedAt = 0; int trees = 0; };
// v1.2 the storm report (IDEAS.md #7): what one storm did, posted when the last tornado is gone
struct StormStats { bool on = false; float start = 0; int taken = 0, gangTaken = 0, trees = 0, flings = 0, rides = 0, lines = 0; float rideSince = -1, longestRide = 0, highest = 0; };
static StormStats g_stats;
struct VoiceState
{
	// the exchange that's playing
	const VoExchange* x = nullptr;
	int ev = -1, step = 0;
	Ped who[VR_ROLES] = {};     // by role
	float lineAt = 0;           // when the last line started
	int gang = -1;              // the subject's gang index (for GREET_*)
	float at = 0;               // the next step's time (once the previous speaker is quiet)
	Ped waitOn = 0;
	float waitSince = 0;
	// pacing
	float evNext[VE_COUNT] = {};
	float arthurNext = 0;       // Arthur doesn't start another exchange before this
	float screamNext = 0, liftScreamNext = 0;
	// what it's watched
	std::unordered_map<int, VoPed> peds;
	std::vector<VoTornado> tors;
	float nextScan = 0;
	bool held = false, sang = false;
	float heldSince = -1, lastHeldAt = -100, lastNearAt = -100, lastLandAt = -100;
	int flings = 0, landings = 0;
	float horseUpAt = -100;
	bool horseUp = false;
	int played = 0, refused = 0, noLine = 0, arthurLines = 0;
	std::string lastArthur[3];
} g_vo;

static float VoGapMul() { return g_chatter == 0 ? 2.0f : g_chatter == 2 ? 0.5f : 1.0f; }

static bool VoUsable(Ped p, float maxDist = 70.0f)
{
	if (!p || !ENTITY::DOES_ENTITY_EXIST(p) || PED::IS_PED_DEAD_OR_DYING(p, TRUE)) return false;
	if (p == PLAYER::PLAYER_PED_ID()) return true;
	return (V3(ENTITY::GET_ENTITY_COORDS(p, TRUE, FALSE)) - PlayerPos()).len() < maxDist;
}

// A human near Arthur who's on the ground and not the subject (nearest first): a passer-by, or one of the gang.
static Ped VoNearest(Ped notThis, float within, bool gang)
{
	Ped me = PLAYER::PLAYER_PED_ID();
	int arr[256];
	int n = worldGetAllPeds(arr, 256);
	V3 pp = PlayerPos();
	Ped best = 0;
	float bd = within;
	for (int i = 0; i < n; i++)
	{
		Ped p = arr[i];
		if (p == me || p == notThis || !ENTITY::DOES_ENTITY_EXIST(p) || !PED::IS_PED_HUMAN(p) || PED::IS_PED_DEAD_OR_DYING(p, TRUE) || IsScripted(p)) continue;
		if (ENTITY::GET_ENTITY_HEIGHT_ABOVE_GROUND(p) > 2.0f) continue;
		if ((VoGangIndex(p) >= 0) != gang) continue;
		float d = (V3(ENTITY::GET_ENTITY_COORDS(p, TRUE, FALSE)) - pp).len();
		if (d < bd) { bd = d; best = p; }
	}
	return best;
}
static Ped VoBystander(Ped notThis, float within = 35.0f) { return VoNearest(notThis, within, false); }

static void VoStop(const char* why)
{
	if (g_vo.x) Log("VOICE exchange '%s' stopped (%s)", kVoEventNames[g_vo.ev], why);
	g_vo.x = nullptr; g_vo.ev = -1; g_vo.waitOn = 0;
}

// Picks a weighted variant for the event whose speakers are all here, and starts it.
static bool VoTrigger(int ev, Ped subject, float t, Ped waitFirst = 0)
{
	if (g_voices == 0 || t < g_vo.evNext[ev]) return false;
	if (kVoArthurFirst[ev] && t < g_vo.arthurNext && kVoEventPrio[ev] < 4) return false;
	if (g_vo.x && kVoEventPrio[ev] <= kVoEventPrio[g_vo.ev]) return false;   // something as important is playing
	Ped by = 0, gang = 0;
	bool byTried = false, gangTried = false;
	int total = 0;
	const VoExchange* pick[16];
	int weights[16], np = 0;
	for (int i = 0; i < kVoExchangeCount && np < 16; i++)
	{
		const VoExchange& x = kVoExchanges[i];
		if (x.ev != ev) continue;
		bool ok = true;
		for (int s = 0; s < x.n && ok; s++)
		{
			if (x.s[s].role == VR_ARTHUR) continue;
			if (g_voices < 2) { ok = false; break; }   // Arthur only
			if (x.s[s].role == VR_SUBJECT && !VoUsable(subject, 90.0f)) ok = false;
			if (x.s[s].role == VR_BYSTANDER) { if (!byTried) { by = VoBystander(subject); byTried = true; } if (!by) ok = false; }
			if (x.s[s].role == VR_GANG) { if (!gangTried) { gang = VoNearest(subject, 45.0f, true); gangTried = true; } if (!gang) ok = false; }
		}
		if (!ok) continue;
		pick[np] = &x; weights[np] = x.weight; total += x.weight; np++;
	}
	if (!np) return false;
	int r = rand() % total, k = 0;
	while (k < np - 1 && r >= weights[k]) { r -= weights[k]; k++; }
	if (g_vo.x) VoStop("cut in");
	g_vo.x = pick[k];
	g_vo.ev = ev;
	g_vo.step = 0;
	g_vo.who[VR_ARTHUR] = PLAYER::PLAYER_PED_ID();
	g_vo.who[VR_SUBJECT] = subject;
	g_vo.who[VR_BYSTANDER] = by;
	g_vo.who[VR_GANG] = gang;
	g_vo.gang = subject ? VoGangIndex(subject) : -1;
	g_vo.at = t + g_vo.x->s[0].gap;
	g_vo.waitOn = waitFirst;   // e.g. the subject's scream: the first line comes once it's over
	g_vo.waitSince = t;
	g_vo.evNext[ev] = t + kVoEventGap[ev] * VoGapMul();
	Log("VOICE exchange '%s' (%d line%s)%s", kVoEventNames[ev], g_vo.x->n, g_vo.x->n > 1 ? "s" : "",
		g_vo.gang >= 0 ? (std::string(" with ") + kVoGang[g_vo.gang].model).c_str() : "");
	return true;
}

// Says the first line in the list this ped has. Arthur doesn't say the same thing twice running if he has another.
static bool VoSay(Ped p, const char* const* ctx, bool shout, const char* tag)
{
	bool isArthur = p == PLAYER::PLAYER_PED_ID();
	std::string have[6];
	int nHave = 0;
	for (int i = 0; i < 6 && ctx[i]; i++)
	{
		std::string c = ctx[i];
		if (c == "GREET_*")
		{
			if (g_vo.gang < 0) continue;
			c = kVoGang[g_vo.gang].greet;
		}
		if (c[0] == '~') { have[nHave++] = c; continue; }   // a vocal: no existence check (they're in the vocal banks)
		if (AUDIO::DOES_CONTEXT_EXIST_FOR_THIS_PED(p, c.c_str(), FALSE)) have[nHave++] = c;
	}
	if (!nHave)
	{
		g_vo.noLine++;
		Log("VOICE %s: none of its lines exist for this voice (%s...)", tag, ctx[0]);
		return false;
	}
	auto bare = [](const std::string& x) { return x[0] == '~' ? x.substr(1) : x; };
	std::string chosen = have[0];
	if (isArthur && nHave > 1 && bare(chosen) == g_vo.lastArthur[0]) chosen = have[1];
	chosen = bare(chosen);
	// Rockstar's own params for scripted lines (BEAT_..._CLEAR, with _SUB for the game's subtitle); FORCE if that's refused
	const char* params = shout ? (g_voiceSubs ? "SPEECH_PARAMS_BEAT_SHOUTED_CLEAR_SUB" : "SPEECH_PARAMS_BEAT_SHOUTED_CLEAR")
		: (g_voiceSubs ? "SPEECH_PARAMS_BEAT_SPOKEN_CLEAR_SUB" : "SPEECH_PARAMS_BEAT_SPOKEN_CLEAR");
	bool ok = Speak(p, chosen.c_str(), params);
	if (!ok) ok = Speak(p, chosen.c_str(), shout ? "SPEECH_PARAMS_FORCE_SHOUTED" : "SPEECH_PARAMS_FORCE");
	(ok ? g_vo.played : g_vo.refused)++;
	Log("VOICE %s says %s -> %s", tag, chosen.c_str(), ok ? "played" : "refused");
	if (ok && g_stats.on) g_stats.lines++;
	if (isArthur && ok)
	{
		g_vo.arthurLines++;
		g_vo.lastArthur[2] = g_vo.lastArthur[1]; g_vo.lastArthur[1] = g_vo.lastArthur[0]; g_vo.lastArthur[0] = chosen;
	}
	return ok;
}

// A flyer's scream (not part of any exchange; never more than one a second).
static void VoScream(Ped p, float t)
{
	if (g_voices < 2 || t < g_vo.screamNext) return;
	// lawmen and outlaws have no panic lines - they curse and fall back instead (research §3.2)
	static const char* kScreams[6] = { "SCARED_HELP", "PANIC_HELP", "GENERIC_FRIGHTENED_HIGH", "COMBAT_FLEE", "GENERIC_CURSE_HIGH", "~SCREAM_TERROR" };
	int saveGang = g_vo.gang;
	if (VoSay(p, kScreams, true, "flyer"))
		g_vo.screamNext = t + RandRange(1.6f, 2.6f);   // v1.3 (playtest 11: 292 flyer lines in 10 minutes - a wall of screaming)
	g_vo.gang = saveGang;
}
static bool VoScreamNow(Ped p, float t)   // the moment it takes someone: heard, then the exchange answers it
{
	if (g_voices < 2) return false;
	if (t < g_vo.liftScreamNext) return false;   // v1.3: at most one lift scream every 0.8 s, however many it takes at once
	g_vo.liftScreamNext = t + 0.8f;
	float keep = g_vo.screamNext;
	g_vo.screamNext = 0;
	VoScream(p, t);
	bool said = g_vo.screamNext > t;
	if (!said) g_vo.screamNext = keep;
	return said;
}

static void VoAdvance(float t)
{
	if (!g_vo.x) return;
	if (g_vo.waitOn)
	{
		// the speech starts a moment after the call, so give it a beat before trusting "not playing"
		bool talking = ENTITY::DOES_ENTITY_EXIST(g_vo.waitOn) && (AUDIO::IS_AMBIENT_SPEECH_PLAYING(g_vo.waitOn) || AUDIO::IS_ANY_SPEECH_PLAYING(g_vo.waitOn));
		// Rockstar's chains wait for quiet AND 2.5 s since the line began; a little snappier here for the comic timing
		if ((talking || t - g_vo.waitSince < 1.2f) && t - g_vo.waitSince < 6.0f) return;
		g_vo.waitOn = 0;
		if (g_vo.step >= g_vo.x->n) { g_vo.x = nullptr; g_vo.ev = -1; return; }
		g_vo.at = t + g_vo.x->s[g_vo.step].gap;   // the next line, a beat after this one ends
	}
	if (t < g_vo.at) return;
	const VoStep& s = g_vo.x->s[g_vo.step];
	Ped p = g_vo.who[s.role];
	char tag[64];
	static const char* kRoleNames[VR_ROLES] = { "Arthur", "subject", "bystander", "gang" };
	sprintf_s(tag, "%s (%s, line %d)", kRoleNames[s.role], kVoEventNames[g_vo.ev], g_vo.step + 1);
	g_vo.step++;
	if (!VoUsable(p, 90.0f))
	{
		Log("VOICE %s: speaker gone", tag);
		if (s.role == VR_ARTHUR || g_vo.step >= g_vo.x->n) { g_vo.x = nullptr; g_vo.ev = -1; }
		else g_vo.at = t;
		return;
	}
	bool ok = VoSay(p, s.ctx, s.shout, tag);
	if (s.role == VR_ARTHUR && ok) g_vo.arthurNext = t + RandRange(8.0f, 12.0f) * VoGapMul();
	if (s.role != VR_ARTHUR && ok) g_vo.peds[p].lastSaid = t;
	g_vo.waitOn = ok ? p : 0;
	g_vo.waitSince = t;
	if (!ok)
	{
		if (g_vo.step >= g_vo.x->n) { g_vo.x = nullptr; g_vo.ev = -1; }
		else g_vo.at = t + 0.2f;
	}
}

static VoTornado& VoT(int uid)
{
	for (auto& v : g_vo.tors) if (v.uid == uid) return v;
	g_vo.tors.push_back(VoTornado());
	g_vo.tors.back().uid = uid;
	return g_vo.tors.back();
}

static void VoWatch(float t)
{
	Ped me = PLAYER::PLAYER_PED_ID();
	V3 pp = PlayerPos();
	// --- the tornadoes ---
	std::vector<int> alive;
	int flings = 0;
	for (auto& up : g_tornadoes)
	{
		Tornado* tp = up.get();
		alive.push_back(tp->uid);
		if (tp->Display()) continue;
		VoTornado& v = VoT(tp->uid);
		float d = (tp->base - pp).len2d();
		if (tp->Mini())
		{
			// the tornado gun: someone stood near where it went off
			if (!v.gun && tp->Age() < 1.5f)
			{
				v.gun = true;
				Ped hit = 0;
				int arr[256];
				int n = worldGetAllPeds(arr, 256);
				float bd = 14.0f;
				for (int i = 0; i < n; i++)
				{
					Ped p = arr[i];
					if (p == me || !ENTITY::DOES_ENTITY_EXIST(p) || !PED::IS_PED_HUMAN(p) || PED::IS_PED_DEAD_OR_DYING(p, TRUE)) continue;
					float dd = (V3(ENTITY::GET_ENTITY_COORDS(p, TRUE, FALSE)) - tp->base).len2d();
					if (dd < bd) { bd = dd; hit = p; }
				}
				if (hit) VoTrigger(VE_GUN_HIT, hit, t);
			}
			continue;
		}
		flings += tp->flings;
		if (!tp->Display()) v.trees = std::max(v.trees, tp->uprooted + tp->realTrees);
		// (audit 3: one or the other - a near one gets its touchdown remarked on, a far one its first sighting)
		bool nearSpawn = d < 350.0f;
		if (!v.sighted && !nearSpawn && d < 700.0f && tp->growth > 0.2f && tp->Age() > 3.0f) { v.sighted = true; VoTrigger(VE_SIGHTED, 0, t); }
		if (!v.touchdown && tp->TouchedDown()) { v.touchdown = true; if (nearSpawn) VoTrigger(VE_TOUCHDOWN, 0, t); }
		if (d < tp->reachRadius() * 1.5f) g_vo.lastNearAt = t;
		if (!g_vo.held)
		{
			if (!v.close && d < tp->reachRadius() && tp->TouchedDown()) { v.close = true; VoTrigger(VE_CLOSE, 0, t); }
			if (v.close && d > tp->reachRadius() * 1.6f) v.close = false;
			if (!v.nearArmed && d < tp->wallRadius() * 2.2f) { v.nearArmed = true; v.nearArmedAt = t; }
		}
		else
			v.nearArmed = false;   // it caught him: no "phew"
		if (v.nearArmed && d > tp->wallRadius() * 3.5f)
		{
			v.nearArmed = false;
			if (t - v.nearArmedAt < 25.0f) VoTrigger(VE_NEAR_MISS, 0, t);
		}
		if (!v.diesDown && tp->Dissipating() && d < 600.0f) { v.diesDown = true; VoTrigger(VE_DIES_DOWN, 0, t); }
	}
	for (auto& v : g_vo.tors)
		if (std::find(alive.begin(), alive.end(), v.uid) == alive.end()) g_stats.trees += v.trees;   // it's gone: bank its trees
	g_vo.tors.erase(std::remove_if(g_vo.tors.begin(), g_vo.tors.end(), [&](const VoTornado& v)
		{ return std::find(alive.begin(), alive.end(), v.uid) == alive.end(); }), g_vo.tors.end());

	// --- Arthur ---
	bool held = t - PlayerLastInWall() < 0.5f;
	if (held && !g_vo.held) { g_vo.heldSince = t; g_vo.sang = false; g_stats.rides++; VoTrigger(VE_ARTHUR_LIFTED, 0, t); }
	if (held) { g_stats.longestRide = std::max(g_stats.longestRide, t - g_vo.heldSince); g_stats.highest = std::max(g_stats.highest, ENTITY::GET_ENTITY_HEIGHT_ABOVE_GROUND(me)); }
	if (held) g_vo.lastHeldAt = t;
	g_vo.held = held;
	if (held && !g_vo.sang && t - g_vo.heldSince > 6.0f && ENTITY::GET_ENTITY_HEIGHT_ABOVE_GROUND(me) > 8.0f)
	{
		g_vo.evNext[VE_ARTHUR_RIDING] = 0;
		if (VoTrigger(VE_ARTHUR_RIDING, 0, t)) g_vo.sang = true;   // (audit 3: tried again until it plays)
	}
	if (flings > g_vo.flings) { g_stats.flings += flings - g_vo.flings; VoTrigger(VE_ARTHUR_THROWN, 0, t); }
	g_vo.flings = flings;
	int landings = GetLanding().landings;
	if (landings > g_vo.landings && t - g_vo.lastHeldAt < 40.0f) { g_vo.lastLandAt = t; VoTrigger(VE_ARTHUR_LANDED, 0, t); }
	g_vo.landings = landings;

	// --- the jet balloon ---
	if (BalloonActive() && BalloonAboard())
	{
		float nd = 1e9f;
		NearestTornado(&nd);
		if (nd < 400.0f)
		{
			if (g_bal.boost > 0.7f) VoTrigger(VE_BALLOON_BOOST, 0, t);
			else VoTrigger(VE_BALLOON_WATCH, 0, t);
		}
	}

	// --- people (and his horse), a few times a second ---
	if (t < g_vo.nextScan) return;
	g_vo.nextScan = t + 0.35f;
	Ped horse = PED::IS_PED_ON_MOUNT(me) ? 0 : PED::GET_LAST_MOUNT(me);
	if (horse && ENTITY::DOES_ENTITY_EXIST(horse) && !PED::IS_PED_DEAD_OR_DYING(horse, TRUE) && !IsScripted(horse))
	{
		bool up = ENTITY::GET_ENTITY_HEIGHT_ABOVE_GROUND(horse) > 3.5f && (V3(ENTITY::GET_ENTITY_COORDS(horse, TRUE, FALSE)) - pp).len() < 120.0f;
		if (up && !g_vo.horseUp && NearestTornado() != nullptr) VoTrigger(VE_HORSE_LIFTED, 0, t);
		g_vo.horseUp = up;
	}
	if (g_tornadoes.empty() && t - g_vo.lastNearAt > 90.0f && t - g_vo.lastLandAt > 90.0f)
	{
		g_vo.peds.clear();   // (audit 3: in a town the list never emptied, so the scan never stopped)
		return;
	}
	int arr[256];
	int n = worldGetAllPeds(arr, 256);
	std::unordered_set<int> seen;
	for (int i = 0; i < n; i++)
	{
		Ped p = arr[i];
		if (p == me || p == horse || !ENTITY::DOES_ENTITY_EXIST(p) || !PED::IS_PED_HUMAN(p) || PED::IS_PED_DEAD_OR_DYING(p, TRUE)) continue;
		V3 pos = ENTITY::GET_ENTITY_COORDS(p, TRUE, FALSE);
		float dMe = (pos - pp).len();
		if (dMe > 90.0f) continue;
		seen.insert(p);
		VoPed& vp = g_vo.peds[p];
		float hag = ENTITY::GET_ENTITY_HEIGHT_ABOVE_GROUND(p);
		bool nearT = false;
		for (auto& up : g_tornadoes) if ((up->base - pos).len2d() < up->reachRadius()) { nearT = true; break; }
		if (IsScripted(p))
		{
			// the intro's posed riders: the mod flies them - no screams (they'd all go at once); one greeting each, at most
			if (!vp.up && hag > 3.0f && nearT && dMe < 70.0f && VoGangIndex(p) >= 0) { vp.up = true; VoTrigger(VE_GANG_LIFTED, p, t); }
			continue;
		}
		if (!vp.up && hag > 3.0f && nearT)
		{
			vp.up = true; vp.upAt = t; vp.downFor = 0;
			vp.nextScream = t + RandRange(3.5f, 6.0f);
			bool screamed = VoScreamNow(p, t);
			g_stats.taken++;
			if (VoGangIndex(p) >= 0) g_stats.gangTaken++;
			if (dMe < 70.0f) VoTrigger(VoGangIndex(p) >= 0 ? VE_GANG_LIFTED : VE_NPC_LIFTED, p, t, screamed ? p : 0);
		}
		else if (vp.up)
		{
			if (hag < 1.0f) { vp.downFor += 0.35f; if (vp.downFor > 1.0f) vp.up = false; }
			else vp.downFor = 0;
			if (vp.up && t >= vp.nextScream && g_vo.waitOn != p && !(g_vo.x && g_vo.who[VR_SUBJECT] == p))
			{
				VoScream(p, t);
				vp.nextScream = t + RandRange(5.0f, 9.0f);
			}
		}
		// folks have worked out who brings the weather: someone Arthur walks right up to after a tornado
		else if (dMe < 5.0f && hag < 1.0f && (t - g_vo.lastNearAt < 90.0f || t - g_vo.lastLandAt < 90.0f) && t - vp.lastSaid > 60.0f)
			if (VoTrigger(VE_BLAMED, p, t)) vp.lastSaid = t;
	}
	for (auto it = g_vo.peds.begin(); it != g_vo.peds.end();)
		it = seen.count(it->first) ? std::next(it) : g_vo.peds.erase(it);
}

// A storm starts with the first real tornado (not a gallery exhibit, not a mini twister) and ends when the last one is gone.
static void StormWatch(float t)
{
	int real = 0;
	for (auto& tp : g_tornadoes) if (!tp->Display() && !tp->Mini()) real++;
	if (real && !g_stats.on)
	{
		g_stats = StormStats();
		g_stats.on = true;
		g_stats.start = t;
		return;
	}
	if (real || !g_stats.on)
		return;
	g_stats.on = false;
	for (auto& v : g_vo.tors) g_stats.trees += v.trees;
	float lasted = t - g_stats.start;
	Finding("STORM REPORT %.0f s | taken %d (gang %d) | trees %d | Arthur rides %d, longest %.0f s, highest %.0f m, thrown %d | lines said %d",
		lasted, g_stats.taken, g_stats.gangTaken, g_stats.trees, g_stats.rides, g_stats.longestRide, g_stats.highest, g_stats.flings, g_stats.lines);
	if (!g_stormReport || lasted < 40.0f || IntroActive() || GalleryActive())
		return;
	char b[300];
	std::string ride = g_stats.rides
		? (std::string("Arthur rode it ") + std::to_string((int)g_stats.longestRide) + " s, " + std::to_string((int)g_stats.highest) + " m up" + (g_stats.flings ? ", and got thrown." : "."))
		: std::string("Arthur kept his boots on the ground.");
	sprintf_s(b, "%d %s taken%s, %d %s torn out. %s", g_stats.taken, g_stats.taken == 1 ? "soul" : "folks",
		g_stats.gangTaken ? (std::string(" (") + std::to_string(g_stats.gangTaken) + " of the gang)").c_str() : "",
		g_stats.trees, g_stats.trees == 1 ? "tree" : "trees", ride.c_str());
	UI::Toast("Storm report", b, 8.0f);
	Log("STORM REPORT shown: %s", b);
}

static void VoiceUpdate(float t)
{
	StormWatch(t);
	bool paused = IntroRunning() || GalleryActive() || MISC::GET_MISSION_FLAG() || PlayerDead();
	// (audit 3: the watch also feeds the storm report, so it runs with Voices Off - its triggers just stay silent then)
	if (!paused && (g_voices || g_stormReport)) VoWatch(t);
	if (g_voices == 0 || paused)
	{
		if (g_vo.x) VoStop("paused");
		return;
	}
	VoAdvance(t);
}

static void VoiceReport()
{
	if (g_vo.played + g_vo.refused + g_vo.noLine)
		Finding("VOICES %d lines played, %d refused by the game, %d with no line for that voice", g_vo.played, g_vo.refused, g_vo.noLine);
}

// ---------- the voice audition (Developer tools) ----------
// research\speech_reactions_research.md §10: no public dump ties a line's words to its name, so the mod plays every line it
// uses, one every 4 s, with the game's subtitle asked for and its name on screen. Record it and the transcript says exactly what
// Arthur (and the nearest passer-by) says for each - then the exchanges can be tuned by ear.
static std::vector<std::string> VoContextsFor(bool arthur)
{
	std::vector<std::string> out;
	for (int i = 0; i < kVoExchangeCount; i++)
		for (int s = 0; s < kVoExchanges[i].n; s++)
		{
			const VoStep& st = kVoExchanges[i].s[s];
			if ((st.role == VR_ARTHUR) != arthur) continue;
			for (int c = 0; c < 6 && st.ctx[c]; c++)
			{
				std::string x = st.ctx[c];
				if (x == "GREET_*") x = "GREET_DUTCH";
				if (std::find(out.begin(), out.end(), x) == out.end()) out.push_back(x);
			}
		}
	return out;
}

static void AutoVoiceAudition()
{
	AutoStop("replaced");
	std::vector<AutoStep> steps;
	std::vector<std::string> a = VoContextsFor(true);
	Ped npc = VoBystander(0, 15.0f);
	std::vector<std::string> n = npc ? VoContextsFor(false) : std::vector<std::string>();
	int total = (int)(a.size() + n.size()), k = 0;
	for (auto& c : a)
	{
		char d[160];
		sprintf_s(d, "Voice audition %d/%d - Arthur: %s", ++k, total, c.c_str());
		steps.push_back({ 4.0f, d, [c]()
		{
			Ped me = PLAYER::PLAYER_PED_ID();
			std::string ctx = c[0] == '~' ? c.substr(1) : c;
			bool has = c[0] == '~' || AUDIO::DOES_CONTEXT_EXIST_FOR_THIS_PED(me, ctx.c_str(), FALSE);
			bool ok = has && Speak(me, ctx.c_str(), "SPEECH_PARAMS_BEAT_SPOKEN_CLEAR_SUB");
			Finding("AUDITION arthur %-36s | has it %d | played %d", ctx.c_str(), (int)has, (int)ok);
		} });
	}
	for (auto& c : n)
	{
		char d[160];
		sprintf_s(d, "Voice audition %d/%d - the person nearby: %s", ++k, total, c.c_str());
		steps.push_back({ 4.0f, d, [c, npc]()
		{
			if (!npc || !ENTITY::DOES_ENTITY_EXIST(npc)) return;
			std::string ctx = c[0] == '~' ? c.substr(1) : c;
			bool has = c[0] == '~' || AUDIO::DOES_CONTEXT_EXIST_FOR_THIS_PED(npc, ctx.c_str(), FALSE);
			bool ok = has && Speak(npc, ctx.c_str(), "SPEECH_PARAMS_BEAT_SPOKEN_CLEAR_SUB");
			Finding("AUDITION npc    %-36s | has it %d | played %d", ctx.c_str(), (int)has, (int)ok);
		} });
	}
	Finding("AUDITION start: %d of Arthur's lines, %d for the person nearby (%s)", (int)a.size(), (int)n.size(), npc ? "found one within 15 m" : "nobody within 15 m");
	AutoRun("voice audition", steps);
}
