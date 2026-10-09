// Tornado Redemption v1.7 - THE INTRO: "Storm Chasers". Part of script.cpp (included there for the tornado, storm, camera and
// balloon helpers). The screenplay and shot list are in INTRO.md.
//
// v1.7, for the video's cold open. The user: "FIX INTRO, 30s to 40s MAX AND LEGIBLE COMEDY". From wherever you are, the game
// takes you to the gang's camp at Horseshoe Overlook near Valentine on a sunny morning (Rockstar's own camp: the map pieces their
// camp script switches on, the gang on Rockstar's own camp spots - "just reuse anything rockstar has... a normal camp day is
// something we can basically pull up"). Arthur, Dutch, Micah and John tell Susan Grimshaw and Abigail they're off storm chasing,
// Micah and Arthur bicker, the four ride out, past Valentine, and pull up on a ridge above Emerald Ranch as the weather turns.
// Thunder, once. The tornado comes down on the ranch. Micah points - it's coming this way - Dutch and John bolt, and Arthur,
// standing beside Micah, pushes him off the ridge into it. Then it's yours: run for the hot air balloon that's appeared behind you
// (Cain's in it), the tornado on your heels.
//
// What carries over from v1.3-v1.6 (and why):
//   - every line is the character's own, from the game's story, by name (research\intro_v17_lines.md); no invented subtitles;
//   - the camera director (v1.4): every shot names what it must show and a camera that can't see it is moved until it can;
//   - the ears (v1.5): no line in the first moments after a cut, a cut waits for a line still being said, far speakers are
//     played from beside the camera;
//   - no grade, no slow motion, no synthesized booms or whooshes (playtest 15: "too loud"); one quiet lightning flash, far off.
// v1.7.1, after playtest 19 ("what I saw was amazing" - then an ERROR FFFFFFFF at the Valentine cut, 2.5 s after the scene had
// switched Rockstar's camp pieces back off, with a load-scene for the ridge running): the camp pieces stay up until the scene is
// over and Arthur is far away; the moves between places are a short dip to black with the riders moved while it's dark (no more
// load-scene); a longer opening with a bigger title; Dutch asks Arthur; Susan shoos them off and walks away; Micah and Arthur in
// over-the-shoulder close-ups; Jack sits still; the riders take the clearest way out of camp; a ridge with more of a drop.
// v1.7.2, after playtest 20 ("way way better... I will cut the final intro fully in vegas myself, give some transition time"):
// every shot has a lead-in and a tail to cut on (the scene runs about 75 s; the edit trims it); Dutch, in front of the line, asks
// Arthur over his shoulder with an upset John beside him; faces with feeling (Arthur angry, Micah smug, scared at the storm);
// the Valentine and ridge cameras are found with the places (the town in view, the riders' path clear of trees); the ridge talk
// in over-the-shoulder close-ups; the touchdown has its own hero shot at the ranch, with its cattle and farmhands for the funnel
// to take; John yells from behind the four, Dutch gets his own shot to say his piece and run, Micah's last line is close on his
// face with Arthur behind him, and the push is shot from behind them.
// v1.7.3, after playtest 21 ("nearly perfect until john says it's right on us then dutch takes a moment to deliver his line and
// everyone runs while he does nothing then it awkwardly cuts to micah who isn't even facing arthur"): Dutch speaks at once and
// breaks into a run mid-line; Micah turns to Arthur for his last word, his back to the drop and the funnel behind him, and Arthur
// shoves him from the front - backwards off the ridge.
// New in v1.7:
//   - three places. They're found once (the road out of Valentine, the ridge over the ranch) while the screen is black and
//     remembered in TornadoRedemption_intro.txt; the riders are moved between them at the cuts, and a cut waits (a little) for
//     the next place's ground to be loaded;
//   - the camp's map pieces are put back the way they were once the scene has left the camp.

// ---------- where ----------
static const V3 kCampCentre(-125.85f, -39.96f, 96.09f);      // Horseshoe Overlook (Rockstar's camp_horseshoeoverlook.c)
static const V3 kCampMuster(-111.06f, -24.12f, 96.61f);      // its "volMuster": where the gang meets to ride out
static const V3 kCampFire(-140.60f, -42.20f, 96.17f);        // its fire ("volEntityRestrictionfire")
static const V3 kValentineC(-213.2f, 691.8f, 112.4f);
static const V3 kRanchC(1332.3f, 300.4f, 86.3f);             // Emerald Ranch

// The camp's map pieces (IPLs). Rockstar's camp script requests one of three base layouts for camp 1 (Horseshoe Overlook) while
// it's the gang's camp, and 633503129 (what's left of it) once they've moved on; the rest are its tents, tables and upgrades -
// one per spot here (femga's imap list for where each one is; the first of each spot's variants in the script).
static const int kCampBaseIpl = -697307430;
static const int kCampBaseAlt[] = { -1873685184, -1536198599 };
static const int kCampAbandonedIpl = 633503129;
static const int kCampIpls[] = {
	-697307430, 825179479, 1453569688, -1166561064, 163126540, -1611076340, -1920340119, 1157066259, 288413571, -441619793,
	-80564929, -1445186253, -1893724593, -268335331, 1546110128, 324486076, -2052582076, 1029525997, -1472352094, -745860880,
	1219276914, -302735166, -2077690059, 1548546221, 1560807181, 106249677, -723982773, 327932996, -15722296, 1079303588,
	-937893311, 1808508475, -1987982797, -182995231, 1267297807, -1150137955, -710911638, 2047954825, 1924209179, 1998041523,
	2022451711, -1177590512,
};

// ---------- who ----------
enum CastRole { ROLE_RIDER, ROLE_HOME, ROLE_CAMP, ROLE_DOG };
struct CastDef
{
	const char* id; const char* model; const char* speaker;
	int role;
	float x, y;                 // camp extras: where in camp (world), near one of Rockstar's spots
	const char* scenario;       // what they do if there's no camp spot to take
	bool wander;                // ...or walk about the camp
};
static const CastDef kCast[] = {
	{ "dutch",    "cs_dutch",               "DUTCH",         ROLE_RIDER, 0, 0, nullptr, false },
	{ "micah",    "cs_micahbell",           "MICAH_BELL",    ROLE_RIDER, 0, 0, nullptr, false },
	{ "john",     "cs_johnmarston",         "JOHN",          ROLE_RIDER, 0, 0, nullptr, false },
	{ "susan",    "cs_susangrimshaw",       "SUSAN",         ROLE_HOME,  0, 0, nullptr, false },
	{ "abigail",  "cs_abigailroberts",      "ABIGAIL",       ROLE_HOME,  0, 0, nullptr, false },
	{ "pearson",  "cs_mrpearson",           "PEARSON",       ROLE_CAMP, -123.7f, -42.4f, "WORLD_HUMAN_CAULDRON_STIR", false },
	{ "javier",   "cs_javierescuella",      "JAVIER",        ROLE_CAMP, -129.6f, -37.4f, "WORLD_HUMAN_SIT_GUITAR", false },
	{ "lenny",    "cs_lenny",               "LENNY",         ROLE_CAMP, -138.4f, -40.6f, "WORLD_CAMP_FIRE_SIT_GROUND", false },
	{ "hosea",    "cs_hoseamatthews",       "HOSEA",         ROLE_CAMP, -118.4f, -35.4f, "WORLD_HUMAN_SIT_GROUND_READING_BOOK", false },
	{ "uncle",    "cs_uncle",               "UNCLE",         ROLE_CAMP, -134.6f, -33.5f, "WORLD_HUMAN_SLEEP_GROUND_ARM", false },
	{ "charles",  "cs_charlessmith",        "CHARLES_SMITH", ROLE_CAMP, -133.0f, -45.0f, nullptr, true },
	{ "karen",    "cs_karen",               "KAREN",         ROLE_CAMP, -128.0f, -49.0f, nullptr, true },
	{ "marybeth", "cs_marybeth",            "MARYBETH",      ROLE_CAMP, -122.5f, -36.5f, "WORLD_HUMAN_SIT_GROUND_READING_BOOK", false },
	{ "jack",     "cs_jackmarston",         "JACK",          ROLE_CAMP, -136.0f, -40.0f, "WORLD_CAMP_FIRE_SIT_GROUND", false },   // (playtest 19: his head wandered into the ride-out)
	{ "cain",     "a_c_dogcatahoulacur_01", "",              ROLE_DOG,   0, 0, nullptr, false },
};
static const int kCastCount = sizeof(kCast) / sizeof(kCast[0]);
enum { CA_DUTCH, CA_MICAH, CA_JOHN, CA_SUSAN, CA_ABIGAIL, CA_PEARSON, CA_JAVIER, CA_LENNY, CA_HOSEA, CA_UNCLE, CA_CHARLES, CA_KAREN,
	CA_MARYBETH, CA_JACK, CA_CAIN };

// The four who ride out, in their line at the ridge, left to right looking at the ranch: Dutch, John, Micah, Arthur (beside Micah).
enum { RS_DUTCH, RS_JOHN, RS_MICAH, RS_ARTHUR, kRiders };
static const int kRiderCast[kRiders] = { CA_DUTCH, CA_JOHN, CA_MICAH, -1 };
static const char* kRiderHorse[kRiders] = { "A_C_HORSE_GANG_DUTCH", "A_C_HORSE_GANG_JOHN", "A_C_HORSE_GANG_MICAH", "A_C_HORSE_MORGAN_BAYROAN" };
static const float kRidgeK[kRiders] = { -2.7f, -1.35f, 0.0f, 1.25f };   // across the ridge (m)
static const float kCampK[kRiders] = { -1.9f, -0.65f, 0.65f, 1.9f };    // across the camp's edge (m)
// v1.7.2 (playtest 20: "I wish it was visibly touching right down on emerald ranch, and visibly sucking up the animals and people"):
// the ranch's own - cattle, pigs, a horse and its farmhands - round where the funnel comes down
static const char* kRanchLife[] = { "a_c_cow", "a_c_cow", "a_c_cow", "a_c_cow", "a_c_pig_01", "a_c_pig_01", "a_c_horse_morgan_bay",
	"a_m_m_emrfarmhand_01", "a_m_m_emrfarmhand_01", "a_m_m_rancher_01" };

// Fallback camp (only if the game wouldn't switch Rockstar's camp on): props around the camp centre. Local x = across (S), y =
// toward the way out (X).
struct PropDef { const char* model; const char* alt; float x, y, heading; };
static const PropDef kCampProps[] = {
	{ "p_campfire02x", "p_campfire01x",         0.0f, 0.0f, 0 },
	{ "p_gangtentlemoyne01x", "p_amb_tent01x",  -3.5f, -7.5f, 160 },
	{ "p_amb_tent01x", "p_amb_tent02x",        -9.0f, 2.0f, 75 },
	{ "p_amb_tent02x", "p_amb_tent01x",         9.5f, 4.5f, -80 },
	{ "p_gangtablemake01x", "p_table01x",      -6.6f, 2.0f, 90 },
	{ "p_cauldron01x", "p_pot01x",              4.4f, -1.4f, 0 },
	{ "p_woodpile01x", nullptr,                 3.0f, -6.5f, 30 },
	{ "p_crate03x", nullptr,                   -2.2f, -5.0f, 15 },
	{ "p_barrel05x", nullptr,                   6.5f, -3.5f, 0 },
	{ "p_chuckwagon01x", "p_chuckwagon02x",     9.0f, -7.5f, -30 },
};
static const int kCampPropCount = sizeof(kCampProps) / sizeof(kCampProps[0]);

// ---------- what they say ----------
// One line of one of Rockstar's own scripted conversations (text block, root, line index), the character's real voice, with the
// game's own subtitle. If the game won't play it here (nothing playing 1.2 s later), fbWho's first ambient context plays instead.
struct IntroLine
{
	float t; int who; const char* ctx[4]; bool shout;   // who: cast index, -1 Arthur. ctx: the fallback (ambient contexts)
	const char* block; const char* root; int idx;       // the story line (block = its subtitles' text block)
	const char* words;                                  // what it says (for the log)
	int fbWho;                                          // who says the fallback
	const char* audio;                                  // the line's audio name in its scene's bank, if known (played beside the
	                                                    // camera when the speaker is too far off to be heard)
};
// research\intro_v17_lines.md: every row is the character's real line, from Rockstar's own scenes (SURE / LIKELY speakers there).
// The three back-and-forths are real exchanges - two lines in a row of one scene: Abigail and John's own lines are kept for
// Abigail's (CCJN4), Micah and Arthur at camp (TRN1_MICAHBANT1 0/1), Micah and Arthur on the ridge (CMCO1_ACTION 2/3).
enum { LN_DUTCH_GO, LN_SUSAN, LN_ABIGAIL, LN_MICAH_LUNGS, LN_ARTHUR_SHUT, LN_MICAH_WEATHER, LN_ARTHUR_LIZARD, LN_JOHN_POINT,
	LN_DUTCH_PLAN, LN_MICAH_FUN, LN_ARTHUR_FALL };
static const IntroLine kIntroLines[] = {
	// t       who          fallback contexts                                               shout  story line                                    words                                                       fallback by  audio
	{ 6.7f,  CA_DUTCH,   { "HEADING_OUT", "CAMP_GOODBYE", "GENERIC_GOODBYE" },          false, "FUD1AUD", "FUD1FOLLDUTCH", 0,          "Come on, let's get out of here for a bit.",                CA_DUTCH,   nullptr },
	{ 11.5f, CA_SUSAN,   { "CAMP_GOODBYE", "GENERIC_GOODBYE" },                         false, "CPGENAU", "CPGEN_SCHECK1", 2,          "Well, hurry it along.",                                    CA_SUSAN,   nullptr },
	{ 13.1f, CA_ABIGAIL, { "SHAME_ON_YOU", "WHATS_YOUR_PROBLEM" },                      false, "CCJN4AU", "CCJN4_ACT", 0,              "Will you go rest, please?",                                CA_ABIGAIL, "CCJN4_AAAA" },
	{ 16.8f, CA_MICAH,   { "GENERIC_INSULT_MED", "PROVOKE_GENERIC" },                   false, "TRN1AUD", "TRN1_MICAHBANT1", 0,        "You sure you got the lungs for this, Morgan?",             CA_MICAH,   nullptr },
	{ 21.4f, -1,         { "TELLS_PED_TO_SHUT_UP" },                                    false, "TRN1AUD", "TRN1_MICAHBANT1", 1,        "Shut the hell up.",                                        -1,         nullptr },
	{ 37.7f, CA_MICAH,   { "DISMISSIVE_REACT", "GENERIC_INSULT_MED" },                  false, "CMCO1AU", "CMCO1_ACTION", 2,           "Weather don't worry me. In fact, I like it.",              CA_MICAH,   "CMCO1_AAAC" },
	{ 42.5f, -1,         { "MOCK_RESPONSE_NEG", "TELLS_PED_TO_SHUT_UP" },               false, "CMCO1AU", "CMCO1_ACTION", 3,           "Funny, I didn't think lizards survived in the mountains.", -1,         "CMCO1_AAAD" },
	{ 58.2f, CA_JOHN,    { "WHOA_ESCALATED", "GENERIC_FRIGHTENED_HIGH", "MOVE_IT_NEAR" }, true, "TRN3AUD", "TRN3_ET_HURRY", 0,          "It's right on us, come on!",                               CA_JOHN,    nullptr },
	{ 61.0f, CA_DUTCH,   { "GENERIC_FRIGHTENED_MED", "LEAVE_NOW" },                     false, "CFDV2AU", "CFDV2_ACT", 6,              "We have a plan. My friends.",                              CA_DUTCH,   "CFDV2_AAAG" },
	{ 64.8f, CA_MICAH,   { "GENERIC_FRIGHTENED_HIGH", "WHOA_ESCALATED" },               false, "CFMB6AU", "CFMB6_ACT", 1,              "Well this is fun, ain't it?",                              CA_MICAH,   "CFMB6_AAAB" },
	{ 68.5f, -1,         { "GENERIC_GOODBYE" },                                         false, "DMONKAU", "DMONK_ANTAG_A", 0,          "Don't fall off.",                                          -1,         nullptr },
};
// v1.7.1 (playtest 19: Dutch's "Come on, let's get out of here for a bit." came out as another of its takes): two of the lines are
// from sets Rockstar plays at random. The take that started is read back (GET_CURRENT_SCRIPTED_CONVERSATION_LINE) and one that
// doesn't fit is stopped and drawn again, before it's heard (up to four times). Every take of Dutch's set is said to Arthur.
struct LinePick { int line; int ok[4]; };
static const LinePick kLinePicks[] = {
	{ LN_DUTCH_GO,    { 0, 1, 3, -1 } },   // "Come on, let's get out of here for a bit." / "Are you coming?" / "Let's go. You need some recreation, my boy."
	{ LN_ARTHUR_FALL, { 0, 1, 4, 7 } },    // "Don't fall off." / "Boo." / "Am I bothering you?" / "Don't lose your focus."
};
static const int kIntroLineCount = sizeof(kIntroLines) / sizeof(kIntroLines[0]);
// Arthur's line as he climbs into the balloon (gameplay, after the handoff): from the real balloon mission, at lift-off
struct AfterLine { const char* block; const char* root; int idx; const char* words; const char* ctx; };
static const AfterLine kAfterLine = { "GNG2AUD", "GNG2_LIFT_OFF", 1, "Okay, here goes nothing.", "PLAYER_ACKNOWLEDGEMENT" };
static void IntroStoryBlocks(void (*fn)(const char*));

// ---------- when ----------
// The shots (the camera code, the cuts and the holds all read this table). About 40 s to the handoff.
// v1.7.2: every shot has a lead-in (a line starts 1.2 s in) and a tail (about 1 s after the last word) for the edit.
enum { SH_CAMP, SH_FAREWELL, SH_WOMEN, SH_MICAH, SH_ARTHUR, SH_RIDEOUT, SH_VALENTINE, SH_ARRIVE, SH_RIDGE_M, SH_RIDGE_A, SH_THUNDER,
	SH_TOUCHDOWN, SH_RANCH, SH_POINT, SH_DUTCH, SH_MICAHLAST, SH_SHOVE, SH_FALL };
static const float kIntroShotAt[] = {
	0.0f,  5.5f,  10.3f, 15.6f, 20.2f, 23.6f, 27.6f, 32.0f, 36.5f, 41.3f, 46.4f, 48.4f, 52.4f, 57.0f, 60.6f, 63.6f, 67.3f, 70.2f,
};
static const int kIntroShotCount = sizeof(kIntroShotAt) / sizeof(kIntroShotAt[0]);
static const float kIntroHandoff = 73.4f;      // the camera eases back to gameplay here
static const float kIntroThunder = 46.8f;      // the one flash, far off over the ranch
static const float kIntroSpawn = 48.6f;        // the funnel comes down on the ranch (the touchdown, sped up to 2.5 s)
static const float kIntroSweep = 56.0f;        // ...takes the ranch apart (its own hero shot), then leaves it for the ridge
static const float kIntroArrive = 68.8f;       // at the foot of the ridge, for the push
static const float kIntroJohnRuns = 60.6f, kIntroDutchRuns = 62.0f;   // John at the cut after his line; Dutch a second into his (v1.7.3)
static const float kIntroShove = 69.0f;        // Arthur pushes Micah off the ridge (on the last word)
static const float kIntroBalloon = 70.4f;      // the balloon appears behind them (off camera)
static const float kIntroPushDist = 36.0f;     // the funnel waits this far out from the ridge (m)
static const float kIntroEarDelay = 1.2f;      // a line waits this long after a cut (v1.7.2: a lead-in for the edit)
static const float kIntroTail = 1.0f;          // v1.7.2: and a cut waits this long after the shot's last word

// the intro's clock source (real time); the offline harness swaps in its mock clock
static DWORD (*g_introTick)() = []() -> DWORD { return GetTickCount(); };

static std::vector<int> g_introFx;            // the camp fire
// what a finished scene handed to the world (IntroReleaseCast). Despawn everything still clears it - each one only if it still
// exists and is still the model we made (a handle can be reused by something else once the game deletes ours).
struct IntroLeft { Entity e; Hash model; bool ped; };
static std::vector<IntroLeft> g_introLeft;

struct CastState
{
	Ped ped = 0;
	bool ok = false, outfit = false;
	bool scenario = false;      // took one of Rockstar's camp spots
};
struct Hidden { Ped p; };       // the real camp's people (chapter 2 saves), out of sight while ours are there
struct IntroState
{
	int stage = 0;               // 0 off, 1 fading out, 2 building, 3 settling, 4 scene, 5 after the handoff (the run for the balloon)
	float stageAt = 0;
	double clock = 0;            // seconds since the fade-in (real time, pauses with the game)
	DWORD lastTick = 0;
	// the places
	V3 G, X, S;                  // camp: where the four meet their horses, the way out, to the right of it
	V3 O, D, Sd;                 // the ridge over Emerald Ranch: where they stand, toward the ranch, to the right
	V3 ranch;                    // the ranch, on the ground
	V3 VR, VD, VS;               // the road out of Valentine: a point on it, along it away from town, to the right
	V3 B;                        // where the balloon appears
	bool surveyed = false;
	int phase = 0;               // 0 camp, 1 riding out, 2 Valentine, 3 arriving, 4 at the ridge on foot
	float streamHold = 0;        // a cut held for the next place's ground (s)
	float holdShove = 0;         // the fall's cut held for the push (s)
	int dip = 0, dipShot = -1;   // v1.7.1: a move to a new place - 1 fading out, 2 waiting for the ground there
	float dipAt = 0;
	V3 W;                        // v1.7.1: the clearest way out of camp
	V3 valCam, arrCam;           // v1.7.2: the Valentine and ridge-arrival cameras, found with the places
	float valOff = 0;            // v1.7.2: the riders' path at Valentine, moved sideways off the road's line if that's clear of trees
	float lineEndReal = -100;    // v1.7.2: when the last line ended (real time), for the tails
	bool lineWasOn = false;
	std::vector<Ped> ranchLife;  // v1.7.2: the ranch's cattle and farmhands, for the funnel
	bool picked[kIntroLineCount] = {};   // a random take checked
	int rerolls = 0;
	bool loadScene = false;
	CastState cast[kCastCount];
	Ped horses[kRiders] = {};
	std::vector<Entity> props;
	std::vector<Hidden> hidden;
	std::vector<int> iplOn;      // camp pieces we switched on (switched off again once the scene leaves the camp)
	bool abandonedWas = false, campMap = false, realCamp = false, campGone = false;
	TornadoRef tp;
	int cam = 0, oldCam = 0;
	bool linePlayed[kIntroLineCount] = {};
	float lineStart[kIntroLineCount] = {};   // scene clock when it started (0 = not yet)
	int storyLine = -1; float storyAt = 0; bool storySeen = false;
	int storyPlayed = 0, storyFell = 0;
	std::vector<int> firedBeats;
	bool savedGod = true, savedGrab = true, savedPeople = true, savedAnimals = true;
	int savedArthur = 2, savedMove = 2, savedSpeed = 1;
	bool savedTouchCam = true, savedRideCam = true;
	Hash savedLock = 0; bool savedManual = false;
	bool skipRequested = false;
	int camShot = -1;
	Ped realHorse[2] = { 0, 0 };  // Arthur's own horse(s): left where they are, kept out of the tornado's hands
	// the camera director
	int dirShot = -1;
	float dirYaw = 0, dirLift = 0, dirPull = 1, dirCheckAt = 0, dirLastT = 0;
	float dirYawS = 0, dirLiftS = 0, dirPullS = 1;
	int dirLogged = -1;
	V3 dirPos;
	int dirMoves = 0;
	// the ears
	float cutAt = 0, lineAt = -100;
	DWORD nearUntilMs = 0;
	Ped ambPed = 0;
	float ambAt = -100;
	float holdHere = 0, holdTotal = 0;
	int nearLines = 0;
	bool blockReq[kIntroLineCount] = {}, blockGone[kIntroLineCount] = {};
	// the push
	Ped pushPed = 0; V3 pushVel; float pushUntil = -1;
	float shovedAt = 1e9f;       // when the push happened (scene clock)
	bool micahGone = false;      // in the funnel
	float micahAng = 0, micahH = 0;
	// after the handoff
	float afterStart = 0;
	int blip = 0;
	bool boarded = false; float boardedAt = 0;
	bool afterLine = false;
	bool shieldOn = false;       // the 10 s the tornado can't take you
	bool grabSet = false;
} g_in;

bool IntroRunning() { return g_in.stage >= 1 && g_in.stage <= 4; }
bool IntroActive() { return g_in.stage != 0; }
bool IntroOwnsGrade() { return IntroRunning(); }
// v1.7: the run for the balloon - the tornado leaves you alone for the first 10 s of it (the user: "10 seconds of ungrabbability")
bool IntroShieldsPlayer() { return IntroRunning() || (g_in.stage == 5 && g_in.shieldOn); }
// v1.7: the sky stays the scene's (cloudy, no lightning) until you're in the balloon
bool IntroHoldsSky() { return IntroRunning() || (g_in.stage == 5 && !g_in.boarded); }

static float HeadingTo(const V3& from, const V3& to) { V3 d = to - from; return atan2f(-d.x, d.y) * 180.0f / PI; }
static V3 FlatDir(const V3& from, const V3& to) { V3 d = to - from; d.z = 0; float L = std::max(0.1f, d.len2d()); return V3(d.x / L, d.y / L, 0); }
static V3 LerpV(const V3& a, const V3& b, float u) { return a + (b - a) * u; }
static float Ease(float u) { u = Clamp(u, 0, 1); return u * u * (3 - 2 * u); }
static Tornado* ITp() { return g_in.tp.get(); }
static V3 OnGround(V3 p, float up = 0.0f)
{
	p.z = GroundZ(p.x, p.y, p.z + 40.0f, p.z) + up;
	return p;
}

// ---------- speech ----------
struct SpeechParams
{
	const char* speechName;
	const char* voiceName;
	alignas(8) int variation;
	alignas(8) Hash speechParamHash;
	alignas(8) Ped listenerPed;
	alignas(8) BOOL syncOverNetwork;
	alignas(8) int v7;
	alignas(8) int v8;
};
static_assert(sizeof(SpeechParams) == 0x40, "speech params are 8 slots of 8 bytes");
static bool Speak(Ped p, const char* context, const char* params = "SPEECH_PARAMS_FORCE_SHOUTED")
{
	if (!p || !ENTITY::DOES_ENTITY_EXIST(p)) return false;
	SpeechParams sp{ context, nullptr, 0, Joaat(params), 0, FALSE, 1, 1 };
	BOOL ok = AUDIO::PLAY_PED_AMBIENT_SPEECH_NATIVE(p, reinterpret_cast<int*>(&sp));
	return ok != 0;
}

static Ped CastPed(int i) { return (i >= 0 && i < kCastCount && g_in.cast[i].ok) ? g_in.cast[i].ped : 0; }
static Ped Speaker(int who) { return who < 0 ? PLAYER::PLAYER_PED_ID() : CastPed(who); }
static Ped Rider(int slot) { return kRiderCast[slot] < 0 ? PLAYER::PLAYER_PED_ID() : CastPed(kRiderCast[slot]); }

static bool LoadAnimDict(const char* dict, int timeoutMs)
{
	if (!dict) return false;
	if (STREAMING::HAS_ANIM_DICT_LOADED(dict)) return true;
	STREAMING::REQUEST_ANIM_DICT(dict);
	DWORD until = GetTickCount() + timeoutMs;
	while (!STREAMING::HAS_ANIM_DICT_LOADED(dict))
	{
		if (GetTickCount() > until) return false;
		WAIT(0);
	}
	return true;
}

static void PlayAnim(Ped p, const char* dict, const char* clip, bool loop, float blendIn = 4.0f)
{
	if (!p || !dict || !STREAMING::HAS_ANIM_DICT_LOADED(dict)) return;
	// RDR2's anim flags (Halen84 eScriptedAnimFlags): 1 looping, 4 not interruptable, 8192 force start
	TASK::TASK_PLAY_ANIM(p, dict, clip, blendIn, -4.0f, -1, (loop ? 1 : 2) | 4 | 8192, 0.0f, FALSE, 0, FALSE, nullptr, FALSE);
}
// upper body only, over whatever they're doing (Rockstar's most common flags for it: upper body + secondary)
static void PlayUpper(Ped p, const char* dict, const char* clip)
{
	if (!p || !dict || !STREAMING::HAS_ANIM_DICT_LOADED(dict)) return;
	TASK::TASK_PLAY_ANIM(p, dict, clip, 4.0f, -4.0f, -1, 16 | 8, 0.0f, FALSE, 0, FALSE, nullptr, FALSE);
}

// the clips the scene uses (research\intro_v17_lines.md, "Animations"; CODE = a Rockstar script loads the dict)
static const char* kDictMelee = "mech_melee@unarmed@_male@_ambient@_healthy@_noncombat";   // the player's own shoves (CODE)
static const char* kAnimShoveAtt = "shove_from_front_dist_close_v1_att";   // (v1.7.3: face to face)
static const char* kAnimShoveVic = "shove_from_front_dist_close_v1_vic";
static const char* kAnimPoint[2] = { "ai_react@point@base", "point_fwd" };                    // (proven)
static const char* kDictStartle = "ai_react@male_stand@big_intro@forward";                     // the thunder (LIKELY)
static const char* kAnimStartle[3] = { "reaction_forward_big_intro_a", "reaction_forward_big_intro_b", "reaction_forward_big_intro_c" };
static const char* kAnimLookUp[2] = { "ai_gestures@arthur@standing@speaker", "neutral_lookup_f_001" };
static const char* kAnimWave[2] = { "ai_gestures@arthur@standing@speaker", "greet_hat_tip_r_001" };   // (proven)
static const char* kDictWindHat = "veh_horseback@seat_saddle@generic@terrain@unarmed@wind@idle@fidget@hat";   // holding hats on the ride
static const char* kDictSusanIdle = "amb_camp@world_camp_grimshaw_stare@wip_base@female_a";   // Susan's own camp stare
static const char* kDictFemaleTalk = "ai_gestures@gen_female@standing@speaker";              // "hurry it along": annoyed_dismiss_r_001
static const char* kIntroDicts[] = {
	"amb_camp@world_camp_grimshaw_stare@wip_base@female_a", "ai_gestures@gen_female@standing@speaker",
	"mech_melee@unarmed@_male@_ambient@_healthy@_noncombat", "ai_react@point@base", "ai_react@male_stand@big_intro@forward",
	"ai_gestures@arthur@standing@speaker", "veh_horseback@seat_saddle@generic@terrain@unarmed@wind@idle@fidget@hat",
};

// ---------- the cast ----------
static void DressPed(Ped ped, Hash m, int i)
{
	// the riders in their own outfit (Rockstar's preset 0: how the mission scripts dress them to ride); the camp in its warm-weather
	// clothes, as the camp script does. (Without an outfit a spawned story ped is invisible.)
	const char* how = "random variation";
	bool named = false;
	if (kCast[i].role == ROLE_RIDER || kCast[i].role == ROLE_DOG)
	{
		PED::EQUIP_META_PED_OUTFIT_PRESET(ped, 0, FALSE);
		how = "preset 0"; named = true;
	}
	else if (PED::DOES_META_PED_OUTFIT_EXIST_FOR_PED_MODEL(Joaat("META_OUTFIT_WARM_WEATHER"), m))
	{
		PED::EQUIP_META_PED_OUTFIT(ped, Joaat("META_OUTFIT_WARM_WEATHER"));
		how = "META_OUTFIT_WARM_WEATHER"; named = true;
	}
	else
	{
		PED::EQUIP_META_PED_OUTFIT_PRESET(ped, 0, FALSE);
		how = "preset 0"; named = true;
	}
	PED::UPDATE_PED_VARIATION(ped, FALSE, TRUE, TRUE, TRUE, FALSE);
	g_in.cast[i].outfit = named;
	Log("INTRO cast %s (%s): ped %d, outfit %s", kCast[i].id, kCast[i].model, ped, how);
}

static void CalmPed(Ped ped)
{
	ENTITY::SET_ENTITY_AS_MISSION_ENTITY(ped, TRUE, TRUE);
	ENTITY::SET_ENTITY_INVINCIBLE(ped, TRUE);
	PED::SET_BLOCKING_OF_NON_TEMPORARY_EVENTS(ped, TRUE);
	PED::SET_PED_KEEP_TASK(ped, TRUE);
	static const int kCalm[] = { 113, 217, 17, 15, 26, 111, 130, 174, 286 };   // see cutscene_research.md 1.3
	for (int f : kCalm) PED::SET_PED_CONFIG_FLAG(ped, f, TRUE);
	PED::SET_PED_CONFIG_FLAG(ped, 168, FALSE);
	SetScripted(ped, true);
	ENTITY::SET_ENTITY_LOD_DIST(ped, 600);
}

static Ped SpawnCastAt(int i, const V3& at, float heading)
{
	const CastDef& d = kCast[i];
	Hash m = H(d.model);
	if (!LoadModel(m, 3000)) { Log("INTRO cast %s: model %s would not load", d.id, d.model); return 0; }
	Ped ped = PED::CREATE_PED(m, at.x, at.y, at.z, heading, FALSE, FALSE, FALSE, FALSE);
	STREAMING::SET_MODEL_AS_NO_LONGER_NEEDED(m);
	if (!ped) { Log("INTRO cast %s: CREATE_PED failed for %s", d.id, d.model); return 0; }
	DressPed(ped, m, i);
	CalmPed(ped);
	if (PED::IS_PED_HUMAN(ped)) PED::SET_PED_RELATIONSHIP_GROUP_HASH(ped, Joaat("REL_GANG_DUTCHS"));
	return ped;
}

static Ped SpawnHorse(int slot, const V3& at, float heading)
{
	Hash m = H(kRiderHorse[slot]);
	if (!LoadModel(m, 3000)) { Log("INTRO horse %s would not load", kRiderHorse[slot]); return 0; }
	Ped h = PED::CREATE_PED(m, at.x, at.y, at.z, heading, FALSE, FALSE, FALSE, FALSE);
	STREAMING::SET_MODEL_AS_NO_LONGER_NEEDED(m);
	if (!h) return 0;
	// dressed and saddled the way Rockstar's balloon mission does its gang horses (gang2.c)
	PED::EQUIP_META_PED_OUTFIT_PRESET(h, 0, FALSE);
	PED::EQUIP_META_PED_OUTFIT(h, Joaat("META_HORSE_SADDLE_ONLY"));
	PED::UPDATE_PED_VARIATION(h, FALSE, TRUE, TRUE, TRUE, FALSE);
	CalmPed(h);
	PED::SET_PED_RELATIONSHIP_GROUP_HASH(h, Joaat("REL_GANG_DUTCHS_HORSES"));
	return h;
}

// ---------- the places ----------
static V3 CampMark(int slot) { return OnGround(g_in.G + g_in.S * kCampK[slot], 0.05f); }
static V3 HorseCampMark(int slot) { return OnGround(g_in.G + g_in.X * 3.4f + g_in.S * (kCampK[slot] * 1.3f), 0.1f); }
static V3 HomeMark(int i) { return OnGround(g_in.G - g_in.X * 3.1f + g_in.S * (i == CA_SUSAN ? -1.7f : -0.45f), 0.05f); }
static V3 RidgeMark(int slot)
{
	V3 p = g_in.O + g_in.Sd * kRidgeK[slot];
	if (slot == RS_MICAH) p = p + g_in.D * 0.6f;       // a step nearer the edge
	if (slot == RS_ARTHUR) p = p - g_in.D * 0.3f;
	return OnGround(p, 0.05f);
}
static V3 HorseRidgeMark(int slot) { return OnGround(g_in.O - g_in.D * 7.5f + g_in.Sd * (kRidgeK[slot] * 1.7f), 0.1f); }

static bool GroundAt(float x, float y, float fromZ, float* z) { return MISC::GET_GROUND_Z_FOR_3D_COORD(x, y, fromZ, z, FALSE) != 0; }
static bool HasGround(const V3& p) { float z; return GroundAt(p.x, p.y, p.z + 60.0f, &z); }
static bool AreaReady(const V3& p)
{
	STREAMING::REQUEST_COLLISION_AT_COORD(p.x, p.y, p.z);
	return STREAMING::HAS_COLLISION_LOADED_AT_COORD(p.x, p.y, p.z) != 0;
}
static bool WaitArea(const V3& p, int ms)
{
	DWORD until = GetTickCount() + ms;
	while (GetTickCount() < until)
	{
		if (AreaReady(p) && HasGround(p)) return true;
		WAIT(0);
	}
	return false;
}

static std::string IntroSpotsPath() { return ModuleDir() + "\\TornadoRedemption_intro.txt"; }
static bool ReadSpots()
{
	FILE* f = nullptr;
	if (fopen_s(&f, IntroSpotsPath().c_str(), "r") != 0 || !f) return false;
	char line[600];
	bool ok = false;
	while (fgets(line, sizeof(line), f))
	{
		float a[24];
		if (sscanf_s(line, "spots4 %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f", &a[0], &a[1], &a[2], &a[3], &a[4], &a[5], &a[6],
			&a[7], &a[8], &a[9], &a[10], &a[11], &a[12], &a[13], &a[14], &a[15], &a[16], &a[17], &a[18], &a[19], &a[20]) == 21)
		{
			g_in.O = V3(a[0], a[1], a[2]); g_in.D = V3(a[3], a[4], 0);
			g_in.VR = V3(a[5], a[6], a[7]); g_in.VD = V3(a[8], a[9], 0);
			g_in.ranch = V3(a[10], a[11], a[12]);
			g_in.B = V3(0, 0, a[13]);   // (the balloon's ground height; x, y from O and D)
			g_in.valCam = V3(a[14], a[15], a[16]); g_in.valOff = a[17];
			g_in.arrCam = V3(a[18], a[19], a[20]);
			ok = true;
		}
	}
	fclose(f);
	return ok;
}
static void WriteSpots()
{
	FILE* f = nullptr;
	if (fopen_s(&f, IntroSpotsPath().c_str(), "w") != 0 || !f) return;
	fprintf(f, "# Tornado Redemption: where the intro's ride goes (found once in your game). Delete this file to look again.\n");
	fprintf(f, "# spots4 ridge x y z, toward the ranch dx dy, Valentine road x y z, along it dx dy, ranch x y z, balloon ground z,\n");
	fprintf(f, "#        Valentine camera x y z, riders' path offset, ridge-arrival camera x y z\n");
	fprintf(f, "spots4 %.2f %.2f %.2f %.4f %.4f %.2f %.2f %.2f %.4f %.4f %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f\n", g_in.O.x, g_in.O.y, g_in.O.z,
		g_in.D.x, g_in.D.y, g_in.VR.x, g_in.VR.y, g_in.VR.z, g_in.VD.x, g_in.VD.y, g_in.ranch.x, g_in.ranch.y, g_in.ranch.z, g_in.B.z,
		g_in.valCam.x, g_in.valCam.y, g_in.valCam.z, g_in.valOff, g_in.arrCam.x, g_in.arrCam.y, g_in.arrCam.z);
	fclose(f);
}

// v1.7.2: a clear line between two points (the map, objects, trees)?
static bool Los(const V3& a, const V3& b)
{
	int h = SHAPETEST::START_EXPENSIVE_SYNCHRONOUS_SHAPE_TEST_LOS_PROBE(a.x, a.y, a.z, b.x, b.y, b.z, 1 | 16 | 256, 0, 7);
	BOOL hit = FALSE; Vector3 end = {}, n = {}; Entity e = 0;
	SHAPETEST::GET_SHAPE_TEST_RESULT(h, &hit, &end, &n, &e);
	return !hit || (V3(end) - b).len() < 1.5f;
}

static void PutPlayer(const V3& p)
{
	Ped me = PLAYER::PLAYER_PED_ID();
	ENTITY::SET_ENTITY_COORDS(me, p.x, p.y, p.z, FALSE, FALSE, FALSE, FALSE);
}

// The road out of Valentine toward the ranch: the nearest road point 130 m out of town that way, and which way it runs.
static void SurveyValentine()
{
	V3 ve = FlatDir(kValentineC, kRanchC);
	V3 s = kValentineC + ve * 130.0f;
	PutPlayer(s + V3(0, 0, 3.0f));
	bool loaded = WaitArea(s, 4000);
	Vector3 node = {};
	float hd = 0;
	bool road = false;
	for (int type : { 1, 0, 3 })   // (v1.7.1: playtest 19 found no node of type 1)
		if (!road) road = PATH::GET_CLOSEST_VEHICLE_NODE_WITH_HEADING(s.x, s.y, s.z, &node, &hd, type, 3.0f, 0.0f) != 0 && (V3(node) - s).len2d() < 120.0f;
	if (road)
	{
		g_in.VR = V3(node);
		g_in.VD = HeadingDir(hd);
		if (g_in.VD.x * ve.x + g_in.VD.y * ve.y < 0) g_in.VD = g_in.VD * -1.0f;   // away from town
	}
	else
	{
		g_in.VR = s;
		g_in.VD = ve;
	}
	g_in.VR.z = GroundZ(g_in.VR.x, g_in.VR.y, g_in.VR.z + 50.0f, g_in.VR.z);
	g_in.VS = V3(g_in.VD.y, -g_in.VD.x, 0);
	// v1.7.2 (playtest 20: a tree in the way; the town out of sight): the riders' path off the road's line if that's clearer, and a
	// camera ahead of them that sees both the riders and the town behind them
	g_in.valOff = 0;
	for (float o : { 0.0f, 3.0f, -3.0f, 6.0f, -6.0f, 9.0f, -9.0f })
		if (Los(OnGround(g_in.VR - g_in.VD * 24.0f + g_in.VS * o, 1.6f), OnGround(g_in.VR + g_in.VD * 40.0f + g_in.VS * o, 1.6f))) { g_in.valOff = o; break; }
	V3 path = OnGround(g_in.VR + g_in.VS * g_in.valOff, 1.6f), town = kValentineC + V3(0, 0, 8.0f);
	bool both = false, any = false;
	for (float along : { 12.0f, 16.0f, 9.0f, 20.0f })
	{
		for (float side : { 6.0f, -6.0f, 9.0f, -9.0f, 4.5f, -4.5f })
			for (float up : { 1.7f, 3.2f })
			{
				V3 c = OnGround(g_in.VR + g_in.VD * along + g_in.VS * (g_in.valOff + side), up);
				if (!Los(c, path)) continue;
				if (!any) { g_in.valCam = c; any = true; }
				if (Los(c, town)) { g_in.valCam = c; both = true; break; }
			}
		if (both) break;
	}
	if (!any) g_in.valCam = OnGround(g_in.VR + g_in.VD * 10.0f + g_in.VS * (g_in.valOff + 6.0f), 2.0f);
	Log("INTRO survey: the Valentine camera sees %s; the riders' path %.0f m off the road's line", both ? "the riders and the town" : any ? "the riders (not the town)" : "nothing clear", g_in.valOff);
	Log("INTRO survey: the road out of Valentine at (%.1f, %.1f, %.1f) heading %.0f (%s, ground %s)", g_in.VR.x, g_in.VR.y, g_in.VR.z,
		HeadingTo(V3(), g_in.VD), road ? "a road node" : "NO road node - straight out of town", loaded ? "loaded" : "NOT loaded");
}

// The ridge: somewhere 100-250 m from the ranch, higher than it, with a drop in front toward it, room to stand on top, room for the
// balloon behind, and a clear view of the ranch.
static void SurveyRidge()
{
	V3 r0 = kRanchC;
	PutPlayer(r0 + V3(0, 0, 3.0f));
	bool loaded = WaitArea(r0, 5000);
	float rz = r0.z;
	GroundAt(r0.x, r0.y, r0.z + 80.0f, &rz);
	g_in.ranch = V3(r0.x, r0.y, rz);
	struct Cand { V3 p, d; float score, elev, drop; };
	Cand best{};
	best.score = -1e9f;
	Cand loose{};
	loose.score = -1e9f;
	int tried = 0, grounded = 0, kept = 0;
	for (float r = 100.0f; r <= 250.0f; r += 15.0f)
		for (int a = 0; a < 40; a++)
		{
			float ang = a * 2.0f * PI / 40.0f;
			V3 p = g_in.ranch + V3(cosf(ang), sinf(ang), 0) * r;
			tried++;
			float z;
			if (!GroundAt(p.x, p.y, rz + 220.0f, &z)) continue;
			grounded++;
			p.z = z;
			float wh = 0;
			if (WATER::GET_WATER_HEIGHT(p.x, p.y, z + 2.0f, &wh) && wh > z - 0.3f) continue;   // (in the river)
			V3 d = FlatDir(p, g_in.ranch), s(d.y, -d.x, 0);
			float z6, z12, z20, zl, zr, zb, zbal;
			if (!GroundAt(p.x + d.x * 6, p.y + d.y * 6, z + 60, &z6) || !GroundAt(p.x + d.x * 12, p.y + d.y * 12, z + 60, &z12) ||
				!GroundAt(p.x + d.x * 20, p.y + d.y * 20, z + 60, &z20)) continue;
			if (!GroundAt(p.x + s.x * 3.2f, p.y + s.y * 3.2f, z + 30, &zl) || !GroundAt(p.x - s.x * 3.2f, p.y - s.y * 3.2f, z + 30, &zr) ||
				!GroundAt(p.x - d.x * 4, p.y - d.y * 4, z + 30, &zb) || !GroundAt(p.x - d.x * 26, p.y - d.y * 26, z + 40, &zbal)) continue;
			float drop = z - std::min(z6, std::min(z12, z20));
			float flat = std::max(fabsf(zl - z), std::max(fabsf(zr - z), fabsf(zb - z)));
			float elev = z - rz;
			// (v1.7.1: playtest 19's pick had a 2.4 m drop - a slope, not a ridge: the drop counts for more now)
			float score = elev * 0.7f + std::min(drop, 30.0f) * 2.6f - fabsf(r - 160.0f) * 0.04f - flat * 2.0f - fabsf(zbal - z) * 0.2f;
			if (flat < 3.0f && score > loose.score) loose = { p, d, score, elev, drop };
			if (flat > 1.6f || elev < 3.0f || drop < 4.0f) continue;
			// can they see the ranch from up here? (the map, objects and trees)
			int h = SHAPETEST::START_EXPENSIVE_SYNCHRONOUS_SHAPE_TEST_LOS_PROBE(p.x, p.y, p.z + 1.7f, g_in.ranch.x, g_in.ranch.y, g_in.ranch.z + 6.0f, 1 | 16 | 256, 0, 7);
			BOOL hit = FALSE; Vector3 end = {}, n = {}; Entity e = 0;
			SHAPETEST::GET_SHAPE_TEST_RESULT(h, &hit, &end, &n, &e);
			if (hit && (V3(end) - g_in.ranch).len2d() > 25.0f) continue;
			// (v1.7.1: and a clear way in - the ride up from 16 m back - and out, to the balloon 26 m back: no tree in either)
			auto clearRun = [&](float back)
			{
				V3 o(p.x, p.y, p.z + 1.4f), e(p.x - d.x * back, p.y - d.y * back, 0);
				e.z = GroundZ(e.x, e.y, z + 30.0f, z) + 1.4f;
				int hh = SHAPETEST::START_EXPENSIVE_SYNCHRONOUS_SHAPE_TEST_LOS_PROBE(o.x, o.y, o.z, e.x, e.y, e.z, 1 | 16 | 256, 0, 7);
				BOOL h2 = FALSE; Vector3 e2 = {}, n2 = {}; Entity en2 = 0;
				SHAPETEST::GET_SHAPE_TEST_RESULT(hh, &h2, &e2, &n2, &en2);
				return !h2;
			};
			if (!clearRun(16.0f) || !clearRun(26.0f)) continue;
			kept++;
			if (score > best.score) best = { p, d, score, elev, drop };
		}
	const char* how = "the best";
	if (best.score < -1e8f) { best = loose; how = "nothing ideal - the highest flat-ish spot"; }
	if (best.score < -1e8f)
	{
		best.d = FlatDir(g_in.ranch + V3(-140.0f, 0, 0), g_in.ranch);
		best.p = g_in.ranch + V3(-140.0f, 0, 0);
		best.p.z = GroundZ(best.p.x, best.p.y, rz + 150.0f, rz);
		how = "NOTHING found - 140 m west of the ranch";
	}
	g_in.O = best.p; g_in.D = best.d;
	g_in.B = g_in.O - g_in.D * 26.0f;
	g_in.B.z = GroundZ(g_in.B.x, g_in.B.y, g_in.O.z + 40.0f, g_in.O.z);
	// v1.7.2 (playtest 20: the arrival's camera couldn't see and went up and away): behind the ridge, seeing the riders' marks and the
	// ranch below
	{
		V3 sd(g_in.D.y, -g_in.D.x, 0);
		bool found = false;
		for (float back : { 16.0f, 20.0f, 13.0f, 24.0f })
		{
			for (float side : { 2.5f, -2.5f, 5.0f, -5.0f, 0.0f })
				for (float up : { 3.0f, 4.5f, 6.5f })
				{
					V3 c = OnGround(g_in.O - g_in.D * back + sd * side, up);
					if (Los(c, g_in.O + V3(0, 0, 1.6f)) && Los(c, g_in.ranch + V3(0, 0, 5.0f))) { g_in.arrCam = c; found = true; break; }
				}
			if (found) break;
		}
		if (!found) g_in.arrCam = OnGround(g_in.O - g_in.D * 16.0f + sd * 2.5f, 4.5f);
		Log("INTRO survey: the ridge-arrival camera %s", found ? "sees the riders and the ranch" : "found nothing clear - a default");
	}
	Log("INTRO survey: the ridge at (%.1f, %.1f, %.1f), %.0f m from the ranch, %.1f m above it, %.1f m drop in front (%s; %d spots, %d on the ground, %d good; ground %s)",
		g_in.O.x, g_in.O.y, g_in.O.z, (g_in.O - g_in.ranch).len2d(), best.elev, best.drop, how, tried, grounded, kept, loaded ? "loaded" : "NOT loaded");
}

// v1.7.1 (playtest 19: a rider hit a tree on the way out): the way out of camp with the longest clear run, nearest the muster's
static V3 ClearWayOut()
{
	static const float kYaw[] = { 0, 15, -15, 30, -30, 45, -45, 60, -60, 80, -80, 100, -100 };
	V3 best = g_in.X; float bestFree = -1;
	for (float y : kYaw)
	{
		float a = y * PI / 180.0f;
		V3 d(g_in.X.x * cosf(a) - g_in.X.y * sinf(a), g_in.X.x * sinf(a) + g_in.X.y * cosf(a), 0);
		float free = 0;
		for (float side : { -1.2f, 0.0f, 1.2f })   // (a horse is wide: three rays)
		{
			V3 o = OnGround(g_in.G + d * 3.0f + V3(d.y, -d.x, 0) * side, 1.5f), e = OnGround(g_in.G + d * 40.0f + V3(d.y, -d.x, 0) * side, 1.5f);
			int h = SHAPETEST::START_EXPENSIVE_SYNCHRONOUS_SHAPE_TEST_LOS_PROBE(o.x, o.y, o.z, e.x, e.y, e.z, 1 | 16 | 256, 0, 7);
			BOOL hit = FALSE; Vector3 end = {}, n = {}; Entity en = 0;
			SHAPETEST::GET_SHAPE_TEST_RESULT(h, &hit, &end, &n, &en);
			float f = hit ? (V3(end) - o).len() : 37.0f;
			free = side == -1.2f ? f : std::min(free, f);
		}
		if (free > bestFree + 3.0f) { bestFree = free; best = d; }
		if (free >= 36.0f) break;
	}
	Log("INTRO the way out of camp: %.0f deg off the muster's, %.0f m clear", acosf(Clamp(best.x * g_in.X.x + best.y * g_in.X.y, -1.0f, 1.0f)) * 180.0f / PI, bestFree);
	return best;
}

// ---------- the camp ----------
static bool IplOn(int h) { return STREAMING::IS_IPL_ACTIVE_HASH((Hash)h) != 0; }
// v1.7.1 (playtest 19 crashed 2.5 s after they were switched off mid-scene, the camp's people deleted the same moment): the pieces
// the scene switched on are switched off only once the scene is over and Arthur is well away from the camp
static std::vector<int> g_iplPending;
static bool g_abandonedPending = false;
static float g_iplCheckAt = 0;
static void CampMapOn()
{
	// pieces an earlier scene left on (not put back yet) are this scene's again
	if (!g_iplPending.empty() || g_abandonedPending)
	{
		g_in.iplOn = g_iplPending; g_in.abandonedWas = g_abandonedPending;
		g_iplPending.clear(); g_abandonedPending = false;
		for (int h : g_in.iplOn) if (!IplOn(h)) STREAMING::REQUEST_IPL_HASH((Hash)h);
		Log("INTRO camp: %d camp pieces still up from the last time - kept", (int)g_in.iplOn.size());
		return;
	}
	// a chapter 2 save has the real camp up already: then it's left alone (and its people are hidden while ours are there)
	g_in.realCamp = IplOn(kCampBaseIpl) || IplOn(kCampBaseAlt[0]) || IplOn(kCampBaseAlt[1]);
	std::string mode = IniUpper("Intro", "CampMap", "ALL");
	if (g_in.realCamp) { g_in.campMap = true; Log("INTRO camp: the gang's real camp is up here (your save is in chapter 2) - using it as it is"); return; }
	if (mode == "NONE") { Log("INTRO camp: [Intro] CampMap=None - the mod's own props"); return; }
	g_in.abandonedWas = IplOn(kCampAbandonedIpl);
	if (g_in.abandonedWas) STREAMING::REMOVE_IPL_HASH((Hash)kCampAbandonedIpl);
	int n = 0;
	for (int h : kCampIpls)
	{
		if (mode == "BASE" && h != kCampBaseIpl && h != 825179479) continue;
		if (IplOn(h)) continue;
		STREAMING::REQUEST_IPL_HASH((Hash)h);
		g_in.iplOn.push_back(h);
		n++;
	}
	Log("INTRO camp: asked for %d of Rockstar's camp pieces (%s)%s", n, mode.c_str(), g_in.abandonedWas ? ", the abandoned camp put away" : "");
}
static void CampMapOff(const char* why)
{
	if (g_in.campGone) return;
	g_in.campGone = true;
	for (int h : g_in.iplOn) g_iplPending.push_back(h);
	g_abandonedPending = g_abandonedPending || g_in.abandonedWas;
	if (!g_in.iplOn.empty()) Log("INTRO camp: %d camp pieces to put back once the scene's over and Arthur is away (%s)", (int)g_in.iplOn.size(), why);
	g_in.iplOn.clear();
	g_in.abandonedWas = false;
	for (auto& h : g_in.hidden)
		if (h.p && ENTITY::DOES_ENTITY_EXIST(h.p))
		{
			ENTITY::SET_ENTITY_VISIBLE(h.p, TRUE);
			ENTITY::SET_ENTITY_COLLISION(h.p, TRUE, TRUE);
			ENTITY::FREEZE_ENTITY_POSITION(h.p, FALSE);
		}
	g_in.hidden.clear();
}
// once a second: the camp pieces go back the way they were once the scene and its run are over and Arthur is 350 m or more away
static void CampMapHousekeeping()
{
	if (g_iplPending.empty() && !g_abandonedPending) return;
	float now = NowSec();
	if (now < g_iplCheckAt) return;
	g_iplCheckAt = now + 1.0f;
	if (g_in.stage != 0 || (PlayerPos() - kCampCentre).len2d() < 350.0f) return;   // (not even during the run for the balloon)
	for (int h : g_iplPending) if (IplOn(h)) STREAMING::REMOVE_IPL_HASH((Hash)h);
	if (g_abandonedPending && !IplOn(kCampAbandonedIpl)) STREAMING::REQUEST_IPL_HASH((Hash)kCampAbandonedIpl);
	Log("INTRO camp: Rockstar's camp pieces put back the way they were (%d)", (int)g_iplPending.size());
	g_iplPending.clear();
	g_abandonedPending = false;
}

// the real camp's people who'd be there twice (chapter 2 saves): out of sight while ours are
static void HideDoubles()
{
	static Ped peds[1024];
	int n = worldGetAllPeds(peds, 1024);
	for (int k = 0; k < n; k++)
	{
		Ped p = peds[k];
		if (!p || p == PLAYER::PLAYER_PED_ID() || (V3(ENTITY::GET_ENTITY_COORDS(p, FALSE, FALSE)) - kCampCentre).len2d() > 90.0f) continue;
		bool ours = false;
		for (auto& c : g_in.cast) if (c.ped == p) ours = true;
		if (ours) continue;
		Hash m = ENTITY::GET_ENTITY_MODEL(p);
		for (int i = 0; i < kCastCount; i++)
			if (m == H(kCast[i].model) && kCast[i].role != ROLE_DOG)
			{
				ENTITY::SET_ENTITY_VISIBLE(p, FALSE);
				ENTITY::SET_ENTITY_COLLISION(p, FALSE, FALSE);
				ENTITY::FREEZE_ENTITY_POSITION(p, TRUE);
				g_in.hidden.push_back({ p });
				break;
			}
	}
	if (!g_in.hidden.empty()) Log("INTRO camp: %d of the real camp's people hidden while ours are there", (int)g_in.hidden.size());
}

static void BuildFallbackProps()
{
	int ok = 0;
	for (int i = 0; i < kCampPropCount; i++)
	{
		const PropDef& d = kCampProps[i];
		const char* name = d.model;
		Hash m = H(name);
		if (!LoadModel(m, 1500) && d.alt) { name = d.alt; m = H(name); if (!LoadModel(m, 1500)) m = 0; }
		else if (!STREAMING::HAS_MODEL_LOADED(m)) m = 0;
		if (!m) continue;
		V3 p = OnGround(kCampCentre + g_in.S * d.x + g_in.X * d.y, 0.0f);
		Object o = OBJECT::CREATE_OBJECT(m, p.x, p.y, p.z, FALSE, FALSE, TRUE, FALSE, FALSE);
		STREAMING::SET_MODEL_AS_NO_LONGER_NEEDED(m);
		if (!o) continue;
		ENTITY::SET_ENTITY_HEADING(o, HeadingTo(V3(), g_in.X) + d.heading);
		OBJECT::PLACE_OBJECT_ON_GROUND_PROPERLY(o, FALSE);
		ENTITY::FREEZE_ENTITY_POSITION(o, TRUE);
		SetScripted(o, true);
		g_in.props.push_back(o);
		ok++;
	}
	Log("INTRO camp: Rockstar's camp didn't come up - %d props of our own", ok);
}

static void CampFire(const V3& at)
{
	if (!LoadPtfxAsset("core", 1000)) return;
	V3 f = OnGround(at, 0.15f);
	GRAPHICS::USE_PARTICLE_FX_ASSET("core");
	int h = GRAPHICS::START_PARTICLE_FX_LOOPED_AT_COORD("ent_amb_campfire_sma", f.x, f.y, f.z, 0, 0, 0, 1.0f, FALSE, FALSE, FALSE, FALSE);
	if (h) { g_introFx.push_back(h); AddLoopsInUse(1); }
}
static void StopFx()
{
	for (int h : g_introFx)
		if (h && GRAPHICS::DOES_PARTICLE_FX_LOOPED_EXIST(h)) { GRAPHICS::STOP_PARTICLE_FX_LOOPED(h, FALSE); GRAPHICS::REMOVE_PARTICLE_FX(h, FALSE); AddLoopsInUse(-1); }
	g_introFx.clear();
}

// a camp extra: on Rockstar's nearest camp spot if there is one (their chores, their seats), else the scenario named, else a stroll
static void StartCampLife(int i)
{
	Ped p = CastPed(i);
	if (!p) return;
	const CastDef& d = kCast[i];
	V3 at = OnGround(V3(d.x, d.y, kCampCentre.z), 0.05f);
	if (d.wander)
	{
		TASK::TASK_WANDER_IN_AREA(p, at.x, at.y, at.z, 5.0f, 2.0f, 6.0f, 0);   // (deep in camp, away from the four)
		return;
	}
	TASK::TASK_USE_NEAREST_SCENARIO_TO_COORD_WARP(p, at.x, at.y, at.z, 5.0f, -1, FALSE, FALSE, FALSE, FALSE);
}
static void CheckCampLife()
{
	for (int i = 0; i < kCastCount; i++)
	{
		const CastDef& d = kCast[i];
		Ped p = CastPed(i);
		if (!p || d.role != ROLE_CAMP || d.wander) continue;
		g_in.cast[i].scenario = PED::IS_PED_USING_ANY_SCENARIO(p) != 0;
		if (!g_in.cast[i].scenario && d.scenario)
			TASK::TASK_START_SCENARIO_IN_PLACE_HASH(p, Joaat(d.scenario), -1, FALSE, 0, -1.0f, FALSE);
		Log("INTRO %s: %s", d.id, g_in.cast[i].scenario ? "on one of Rockstar's camp spots" : d.scenario ? d.scenario : "standing");
	}
}

// ---------- start, build ----------
static void IntroCleanupCams()
{
	if (g_in.cam && CAMERA::DOES_CAM_EXIST(g_in.cam)) { CAMERA::SET_CAM_ACTIVE(g_in.cam, FALSE); CAMERA::DESTROY_CAM(g_in.cam, FALSE); }
	g_in.cam = 0;
}

static void IntroStoryEnd();
static void IntroRestoreLook()
{
	MISC::SET_TIME_SCALE(1.0f);
	IntroStoryEnd();
	HUD::DISPLAY_HUD(TRUE);
	MAP::DISPLAY_RADAR(TRUE);
}

// the scene locked its own skies - give the weather back to whoever owned it
static void IntroRestoreWeather()
{
	g_manualWeather = g_in.savedManual;
	if (g_in.savedManual && g_in.savedLock)
	{
		MISC::SET_WEATHER_TYPE_FROZEN(FALSE);
		MISC::SET_WEATHER_TYPE(g_in.savedLock, TRUE, TRUE, TRUE, 4.0f, FALSE);
		MISC::SET_OVERRIDE_WEATHER(g_in.savedLock);
		MISC::SET_WEATHER_TYPE_FROZEN(TRUE);
		g_lockedHash = g_in.savedLock;
		Log("INTRO weather: your locked weather is back");
	}
	else if (g_set.weatherMode == 0)
	{
		UnlockWeather();
		g_storm.lockedByStorm = false;
	}
	else
	{
		g_storm.lockedByStorm = true;   // the tornado's storm owns it now, and lets go when the storm's over
		g_storm.appliedWeather = -1;
	}
	MISC::SET_RAIN(-1.0f);
	g_storm.rainHeld = false;
	CLOCK::PAUSE_CLOCK(FALSE, 0);
}

static void IntroReleaseHorses()
{
	for (Ped& h : g_in.realHorse)
	{
		if (h) SetScripted(h, false);
		if (h && ENTITY::DOES_ENTITY_EXIST(h)) ENTITY::SET_ENTITY_INVINCIBLE(h, FALSE);
		h = 0;
	}
}

static void IntroDeletePed(Ped& p);
// After the scene (or an abort): what's left belongs to the world now - the game tidies it up when you're away.
static void IntroReleaseCast()
{
	auto letGo = [](Ped& p)
	{
		if (!p || !ENTITY::DOES_ENTITY_EXIST(p)) { p = 0; return; }
		SetScripted(p, false);
		ENTITY::FREEZE_ENTITY_POSITION(p, FALSE);
		PED::SET_BLOCKING_OF_NON_TEMPORARY_EVENTS(p, FALSE);
		Ped q = p;
		g_introLeft.push_back({ q, ENTITY::GET_ENTITY_MODEL(q), true });
		ENTITY::SET_PED_AS_NO_LONGER_NEEDED(&q);
		p = 0;
	};
	for (auto& c : g_in.cast) { letGo(c.ped); c = CastState(); }
	for (Ped& h : g_in.horses) letGo(h);
	for (Ped& r : g_in.ranchLife) letGo(r);
	g_in.ranchLife.clear();
	for (Entity e : g_in.props)
		if (e && ENTITY::DOES_ENTITY_EXIST(e))
		{
			SetScripted(e, false);
			g_introLeft.push_back({ e, ENTITY::GET_ENTITY_MODEL(e), false });
			Entity x = e;
			ENTITY::SET_ENTITY_AS_NO_LONGER_NEEDED(&x);
		}
	g_in.props.clear();
	if (g_introLeft.size() > 120) g_introLeft.erase(g_introLeft.begin(), g_introLeft.end() - 120);
	StopFx();
}

static void IntroDeletePed(Ped& p)
{
	if (p && ENTITY::DOES_ENTITY_EXIST(p)) { SetScripted(p, false); Ped q = p; ENTITY::SET_ENTITY_AS_MISSION_ENTITY(q, TRUE, TRUE); PED::DELETE_PED(&q); }
	p = 0;
}

void IntroRestoreSettings()
{
	g_set.arthur = g_in.savedArthur;
	g_set.playerGod = g_in.savedGod;
	g_set.movement = g_in.savedMove;
	g_set.speed = g_in.savedSpeed;
	g_set.touchdownCam = g_in.savedTouchCam;
	g_set.rideCam = g_in.savedRideCam;
	g_set.grabPeople = g_in.savedPeople;
	g_set.grabAnimals = g_in.savedAnimals;
	g_holdStorm = false;
	g_shieldPlayer = false;
}

static void IntroReleaseDicts() { for (const char* d : kIntroDicts) STREAMING::REMOVE_ANIM_DICT(d); }

static void IntroRemoveBlip()
{
	if (g_in.blip) { Blip b = g_in.blip; if (MAP::DOES_BLIP_EXIST(b)) MAP::REMOVE_BLIP(&b); g_in.blip = 0; }
}

// Abort at any point (death, despawn, script restart): put the player and the camera back, leave the world as it is.
static void IntroAbort(const char* why, bool releaseCast = true)
{
	if (!g_in.stage) return;
	if (Tornado* tp = ITp()) tp->scripted = false;
	IntroReleaseHorses();
	if (g_in.stage >= 2) IntroReleaseDicts();
	if (g_in.oldCam && CAMERA::DOES_CAM_EXIST(g_in.oldCam)) CAMERA::DESTROY_CAM(g_in.oldCam, FALSE);
	g_in.oldCam = 0;
	if (g_in.loadScene) { STREAMING::LOAD_SCENE_STOP(); g_in.loadScene = false; }
	if (g_in.dip && CAMERA::IS_SCREEN_FADED_OUT()) CAMERA::DO_SCREEN_FADE_IN(300);
	for (Ped h : g_in.horses) if (h && ENTITY::DOES_ENTITY_EXIST(h)) ENTITY::FREEZE_ENTITY_POSITION(h, FALSE);
	g_in.dip = 0;
	if (g_bal.on && g_bal.waiting) BalloonRemove("intro ended early");   // no orphan balloon
	IntroRemoveBlip();
	CampMapOff(why);
	Ped me = PLAYER::PLAYER_PED_ID();
	if (g_in.stage <= 4)
	{
		IntroCleanupCams();
		CAMERA::RENDER_SCRIPT_CAMS(FALSE, FALSE, 0, TRUE, FALSE, 0);
		if (CAMERA::IS_SCREEN_FADED_OUT()) CAMERA::DO_SCREEN_FADE_IN(500);
		PLAYER::SET_PLAYER_CONTROL(PLAYER::PLAYER_ID(), TRUE, 0, FALSE);
		ENTITY::FREEZE_ENTITY_POSITION(me, FALSE);
		ENTITY::SET_ENTITY_VISIBLE(me, TRUE);
		PED::SET_PED_CAN_RAGDOLL(me, TRUE);
		if (!(g_bal.on && BalloonAboard()) && !PlayerDead())
		{
			if (PED::IS_PED_ON_MOUNT(me)) PED::REMOVE_PED_FROM_MOUNT(me, TRUE, FALSE);
			TASK::CLEAR_PED_TASKS(me, TRUE, FALSE);
		}
		IntroRestoreLook();
		IntroRestoreWeather();
	}
	else if (!g_in.boarded) IntroRestoreWeather();   // (the run for the balloon still held the scene's sky)
	IntroRestoreSettings();
	if (releaseCast) IntroReleaseCast();
	Log("INTRO ended (%s) at %.1f s", why, (float)g_in.clock);
	g_in.stage = 0;
	UI::Letterbox(0);
}

// Despawn everything: the cast and anything left from an earlier scene go too.
static void IntroRemoveAll()
{
	IntroAbort("cleared", false);
	for (auto& c : g_in.cast) { IntroDeletePed(c.ped); c = CastState(); }
	for (Ped& h : g_in.horses) IntroDeletePed(h);
	for (Ped& r : g_in.ranchLife) IntroDeletePed(r);
	g_in.ranchLife.clear();
	for (Entity e : g_in.props)
		if (e && ENTITY::DOES_ENTITY_EXIST(e)) { SetScripted(e, false); Object o = e; DeleteObj(o); }
	g_in.props.clear();
	StopFx();
	if (!g_introLeft.empty())
	{
		// never Arthur or his horse, never something that isn't ours now
		Ped me = PLAYER::PLAYER_PED_ID();
		Ped horse = PLAYER::GET_SADDLE_HORSE_FOR_PLAYER(PLAYER::PLAYER_ID()), mount = PED::IS_PED_ON_MOUNT(me) ? PED::GET_MOUNT(me) : PED::GET_LAST_MOUNT(me);
		int gone = 0;
		for (auto& l : g_introLeft)
		{
			if (!l.e || l.e == me || l.e == horse || l.e == mount || !ENTITY::DOES_ENTITY_EXIST(l.e) || ENTITY::GET_ENTITY_MODEL(l.e) != l.model) continue;
			SetScripted(l.e, false);
			ENTITY::SET_ENTITY_AS_MISSION_ENTITY(l.e, TRUE, TRUE);
			if (l.ped) { Ped p = l.e; PED::DELETE_PED(&p); }
			else { Object o = l.e; DeleteObj(o); }
			gone++;
		}
		Log("INTRO cleared %d of the %d things earlier scenes left in the world", gone, (int)g_introLeft.size());
		g_introLeft.clear();
	}
}

static void IntroStart()
{
	if (IntroRunning()) { Notify("The intro is already running"); return; }
	if (PlayerDead()) return;
	if (MISC::GET_MISSION_FLAG()) { Notify("Not during a story mission - finish or leave it first"); return; }
	AutoStop("replaced");
	SurvivalStop("STOPPED");
	if (g_bal.chase) ChaseEnd("STOPPED");
	BalloonRemove("intro");
	DespawnAll();
	IntroRemoveAll();
	g_menuOpen = false;
	g_drone.mode = 0;
	DroneRelease(false);
	TouchCamStop(false);
	RideCamStop(false);
	g_in = IntroState();
	g_in.savedArthur = g_set.arthur; g_in.savedGod = g_set.playerGod; g_in.savedMove = g_set.movement; g_in.savedSpeed = g_set.speed;
	g_in.savedTouchCam = g_set.touchdownCam; g_in.savedRideCam = g_set.rideCam;
	g_in.savedPeople = g_set.grabPeople; g_in.savedAnimals = g_set.grabAnimals;
	g_in.savedLock = g_lockedHash;
	g_in.savedManual = g_manualWeather;
	g_in.stage = 1;
	g_in.stageAt = NowSec();
	CAMERA::DO_SCREEN_FADE_OUT(700);
	Log("INTRO start (Storm Chasers): fading out, %d lines in the characters' own voices", kIntroLineCount);
}

// Built while the screen is black: the places (found once), the camp, the gang and their horses, the morning.
static void IntroBuild()
{
	Ped me = PLAYER::PLAYER_PED_ID();
	DWORD t0 = GetTickCount();
	// Arthur's own horse stays where it is, out of the tornado's hands
	Ped own[2] = { PLAYER::GET_SADDLE_HORSE_FOR_PLAYER(PLAYER::PLAYER_ID()), PED::IS_PED_ON_MOUNT(me) ? PED::GET_MOUNT(me) : PED::GET_LAST_MOUNT(me) };
	if (PED::IS_PED_ON_MOUNT(me)) PED::REMOVE_PED_FROM_MOUNT(me, TRUE, FALSE);
	if (PED::IS_PED_ON_MOUNT(me) || PED::IS_PED_IN_ANY_VEHICLE(me, FALSE)) TASK::CLEAR_PED_TASKS_IMMEDIATELY(me, FALSE, TRUE);
	for (int k = 0; k < 2; k++)
	{
		Ped h = own[k];
		if (!h || !ENTITY::DOES_ENTITY_EXIST(h) || (k == 1 && h == g_in.realHorse[0])) continue;
		SetScripted(h, true);
		ENTITY::SET_ENTITY_INVINCIBLE(h, TRUE);
		g_in.realHorse[k] = h;
	}
	PLAYER::SET_PLAYER_CONTROL(PLAYER::PLAYER_ID(), FALSE, 0, FALSE);
	ENTITY::SET_ENTITY_VISIBLE(me, FALSE);
	ENTITY::FREEZE_ENTITY_POSITION(me, TRUE);
	PED::SET_PED_CAN_RAGDOLL(me, FALSE);
	HUD::DISPLAY_HUD(FALSE);
	MAP::DISPLAY_RADAR(FALSE);
	// the tornado's settings for the scene
	g_set.arthur = 0;
	g_set.grabPeople = g_set.grabAnimals = true;
	g_set.playerGod = true;
	g_set.touchdownCam = false;
	g_set.rideCam = false;
	g_holdStorm = true;
	g_shieldPlayer = true;
	// ask for everything at once (the camp's people, the horses, the balloon, the clips), then go and look at the places
	for (const char* d : kIntroDicts) if (STREAMING::DOES_ANIM_DICT_EXIST(d)) STREAMING::REQUEST_ANIM_DICT(d);
	for (auto& c : kCast) STREAMING::REQUEST_MODEL(H(c.model), FALSE);
	for (const char* h : kRiderHorse) STREAMING::REQUEST_MODEL(H(h), FALSE);
	for (const char* m : kRanchLife) STREAMING::REQUEST_MODEL(H(m), FALSE);
	STREAMING::REQUEST_MODEL(H(kBalloonVehicle), FALSE);
	STREAMING::REQUEST_NAMED_PTFX_ASSET(H(kJetDict));
	for (int i = 0; i < kIntroLineCount; i++)
		if (kIntroLines[i].block && kIntroLines[i].t < 14.0f) { HUD::TEXT_BLOCK_REQUEST(kIntroLines[i].block); g_in.blockReq[i] = true; }
	AUDIO::CLEAR_CONVERSATION_HISTORY();
	AUDIO::SET_AUDIO_FLAG("DisableAbortConversationForDeathAndInjury", TRUE);
	// the places: found once, then remembered
	g_in.surveyed = ReadSpots();
	if (!g_in.surveyed)
	{
		SurveyValentine();
		SurveyRidge();
		WriteSpots();
		Log("INTRO survey done in %u ms (remembered in %s)", GetTickCount() - t0, IntroSpotsPath().c_str());
	}
	else
	{
		g_in.B = V3(g_in.O.x - g_in.D.x * 26.0f, g_in.O.y - g_in.D.y * 26.0f, g_in.B.z);
		Log("INTRO places from %s: the ridge (%.1f, %.1f, %.1f), %.0f m from the ranch", IntroSpotsPath().c_str(), g_in.O.x, g_in.O.y, g_in.O.z,
			(g_in.O - g_in.ranch).len2d());
	}
	g_in.Sd = V3(g_in.D.y, -g_in.D.x, 0);
	g_in.VS = V3(g_in.VD.y, -g_in.VD.x, 0);
	// the camp: the four meet their horses at Rockstar's muster spot, on the way out
	g_in.X = FlatDir(kCampCentre, kCampMuster);
	g_in.S = V3(g_in.X.y, -g_in.X.x, 0);
	g_in.G = kCampMuster;
	PutPlayer(g_in.G + V3(0, 0, 2.0f));
	CampMapOn();
	bool campLoaded = WaitArea(g_in.G, 5000);
	g_in.G.z = GroundZ(g_in.G.x, g_in.G.y, g_in.G.z + 30.0f, g_in.G.z);
	// the morning: sunny, eight o'clock, the clock held still for the scene
	LockWeather("SUNNY", 0.0f);
	MISC::SET_RAIN(0.0f);
	g_storm.rainHeld = true;
	CLOCK::SET_CLOCK_TIME(8, 0, 0);
	CLOCK::PAUSE_CLOCK(TRUE, 0);
	// wait (a little) for the people, the horses and the camp
	{
		DWORD until = GetTickCount() + 5000;
		bool all = false;
		while (GetTickCount() < until)
		{
			all = STREAMING::HAS_MODEL_LOADED(H(kBalloonVehicle));
			for (auto& c : kCast) all = all && STREAMING::HAS_MODEL_LOADED(H(c.model));
			for (const char* h : kRiderHorse) all = all && STREAMING::HAS_MODEL_LOADED(H(h));
			for (const char* d : kIntroDicts) if (STREAMING::DOES_ANIM_DICT_EXIST(d)) all = all && STREAMING::HAS_ANIM_DICT_LOADED(d);
			if (all && (g_in.realCamp || g_in.iplOn.empty() || IplOn(kCampBaseIpl))) break;
			WAIT(0);
		}
		std::string missing;
		for (auto& c : kCast) if (!STREAMING::HAS_MODEL_LOADED(H(c.model))) missing += std::string(" ") + c.model;
		for (const char* h : kRiderHorse) if (!STREAMING::HAS_MODEL_LOADED(H(h))) missing += std::string(" ") + h;
		Log("INTRO loading: %s after %u ms%s", missing.empty() ? "everything" : "still waiting for" , GetTickCount() - t0, missing.c_str());
	}
	if (!g_in.realCamp && !g_in.iplOn.empty())
	{
		g_in.campMap = IplOn(kCampBaseIpl);
		int on = 0;
		for (int h : g_in.iplOn) on += IplOn(h);
		Log("INTRO camp: %d of %d of Rockstar's camp pieces are up%s", on, (int)g_in.iplOn.size(), g_in.campMap ? "" : " - NOT the base layout");
	}
	if (!g_in.campMap) BuildFallbackProps();
	CampFire(g_in.campMap ? kCampFire : kCampCentre);
	// the people
	int castOk = 0;
	for (int i = 0; i < kCastCount; i++)
	{
		const CastDef& d = kCast[i];
		V3 at; float hd = 0;
		if (d.role == ROLE_RIDER)
		{
			int slot = i == CA_DUTCH ? RS_DUTCH : i == CA_JOHN ? RS_JOHN : RS_MICAH;
			at = CampMark(slot); hd = HeadingTo(at, at - g_in.X);
			// v1.7.2: Dutch out in front of the line, facing it - he asks Arthur over his own shoulder, John beside Arthur
			if (i == CA_DUTCH) { at = OnGround(g_in.G - g_in.X * 1.35f + g_in.S * -2.4f, 0.05f); hd = HeadingTo(at, CampMark(RS_ARTHUR)); }
		}
		else if (d.role == ROLE_HOME) { at = HomeMark(i); hd = HeadingTo(at, at + g_in.X); }
		else if (d.role == ROLE_CAMP) { at = OnGround(V3(d.x, d.y, kCampCentre.z), 0.05f); hd = RandRange(0, 360); }
		else { at = OnGround(g_in.G + g_in.X * 30.0f, 0.1f); hd = 0; }   // Cain: out of sight until the balloon
		g_in.cast[i].ped = SpawnCastAt(i, at, hd);
		g_in.cast[i].ok = g_in.cast[i].ped != 0;
		if (!g_in.cast[i].ok) continue;
		castOk++;
		if (d.role == ROLE_CAMP) StartCampLife(i);
		else if (d.role == ROLE_DOG) { ENTITY::SET_ENTITY_VISIBLE(g_in.cast[i].ped, FALSE); ENTITY::FREEZE_ENTITY_POSITION(g_in.cast[i].ped, TRUE); ENTITY::SET_ENTITY_COLLISION(g_in.cast[i].ped, FALSE, FALSE); }
		else if (i == CA_SUSAN) PlayAnim(g_in.cast[i].ped, kDictSusanIdle, "base", true, 8.0f);   // (her own stare; v1.7: the impatient scenario)
		else TASK::TASK_STAND_STILL(g_in.cast[i].ped, -1);
	}
	if (g_in.realCamp) HideDoubles();
	g_in.W = ClearWayOut();
	// the horses, behind the four, saddled
	int horsesOk = 0;
	for (int s = 0; s < kRiders; s++)
	{
		V3 hp = HorseCampMark(s);
		g_in.horses[s] = SpawnHorse(s, hp, HeadingTo(hp, hp + g_in.X));
		horsesOk += g_in.horses[s] != 0;
	}
	// Arthur on his mark, at the end of the line, facing the camp
	V3 am = CampMark(RS_ARTHUR);
	ENTITY::FREEZE_ENTITY_POSITION(me, FALSE);
	ENTITY::SET_ENTITY_COORDS(me, am.x, am.y, am.z, FALSE, FALSE, FALSE, FALSE);
	ENTITY::SET_ENTITY_HEADING(me, HeadingTo(am, am - g_in.X));
	ENTITY::SET_ENTITY_VISIBLE(me, TRUE);
	TASK::CLEAR_PED_TASKS_IMMEDIATELY(me, FALSE, TRUE);
	TASK::TASK_STAND_STILL(me, -1);
	Log("INTRO built in %u ms: %d of %d cast, %d of %d horses, camp %s (ground %s)", GetTickCount() - t0, castOk, kCastCount, horsesOk, kRiders,
		g_in.realCamp ? "the real one" : g_in.campMap ? "Rockstar's (switched on)" : "our props", campLoaded ? "loaded" : "NOT loaded");
	g_in.stage = 3;
	g_in.stageAt = NowSec();
}

// ---------- the camera ----------
static void IntroCam(const V3& pos, const V3& look, float fov)
{
	if (!g_in.cam)
	{
		g_in.cam = CAMERA::CREATE_CAM_WITH_PARAMS("DEFAULT_SCRIPTED_CAMERA", pos.x, pos.y, pos.z, 0, 0, 0, fov, FALSE, 2);
		if (!g_in.cam) { Log("INTRO: CREATE_CAM_WITH_PARAMS failed"); return; }
		CAMERA::SET_CAM_ACTIVE(g_in.cam, TRUE);
		CAMERA::RENDER_SCRIPT_CAMS(TRUE, FALSE, 0, TRUE, FALSE, 0);
	}
	CAMERA::SET_CAM_COORD(g_in.cam, pos.x, pos.y, pos.z);
	CAMERA::POINT_CAM_AT_COORD(g_in.cam, look.x, look.y, look.z);
	CAMERA::SET_CAM_FOV(g_in.cam, fov);
}

// the head bone (SKEL_Head)
static V3 PedHead(Ped p, float up = 1.6f)
{
	if (!p) return g_in.G;
	V3 h = PED::GET_PED_BONE_COORDS(p, 21030, 0, 0, 0);
	V3 body = ENTITY::GET_ENTITY_COORDS(p, FALSE, FALSE);
	if (h.len() > 1.0f && (h - body).len() < 2.5f) return h + V3(0, 0, up - 1.6f);
	return body + V3(0, 0, up - 1.0f);
}

// v1.4 the camera director: every shot says what it has to show (its subject); when a shot starts, the director tries the wanted
// spot, then the same spot lifted, swung round its pivot and pulled back - the smallest change first - and keeps the first one
// with a clear line to the subject that is also out of the funnel's smoke. Checked again twice a second; corrections glide.
static const int kIntroLosFlags = 1 | 2 | 16 | 256;   // the map, vehicles, objects, foliage
struct IntroShot
{
	V3 pos, look;
	float fov = 45;
	V3 subject;
	Entity ignore = 0;
	V3 pivot;
	bool wide = false;
	bool loose = false;
};

static bool IntroClear(const V3& subject, const V3& cam, Entity ignore)
{
	int h = SHAPETEST::START_EXPENSIVE_SYNCHRONOUS_SHAPE_TEST_LOS_PROBE(subject.x, subject.y, subject.z, cam.x, cam.y, cam.z, kIntroLosFlags, ignore, 7);
	BOOL hit = FALSE;
	Vector3 end = {}, n = {};
	Entity e = 0;
	SHAPETEST::GET_SHAPE_TEST_RESULT(h, &hit, &end, &n, &e);
	if (!hit) return true;
	return (V3(end) - subject).len() > (cam - subject).len() - 0.3f;
}

static bool IntroInSmoke(const V3& cam)
{
	Tornado* tp = ITp();
	if (!tp || tp->growth < 0.2f) return false;
	float hf = (cam.z - tp->base.z) / std::max(1.0f, tp->height());
	if (hf > 1.05f) return false;
	return (cam - tp->base).len2d() < tp->radiusAt(Clamp(hf, 0.0f, 1.0f)) * 1.3f + 7.0f;
}

static V3 IntroOffset(const IntroShot& s, float yawDeg, float lift, float pull)
{
	V3 rel = s.pos - s.pivot;
	float a = yawDeg * PI / 180.0f, ca = cosf(a), sa = sinf(a);
	V3 p = s.pivot + V3((rel.x * ca - rel.y * sa) * pull, (rel.x * sa + rel.y * ca) * pull, rel.z + lift);
	float gz = GroundZ(p.x, p.y, p.z + 30.0f, p.z - 100.0f);
	if (p.z < gz + 0.7f) p.z = gz + 0.7f;   // never under the ground
	return p;
}

static void IntroDirect(int shot, const IntroShot& s, float t)
{
	if (s.loose)
	{
		g_in.dirShot = shot; g_in.dirYaw = g_in.dirLift = g_in.dirYawS = g_in.dirLiftS = 0; g_in.dirPull = g_in.dirPullS = 1; g_in.dirPos = s.pos;
		IntroCam(s.pos, s.look, s.fov);
		return;
	}
	bool fresh = shot != g_in.dirShot;
	if (fresh) { g_in.dirShot = shot; g_in.dirYaw = g_in.dirLift = 0; g_in.dirPull = 1; }
	float tPrev = g_in.dirLastT;
	if (fresh || t >= g_in.dirCheckAt)
	{
		g_in.dirCheckAt = t + 0.5f;
		V3 cur = IntroOffset(s, g_in.dirYaw, g_in.dirLift, g_in.dirPull);
		if (!IntroClear(s.subject, cur, s.ignore) || IntroInSmoke(cur))
		{
			static const float kYaw[] = { 0, 18, -18, 36, -36, 60, -60, 90, -90, 125, -125, 180 };
			static const float kLiftNear[] = { 0, 0.8f, 2.0f, 4.0f };
			static const float kLiftFar[] = { 0, 4.0f, 9.0f, 16.0f };
			const float* lifts = s.wide ? kLiftFar : kLiftNear;
			struct Cand { float yaw, lift, pull, cost; };
			Cand cs[12 * 4 * 2];
			int n = 0;
			for (float y : kYaw)
				for (int li = 0; li < 4; li++)
					for (int pi = 0; pi < 2; pi++)
						cs[n++] = { y, lifts[li], pi ? 1.4f : 1.0f, fabsf(y) / 20.0f + li * 1.1f + pi * 1.6f };
			std::sort(cs, cs + n, [](const Cand& x, const Cand& y) { return x.cost < y.cost; });
			int tested = 0;
			bool found = false;
			for (int i = 0; i < n && tested < 40; i++)
			{
				V3 p = IntroOffset(s, cs[i].yaw, cs[i].lift, cs[i].pull);
				if (IntroInSmoke(p)) continue;
				tested++;
				if (!IntroClear(s.subject, p, s.ignore)) continue;
				g_in.dirYaw = cs[i].yaw; g_in.dirLift = cs[i].lift; g_in.dirPull = cs[i].pull;
				found = true;
				break;
			}
			if (!found)
			{
				g_in.dirYaw = 0; g_in.dirLift = lifts[3]; g_in.dirPull = 1.4f;
				for (float y : kYaw)
					if (!IntroInSmoke(IntroOffset(s, y, lifts[3], 1.4f))) { g_in.dirYaw = y; break; }
			}
			if (found || g_in.dirLogged != shot)
			{
				g_in.dirMoves++;
				Log("INTRO shot %d: the wanted camera couldn't see its subject - %s (swung %.0f deg, up %.1f m, x%.1f), %d spots tried",
					shot, found ? "moved" : "nothing clear, up and back", g_in.dirYaw, g_in.dirLift, g_in.dirPull, tested);
				if (!found) g_in.dirLogged = shot;
			}
		}
	}
	if (fresh) { g_in.dirYawS = g_in.dirYaw; g_in.dirLiftS = g_in.dirLift; g_in.dirPullS = g_in.dirPull; }
	else
	{
		float k = Clamp((t - tPrev) * 3.0f, 0.0f, 1.0f);
		g_in.dirYawS += (g_in.dirYaw - g_in.dirYawS) * k;
		g_in.dirLiftS += (g_in.dirLift - g_in.dirLiftS) * k;
		g_in.dirPullS += (g_in.dirPull - g_in.dirPullS) * k;
	}
	V3 p = IntroOffset(s, g_in.dirYawS, g_in.dirLiftS, g_in.dirPullS);
	g_in.dirPos = p;
	g_in.dirLastT = t;
	IntroCam(p, s.look + V3(0, 0, g_in.dirLiftS * 0.15f), s.fov);
}

static int IntroShotIndex(float t)
{
	int i = 0;
	while (i + 1 < kIntroShotCount && t >= kIntroShotAt[i + 1]) i++;
	return i;
}
static float IntroNextCut(float t)
{
	int i = IntroShotIndex(t);
	return i + 1 < kIntroShotCount ? kIntroShotAt[i + 1] : kIntroHandoff;
}
static float IntroShotU(float t, int i)
{
	float a = kIntroShotAt[i], b = i + 1 < kIntroShotCount ? kIntroShotAt[i + 1] : kIntroHandoff;
	return Ease(Clamp((t - a) / std::max(0.1f, b - a), 0.0f, 1.0f));
}

static V3 PedPos(Ped p, const V3& fb) { return p && ENTITY::DOES_ENTITY_EXIST(p) ? V3(ENTITY::GET_ENTITY_COORDS(p, FALSE, FALSE)) : fb; }
static V3 RidersCentre()
{
	V3 c; int n = 0;
	for (int s = 0; s < kRiders; s++) if (Ped p = Rider(s)) { c = c + V3(ENTITY::GET_ENTITY_COORDS(p, FALSE, FALSE)); n++; }
	return n ? c * (1.0f / n) : g_in.G;
}
static V3 FunnelLook(float hf)
{
	Tornado* tp = ITp();
	return tp ? tp->base + V3(0, 0, tp->height() * hf) : g_in.ranch + V3(0, 0, 60.0f * hf);
}

static void IntroShots(float t)
{
	int si = IntroShotIndex(t);
	float u = IntroShotU(t, si);
	IntroShot s;
	Ped me = PLAYER::PLAYER_PED_ID();
	Ped dutch = CastPed(CA_DUTCH), micah = CastPed(CA_MICAH), john = CastPed(CA_JOHN);
	auto at = [&](const V3& pos, const V3& look, float fov, const V3& subject, Entity ignore, const V3& pivot)
	{
		s.pos = pos; s.look = look; s.fov = fov; s.subject = subject; s.ignore = ignore; s.pivot = pivot;
	};
	// over one's shoulder at the other (v1.7.1: the user's favourite framing)
	auto ots = [&](Ped pNear, Ped pFar, float side, float back, float fov)
	{
		V3 nh = PedHead(pNear), fh = PedHead(pFar);
		V3 toF = FlatDir(nh, fh), sf(toF.y, -toF.x, 0);
		at(nh - toF * back + sf * side + V3(0, 0, 0.08f), fh + V3(0, 0, -0.04f), fov, fh, pFar, nh);
	};
	switch (si)
	{
	case SH_CAMP:
	{
		// a sunny morning at the camp: over the camp, down to the four at its edge with their horses (5.5 s, the title big)
		V3 g = g_in.G + V3(0, 0, 1.3f);
		at(LerpV(kCampCentre - g_in.X * 15.0f - g_in.S * 7.0f + V3(0, 0, 12.0f), kCampCentre - g_in.X * 6.0f - g_in.S * 5.0f + V3(0, 0, 5.5f), u),
			LerpV(kCampCentre + V3(0, 0, 1.0f), g, u), 50.0f, g, 0, kCampCentre);
		s.wide = true;
		float a = Clamp(std::min(t - 0.8f, kIntroShotAt[SH_FAREWELL] - 0.4f - t) * 1.4f, 0.0f, 1.0f);
		UI::PlaceCard("THE VAN DER LINDE GANG", "Valentine, 1899. The morning before the great storm.", a, 1.45f);
		break;
	}
	case SH_FAREWELL:
		// v1.7.2 (playtest 20: "move the camera closer to all four of them excluding the girls, over Dutch's shoulder but still showing
		// an upset John"): Dutch, out in front of the line, asks Arthur; John beside Arthur doesn't like it
		if (Ped a = me) ots(dutch, a, 0.45f, 0.85f, 38.0f);
		s.look = (PedHead(me) + PedHead(john)) * 0.5f + V3(0, 0, -0.05f);
		break;
	case SH_WOMEN:
	{
		// the reverse, over John's shoulder: Susan shoos them off and goes back to her camp, Abigail has a word for John
		V3 jh = PedHead(john);
		Ped ab = CastPed(CA_ABIGAIL), su = CastPed(CA_SUSAN);
		V3 wm = ab ? PedHead(ab) : jh - g_in.X * 3.0f;
		if (su) wm = (wm + PedHead(su)) * 0.5f;
		V3 toW = FlatDir(jh, wm), sw(toW.y, -toW.x, 0);
		at(jh - toW * 0.9f + sw * 0.55f + V3(0, 0, 0.1f), wm + V3(0, 0, -0.1f), 40.0f, ab ? PedHead(ab) : wm, ab, jh);
		break;
	}
	case SH_MICAH:  ots(me, micah, 0.42f, 0.75f, 30.0f); break;   // over Arthur's shoulder: "You sure you got the lungs for this, Morgan?"
	case SH_ARTHUR: ots(micah, me, -0.42f, 0.75f, 30.0f); break;  // over Micah's: "Shut the hell up."
	case SH_RIDEOUT:
	{
		// they ride out of camp the clear way, the camera behind them (4 s: they're well away by the end)
		V3 c = RidersCentre();
		V3 ws(g_in.W.y, -g_in.W.x, 0);
		at(OnGround(g_in.G - g_in.W * 5.0f + ws * 3.2f, 1.5f), c + V3(0, 0, 1.6f), 46.0f, c + V3(0, 0, 1.8f), 0, g_in.G);
		s.wide = true;
		break;
	}
	case SH_VALENTINE:
	{
		// v1.7.2: leaving Valentine - from the camera found with the place: the town behind them, the four coming up and past
		V3 c = RidersCentre();
		at(g_in.valCam, LerpV(kValentineC + V3(0, 0, 6.0f), c + V3(0, 0, 1.6f), Clamp(u * 1.6f - 0.15f, 0.0f, 1.0f)), 48.0f, c + V3(0, 0, 1.8f), 0, g_in.valCam);
		s.wide = true;
		float a = Clamp(std::min(t - kIntroShotAt[SH_VALENTINE] - 0.4f, kIntroShotAt[SH_ARRIVE] - 0.3f - t) * 1.6f, 0.0f, 1.0f);
		UI::PlaceCard("VALENTINE", "Late morning", a);
		break;
	}
	case SH_ARRIVE:
	{
		// v1.7.2: the ranch below first (it's had the dip to load), then they ride up past the camera and pull up on the ridge
		at(g_in.arrCam, LerpV(g_in.ranch + V3(0, 0, 6.0f), (g_in.O + g_in.ranch) * 0.5f + V3(0, 0, 2.0f), Clamp(u * 1.4f - 0.2f, 0.0f, 1.0f)), 48.0f,
			g_in.O + V3(0, 0, 1.6f), 0, g_in.arrCam);
		s.wide = true;
		float a = Clamp(std::min(t - kIntroShotAt[SH_ARRIVE] - 0.6f, kIntroShotAt[SH_RIDGE_M] - 0.2f - t) * 1.6f, 0.0f, 1.0f);
		UI::PlaceCard("EMERALD RANCH", "That afternoon", a);
		break;
	}
	case SH_RIDGE_M: ots(me, micah, 0.42f, 0.8f, 32.0f); break;    // v1.7.2 (playtest 20: "they just stand silently"): over Arthur's shoulder
	case SH_RIDGE_A: ots(micah, me, -0.42f, 0.8f, 32.0f); break;   // ...and over Micah's for "lizards"
	case SH_THUNDER:
	{
		// thunder: all four look up. In front of them at eye height, so the letterbox doesn't take their heads (playtest 20)
		V3 c = RidersCentre();
		at(c + g_in.D * 5.6f + V3(0, 0, 1.15f), c + V3(0, 0, 1.6f), 46.0f, c + V3(0, 0, 1.6f), 0, c);
		break;
	}
	case SH_TOUCHDOWN:
	{
		// over their shoulders: the funnel comes down on the ranch - and the camera leans in
		V3 look = LerpV(FunnelLook(0.55f), FunnelLook(0.25f), u);
		at(OnGround(g_in.O - g_in.D * 4.2f + g_in.Sd * 0.9f, 2.5f), look, Lerp(46.0f, 31.0f, u), FunnelLook(0.3f), 0, g_in.O);
		s.wide = true;
		break;
	}
	case SH_RANCH:
	{
		// v1.7.2 THE HERO SHOT: down at the ranch, the funnel on it, its cattle and people going up
		Tornado* tp = ITp();
		V3 b = tp ? tp->base : g_in.ranch;
		V3 toR = FlatDir(g_in.ranch, g_in.O);
		float a = 0.7f;   // (40 deg round from the ridge's side)
		V3 dir(toR.x * cosf(a) - toR.y * sinf(a), toR.x * sinf(a) + toR.y * cosf(a), 0);
		at(OnGround(b + dir * Lerp(80.0f, 64.0f, u), 3.0f), b + V3(0, 0, Lerp(14.0f, 20.0f, u)), 54.0f, b + V3(0, 0, 10.0f), 0, b);
		s.wide = true;
		break;
	}
	case SH_POINT:
	{
		// v1.7.2: behind the four, the funnel coming up the valley at them - John yells (and stays put until the cut)
		V3 c = RidersCentre();
		at(OnGround(c - g_in.D * 4.6f + g_in.Sd * 0.6f, 2.0f), FunnelLook(0.3f), 50.0f, FunnelLook(0.3f), 0, c);
		s.wide = true;
		break;
	}
	case SH_DUTCH:
	{
		// v1.7.2: Dutch, from in front: "We have a plan. My friends." - and he's off (John's already running behind him)
		V3 dh = PedHead(dutch);
		at(dh + g_in.D * 2.5f + g_in.Sd * 0.6f + V3(0, 0, 0.05f), dh + V3(0, 0, -0.08f), 36.0f, dh, dutch, dh);
		break;
	}
	case SH_MICAHLAST:
		// v1.7.3 (playtest 21: "micah isn't even facing arthur"): over Arthur's shoulder at Micah, who's turned to him - his back to the
		// drop, the funnel coming up behind him. "Well this is fun, ain't it?"
		ots(me, micah, 0.42f, 0.85f, 34.0f);
		s.look = PedHead(micah) + g_in.D * 0.6f + V3(0, 0, 0.1f);
		break;
	case SH_SHOVE:
		// a little closer, the same side: "Don't fall off." - and Arthur shoves him, backwards off the ridge
		ots(me, micah, 0.38f, 0.6f, 40.0f);
		s.look = PedHead(micah) + g_in.D * 1.4f + V3(0, 0, -0.2f);
		break;
	case SH_FALL:
	default:
	{
		// from the ridge, beside them: Micah goes down and the funnel takes him up
		V3 mp = PedPos(micah, g_in.O + g_in.D * 10.0f);
		V3 look = LerpV(mp + V3(0, 0, 0.5f), FunnelLook(0.35f), Clamp((t - g_in.shovedAt - 1.6f) / 2.0f, 0.0f, 0.6f));
		at(OnGround(g_in.O + g_in.Sd * 7.5f + g_in.D * 0.8f, 2.6f), look, 54.0f, FunnelLook(0.3f), micah, g_in.O);
		s.wide = true;
		break;
	}
	}
	IntroDirect(si, s, t);
	if (si != g_in.camShot)
	{
		g_in.camShot = si;
		g_in.cutAt = t;
		if (g_in.cam && (si == SH_TOUCHDOWN || si == SH_RANCH || si == SH_FALL)) CAMERA::SHAKE_CAM(g_in.cam, "HAND_SHAKE", si == SH_TOUCHDOWN ? 0.3f : 0.5f);
		else if (g_in.cam) CAMERA::STOP_CAM_SHAKING(g_in.cam, TRUE);
	}
}

// v1.7.2 (playtest 20: "make arthur look mad, try and put some emotion on the face"): who feels what, shot by shot (every frame)
static void IntroFaces(float t)
{
	int si = IntroShotIndex(t);
	Ped me = PLAYER::PLAYER_PED_ID(), dutch = CastPed(CA_DUTCH), micah = CastPed(CA_MICAH), john = CastPed(CA_JOHN);
	auto mood = [](Ped p, Hash m) { if (p && ENTITY::DOES_ENTITY_EXIST(p)) PED::REQUEST_PED_FACIAL_MOOD_THIS_FRAME(p, m, 6); };
	const Hash kAngry = 137506481u, kSmug = 3347446607u, kScared = 3716590166u, kShocked = 2583247227u, kCocky = 3070686211u, kPanic = 3729996742u;
	if (si <= SH_ARTHUR) { mood(john, kAngry); mood(me, si >= SH_MICAH ? kAngry : kShocked); mood(micah, kSmug); }
	else if (si == SH_RIDGE_M || si == SH_RIDGE_A) { mood(micah, kSmug); mood(me, kAngry); }
	else if (si >= SH_THUNDER && si <= SH_DUTCH) { mood(me, kShocked); mood(dutch, kScared); mood(john, kPanic); mood(micah, kScared); }
	else if (si == SH_MICAHLAST || si == SH_SHOVE) { mood(micah, kSmug); mood(me, kCocky); }
}

static bool Beat(int id, bool when)
{
	if (!when) return false;
	for (int b : g_in.firedBeats) if (b == id) return false;
	g_in.firedBeats.push_back(id);
	return true;
}
static bool Fired(int id) { for (int b : g_in.firedBeats) if (b == id) return true; return false; }

// ---------- the lines (v1.3-v1.5, unchanged) ----------
static const float kIntroEarRange = 15.0f;
static bool IntroFar(Ped p)
{
	if (!p || !ENTITY::DOES_ENTITY_EXIST(p)) return false;
	return (V3(ENTITY::GET_ENTITY_COORDS(p, FALSE, FALSE)) - V3(CAMERA::GET_FINAL_RENDERED_CAM_COORD())).len() > kIntroEarRange;
}
static bool IntroSpeakNear(Ped p, const char* voice, const char* ctx, bool shout)
{
	if (!voice || !voice[0] || !ctx) return false;
	V3 cam = CAMERA::GET_FINAL_RENDERED_CAM_COORD();
	V3 d = (p && ENTITY::DOES_ENTITY_EXIST(p)) ? V3(ENTITY::GET_ENTITY_COORDS(p, FALSE, FALSE)) - cam : V3(0, 1, 0);
	float L = std::max(0.1f, d.len());
	V3 at = cam + d * (2.5f / L);
	SpeechParams sp{ ctx, voice, 0, Joaat(shout ? "SPEECH_PARAMS_BEAT_SHOUTED_CLEAR_SUB" : "SPEECH_PARAMS_BEAT_SPOKEN_CLEAR_SUB"), 0, FALSE, 1, 1 };
	bool ok = AUDIO::PLAY_AMBIENT_SPEECH_FROM_POSITION_NATIVE(at.x, at.y, at.z, reinterpret_cast<int*>(&sp)) != 0;
	if (ok)
	{
		g_in.nearLines++;
		g_in.nearUntilMs = g_introTick() + 2200;
		if (!g_in.nearUntilMs) g_in.nearUntilMs = 1;
		if (p) PED::REQUEST_PED_FACIAL_MOOD_THIS_FRAME(p, 1751822680u /*MoodTalking*/, 6);
	}
	return ok;
}
static const char* IntroVoice(int who) { return who < 0 ? "ARTHUR" : kCast[who].speaker; }
static const char* IntroWho(int who) { return who < 0 ? "arthur" : kCast[who].id; }

static bool IntroSay(Ped p, const char* const* ctx, bool shout, const char* who, const char* voice)
{
	if (!p || !ENTITY::DOES_ENTITY_EXIST(p) || !ctx[0]) return false;
	for (int i = 0; i < 4 && ctx[i]; i++)
	{
		if (!AUDIO::DOES_CONTEXT_EXIST_FOR_THIS_PED(p, ctx[i], FALSE)) continue;
		g_in.lineAt = (float)g_in.clock;
		if (IntroFar(p) && IntroSpeakNear(p, voice, ctx[i], shout))
		{
			Log("INTRO %.1f s: %s says %s -> played beside the camera", (float)g_in.clock, who, ctx[i]);
			return true;
		}
		g_in.ambPed = p; g_in.ambAt = (float)g_in.clock;
		bool ok = Speak(p, ctx[i], shout ? "SPEECH_PARAMS_BEAT_SHOUTED_CLEAR_SUB" : "SPEECH_PARAMS_BEAT_SPOKEN_CLEAR_SUB");
		if (!ok) ok = Speak(p, ctx[i], shout ? "SPEECH_PARAMS_FORCE_SHOUTED" : "SPEECH_PARAMS_FORCE");
		Log("INTRO %.1f s: %s says %s -> %s", (float)g_in.clock, who, ctx[i], ok ? "played" : "refused");
		return ok;
	}
	Log("INTRO %.1f s: %s has none of %s...", (float)g_in.clock, who, ctx[0]);
	return false;
}

static void IntroStoryBlocks(void (*fn)(const char*))
{
	for (int i = 0; i < kIntroLineCount; i++)
	{
		const char* b = kIntroLines[i].block;
		if (!b) continue;
		bool seen = false;
		for (int j = 0; j < i; j++) if (kIntroLines[j].block && strcmp(kIntroLines[j].block, b) == 0) seen = true;
		if (!seen) fn(b);
	}
	if (kAfterLine.block)
	{
		bool seen = false;
		for (int j = 0; j < kIntroLineCount; j++) if (kIntroLines[j].block && strcmp(kIntroLines[j].block, kAfterLine.block) == 0) seen = true;
		if (!seen) fn(kAfterLine.block);
	}
}

// one line of one of Rockstar's own conversations, as their scripts do it: create, every voice in the scene, start, the one line
static bool IntroStoryRoot(const char* root, int idx, const char* words, const char* who, const char* block)
{
	if (!AUDIO::IS_SCRIPTED_CONVERSATION_CREATED(root) && !AUDIO::CREATE_NEW_SCRIPTED_CONVERSATION(root))
	{
		Log("INTRO %.1f s: STORY %s couldn't be created", (float)g_in.clock, root);
		return false;
	}
	AUDIO::ADD_PED_TO_CONVERSATION(root, PLAYER::PLAYER_PED_ID(), "ARTHUR");
	for (int c = 0; c < kCastCount; c++)
	{
		Ped p = CastPed(c);
		if (!p || !kCast[c].speaker[0] || !ENTITY::DOES_ENTITY_EXIST(p) || ENTITY::IS_ENTITY_DEAD(p)) continue;
		AUDIO::ADD_PED_TO_CONVERSATION(root, p, kCast[c].speaker);
		if (c == CA_MICAH) AUDIO::ADD_PED_TO_CONVERSATION(root, p, "MICAH");
		if (c == CA_CHARLES) AUDIO::ADD_PED_TO_CONVERSATION(root, p, "CHARLES");
		if (c == CA_SUSAN) AUDIO::ADD_PED_TO_CONVERSATION(root, p, "GRIMSHAW");
	}
	AUDIO::START_SCRIPT_CONVERSATION(root, TRUE, TRUE, FALSE);
	AUDIO::PLAY_SINGLE_LINE_OF_CONVERSATION(root, idx);
	Log("INTRO %.1f s: STORY %s: %s[%d] \"%s\"%s", (float)g_in.clock, who, root, idx, words,
		!block || HUD::TEXT_BLOCK_IS_LOADED(block) ? "" : " (its subtitles aren't loaded)");
	return true;
}
static bool IntroStory(int i)
{
	const IntroLine& l = kIntroLines[i];
	return IntroStoryRoot(l.root, l.idx, l.words, IntroWho(l.who), l.block);
}

static void IntroStoryStop()
{
	if (g_in.storyLine >= 0 && AUDIO::IS_SCRIPTED_CONVERSATION_PLAYING(kIntroLines[g_in.storyLine].root))
		AUDIO::STOP_SCRIPTED_CONVERSATION(kIntroLines[g_in.storyLine].root, FALSE, FALSE);
	g_in.storyLine = -1;
}

static void IntroStoryEnd()
{
	IntroStoryStop();
	AUDIO::SET_AUDIO_FLAG("DisableAbortConversationForDeathAndInjury", FALSE);
	IntroStoryBlocks([](const char* b) { HUD::TEXT_BLOCK_DELETE(b); });
}

static void IntroFallback(int i)
{
	const IntroLine& l = kIntroLines[i];
	g_in.storyFell++;
	if (!l.ctx[0]) { Log("INTRO %.1f s: STORY %s[%d] didn't play (no stand-in line)", (float)g_in.clock, l.root, l.idx); return; }
	if (l.root) Log("INTRO %.1f s: STORY %s[%d] didn't play - %s's own line instead", (float)g_in.clock, l.root, l.idx, IntroWho(l.fbWho));
	int who = l.root ? l.fbWho : l.who;
	IntroSay(Speaker(who), l.ctx, l.shout, IntroWho(who), IntroVoice(who));
}

// a story line's subtitles are asked for 8 s before it, and let go once no line still to come needs that block
static void IntroBlocks(float t)
{
	for (int i = 0; i < kIntroLineCount; i++)
	{
		const IntroLine& l = kIntroLines[i];
		if (!l.block) continue;
		if (!g_in.blockReq[i] && t >= l.t - 8.0f)
		{
			bool asked = false;
			for (int j = 0; j < kIntroLineCount; j++) if (j != i && g_in.blockReq[j] && !g_in.blockGone[j] && kIntroLines[j].block && strcmp(kIntroLines[j].block, l.block) == 0) asked = true;
			if (!asked) HUD::TEXT_BLOCK_REQUEST(l.block);
			g_in.blockReq[i] = true;
		}
		if (g_in.blockReq[i] && !g_in.blockGone[i] && g_in.linePlayed[i] && t > l.t + 8.0f)
		{
			bool later = false;
			for (int j = 0; j < kIntroLineCount; j++) if (kIntroLines[j].block && strcmp(kIntroLines[j].block, l.block) == 0 && (!g_in.linePlayed[j] || t <= kIntroLines[j].t + 8.0f)) later = true;
			if (kAfterLine.block && strcmp(kAfterLine.block, l.block) == 0) later = true;   // (Arthur's line after the handoff)
			if (!later)
			{
				HUD::TEXT_BLOCK_DELETE(l.block);
				for (int j = 0; j < kIntroLineCount; j++) if (kIntroLines[j].block && strcmp(kIntroLines[j].block, l.block) == 0) g_in.blockGone[j] = true;
			}
		}
	}
}

static void IntroLines(float t)
{
	IntroBlocks(t);
	if (g_in.storyLine >= 0)
	{
		const IntroLine& l = kIntroLines[g_in.storyLine];
		bool playing = AUDIO::IS_SCRIPTED_CONVERSATION_PLAYING(l.root) != 0;
		if (playing && !g_in.storySeen)
		{
			g_in.storySeen = true;
			g_in.storyPlayed++;
			Log("INTRO %.1f s: STORY %s[%d] is playing (%.1f s after the cue)", t, l.root, l.idx, t - g_in.storyAt);
		}
		else if (!playing && !g_in.storySeen && t - g_in.storyAt > 1.2f)
		{
			int i = g_in.storyLine;
			IntroStoryStop();
			IntroFallback(i);
		}
		else if (!playing && g_in.storySeen)
			g_in.storyLine = -1;
		// v1.7.1: a line from a set Rockstar plays at random - which take is it? (checked in its first 0.3 s, before it's heard)
		int i = g_in.storyLine;
		if (i >= 0 && !g_in.picked[i] && t - g_in.storyAt < 0.3f)
			for (const LinePick& lp : kLinePicks)
				if (lp.line == i)
				{
					int cur = AUDIO::GET_CURRENT_SCRIPTED_CONVERSATION_LINE(kIntroLines[i].root);
					if (cur < 0) break;
					bool ok = false;
					for (int k : lp.ok) ok = ok || k == cur;
					if (ok || g_in.rerolls >= 4)
					{
						g_in.picked[i] = true;
						Log("INTRO %.1f s: %s took take %d of its set%s", t, kIntroLines[i].root, cur, ok ? "" : " (kept: four draws)");
					}
					else
					{
						Log("INTRO %.1f s: %s drew take %d - drawn again", t, kIntroLines[i].root, cur);
						AUDIO::STOP_SCRIPTED_CONVERSATION(kIntroLines[i].root, FALSE, FALSE);
						AUDIO::CLEAR_CONVERSATION_HISTORY_FOR_SCRIPTED_CONVERSATION(kIntroLines[i].root);
						g_in.rerolls++;
						if (IntroStory(i)) { g_in.storyAt = t; g_in.storySeen = false; }
					}
				}
		if (i >= 0 && !g_in.picked[i] && t - g_in.storyAt >= 0.3f)
		{
			g_in.picked[i] = true;
			for (const LinePick& lp : kLinePicks) if (lp.line == i) Log("INTRO %s: the take couldn't be read back (%d)", kIntroLines[i].root, AUDIO::GET_CURRENT_SCRIPTED_CONVERSATION_LINE(kIntroLines[i].root));
		}
	}
	for (int i = 0; i < kIntroLineCount; i++)
	{
		const IntroLine& l = kIntroLines[i];
		if (t >= l.t && t < l.t + 1.6f)
			{}   // (v1.7.2: IntroFaces sets the speaker's face - the words still move the mouth)
		if (t < l.t || g_in.linePlayed[i]) continue;
		// the ears - never right after a cut (wait up to 1.5 s for it)
		float ear = std::min(kIntroEarDelay, std::max(0.3f, l.t - kIntroShotAt[IntroShotIndex(l.t)]));   // (v1.7.3: Dutch's comes sooner)
		if (t < g_in.cutAt + ear && t < l.t + 1.5f) continue;
		// one at a time: wait (up to 1.5 s) for the story line being said to finish
		bool busy = g_in.storyLine >= 0 && (l.root || kIntroLines[g_in.storyLine].who == l.who);
		if (busy && t < l.t + 1.5f) continue;
		if (busy && l.root) IntroStoryStop();
		g_in.linePlayed[i] = true;
		g_in.lineStart[i] = t;
		Ped sp = Speaker(l.who);
		if (l.root)
		{
			g_in.lineAt = t;
			if (l.audio && IntroFar(sp) && IntroSpeakNear(sp, IntroVoice(l.who), l.audio, l.shout))
			{
				Log("INTRO %.1f s: STORY %s: %s \"%s\" -> played beside the camera by its audio name", t, IntroWho(l.who), l.audio, l.words);
				g_in.storyPlayed++;
			}
			else if (IntroStory(i)) { g_in.storyLine = i; g_in.storyAt = t; g_in.storySeen = false; g_in.rerolls = 0; }   // (four redraws per line)
			else IntroFallback(i);
		}
		else
			IntroSay(sp, l.ctx, l.shout, IntroWho(l.who), IntroVoice(l.who));
	}
}

static bool IntroLineOn(float t)
{
	if (g_in.storyLine >= 0 && AUDIO::IS_SCRIPTED_CONVERSATION_PLAYING(kIntroLines[g_in.storyLine].root)) return true;
	if (g_in.ambPed && t - g_in.ambAt < 4.0f && ENTITY::DOES_ENTITY_EXIST(g_in.ambPed) && AUDIO::IS_AMBIENT_SPEECH_PLAYING(g_in.ambPed)) return true;
	return g_in.nearUntilMs && (int)(g_in.nearUntilMs - g_introTick()) > 0;
}

// ---------- the phases: the riders moved between the places at the cuts ----------
static void Mount(int s)
{
	Ped p = Rider(s), h = g_in.horses[s];
	if (!p || !h || !ENTITY::DOES_ENTITY_EXIST(h)) return;
	if (!PED::IS_PED_ON_MOUNT(p)) { TASK::CLEAR_PED_TASKS_IMMEDIATELY(p, FALSE, TRUE); PED::SET_PED_ONTO_MOUNT(p, h, -1, TRUE); }
}
static void PutHorse(int s, const V3& at, float heading, float speed)
{
	Ped h = g_in.horses[s];
	if (!h || !ENTITY::DOES_ENTITY_EXIST(h)) return;
	ENTITY::FREEZE_ENTITY_POSITION(h, FALSE);
	ENTITY::SET_ENTITY_COORDS(h, at.x, at.y, at.z, FALSE, FALSE, FALSE, FALSE);
	ENTITY::SET_ENTITY_HEADING(h, heading);
	if (speed > 0) { V3 v = HeadingDir(heading) * speed; ENTITY::SET_ENTITY_VELOCITY(h, v.x, v.y, 0.0f); }
}
static void RideTo(int s, const V3& to, float speed, bool nav, float finalHeading = 40000.0f)
{
	Ped p = Rider(s);
	if (!p) return;
	if (nav) TASK::TASK_FOLLOW_NAV_MESH_TO_COORD(p, to.x, to.y, to.z, speed, -1, 1.5f, 0, finalHeading);
	else TASK::TASK_GO_STRAIGHT_TO_COORD(p, to.x, to.y, to.z, speed, -1, finalHeading > 999.0f ? HeadingTo(PedPos(p, to), to) : finalHeading, 0.8f, 0);
}

static void PhaseRideOut()
{
	g_in.phase = 1;
	for (int s = 0; s < kRiders; s++) Mount(s);
	// (v1.7.1: in pairs, the clear way, not four abreast into the trees)
	V3 ws(g_in.W.y, -g_in.W.x, 0);
	for (int s = 0; s < kRiders; s++)
		RideTo(s, OnGround(g_in.G + g_in.W * (46.0f - (s / 2) * 4.0f) + ws * (s % 2 ? 0.9f : -0.9f), 0.0f), 2.2f, true);
	LockWeather("CLOUDS", 0.0f);
	CLOCK::SET_CLOCK_TIME(11, 0, 0);
	Log("INTRO %.1f s: mounted up and riding out", (float)g_in.clock);
}
static void PhaseValentine()
{
	g_in.phase = 2;
	float hd = HeadingTo(V3(), g_in.VD);
	for (int s = 0; s < kRiders; s++)
	{
		Mount(s);
		float row = (s / 2) * 3.4f, side = g_in.valOff + (s % 2 ? 1.1f : -1.1f);
		V3 at = OnGround(g_in.VR - g_in.VD * (22.0f + row) + g_in.VS * side, 0.2f);
		PutHorse(s, at, hd, 3.0f);   // (v1.7.1: 6.5 m/s made one rear up; v1.7.2: slower still - "slower, not colliding with a tree")
		RideTo(s, OnGround(g_in.VR + g_in.VD * 45.0f + g_in.VS * side, 0.0f), 2.1f, false, hd);
	}
	LockWeather("OVERCAST", 0.0f);
	CLOCK::SET_CLOCK_TIME(13, 30, 0);
	Log("INTRO %.1f s: on the road out of Valentine", (float)g_in.clock);
}
static void PhaseArrive()
{
	g_in.phase = 3;
	float hd = HeadingTo(V3(), g_in.D);
	for (int s = 0; s < kRiders; s++)
	{
		Mount(s);
		V3 at = OnGround(g_in.O - g_in.D * (19.0f + (s % 2) * 2.5f) + g_in.Sd * (kRidgeK[s] * 1.4f), 0.2f);   // (v1.7.2: behind the camera)
		PutHorse(s, at, hd, 3.0f);
		RideTo(s, OnGround(g_in.O - g_in.D * 2.2f + g_in.Sd * (kRidgeK[s] * 1.3f), 0.0f), 2.2f, false, hd);
	}
	LockWeather("OVERCASTDARK", 0.0f);
	CLOCK::SET_CLOCK_TIME(16, 0, 0);
	Log("INTRO %.1f s: arriving on the ridge over Emerald Ranch", (float)g_in.clock);
}
static void PhaseRidge()
{
	g_in.phase = 4;
	float hd = HeadingTo(V3(), g_in.D);
	for (int s = 0; s < kRiders; s++)
	{
		Ped p = Rider(s);
		if (!p) continue;
		if (PED::IS_PED_ON_MOUNT(p)) PED::REMOVE_PED_FROM_MOUNT(p, TRUE, FALSE);
		TASK::CLEAR_PED_TASKS_IMMEDIATELY(p, FALSE, TRUE);
		V3 m = RidgeMark(s);
		ENTITY::SET_ENTITY_COORDS(p, m.x, m.y, m.z, FALSE, FALSE, FALSE, FALSE);
		ENTITY::SET_ENTITY_HEADING(p, hd);
		TASK::TASK_STAND_STILL(p, -1);
		PutHorse(s, HorseRidgeMark(s), hd + RandRange(-25.0f, 25.0f), 0.0f);
		if (Ped h = g_in.horses[s]) TASK::TASK_STAND_STILL(h, -1);
	}
	Log("INTRO %.1f s: on foot at the edge", (float)g_in.clock);
}
// the camp's done with: its people go (out of their camp spots first), and its map pieces are put back later (CampMapHousekeeping)
static void LeaveCamp()
{
	for (int i = 0; i < kCastCount; i++)
		if (kCast[i].role == ROLE_HOME || kCast[i].role == ROLE_CAMP)
		{
			if (Ped p = CastPed(i)) TASK::CLEAR_PED_TASKS_IMMEDIATELY(p, FALSE, TRUE);
			IntroDeletePed(g_in.cast[i].ped);
			g_in.cast[i].ok = false;
		}
	for (Entity e : g_in.props)
		if (e && ENTITY::DOES_ENTITY_EXIST(e)) { SetScripted(e, false); Object o = e; DeleteObj(o); }
	g_in.props.clear();
	StopFx();
	CampMapOff("the scene has left the camp");
}

// v1.7.1: the riders are taken to a new place while the screen is black (playtest 19's Valentine cut was a hard teleport into
// ground that wasn't loaded, with a load-scene for the ridge running - then the crash): the horses go first, frozen; then the cut
// waits for the ground round Arthur, and they're put on it and set going as the picture comes back
static void DipStart(int shot)
{
	g_in.dip = 1; g_in.dipShot = shot; g_in.dipAt = NowSec();
	CAMERA::DO_SCREEN_FADE_OUT(260);
	Log("INTRO %.1f s: dip to black for %s", (float)g_in.clock, shot == SH_VALENTINE ? "Valentine" : "the ridge");
}
static bool DipUpdate()   // true while it's holding the clock
{
	if (!g_in.dip) return false;
	float now = NowSec();
	Ped me = PLAYER::PLAYER_PED_ID();
	if (g_in.dip == 1)
	{
		if (!CAMERA::IS_SCREEN_FADED_OUT() && now - g_in.dipAt < 0.9f) return true;
		if (g_in.dipShot == SH_VALENTINE && !Fired(6)) { LeaveCamp(); g_in.firedBeats.push_back(6); }
		V3 to = g_in.dipShot == SH_VALENTINE ? g_in.VR - g_in.VD * 20.0f : g_in.O - g_in.D * 14.0f;
		to.z = (g_in.dipShot == SH_VALENTINE ? g_in.VR.z : g_in.O.z) + 1.0f;
		for (int s = 0; s < kRiders; s++)
			if (Ped h = g_in.horses[s])
			{
				Mount(s);
				V3 at = to + V3(0, 0, 0) + (g_in.dipShot == SH_VALENTINE ? g_in.VS : g_in.Sd) * (s * 2.5f - 3.75f);
				ENTITY::SET_ENTITY_COORDS(h, at.x, at.y, at.z, FALSE, FALSE, FALSE, FALSE);
				ENTITY::FREEZE_ENTITY_POSITION(h, TRUE);
			}
		g_in.dip = 2; g_in.dipAt = now;
		return true;
	}
	V3 pp = PlayerPos();
	STREAMING::REQUEST_COLLISION_AT_COORD(pp.x, pp.y, pp.z);
	bool ready = ENTITY::HAS_COLLISION_LOADED_AROUND_ENTITY(me) != 0 && HasGround(pp);
	if ((!ready || now - g_in.dipAt < (g_in.dipShot == SH_ARRIVE ? 1.2f : 0.5f)) && now - g_in.dipAt < 3.5f) return true;   // (v1.7.2: the ranch a moment to load)
	if (g_in.dipShot == SH_VALENTINE) { PhaseValentine(); g_in.firedBeats.push_back(5); }
	else { PhaseArrive(); g_in.firedBeats.push_back(7); }
	CAMERA::DO_SCREEN_FADE_IN(380);
	Log("INTRO the ground there %s after %.1f s in the dark", ready ? "loaded" : "WASN'T loaded", now - g_in.dipAt);
	g_in.clock = kIntroShotAt[g_in.dipShot];
	g_in.dip = 0;
	return false;
}


// ---------- the beats ----------
static void IntroSpawnTornado()
{
	V3 p = g_in.ranch;
	p.z = GroundZ(p.x, p.y, p.z + 60.0f, p.z);
	SpawnOpts so;
	so.grow = 2.5f;
	Tornado* nt = SpawnTornadoAt(p, HeadingTo(g_in.ranch, g_in.O), g_set.style, GetStyles()[g_set.style].name, so);
	g_in.tp = TornadoRef(nt);
	if (nt) { nt->growSeconds = 2.5f; nt->scripted = true; nt->scriptVel = V3(); nt->stationary = false; }
	Log("INTRO tornado %s on the ranch, %.0f m from the ridge", ITp() ? "down" : "FAILED", (g_in.ranch - g_in.O).len2d());
}

static void SteerTornado(const V3& target, float maxSpeed, float arriveBy, float t)
{
	Tornado* tp = ITp();
	if (!tp) return;
	V3 d = target - tp->base; d.z = 0;
	float L = d.len2d();
	float speed = arriveBy > t ? std::min(maxSpeed, L / std::max(0.3f, arriveBy - t)) : std::min(maxSpeed, L * 0.6f);
	tp->scriptVel = L > 0.5f ? d * (speed / L) : V3();
}

static void SpawnBalloon()
{
	V3 b = g_in.B;
	b.z = GroundZ(b.x, b.y, b.z + 30.0f, b.z);
	float hd = HeadingTo(b, b - g_in.D);   // facing away from the storm
	if (!BalloonSpawnParked(b, hd)) { Log("INTRO the balloon wouldn't spawn at (%.1f, %.1f, %.1f)", b.x, b.y, b.z); return; }
	if (Ped dog = CastPed(CA_CAIN))
	{
		ENTITY::FREEZE_ENTITY_POSITION(dog, FALSE);
		ENTITY::SET_ENTITY_COLLISION(dog, TRUE, TRUE);
		ENTITY::SET_ENTITY_VISIBLE(dog, TRUE);
		TASK::CLEAR_PED_TASKS_IMMEDIATELY(dog, FALSE, TRUE);
		ENTITY::ATTACH_ENTITY_TO_ENTITY(dog, g_bal.body, 0, 0.35f, -0.45f, g_bal.basketZ + 0.55f, 0, 0, 160.0f, FALSE, FALSE, FALSE, FALSE, 2, TRUE, FALSE, FALSE);
		TASK::TASK_START_SCENARIO_IN_PLACE_HASH(dog, Joaat("WORLD_ANIMAL_DOG_SITTING"), -1, FALSE, 0, -1.0f, FALSE);
	}
	Log("INTRO the balloon is waiting %.0f m behind the ridge (Cain's in it)", (g_bal.pos - g_in.O).len2d());
}

// Micah, after the push: over the edge, a moment's fall, then the funnel has him - in, round and up
static void UpdateMicah(float t, float dt)
{
	Ped m = CastPed(CA_MICAH);
	if (!m || !Fired(40)) return;
	if (g_in.pushPed && t < g_in.pushUntil) ENTITY::SET_ENTITY_VELOCITY(g_in.pushPed, g_in.pushVel.x, g_in.pushVel.y, g_in.pushVel.z);
	Tornado* tp = ITp();
	float since = t - g_in.shovedAt;
	if (!tp || since < 1.15f) return;
	if (!PED::IS_PED_RAGDOLL(m)) PED::SET_PED_TO_RAGDOLL(m, 20000, 25000, 0, FALSE, FALSE, nullptr);
	V3 mp = ENTITY::GET_ENTITY_COORDS(m, FALSE, FALSE);
	V3 rel = mp - tp->base;
	float r = rel.len2d();
	if (!g_in.micahGone && r < 9.0f)
	{
		g_in.micahGone = true;
		g_in.micahAng = atan2f(rel.y, rel.x);
		g_in.micahH = std::max(2.0f, mp.z - tp->base.z);
		Log("INTRO %.1f s: the funnel has Micah", t);
	}
	V3 want;
	if (!g_in.micahGone)
	{
		// pulled in, rising: harder the longer it has him
		float k = Clamp((since - 1.15f) / 1.5f, 0.0f, 1.0f);
		V3 to = tp->base + V3(0, 0, Lerp(4.0f, 12.0f, k)) - mp;
		float L = std::max(0.1f, to.len());
		want = to * (Lerp(9.0f, 24.0f, k) / L);
	}
	else
	{
		// round and up (to 60 m), then he's thrown out later (stage 5)
		g_in.micahAng += 2.6f * dt;
		g_in.micahH = std::min(60.0f, g_in.micahH + 11.0f * dt);
		float rr = std::max(5.0f, tp->radiusAt(Clamp(g_in.micahH / std::max(1.0f, tp->height()), 0.0f, 1.0f)) * 0.9f);
		V3 goal = tp->base + V3(cosf(g_in.micahAng) * rr, sinf(g_in.micahAng) * rr, g_in.micahH);
		want = (goal - mp) * 6.0f;
		float L = want.len();
		if (L > 30.0f) want = want * (30.0f / L);
	}
	ENTITY::SET_ENTITY_VELOCITY(m, want.x, want.y, want.z);
	PED::REQUEST_PED_FACIAL_MOOD_THIS_FRAME(m, 4127737830u /*MoodWindExtreme*/, 6);
}

// v1.7.2: the ranch's cattle and farmhands, set down round where the funnel will come down (the ridge's ground is loaded; the ranch's
// is asked for every frame from the arrival on), free for the tornado to take
static void SpawnRanchLife()
{
	int n = 0;
	for (int k = 0; k < (int)(sizeof(kRanchLife) / sizeof(kRanchLife[0])); k++)
	{
		Hash m = H(kRanchLife[k]);
		if (!STREAMING::HAS_MODEL_LOADED(m)) continue;
		float ang = k * 2.39996f, r = 7.0f + (k % 4) * 4.5f;
		V3 at = g_in.ranch + V3(cosf(ang), sinf(ang), 0) * r;
		at.z = GroundZ(at.x, at.y, g_in.ranch.z + 30.0f, g_in.ranch.z) + 0.3f;
		Ped p = PED::CREATE_PED(m, at.x, at.y, at.z, RandRange(0, 360), FALSE, FALSE, FALSE, FALSE);
		if (!p) continue;
		PED::SET_RANDOM_OUTFIT_VARIATION(p, TRUE);
		PED::UPDATE_PED_VARIATION(p, FALSE, TRUE, TRUE, TRUE, FALSE);
		ENTITY::SET_ENTITY_AS_MISSION_ENTITY(p, TRUE, TRUE);
		ENTITY::SET_ENTITY_LOD_DIST(p, 500);
		TASK::TASK_WANDER_IN_AREA(p, at.x, at.y, at.z, 6.0f, 2.0f, 6.0f, 0);
		g_in.ranchLife.push_back(p);
		n++;
	}
	for (const char* m : kRanchLife) STREAMING::SET_MODEL_AS_NO_LONGER_NEEDED(H(m));
	Log("INTRO the ranch: %d head of cattle and farmhands set down for the funnel", n);
}
// in the hero shot, what's in the funnel's reach goes up, one after another (the tornado's own grab does the rest)
static void RanchLifeUpdate(float t)
{
	Tornado* tp = ITp();
	if (!tp || t < kIntroShotAt[SH_RANCH] + 0.6f || t > kIntroSweep) return;
	int k = 0;
	for (Ped p : g_in.ranchLife)
	{
		k++;
		if (!p || !ENTITY::DOES_ENTITY_EXIST(p)) continue;
		float due = kIntroShotAt[SH_RANCH] + 0.6f + k * 0.35f;
		if (t < due || t > due + 1.2f) continue;
		V3 pp = ENTITY::GET_ENTITY_COORDS(p, FALSE, FALSE);
		V3 to = tp->base - pp; to.z = 0;
		float L = std::max(1.0f, to.len2d());
		if (L > 45.0f) continue;
		if (PED::IS_PED_HUMAN(p) && !PED::IS_PED_RAGDOLL(p)) PED::SET_PED_TO_RAGDOLL(p, 6000, 9000, 0, FALSE, FALSE, nullptr);
		ENTITY::SET_ENTITY_VELOCITY(p, to.x / L * 9.0f, to.y / L * 9.0f, 16.0f);
	}
}

static void IntroBeats(float t, float dt)
{
	Ped me = PLAYER::PLAYER_PED_ID();
	Ped dutch = CastPed(CA_DUTCH), micah = CastPed(CA_MICAH), john = CastPed(CA_JOHN), susan = CastPed(CA_SUSAN), abigail = CastPed(CA_ABIGAIL);
	// the farewell: Dutch turns to Arthur ("let's go" - every take of his line is said to Arthur), the women watch them
	if (Beat(1, t >= 0.5f))
	{
		TASK::TASK_TURN_PED_TO_FACE_ENTITY(me, dutch, 1200, 0, 0, 0);
		if (dutch) { TASK::TASK_TURN_PED_TO_FACE_ENTITY(dutch, me, 1200, 0, 0, 0); TASK::TASK_LOOK_AT_ENTITY(dutch, me, 9000, 0, 51, 0); }
		TASK::TASK_LOOK_AT_ENTITY(me, dutch, 6000, 0, 51, 0);
		if (susan && dutch) TASK::TASK_LOOK_AT_ENTITY(susan, dutch, 9000, 0, 51, 0);
		if (john && abigail) { TASK::TASK_LOOK_AT_ENTITY(john, abigail, 12000, 0, 51, 0); TASK::TASK_LOOK_AT_ENTITY(abigail, john, 12000, 0, 51, 0); }
	}
	// Susan: "Well, hurry it along." - a shooing hand, and back to her camp
	if (Beat(19, g_in.lineStart[LN_SUSAN] > 0 && t >= g_in.lineStart[LN_SUSAN] + 0.15f) && susan)
		PlayAnim(susan, kDictFemaleTalk, "annoyed_dismiss_r_001", false, 6.0f);
	if (Beat(20, g_in.lineStart[LN_SUSAN] > 0 && t >= g_in.lineStart[LN_SUSAN] + 1.5f) && susan)
	{
		V3 to = OnGround(kCampCentre + V3(RandRange(-3.0f, 3.0f), RandRange(-3.0f, 3.0f), 0), 0.0f);
		TASK::TASK_GO_STRAIGHT_TO_COORD(susan, to.x, to.y, to.z, 1.0f, -1, 0.0f, 1.0f, 0);
	}
	// Micah and Arthur turn to each other
	if (Beat(2, t >= kIntroShotAt[SH_MICAH] - 0.6f) && micah)
	{
		TASK::TASK_TURN_PED_TO_FACE_ENTITY(micah, me, 1500, 0, 0, 0);
		TASK::TASK_TURN_PED_TO_FACE_ENTITY(me, micah, 1500, 0, 0, 0);
		TASK::TASK_LOOK_AT_ENTITY(micah, me, 5000, 0, 51, 0);
		TASK::TASK_LOOK_AT_ENTITY(me, micah, 5000, 0, 51, 0);
	}
	// the cuts between the places (each waits for the next place's ground: see IntroUpdate)
	if (Beat(3, t >= kIntroShotAt[SH_RIDEOUT])) PhaseRideOut();
	if (Beat(4, t >= kIntroShotAt[SH_RIDEOUT] + 0.4f))
	{
		if (abigail) TASK::TASK_LOOK_AT_ENTITY(abigail, Rider(RS_JOHN), 4000, 0, 51, 0);
	}
	if (Beat(18, t >= kIntroShotAt[SH_VALENTINE] + 0.3f))
		for (Ped p : { dutch, john }) PlayUpper(p, kDictWindHat, "wind_hat_a");   // (the wind's getting up)
	// (v1.7.1: Valentine and the ridge are reached in a dip to black - IntroDip)
	if (Beat(8, t >= kIntroShotAt[SH_RIDGE_M])) { PhaseRidge(); SpawnRanchLife(); }
	if (g_in.phase >= 3) STREAMING::REQUEST_COLLISION_AT_COORD(g_in.ranch.x, g_in.ranch.y, g_in.ranch.z);   // (the ranch, for the tornado)
	// the ridge: Arthur and Micah turn to each other for their words, then back to the view
	if (Beat(9, t >= kIntroShotAt[SH_RIDGE_M] + 0.1f) && micah)
	{
		TASK::TASK_TURN_PED_TO_FACE_ENTITY(micah, me, 1500, 0, 0, 0);
		TASK::TASK_TURN_PED_TO_FACE_ENTITY(me, micah, 1500, 0, 0, 0);
		TASK::TASK_LOOK_AT_ENTITY(micah, me, 9000, 0, 51, 0);
		TASK::TASK_LOOK_AT_ENTITY(me, micah, 9000, 0, 51, 0);
	}
	// ...and back to the view before the thunder
	if (Beat(21, t >= kIntroShotAt[SH_THUNDER] - 0.3f))
		for (int s = 0; s < kRiders; s++)
			if (Ped p = Rider(s)) { V3 f = g_in.O + g_in.D * 50.0f; TASK::TASK_TURN_PED_TO_FACE_COORD(p, f.x, f.y, f.z, 800); }
	// thunder, once, far off over the ranch: all four look up
	if (Beat(10, t >= kIntroThunder))
	{
		V3 sky = g_in.ranch + g_in.D * 160.0f + V3(0, 0, 160.0f);
		MISC::FORCE_LIGHTNING_FLASH_AT_COORDS(sky.x, sky.y, sky.z, -1.0f);
		V3 up = g_in.O + g_in.D * 40.0f + V3(0, 0, 60.0f);
		for (int s = 0; s < kRiders; s++)
			if (Ped p = Rider(s)) TASK::TASK_LOOK_AT_COORD(p, up.x, up.y, up.z, 4000, 0, 51, FALSE);
		PlayUpper(me, kAnimLookUp[0], kAnimLookUp[1]);
		int k = 0;
		for (Ped p : { dutch, john, micah }) if (p) PlayAnim(p, kDictStartle, kAnimStartle[k++ % 3], false, 8.0f);
	}
	if (Beat(11, t >= kIntroSpawn)) IntroSpawnTornado();
	if (Beat(12, t >= kIntroSpawn + 0.2f))
		for (int s = 0; s < kRiders; s++)
			if (Ped p = Rider(s)) { V3 f = FunnelLook(0.3f); TASK::TASK_LOOK_AT_COORD(p, f.x, f.y, f.z, 12000, 0, 51, FALSE); }
	// the tornado: on the ranch (circling its yard for the hero shot), then for the ridge - at its foot for the push
	if (ITp())
	{
		V3 foot = g_in.O + g_in.D * kIntroPushDist;
		if (t < kIntroSpawn + 3.0f) ITp()->scriptVel = V3();
		else if (t < kIntroSweep) SteerTornado(g_in.ranch + V3(cosf(t * 0.6f), sinf(t * 0.6f), 0) * 9.0f, 5.0f, 0.0f, t);
		else if (t < kIntroShove) SteerTornado(foot, 26.0f, kIntroArrive, t);
		else SteerTornado(g_in.O + g_in.D * 24.0f, 2.0f, 0.0f, t);
	}
	// Micah points: it's coming this way
	if (Beat(13, t >= kIntroLines[LN_JOHN_POINT].t - 0.2f) && john) PlayAnim(john, kAnimPoint[0], kAnimPoint[1], false, 6.0f);
	RanchLifeUpdate(t);
	// the horses have had enough
	if (Beat(14, t >= kIntroShotAt[SH_POINT] + 1.2f))
		for (Ped h : g_in.horses)
			if (h) { TASK::CLEAR_PED_TASKS(h, TRUE, FALSE); V3 c = ITp() ? ITp()->base : g_in.ranch; TASK::TASK_SMART_FLEE_COORD(h, c.x, c.y, c.z, 300.0f, -1, 0, 3.0f); }
	// Dutch and John: a word each, and they run for it
	auto run = [&](Ped p, float side)
	{
		if (!p) return;
		TASK::CLEAR_PED_TASKS(p, TRUE, FALSE);
		V3 to = OnGround(g_in.O - g_in.D * 95.0f + g_in.Sd * side, 0.0f);
		TASK::TASK_GO_STRAIGHT_TO_COORD(p, to.x, to.y, to.z, 3.0f, -1, HeadingTo(to, to - g_in.D), 1.0f, 0);
	};
	auto after = [&](int line, float d, float latest) { float s0 = g_in.lineStart[line]; return (s0 > 0 && t >= s0 + d) || t >= latest; };
	// (v1.7.2, playtest 20: John ran mid-shot - now he goes at the cut to Dutch's shot, and Dutch as soon as he's said it)
	if (Beat(16, t >= kIntroShotAt[SH_DUTCH])) run(john, -6.0f);
	if (Beat(15, after(LN_DUTCH_PLAN, 1.0f, kIntroDutchRuns + 1.0f))) run(dutch, -14.0f);   // (v1.7.3: off he goes mid-line)
	// Arthur steps in behind Micah (at the cut, unseen) - then the push
	if (Beat(17, t >= kIntroShotAt[SH_MICAHLAST]) && micah)
	{
		V3 mp = ENTITY::GET_ENTITY_COORDS(micah, FALSE, FALSE);
		V3 behind = OnGround(mp - g_in.D * 0.95f + g_in.Sd * 0.1f, 0.05f);   // (v1.7.3: in front of Micah, who turns his back to the drop)
		TASK::CLEAR_PED_TASKS_IMMEDIATELY(me, FALSE, TRUE);
		ENTITY::SET_ENTITY_COORDS(me, behind.x, behind.y, behind.z, FALSE, FALSE, FALSE, FALSE);
		ENTITY::SET_ENTITY_HEADING(me, HeadingTo(V3(), g_in.D));
		TASK::TASK_LOOK_AT_ENTITY(me, micah, 6000, 0, 51, 0);
		TASK::CLEAR_PED_TASKS_IMMEDIATELY(micah, FALSE, TRUE);
		ENTITY::SET_ENTITY_HEADING(micah, HeadingTo(V3(), g_in.D * -1.0f));   // facing Arthur
		TASK::TASK_LOOK_AT_ENTITY(micah, me, 6000, 0, 51, 0);
		TASK::TASK_STAND_STILL(micah, -1);
	}
	// the push: Rockstar's own shove from behind (Arthur's half and Micah's), on the last word - then Micah's a ragdoll, going over
	float shoveAt = g_in.lineStart[LN_ARTHUR_FALL] > 0 ? std::max(kIntroShove, g_in.lineStart[LN_ARTHUR_FALL] + 0.5f) : kIntroShove + 1.0f;
	if (Beat(39, t >= shoveAt - 0.3f) && micah)
	{
		PlayAnim(me, kDictMelee, kAnimShoveAtt, false, 8.0f);
		PlayAnim(micah, kDictMelee, kAnimShoveVic, false, 8.0f);
	}
	if (Beat(40, t >= shoveAt) && micah)
	{
		TASK::CLEAR_PED_TASKS_IMMEDIATELY(micah, FALSE, TRUE);
		PED::SET_PED_TO_RAGDOLL(micah, 20000, 25000, 0, FALSE, FALSE, nullptr);
		g_in.pushPed = micah;
		g_in.pushVel = g_in.D * 7.5f + V3(0, 0, 4.2f);
		g_in.pushUntil = t + 0.3f;
		g_in.shovedAt = t;
		ENTITY::SET_ENTITY_VELOCITY(micah, g_in.pushVel.x, g_in.pushVel.y, g_in.pushVel.z);
		Log("INTRO %.1f s: Arthur pushes Micah off the ridge (the funnel %.0f m out)", t, ITp() ? (ITp()->base - g_in.O).len2d() : -1.0f);
	}
	UpdateMicah(t, dt);
	if (Beat(41, t >= kIntroBalloon)) SpawnBalloon();
	IntroLines(t);
	IntroFaces(t);
}

// ---------- the handoff and the run for the balloon ----------
static void IntroHandoff()
{
	Ped me = PLAYER::PLAYER_PED_ID();
	// the game's camera comes back behind Arthur looking at the balloon: that's where you're going
	{
		V3 pp = PlayerPos();
		V3 to = g_bal.on ? g_bal.pos : g_in.B;
		float rel = HeadingTo(pp, to) - ENTITY::GET_ENTITY_HEADING(me);
		while (rel > 180.0f) rel -= 360.0f;
		while (rel < -180.0f) rel += 360.0f;
		CAMERA::SET_GAMEPLAY_CAM_RELATIVE_HEADING(rel, 1.0f);
		CAMERA::SET_GAMEPLAY_CAM_RELATIVE_PITCH(-6.0f, 1.0f);
	}
	if (g_in.cam && CAMERA::DOES_CAM_EXIST(g_in.cam))
	{
		CAMERA::STOP_CAM_SHAKING(g_in.cam, TRUE);
		CAMERA::SET_CAM_ACTIVE(g_in.cam, FALSE);
		CAMERA::RENDER_SCRIPT_CAMS(FALSE, TRUE, 1800, TRUE, FALSE, 0);
		g_in.oldCam = g_in.cam;
	}
	g_in.cam = 0;
	PLAYER::SET_PLAYER_CONTROL(PLAYER::PLAYER_ID(), TRUE, 0, FALSE);
	PED::SET_PED_CAN_RAGDOLL(me, TRUE);
	TASK::CLEAR_PED_TASKS(me, TRUE, FALSE);
	MISC::SET_TIME_SCALE(1.0f);
	HUD::DISPLAY_HUD(TRUE);
	MAP::DISPLAY_RADAR(TRUE);
	// the user's own settings back - except Arthur: untouchable for 10 s, then the tornado can take him until he's in the balloon
	IntroRestoreSettings();
	g_set.arthur = 0;
	g_in.shieldOn = true;
	g_holdStorm = true;   // (the cloudy sky holds until he's in the balloon)
	if (g_bal.on && g_bal.body)
	{
		g_in.blip = MAP::BLIP_ADD_FOR_ENTITY(Joaat("BLIP_STYLE_OBJECTIVE"), g_bal.body);
		if (g_in.blip) MAP::SET_BLIP_NAME(g_in.blip, "Hot air balloon");
	}
	UI::Objective("Reach the ~COLOR_YELLOW~hot air balloon~s~.", 9.0f);
	if (kAfterLine.block) HUD::TEXT_BLOCK_REQUEST(kAfterLine.block);
	g_in.stage = 5;
	g_in.afterStart = NowSec();
	int castOk = 0;
	for (auto& c : g_in.cast) castOk += c.ok;
	Finding("INTRO played to the handoff | cast %d/%d | story lines %d heard, %d fell back | balloon %s", castOk, kCastCount, g_in.storyPlayed, g_in.storyFell,
		g_bal.on ? "waiting" : "missing");
	Log("INTRO handoff: control is back - run for the balloon (%.0f m)", g_bal.on ? (g_bal.pos - PlayerPos()).len2d() : -1.0f);
}

static void IntroAfterLine()
{
	Ped me = PLAYER::PLAYER_PED_ID();
	if (kAfterLine.root && IntroStoryRoot(kAfterLine.root, kAfterLine.idx, kAfterLine.words, "arthur", kAfterLine.block)) return;
	const char* ctx[4] = { kAfterLine.ctx, nullptr, nullptr, nullptr };
	IntroSay(me, ctx, false, "arthur", "ARTHUR");
}

// the scene's over: what's left belongs to the world
static void IntroDone(const char* why)
{
	IntroRemoveBlip();
	IntroReleaseHorses();
	IntroReleaseDicts();
	if (Tornado* tp = ITp()) tp->scripted = false;   // (your Movement setting has it now)
	IntroStoryEnd();
	IntroRestoreSettings();
	IntroRestoreWeather();
	IntroReleaseCast();
	g_in.stage = 0;
	g_in.shieldOn = false;
	UI::Letterbox(0);
	Log("INTRO done (%s)", why);
}

static void IntroAfter(float t, float dt)
{
	float a = t - g_in.afterStart;
	Ped me = PLAYER::PLAYER_PED_ID();
	UI::Letterbox(1.0f - Clamp(a / 1.4f, 0.0f, 1.0f));
	if (a < 1.5f) HUD::HIDE_HUD_AND_RADAR_THIS_FRAME();
	if (g_in.oldCam && a > 2.4f)
	{
		if (CAMERA::DOES_CAM_EXIST(g_in.oldCam)) CAMERA::DESTROY_CAM(g_in.oldCam, FALSE);
		g_in.oldCam = 0;
	}
	float tc = (float)g_in.clock + a;
	UpdateMicah(tc, dt);
	IntroBlocks(tc);
	// 10 s the tornado can't take you; then it can (Grabbable) until you're in the balloon
	if (g_in.shieldOn && a > 10.0f)
	{
		g_in.shieldOn = false;
		if (!g_in.boarded) { g_set.arthur = std::max(2, g_in.savedArthur); g_in.grabSet = true; Log("INTRO 10 s up: the tornado can take Arthur now (until he's in the balloon)"); }
	}
	// the tornado comes on, over the ridge, on your heels - never closer than 20 m in the first 10 s
	if (Tornado* tp = ITp())
	{
		if (!g_in.boarded)
		{
			V3 pp = PlayerPos();
			float d = (pp - tp->base).len2d();
			float speed = a < 10.0f ? (d > 24.0f ? 4.5f : 0.0f) : 6.8f;
			V3 dir = FlatDir(tp->base, pp);
			tp->scripted = true;
			tp->scriptVel = dir * speed;
		}
		else if (t - g_in.boardedAt > 4.0f && tp->scripted)
			tp->scripted = false;   // your Movement setting has it now
	}
	// Dutch and John: fair game once they're well away; Micah is thrown out after a while
	if (Beat(50, a > 6.0f))
		for (int i : { CA_DUTCH, CA_JOHN }) if (Ped p = CastPed(i)) SetScripted(p, false);
	if (Beat(51, a > 12.0f))
		if (Ped m = CastPed(CA_MICAH))
		{
			SetScripted(m, false);
			if (Tornado* tp = ITp())
			{
				V3 out = FlatDir(tp->base, ENTITY::GET_ENTITY_COORDS(m, FALSE, FALSE));
				ENTITY::SET_ENTITY_VELOCITY(m, out.x * 24.0f, out.y * 24.0f, 6.0f);
			}
			g_in.micahGone = false;
			g_in.firedBeats.erase(std::remove(g_in.firedBeats.begin(), g_in.firedBeats.end(), 40), g_in.firedBeats.end());   // (no more steering)
			Log("INTRO Micah is thrown out of the funnel");
		}
	// in the balloon: the mission's done
	if (!g_in.boarded && g_bal.on && g_bal.waiting && !PlayerDead() && !PED::IS_PED_ON_MOUNT(me) &&
		(PlayerPos() - g_bal.pos).len2d() < 4.0f && fabsf(PlayerPos().z - g_bal.pos.z) < 6.0f)
	{
		BalloonBoard(false);
		g_in.boarded = true;
		g_in.boardedAt = t;
		IntroRemoveBlip();
		UI::Objective("", 0.1f);
		UI::Shard("STORM CHASERS", "Mission complete", 4.5f, true);
		if (!g_in.afterLine) { g_in.afterLine = true; IntroAfterLine(); }   // "Okay, here goes nothing."
		if (g_in.grabSet) { g_set.arthur = g_in.savedArthur; g_in.grabSet = false; }
		g_holdStorm = false;
		IntroRestoreWeather();
		Log("INTRO %.1f s after the handoff: Arthur's in the balloon - mission complete", a);
	}
	// over: a while after boarding, or if you've gone off and left it
	bool away = (PlayerPos() - g_in.O).len2d() > 450.0f;
	if ((g_in.boarded && t - g_in.boardedAt > 15.0f) || a > 150.0f || away || (!g_bal.on && !g_in.boarded && a > 2.0f))
	{
		if (g_in.grabSet) { g_set.arthur = g_in.savedArthur; g_in.grabSet = false; }
		IntroDone(g_in.boarded ? "mission complete" : away ? "Arthur went his own way" : !g_bal.on ? "the balloon is gone" : "time");
	}
}

// ---------- skip: straight to the ridge, the push done, the balloon waiting ----------
static void IntroSkip()
{
	if (g_in.stage != 4 || g_in.clock > kIntroHandoff - 1.5) return;
	Log("INTRO skipped at %.1f s", (float)g_in.clock);
	IntroStoryStop();
	// (v1.7.1: in the dark, waiting for the ridge's ground, as the dips do)
	CAMERA::DO_SCREEN_FADE_OUT(200);
	for (DWORD until = GetTickCount() + 700; !CAMERA::IS_SCREEN_FADED_OUT() && GetTickCount() < until;) WAIT(0);
	g_in.dip = 0;
	if (g_in.phase < 1) PhaseRideOut();
	if (!Fired(6)) { LeaveCamp(); g_in.firedBeats.push_back(6); }
	if (g_in.phase < 4)
	{
		PutPlayer(g_in.O - g_in.D * 6.0f + V3(0, 0, 1.0f));
		for (DWORD until = GetTickCount() + 3000; GetTickCount() < until;)
		{
			V3 pp = PlayerPos();
			STREAMING::REQUEST_COLLISION_AT_COORD(pp.x, pp.y, pp.z);
			if (ENTITY::HAS_COLLISION_LOADED_AROUND_ENTITY(PLAYER::PLAYER_PED_ID()) && HasGround(pp)) break;
			WAIT(0);
		}
		PhaseArrive(); PhaseRidge();
	}
	CAMERA::DO_SCREEN_FADE_IN(500);
	if (!ITp()) IntroSpawnTornado();
	if (Tornado* tp = ITp())
	{
		V3 foot = g_in.O + g_in.D * (kIntroPushDist - 4.0f);
		foot.z = GroundZ(foot.x, foot.y, g_in.O.z + 20.0f, g_in.O.z - 40.0f);
		tp->base = foot;
		tp->growth = 1.0f;
	}
	for (Ped h : g_in.horses) if (h) { V3 c = g_in.ranch; TASK::TASK_SMART_FLEE_COORD(h, c.x, c.y, c.z, 300.0f, -1, 0, 3.0f); }
	for (int i : { CA_DUTCH, CA_JOHN })
		if (Ped p = CastPed(i)) { V3 to = OnGround(g_in.O - g_in.D * 95.0f, 0.0f); TASK::TASK_GO_STRAIGHT_TO_COORD(p, to.x, to.y, to.z, 3.0f, -1, 0, 1.0f, 0); }
	if (Ped m = CastPed(CA_MICAH))
	{
		TASK::CLEAR_PED_TASKS_IMMEDIATELY(m, FALSE, TRUE);
		if (Tornado* tp = ITp()) { V3 at = tp->base + V3(6.0f, 0, 10.0f); ENTITY::SET_ENTITY_COORDS(m, at.x, at.y, at.z, FALSE, FALSE, FALSE, FALSE); }
		PED::SET_PED_TO_RAGDOLL(m, 20000, 25000, 0, FALSE, FALSE, nullptr);
	}
	for (int id = 1; id <= 21; id++) if (!Fired(id)) g_in.firedBeats.push_back(id);
	g_in.firedBeats.push_back(39);
	g_in.firedBeats.push_back(40);
	g_in.shovedAt = (float)g_in.clock;
	if (!g_bal.on) { SpawnBalloon(); g_in.firedBeats.push_back(41); }
	for (int i = 0; i < kIntroLineCount; i++) g_in.linePlayed[i] = true;
	g_in.holdHere = 0; g_in.nearUntilMs = 0; g_in.ambPed = 0;
	g_in.clock = kIntroShotAt[SH_FALL] + 0.2;
	g_in.shovedAt = (float)g_in.clock - 1.3f;
	g_in.camShot = -1;
}

// ---------- the frame ----------
void IntroUpdate(float dt, float t)
{
	CampMapHousekeeping();
	if (!g_in.stage) return;
	if (PlayerDead()) { IntroAbort("Arthur died"); return; }
	switch (g_in.stage)
	{
	case 1:   // fading out
		if (CAMERA::IS_SCREEN_FADED_OUT() || t - g_in.stageAt > 2.0f) { g_in.stage = 2; IntroBuild(); }
		return;
	case 3:   // settling: the camp's people take their spots, the first camera is placed
		UI::Letterbox(1.0f);
		IntroShots(0.0f);
		if (t - g_in.stageAt > 1.6f)
		{
			g_in.stage = 4;
			g_in.clock = 0;
			g_in.lastTick = g_introTick();
			CAMERA::DO_SCREEN_FADE_IN(1200);
			CheckCampLife();
			int blocks = 0, loaded = 0;
			for (int i = 0; i < kIntroLineCount; i++)
				if (kIntroLines[i].block && g_in.blockReq[i]) { blocks++; if (HUD::TEXT_BLOCK_IS_LOADED(kIntroLines[i].block)) loaded++; }
			Log("INTRO action (the first lines' subtitles: %d of %d loaded)", loaded, blocks);
		}
		return;
	case 4:
	{
		DWORD now = g_introTick();
		double step = (now - g_in.lastTick) / 1000.0;
		g_in.lastTick = now;
		if (step > 0.5) step = 0.016;
		float tc0 = (float)g_in.clock, next = IntroNextCut(tc0);
		if ((float)(g_in.clock + step) >= next)   // (as the shots will see it: a float)
		{
			// a cut waits for a line still being said (up to 2.2 s here, 6 s in all)...
			// (a reply still due in this shot - its line waited for the last one - is said here too, not on the next shot's first frame)
			bool replyDue = false;
			for (int i = 0; i < kIntroLineCount; i++)
				if (!g_in.linePlayed[i] && kIntroLines[i].t >= kIntroShotAt[IntroShotIndex(tc0)] && kIntroLines[i].t < next) replyDue = true;
			// (v1.7.2: and for its tail - a second after the shot's last word)
			bool tail = g_in.lineAt >= g_in.cutAt && NowSec() - g_in.lineEndReal < kIntroTail;
			if (((g_in.lineAt >= g_in.cutAt && IntroLineOn(tc0)) || replyDue || tail) && g_in.holdHere < 3.2f && g_in.holdTotal < 14.0f)
			{
				if (g_in.holdHere == 0) Log("INTRO %.1f s: holding the cut for a line still being said", tc0);
				g_in.holdHere += (float)step; g_in.holdTotal += (float)step;
				step = 0;
			}
			// ...the fall waits for the push (up to 2 s)...
			else if (IntroShotIndex(tc0) + 1 == SH_FALL && !Fired(40) && g_in.holdShove < 2.0f)
			{
				g_in.holdShove += (float)step;
				step = 0;
			}
			// ...and a new place is reached in a dip to black (v1.7.1)
			else
			{
				g_in.holdHere = 0;
				int si = IntroShotIndex(tc0) + 1;
				if ((si == SH_VALENTINE && !Fired(5)) || (si == SH_ARRIVE && !Fired(7))) { if (!g_in.dip) DipStart(si); step = 0; }
			}
		}
		if (g_in.dip) step = 0;
		{
			bool on = IntroLineOn(tc0);
			if (g_in.lineWasOn && !on) g_in.lineEndReal = NowSec();
			g_in.lineWasOn = on;
		}
		g_in.clock += step;
		DipUpdate();
		float tc = (float)g_in.clock;
		// the next place's ground, asked for ahead of its cut (no load-scene: v1.7.1)
		for (int sh : { SH_VALENTINE, SH_ARRIVE })
			if (tc > kIntroShotAt[sh] - 3.0f && tc < kIntroShotAt[sh] + 1.0f)
			{
				V3 p = sh == SH_VALENTINE ? g_in.VR : g_in.O;
				STREAMING::REQUEST_COLLISION_AT_COORD(p.x, p.y, p.z);
			}
		if (Pressed(g_keys.back) || PadPressed(PB_B)) IntroSkip();
		PAD::DISABLE_ALL_CONTROL_ACTIONS(0);
		HUD::HIDE_HUD_AND_RADAR_THIS_FRAME();
		UI::Letterbox(1.0f);
		IntroBeats((float)g_in.clock, dt);
		IntroShots((float)g_in.clock);
		if (g_in.clock >= kIntroHandoff) IntroHandoff();
		return;
	}
	case 5:
		IntroAfter(t, dt);
		return;
	default:
		return;
	}
}
