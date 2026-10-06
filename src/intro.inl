// NadoTest v1.3 - THE INTRO: "Arthur Had a Feeling". Part of script.cpp (included there for the tornado, storm, camera and
// balloon helpers). The screenplay and shot list are in INTRO.md.
//
// v1.3, rebuilt after playtest 11 ("weird shading lighting issues, I couldn't hear any of the characters, I couldn't tell what
// was going on... we want to splice the character's actual lines... arthur saying chat lowk a lil cringey... funnier if arthur
// is just ditching them in the balloon and they are all like WHAT SERIOUSLY? and he's off like he had it prepped somehow knew
// the storm was coming. 1 min long max... use ragdoll, be funny"). So:
//   - Arthur sits in a hot air balloon in the middle of camp from the first shot. The gang wants to know what's got into him;
//     he grumbles about the weather. The storm comes; he pulls the burner and leaves them to it;
//   - every line is the character's own voice (their ambient banks), with the game's own subtitle asked for (the _SUB speech
//     params - playtest 11 showed those subtitles do appear). No invented subtitles;
//   - the gang goes up as ragdolls in the real tornado (Uncle with his chair, Pearson with his pot, Javier with his guitar);
//     Dutch is thrown right past the balloon in slow motion;
//   - no colour grade, no Dead Eye tint (the "weird shading"), and every camera is outside the funnel's smoke (playtest 11's
//     frames: the old close-ups were inside it - a white fog);
//   - real time drives the timeline (so slow motion doesn't stretch it); a paused game pauses it.
// v1.4, after playtest 12 ("needs to be more obvious what's going on... establish better, insert its context better, be more
// cinematic, use the game's animations and fix its current angles"; the frames showed the basket's wicker filling Arthur's
// close-ups and tree trunks filling the wide shots):
//   - a camera director: every shot names what it has to show, and a camera that can't see it (a shape-test probe) is swung
//     round, lifted or pulled back until it can - and kept out of the funnel's smoke;
//   - Arthur is filmed from above the basket's rim, aimed at his real head bone;
//   - a longer setup that tells the story before the storm: a place card, the camp, the reveal of Arthur in the balloon as
//     the gang turns to look, his look at the sky and what he sees there (a storm on the horizon), Dutch's "Have a little
//     faith", his facepalm and "If you say so." - and the thunder;
//   - the storm half is the same story, 5 s later, with a shot for each beat (the lift-off, Dutch, Micah, the camp going up).
// v1.5, after playtest 13 ("really coming along... super funny now and readable... give ample time for the 'ears' of the game
// camera to catch up... a little more long... drawn out... a few more key characters, lines, back and forth and jokes. He should
// explain why he's leaving, clear and funny... hero shot, tornado sucking people up and Arthur confidently leaving"; and "some of
// those lines can't be heard because of character location"):
//   - about 100 s, with Hosea, John and Lenny; 20 real lines from the game (research\intro_v15_lines.md) in back-and-forths, each
//     line in its own shot of its speaker, so it's said close to the camera. Arthur says why he's leaving: "I'm leaving. Don't work
//     too hard." / "I ain't dying for whatever nonsense Micah and Dutch have concocted...";
//   - the ears: a line never starts in the first second after a cut, the scene holds a cut while a line is still being said, and a
//     far-off speaker is played from beside the camera in their own voice (camp lines by their audio name);
//   - the funnel waits 35 m out, behind the camp, while the gang has their last words, then sweeps through;
//   - the hero shot: Arthur at the burner, calm, the funnel taking the camp behind him, Javier flying past still playing his guitar.
// Every beat is logged (INTRO lines) with what worked: models, outfits, lines played, the balloon, and where the director
// had to move a camera.

enum RideMode { RIDE_STAY, RIDE_YEET, RIDE_POSED, RIDE_LOOSE };
struct CastDef
{
	const char* id; const char* model; const char* speaker;
	float x, y;                 // local camp position (x right, y toward the tornado)
	float fx, fy;               // local point it faces
	const char* scenario;       // what it's doing at camp (scenario), or
	const char* dict; const char* clip;   // a looping anim instead
	int ride; float liftAt;     // how and when the tornado takes it
	float rMul, h0, h1;         // posed riders: orbit radius (x the funnel), climb from h0 to h1 m
};
static const CastDef kCast[] = {
	// id        model                       speaker       x     y     faces        scenario / anim                                                  ride        lift   r    h0   h1
	{ "dutch",   "cs_dutch",                 "DUTCH",      0.0f, 3.0f,  16.0f, -10.0f, nullptr, "script_re@rally@rally", "base_leader",              RIDE_LOOSE, 80.0f, 0, 0, 0 },
	{ "micah",   "cs_micahbell",             "MICAH_BELL", 1.2f, -4.4f, 0.0f, 0.0f,  "WORLD_CAMP_FIRE_SIT_GROUND", nullptr, nullptr,                    RIDE_YEET,  69.0f, 0, 0, 0 },   // (v1.6: on the balloon's side of the fire - he runs for it)
	{ "bill",    "cs_billwilliamson",        "BILL",      -1.2f, -3.0f, 0.0f, 0.0f,  "WORLD_CAMP_FIRE_SIT_GROUND", nullptr, nullptr,                    RIDE_LOOSE, 79.0f, 0, 0, 0 },
	{ "javier",  "cs_javierescuella",        "JAVIER",     1.8f, -2.9f, 0.0f, 0.0f,  "WORLD_HUMAN_SIT_GUITAR", nullptr, nullptr,                        RIDE_POSED, 77.0f, 1.25f, 3.0f, 14.0f },
	{ "uncle",   "cs_uncle",                 "UNCLE",     -5.2f, 1.4f,  0.0f, 0.0f,  "WORLD_HUMAN_SLEEP_GROUND_ARM", nullptr, nullptr,                  RIDE_LOOSE, 75.4f, 0, 0, 0 },
	{ "pearson", "cs_mrpearson",             "PEARSON",    4.4f, 2.4f,  0.0f, 0.0f,  "WORLD_HUMAN_CAULDRON_STIR", nullptr, nullptr,                     RIDE_LOOSE, 78.2f, 0, 0, 0 },
	{ "strauss", "cs_leostrauss",            "STRAUSS",   -6.4f, -3.2f, -2.0f, -1.0f, "WORLD_HUMAN_WRITE_NOTEBOOK", nullptr, nullptr,                   RIDE_LOOSE, 79.6f, 0, 0, 0 },
	{ "cow",     "a_c_cow",                  "",         -15.0f, 8.0f, -10.0f, 12.0f, nullptr, nullptr, nullptr,                                        RIDE_POSED, 66.5f, 1.25f, 3.0f, 12.0f },
	{ "cain",    "a_c_dogcatahoulacur_01",   "",           2.3f, -0.7f, 0.0f, 0.0f,  "WORLD_ANIMAL_DOG_RESTING", nullptr, nullptr,                      RIDE_STAY,  0, 0, 0, 0 },
	// v1.5: three more of the gang
	{ "hosea",   "cs_hoseamatthews",         "HOSEA",      9.0f, -5.5f, 16.0f, -10.0f, nullptr, nullptr, nullptr,                                       RIDE_LOOSE, 81.0f, 0, 0, 0 },
	{ "john",    "cs_johnmarston",           "JOHN",      13.0f, -7.0f, 16.0f, -10.0f, nullptr, nullptr, nullptr,                                       RIDE_LOOSE, 80.4f, 0, 0, 0 },
	{ "lenny",   "cs_lenny",                 "LENNY",      2.6f, 1.6f,  0.0f, 0.0f,  "WORLD_CAMP_FIRE_SIT_GROUND", nullptr, nullptr,                    RIDE_POSED, 79.0f, 1.5f, 4.0f, 18.0f },
};
static const int kCastCount = sizeof(kCast) / sizeof(kCast[0]);
enum { CA_DUTCH, CA_MICAH, CA_BILL, CA_JAVIER, CA_UNCLE, CA_PEARSON, CA_STRAUSS, CA_COW, CA_CAIN, CA_HOSEA, CA_JOHN, CA_LENNY };

// The camp. Props are frozen until the tornado arrives; then they're fair game (it rips them loose like any prop).
struct PropDef { const char* model; const char* alt; float x, y, heading; bool flies; };
static const PropDef kCampProps[] = {
	{ "p_campfire02x", "p_campfire01x",         0.0f, 0.0f, 0, false },
	{ "p_gangtentlemoyne01x", "p_amb_tent01x",  -3.5f, 7.5f, 160, true },    // Dutch's tent, behind him
	{ "p_amb_tent01x", "p_amb_tent02x",        -9.0f, -2.0f, 75, true },
	{ "p_amb_tent02x", "p_amb_tent01x",         9.5f, -4.5f, -80, true },
	{ "p_phonograph01x", nullptr,              -1.6f, 6.0f, 200, true },     // Dutch's gramophone
	{ "p_gangtablemake01x", "p_table01x",      -6.6f, -2.0f, 90, true },     // Strauss's table
	{ "p_map01x", nullptr,                     -6.6f, -2.0f, 30, true },
	{ "p_lantern05x", "p_lantern04x",          -6.2f, -1.6f, 0, true },
	{ "p_cauldron01x", "p_pot01x",              4.4f, 1.4f, 0, true },       // Pearson's stew (the scenario may bring its own)
	{ "p_bedrollopen01x", "p_gen_bedrollopen01x", -6.6f, 2.8f, 10, true },   // Uncle's bedroll (v1.2: he naps in a chair beside it)
	{ "p_woodpile01x", nullptr,                 3.0f, 6.5f, 30, true },
	{ "p_crate03x", nullptr,                   -2.2f, 5.0f, 15, true },
	{ "p_barrel05x", nullptr,                   6.5f, 3.5f, 0, true },
	{ "p_barrel05x", nullptr,                   7.1f, 4.2f, 0, true },
	{ "p_chuckwagon01x", "p_chuckwagon02x",     9.0f, 7.5f, -30, false },    // Pearson's wagon
	{ "p_hitchingpost01x", nullptr,            -11.0f, 5.0f, 90, false },
	{ "p_bench_log01x", nullptr,               -2.2f, -1.8f, 60, false },
};
static const int kCampPropCount = sizeof(kCampProps) / sizeof(kCampProps[0]);

// v1.3: the lines are the characters' own (no invented subtitles). Two kinds:
//   - story lines: the character's real line from the game's story, by name - one line of one of Rockstar's scripted
//     conversations (text block, root, line index; research\story_lines_research.md §6-7). The words are known exactly. If the
//     game won't play it here (nothing is playing 1.2 s later), the fallback plays instead: fbWho's first ambient context;
//   - ambient lines: the first context of the list this voice has (their ambient banks), with the game's own subtitle asked for.
//     What they mean: research\speech_reactions_research.md and research\phrasebook_run12.md (what playtest 11's lines said).
// One story line at a time: a line that's due while the last one is still being said waits for it (up to 1.5 s), and so does an
// ambient line from the same mouth. That's the comedic timing: the answer comes when the other one's finished.
struct IntroLine
{
	float t; int who; const char* ctx[4]; bool shout;   // who: cast index, -1 Arthur. ctx: an ambient line (or a story line's fallback)
	const char* block; const char* root; int idx;       // a story line (block = its subtitles' text block), or none
	const char* words;                                  // what it says (for the log)
	int fbWho;                                          // who says the fallback (story lines)
	const char* audio;                                  // v1.5: the line's audio name in its scene's bank, if known (played beside the
	                                                    // camera when the speaker is too far off to be heard)
};
static const IntroLine kIntroLines[] = {
	// t       who          ambient contexts / the fallback                                                        shout  story line                                words                                                       fallback by  audio name
	{ 2.2f,  CA_DUTCH,   { nullptr },                                                                          false, "CFDV2AU", "CFDV2_ACT", 6,          "We have a plan. My friends.",                                CA_DUTCH,   "CFDV2_AAAG" },
	{ 6.0f,  CA_HOSEA,   { "WHATS_YOUR_PROBLEM", "PLAYER_ACTING_WEIRD" },                                      false, "RHUNT", "RH1_CAMP_WANDER", 0,      "Hey, where are you wandering off to?",                       CA_HOSEA,   nullptr },
	{ 10.3f, -1,         { "GREET_THIRD_FAREWELL_GENERAL_CONV", "GENERIC_GOODBYE" },                            false, "LCMPAUD", "LCMP_GOODBYE", 0,       "I'm leaving. Don't work too hard.",                          -1,         nullptr },
	{ 14.2f, CA_JOHN,    { "PLAYER_ACTING_WEIRD", "WHATS_YOUR_PROBLEM" },                                      false, "MUD4AUD", "MUD4_BADLY", 2,         "I thought you said you'd done this before?",                 CA_JOHN,    nullptr },
	{ 18.8f, -1,         { "TELLS_PED_TO_SHUT_UP", "GENERIC_CURSE_MED" },                                       false, "GNG2AUD", "GNG2_FTPI_BANT3", 3,    "Look, I'm trying to concentrate here.",                      -1,         nullptr },
	{ 22.6f, CA_BILL,    { "WRONGED_BY_PLAYER", "EXPLAIN_YOURSELF" },                                          false, "CWBV2AU", "CWBV2_ACT", 4,          "Ain't loyalty mean nothing to you?",                         CA_BILL,    "CWBV2_AAAE" },
	{ 26.4f, -1,         { "GREET_THIRD_BAD_WEATHER_CONV", "GREET_SECOND_BAD_WEATHER_CONV" },                   false, "CWBV2AU", "CWBV2_ACT", 5,          "I ain't dying for whatever nonsense Micah and Dutch have concocted...", -1, "CWBV2_AAAF" },
	{ 33.8f, CA_DUTCH,   { "PLAYER_ACTING_WEIRD", "EXPLAIN_YOURSELF", "GENERIC_INSULT_MED", "WHATS_YOUR_PROBLEM" }, false, "CWDS1AU", "CWDS1_ACT", 4,          "Faith, Arthur. Have a little faith.",                        CA_DUTCH,   "CWDS1_AAAE" },
	{ 37.8f, -1,         { "TELLS_PED_TO_SHUT_UP", "GENERIC_CURSE_MED" },                                       false, "CWDS1AU", "CWDS1_ACT", 1,          "If you say so.",                                             -1,         "CWDS1_AAAB" },
	{ 40.8f, CA_BILL,    { "PLAYER_ACTING_WEIRD", "WHATS_YOUR_PROBLEM" },                                      false, "CFBW3AU", "CFBW3_ACT", 5,          "There's nothing to worry about.",                            CA_BILL,    "CFBW3_AAAF" },   // - then the thunder
	{ 45.0f, CA_JAVIER,  { "GENERIC_SHOCKED_HIGH", "WHOA_ESCALATED", "EVENT_SHOCKED" },                         true,  nullptr, nullptr, 0,                   "(shocked)",                                                  0,          nullptr },
	{ 48.0f, CA_MICAH,   { "GENERIC_FRIGHTENED_HIGH", "GENERIC_CURSE_HIGH" },                                  true,  "UTP1AUD", "UTP1_IGOUT", 0,         "Reckon it's time we got out of here, Morgan.",               CA_MICAH,   nullptr },
	{ 52.9f, -1,         { "GREET_THIRD_GOOD_WEATHER_CONV", "GENERIC_GOODBYE" },                                false, "UTP1AUD", "UTP1_IGOUT", 1,         "Now you want get out of here?",                              -1,         nullptr },   // pulling the burner
	{ 57.0f, CA_DUTCH,   { "GENERIC_SHOCKED_DISBELIEF", "WRONGED_BY_PLAYER", "EXPLAIN_YOURSELF" },              true,  nullptr, nullptr, 0,                   "WHAT?!",                                                     0,          nullptr },
	{ 58.6f, CA_HOSEA,   { "WRONGED_BY_PLAYER", "WHATS_YOUR_PROBLEM" },                                        false, "HMR0AUD", "RH0_CMBT_OVER", 0,      "Was that really necessary, Arthur?",                         CA_HOSEA,   nullptr },
	{ 62.0f, -1,         { "GREET_THIRD_SORRY_TO_HEAR_THAT_CONV", "GENERIC_SHOCKED_MED" },                      false, "HMR0AUD", "RH0_CMBT_OVER", 1,      "If I had a dollar for every time I've been asked that.",     -1,         nullptr },
	{ 68.0f, CA_MICAH,   { "GENERIC_FRIGHTENED_HIGH", "GENERIC_CURSE_HIGH" },                                  true,  "CFMB5AU", "CFMB5_ACT", 12,         "Every man equal! Every man for himself!",                    CA_MICAH,   "CFMB5_AAAN" },
	{ 72.2f, CA_UNCLE,   { "GENERIC_CURSE_HIGH", "GENERIC_SHOCKED_HIGH" },                                      true,  "MUD3AUD", "MUD3_IG22_B", 1,        "Oh, I got lumbago, it's very serious!",                      CA_UNCLE,   nullptr },
	{ 76.4f, -1,         { "GENERIC_SHOCKED_MED" },                                                            false, "MUD3AUD", "MUD3_RE_LUM", 0,        "Lumbago. Really...",                                         -1,         nullptr },
	{ 79.2f, CA_PEARSON, { "GENERIC_FRIGHTENED_HIGH", "GENERIC_CURSE_HIGH" },                                   true,  "CFMP2AU", "CFMP2_ACT", 3,          "I'm designed to float.",                                     CA_PEARSON, "CFMP2_AAAD" },
	{ 83.3f, CA_JAVIER,  { "WHOA_ESCALATED", "GENERIC_FRIGHTENED_HIGH" },                                       true,  "WNT2AUD", "WNT2_CLIFF2", 0,        "Mierda, we're high up here.",                                CA_JAVIER,  nullptr },   // flying past, still playing
	{ 85.2f, -1,         { "GENERIC_SHOCKED_MED" },                                                            false, "WNT2AUD", "WNT2_CLIFF2", 1,        "You're telling me.",                                         -1,         nullptr },
	{ 88.4f, CA_DUTCH,   { "WRONGED_BY_PLAYER", "EXPLAIN_YOURSELF", "GENERIC_ANGRY_REACTION" },                  true,  "CDT26AU", "CDT26_ACT", 0,          "What is wrong with you, Arthur?",                            CA_DUTCH,   "CDT26_AAAA" },
	{ 91.6f, -1,         { "GREET_THIRD_SORRY_TO_HEAR_THAT_CONV", "GENERIC_SHOCKED_MED" },                      false, "CPGENAU", "CPGEN_CONF_GEN", 1,     "Sorry, this ain't a good time.",                             -1,         nullptr },   // his own close-up
	{ 95.5f, CA_LENNY,   { "GENERIC_SHOCKED_DISBELIEF", "WHOA_ESCALATED" },                                     false, "XCF1AUD", "XCF1_LEN_ELY", 2,       "I guess you know best, Arthur.",                             CA_LENNY,   nullptr },
};
static const int kIntroLineCount = sizeof(kIntroLines) / sizeof(kIntroLines[0]);
static void IntroStoryBlocks(void (*fn)(const char*));   // (the story lines' text blocks, below)

static std::vector<int> g_introFx;            // the camp fire
// v1.1 audit 2: what a finished scene handed to the world (IntroReleaseCast). Despawn everything still clears it - each one only
// if it still exists and is still the model we made (a handle can be reused by something else once the game deletes ours).
struct IntroLeft { Entity e; Hash model; bool ped; };
static std::vector<IntroLeft> g_introLeft;
// v1.5: about 100 s - the setup is a string of back-and-forths, the funnel waits out past the camp for the last words
static const float kIntroHandoff = 102.5f;     // the camera eases back to gameplay here
static const float kIntroThunder = 43.5f;      // right after "There's nothing to worry about."
static const float kIntroSpawn = 43.7f;        // the funnel drops out of the cloud, 110 m out
static const float kIntroLiftOff = 52.0f;      // Arthur pulls the burner ("Now you want get out of here?")
static const float kIntroSwoopHit = 54.0f;     // v1.6.1: ...and swoops the balloon low across the camp into Micah, then climbs away
static const float kIntroDutchPass = 88.95f;   // v1.6.1: Dutch crosses the slow-motion frame (1.55 s after the cut at 0.4)
static const float kIntroLunge = 63.0f;        // the tornado comes up to the camp's edge...
static const float kIntroSweep = 75.0f;        // ...and through it
static const float kIntroSlowMo0 = 87.4f, kIntroSlowMo1 = 90.3f;
static const float kIntroDutchThrow = 87.4f;   // v1.6: at the slow motion's cut - from 9 m off, across the frame (see Beat 11)
static const float kIntroTitle0 = 98.6f, kIntroTitle1 = 102.0f;
static const float kIntroSkipTo = 94.5f;       // a skip lands here: the last word, then the title
static const float kIntroTornadoDist = 110.0f;

// the intro's clock source (real time); the offline harness swaps in its mock clock
static DWORD (*g_introTick)() = []() -> DWORD { return GetTickCount(); };

struct CastState
{
	Ped ped = 0;
	bool ok = false, outfit = false, lifted = false, released = false;
	float ang = 0, r = 0, h = 0, rot = 0;      // posed: orbit state (around the tornado)
	V3 pos;
	int scenarioLostLogged = 0;
	bool flailing = false;          // (also set when a rider has its own in-the-air business: Uncle's chair, Pearson's pot, Javier's guitar)
	Entity held[2] = { 0, 0 };      // v1.2: what goes up with them (Uncle's chair, Pearson's pot and ladle, Javier's guitar)
	Entity carrier = 0;             // v1.6.1: posed riders ride an invisible carrier (a ped frozen in mid-air doesn't animate)
	float releasedAt = -1;          // v1.2: thrown out of the funnel - then they get up and walk it off
	bool walked = false;
};
struct IntroState
{
	int stage = 0;               // 0 off, 1 fading out, 2 building, 3 settling, 4 scene, 5 after the handoff
	float stageAt = 0;
	double clock = 0;            // seconds since the fade-in (real time, pauses with the game)
	DWORD lastTick = 0;
	V3 C;                        // camp centre (ground)
	float fwdH = 0;              // heading toward the tornado
	V3 F, R;                     // unit vectors: toward the tornado, to the right
	CastState cast[kCastCount];
	std::vector<Entity> props;
	TornadoRef tp;           // the intro's tornado
	int cam = 0;
	bool linePlayed[kIntroLineCount] = {};
	int storyLine = -1;          // v1.3: the story line being said (kIntroLines index), or -1
	float storyAt = 0;           //   when it was started (scene clock)
	bool storySeen = false;      //   the game said it's playing
	int storyPlayed = 0, storyFell = 0;   // story lines heard / that fell back to an ambient line
	std::vector<int> firedBeats;
	float letterbox = 0;         // 0..1
	float timeScale = 1;
	bool savedGod = true, savedGrab = true, savedPeople = true, savedAnimals = true;
	int savedArthur = 2, savedMove = 2, savedSpeed = 1, savedHour = 12, savedMin = 0;
	bool savedTouchCam = true, savedRideCam = true;
	bool deadEye = false, honor = false;
	float afterStart = 0;
	int handoffDone = 0;
	std::string grade;
	float gradeStrength = 0.8f;
	bool skipRequested = false;
	int camShot = -1;
	Ped horses[2] = { 0, 0 };   // Arthur's horse(s): moved out of the way and kept out of the tornado's hands
	Hash savedLock = 0;         // the weather lock before the intro (the user's own, if savedManual)
	bool savedManual = false;
	int oldCam = 0;             // the last shot's camera, destroyed once the ease back to gameplay is done
	float gradeK = 0;           // the grade fading out after the handoff
	// v1.4 the camera director: the shot it chose for, and how it moved the camera to see the subject
	int dirShot = -1;
	float dirYaw = 0, dirLift = 0, dirPull = 1, dirCheckAt = 0, dirLastT = 0;
	float dirYawS = 0, dirLiftS = 0, dirPullS = 1;   // the correction as it's applied (glides to the chosen one)
	int dirLogged = -1;         // the shot whose "nothing clear" was logged (once per shot)
	V3 dirPos;
	int dirMoves = 0;           // shots where the wanted spot couldn't see its subject
	// v1.5 the ears (playtest 13: "give ample time for the 'ears' of the game camera to catch up to the dialogue" - and lines from
	// far-off characters couldn't be heard): when the shot changed, when the last line started and who said it
	float cutAt = 0, lineAt = -100;
	DWORD nearUntilMs = 0;      // a line played beside the camera is still being said until then (real time: a held cut stops the scene clock)
	Ped ambPed = 0;
	float ambAt = -100;
	float holdHere = 0, holdTotal = 0;
	int nearLines = 0;          // lines played beside the camera
	V3 liftAnchor;              // v1.6: where the balloon stood when the lift-off shot began (the camera stays on the ground)
	V3 micahAnchor;             // v1.6.1: where Micah stood then (the swoop's other end)
	bool swoop = false, swoopHit = false;   // v1.6.1: the balloon skimming across the camp into Micah; it got him
	float climbUntil = -1;      // v1.6.1: after the hit, up and away past him until then
	V3 swoopDir;
	Ped pushPed = 0; V3 pushVel; float pushUntil = -1;   // v1.6.1: a knock held for a few frames, so the ragdoll takes it
	float swoopFollow = 0.5f;   // v1.6.1 review: shot 13's tilt after the balloon, eased
	V3 dutchDir; float dutchSpeed = 0;   // v1.6.1 review: Dutch's line and speed - steered in direction only, never braked
	V3 dutchFrom; bool dutchRetried = false, dutchLogged = false;   // v1.6.2: where he's put; moved again if he wasn't; the pass logged
	bool blockReq[kIntroLineCount] = {}, blockGone[kIntroLineCount] = {};   // v1.5: subtitles asked for 8 s ahead, let go after
} g_in;

bool IntroRunning() { return g_in.stage >= 1 && g_in.stage <= 4; }
bool IntroActive() { return g_in.stage != 0; }
// v1.1 audit 2: the scene's grade is the intro's until it has faded out after the handoff (the Dark sky waits for it)
bool IntroOwnsGrade() { return IntroRunning() || g_in.gradeK > 0; }

// The intro let go of the timecycle modifier: the Dark sky (if it's wanted) eases back in from nothing
static void IntroClearGrade()
{
	GRAPHICS::CLEAR_TIMECYCLE_MODIFIER();
	g_in.gradeK = 0;
	g_storm.dark = 0;
	g_storm.darkApplied = false;
}

static V3 IL(float x, float y, float z = 0)   // local -> world (z above the camp's ground)
{
	return g_in.C + g_in.R * x + g_in.F * y + V3(0, 0, z);
}
static V3 ILg(float x, float y, float up)       // local -> world, on the ground there
{
	V3 p = IL(x, y);
	p.z = GroundZ(p.x, p.y, g_in.C.z + 25.0f, g_in.C.z) + up;
	return p;
}
static float HeadingTo(const V3& from, const V3& to) { V3 d = to - from; return atan2f(-d.x, d.y) * 180.0f / PI; }
static Tornado* ITp() { return g_in.tp.get(); }   // the intro's tornado, if it still exists
// v1.3: where the funnel settles - the middle of camp (the fire); Arthur's in the balloon, and it doesn't chase him
static V3 IntroSettleSpot()
{
	return IL(0.0f, 1.0f);
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

static const char* kIntroDicts[] = {
	"script_re@rally@rally", "ai_react@point@base", "amb_temp@code_human_cower@male@base",
	// v1.2 (research\intro_extras_research.md): the @upper emotes play over a seated pose (the @full ones fight it)
	// v1.6 (playtest 14's log: the script_mp@ emotes never loaded in story mode - six intros, each held 6 s on a black screen
	// waiting for them): Arthur's own conversation gestures - the hat tip, a rub of the eyes ("If you say so."), a cocky nod
	"ai_gestures@arthur@standing@speaker",
	"amb_camp@prop_camp_cauldron_serve_stew@male_b@base",  // v1.6: Pearson at his stew, when the stirring scenario won't start
	"amb_camp@world_camp_fire_sit_ground@male_a@idle_a",   // v1.6: Lenny, still sitting by the fire - in the air
	"amb_camp@prop_camp_bill_sleep@male_a@base",           // Uncle asleep in a chair (Bill's camp nap)
	"script_story@gng2@ig@ig_2_balloon_control",           // Arthur at the burner
	// v1.5 review: what a posed rider plays if its scenario drops in the air (Javier keeps playing; the others flail)
	"amb_misc@world_human_sit_guitar@male_a@idle_a",
	"script_story@gng2@ig@ig_10_sadie_climb_balloon_rope",
};
// v1.6 review: the stand-ins - only needed if something else doesn't take - and the gestures aren't waited for at the start
static const char* kIntroDictsOptional[] = {
	"ai_gestures@arthur@standing@speaker", "amb_camp@prop_camp_cauldron_serve_stew@male_b@base",
	"amb_camp@world_camp_fire_sit_ground@male_a@idle_a", "amb_misc@world_human_sit_guitar@male_a@idle_a",
	"script_story@gng2@ig@ig_10_sadie_climb_balloon_rope",
};
static bool IntroDictOptional(const char* d)
{
	for (const char* o : kIntroDictsOptional) if (strcmp(o, d) == 0) return true;
	return false;
}
// v1.2: the props that go up with the gang
static const char* kIntroExtraModels[] = { "p_chairfolding02x", "p_kettle03x", "p_ladle04x", "p_guitar01x" };
// v1.2 the user: "twisted custom outfits". Named outfits Rockstar dresses them in elsewhere (research §1): Uncle in the long
// johns from "Uncle's Bad Day", Dutch in his party suit from the Saint Denis gala - for a night round the campfire.
struct GagOutfit { int cast; Hash outfit; const char* name; Hash fallback; };
static const GagOutfit kGagOutfits[] = {
	{ CA_UNCLE, 0xB93CB089, "ENDLESS_SUMMER_COOKED_ALT (his long johns)", 0x79155250 },
	{ CA_DUTCH, 0x84793D7F, "GALA (the party suit)", 0 },
};

// upper body only, over whatever they're doing (Rockstar's most common flags for it: upper body + secondary)
static void PlayUpper(Ped p, const char* dict, const char* clip)
{
	if (!p || !dict || !STREAMING::HAS_ANIM_DICT_LOADED(dict)) return;
	TASK::TASK_PLAY_ANIM(p, dict, clip, 4.0f, -4.0f, -1, 16 | 8, 0.0f, FALSE, 0, FALSE, nullptr, FALSE);
}

static Entity SpawnHeld(const char* model, const V3& at)
{
	Hash m = H(model);
	if (!STREAMING::HAS_MODEL_LOADED(m)) return 0;
	Entity o = OBJECT::CREATE_OBJECT(m, at.x, at.y, at.z, FALSE, FALSE, TRUE, FALSE, FALSE);
	if (!o) return 0;
	ENTITY::SET_ENTITY_COLLISION(o, FALSE, FALSE);
	ENTITY::SET_ENTITY_INVINCIBLE(o, TRUE);
	SetScripted(o, true);
	g_in.props.push_back(o);   // cleaned up with the camp
	return o;
}
// on a hand bone (PH_L_Hand 37709, PH_R_Hand 7966), or in front of the chest if the bone isn't there
static void HoldInHand(Ped p, Entity o, int boneId, const V3& off)
{
	if (!p || !o) return;
	int bone = PED::GET_PED_BONE_INDEX(p, boneId);
	ENTITY::ATTACH_ENTITY_TO_ENTITY(o, p, bone >= 0 ? bone : 0, bone >= 0 ? 0.0f : off.x, bone >= 0 ? 0.0f : off.y, bone >= 0 ? 0.0f : off.z,
		0, 0, 0, FALSE, FALSE, FALSE, FALSE, 2, TRUE, FALSE, FALSE);
}

// ---------- the cast ----------
static Ped SpawnCast(int i)
{
	const CastDef& d = kCast[i];
	Hash m = H(d.model);
	if (!LoadModel(m, 5000))
	{
		Log("INTRO cast %s: model %s would not load", d.id, d.model);
		return 0;
	}
	V3 p = ILg(d.x, d.y, 0.05f);
	V3 face = IL(d.fx, d.fy);
	Ped ped = PED::CREATE_PED(m, p.x, p.y, p.z, HeadingTo(p, face), FALSE, FALSE, FALSE, FALSE);
	STREAMING::SET_MODEL_AS_NO_LONGER_NEEDED(m);
	if (!ped)
	{
		Log("INTRO cast %s: CREATE_PED failed for %s", d.id, d.model);
		return 0;
	}
	// dressed the way Rockstar's camp script does it (without an outfit a spawned story ped is invisible)
	Hash outfit = Joaat("META_OUTFIT_WARM_WEATHER");
	const char* outfitName = "META_OUTFIT_WARM_WEATHER";
	if (IniUpper("Intro", "Outfits", "GAG") != "STORY")
		for (auto& g : kGagOutfits)
			if (g.cast == i)
			{
				if (PED::DOES_META_PED_OUTFIT_EXIST_FOR_PED_MODEL(g.outfit, m)) { outfit = g.outfit; outfitName = g.name; }
				else if (g.fallback && PED::DOES_META_PED_OUTFIT_EXIST_FOR_PED_MODEL(g.fallback, m)) { outfit = g.fallback; outfitName = "the gag outfit's fallback"; }
			}
	bool named = PED::DOES_META_PED_OUTFIT_EXIST_FOR_PED_MODEL(outfit, m) != 0;
	if (named) PED::EQUIP_META_PED_OUTFIT(ped, outfit);
	else PED::SET_RANDOM_OUTFIT_VARIATION(ped, TRUE);
	PED::UPDATE_PED_VARIATION(ped, FALSE, TRUE, TRUE, TRUE, FALSE);
	g_in.cast[i].outfit = named;
	ENTITY::SET_ENTITY_AS_MISSION_ENTITY(ped, TRUE, TRUE);
	ENTITY::SET_ENTITY_INVINCIBLE(ped, TRUE);
	if (PED::IS_PED_HUMAN(ped))
		PED::SET_PED_RELATIONSHIP_GROUP_HASH(ped, Joaat("REL_GANG_DUTCHS"));
	PED::SET_BLOCKING_OF_NON_TEMPORARY_EVENTS(ped, TRUE);
	PED::SET_PED_KEEP_TASK(ped, TRUE);
	static const int kCalm[] = { 113, 217, 17, 15, 26, 111, 130, 174, 286 };   // see cutscene_research.md 1.3
	for (int f : kCalm) PED::SET_PED_CONFIG_FLAG(ped, f, TRUE);
	PED::SET_PED_CONFIG_FLAG(ped, 168, FALSE);
	SetScripted(ped, true);
	ENTITY::SET_ENTITY_LOD_DIST(ped, 600);
	Log("INTRO cast %s (%s): ped %d at local (%.1f, %.1f), outfit %s", d.id, d.model, ped, d.x, d.y, named ? outfitName : "random variation");
	return ped;
}

static void StartActivity(int i)
{
	const CastDef& d = kCast[i];
	Ped p = CastPed(i);
	if (!p) return;
	if (i == CA_UNCLE)
	{
		// v1.2: asleep in a camp chair (there's no "sleep in a chair" scenario: Bill's camp nap, on a folding chair)
		V3 at = ILg(d.x, d.y, 0.0f);
		Entity chair = SpawnHeld("p_chairfolding02x", at);
		if (chair && STREAMING::HAS_ANIM_DICT_LOADED("amb_camp@prop_camp_bill_sleep@male_a@base"))
		{
			ENTITY::SET_ENTITY_HEADING(chair, HeadingTo(at, at + g_in.F));
			OBJECT::PLACE_OBJECT_ON_GROUND_PROPERLY(chair, FALSE);
			ENTITY::FREEZE_ENTITY_POSITION(chair, TRUE);
			ENTITY::ATTACH_ENTITY_TO_ENTITY(p, chair, 0, 0, 0, 0.5f, 0, 0, 180.0f, FALSE, FALSE, FALSE, FALSE, 2, TRUE, FALSE, FALSE);
			// v1.6 (playtest 14: his close-up only ever saw the back of the chair): he faces away from where the funnel comes
			// from, so his shot has his face and the funnel behind him (turned round if the chair put him the other way)
			V3 hf = HeadingDir(ENTITY::GET_ENTITY_HEADING(p));
			if (hf.x * g_in.F.x + hf.y * g_in.F.y > 0.0f)
				ENTITY::SET_ENTITY_HEADING(chair, HeadingTo(at, at - g_in.F));
			PlayAnim(p, "amb_camp@prop_camp_bill_sleep@male_a@base", "base", true, 8.0f);
			g_in.cast[i].held[0] = chair;
			g_in.cast[i].flailing = true;   // no scenario to lose: he's in his chair
			Log("INTRO Uncle naps in a chair");
			return;
		}
	}
	if (d.scenario)
		TASK::TASK_START_SCENARIO_IN_PLACE_HASH(p, Joaat(d.scenario), -1, FALSE, 0, -1.0f, FALSE);
	else if (d.dict)
		PlayAnim(p, d.dict, d.clip, true, 8.0f);
	else if (PED::IS_PED_HUMAN(p))
		TASK::TASK_STAND_STILL(p, -1);   // v1.5: Hosea and John stand watching the balloon
}

static void BuildCamp()
{
	int ok = 0, fail = 0;
	for (int i = 0; i < kCampPropCount; i++)
	{
		const PropDef& d = kCampProps[i];
		const char* name = d.model;
		Hash m = H(name);
		if (!LoadModel(m, 2500) && d.alt) { name = d.alt; m = H(name); if (!LoadModel(m, 2500)) m = 0; }
		else if (!STREAMING::HAS_MODEL_LOADED(m)) m = 0;
		if (!m) { fail++; Log("INTRO prop %s would not load", d.model); continue; }
		V3 p = ILg(d.x, d.y, 0.0f);
		Object o = OBJECT::CREATE_OBJECT(m, p.x, p.y, p.z, FALSE, FALSE, TRUE, FALSE, FALSE);
		STREAMING::SET_MODEL_AS_NO_LONGER_NEEDED(m);
		if (!o) { fail++; continue; }
		ENTITY::SET_ENTITY_HEADING(o, g_in.fwdH + d.heading);
		OBJECT::PLACE_OBJECT_ON_GROUND_PROPERLY(o, FALSE);
		ENTITY::FREEZE_ENTITY_POSITION(o, TRUE);
		ENTITY::SET_ENTITY_LOD_DIST(o, 500);
		if (!d.flies) SetScripted(o, true);   // the fire, the wagon and the hitching post stay put
		g_in.props.push_back(o);
		ok++;
	}
	// v1.1 audit 2: IntroBuild asked for every prop's alternative too, and the unused ones stayed loaded - let them all go
	for (int i = 0; i < kCampPropCount; i++)
	{
		STREAMING::SET_MODEL_AS_NO_LONGER_NEEDED(H(kCampProps[i].model));
		if (kCampProps[i].alt) STREAMING::SET_MODEL_AS_NO_LONGER_NEEDED(H(kCampProps[i].alt));
	}
	// a real fire in the fire pit (the prop may or may not have its own flame)
	if (LoadPtfxAsset("core", 1000))
	{
		V3 f = ILg(0, 0, 0.15f);
		GRAPHICS::USE_PARTICLE_FX_ASSET("core");
		int h = GRAPHICS::START_PARTICLE_FX_LOOPED_AT_COORD("ent_amb_campfire_sma", f.x, f.y, f.z, 0, 0, 0, 1.0f, FALSE, FALSE, FALSE, FALSE);
		if (h) { g_introFx.push_back(h); AddLoopsInUse(1); }
	}
	Log("INTRO camp: %d props placed, %d would not load", ok, fail);
}

// ---------- posed riders: frozen and flown round the funnel, still doing what they were doing ----------
static void LiftRider(int i, float t)
{
	CastState& c = g_in.cast[i];
	if (!c.ok || c.lifted) return;
	c.lifted = true;
	const CastDef& d = kCast[i];
	Ped p = c.ped;
	c.pos = ENTITY::GET_ENTITY_COORDS(p, FALSE, FALSE);
	if (d.ride == RIDE_YEET)
	{
		// straight up and gone (the tornado always takes Micah first)
		ENTITY::FREEZE_ENTITY_POSITION(p, FALSE);
		TASK::CLEAR_PED_TASKS_IMMEDIATELY(p, FALSE, TRUE);
		PED::SET_PED_TO_RAGDOLL(p, 12000, 15000, 0, FALSE, FALSE, nullptr);
		V3 toT = ITp() ? (ITp()->base - c.pos) : g_in.F * 40.0f;
		float L = std::max(1.0f, toT.len2d());
		ENTITY::SET_ENTITY_VELOCITY(p, toT.x / L * 6.0f, toT.y / L * 6.0f, 46.0f);
		Log("INTRO %s launched", d.id);
		return;
	}
	if (d.ride == RIDE_LOOSE)
	{
		// v1.3: handed to the tornado's own physics (ragdoll whirl - playtest 11: "use ragdoll"), with their things
		if (i == CA_UNCLE && c.held[0] && ENTITY::DOES_ENTITY_EXIST(c.held[0]))
		{
			// the chair goes with him, stuck to his backside
			ENTITY::DETACH_ENTITY(p, FALSE, FALSE);
			ENTITY::FREEZE_ENTITY_POSITION(c.held[0], FALSE);
			int pelvis = PED::GET_PED_BONE_INDEX(p, 11816);
			ENTITY::ATTACH_ENTITY_TO_ENTITY(c.held[0], p, pelvis >= 0 ? pelvis : 0, 0, 0, pelvis >= 0 ? -0.1f : -0.5f, 0, 0, 180.0f, FALSE, FALSE, FALSE, FALSE, 2, TRUE, FALSE, FALSE);
		}
		if (i == CA_PEARSON)
		{
			V3 at = c.pos + V3(0, 0, 1.0f);
			c.held[0] = SpawnHeld("p_kettle03x", at);
			c.held[1] = SpawnHeld("p_ladle04x", at);
			HoldInHand(p, c.held[0], 37709, V3(0.0f, 0.35f, 0.1f));
			HoldInHand(p, c.held[1], 7966, V3(0.15f, 0.35f, 0.25f));
		}
		if (i == CA_JAVIER && (c.held[0] = SpawnHeld("p_guitar01x", c.pos + V3(0, 0, 1.0f))) != 0)
			HoldInHand(p, c.held[0], 7966, V3(0.0f, 0.3f, 0.1f));
		TASK::CLEAR_PED_TASKS_IMMEDIATELY(p, FALSE, TRUE);
		SetScripted(p, false);
		PED::SET_PED_TO_RAGDOLL(p, 4000, 6000, 0, FALSE, FALSE, nullptr);
		V3 toT = ITp() ? (ITp()->base - c.pos) : g_in.F * 40.0f;
		float L = std::max(1.0f, toT.len2d());
		ENTITY::SET_ENTITY_VELOCITY(p, toT.x / L * 14.0f, toT.y / L * 14.0f, 9.0f);
		Log("INTRO %s handed to the tornado", d.id);
		return;
	}
	// posed
	// v1.6.1 (playtest 15: Lenny still went round stiff with his sitting clip playing - a ped frozen in mid-air doesn't animate):
	// they ride an invisible carrier, attached the way Uncle sits in his chair (which animates fine), and the carrier is flown
	c.carrier = SpawnHeld("p_kettle03x", c.pos);
	if (c.carrier)
	{
		ENTITY::SET_ENTITY_VISIBLE(c.carrier, FALSE);
		ENTITY::FREEZE_ENTITY_POSITION(c.carrier, TRUE);
		ENTITY::ATTACH_ENTITY_TO_ENTITY(p, c.carrier, 0, 0, 0, 0, 0, 0, 0, FALSE, FALSE, FALSE, FALSE, 2, TRUE, FALSE, FALSE);
	}
	else
		ENTITY::FREEZE_ENTITY_POSITION(p, TRUE);
	PED::SET_PED_CAN_RAGDOLL(p, FALSE);
	V3 rel = c.pos - (ITp() ? ITp()->base : c.pos);
	c.ang = atan2f(rel.y, rel.x);
	c.r = std::max(3.0f, rel.len2d());
	c.h = ITp() ? c.pos.z - ITp()->base.z : 0.0f;   // from where they are (audit: they used to start inside the ground)
	c.rot = ENTITY::GET_ENTITY_HEADING(p);
	if (i == CA_DUTCH) PlayAnim(p, "script_re@rally@rally", "action_leader", true);
	if (i == CA_UNCLE && c.held[0] && ENTITY::DOES_ENTITY_EXIST(c.held[0]))
	{
		// the chair comes too: he's off it and it's under him, still asleep, for the rotisserie
		ENTITY::DETACH_ENTITY(p, FALSE, FALSE);
		ENTITY::FREEZE_ENTITY_POSITION(c.held[0], FALSE);
		ENTITY::ATTACH_ENTITY_TO_ENTITY(c.held[0], p, 0, 0, 0, -0.5f, 0, 0, 180.0f, FALSE, FALSE, FALSE, FALSE, 2, TRUE, FALSE, FALSE);
		PlayAnim(p, "amb_camp@prop_camp_bill_sleep@male_a@base", "base", true, 2.0f);
	}
	if (i == CA_PEARSON)
	{
		// the stew pot in one hand, the ladle in the other, still stirring
		V3 at = c.pos + V3(0, 0, 1.0f);
		c.held[0] = SpawnHeld("p_kettle03x", at);
		c.held[1] = SpawnHeld("p_ladle04x", at);
		HoldInHand(p, c.held[0], 37709, V3(0.0f, 0.35f, 0.1f));
		HoldInHand(p, c.held[1], 7966, V3(0.15f, 0.35f, 0.25f));
		PlayAnim(p, "amb_wander@code_human_cauldron_wander@base", "base", true, 4.0f);
		c.flailing = true;
		Log("INTRO Pearson goes up stirring (pot %d, ladle %d)", c.held[0], c.held[1]);
	}
	// (v1.1 audit 2: the cow no longer gets Sadie's human flailing clip - it's another skeleton)
	Log("INTRO %s lifted (posed, %s)", d.id, d.scenario ? (PED::IS_PED_USING_ANY_SCENARIO(p) ? "scenario running" : "scenario NOT running") : "anim");
}

static void UpdateRiders(float dt, float t, bool after)
{
	Tornado* tp = ITp();
	if (!tp) return;
	for (int i = 0; i < kCastCount; i++)
	{
		CastState& c = g_in.cast[i];
		const CastDef& d = kCast[i];
		if (!c.ok || !c.lifted || c.released || d.ride != RIDE_POSED) continue;
		Ped p = c.ped;
		if (!ENTITY::DOES_ENTITY_EXIST(p)) { c.ok = false; continue; }
		float age = t - d.liftAt;
		// pulled in over ~2 s (rising), then round and round, climbing toward h1
		float wantR = std::max(4.0f, tp->radiusAt(Clamp(c.h / tp->height(), 0.0f, 1.0f)) * d.rMul + 2.0f);
		c.r += Clamp(wantR - c.r, -14.0f * dt, 14.0f * dt);
		float wantH = Lerp(d.h0, d.h1, Clamp(age / 9.0f, 0.0f, 1.0f)) + sinf(t * 1.3f + i) * 1.5f;
		if (i == CA_DUTCH && t > 91.0f) wantH = Lerp(d.h1, 70.0f, Clamp((t - 91.0f) / 2.4f, 0.0f, 1.0f));   // TAHITI (and he stays up there: v1.2)
		c.h += Clamp(wantH - c.h, -8.0f * dt, 10.0f * dt);
		float slow = (i == CA_LENNY && !after && t >= 94.0f && t < 98.8f) ? 0.35f : 1.0f;   // (v1.5: Lenny's last word, close up)
		c.ang += std::min(1.25f + (i % 3) * 0.12f, 22.0f / std::max(c.r, 1.0f)) * dt * slow;   // never faster than 22 m/s round the funnel
		V3 pos = tp->base + V3(cosf(c.ang) * c.r, sinf(c.ang) * c.r, c.h);
		if (i == CA_JAVIER && !after && t >= 82.3f && t < 85.3f)
		{
			// v1.5 the hero shot: Javier sails past the basket between Arthur and the funnel, still playing his guitar
			V3 a = PlayerPos();
			V3 f = tp->base - a; f.z = 0; float L = std::max(0.1f, f.len2d()); f = V3(f.x / L, f.y / L, 0);
			V3 sd(-f.y, f.x, 0);
			float u = (t - 82.3f) / 3.0f;
			pos = a + f * 5.0f + sd * Lerp(-7.0f, 7.0f, u) + V3(0, 0, Lerp(-0.5f, 1.5f, u));
			V3 rel = pos - tp->base;
			c.ang = atan2f(rel.y, rel.x); c.r = std::max(3.0f, rel.len2d()); c.h = rel.z;
		}
		if (i == CA_COW && !after && t >= 69.5f && t < 70.9f)
		{
			// the cow's moment: it drifts past the balloon, right across the over-the-shoulder shot (shot 12), then back into the funnel
			V3 a = PlayerPos();
			V3 f = tp->base - a; f.z = 0; float L = std::max(0.1f, f.len2d()); f = V3(f.x / L, f.y / L, 0);
			V3 sd(-f.y, f.x, 0);
			float u = (t - 69.5f) / 1.4f;
			pos = a + f * 7.5f + sd * Lerp(-9.0f, 9.0f, u) + V3(0, 0, Lerp(-1.0f, 0.6f, u));
			V3 rel = pos - tp->base;
			c.ang = atan2f(rel.y, rel.x); c.r = std::max(3.0f, rel.len2d()); c.h = rel.z;
		}
		c.pos = pos;
		float tangent = c.ang * 180.0f / PI;   // facing along the spin
		if (i == CA_DUTCH && !after && t >= 85.0f && t < 87.6f) tangent = HeadingTo(pos, PlayerPos());   // v1.2: pointing at Arthur as he whirls past
		c.rot = tangent;
		float roll = i == CA_UNCLE ? fmodf(t * 70.0f, 360.0f) : sinf(t * 2.0f + i) * 12.0f;   // Uncle: the rotisserie
		if (c.carrier && !ENTITY::DOES_ENTITY_EXIST(c.carrier)) { c.carrier = 0; ENTITY::FREEZE_ENTITY_POSITION(p, TRUE); }   // (review: lost - frozen, as before)
		Entity mover = c.carrier ? c.carrier : p;   // (v1.6.1: the carrier, if they have one)
		ENTITY::SET_ENTITY_COORDS_NO_OFFSET(mover, pos.x, pos.y, pos.z, FALSE, FALSE, FALSE);
		ENTITY::SET_ENTITY_ROTATION(mover, sinf(t * 1.7f + i) * 10.0f, roll, tangent, 2, TRUE);
		// a rider whose scenario dropped (attaching/moving can end it) flails instead of standing stiff
		if (d.scenario && !c.flailing && age > 1.0f && !PED::IS_PED_USING_ANY_SCENARIO(p))
		{
			c.flailing = true;
			if (i == CA_JAVIER && (c.held[0] = SpawnHeld("p_guitar01x", pos)) != 0)
			{
				// v1.2: Javier keeps playing (his guitar went with the scenario - here's another)
				HoldInHand(p, c.held[0], 7966, V3(0.0f, 0.3f, 0.1f));
				PlayAnim(p, "amb_misc@world_human_sit_guitar@male_a@idle_a", "idle_a", true);
				Log("INTRO javier: the guitar scenario stopped in the air (after %.1f s) - playing on with a new guitar", age);
			}
			else if (i == CA_LENNY)
			{
				// v1.6 (playtest 14: he went round stiff as a board - the flailing clip didn't take): still sitting by the fire, calm
				// as you like, as he goes round - "I guess you know best, Arthur."
				PlayAnim(p, "amb_camp@world_camp_fire_sit_ground@male_a@idle_a", "idle_a", true);
				Log("INTRO lenny: the %s scenario stopped in the air (after %.1f s) - still sitting, in the air", d.scenario, age);
			}
			else
			{
				PlayAnim(p, "script_story@gng2@ig@ig_10_sadie_climb_balloon_rope", "sadie_legs_flailing_mrsadler", true);
				Log("INTRO %s: the %s scenario stopped in the air (after %.1f s) - flailing instead", d.id, d.scenario, age);
			}
		}
		PED::REQUEST_PED_FACIAL_MOOD_THIS_FRAME(p, 4127737830u /*MoodWindExtreme*/, 6);
	}
}

static void ReleaseRider(int i, float t)
{
	CastState& c = g_in.cast[i];
	if (!c.ok || c.released) return;
	c.released = true;
	Ped p = c.ped;
	if (!ENTITY::DOES_ENTITY_EXIST(p)) return;
	if (c.carrier)
	{
		// (v1.6.1: off the carrier; v1.6.1 review: and it's deleted - left frozen in mid-air, a tornado or the sweeper would find it)
		ENTITY::DETACH_ENTITY(p, FALSE, FALSE);
		Entity cr = c.carrier;
		g_in.props.erase(std::remove(g_in.props.begin(), g_in.props.end(), cr), g_in.props.end());
		if (ENTITY::DOES_ENTITY_EXIST(cr)) { ENTITY::SET_ENTITY_AS_MISSION_ENTITY(cr, TRUE, TRUE); Object o = cr; OBJECT::DELETE_OBJECT(&o); }
		c.carrier = 0;
	}
	ENTITY::FREEZE_ENTITY_POSITION(p, FALSE);
	PED::SET_PED_CAN_RAGDOLL(p, TRUE);
	TASK::CLEAR_PED_TASKS_IMMEDIATELY(p, FALSE, TRUE);
	PED::SET_PED_TO_RAGDOLL(p, 5000, 8000, 0, FALSE, FALSE, nullptr);
	if (Tornado* tp = ITp())
	{
		V3 rel = c.pos - tp->base;
		float L = std::max(1.0f, rel.len2d());
		ENTITY::SET_ENTITY_VELOCITY(p, rel.x / L * 26.0f, rel.y / L * 26.0f, 8.0f);
	}
	SetScripted(p, false);   // fair game again - the tornado may well pick them back up
	for (Entity& h : c.held)
		if (h && ENTITY::DOES_ENTITY_EXIST(h))
		{
			ENTITY::DETACH_ENTITY(h, TRUE, TRUE);
			ENTITY::SET_ENTITY_COLLISION(h, TRUE, TRUE);
			ENTITY::SET_ENTITY_VELOCITY(h, RandRange(-12.f, 12.f), RandRange(-12.f, 12.f), 10.0f);
			SetScripted(h, false);
		}
	c.releasedAt = t;
	Log("INTRO %s released from the funnel", kCast[i].id);
}

// ---------- the scene ----------
static void IntroCleanupCams()
{
	if (g_in.cam && CAMERA::DOES_CAM_EXIST(g_in.cam))
	{
		CAMERA::SET_CAM_ACTIVE(g_in.cam, FALSE);
		CAMERA::DESTROY_CAM(g_in.cam, FALSE);
	}
	g_in.cam = 0;
}

static void IntroStoryEnd();
static void IntroRestoreLook(bool fadeGrade = false)
{
	MISC::SET_TIME_SCALE(1.0f);
	IntroStoryEnd();
	AUDIO::SET_AUDIO_FLAG("AllowScriptedSpeechInSlowMo", FALSE);
	if (g_in.deadEye) { GRAPHICS::ANIMPOSTFX_STOP("DEADEYE"); g_in.deadEye = false; }
	if (!g_in.grade.empty())
	{
		if (fadeGrade) g_in.gradeK = g_in.gradeStrength;   // faded out over the handoff (stage 5)
		else IntroClearGrade();
	}
	HUD::DISPLAY_HUD(TRUE);
	MAP::DISPLAY_RADAR(TRUE);
}

// v1.1 audit: the scene locked THUNDER - give the weather back to whoever owned it
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
		UnlockWeather();   // Weather override Off: your own weather again
		g_storm.lockedByStorm = false;
	}
	else
	{
		g_storm.lockedByStorm = true;   // the tornado's storm owns it now, and lets go when the storm's over
		g_storm.appliedWeather = -1;
	}
	MISC::SET_RAIN(-1.0f);   // the storm holds it off again next frame if it's Storm clouds
	g_storm.rainHeld = false;
}

// After the scene (or an abort): the gang and the camp belong to the world now - the game tidies them up when you're away.
static void IntroReleaseCast()
{
	for (auto& c : g_in.cast)
	{
		if (c.ped && ENTITY::DOES_ENTITY_EXIST(c.ped))
		{
			SetScripted(c.ped, false);
			if (!c.released)
			{
				if (c.carrier) ENTITY::DETACH_ENTITY(c.ped, FALSE, FALSE);   // (v1.6.1)
				ENTITY::FREEZE_ENTITY_POSITION(c.ped, FALSE); PED::SET_PED_CAN_RAGDOLL(c.ped, TRUE);
			}
			Ped p = c.ped;
			g_introLeft.push_back({ p, ENTITY::GET_ENTITY_MODEL(p), true });
			ENTITY::SET_PED_AS_NO_LONGER_NEEDED(&p);
		}
		c = CastState();
	}
	for (Entity e : g_in.props)
		if (e && ENTITY::DOES_ENTITY_EXIST(e))
		{
			SetScripted(e, false);
			g_introLeft.push_back({ e, ENTITY::GET_ENTITY_MODEL(e), false });
			Entity x = e;
			ENTITY::SET_ENTITY_AS_NO_LONGER_NEEDED(&x);
		}
	g_in.props.clear();
	if (g_introLeft.size() > 120) g_introLeft.erase(g_introLeft.begin(), g_introLeft.end() - 120);   // (a few scenes' worth)
	for (int h : g_introFx)
		if (h && GRAPHICS::DOES_PARTICLE_FX_LOOPED_EXIST(h)) { GRAPHICS::STOP_PARTICLE_FX_LOOPED(h, FALSE); GRAPHICS::REMOVE_PARTICLE_FX(h, FALSE); AddLoopsInUse(-1); }
	g_introFx.clear();
}

static void IntroRestoreSettings()
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

// v1.1 audit 2: the scene's anim dictionaries go back to the game when it's over (or aborted)
static void IntroReleaseDicts()
{
	for (const char* d : kIntroDicts) STREAMING::REMOVE_ANIM_DICT(d);
}

static void IntroReleaseHorses()
{
	for (Ped& h : g_in.horses)
	{
		if (h) SetScripted(h, false);
		if (h && ENTITY::DOES_ENTITY_EXIST(h))
		{
			ENTITY::SET_ENTITY_INVINCIBLE(h, FALSE);
			PED::SET_BLOCKING_OF_NON_TEMPORARY_EVENTS(h, FALSE);
		}
		h = 0;
	}
}

// Abort at any point (death, despawn, script restart): put the player and the camera back, leave the world as it is.
static void IntroAbort(const char* why, bool releaseCast = true)
{
	if (!g_in.stage) return;
	if (Tornado* tp = ITp()) tp->scripted = false;   // (audit 3: stage 5 keeps it scripted - an abort hands it to the Movement setting)
	IntroReleaseHorses();
	if (g_in.stage >= 2) IntroReleaseDicts();
	if (g_in.oldCam && CAMERA::DOES_CAM_EXIST(g_in.oldCam)) CAMERA::DESTROY_CAM(g_in.oldCam, FALSE);
	g_in.oldCam = 0;
	if (g_in.gradeK > 0) IntroClearGrade();
	if (g_bal.on && (g_bal.waiting || g_bal.intro)) BalloonRemove("intro ended early");   // v1.1 audit: no orphan balloon
	Ped me = PLAYER::PLAYER_PED_ID();
	if (g_in.stage <= 4)
	{
		IntroCleanupCams();
		CAMERA::RENDER_SCRIPT_CAMS(FALSE, FALSE, 0, TRUE, FALSE, 0);
		if (CAMERA::IS_SCREEN_FADED_OUT()) CAMERA::DO_SCREEN_FADE_IN(500);
		PLAYER::SET_PLAYER_CONTROL(PLAYER::PLAYER_ID(), TRUE, 0, FALSE);
		PED::SET_PED_CAN_RAGDOLL(me, TRUE);
		if (!(g_bal.on && BalloonAboard()) && !PlayerDead())
			TASK::CLEAR_PED_TASKS(me, TRUE, FALSE);
		IntroRestoreLook();
		IntroRestoreSettings();
		IntroRestoreWeather();
	}
	for (int i = 0; i < kCastCount; i++)
		if (g_in.cast[i].ok && !g_in.cast[i].released && kCast[i].ride == RIDE_POSED && g_in.cast[i].lifted)
			ReleaseRider(i, NowSec());
	if (releaseCast) IntroReleaseCast();
	Log("INTRO ended (%s) at %.1f s", why, (float)g_in.clock);
	g_in.stage = 0;
	UI::Letterbox(0);
}

// Despawn everything: the camp and anyone left from it go too.
static void IntroRemoveAll()
{
	IntroAbort("cleared", false);   // (the cast is deleted below, not handed to the world)
	for (int i = 0; i < kCastCount; i++)
	{
		CastState& c = g_in.cast[i];
		if (c.ped && ENTITY::DOES_ENTITY_EXIST(c.ped))
		{
			SetScripted(c.ped, false);
			Ped p = c.ped;
			ENTITY::SET_ENTITY_AS_MISSION_ENTITY(p, TRUE, TRUE);
			PED::DELETE_PED(&p);
		}
		c = CastState();
	}
	for (Entity e : g_in.props)
		if (e && ENTITY::DOES_ENTITY_EXIST(e)) { SetScripted(e, false); Object o = e; DeleteObj(o); }
	g_in.props.clear();
	for (int h : g_introFx)
		if (h && GRAPHICS::DOES_PARTICLE_FX_LOOPED_EXIST(h)) { GRAPHICS::STOP_PARTICLE_FX_LOOPED(h, FALSE); GRAPHICS::REMOVE_PARTICLE_FX(h, FALSE); AddLoopsInUse(-1); }
	g_introFx.clear();
	// v1.1 audit 2: and what an earlier scene left to the world - never Arthur or his horse, never something that isn't ours now
	if (!g_introLeft.empty())
	{
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
	if (MISC::GET_MISSION_FLAG()) { Notify("Not during a story mission - finish or leave it first"); return; }   // v1.1 audit 2
	// v1.6.2 (playtest 16 ended in a crash: the intro staged in Saint Denis, the balloon then high over the city - where every
	// crash so far has been): not in the city. Ride out of town first.
	if (InSaintDenis(ENTITY::GET_ENTITY_COORDS(PLAYER::PLAYER_PED_ID(), TRUE, FALSE), 150.0f))
	{
		Notify("The intro needs open country - ride out of Saint Denis first", 6000);
		Log("INTRO not started: in Saint Denis");
		return;
	}
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
	g_in.savedHour = CLOCK::GET_CLOCK_HOURS(); g_in.savedMin = CLOCK::GET_CLOCK_MINUTES();
	g_in.grade = IniStr("Intro", "Grade", "");   // v1.3: none by default (playtest 11: "weird shading lighting issues")
	g_in.savedPeople = g_set.grabPeople; g_in.savedAnimals = g_set.grabAnimals;
	g_in.savedLock = g_lockedHash;
	g_in.savedManual = g_manualWeather;
	g_in.gradeStrength = IniFloat("Intro", "GradeStrength", 0.75f);
	g_in.stage = 1;
	g_in.stageAt = NowSec();
	CAMERA::DO_SCREEN_FADE_OUT(700);
	Log("INTRO start: fading out (time %02d:%02d)", g_in.savedHour, g_in.savedMin);
	{
		int story = 0;
		for (auto& l : kIntroLines) story += l.root != nullptr;
		Log("INTRO lines: %d, all in the characters' own voices (%d of them story lines, by name)", kIntroLineCount, story);
	}
}

// Built while the screen is black: the camp, the gang, the weather, Arthur on his mark, the balloon.
static void IntroBuild()
{
	Ped me = PLAYER::PLAYER_PED_ID();
	Ped horses[2] = { PLAYER::GET_SADDLE_HORSE_FOR_PLAYER(PLAYER::PLAYER_ID()), PED::IS_PED_ON_MOUNT(me) ? PED::GET_MOUNT(me) : PED::GET_LAST_MOUNT(me) };
	// off the horse / out of a wagon, so Arthur can stand on his mark
	if (PED::IS_PED_ON_MOUNT(me) || PED::IS_PED_IN_ANY_VEHICLE(me, FALSE))
		TASK::CLEAR_PED_TASKS_IMMEDIATELY(me, FALSE, TRUE);
	g_in.fwdH = CameraHeading();
	g_in.F = HeadingDir(g_in.fwdH);
	g_in.R = HeadingDir(g_in.fwdH - 90.0f);
	V3 pp = PlayerPos();
	// the camp centre: 6 m ahead and to the left of Arthur, so he stands at his mark (local 5.6, -0.8)
	g_in.C = pp - g_in.R * 5.6f + g_in.F * 0.8f;
	g_in.C.z = GroundZ(g_in.C.x, g_in.C.y, pp.z + 25.0f, pp.z);
	float spread = 0;
	for (int k = 0; k < 8; k++)
	{
		V3 q = IL(cosf(k * 0.785f) * 12.0f, sinf(k * 0.785f) * 12.0f);
		spread = std::max(spread, fabsf(GroundZ(q.x, q.y, g_in.C.z + 25.0f, g_in.C.z) - g_in.C.z));
	}
	Log("INTRO build at (%.1f, %.1f, %.1f) facing %.0f deg; ground varies by up to %.1f m within 12 m%s", g_in.C.x, g_in.C.y, g_in.C.z, g_in.fwdH,
		spread, spread > 3.0f ? " (hilly - flatter ground looks better)" : "");
	// the look: a storm sky in the late afternoon (v1.3: no grade unless [Intro] Grade asks for one)
	LockWeather("THUNDER", 0.0f);
	MISC::SET_RAIN(0.0f);
	g_storm.rainHeld = true;   // (handed back with -1 after the scene)
	CLOCK::SET_CLOCK_TIME((int)IniFloat("Intro", "Hour", 17), 0, 0);
	if (!g_in.grade.empty())
	{
		GRAPHICS::SET_TIMECYCLE_MODIFIER(g_in.grade.c_str());
		GRAPHICS::SET_TIMECYCLE_MODIFIER_STRENGTH(g_in.gradeStrength);
	}
	HUD::DISPLAY_HUD(FALSE);
	MAP::DISPLAY_RADAR(FALSE);
	// the tornado's settings for the scene
	g_set.arthur = 0;                 // he's in the balloon: it won't touch him
	g_set.grabPeople = g_set.grabAnimals = true;   // the gang has to go up
	g_set.playerGod = true;
	g_set.touchdownCam = false;
	g_set.rideCam = false;
	g_holdStorm = true;
	g_shieldPlayer = true;
	PLAYER::SET_PLAYER_CONTROL(PLAYER::PLAYER_ID(), FALSE, 0, FALSE);
	PED::SET_PED_CAN_RAGDOLL(me, FALSE);
	TASK::CLEAR_PED_TASKS_IMMEDIATELY(me, FALSE, TRUE);
	// ask for everything at once, then wait once (a model at a time could leave the screen black for a minute)
	// (v1.6 review: a dictionary the game doesn't have is never waited for - playtest 14's held the screen black for 6 s)
	for (const char* d : kIntroDicts)
	{
		if (STREAMING::DOES_ANIM_DICT_EXIST(d)) STREAMING::REQUEST_ANIM_DICT(d);
		else Log("INTRO: the game has no anim dictionary %s - going on without it", d);
	}
	for (auto& c : kCast) STREAMING::REQUEST_MODEL(H(c.model), FALSE);
	for (auto& p : kCampProps) { STREAMING::REQUEST_MODEL(H(p.model), FALSE); if (p.alt) STREAMING::REQUEST_MODEL(H(p.alt), FALSE); }
	STREAMING::REQUEST_MODEL(H(kBalloonVehicle), FALSE);
	for (const char* m : kIntroExtraModels) STREAMING::REQUEST_MODEL(H(m), FALSE);
	STREAMING::REQUEST_NAMED_PTFX_ASSET(H(kJetDict));   // v1.1 audit 2: the jets light at 36 s - loading them then froze the scene
	// v1.3: the story lines' subtitles; a fresh conversation history (so a line isn't skipped as "said already"); and the gang
	// keeps talking while it's thrown about (conversations stop when a speaker is hurt, unless this flag is on - Rockstar's camp)
	// (v1.5: 17 text blocks in all - the first few now, the rest 8 s before their line, and each let go when it's done)
	for (int i = 0; i < kIntroLineCount; i++)
		if (kIntroLines[i].block && kIntroLines[i].t < 20.0f) { HUD::TEXT_BLOCK_REQUEST(kIntroLines[i].block); g_in.blockReq[i] = true; }
	AUDIO::CLEAR_CONVERSATION_HISTORY();
	AUDIO::SET_AUDIO_FLAG("DisableAbortConversationForDeathAndInjury", TRUE);
	{
		// v1.4: playtest 12's screen stayed black for the whole 8 s - something never loaded. 6 s at most now, and the log names it.
		DWORD t0 = GetTickCount(), until = t0 + 6000;
		bool all = false;
		while (GetTickCount() < until)
		{
			all = STREAMING::HAS_MODEL_LOADED(H(kBalloonVehicle)) && STREAMING::HAS_NAMED_PTFX_ASSET_LOADED(H(kJetDict));
			for (auto& c : kCast) all = all && STREAMING::HAS_MODEL_LOADED(H(c.model));
			for (const char* d : kIntroDicts)
				if (!IntroDictOptional(d) && STREAMING::DOES_ANIM_DICT_EXIST(d)) all = all && STREAMING::HAS_ANIM_DICT_LOADED(d);
			if (all) break;
			WAIT(0);
		}
		if (!all)
		{
			std::string missing;
			if (!STREAMING::HAS_MODEL_LOADED(H(kBalloonVehicle))) missing += std::string(" ") + kBalloonVehicle;
			if (!STREAMING::HAS_NAMED_PTFX_ASSET_LOADED(H(kJetDict))) missing += std::string(" ") + kJetDict;
			for (auto& c : kCast) if (!STREAMING::HAS_MODEL_LOADED(H(c.model))) missing += std::string(" ") + c.model;
			for (const char* d : kIntroDicts) if (!IntroDictOptional(d) && STREAMING::DOES_ANIM_DICT_EXIST(d) && !STREAMING::HAS_ANIM_DICT_LOADED(d)) missing += std::string(" ") + d;
			Log("INTRO loading: still waiting after %u ms for:%s - going on without them", GetTickCount() - t0, missing.c_str());
		}
		else
			Log("INTRO loading: everything in %u ms", GetTickCount() - t0);
	}
	for (const char* d : kIntroDicts) if (STREAMING::DOES_ANIM_DICT_EXIST(d)) LoadAnimDict(d, IntroDictOptional(d) ? 0 : 200);
	// Arthur's horse waits behind the camp, well away from where the tornado goes, and the tornado leaves it alone
	for (int k = 0; k < 2; k++)
	{
		Ped h = horses[k];
		if (!h || !ENTITY::DOES_ENTITY_EXIST(h) || (k == 1 && h == g_in.horses[0])) continue;
		if ((V3(ENTITY::GET_ENTITY_COORDS(h, FALSE, FALSE)) - g_in.C).len2d() > 120.0f) continue;
		V3 hp = ILg(-6.0f + k * 3.0f, -34.0f, 0.3f);
		ENTITY::SET_ENTITY_COORDS(h, hp.x, hp.y, hp.z, FALSE, FALSE, FALSE, FALSE);
		ENTITY::SET_ENTITY_HEADING(h, g_in.fwdH);
		SetScripted(h, true);
		ENTITY::SET_ENTITY_INVINCIBLE(h, TRUE);                 // v1.1 audit 2: nothing in the scene hurts or spooks it
		PED::SET_BLOCKING_OF_NON_TEMPORARY_EVENTS(h, TRUE);     // (it stays put through the screams; both undone after)
		g_in.horses[k] = h;
		Log("INTRO Arthur's horse (%d) waits 34 m behind the camp", h);
	}
	BuildCamp();
	int castOk = 0;
	for (int i = 0; i < kCastCount; i++)
	{
		g_in.cast[i].ped = SpawnCast(i);
		g_in.cast[i].ok = g_in.cast[i].ped != 0;
		if (g_in.cast[i].ok) { castOk++; StartActivity(i); }
	}
	// v1.3: the balloon sits at the edge of camp with Arthur already in it (and the dog), facing the fire. He had a feeling.
	V3 bp = ILg(16.0f, -10.0f, 0.0f);
	bool balloon = BalloonSpawnParked(bp, HeadingTo(bp, IL(0, 0)));
	if (balloon)
	{
		BalloonSeat();
		TASK::TASK_PLAY_ANIM(me, "script_story@gng2@ig@ig_2_balloon_control", "idle_burner_line_arthur", 8.0f, -4.0f, -1, 1 | 16, 0.0f, FALSE, 0, FALSE, nullptr, FALSE);
		if (Ped dog = CastPed(CA_CAIN))
		{
			TASK::CLEAR_PED_TASKS_IMMEDIATELY(dog, FALSE, TRUE);
			ENTITY::ATTACH_ENTITY_TO_ENTITY(dog, g_bal.body, 0, 0.35f, -0.45f, g_bal.basketZ + 0.55f, 0, 0, 160.0f, FALSE, FALSE, FALSE, FALSE, 2, TRUE, FALSE, FALSE);
			TASK::TASK_START_SCENARIO_IN_PLACE_HASH(dog, Joaat("WORLD_ANIMAL_DOG_SITTING"), -1, FALSE, 0, -1.0f, FALSE);
		}
	}
	Log("INTRO built: %d of %d cast, balloon %s (Arthur %s)", castOk, kCastCount, balloon ? "parked" : "MISSING", balloon && BalloonAboard() ? "aboard" : "not aboard");
	g_in.stage = 3;
	g_in.stageAt = NowSec();
}

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

static float Ease(float u) { u = Clamp(u, 0, 1); return u * u * (3 - 2 * u); }
// v1.4: the head bone (SKEL_Head) - playtest 12 aimed at the body + 0.6 m, which is the basket's wicker when Arthur's in it
static V3 PedHead(Ped p, float up = 1.6f)
{
	if (!p) return g_in.C;
	V3 h = PED::GET_PED_BONE_COORDS(p, 21030, 0, 0, 0);
	V3 body = ENTITY::GET_ENTITY_COORDS(p, FALSE, FALSE);
	if (h.len() > 1.0f && (h - body).len() < 2.5f) return h + V3(0, 0, up - 1.6f);
	return body + V3(0, 0, up - 1.0f);
}

// ---------- v1.4: the camera director ----------
// Playtest 12's frames: Arthur's close-ups showed only the basket's wicker, a tree trunk filled the wide shot of the camp going
// up and another tree hid the funnel. Now every shot says what it has to show (its subject). When a shot starts, the director
// tries the wanted camera spot, then the same spot lifted, swung round its pivot and pulled back - the smallest change first -
// and keeps the first one with a clear line to the subject (a probe against the ground, trees, the balloon, wagons and props;
// people don't count) that is also out of the funnel's smoke. The choice holds for the shot, so the camera never hops; it's
// checked again twice a second, and if something has moved in the way the camera glides to a new spot.
static V3 FlatDir(const V3& from, const V3& to) { V3 d = to - from; d.z = 0; float L = std::max(0.1f, d.len2d()); return V3(d.x / L, d.y / L, 0); }
static const int kIntroLosFlags = 1 | 2 | 16 | 256;   // the map, vehicles, objects, foliage
static V3 LerpV(const V3& a, const V3& b, float u) { return a + (b - a) * u; }

struct IntroShot
{
	V3 pos, look;      // where the camera wants to be, and what it looks at
	float fov = 45;
	V3 subject;        // what must be in plain sight
	Entity ignore = 0; // the subject's own body
	V3 pivot;          // what the camera swings round to find a clear view
	bool wide = false; // a wide shot: bigger lifts (over trees and hills)
	bool loose = false;// an insert (the sky, the title): no check
};

static bool IntroClear(const V3& subject, const V3& cam, Entity ignore)
{
	int h = SHAPETEST::START_EXPENSIVE_SYNCHRONOUS_SHAPE_TEST_LOS_PROBE(subject.x, subject.y, subject.z, cam.x, cam.y, cam.z, kIntroLosFlags, ignore, 7);
	BOOL hit = FALSE;
	Vector3 end = {}, n = {};
	Entity e = 0;
	SHAPETEST::GET_SHAPE_TEST_RESULT(h, &hit, &end, &n, &e);
	if (!hit) return true;
	return (V3(end) - subject).len() > (cam - subject).len() - 0.3f;   // (it only grazed the lens)
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
				// up and back, over whatever it is - the highest such spot that's out of the smoke
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
	// a cut is a cut; a correction mid-shot glides (about a third of a second). Only the correction glides: the shot's own
	// movement (a rising balloon) is followed exactly (v1.4 review: gliding the position left the camera trailing into the wicker)
	if (fresh)
	{
		g_in.dirYawS = g_in.dirYaw; g_in.dirLiftS = g_in.dirLift; g_in.dirPullS = g_in.dirPull;
	}
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

// ---------- the shots ----------
// v1.5: where each shot starts (the camera code and the hold read the same table). Every line's speaker has the shot when they
// speak (a reply is its own shot): the game hears from the camera.
static const float kIntroShotAt[] = {
	0.0f,  4.8f,  9.3f,  13.2f, 17.8f, 21.6f, 25.4f, 31.2f, 32.8f, 36.6f,   //  0 crane  1 Hosea  2 Arthur  3 John  4 Arthur  5 Bill  6 Arthur  7 sky  8 Dutch  9 Arthur
	39.8f, 43.5f, 47.0f, 51.9f, 56.0f, 61.0f, 67.0f, 71.2f, 75.4f, 78.0f,   // 10 Bill  11 storm  12 Micah  13 lift-off  14 Hosea  15 Arthur  16 over the rim  17 Uncle  18 Arthur  19 wide
	81.0f, 87.4f, 90.6f, 94.5f, 98.6f };                                     // 20 hero  21 slow motion  22 "Sorry...", the hat  23 Lenny  24 title
static const int kIntroShotCount = sizeof(kIntroShotAt) / sizeof(kIntroShotAt[0]);
static int IntroShotIndex(float t)
{
	int i = 0;
	while (i + 1 < kIntroShotCount && t >= kIntroShotAt[i + 1]) i++;
	return i;
}
static float IntroNextCut(float t)
{
	int i = IntroShotIndex(t);
	return i + 1 < kIntroShotCount ? kIntroShotAt[i + 1] : 1e9f;
}
static float IntroShotU(float t, int i)   // 0..1 through shot i
{
	float a = kIntroShotAt[i], b = i + 1 < kIntroShotCount ? kIntroShotAt[i + 1] : a + 4.0f;
	return Ease(Clamp((t - a) / std::max(0.1f, b - a), 0.0f, 1.0f));
}

static void IntroShots(float t)
{
	Ped me = PLAYER::PLAYER_PED_ID();
	V3 head = PedHead(me, 1.6f);
	V3 bal = g_bal.on ? g_bal.pos : IL(16.0f, -10.0f, 1.0f);
	Tornado* tp = ITp();
	V3 tb = tp ? tp->base : IL(0, kIntroTornadoDist);
	float th = tp ? tp->height() : 100.0f;
	V3 toCamp = FlatDir(head, IL(0, 0));
	V3 sideC(-toCamp.y, toCamp.x, 0);
	V3 toT = FlatDir(head, tb);
	V3 sideT(-toT.y, toT.x, 0);
	Ped dutch = CastPed(CA_DUTCH);
	V3 dh = PedHead(dutch);
	V3 toB = FlatDir(dh, bal);
	V3 sd(-toB.y, toB.x, 0);
	int si = IntroShotIndex(t);
	float u = IntroShotU(t, si);
	int shot = si;
	IntroShot s;
	auto arthur = [&](const V3& from, float fov)   // Arthur in the basket, from above the rim, at his face
	{
		s.pos = from; s.look = head + V3(0, 0, 0.02f); s.fov = fov; s.subject = head; s.ignore = me; s.pivot = head;
	};
	// v1.6 (playtest 14: after lift-off most of his close-ups were of the back of his head - he faces the way the balloon goes,
	// not the camp): in front of his face, wherever it's pointing
	V3 af = HeadingDir(ENTITY::GET_ENTITY_HEADING(me)), ar(af.y, -af.x, 0);
	auto arthurF = [&](float d, float side, float up, float fov) { arthur(head + af * d + ar * side + V3(0, 0, up), fov); };
	auto onPed = [&](int ci, const V3& from, const V3& look, float fov)   // one of the gang, at their face
	{
		Ped p = CastPed(ci);
		V3 h = PedHead(p);
		s.pos = from; s.look = look; s.fov = fov; s.subject = h; s.ignore = p; s.pivot = h;
	};
	switch (si)
	{
	case 0:
	{
		// establishing: a crane down over the camp to Dutch holding court ("We have a plan. My friends.")
		// (v1.6, playtest 14: it began inside a tree's crown - leaves don't stop a line of sight. It starts high over the camp's
		// own clearing now, and ends a little closer on Dutch)
		V3 end = dh + FlatDir(dh, IL(-5.0f, -6.0f)) * 4.6f + V3(0, 0, 1.2f);
		s.pos = LerpV(IL(-5.0f, -9.0f, 21.0f), end, u);
		s.look = LerpV(IL(1.0f, 1.0f, 2.0f), dh, u);
		s.fov = 46.0f; s.subject = dh; s.ignore = dutch; s.pivot = IL(0, 0); s.wide = true;
		float a = Clamp(std::min(t - 0.6f, 3.8f - t) * 1.6f, 0.0f, 1.0f);
		UI::PlaceCard("VAN DER LINDE CAMP", "1899, an hour before the storm", a);
		break;
	}
	case 1:
	{
		// the reveal, over Hosea's shoulder: Arthur, standing in a balloon, burner lit. "Hey, where are you wandering off to?"
		V3 hh = PedHead(CastPed(CA_HOSEA));
		V3 toBal = FlatDir(hh, bal), sdh(-toBal.y, toBal.x, 0);
		s.pos = hh - toBal * Lerp(1.6f, 1.15f, u) + sdh * 0.5f + V3(0, 0, 0.2f);
		s.look = head + V3(0, 0, 0.2f); s.fov = 34.0f; s.subject = head; s.ignore = me; s.pivot = hh;
		break;
	}
	case 2: arthurF(Lerp(2.1f, 1.8f, u), 0.55f, 0.4f, 32.0f); break;   // "I'm leaving. Don't work too hard."
	case 3:
	{
		// John by the basket, looking up: "I thought you said you'd done this before?" (v1.6, playtest 14: it was the back of his
		// head - now over Arthur's shoulder, at John's face)
		Ped john = CastPed(CA_JOHN);
		V3 jh = PedHead(john);
		V3 toJ = FlatDir(head, jh), sj(-toJ.y, toJ.x, 0);
		onPed(CA_JOHN, head - toJ * 0.9f + sj * 0.75f + V3(0, 0, 0.12f), jh + V3(0, 0, -0.05f), 34.0f);
		break;
	}
	case 4: arthurF(1.8f, -0.6f, 0.35f, 30.0f); break;   // "Look, I'm trying to concentrate here."
	case 5:
	{
		// Bill at the fire: "Ain't loyalty mean nothing to you?"
		V3 bh = PedHead(CastPed(CA_BILL));
		V3 toF = FlatDir(bh, IL(0, 0)), sb(-toF.y, toF.x, 0);
		onPed(CA_BILL, bh + toF * 2.0f + sb * 0.6f + V3(0, 0, 0.2f), bh + V3(0, 0, -0.1f), 34.0f);
		break;
	}
	case 6: arthurF(1.9f, 0.3f, 0.3f, 30.0f); break;   // why he's leaving - then he looks at the sky
	case 7:
		// what he sees: the storm on the horizon (lightning at 31.7 s)
		s.pos = head + V3(0, 0, 0.25f) + toT * 0.4f; s.look = IL(14.0f, 420.0f, 150.0f); s.fov = 50.0f; s.loose = true;
		break;
	case 8:
	{
		// Dutch, from the balloon's side: "Faith, Arthur. Have a little faith."
		s.pos = dh + toB * 2.5f - sd * 1.0f + V3(0, 0, 0.3f); s.look = dh + V3(0, 0, -0.1f); s.fov = 36.0f; s.subject = dh; s.ignore = dutch; s.pivot = dh;
		break;
	}
	case 9: arthurF(1.8f, -0.55f, 0.35f, 30.0f); break;   // a rub of the eyes, "If you say so."
	case 10:
	{
		// Bill, with the sky the storm's coming from behind him: "There's nothing to worry about." - thunder
		V3 bh = PedHead(CastPed(CA_BILL));
		onPed(CA_BILL, bh - g_in.F * 2.2f + g_in.R * 0.55f + V3(0, 0, 0.3f), bh + g_in.F * 1.0f + V3(0, 0, 0.4f), 38.0f);
		break;
	}
	case 11:
	{
		// low and wide behind the gang: the funnel drops out of the sky
		V3 look = tp ? tb + V3(0, 0, th * 0.35f) : IL(0, 70.0f, 26.0f);
		s.pos = LerpV(IL(-3.5f, -10.0f, 1.6f), IL(-3.5f, -10.5f, 2.4f), u);
		s.look = look; s.fov = t < kIntroSpawn ? 52.0f : 44.0f; s.subject = look; s.pivot = IL(0, 0); s.wide = true;
		break;
	}
	case 12:
	{
		// Micah, on his feet now and making for the balloon: "Reckon it's time we got out of here, Morgan." (v1.6: the camera
		// goes ahead of him as he runs)
		V3 mh = PedHead(CastPed(CA_MICAH));
		V3 toBal = FlatDir(mh, bal), sm(-toBal.y, toBal.x, 0);
		// (v1.6 review: as he gets to the basket the camera swings to his side - 2.2 m ahead of him would be in the basket)
		float farK = Clamp(((mh - bal).len2d() - 2.4f) / 2.2f, 0.0f, 1.0f);
		onPed(CA_MICAH, mh + toBal * Lerp(0.4f, 2.2f, farK) + sm * Lerp(1.7f, 0.55f, farK) + V3(0, 0, 0.1f), mh, 36.0f);
		break;
	}
	case 13:
	{
		// v1.6 (the user: "arthur should fly into micah, ragdolling him with the hot air balloon as he takes off"): from the
		// ground beside the basket - Micah at it, Arthur pulls the burner, the basket takes Micah out, and the camera tilts up
		// after the balloon. "Now you want get out of here?"
		// v1.6.1 (the user: "fly the balloon into micah, knocking him over before taking off, all in one swoop - see ya"): from the
		// side of the swoop's path, far enough back to hold both ends - the balloon skims across the camp into Micah, he goes flying,
		// and the camera follows the balloon up and away
		Ped mi = CastPed(CA_MICAH);
		if (g_in.camShot != 13)
		{
			g_in.liftAnchor = bal;
			g_in.micahAnchor = mi ? V3(ENTITY::GET_ENTITY_COORDS(mi, FALSE, FALSE)) : IL(8.0f, -7.0f);
		}
		V3 an = g_in.liftAnchor, ma = g_in.micahAnchor;
		V3 mid = (an + ma) * 0.5f;
		V3 path = FlatDir(an, ma), sp(-path.y, path.x, 0);
		if ((mid + sp * 5.0f - IL(0, 0)).len2d() < (mid - sp * 5.0f - IL(0, 0)).len2d()) sp = sp * -1.0f;   // (the side away from the camp)
		float d = std::max(8.0f, (ma - an).len2d() * 0.55f + 3.0f);
		s.pos = mid + sp * d + V3(0, 0, 1.4f);
		V3 mp = mi ? V3(ENTITY::GET_ENTITY_COORDS(mi, FALSE, FALSE)) : ma;
		if (g_in.camShot != 13) g_in.swoopFollow = 0.5f;
		g_in.swoopFollow += ((g_in.swoopHit ? 0.8f : 0.5f) - g_in.swoopFollow) * Clamp((t - g_in.dirLastT) * 2.5f, 0.0f, 1.0f);   // (eased)
		s.look = LerpV(mp + V3(0, 0, 0.9f), head, g_in.swoopFollow); s.fov = 56.0f;
		s.subject = head; s.ignore = me; s.pivot = mid;
		break;
	}
	case 14:
	{
		// Hosea, low and from the side, the balloon going up past him: Dutch "WHAT?!", Hosea "Was that really necessary, Arthur?"
		// (v1.6, playtest 14: it was the back of his head and a sky)
		V3 hh = PedHead(CastPed(CA_HOSEA));
		V3 toBal = FlatDir(hh, bal), sdh(-toBal.y, toBal.x, 0);
		// (v1.6.1, playtest 15: from the side, looking up at the balloon, he was out of the frame - at his face now)
		onPed(CA_HOSEA, hh + toBal * 1.9f + sdh * 0.7f + V3(0, 0, -0.3f), hh + V3(0, 0, 0.05f), 40.0f);
		break;
	}
	case 15: arthurF(1.9f, -0.5f, 0.25f, 32.0f); break;   // "If I had a dollar for every time I've been asked that."
	case 16:
	{
		// v1.6 (playtest 14: this looked into the basket's floor): from just outside the rising basket, down at Micah where the
		// balloon knocked him flat - "Every man equal! Every man for himself!" - and the tornado takes him first, straight up
		// past the lens (and a cow drifts by)
		Ped micah = CastPed(CA_MICAH);
		V3 mp = micah ? V3(ENTITY::GET_ENTITY_COORDS(micah, FALSE, FALSE)) : IL(14.0f, -9.0f);
		V3 toM = FlatDir(head, mp);
		V3 look = mp + V3(0, 0, 1.0f);
		if (look.z > head.z + 6.0f) look.z = head.z + 6.0f;   // (he goes past fast: the camera only tilts so far)
		s.pos = head + toM * 2.4f + V3(0, 0, 0.5f); s.look = look; s.fov = 36.0f;   // (v1.6.1: tighter - he was a speck)
		s.subject = mp + V3(0, 0, 1.0f); s.ignore = micah; s.pivot = head;
		break;
	}
	case 17:
	{
		// Uncle in his chair, the funnel behind him: "Oh, I got lumbago, it's very serious!" (v1.6: in front of his face - the
		// chair faces away from the funnel now)
		Ped un = CastPed(CA_UNCLE);
		V3 uh = PedHead(un);
		V3 uf = un ? HeadingDir(ENTITY::GET_ENTITY_HEADING(un)) : FlatDir(tb, uh), su(-uf.y, uf.x, 0);
		onPed(CA_UNCLE, uh + uf * 1.9f + su * 0.5f + V3(0, 0, 0.3f), uh + V3(0, 0, -0.1f), 38.0f);
		break;
	}
	case 18: arthurF(1.8f, 0.5f, 0.45f, 32.0f); break;   // "Lumbago. Really..."
	case 19:
	{
		// THE GANG GOES UP: from the camp's edge, low, just clear of the funnel - Pearson with his pot ("I'm designed to float."),
		// Uncle with his chair, the rest as ragdolls, rising into it. (v1.6, playtest 14: from 58 m+ and 9 m up the trees, roofs
		// and distance hid it in all five intros - this is the shot)
		float r0 = tp ? tp->radiusAt(0.12f) : 14.0f;
		float d = std::max(26.0f, r0 * 1.3f + 11.0f);
		V3 c0 = IntroSettleSpot();
		V3 dir = g_in.R * -0.8f - g_in.F * 0.6f;
		s.pos = c0 + dir * d + V3(0, 0, 2.2f); s.look = c0 + V3(0, 0, Lerp(5.0f, 11.0f, u)); s.fov = 54.0f;
		s.subject = c0 + V3(0, 0, 4.0f); s.pivot = c0; s.wide = true;
		break;
	}
	case 20:
		// THE HERO SHOT: Arthur at the burner, calm, close and a little below; past him the funnel takes the camp. Javier sails by.
		s.pos = head - toT * Lerp(3.2f, 2.5f, u) + sideT * 1.1f + V3(0, 0, -0.25f);
		s.look = head + toT * 6.0f + V3(0, 0, -1.5f); s.fov = 48.0f; s.subject = head; s.ignore = me; s.pivot = head;
		break;
	case 21:
	{
		// slow motion, from beside the basket: Dutch is thrown out of the smoke and across the frame, between the lens and
		// Arthur. "What is wrong with you, Arthur?" (v1.6, playtest 14: thrown from 60-99 m at 45-76 m/s, he was never in this
		// shot - see Beat 11; the camera stays on Arthur)
		s.pos = head + sideT * 6.5f - toT * 1.5f + V3(0, 0, 0.8f); s.look = head + sideT * 1.6f + V3(0, 0, -0.1f); s.fov = 50.0f;
		s.subject = head; s.ignore = me; s.pivot = head;
		break;
	}
	case 22: arthurF(1.7f, 0.6f, 0.3f, 34.0f); break;   // "Sorry, this ain't a good time." The hat. Honor goes down.
	case 23:
	{
		// Lenny, going round in the funnel, close: "I guess you know best, Arthur." (v1.5 review: a checked shot, so the camera is
		// never left in the smoke; without Lenny, the title's shot)
		Ped lenny = CastPed(CA_LENNY);
		if (lenny)
		{
			V3 lp = PedHead(lenny);
			V3 out = FlatDir(tb, lp);
			s.pos = lp + out * 4.2f + V3(0, 0, 0.7f); s.look = lp; s.fov = 42.0f; s.subject = lp; s.ignore = lenny; s.pivot = lp;
			break;
		}
	}
	[[fallthrough]];
	default:
	{
		// behind the balloon, where the gameplay camera will be: the title. (v1.6.1: looking the way he's escaping - away from the
		// funnel - which is where the game's camera comes back; the balloon's heading isn't, his seat faces back at the funnel)
		V3 fwd = FlatDir(tb, bal);
		s.pos = bal - fwd * 15.0f + V3(0, 0, 5.0f); s.look = bal + fwd * 8.0f + V3(0, 0, 2.0f); s.fov = 50.0f;
		s.subject = head; s.ignore = me; s.pivot = bal;   // (v1.6.1: checked - on the funnel's side now, it mustn't sit in the smoke)
		break;
	}
	}
	IntroDirect(shot, s, t);
	if (shot != g_in.camShot)
	{
		g_in.camShot = shot;
		g_in.cutAt = t;   // (v1.5: a line waits a second after a cut, for the game's ears to catch up)
		if (g_in.cam && (shot == 11 || shot == 16 || shot == 19))
			CAMERA::SHAKE_CAM(g_in.cam, "HAND_SHAKE", shot == 19 ? 0.6f : 0.3f);
		else if (g_in.cam)
			CAMERA::STOP_CAM_SHAKING(g_in.cam, TRUE);
	}
}

static bool Beat(int id, bool when)
{
	if (!when) return false;
	for (int b : g_in.firedBeats) if (b == id) return false;
	g_in.firedBeats.push_back(id);
	return true;
}

// v1.3: the characters' own lines - the first of the list this voice has, with the game's own subtitle asked for
// v1.5: the game hears from the camera, and a line from somebody 30 m off is lost (playtest 13). A far speaker's line is played
// from a point just beside the camera, toward them, in their own voice - so it's heard, and from the right side.
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
		g_in.nearUntilMs = g_introTick() + 2200;   // (no ped to ask: about how long a line lasts)
		if (!g_in.nearUntilMs) g_in.nearUntilMs = 1;
		if (p) PED::REQUEST_PED_FACIAL_MOOD_THIS_FRAME(p, 1751822680u /*MoodTalking*/, 6);
	}
	return ok;
}
static const char* IntroVoice(int who) { return who < 0 ? "ARTHUR" : kCast[who].speaker; }

static bool IntroSay(Ped p, const char* const* ctx, bool shout, const char* who, const char* voice)
{
	if (!p || !ENTITY::DOES_ENTITY_EXIST(p) || !ctx[0]) return false;
	for (int i = 0; i < 4 && ctx[i]; i++)
	{
		if (!AUDIO::DOES_CONTEXT_EXIST_FOR_THIS_PED(p, ctx[i], FALSE)) continue;
		g_in.lineAt = (float)g_in.clock;
		if (IntroFar(p) && IntroSpeakNear(p, voice, ctx[i], shout))
		{
			Log("INTRO %.1f s: %s says %s -> played beside the camera (%.0f m off)", (float)g_in.clock, who, ctx[i],
				(V3(ENTITY::GET_ENTITY_COORDS(p, FALSE, FALSE)) - V3(CAMERA::GET_FINAL_RENDERED_CAM_COORD())).len());
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

static const char* IntroWho(int who) { return who < 0 ? "arthur" : kCast[who].id; }

// the text blocks of the story lines' subtitles
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
}

// v1.3 (playtest 11: "We want to splice the character's actual lines"): one line of one of Rockstar's own conversations, as their
// scripts do it - create, every voice in the scene under its conversation name, start, then just the one line
static bool IntroStory(int i)
{
	const IntroLine& l = kIntroLines[i];
	if (!AUDIO::IS_SCRIPTED_CONVERSATION_CREATED(l.root) && !AUDIO::CREATE_NEW_SCRIPTED_CONVERSATION(l.root))
	{
		Log("INTRO %.1f s: STORY %s couldn't be created", (float)g_in.clock, l.root);
		return false;
	}
	AUDIO::ADD_PED_TO_CONVERSATION(l.root, PLAYER::PLAYER_PED_ID(), "ARTHUR");
	for (int c = 0; c < kCastCount; c++)
	{
		Ped p = CastPed(c);
		if (!p || !kCast[c].speaker[0] || !ENTITY::DOES_ENTITY_EXIST(p) || ENTITY::IS_ENTITY_DEAD(p)) continue;
		AUDIO::ADD_PED_TO_CONVERSATION(l.root, p, kCast[c].speaker);
		if (c == CA_MICAH) AUDIO::ADD_PED_TO_CONVERSATION(l.root, p, "MICAH");   // (some of his mission scenes call him that)
	}
	AUDIO::START_SCRIPT_CONVERSATION(l.root, TRUE, TRUE, FALSE);
	AUDIO::PLAY_SINGLE_LINE_OF_CONVERSATION(l.root, l.idx);
	Log("INTRO %.1f s: STORY %s: %s[%d] \"%s\"%s", (float)g_in.clock, IntroWho(l.who), l.root, l.idx, l.words,
		HUD::TEXT_BLOCK_IS_LOADED(l.block) ? "" : " (its subtitles aren't loaded)");
	return true;
}

static void IntroStoryStop()
{
	if (g_in.storyLine >= 0 && AUDIO::IS_SCRIPTED_CONVERSATION_PLAYING(kIntroLines[g_in.storyLine].root))
		AUDIO::STOP_SCRIPTED_CONVERSATION(kIntroLines[g_in.storyLine].root, FALSE, FALSE);
	g_in.storyLine = -1;
}

// the scene's over (or aborted): no line left talking, the flag and the text blocks given back
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
	Log("INTRO %.1f s: STORY %s[%d] didn't play - %s's own line instead", (float)g_in.clock, l.root, l.idx, IntroWho(l.fbWho));
	Ped p = Speaker(l.fbWho);
	IntroSay(p, l.ctx, l.shout, IntroWho(l.fbWho), IntroVoice(l.fbWho));
}

// v1.5: a story line's subtitles are asked for 8 s before it, and let go once no line still to come needs that block
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
	// the story line being said: did the game take it? (nothing playing 1.2 s on = no: the fallback)
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
			g_in.storyLine = -1;   // said
	}
	for (int i = 0; i < kIntroLineCount; i++)
	{
		const IntroLine& l = kIntroLines[i];
		if (t >= l.t && t < l.t + 1.6f)
			if (Ped sp = Speaker(l.who)) PED::REQUEST_PED_FACIAL_MOOD_THIS_FRAME(sp, 1751822680u /*MoodTalking*/, 6);
		if (t < l.t || g_in.linePlayed[i]) continue;
		// v1.5: the ears - never in the first second after a cut (wait up to 1.5 s for it)
		if (t < g_in.cutAt + 1.0f && t < l.t + 1.5f) continue;
		// one at a time: wait (up to 1.5 s) for the story line being said to finish - a new story line, or the same mouth
		bool busy = g_in.storyLine >= 0 && (l.root || kIntroLines[g_in.storyLine].who == l.who);
		if (busy && t < l.t + 1.5f) continue;
		if (busy && l.root) IntroStoryStop();
		g_in.linePlayed[i] = true;
		Ped sp = Speaker(l.who);
		if (l.root)
		{
			g_in.lineAt = t;
			if (l.audio && IntroFar(sp) && IntroSpeakNear(sp, IntroVoice(l.who), l.audio, l.shout))
			{
				Log("INTRO %.1f s: STORY %s: %s \"%s\" -> played beside the camera by its audio name", t, IntroWho(l.who), l.audio, l.words);
				g_in.storyPlayed++;
			}
			else if (IntroStory(i)) { g_in.storyLine = i; g_in.storyAt = t; g_in.storySeen = false; }
			else IntroFallback(i);
		}
		else
			IntroSay(sp, l.ctx, l.shout, IntroWho(l.who), IntroVoice(l.who));
	}
}

// v1.5: is the last line still being said? (the story line's conversation; an ambient line's speaker; a line played beside
// the camera - about 2 s)
static bool IntroLineOn(float t)
{
	if (g_in.storyLine >= 0 && AUDIO::IS_SCRIPTED_CONVERSATION_PLAYING(kIntroLines[g_in.storyLine].root)) return true;
	if (g_in.ambPed && t - g_in.ambAt < 4.0f && ENTITY::DOES_ENTITY_EXIST(g_in.ambPed) && AUDIO::IS_AMBIENT_SPEECH_PLAYING(g_in.ambPed)) return true;
	return g_in.nearUntilMs && (int)(g_in.nearUntilMs - g_introTick()) > 0;
}

static void IntroBeats(float t)
{
	Ped me = PLAYER::PLAYER_PED_ID();
	Ped dutch = CastPed(CA_DUTCH);
	V3 bal = g_bal.on ? g_bal.pos : IL(16.0f, -10.0f, 1.0f);
	// the reveal: everyone turns to look at Arthur in his balloon; Dutch points
	if (Beat(17, t >= 4.9f))
		for (int i : { CA_MICAH, CA_BILL, CA_JAVIER, CA_PEARSON, CA_STRAUSS, CA_HOSEA, CA_JOHN, CA_LENNY })
			if (Ped p = CastPed(i)) TASK::TASK_LOOK_AT_ENTITY(p, me, 30000, 0, 51, 0);
	if (Beat(1, t >= 4.9f) && dutch) TASK::TASK_TURN_PED_TO_FACE_COORD(dutch, bal.x, bal.y, bal.z, 900);
	if (Beat(2, t >= 5.4f) && dutch) PlayAnim(dutch, "ai_react@point@base", "point_fwd", false, 6.0f);
	if (Beat(4, t >= 8.0f) && dutch) PlayAnim(dutch, "script_re@rally@rally", "base_leader", true);
	// Arthur looks at the sky... and what he sees: the storm out on the horizon, flickering
	if (Beat(3, t >= 30.5f)) { V3 sky = IL(14.0f, 420.0f, 150.0f); TASK::TASK_LOOK_AT_COORD(me, sky.x, sky.y, sky.z, 4500, 0, 51, FALSE); }
	if (Beat(18, t >= 31.7f)) { V3 storm = IL(25.0f, 520.0f, 180.0f); MISC::FORCE_LIGHTNING_FLASH_AT_COORDS(storm.x, storm.y, storm.z, -1.0f); }
	// Dutch's speech gesture for "Have a little faith"; Arthur's facepalm
	if (Beat(20, t >= 33.4f) && dutch) PlayAnim(dutch, "script_re@rally@rally", "action_leader", false, 6.0f);
	if (Beat(19, t >= 36.7f)) PlayUpper(me, "ai_gestures@arthur@standing@speaker", "neutral_fidget_rubeye_r_001");   // (v1.6: his own gesture)
	// thunder, right after "There's nothing to worry about.": everyone looks up the valley
	if (Beat(5, t >= kIntroThunder))
	{
		V3 horizon = IL(8.0f, 150.0f, 70.0f);
		MISC::FORCE_LIGHTNING_FLASH_AT_COORDS(horizon.x, horizon.y, horizon.z, -1.0f);
		// (v1.6, playtest 14: "the thunder lightning sfx during it is way too loud" - the frontend strike was 15 dB over the
		// voices. The flash's own thunder, from up the valley, is enough)
		for (int i = 0; i < kCastCount; i++)
			if (Ped p = CastPed(i)) if (PED::IS_PED_HUMAN(p)) TASK::TASK_LOOK_AT_COORD(p, horizon.x, horizon.y, horizon.z - 40.0f, 6000, 0, 51, FALSE);
		TASK::TASK_LOOK_AT_COORD(me, horizon.x, horizon.y, horizon.z - 40.0f, 6000, 0, 51, FALSE);
	}
	if (Beat(6, t >= kIntroSpawn))
	{
		// the funnel drops out of the cloud 110 m out (the touchdown, sped up to 2.5 s)
		V3 p = IL(0.0f, kIntroTornadoDist);
		p.z = GroundZ(p.x, p.y, g_in.C.z + 60.0f, g_in.C.z);
		Tornado* nt = SpawnTornadoAt(p, g_in.fwdH + 180.0f, g_set.style, GetStyles()[g_set.style].name);
		g_in.tp = TornadoRef(nt);
		if (nt) { nt->growSeconds = 2.5f; nt->scripted = true; nt->scriptVel = V3(); nt->stationary = false; }
		Log("INTRO tornado %s at %.0f m", ITp() ? "down" : "FAILED", kIntroTornadoDist);
	}
	if (Beat(7, t >= 46.0f))
	{
		for (int i : { CA_BILL, CA_STRAUSS })
			if (Ped p = CastPed(i)) PlayAnim(p, "amb_temp@code_human_cower@male@base", "base", true, 4.0f);
	}
	// v1.6: Micah heads for the balloon ("Reckon it's time we got out of here, Morgan."). (v1.6.1, playtest 15: getting up out of
	// his seat took him till lift-off in all four intros - he's on his feet at the cut to his close-up, and strides a few metres)
	if (Beat(22, t >= 47.0f))
		if (Ped m = CastPed(CA_MICAH))
		{
			V3 mp = ENTITY::GET_ENTITY_COORDS(m, FALSE, FALSE);
			V3 toB = FlatDir(mp, bal);
			float go = std::max(0.0f, std::min(7.0f, (bal - mp).len2d() - 4.0f));
			V3 at = mp + toB * go;
			at.z = GroundZ(at.x, at.y, at.z + 10.0f, at.z - 10.0f);
			TASK::CLEAR_PED_TASKS_IMMEDIATELY(m, FALSE, TRUE);   // (on his feet - hidden by the cut)
			TASK::TASK_GO_STRAIGHT_TO_COORD(m, at.x, at.y, at.z, 1.6f, -1, HeadingTo(at, bal), 0.3f, 0);   // (-1: never teleported there)
			Log("INTRO micah heads for the balloon (%.0f m)", go);
		}
	// ...and the balloon comes for him (the swoop, with the balloon below)
	// Arthur pulls the burner. He had a feeling.
	if (Beat(8, t >= kIntroLiftOff))
	{
		g_in.swoop = CastPed(CA_MICAH) != 0;
		if (Ped m = CastPed(CA_MICAH))
			Log("INTRO lift-off: the balloon swoops for Micah (%.0f m off)", (V3(ENTITY::GET_ENTITY_COORDS(m, FALSE, FALSE)) - bal).len2d());
		BalloonBoard(true);
		TASK::TASK_PLAY_ANIM(me, "script_story@gng2@ig@ig_2_balloon_control", "pull_burner_arthur", 4.0f, -4.0f, -1, 28, 0.0f, FALSE, 0, FALSE, nullptr, FALSE);
		UI::PlayWhoosh(true);   // (v1.6.1: the soft one - playtest 15 had it 10 dB over the voices)
	}
	// v1.6.1: a knock is held for a few frames, so the ragdoll takes it (one velocity set as it starts gets lost)
	if (g_in.pushPed && t < g_in.pushUntil && ENTITY::DOES_ENTITY_EXIST(g_in.pushPed))
		ENTITY::SET_ENTITY_VELOCITY(g_in.pushPed, g_in.pushVel.x, g_in.pushVel.y, g_in.pushVel.z);
	if (Beat(9, t >= 56.3f) && dutch) { TASK::TASK_TURN_PED_TO_FACE_COORD(dutch, bal.x, bal.y, bal.z, 600); PlayAnim(dutch, "ai_react@point@base", "point_fwd", false, 8.0f); }
	// the tornado comes up to the camp's edge and waits there, looming, for the last words - then sweeps through and stays
	// on the camp (it doesn't chase the balloon)
	if (ITp() && t >= kIntroLunge)
	{
		V3 target = t < kIntroSweep ? IntroSettleSpot() + g_in.F * 35.0f
			: t < 82.0f ? IntroSettleSpot() : IntroSettleSpot() + V3(cosf(t * 0.4f), sinf(t * 0.4f), 0) * 5.0f;
		V3 d = target - ITp()->base; d.z = 0;
		float L = d.len2d();
		float speed = L > 15.0f ? 26.0f : L * 0.5f;
		ITp()->scriptVel = L > 0.5f ? d * (speed / L) : V3();
	}
	// the gang goes up - into the real thing (ragdolls), as the funnel reaches each of them (or a moment late at the most)
	for (int i = 0; i < kCastCount; i++)
	{
		if (kCast[i].ride == RIDE_STAY || t < kCast[i].liftAt || !g_in.cast[i].ok || g_in.cast[i].lifted) continue;
		bool inReach = !ITp() || (V3(ENTITY::GET_ENTITY_COORDS(g_in.cast[i].ped, FALSE, FALSE)) - ITp()->base).len2d() < ITp()->wallRadius() * 2.8f;
		if (inReach || t >= kCast[i].liftAt + 2.0f) LiftRider(i, t);
	}
	// v1.5 the hero shot: a confident flick of the hat
	if (Beat(21, t >= 81.3f)) PlayUpper(me, "ai_gestures@arthur@standing@speaker", "greet_cocky_r_003");   // (v1.6: his own)
	// slow motion, and the lines can still be heard in it (Rockstar's own flag)
	if (Beat(10, t >= kIntroSlowMo0)) { AUDIO::SET_AUDIO_FLAG("AllowScriptedSpeechInSlowMo", TRUE); MISC::SET_TIME_SCALE(0.4f); g_in.timeScale = 0.4f; }
	// Dutch is thrown right past the balloon - close enough to look Arthur in the eye. (v1.6, playtest 14: from where the funnel
	// had him - 60-99 m off - he flew at 45-76 m/s and was never in the shot. At the cut he comes out of the smoke 9 m from the
	// basket on the funnel's side, off the edge of the frame, and crosses it between the lens and Arthur in the middle of the
	// slow motion: 0.62 s of game time, 1.55 s at 0.4)
	if (Beat(11, t >= kIntroDutchThrow) && dutch)
	{
		V3 head = PedHead(me, 1.6f);
		V3 toT = FlatDir(head, ITp() ? ITp()->base : IL(0, kIntroTornadoDist));
		V3 side(-toT.y, toT.x, 0);   // (the slow-motion shot's side - its camera is out this way)
		float T = 0.62f;
		V3 pass = head + g_bal.cmdVel * T + side * 3.2f;   // (v1.6 review: clear of the rigging and the jets)
		V3 from = pass + toT * 9.0f + V3(0, 0, 1.2f);
		V3 v = (pass - from) * (1.0f / T) + V3(0, 0, 0.5f * 9.8f * T);
		SetScripted(dutch, true);                 // the tornado lets go of him for this one
		ENTITY::DETACH_ENTITY(dutch, FALSE, FALSE);
		// (v1.6.2, playtest 16: a ragdoll can't be moved - he stayed in the funnel 60 m off, and in v1.6.1 the steering couldn't
		// bring him in time. Out of the ragdoll, moved, back into it; then steered to the pass point every frame, below - which
		// is what makes the throw take, the thing v1.6.0 lost)
		TASK::CLEAR_PED_TASKS_IMMEDIATELY(dutch, FALSE, TRUE);
		ENTITY::SET_ENTITY_COORDS_NO_OFFSET(dutch, from.x, from.y, from.z, FALSE, FALSE, FALSE);
		PED::SET_PED_TO_RAGDOLL(dutch, 5000, 7000, 0, FALSE, FALSE, nullptr);
		ENTITY::SET_ENTITY_VELOCITY(dutch, v.x, v.y, v.z);
		g_in.dutchFrom = from;
		g_in.dutchSpeed = (pass - from).len() / T;
		g_in.dutchDir = (pass - from) * (1.0f / std::max(0.1f, (pass - from).len()));
		Log("INTRO Dutch thrown past the balloon (from %.0f m, %.0f m/s)", (pass - from).len(), v.len());
	}
	// v1.6.1: steered across the frame to the pass point beside the basket (the basket moves), then sent on through
	if (dutch && t >= kIntroDutchThrow && t < kIntroDutchPass + 0.35f)
	{
		bool fired = false;
		for (int b : g_in.firedBeats) if (b == 11) fired = true;
		if (fired)
		{
			V3 head = PedHead(me, 1.6f);
			V3 toT = FlatDir(head, ITp() ? ITp()->base : IL(0, kIntroTornadoDist));
			V3 side(-toT.y, toT.x, 0);
			V3 pass = head + side * 3.2f;
			V3 dp = ENTITY::GET_ENTITY_COORDS(dutch, FALSE, FALSE);
			// (v1.6.2: still far off a moment after the move - it didn't take: once more)
			if (!g_in.dutchRetried && t > kIntroDutchThrow + 0.2f && (dp - pass).len() > 18.0f)
			{
				g_in.dutchRetried = true;
				TASK::CLEAR_PED_TASKS_IMMEDIATELY(dutch, FALSE, TRUE);
				ENTITY::SET_ENTITY_COORDS_NO_OFFSET(dutch, g_in.dutchFrom.x, g_in.dutchFrom.y, g_in.dutchFrom.z, FALSE, FALSE, FALSE);
				PED::SET_PED_TO_RAGDOLL(dutch, 5000, 7000, 0, FALSE, FALSE, nullptr);
				Log("INTRO Dutch was still %.0f m off the pass - moved again", (dp - pass).len());
				dp = g_in.dutchFrom;
			}
			if (!g_in.dutchLogged && t >= kIntroDutchPass)
			{
				g_in.dutchLogged = true;
				Log("INTRO Dutch at the pass: %.1f m from the point beside the basket, %.1f m from Arthur", (dp - pass).len(), (dp - head).len());
			}
			V3 d = pass - dp;
			float speed = Clamp(g_in.dutchSpeed, 8.0f, 22.0f);
			// (v1.6.1 review: steered in direction only, at his throw speed - aiming at a point that arrives in time braked him to
			// 5 m/s right at the pass. Once he's level with it, he flies on the way he was going)
			bool before = t < kIntroDutchPass && d.x * g_in.dutchDir.x + d.y * g_in.dutchDir.y + d.z * g_in.dutchDir.z > 0.4f;   // (not level with it yet)
			V3 v = before ? d * (speed / std::max(0.1f, d.len())) : g_in.dutchDir * speed;
			ENTITY::SET_ENTITY_VELOCITY(dutch, v.x, v.y, v.z);
		}
	}
	if (Beat(12, t >= kIntroSlowMo1)) { MISC::SET_TIME_SCALE(1.0f); g_in.timeScale = 1.0f; AUDIO::SET_AUDIO_FLAG("AllowScriptedSpeechInSlowMo", FALSE); }
	if (Beat(13, t >= kIntroSlowMo1 + 0.6f) && dutch) SetScripted(dutch, false);   // back to the tornado with him
	if (Beat(14, t >= 93.4f)) PlayUpper(me, "ai_gestures@arthur@standing@speaker", "greet_hat_tip_r_001");   // (v1.6: his own hat tip)
	if (Beat(15, t >= 94.0f))
	{
		GRAPHICS::ANIMPOSTFX_PLAY("PlayerHonorChoiceBad");
		UI::HonorLost(true);   // (the HUD is hidden in the scene: the mod's sting, not the game's feed toast)
	}
	// the balloon: up gently, then drifting away from the funnel and climbing slowly (it has 50 s in the air now), then away
	if (g_bal.on && g_bal.intro && t >= kIntroLiftOff)
	{
		V3 c = ITp() ? ITp()->base : g_in.C;
		V3 away = FlatDir(c, g_bal.pos);
		if (g_in.swoop)
		{
			// v1.6.1 the swoop: low across the camp (the basket's bottom 35 cm off the grass), straight at Micah wherever he is,
			// to get there about 2 s after lift-off - then he goes flying and the balloon climbs away past him
			Ped m = CastPed(CA_MICAH);
			V3 mp = m ? V3(ENTITY::GET_ENTITY_COORDS(m, FALSE, FALSE)) : g_in.micahAnchor;
			V3 toM = FlatDir(g_bal.pos, mp);
			float dist = (mp - g_bal.pos).len2d();
			float speed = Clamp(dist / std::max(0.35f, kIntroSwoopHit - t), 4.0f, 11.0f);
			float h = 1.0f, vz = 0.0f;
			if (TrueHeight(g_bal.pos + V3(0, 0, g_bal.basketZ), &h)) vz = Clamp((0.35f - h) * 3.0f, -2.5f, 2.5f);
			g_bal.cmdVel = toM * speed + V3(0, 0, vz);
			g_bal.skim = true;
			// (v1.6.1 review: the basket goes through the rest of the gang without touching them - Hosea and John stand near its path)
			if (Entity body = g_bal.body)
				for (int i = 0; i < kCastCount; i++)
					if (i != CA_MICAH) if (Ped q = CastPed(i)) ENTITY::SET_ENTITY_NO_COLLISION_ENTITY(body, q, TRUE);
			bool low = h < 1.5f;   // (v1.6.1 review: it's only a hit if the basket is down at his level)
			if ((dist < 1.9f && low) || t > kIntroSwoopHit + 1.0f)
			{
				g_in.swoop = false; g_bal.skim = false;
				g_in.swoopDir = toM; g_in.climbUntil = t + 1.4f;
				if (m && dist < 4.0f && low && Beat(23, true))
				{
					g_in.swoopHit = true;
					TASK::CLEAR_PED_TASKS_IMMEDIATELY(m, FALSE, TRUE);
					PED::SET_PED_TO_RAGDOLL(m, 3500, 6000, 0, FALSE, FALSE, nullptr);
					// (v1.6.1 review: knocked off to the side - away from the camp, toward the camera - not on into Bill and Javier)
					V3 sideM(-toM.y, toM.x, 0);
					if ((g_bal.pos + sideM - IL(0, 0)).len2d() < (g_bal.pos - sideM - IL(0, 0)).len2d()) sideM = sideM * -1.0f;
					V3 knock = toM * 0.5f + sideM * 0.85f;
					g_in.pushPed = m; g_in.pushVel = knock * 9.0f + V3(0, 0, 4.5f); g_in.pushUntil = t + 0.3f;
					ENTITY::SET_ENTITY_VELOCITY(m, g_in.pushVel.x, g_in.pushVel.y, g_in.pushVel.z);
					Log("INTRO the balloon takes Micah out on its way up (%.1f m off, %.1f s after lift-off)", dist, t - kIntroLiftOff);
				}
				else
					Log("INTRO the swoop missed Micah (%.1f m off, the basket %.1f m up)", dist, h);
			}
		}
		else if (t < g_in.climbUntil)
			g_bal.cmdVel = (g_in.swoopDir * 0.4f + away * 0.6f) * 5.0f + V3(0, 0, 5.0f);   // (up and away - from the funnel, not on into camp)
		else if (t < 57.0f) g_bal.cmdVel = away * 0.6f + V3(0, 0, Lerp(1.2f, 3.0f, Clamp((t - kIntroLiftOff) / 3.0f, 0.0f, 1.0f)));
		else if (t < 88.0f) g_bal.cmdVel = away * 1.6f + V3(0, 0, 1.6f);
		else g_bal.cmdVel = away * Lerp(4.0f, 12.0f, Clamp((t - 88.0f) / 4.0f, 0.0f, 1.0f)) + V3(0, 0, 3.0f);
		g_bal.boostShown = t < 55.0f ? 0.25f : 0.0f;   // (v1.6: the jets small after the lift-off - they hid Arthur in his close-ups)
	}
	// the lines: the characters' own voices, with the game's own subtitles
	IntroLines(t);
	// the title, at the end: the joke is that he knew
	if (Beat(16, t >= kIntroTitle0) && g_set.memeSounds) UI::PlayBoom(true);   // (v1.6.1: the soft one - playtest 15's "thunder", 12-17 dB over the voices)
	if (t >= kIntroTitle0 && t < kIntroTitle1)
		UI::ChapterCard("ARTHUR HAD A FEELING", "about the weather", Clamp(std::min(t - kIntroTitle0, kIntroTitle1 - t) * 2.0f, 0.0f, 1.0f));
}

static void IntroHandoff()
{
	// the camera eases from the chase shot into the gameplay camera behind the balloon; control is yours
	Ped me = PLAYER::PLAYER_PED_ID();
	// v1.6.1 (playtest 15: "show him escaping like normal, but flip the camera around when it hands it back"): the game's camera
	// comes back behind him looking the way he's escaping - away from the funnel - not the way his seat faces (back at it)
	{
		V3 bp = g_bal.on ? g_bal.pos : PlayerPos();
		V3 esc = FlatDir(ITp() ? ITp()->base : g_in.C, bp);
		float rel = HeadingTo(bp, bp + esc) - ENTITY::GET_ENTITY_HEADING(me);
		while (rel > 180.0f) rel -= 360.0f;
		while (rel < -180.0f) rel += 360.0f;
		CAMERA::SET_GAMEPLAY_CAM_RELATIVE_HEADING(rel, 1.0f);
		Log("INTRO handoff: the camera looks the way he's escaping (%.0f deg from where he faces)", rel);
	}
	CAMERA::SET_GAMEPLAY_CAM_RELATIVE_PITCH(-8.0f, 1.0f);
	if (g_in.cam && CAMERA::DOES_CAM_EXIST(g_in.cam))
	{
		CAMERA::STOP_CAM_SHAKING(g_in.cam, TRUE);
		CAMERA::SET_CAM_ACTIVE(g_in.cam, FALSE);
		CAMERA::RENDER_SCRIPT_CAMS(FALSE, TRUE, 2200, TRUE, FALSE, 0);
		g_in.oldCam = g_in.cam;   // destroyed after the 2.2 s ease (stage 5)
	}
	g_in.cam = 0;
	PLAYER::SET_PLAYER_CONTROL(PLAYER::PLAYER_ID(), TRUE, 0, FALSE);
	PED::SET_PED_CAN_RAGDOLL(me, TRUE);
	// the scene left him standing still / holding a pose: let go of that unless he's in the balloon
	if (!(g_bal.on && BalloonAboard()))
		TASK::CLEAR_PED_TASKS(me, TRUE, FALSE);
	IntroRestoreLook(true);
	// gameplay settings: the user's own, with Arthur shielded while he's in the balloon
	IntroRestoreSettings();
	IntroRestoreWeather();
	g_holdStorm = false;
	if (Tornado* tp = ITp()) tp->growSeconds = 6.0f;   // (v1.2: still scripted - it chews through the camp until the gang's thrown out)
	if (g_bal.on) { g_bal.intro = false; g_bal.startedAt = NowSec(); g_bal.helpShown = false; g_bal.boostShown = 0; }   // (audit 2: jets back to normal size)
	UI::Objective("Enjoy the view.", 6.5f);   // v1.3 (playtest 11: "find Matt... feels a little lame")
	g_in.stage = 5;
	g_in.afterStart = NowSec();
	Finding("INTRO played to the handoff | cast %d/%d | balloon %s", [] { int n = 0; for (auto& c : g_in.cast) n += c.ok; return n; }(), kCastCount,
		g_bal.on ? (g_bal.veh ? "vehicle" : "prop") : "missing");
	Log("INTRO handoff: control is back (gameplay)");
}

// v1.5 review: what a skip leaves behind - the skipped lines' subtitles (let go now, not asked for and dropped in one frame), a
// hold in progress, a line beside the camera
static void IntroSkipLoose()
{
	for (int i = 0; i < kIntroLineCount; i++)
	{
		const char* b = kIntroLines[i].block;
		if (!b || !g_in.linePlayed[i] || (g_in.blockReq[i] && g_in.blockGone[i])) continue;
		bool later = false, live = false;
		for (int j = 0; j < kIntroLineCount; j++)
		{
			if (!kIntroLines[j].block || strcmp(kIntroLines[j].block, b) != 0) continue;
			if (!g_in.linePlayed[j]) later = true;
			if (g_in.blockReq[j] && !g_in.blockGone[j]) live = true;
		}
		if (later) continue;
		if (live) HUD::TEXT_BLOCK_DELETE(b);
		for (int j = 0; j < kIntroLineCount; j++)
			if (kIntroLines[j].block && strcmp(kIntroLines[j].block, b) == 0) g_in.blockReq[j] = g_in.blockGone[j] = true;
	}
	g_in.holdHere = 0;
	g_in.nearUntilMs = 0;
	g_in.ambPed = 0;
	g_in.swoop = false; g_in.pushPed = 0; g_bal.skim = false;   // (v1.6.1)
}

static void IntroSkip()
{
	if (g_in.stage != 4 || g_in.clock > kIntroHandoff - 1.0) return;
	Log("INTRO skipped at %.1f s", (float)g_in.clock);
	IntroStoryStop();   // (no line carries on over the skip)
	if (g_in.clock >= kIntroSkipTo)
	{
		// already rising out of it: straight to the handoff (v1.3 audit: without the lines and the boom all at once)
		g_in.clock = kIntroHandoff;
		for (int i = 0; i < kIntroLineCount; i++) g_in.linePlayed[i] = true;
		g_in.firedBeats.push_back(16);
		IntroSkipLoose();
		return;
	}
	// fast-forward: the tornado in the camp, the gang in it, Arthur rising in the balloon
	g_in.clock = kIntroSkipTo;
	if (!ITp())
	{
		V3 p = IntroSettleSpot();
		p.z = GroundZ(p.x, p.y, g_in.C.z + 30.0f, g_in.C.z);
		SpawnOpts so;
		so.grow = 1.0f;   // down already
		Tornado* nt = SpawnTornadoAt(p, g_in.fwdH, g_set.style, GetStyles()[g_set.style].name, so);
		g_in.tp = TornadoRef(nt);
		if (nt) nt->scripted = true;
	}
	if (g_bal.on && g_bal.waiting) BalloonBoard(true);
	bool justLifted[kCastCount] = {};
	for (int i = 0; i < kCastCount; i++)
		if (kCast[i].ride != RIDE_STAY && g_in.cast[i].ok && !g_in.cast[i].lifted) { LiftRider(i, NowSec()); justLifted[i] = g_in.cast[i].lifted; }
	// (v1.5 review: a posed rider lifted just now starts out on its orbit, not by the funnel's axis - Lenny's close-up is next)
	if (Tornado* tp = ITp())
		for (int i = 0; i < kCastCount; i++)
			if (justLifted[i] && kCast[i].ride == RIDE_POSED)
			{
				CastState& c = g_in.cast[i];
				c.h = std::max(c.h, kCast[i].h0);
				c.r = std::max(c.r, std::max(4.0f, tp->radiusAt(Clamp(c.h / std::max(1.0f, tp->height()), 0.0f, 1.0f)) * kCast[i].rMul + 2.0f));
			}
	if (Ped d = CastPed(CA_DUTCH)) SetScripted(d, false);   // (v1.3 audit: a skip mid-throw left him out of the tornado's hands)
	MISC::SET_TIME_SCALE(1.0f);
	AUDIO::SET_AUDIO_FLAG("AllowScriptedSpeechInSlowMo", FALSE);
	for (int id = 1; id <= 23; id++) if (id != 16) g_in.firedBeats.push_back(id);   // every beat before this point (no gestures fired late)
	for (int i = 0; i < kIntroLineCount; i++) if (kIntroLines[i].t < kIntroSkipTo) g_in.linePlayed[i] = true;
	IntroSkipLoose();
}

void IntroUpdate(float dt, float t)
{
	if (!g_in.stage) return;
	Ped me = PLAYER::PLAYER_PED_ID();
	if (PlayerDead()) { IntroAbort("Arthur died"); return; }
	switch (g_in.stage)
	{
	case 1:   // fading out
		if (CAMERA::IS_SCREEN_FADED_OUT() || t - g_in.stageAt > 2.0f) { g_in.stage = 2; IntroBuild(); }
		return;
	case 3:   // settling: scenarios start, the balloon envelope appears, the first camera is placed
		UI::Letterbox(1.0f);
		IntroShots(0.0f);
		if (t - g_in.stageAt > 1.8f)
		{
			g_in.stage = 4;
			g_in.clock = 0;
			g_in.lastTick = g_introTick();
			CAMERA::DO_SCREEN_FADE_IN(1400);
			for (int i = 0; i < kCastCount; i++)
			{
				if (!g_in.cast[i].ok || !kCast[i].scenario || (i == CA_UNCLE && g_in.cast[i].held[0])) continue;   // (Uncle's in his chair)
				bool running = PED::IS_PED_USING_ANY_SCENARIO(g_in.cast[i].ped) != 0;
				Log("INTRO %s: %s %s", kCast[i].id, kCast[i].scenario, running ? "running" : "NOT running");
				// v1.6 (playtest 14: Pearson's stirring never started in six intros - he stood there stiff): ladling the stew instead
				if (!running && i == CA_PEARSON)
				{
					PlayAnim(g_in.cast[i].ped, "amb_camp@prop_camp_cauldron_serve_stew@male_b@base", "base", true, 4.0f);
					Log("INTRO pearson: serving the stew instead");
				}
			}
			{
				static int blocks, loaded;
				blocks = loaded = 0;
				for (int i = 0; i < kIntroLineCount; i++)
					if (kIntroLines[i].block && g_in.blockReq[i]) { blocks++; if (HUD::TEXT_BLOCK_IS_LOADED(kIntroLines[i].block)) loaded++; }
				Log("INTRO action (the first story lines' subtitles: %d of %d loaded; the rest are asked for as they come up)", loaded, blocks);
			}
		}
		return;
	case 4:
	{
		// real-time clock, so slow motion doesn't stretch the scene; a pause (long gap) doesn't count
		DWORD now = g_introTick();
		double step = (now - g_in.lastTick) / 1000.0;
		g_in.lastTick = now;
		if (step > 0.5) step = 0.016;
		{
			// v1.5 (playtest 13: lines were cut off by the next shot): at a cut, if a line that started in this shot is still being
			// said, the scene waits for it - up to 2.2 s at a cut, 8 s in all. Never at the cuts round Dutch's pass (v1.5 review):
			// he's thrown on this clock and flies in real time - a held cut would let him sail past before the slow motion
			float tc0 = (float)g_in.clock, next = IntroNextCut(tc0);
			bool throwCut = next >= kIntroDutchThrow && next <= kIntroSlowMo1 + 0.7f;
			if (tc0 + step >= next && !throwCut)
			{
				if (g_in.lineAt >= g_in.cutAt && IntroLineOn(tc0) && g_in.holdHere < 2.2f && g_in.holdTotal < 8.0f)
				{
					if (g_in.holdHere == 0) Log("INTRO %.1f s: holding the cut for a line still being said", tc0);
					g_in.holdHere += (float)step; g_in.holdTotal += (float)step;
					step = 0;
				}
				else g_in.holdHere = 0;
			}
		}
		g_in.clock += step;
		float tc = (float)g_in.clock;
		if (Pressed(g_keys.back) || PadPressed(PB_B)) IntroSkip();
		PAD::DISABLE_ALL_CONTROL_ACTIONS(0);
		HUD::HIDE_HUD_AND_RADAR_THIS_FRAME();
		UI::Letterbox(1.0f);
		IntroBeats(tc);
		IntroShots(tc);
		UpdateRiders(dt, tc, false);
		if (tc >= kIntroHandoff) IntroHandoff();
		return;
	}
	case 5:
	{
		// after the handoff: the letterbox slides away, the gang keeps orbiting for ~25 s, then they're thrown out one by one
		float a = t - g_in.afterStart;
		UI::Letterbox(1.0f - Clamp(a / 1.6f, 0.0f, 1.0f));
		if (a < 1.7f) HUD::HIDE_HUD_AND_RADAR_THIS_FRAME();   // v1.2: the HUD comes up as the bars leave, not before
		if (g_in.oldCam && a > 2.6f)
		{
			if (CAMERA::DOES_CAM_EXIST(g_in.oldCam)) CAMERA::DESTROY_CAM(g_in.oldCam, FALSE);
			g_in.oldCam = 0;
		}
		if (g_in.gradeK > 0)
		{
			g_in.gradeK = std::max(0.0f, g_in.gradeK - dt * 0.4f);   // ~2 s
			if (g_in.gradeK > 0) GRAPHICS::SET_TIMECYCLE_MODIFIER_STRENGTH(g_in.gradeK);
			else IntroClearGrade();
		}
		float tc = (float)g_in.clock + a;
		UpdateRiders(dt, tc, true);
		// v1.2 (INTRO.md: "it keeps chewing through the camp"): slow circles round the camp while the gang goes round, then the
		// world's - your Movement setting takes over once they've all been thrown out
		if (Tornado* tp = ITp())
		{
			if (a < 40.0f)
			{
				float ang = a * 0.25f;
				V3 target = g_in.C + V3(cosf(ang), sinf(ang), 0) * 9.0f;
				V3 d = target - tp->base; d.z = 0;
				float L = d.len2d();
				tp->scripted = true;
				tp->scriptVel = L > 0.5f ? d * (std::min(4.0f, L * 0.5f) / L) : V3();
			}
			else if (tp->scripted)
				tp->scripted = false;
		}
		// v1.3: the gang are ragdolls in the real tornado - it throws them out itself. Only the cow was flown by hand.
		if (a > 10.0f || !ITp()) ReleaseRider(CA_COW, t);
		if (a > 12.0f || !ITp()) ReleaseRider(CA_JAVIER, t);   // (v1.5: the posed ones)
		if (a > 14.0f || !ITp()) ReleaseRider(CA_LENNY, t);
		// v1.2: once they're down they get up, curse, and walk it off (INTRO.md promised it)
		for (int i = 0; i < kCastCount; i++)
		{
			CastState& c = g_in.cast[i];
			const CastDef& d = kCast[i];
			if (!c.ok || c.walked || i == CA_COW || i == CA_CAIN || !c.ped || !ENTITY::DOES_ENTITY_EXIST(c.ped)) continue;
			if (d.ride == RIDE_POSED && !c.released) continue;
			bool down = c.released ? t - c.releasedAt > 7.0f : (d.ride == RIDE_YEET || d.ride == RIDE_LOOSE) && a > 10.0f;
			if (!down || ENTITY::GET_ENTITY_HEIGHT_ABOVE_GROUND(c.ped) > 1.5f || PED::IS_PED_RAGDOLL(c.ped)) continue;
			c.walked = true;
			TASK::CLEAR_PED_TASKS(c.ped, TRUE, FALSE);
			TASK::TASK_WANDER_STANDARD(c.ped, 10.0f, 10);
			static const char* kGrumbles[] = { "GENERIC_CURSE_HIGH", "GENERIC_ANGRY_REACTION", "WHATS_YOUR_PROBLEM" };
			Speak(c.ped, kGrumbles[i % 3], "SPEECH_PARAMS_FORCE");
			Log("INTRO %s walks it off", d.id);
		}
		if (a > 40.0f)
		{
			IntroReleaseHorses();
			IntroReleaseDicts();
			IntroReleaseCast();
			g_in.stage = 0;
			UI::Letterbox(0);
			Log("INTRO done");
		}
		return;
	}
	default:
		return;
	}
	(void)me;
}
