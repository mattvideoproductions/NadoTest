# Tornado Redemption changelog

The mod was called **NadoTest** up to v1.6.2.

Version numbers are **playtested releases** (decided 2026-09-27). Every build that goes to a playtest is archived in
`builds\<version>\` with its SHA-256. Playtest evidence (transcripts, frames, logs, findings) lives in

## v1.7.3 (2026-10-09, from playtest 21): the last beats
The user: "nearly perfect until john says it's right on us, then dutch takes a moment to deliver his line and everyone runs while he
does nothing, then it awkwardly cuts to micah who isn't even facing arthur".

- **Dutch** speaks 0.4 s into his shot and breaks into a run a second into his line.
- **Micah** turns to Arthur for "Well this is fun, ain't it?" - his back to the drop, the funnel coming up behind him, over
  Arthur's shoulder - and Arthur shoves him from the front (Rockstar's face-to-face shove), backwards off the ridge.
- The end is 1.6 s shorter (73.4 s to the handoff).

## v1.7.2 (2026-10-09, from playtest 20): the cut
fully in vegas myself, give some transition time" - it played through, every line heard, no crash.

- **Handles for the edit:** every shot has a lead-in (a line starts 1.2 s in) and a tail (a cut waits a second after the shot's
  last word). The scene runs about 75 s to the handoff; the edit trims it.
- **The farewell:** Dutch stands out in front of the line and asks Arthur, shot over Dutch's shoulder, with an upset John beside
  Arthur (the women out of it).
- **Faces:** Arthur angry in the bickering, Micah smug, John upset, everyone scared once the storm comes, Arthur cocky at the push.
- **Valentine:** the camera and the riders' path are found with the place: a camera that sees the riders and the town, a path clear
  of trees; slower horses.
- **The ridge:** the arrival camera is found with the place too (it sees the riders' marks and the ranch), the dip gives the ranch
  a moment to load, and the riders come up from behind the camera. Micah and Arthur's words there are in over-the-shoulder
  close-ups. The thunder shot keeps their heads clear of the letterbox.
- **The touchdown's hero shot:** a new shot down at Emerald Ranch, the funnel circling its yard and taking its cattle, pigs, a
  horse and farmhands (set down there for it).
- **The end:** John yells from behind the four (he runs only at the cut), Dutch gets his own shot to say his piece and run, Micah's
  last line is close on his face with Arthur behind him, and the push is over Arthur's shoulder.

## v1.7.1 (2026-10-09, from playtest 19): the crash, and the polish
seconds but what I saw was amazing great work - fix the FFFF error... polish fix correct pass".

- **The crash (ERROR FFFFFFFF):** it froze 2.5 s into the Valentine shot. In that window the scene had switched Rockstar's camp
  pieces back off and deleted the camp's people (the last line in the log), and a load-scene for the ridge, 1.5 km away, was
  running. Now:
  - the camp pieces are put back only once the whole scene and the run for the balloon are over and Arthur is 350 m or more from
    the camp, with the camp's people gone long before (and taken out of their camp spots first);
  - no load-scene at all. A move to a new place is a short dip to black: the horses go there first, frozen, the picture waits for
    the ground round Arthur to load (3 s at most), then they're put on it and set going as it comes back.
- **The opening** is longer (4.2 s), with the gang's title card bigger.
- **Dutch** turns to Arthur for his line. (Rockstar plays it as one of four takes at random, "You need some recreation, my boy"
  among them; all four are said to Arthur, and the one take that wouldn't fit is drawn again before it's heard. "Don't fall off."
  is drawn the same way.)
- **Susan** gets her own shot: "Well, hurry it along." with a shooing hand, then she walks back into camp; Abigail's word to John is
  in the same shot.
- **Micah and Arthur** in tight over-the-shoulder close-ups: over Arthur's shoulder for Micah's line, over Micah's for "Shut the
  hell up."
- **The ride out:** the riders take the clearest way out of camp (it's measured), in pairs, and Jack sits by the fire instead of
  wandering into the shot.
- **Valentine:** horses start at 4 m/s, not 6.5 (one reared up); more kinds of road are tried when it looks for the road.
- **The ridge** must have a real drop now (4 m or more if there's one; the drop counts for more than the height). The places are
  looked for again on the first run (the old remembered spots are ignored).

## v1.7.0 (2026-10-09): Tornado Redemption, and a new intro
For the video's cold open. The user: "FIX INTRO, 30s to 40s MAX AND LEGIBLE COMEDY", and "rename the mod so it's no longer
nado test... and the mod itself too, final version". Not playtested yet.

- **The name:** Tornado Redemption. The files are `TornadoRedemption.asi` and `TornadoRedemption.ini`, and the log, findings,
  best scores and tree scan follow. An install that still has only `NadoTest.ini` keeps using it; the best scores and the tree scan
  are copied over once. If the old `NadoTest.asi` is still in the game folder, a tip says to delete it (both would run at once).
- **The intro: "Storm Chasers"** (replaces "Arthur Had a Feeling"; 40 s; `INTRO.md` has the shot list):
  - from anywhere, the game takes you to the gang's camp at Horseshoe Overlook on a sunny morning. It's **Rockstar's own camp**:
    the map pieces their camp script switches on (put back the way they were once the scene has left), with the gang on Rockstar's
    own camp spots;
  - Dutch, John, Micah and Arthur tell Susan Grimshaw and Abigail they're going; Micah and Arthur bicker; they ride out, past
    Valentine, and pull up on a ridge above Emerald Ranch as the weather turns (sunny, cloudy, overcast, dark);
  - thunder once, far off; the tornado comes down on the ranch; John points ("It's right on us, come on!"), he and Dutch run ("We
    have a plan. My friends."), and Arthur pushes Micah off the ridge ("Don't fall off.") into the funnel;
  - then it's gameplay: *Reach the hot air balloon* that's appeared behind you, Cain in it, the tornado on your heels. 10 s it
    can't take you, then it can, until you're in. Boarding is "mission complete" ("Okay, here goes nothing.");
  - all eleven lines are real lines from the game's story, in three real back-and-forths (`research\intro_v17_lines.md`), with
    the game's own subtitles; Rockstar's own shove (the player's melee shove from behind), startle and pointing clips;
  - the road out of Valentine and the ridge are found in your game the first time (a road-network lookup and a ground scan) and
    remembered in `TornadoRedemption_intro.txt`; a cut to a new place waits (a little) for its ground to load;
  - no colour grade, no slow motion, no synthesized booms, and no lightning from the storm until you're in the balloon.
- **Removed with the old intro:** the `[Intro]` Hour, Grade, Outfits and MemeSounds settings, and the Meme sounds toggle. New:
  `[Intro] CampMap` (All, Base or None).
- The "mission complete" card is cream, not the failure red.

## v1.6.2 (2026-10-06, built from playtest 16): the crash, and the release
but it errored out" - another ERROR FFFFFFFF, the fifth.

- **The crash** (all five were in Saint Denis):
  - each of the four that left a log **ends on the same line**: the game's entity lists read empty, and the mod falls back to its
    last list - 521, 658, 199 and 595 entities, where 100-150 is normal;
  - then the game freezes for 12-21 s and gives up (its own fatal-error routine, the same address every time);
  - the likeliest reason: Script Hook's list read gives every entity it returns a script handle, and the game has only so many.
    Saint Denis, with its hundreds of props, more still from the air, is where they run out: first our reads come back empty, then
    the game's own scripts can't get one.
- **The fix:**
  - in and near the big towns (with Town safety on, the default), the object list isn't read at all, since there's nothing there
    the tornado may rip loose anyway; people and wagons still are;
  - after an empty read the mod waits 1.5 s before the next (it used to retry 10 times a second, straight into the crashes);
  - the extra diagnostic read at every empty read is gone.
- **The intro won't stage in Saint Denis:** the last crash came seconds after the intro's hand-back high over the city. Ride out of
  town first; a message says so.
- **Dutch crosses the slow motion, for real this time:**
  - a character who's already a ragdoll can't be moved, so he stayed in the funnel 60 m off and never made the shot;
  - he's taken out of the ragdoll, moved, put back in it, and steered;
  - if the move doesn't take, it's done once more, and where he passes is logged.
- **Tests:**
  - 125 now expects a big town's objects not to be read at all;
  - new 134: no object list in Saint Denis, a back-off after an empty read, and no intro in the city.

134 scenarios, all FIXED.

## v1.6.1 (2026-10-06, built from playtest 15): the intro, finished
pretty great and hilarious", with three notes.

- **"The thunder or lightning in that cinematic is too loud":**
  - the loud thing was the mod's own title boom, a synthesized 80-to-34 Hz thump that sounds like thunder, 12-17 dB over the
    voices; the lift-off whoosh was 10 dB over;
  - the intro now plays soft copies of both, the boom 14 dB down and the whoosh 8 dB down.
- **The swoop** ("fly the balloon into micah, knocking him over before taking off, all in one swoop... see ya... knocks over micah
  on the way out"). In v1.6 Micah never reached the basket: getting up out of his seat took him till lift-off, in all four intros.
  Now:
  - he's on his feet at the cut to his close-up and strides a few metres toward the balloon;
  - Arthur pulls the burner and the balloon skims low across the camp (the basket 35 cm off the grass) straight at him;
  - Micah goes flying, and the balloon climbs away past him;
  - it's filmed from the side of the swoop's path, then follows the balloon up.
- **The hand-back** ("show him escaping like normal, but flip the camera around when it hands it back"):
  - the game's camera came back looking at the tornado, because his seat in the balloon faces back the way he came;
  - the title shot and the game's camera now look the way he's escaping.
- **Lenny no longer goes round stiff:**
  - a ped frozen in mid-air doesn't animate, even with a clip playing;
  - the posed riders (Lenny, Javier, the cow) now ride an invisible carrier they're attached to, the way Uncle sits in his chair,
    and the carrier is what's flown.
- **Dutch crosses the slow motion:**
  - the v1.6 review's "clear his tasks first" stood him up out of his ragdoll, and the throw was lost;
  - he stays a ragdoll and is steered to the pass point beside the basket every frame until he's past.
- **Hosea's line** is at his face; v1.6's angle had him out of the frame. The bird's-eye shot of Micah is tighter.

**Tests:**
- the mock now keeps attached things where their parent is;
- 59 reads the posed riders through their carrier;
- 132 checks the swoop: low, through Micah, knocked on at 9 m/s, then climbing;
- new 133: the riders on carriers and let off when released, and the camera handed back looking the way he escapes.

133 scenarios, all FIXED.

**The review's fixes** (an independent read of the v1.6.1 code)
- **The balloon's bob would have wrecked the swoop:** its gentle bob builds up every frame (about 6 m/s at 60 fps), more than the
  swoop's height hold could fight, so the basket would have ground into the grass or floated over Micah's head. It's off during the
  swoop, and a hit needs the basket below 1.5 m.
- **Dutch was braked at the pass point:** steering him to arrive on time slowed him to 5 m/s right in the middle of the frame.
  He's now steered in direction only, at his throw speed.
- **The swoop goes through the rest of the gang** without touching them (Hosea and John stand near its path).
- **Micah is knocked off to the side,** away from the camp and toward the camera, not on into Bill and Javier. The balloon climbs
  away from the funnel, not on into the camp.
- **Smaller ones:**
  - a rider's carrier is deleted when they're let off;
  - a lost carrier freezes the rider, as before;
  - shot 13's tilt after the balloon is eased;
  - a soft sound file deleted mid-session is made again.

## v1.6.0 (2026-10-06, built from playtest 14 and the user's notes): the polish pass
"Didn't crash that time, felt pretty stable", and the intro is "nearly perfect... pretty funny".

**Gameplay**
- **The pocket twisters are back** (playtest 14: "my little cute pocket tornados are gone"). v1.5 drew them with the game's
  ambient whirls, which barely show; their own little funnel is back. The puffs that built the cloud stay off.
- **A little stronger** ("a little strength boost on those"): the pocket twister's pull is up a third; someone you shoot spins
  faster and is knocked back harder (14 m/s, was 11).
- **The wide ride camera is built round Arthur** ("a little too wide... I can't track or see Arthur... a natural transition to
  wide angle to see the tornado and situation, and then back"):
  - it starts about where the game's camera is and eases out to 22-38 m, Arthur in the middle and the funnel behind him;
  - then it eases back in and hands back (7 s). Look swings it round him; forward/back zooms.
- **A throw ends the ride camera** ("make sure it disables that camera when the tornado throws Arthur"): either one hands back
  the moment he's thrown.
- **Four more throws** ("an even farther one, a stratosphere higher in the sky one, a far soft landing, plus one more dramatic
  fast into the ground, but this one doesn't hurt you"). Landings: Mixed picks from all eight:
  - **far:** 50 m/s out and 30 up, keeping its speed through the catch: about 300 m. A hard landing.
  - **sky-high:** straight up at 55 m/s, still climbing at 50 m/s for 3.5 s: 300 m+. Never faster than 55 m/s on the way down.
  - **far soft:** out and up, then a long, gentle glide down at 4.5 m/s, drifting all the way.
  - **meteor:** a hop, then straight into the ground at 46 m/s: a burst of dirt, the camera jolts, and not a scratch.
  - Soft picks the glide now and then; Real picks only the ones that hurt. The far and sky-high throws, like the stratosphere,
    never happen over a big town.

**The intro** (`INTRO.md`)
- **Arthur's own gestures:** the hat tip, a rub of the eyes at "If you say so.", a cocky nod in the hero shot. They're from his
  story-mode conversation set. The Online emotes v1.2-1.5 asked for never loaded in story mode, and the wait for them held every
  intro on a black screen for 6 s.
- **Less T-posing:**
  - Pearson's stew scenario never started, so he stood stiff; now he ladles the stew instead;
  - Lenny went round the funnel stiff as a board; now he goes round still sitting, calm as you like ("I guess you know best,
    Arthur.").
- **Faces, not the backs of heads:**
  - every close-up of Arthur is in front of his face (after lift-off they were all the back of his head);
  - John is shot over Arthur's shoulder, at his face;
  - Uncle's chair faces away from the funnel, so his shot has his face with the funnel behind him;
  - Hosea is low and from the side, with the balloon going up past him.
- **Arthur flies into Micah** ("arthur should fly into micah ragdolling him with the hot air balloon as he takes off"):
  - Micah runs for the basket as he says "Reckon it's time we got out of here, Morgan";
  - the balloon's first lurch goes through him and knocks him flat;
  - the lift-off shot is from the ground beside the basket, tilting up after it;
  - later the camera looks down from the basket at Micah where he fell, as the tornado yanks him up past the lens.
- **The gang going up** ("that magic shot of the gang getting sucked up"): from the camp's edge, low and close. From 58 m out and
  9 m up, trees, roofs and distance hid it in all five intros.
- **Dutch's slow-motion pass:**
  - he was thrown from where the funnel had him, 60-99 m off at up to 76 m/s, and never made the shot;
  - now he comes out of the smoke beside the frame and crosses it between the lens and Arthur, 2.6 m from him.
- **Tighter shots,** "by just a little bit": Hosea's reveal, Dutch, Bill, Lenny and the hero shot are 15-25% closer. The opening
  crane starts high over the camp's clearing (in playtest 14 it began inside a tree).
- **Quieter thunder** ("the thunder lightning sfx... way too loud"):
  - the frontend lightning strike, 15 dB over the voices, is gone, and so is the flash at the funnel's touchdown;
  - the storm's own random lightning waits until the intro's over;
  - the balloon's jets are smaller in the intro, so they no longer wall Arthur in with fire.

**Tests:** scenarios updated for the new behaviour:
- 112, 117: eight landing kinds;
- 119: Dutch is thrown at the slow motion's cut;
- 124: the trunk in front of Arthur's face;
- 127: the pocket twister's own funnel;
- 129: the wide ride camera round Arthur, and a throw ending it.

New:
- 131: the new throws through the catch;
- 132: the intro's gestures, Micah knocked flat, Dutch's pass, faces in the close-ups, and the gang going up close.

132 scenarios, all FIXED.

**The review's fixes** (an independent read of the v1.6 code)
- **The glide could outlast its catch:** a long glide from 200 m+ ran past its 40 s window and dropped him unprotected. The glide
  now holds the window open until he's down (scenario 131 flies a whole 44 s glide).
- **The far throw** is protected from the top of its fall: 50 m/s sideways into a hillside.
- **The town check reaches as far as the throw goes:** 450 m for the far throw, 150 m for the stratosphere and sky-high.
- **The intro never waits for an animation the game doesn't have, nor for the stand-ins.** One wrong name had meant 6 s of
  black.
- **Micah's shot** swings to his side as he reaches the basket; 2.2 m ahead of him was inside it.
- **Smaller ones:**
  - no ride camera within 1.5 s of a throw;
  - the wide camera's swing round the funnel is smoothed;
  - the lightning that fell due during the intro doesn't go off the moment it's over;
  - the pocket twister's force is capped in m/s (Extreme had it spinning people at 47 m/s);
  - the tornado can't take him back while sky-high is still climbing;
  - a meteor over water becomes a soft landing;
  - Micah is never teleported to the basket if he's held up;
  - the knock comes sooner, before the basket lifts over him;
  - Dutch's tasks are cleared before he's moved, and his pass is 3.2 m off, clear of the rigging.
- **Harness:** 131 flies a whole glide and checks a launch is counted; 132 checks Dutch passes on the camera's side.

## v1.5.0 (2026-10-05, built from playtest 13 and the user's notes): the final pass
intro is really coming along... actually super funny now and readable", with a last list: two more crashes, the gun, the intro
("give ample time for the 'ears' of the game camera... a little more long... a few more key characters, lines, back and forth
and jokes... he should explain why he's leaving... hero shot"), mash to get up, and a wide steerable camera.

**The crashes: town safety** (on by default; menu toggle; `[Defaults] TownSafety`)
- **The pattern:** playtest 13 crashed twice (ERROR FFFFFFFF), both in Saint Denis, like playtest 12. All three crashes shared:
  - the city;
  - the tornado having ripped loose and carried off hundreds of its fixed props (lamp posts, wired poles, fences, signs);
  - then a sudden jump in where Arthur was: thrown at 90 m/s, killed and respawned 126 m away, a 104 m drop from the balloon.
- **The game stalls, then gives up:** the dump shows the game's fatal-error routine after a main-thread stall (9.5 s in crash 1).
- **The fix:** in the big towns (Saint Denis, Blackwater, Valentine, Rhodes, Annesburg, Strawberry, Armadillo, Tumbleweed, Van
  Horn) the street furniture stays put. No ripping it loose, no carrying it off, no long draw distance on map objects, and no
  real-tree hides. People, horses, wagons and loose things still fly.

**The intro: about 100 s, more of the gang, and the ears** (`INTRO.md`)
- **More of the gang:** Hosea, John and Lenny join.
- **20 real lines from the game, as back-and-forths** (`research\intro_v15_lines.md`):
  - Hosea: "Hey, where are you wandering off to?" Arthur: **"I'm leaving. Don't work too hard."**
  - John: "I thought you said you'd done this before?" Arthur: "Look, I'm trying to concentrate here."
  - Bill: "Ain't loyalty mean nothing to you?" Arthur: **"I ain't dying for whatever nonsense Micah and Dutch have concocted..."**
  - Dutch: "Faith, Arthur. Have a little faith." Arthur: "If you say so." Bill: "There's nothing to worry about." Then the thunder.
  - Micah: "Reckon it's time we got out of here, Morgan." Arthur, pulling the burner: "Now you want get out of here?"
  - Hosea: "Was that really necessary, Arthur?" Arthur: "If I had a dollar for every time I've been asked that."
  - Uncle: "Oh, I got lumbago, it's very serious!" Arthur: "Lumbago. Really..."
  - Javier, sailing past the basket still playing his guitar: "Mierda, we're high up here." Arthur: "You're telling me."
  - Lenny, in the funnel: "I guess you know best, Arthur."
- **The ears** (playtest 13: lines from far-off characters couldn't be heard):
  - Every line has its own shot of its speaker, so it's said close to the camera.
  - A line never starts in the first second after a cut.
  - A cut waits (up to 2.2 s, 8 s in all) for a line that's still being said.
  - A far-off speaker's line is played from beside the camera in their own voice (camp lines by their audio name).
- **The funnel waits 35 m out,** looming behind the camp for the last words, then sweeps through.
- **The hero shot:** Arthur at the burner, calm, a flick of the hat; past him the funnel takes the camp; Javier flies by.
- **Rolling subtitles:** the story lines' 17 text blocks are each loaded 8 s ahead and let go after.

**The tornado gun**
- **Shot people** (playtest 13: "it makes them spin and then sends them absolutely careening in a random direction"): they're spun
  round the twister for a second, then knocked back the way the bullet was going, away from Arthur, like a real hit. The twister
  still spawns.
- **Pocket twisters are drawn with the game's own whirls:** its ambient leaf, litter and dust swirls. There's no smoke, which had
  still built up into a cloud.

**Arthur**
- **Mash to get up:** knocked down by a twister but not in its hands, a prompt appears: MASH A (pad) / SPACE. Fill the bar and
  he's back on his feet.
- **The wide ride camera:** about every other time the tornado lifts him, the camera is a wide orbit round the whole funnel, and
  it's yours to steer: look to swing it round and up, move forward or back to zoom. It lasts while he's up there.

**Tests:** harness scenarios 125-130 cover:
- town safety on a Saint Denis street;
- a shot person spun, then knocked back;
- the pocket twister's whirls;
- mash to get up;
- the wide steerable ride camera;
- the intro's ears: a second after every cut, held cuts within budget, far lines beside the camera, rolling subtitles.

The intro scenarios follow the 100 s timeline. 130 scenarios, all FIXED.

**The final review's fixes** (an independent read of the v1.5 code)
- **Dutch's slow-motion pass:** a cut held for Arthur's "You're telling me." could freeze the scene clock while Dutch, already
  thrown, flew past in real time; the slow motion then started with him gone. The cuts round his pass are never held now, and
  Arthur's reply comes 0.6 s sooner.
- **Arthur's answer is heard:** the slow-motion shot followed the point between Dutch and Arthur, so it raced off after Dutch
  and Arthur's "Sorry, this ain't a good time." was out of frame and out of earshot. The shot never follows Dutch more than 8 m
  from Arthur now, and the answer has its own close-up (the hat-tip shot, from 90.6 s).
- **A line beside the camera** no longer holds the next cut for the full 2.2 s: its time runs in real time (the hold stops the
  scene clock).
- **Javier's guitar and the flailing** in the air are loaded with the rest (they could have flown round stiff).
- **Lenny's close-up** is a checked shot, never in the smoke, and a skip puts the posed riders straight on their orbit.
- **Smaller ones:**
  - a pocket twister still shows its whirls with Funnel render on Puffs only;
  - the mash prompt says A or SPACE by what you last used, and doesn't flash back while he gets up;
  - the wide ride camera stays on its own tornado, and a mouse steers it by how far it moves;
  - a skip lets the skipped lines' subtitles go cleanly;
  - town safety reaches as far as the tornado does;
  - Dutch's "Faith" falls back to Dutch's own voice.
- **Harness:** 130 now checks that cuts really are held (2.2 s at most, never with nothing being said, never round Dutch's pass).

## v1.4.0 (2026-10-05, built from playtest 12 and the user's notes): the release
fun, very ready to ship", with a few fixes: the crash, the intro, the gun's twisters, the wind sound.

**The crash at the end of playtest 12** (ERROR FFFFFFFF)
- **What the evidence shows:**
  - 17:49:54: the first stratosphere throw of the session launched Arthur at 90 m/s straight up over Saint Denis.
  - 17:49:59.5: five seconds later the game froze. The mod's log and the recording both stop on that frame.
  - The game's own crash report shows its last script update 21 s before the crash.
  - Its dump has the game's own fatal-error routine, called on its main thread.
  - There was no graphics-driver timeout, and memory was healthy (7 GB RAM, 7 GB VRAM).
- **What it can't show:** without the game's symbols, which part of the game stalled.
- **The fix:**
  - The stratosphere throw is 40-52 m/s, not 70-95. It's still a 100 m+ trip, but at speeds the game sees from a balloon.
  - It never happens over a big town (Saint Denis, Blackwater, Valentine, Rhodes, Annesburg, Strawberry, Armadillo, Tumbleweed,
    Van Horn); you get a hard landing instead.
  - Nothing the tornado does throws Arthur faster than 60 m/s.
  - Nothing is aimed at Arthur while he's in the air.

**The intro: the camera and the setup rebuilt** (`INTRO.md`, now 49 s)
- **The user:** "it should be more obvious what's going on... establish better, insert its context better, be more cinematic, use
  the game's animations and fix its current angles".
- **What playtest 12's frames showed:**
  - Arthur's three close-ups were the basket's wicker: the camera aimed 0.6 m above his body's centre, and he's down in the basket.
  - A tree trunk filled the wide shot of the camp going up, and another tree hid the funnel.
- **A camera director:** every shot names what it must show. If the wanted camera can't see it (a line-of-sight probe hits the
  basket, a tree, a hill or a wagon), the camera swings round, lifts or pulls back until it can, and stays out of the funnel's
  smoke. A correction mid-shot glides; the log names every shot it had to move.
- **Arthur is filmed from above the basket's rim,** aimed at his real head bone.
- **The setup tells the story before the storm:**
  - a place card ("VAN DER LINDE CAMP, 1899, an hour before the storm");
  - a crane down over the camp;
  - Dutch mid-speech;
  - the reveal of Arthur in the balloon as the gang turns to look;
  - Arthur's look at the sky, and what he sees (lightning on the horizon);
  - Dutch's "Faith, Arthur. Have a little faith.", Arthur's facepalm and "If you say so.";
  - then the thunder.
- **A shot for each beat of the storm:**
  - the whole balloon lifting off, seen from the ground;
  - Dutch's "WHAT?!";
  - Micah's "Every man for himself!" before he goes straight up (the camera tilts after him);
  - the camp going up, from the side.
- **The storm's roar is ducked** under the intro's lines: in playtest 12 the story lines in the storm half were drowned out (the
  calm ones were clear: "Faith, Arthur, have a little faith... If you say so.").
- **Loading waits 6 s at most** (it sat on a black screen for 8 s in playtest 12), and the log names anything still missing.

**The tornado gun: pocket twisters**
- The user: "you want them to be desktop size, like the size of a person or a little smaller", and "they make too much dust, you
  can't see anything".
- Each shot now makes a pocket twister: about 1.7 m tall, a 0.5 m wall and a 2.5 m reach (it used to be 25 m tall).
- Its smoke is small: 16 plumes at most. There's no ground storm, smoke collar, skirt, dust wall or debris ring, no smoke while it
  grows in, and it lands with a little poof instead of a blast. It stands up straight (its lean, wiggle and hop are scaled to its
  size).
- It wanders about where it landed for 30 s, spinning up whatever's close: violent for its size (things orbit it tight and are
  thrown at 14-21 m/s, not 40-60). It doesn't throw wagons or uproot trees.

**Quieter**
- The user: "the wind sound mode should be quiet and it should be quieter. normal can be quieter too".
- The tornado's roar defaults to **Quiet**, and every level is quieter: Quiet 0.45 → 0.22, Normal 0.75 → 0.45, Loud 1.0 → 0.75.
- The game's own wind picks up less near a tornado.
- A pocket twister is a whisper.

**From a code review of v1.4:**
- The camera's mid-shot glide only applies to the director's corrections. Gliding the whole position left the rising balloon's
  close-ups trailing 1.5-3.7 m below Arthur, back in the wicker.
- The camp goes up in its own wide shot (31-33 s), not during Micah's.
- Arthur's orbit is capped at 60 m/s as well; on Extreme it was driven at up to 84.
- Nothing is aimed at Arthur when the ground can't be found under him.
- The director's last resort stays out of the smoke and logs once per shot.

**Tests:** harness scenarios 122-124 cover:
- the pocket twister: its size, no dust, no cone, standing up straight;
- landings: no stratosphere over a town, the 60 m/s cap on launches and on his orbit, nothing thrown at a flying Arthur;
- the director: trees planted in the way of Arthur's and Dutch's close-ups, and every frame of those shots still sees its man; the
  camera stays above Arthur's head while the balloon rises.

The intro scenarios follow the new timeline. 124 scenarios, all FIXED.

## v1.3.0 (2026-10-05, built from playtest 11 and the user's notes)
stretches... new styles are all fun... love the UI overhaul", plus a list of fixes and changes.

**The intro, rebuilt: "Arthur Had a Feeling"** (44 s, `INTRO.md`)
- **Why:** playtest 11's frames show the old close-ups filmed from inside the funnel's smoke. It reads as a white fog in
  daylight, with a colour grade and the Dead Eye tint on top: "weird shading", "couldn't tell what was going on". Its dialogue was
  invented subtitles, so "I couldn't hear any of the characters". He also said "arthur saying chat lowk a lil cringey" and "find
  Matt... feels a little lame".
- **The new story:** Arthur sits in a hot air balloon in camp before the storm. The gang asks what's got into him, and he grumbles
  about the weather. The funnel drops. He pulls the burner and lifts off. Dutch: "WHAT?!" The camp goes up as ragdolls in the
  real funnel: Uncle with his chair, Pearson with his pot, Javier with his guitar, and a cow drifts past the balloon. In slow
  motion Dutch is thrown past the basket; Arthur tips his hat and says goodbye. Honor ▼, then the title card "ARTHUR HAD A
  FEELING".
- **Every line is the character's own voice,** with the game's own subtitle. Playtest 11 showed those subtitles appear in free
  roam.
- **Nine of them are their real story lines, by name** (the user: "we want to splice the character's actual lines... there are so
  so many voice lines"). Each is one line of one of Rockstar's own scripted conversations, played the way their camp scripts do it,
  so the words are exact:
  - Dutch: "We have a plan. My friends." and "Faith, Arthur. Have a little faith."
  - Arthur: "If you say so." Then the thunder.
  - Uncle: "Oh, I got lumbago, it's very serious!"
  - Micah, as he's yeeted: "Every man equal! Every man for himself!"
  - Pearson, going up with his pot: "I'm designed to float."
  - Dutch, thrown past the basket: "What is wrong with you, Arthur?" Arthur: "Sorry, this ain't a good time."
  - Dutch, still in it: "We are gonna be free!"

  One at a time: an answer waits for the question to finish. If the game won't play one in free roam (nothing is playing 1.2 s
  later), that line falls back to an ambient one from the old table. The log shows which (`INTRO ... STORY`).
- **Every camera is outside the funnel.** No colour grade by default (`[Intro] Grade=`), and no Dead Eye tint.
- The objective at the handoff is "Enjoy the view." There's no Matt and no "Hi, chat".

**Arthur gets his own page** (main menu → Arthur)
- **The page:**
  - Invincible
  - In the tornado (Immune / Tugged / Grabbable / Easy prey)
  - **Landings**
  - Fling and chase
  - **Things thrown at him**
  - Soft landings cost a little
  - Ride camera
  - Heal Arthur
- The main menu shows his state: "Arthur: grabbable, mixed landings".
- **Landings: Soft / Mixed (the new default) / Real.** The user: "we want it to be able to hurl around and hurt arthur too... fling
  to the stratosphere, towards the ground... make sure there is some variety".
  - In Mixed, every throw is one of these: caught soft; a hard landing, where he hits the ground in a heap and it hurts; launched
    to the stratosphere, 70-95 m/s straight up; or slammed down at the ground.
  - A Mixed landing costs a quarter to nearly half his health but never takes him below a fifth, so it never kills him.
  - Real is no catch at all.
- **Invincible is off by default now,** so the landings and debris can hurt him. One tap on his page turns it back on.
- **Things thrown at him:** "occasionally, rarely, throw something directly at arthur or near him... a few more things thrown out
  in his general direction". Every 7-16 s, while he's in its reach but not in its hands, a piece of the cone is thrown straight at
  him (35%) or just short. Half of the debris it throws from the top of the cone now lands round him.
- **Invincible covers the gun's minis too.** Playtest 11: Arthur died with only minis out; the lawmen shot him after the gun made
  him wanted.

**The tornado gun: cute mini twisters**
- The user: "cute little mini tornados that wander around, cause localized destruction, die down after 30 seconds, spin everything
  around it, small radius but still violent".
- Each mini is now a little **Toon Twister** that wanders about where it landed (it turns back before it strays 22 m) and dies down
  after **30 s**. Its reach is smaller, but its spin, lift and throw are 1.35-1.6x a full tornado's.
- The minis only shove Arthur about (he's the one shooting).

**Real map trees work now**
- Playtest 11's log rejected every hit (160+) as "an entity, not the map", and tore out 0 trees.
- The shape test returns the map's trees **as entities**. A hit on one of the 219 tree models is now taken as that tree: its own
  position is the trunk, and it's hidden and torn out.

**Fixes and tuning**
- **The jet balloon** starts on entity velocity. The balloon native never moved it in playtest 11 (2.6 m of 36 in the intro; 0 in
  the systems check), and entity velocity flew it every time.
- **Fewer screams:** 292 flyer lines in 10 minutes became a wall of noise. Now there's one lift scream every 0.8 s, and each flyer
  screams every 5-9 s.
- **The default style is W: Wedge.** The user: "rope, wedge, dark column, any of them can be our default... or decide for
  yourself". It reads best at a distance in playtest 11's footage, and playtest 6 called it the best.
- **The main menu** says where the time lives: "Weather, time & sound".
- **Meme sounds** is now the boom under the intro's title card.

**Code audit fixes** (a reviewer pass over the v1.3 changes)
- **A throw is the landing it picked.** When the tornado let go of Arthur at the top of his ride (not fling and chase), it picked a
  landing kind but never launched him that way: the stratosphere and slammed kinds hardly happened, yet still did their damage.
  Both paths now go through one launch, and his horse goes with him.
- **Things thrown at him are safe to be near.** They're never aimed at him in the intro, the balloon or a wagon, they're thrown
  from low on the cone (not dropped from the top), and nothing flies faster than 40 m/s (a drop from high up could reach 280).
  Immune Arthur only gets near misses.
- **Landings: Real still catches a balloon bail or the drop test.** Real is about the tornado's throws.
- **A landing kind never outlives its drop** (window closed, drop test, script reload).
- **The real-tree scan never takes the mod's own trees:** test trees, stand-ins, anything flying, or a map tree already torn out.
- **The intro:** Dutch is thrown 0.8 s before the slow motion, so he passes the basket in the middle of it (he passed after it).
  He's aimed where the balloon will be. A skip in the slow motion hands him back to the tornado. A late skip no longer fires the
  last lines and the boom all at once. A line from someone too far away to hear (60 m, 90 m for a shout) is skipped.
- **Invincible doesn't blink off** for a frame when a big tornado ends while the gun's minis are still out.
- The modes menu's intro entry is "The intro: Arthur Had a Feeling".

**Tests:** harness scenarios 112-121 cover:
- landings, and both throw paths (117);
- things thrown at Arthur: the speed cap, wagons and shielding (113, 118);
- the gun's minis (114);
- map trees as entities, never the mod's own (115, 121);
- invincibility with minis plus the menu (116);
- the intro's story lines and their fallbacks (119), and the skips (120).

The intro scenarios moved to the new timeline. 121 scenarios, all FIXED.

## v1.2.1 (2026-10-04): a memory monitor
The user, on the first launch of v1.2.0: "RDR2 is running out of memory pinging at 12 GB system... GPU pinged at 31 GB, closed
game" (before spawning anything). That session's log shows the mod loaded and idle: no tornado, no intro, no gallery, no slow
frames. The mod's idle footprint is a few menu textures, the core smoke library and a handful of debris models. The game runs on
Vulkan with Ultra textures on an RTX 5090 (32 GB), where RDR2 fills video memory as a streaming cache. So this release measures
instead of guessing:
- **MEM log line every 30 s from the moment it loads:** the game's RAM (working set, private, peak), its video memory against the
  budget Windows gives it on the biggest card (plus what spilled into shared memory), any jump of more than 1.5 GB in 30 s, and what
  the mod has out at that moment (tornadoes, smoke effects, props, intro, gallery, balloon).
- **Developer tools → Memory now:** the same numbers on screen.
- Harness scenario 111 reads the memory on the machine it runs on. 111 scenarios, all FIXED.

## v1.2.0 "CINEMA, the full cut" (2026-10-04, on top of v1.1.0; neither is playtested yet)
The user, after v1.1: "make sure that no details, features, animations, additions, ideas have been left behind", "Arthur custom
reaction to tornados... a line from the game, cleverly string them together, same for other characters and NPCs", "a tornado
texture gallery, tornado visual gallery", and "in the mod: different tornado comparisons, some new types, animations, methods for
creating tornado visually". Research: `..\research\speech_reactions_research.md`, `..\research\intro_extras_research.md`.

**Voices: the cast reacts** (Weather & sound → Voices / Chatter / Voice subtitles)
- Arthur, the gang and passers-by react out loud with the game's own lines (no samples), checked against each voice before they
  play. They're chained into exchanges:
  - Someone screams as it takes them; Arthur: a goodbye, or "sorry to hear that".
  - Someone asks "was this you?"; Arthur is caught out.
  - One of the gang goes round: Arthur greets him by name, he says it's going badly, Arthur answers.
  - It drops Arthur: "thanks for the lift", or his getting-up line. A passer-by remarks how muddy he is; Arthur: "lovely weather".
  - His horse is taken: he whistles, nothing comes, his "horse no-show" line.
  - Riding the funnel he sings; in the jet balloon he spurs it like a horse and shouts down at folks.
  - The tornado gun goes off next to someone: "cut that out", and Arthur laughs.
  - A tornado dying down: he's brave now ("won that fight").
- Rockstar's own speech params; the game's subtitle is asked for when a line has one. The lines are paced: never over each other,
  and Arthur doesn't remark on every one. Lawmen and outlaws, who have no panic lines, curse instead. Off / Arthur only /
  Everyone; Rare / Normal / Chatty.
- **Storm report:** when a storm is over, a card: how many it took (and how many of the gang), trees torn out, Arthur's longest ride
  and how high. Also written to `NadoTest_findings.txt`.
- **Voice audition** (Developer tools): plays every line the reactions use, one every 4 s with its name on screen, so a recording
  tells us exactly what each one says.

**Seven new tornado types, and new ways to build one** (the menu's Style, the ini's `Style=`)
- **D: Dust Devil**: short, tan and fast, whipping about.
- **F: Firenado**: a core of flame in black smoke, glowing from inside, throwing embers.
- **G: Ghost Twister**: pale, slow-turning, lit cold blue, made of the ghost train's steam.
- **S: Snow Devil**: wind-blown snow spun into a column.
- **H: Waterspout**: tall and thin, mist and spray.
- **M: Multi-vortex**: three thin funnels circling each other inside one storm.
- **J: Junknado**: the wall is made of planks, barrels, crates, wheels and doors climbing round it.
- The engine behind them:
  - Each style can have its own smoke colour (or the effects' own colours: fire, steam, snow).
  - Glow lights spiral up inside the funnel, with a fire flicker.
  - Sub-funnels.
  - A wall built from props.
  - A per-tornado render mode.
- These use effects from the game's "core" library that no playtest has tried in a funnel yet. The galleries are the place to
  judge them.

**Galleries** (Modes & toys)
- **Tornado gallery:** tornadoes side by side on the horizon, harmless, with their names over them, in rooms:
  - The classics.
  - The elements (dust, fire, snow, water).
  - The odd ones (ghost, multi-vortex, junk, your FX Lab pick).
  - **Five ways to build one:** smoke loops only, one-shot puffs only, rings that climb, props, glow.

  Left / right glides the camera between them, up / down changes room, Enter spawns that one for real.
- **Texture gallery:** every smoke, dust, steam and cloud effect a funnel can be made of, five at a time, each spun into a little
  twister. Enter builds style E out of one; the note key saves it.

**The intro, everything INTRO.md promised** (the traceability sweep found these written down but not staged)
- **Gag outfits** (`[Intro] Outfits=Gag`, the default): Uncle in his long johns from "Uncle's Bad Day", and Dutch in his party suit
  from the Saint Denis gala.
- **The gang in the air:**
  - Uncle naps in a folding chair, and the chair goes up under him for the rotisserie.
  - Pearson goes up with the stew pot and ladle in his hands, still stirring.
  - If Javier's guitar scenario drops in the air, he gets a new guitar and plays on.
- **Dutch:** he paces while he talks, and points at Arthur as he whirls past in the slow motion.
- **Bill asks "...Who's Matt?" and Micah facepalms.** Shot 6 is now a two-shot.
- **"Hi, chat." reads as a turn:** Arthur's looking off to one side first, and the camera sits off his nose.
- **"Matt's got a plan.":** Arthur tips his hat.
- **Sound:**
  - Rockstar's thunder crack on the touchdown lightning.
  - The game's own jump-cut sting (from "A Quiet Time") on the cut to the balloon.
  - A synthesized jet whoosh as the burners light.
- **Screams heard in slow motion:** speech is allowed during the slow motion (`AllowScriptedSpeechInSlowMo`, the flag Rockstar
  uses), so the gang's real screams in it (Bill's, at 30.6 s) aren't muted. Dutch's lines there are subtitles, as before.
- **At the handoff:** the HUD comes up as the letterbox leaves, not before.
- **After the handoff:**
  - The funnel keeps circling the camp until the last of the gang is thrown out; then your Movement setting takes over. Before, it
    went straight after your balloon.
  - Dutch stays up at TAHITI height instead of sinking.
  - The gang is thrown out, gets up, grumbles and walks it off.

**The jet balloon:** a fresh boost press makes Arthur yank the burner line (Rockstar's balloon clip), and the jets whoosh.

**Gallery hygiene:** gallery exhibits aren't "the tornado" for the tracker, the music, the cameras or the voices, and Storm season
waits until you leave the gallery.

**Menu:** every item has its explanation now (13 had none), and the page descriptions mention the new items. The v1.1 check is the "Systems check": it also lists which reaction
lines Arthur's voice has, checks a Junknado's prop wall, and opens and closes the tornado gallery.

**Audit 3 fixes** (a code audit of v1.2 and a requirements re-check; no crashes or soft-locks found)
- **Two serious bugs:**
  - **The gallery's camera** was switched off in the same frame if a drone, touchdown or ride camera was on. They're stopped first
    now, as the intro does.
  - **Arthur's riding line** could never play: his "lifted" line's cooldown blocked it and it was never retried. It now has the
    priority to cut in, and is retried until it plays.
- **The touchdown remark** never fired for a normal spawn. Now a near one gets its touchdown remarked on, a far one its first
  sighting.
- **The storm report** counted nothing with Voices Off. The watch now runs for the report too; with Voices Off its lines stay
  silent.
- **The intro's riders after the handoff:** the posed riders all screamed in the same frame, and Uncle kept shouting for help in his
  sleep. Peds the mod is flying don't scream now; the gang can still be greeted, once each.
- **The gallery:**
  - A key that closed the menu also acted in the gallery (Enter could spawn a real tornado).
  - Switching straight from one gallery to the other leaked the swatches' smoke.
  - The Style menu restyled the exhibits.
  - Exhibits ran a full debris cone each (220 props for a room) and called lightning down. Now: 10 cone props each, no throwing, no
    lightning.
  - A Junknado runs a third of the cone.
  - The gallery won't open over the intro's handoff or from the balloon, and stops Survive the storm and Storm chaser.
  - The texture gallery's page wrap lands on the right swatch, and its ground probes are cached.
- **Skipping the intro** fired all the v1.2 gags at once (the facepalm mid-flight, the hat tip while boarding). They're skipped too.
- **Colours:** the Firenado's embers glow instead of being darkened, and the Snow, Ghost and Waterspout puffs stay bright.
- **Small fixes:**
  - The ini reads style names as the menu writes them ("Firenado", "Dust Devil").
  - Dutch stays up at TAHITI height after the handoff.
  - The intro's funnel circles the camp until the gang's out.
  - An aborted intro un-scripts its tornado.
  - The voice scan stops once a storm is long over.
  - Vocal lines don't repeat.
  - The note key isn't doubled in the texture gallery.
  - A script restart turns the slow-motion speech flag off.
- **The build no longer undoes the ini:** `build.bat` copies the root `NadoTest.ini` into `bin\`, and the root copy was still v1.1.

**Tests:** harness scenarios 99-110 cover:
- voices: scream then answer, landing lines, gang small talk, pacing, Voices Off, the touchdown remark, the riding line
- the storm report, including with Voices Off
- the new types' smoke, glow, junk and sub-funnels
- both galleries: over a drone camera, switching, the Style menu
- the intro gags, and a skip

110 scenarios, all FIXED.

## v1.1.0 "CINEMA" (2026-10-04, built from playtests 9 and 10 and the user's v1.1 wish list)
One super-update: an intro cutscene that runs straight into gameplay, new ways to play, a denser funnel, real trees, sound, and a

**The intro: "Chapter VII: Finding Matt"** (Modes & toys; screenplay in `INTRO.md`)
- About 40 s in-game:
  - The Van der Linde gang at a camp built where you stand, doing real camp activities: guitar, stew, sleeping, writing.
  - They're dressed the way Rockstar's camp script does it.
  - Dutch works out someone called Matt is steering Arthur. Matt is: Arthur turns to the camera ("...Hi, chat."), with the Dead Eye
    tint and a boom, and raises his arm. A tornado drops out of the sky (the touchdown, sped up to 2.5 s).
  - The gang goes up one by one, still doing what they were doing:
    - Micah first, straight up.
    - Uncle asleep, slowly rotating.
    - Pearson stirring.
    - Javier playing.
    - A cow (Twister).
    - Dutch, still preaching, then TAHITI.
  - A slow-motion hero orbit. Arthur alone in the calm eye: "Sorry, Dutch. Matt's got a plan."
  - The jet balloon (with Cain the dog), and an honor-lost sting with the game's honor sound.
- **Seamless handoff:** the camera eases from the chase shot into the gameplay camera, the letterbox slides away, and you're flying
  the balloon. Objective: *Find Matt.*
- **Cutscene craft:**
  - script cameras with pushes and a punch-in
  - letterbox
  - game-style subtitles with the speaker's name
  - a chapter card
  - Rockstar's camp colour grade
  - real gang voices for the screams
  - a "talking" face mood on whoever has the line
- **Optional voice files:** any line can be voiced with `NadoTest_intro\<id>.wav`.
- **Skip and safety:** Backspace / B skips. Every exit path restores control, the camera, time scale, HUD and your settings. Arthur's
  horse is moved behind the camp and kept out of the tornado's reach.

**New ways to play**
- **Jet balloon:** the game's own balloon, flown with the balloon natives Rockstar's balloon mission uses, with automatic fallbacks.
  - Camera-relative controls: WASD / left stick, Space/Ctrl or RT/LT, Shift/A boost, F/Y bail out.
  - Jet flames, a burner and a smoke trail. The tornado leaves you alone up there.
- **Storm chaser:** 90 s in the balloon. The closer to the funnel, the faster the points (x1 to x10); touch the wall and it has you.
  Your best is saved.
- **Storm season:** wild tornadoes turn up on their own (Rare / Regular / Frequent).
  - Each storm: the sky darkens, a warning toast says how far and which way, and a lightning marker goes on the map.
  - The tornado touches down 380-520 m away and wanders toward you for 5-8 minutes.
  - Never during a mission or another mode.
- **Tornado gun:** every shot spawns a mini twister where it lands. Up to 3 at once, 15-25 s each, and no weather change.
- **T: Toon Twister:** a cartoon tube with level stripes of smoke climbing it and wrapping round, a big S-wiggle, and a tip that
  whips round and hops.

**The look (playtests 9-10: "leaving lots of gaps in the tornado's form")**
- **Each plume at its own height:** body layers no longer place 2-3 plumes at the same height. Each one is a golden angle round from
  the one below: twice the heights for the same budget, no strands.
- **One tip:** the core pulls into a single tip (the frames showed "two prongs").
- **A thicker wall:** the wall plumes sit a little in or out.
- **Fill puffs:** about 30% of the puffs aim at the funnel's surface wherever it's wider.
- **Streamers:** three bands instead of two, so the train smoke's chuffs overlap into a line.
- **Fair trimming:** the budget is trimmed in proportion, so Balanced keeps its ground ring and the cloud collar.
- **Dark Column** gets a dark core. **The debris cone** carries more props: 60 / 44 / 18.
- **Dark sky:** Rockstar's stormy-sky grade, eased in while a storm is out. **Storm clouds** is now the THUNDER sky with the rain
  held off (OVERCASTDARK never looked dark).
- **Flattened grass:** grass and bushes are flattened along its track (vegetation modifiers).

**Real map trees** (experimental)
- **How:**
  1. Rays from the funnel find narrow, tall, vertical trunks. Walls, rocks, posts and the ground are rejected.
  2. Only tree models whose art is loaded nearby are hidden, in a 2.4 m circle (capped at 600 hides a session).
  3. A matching stand-in (same model if spawnable, else the same family and height) is torn out.
- **Map tree check** (Tests & tools) answers the open questions. It tries Rockstar's own way to get a handle on a map object.

**Sound**
- **The tornado's roar:** synthesized live, with no samples:
  - a deep rumble
  - gusts
  - a howl
  - cracks of debris
  - louder as it gets closer, panned to its side
  - minis are quieter
- **Mission music** (optional, off by default): six of Rockstar's action scores, layered like a mission. It builds when one appears,
  kicks in when it's on you, and fades when it's over.

**Menu and HUD (the user: "consolidating and simplifying the menu with good explanations... match rdr2")**
- **Five pages:** a short main page, then Modes & toys, The tornado, Weather & sound, Camera & HUD, Tests & tools. Every item
  explains itself in the footer.
- **Drawn in the game's own style:**
  - the ink-roller panel, the header banner, the red crafting frame that glides between rows
  - tick boxes, arrows
  - the `$title` / `$body` fonts
  - the menu's own sounds
- **Animation:** a slide-in on open, a fade on page change. **Menu look: Simple** brings back the v1.0 boxes.
- **Tracker:** the game's fonts on its soft plate, a lightning-bolt marker, and it fades in.
- **Status messages:** a soft plate that slides up. The map marker is Rockstar's storm icon, pulsing.

**Fixes**
- **Survive the storm couldn't be lost** (playtest 10). "Held" was only recorded inside the fling-and-chase branch, which survival
  turns off, so it never caught you, and the ride camera never came on (the camera ended up in the treetops). Now recorded for
  every level.
- A run ended by hand no longer sets a best time. `NadoTest_best.txt` is now `name value` lines, so the inflated 2:05 from
  playtest 10 is ignored.
- **A spawn replaced your locked weather** (playtest 10): a weather you lock yourself always wins now.
- **Wind direction is in degrees** (Rockstar's scripts): the wind swirl only moved 6 degrees before. Wind and rain are handed back
  with -1 (v1.0 pinned the old wind speed). Lightning at a point passes -1, like every Rockstar call.
- The PERF log line said "Low PC" on Balanced.
- **The sweeper gave up on floaters when the flight slots were full** (a bucket left 67 m up in playtest 9). It retries now, and keeps
  known floaters for 5 minutes.
- **Hitch hunting** (playtest 9: a 735 ms frame): every frame is timed by part, and any frame over 100 ms logs which part took
  it (`SLOW FRAME`).

**Audit 2 fixes** (a code audit of the finished v1.1.0: 1 high, 6 medium, 20 low findings. L18's post-effect names were kept on
purpose, see `AUDIT_NOTES.md`; L16's Dutch-sinking part was finished in v1.2)
- **The colour grade popped at the intro's handoff**, and the Dark sky was then often lost (at high frame rates for good). The Dark
  sky now waits until the intro's grade has faded out, then eases in from nothing. Its ease is per second, not per frame.
- **The intro's funnel** settles about 6 m off Arthur (1.2 x its ground radius), on the far side from the close-up camera, so the
  calm-eye shots no longer film through its core. From 36 s it stays put instead of chasing his balloon, so the balloon gets away.
- The jets' effects load while the screen is black. Loading them at 36 s froze the letterbox and subtitles for up to 1.5 s.
- **The roar's rumble** was mostly below 20 Hz and clipped half the time. It's now a band of about 18-60 Hz with headroom.
- **Gun minis** gave off about 70% of a full tornado's puffs; now it's in proportion to their size. They also no longer start the
  ride camera, keep Survive the storm going (or end it), or flatten grass.
- **Cinematic mode** hides the mod's toasts, help tips, objective, honor sting and score plate again. Subtitles and the chapter card
  stay.
- **Storm season:** switching it Off no longer lets a wild tornado live on. Switching Weather override Off during the warning no
  longer leaves THUNDER locked.
- **Mission music** never sends the global stop over a mission's own score, and plays its end layer only if its start played.
- No intro during a story mission. The gun doesn't fire in a mission or an auto test.
- **Despawn everything** clears what a finished intro left in the world (only things that are still what the intro made).
- **Real trees:** telegraph poles and tall posts are rejected (thin, with nothing 10.5 m up). No stand-in where the hide cap runs out
  partway. A stand-in model that never streams in is released. Stand-in hashes are worked out once.
- **Smaller ones:**
  - The roar follows the camera that's drawn (the intro's, the drone's).
  - "&" is escaped in menu text; the toast structs have zeroed spare slots.
  - The honor sting is the game's toast or the mod's, never both. In the intro, with the HUD hidden, it's the mod's.
  - The tracker's closing speed goes by the tornado's id, not its address.
  - A script restart gives back the intro's camera, Arthur's ragdoll and the balloon flag, and resets the Map tree check.
  - The balloon is never deleted with Arthur in the seat, and Teleport takes him out of it first.
  - After the intro: the jets go back to normal size, the horse (invincible and calm during the scene) is given back, and the
    scene's anim dictionaries and unused camp models are released.
  - Late lifts in the intro are framed where the ped is; the cow no longer gets a human anim.
  - Best scores load at startup, so the menu's help shows them. Help text can run to 5 lines (Simple cut it at 3).
  - The balloon prop fallback no longer tries the cloth envelope on its own.

**Tests**
- **Offline harness: 98 fixed, 0 still broken.** The 54 earlier scenarios pass, plus 21 new: the intro end to end (and its skip and an
  abort), the balloon and its fallbacks,
  Storm chaser, Storm season, the gun, real trees on a mock forest, flattened grass, the density layout, the Toon stripes, the
  storm/weather fixes, survival, the menu, the roar, UI markup, and regressions for the audit fixes. 76-98 cover the audit 2 fixes.
- **Two independent code audits** (read-only agents):
  - **First audit:** 29 findings, all fixed. Among them:
    - Real trees would all have become the same stand-in, possibly hiding itself.
    - "A wild storm, now" did nothing with the season Off.
    - Removing the balloon high up skipped the soft landing.
    - The drone key could break the intro camera.
    - The intro leaked its THUNDER lock and rain.
    - The gang was never handed back to the world.
    - The gun permanently stripped the big tornado's smoke.
    - The roar's howl drifted into a siren.
    - A story reload left the modes' settings changed.
  - **Second audit:** found 27 more (see "Audit 2 fixes" above), fixed after this entry was first written.
- **In-game v1.1 systems check** (Tests & tools, about 30 s): the balloon spawns, seats Arthur, gets its envelope and climbs; a mini
  twister; a Storm season warning; the menu art and stand-in trees load.

## v1.0.0 (2026-10-02, built from playtest 8): the release
"This is very much shippable" (playtest 8). The last pass: everything from the final feedback round, plus a GitHub-ready source tree.

**New**
- **Survive the storm** (main menu): the tornado touches down where you look and hunts you, getting faster every 30 s.
  - The clock stops when it lifts Arthur off the ground, or when he dies.
  - Your best time is saved in `NadoTest_best.txt`.
- **Performance: High / Balanced / Low PC**, with **Balanced** the default. Balanced caps:
  - 80 looped smoke effects
  - 34 debris-cone props
  - 110 grabbed things
  - 16 props in flight
  - puffs x0.8
- **Weather override:**
  - **Storm clouds** (the default): a dark sky the funnel connects up into, no rain.
  - **Off**: your own weather, for a sunshine twister.
  - Thunderstorm and Rain are also available.
- **Wind roar:** the game's wind howls louder the closer the tornado gets.
- **`[Defaults]` in NadoTest.ini** sets the starting scene: style, strength, size, speed, movement, reach, Arthur, fling, invincible
  and cameras.
- **Default Arthur: Grabbable** (Arthur invincible stays on, and soft landing catches every drop).

**Fixes**
- A tornado that replaced another, or changed style live, could start with only a handful of its smoke effects, because the game
  hadn't freed the old smoke that frame. Refused effects now retry every 0.5 s until they start.
- **Tugged** is escapable: about a third of the pull.
- The fling is sometimes up to 1.6x farther.
- Tornadoes touch down farther out (reach + 80 m).
- The menu closes after **Spawn tornado**, so pressing A to run doesn't spawn a second one.
- The extra debris is carried by the mod and always lands. No more trails of floating barrels.
- Only real drops (from more than 3 m) count as landings.

**Polish**
- The camera shakes harder while the tornado has you.
- The smoke layers' shades are closer together: "consistent colour, only slight gradients".
- V Dark Vortex gets an extra wall ring to fill the gaps, and 4 fewer (expensive) streamers.
- The Spin check moved to Developer tools.

**Source / GitHub**
- **Script Hook SDK:** the build finds it in `sdk\` or via the `SDK` environment variable (it may not be redistributed, so it isn't in
  the repo).
- **Visual Studio:** found with vswhere (2019 or newer).
- **Harness mock SDK:** built from your own SDK at run time.
- Offline harness: **54 fixed, 0 still broken** (49 earlier + 5 new: survival, the performance levels, small hops, debris through the
  flight system, the ini defaults). The mock self-test is clean.

## v0.7.0 "ENTRANCE" (2026-10-02, built from playtest 7) — playtest 8 (2026-10-02)

**Look**
- **V Dark Vortex is the default** ("dark vortex is awesome").
- **Streamers that show:** playtest 7's Spin check proved the locomotive exhaust never rendered; the ambient train smoke did, and swirled.
  Every style's streamers now use it.
- **A dramatic entrance:**
  - while the funnel reaches down, dark smoke boils at its tip
  - at touchdown, a ring of dust and dirt blasts outward and lightning strikes the base
- Darker puffs (no "white cotton balls").
- Debris cone: 48 props (14 on Low PC), a bit faster, half as many big logs, and the logs tumble faster.

**Arthur**
- Whirled 40 % faster, and thrown about twice as far and higher ("a little bit of a cartoonish tornado").
- Fling-and-chase after 10-18 s (was 15-30).
- **Tugged** gets a low toss every 6-10 s ("throw me more").
- **Ride camera** (on): when the tornado lifts Arthur more than 6 m, the camera pulls out and circles him for 5 s, then hands back.
- **Landings hurt a little** (on): −20 health per soft landing, never below a quarter; nothing when invincible.

**Wreckage**
- Props flung off the debris cone always land, via the flight system ("trees frozen in the sky").
- Anything first seen more than 12 m up is never mistaken for a hanging sign.
- When a tornado ends, everything it left hanging within its reach is brought down.

**Performance**
- **One tornado at a time** by default: a new spawn makes the old one die down (Advanced → Multiple tornadoes to allow up to 6).
- With several out, the smoke budget is shared evenly; playtest 7's three tornadoes got 85, 24 and 1 effects.
- One press = one tornado (1 s debounce): playtest 7 had two spawns in the same second.
- Impact bursts only for hard hits (> 18 m/s, max 4/s); playtest 7 logged 1,583.
- The floating-prop sweeper runs 96 shape tests a second instead of 240.
- PERF log lines now include tornadoes, looped effects, cone props, flights and the puff rate.

**Other**
- **Changing Style** re-dresses tornadoes that are already out.
- Tracker plate: a soft Red Dead-style gradient, feathered edges and a cream rule.
- **Spin check round 2:** the visible train smoke at three aims, at double speed, and with the new **Wind swirl** experiment.
  - Wind swirl turns the wind with the funnel so all smoke drifts round.
  - It's in Advanced, off by default.
- Offline harness: **49 fixed, 0 still broken** (41 earlier + 8 new). The mock self-test is clean.

## v0.6.0 "SPIN" (2026-10-02, built from playtest 6) — playtest 7 (2026-10-02)

**The spin (the headline)**
- **Debris cone.** Up to 36 real props circle each tornado: planks, barrels, crates, wheels, doors, hay, and now and then a whole log.
  - They ride the funnel's own cone in three spiral arms: fast near the ground, slower higher up, so the arms wind into a visible spiral.
  - They tumble as they go, and the occasional one is flung out with real physics.
  - Despawn deletes them; dying down throws them out as wreckage.
  - This is what you described: "much more stuff moving around that circle, that creates the cone".
- **Things caught orbit the cone too**, tight near the ground and wider higher up, instead of on one fixed ring ("pull them more tightly
  towards the centre").
- **Black train-smoke streamers** (the locomotive exhaust you suggested) are aimed along the spin. Whether that makes the smoke itself
  swirl depends on which way the effect points. **Tests & demos → Spin check** shows five aims side by side; the best one goes into
  `NadoTest.ini [Visuals] StreamerPitch`.

**The look**
- Black smoke only: the pale bands are gone, and ground dust is brown-grey instead of white.
- **W Wedge is the default**, pulled into a tighter cone that comes up ("a tight little cone... it could be the best").
- **R Rope** is denser and snakes, with a wave travelling up the funnel ("animated like an old-timey twister").
- S+ and S removed. Styles: W, V, C, R, E.
- No random wood-splinter sprays. **Impact bursts** happen only where something thrown actually hits:
  - dust for people and animals
  - splintering planks for wagons
  - smashing glass or dust for props

**Fixes**
- **Floating wreckage comes down.**
  - Props still where the tornado found them (hanging signs, lanterns) are left alone.
  - Props it carried and left in mid-air get one physics wake, then fall under the mod's own gravity and are laid on the ground.
  - In playtest 6 the wake alone never worked: 93 "gave up", and the self-test barrel stayed 6 m up.
- **Props with dead physics move:** a prop that hasn't budged after 1.2 s in the wall is carried up it by the mod, thrown, and laid down.
- **NPCs keep flying:** in the wall they're steered by velocity and re-ragdolled every 2 s ("the funnier the better").
- **Lightning is rarer:** near the funnel every 45-90 s, rare every 70-140 s.
- **Tests happen where you're looking:** every spawn, test or not, goes where the camera looks, and Plant trees uses the camera too.
- **The tree scan runs once:** the result is cached in `NadoTest_trees.txt`, so the tree demo no longer scans for 40 s first.
- No pad combos during the controller check (RB, then Y, had switched the drone on). A hint line shows while a drone view is on.

**New**
- **Arthur:** Immune / Tugged (dragged about, never carried off) / Grabbable / Easy prey (rides it high). For streamers.
- **Weather:** Keep yours (the default; the mod leaves the weather alone) / Overcast / Thunderstorm / Rain.
- **Performance:** High / Low PC. Low PC uses half the smoke and puffs, 12 debris-cone props instead of 36, grabs at most 70 things,
  skips impact bursts, and keeps normal draw distances.
- **Touchdown camera** (on by default): every spawn swings the camera to the touchdown for 7.5 s, then hands back.
- **Off-screen tornado icon:** a small funnel at the screen edge with the distance, flashing while it's closing in.
- Menu reorganised: the main page is what you change while playing. Developer tools (render check, controller check, tree scan, FX Lab)
  sit one level down. The cloud experiments are gone.
- Self-test: checks the debris cone and that a carried, floating barrel comes down, and verifies every debris prop is deleted after
  the stress cycles.
- Offline harness: **41 fixed, 0 still broken** (31 earlier + 10 new). The mock self-test is clean.
- `release\`: a GitHub-ready README with a tutorial and warnings, and an MIT LICENSE.

## v0.5.0 "FINISH LINE" (2026-10-02, built from playtest 5) — playtest 6 (2026-10-02)
The final polish pass you called at 52:10 ("0.5 will be our final polish pass... then it should be ready to ship"). Evidence for every

**The look (the headline)**
- **World-space smoke is now the default engine.** Playtest 5's render check settled why the funnel flashed in and out: looped smoke
  attached to hidden props doesn't render (variants 1-4: "nothing", "a flicker"), while the same smoke placed in the world renders
  solid (variants 6-7). Until now the funnel you saw was mostly one-shot puffs. Every looped layer is now placed and moved in world
  space, so the whole cone shows. The old engine stays under Advanced → Funnel engine → Legacy anchors.
- New flagship style **V: Dark Vortex** (the default), built from your favourite, C Dark Column ("very easy to spot it"):
  - a near-black core, for "I wish it was a little bit darker"
  - **two pale spiral bands** that wind twice round the funnel from the ground to the cloud and turn as one piece
  - dark puff bands that sit exactly between the pale bands and turn with them, so it reads like a barber's pole
  - a wide, slow, dark **wall cloud** ring on top
- Why the bands matter: a uniform ring of smoke looks the same at every angle, so its spin is invisible. Stripes make the rotation
  show ("if it could spin in circles... chef's kiss").
- **Debris ring:** dark dirt and splinter bursts circle 15-80 m up just outside the funnel, sized to read from far away ("you can't
  really see it picking stuff up this far away").
- Things caught in the wall get a 1000 m draw distance, so a horse or wagon in the funnel doesn't vanish at range.
- The top emitters of every style now form that wall-cloud ring (wider, slower and darker than before).

**Fixes from the logs**
- **Floating-prop sweeper no longer teleports anything** (playtest 5: 836 props lying on the ground were moved).
  - Height now comes from a ground probe (`TrueHeight`). The old height function returned world Z for many props.
  - No probe result means no action.
  - At most two physics wakes per prop, then it gives up and logs it.
  - At most 20 wakes a minute, each one logged.
- **Soft landing actually lands you** (Arthur died at 17:18:24 after "falling -16.2 m/s at 15.8 m").
  - Real height, starting from 45 m up.
  - The fall speed is set every frame to a safe profile (about −24 m/s at 45 m down to −5 m/s at the ground).
  - Below 12 m, Arthur and his horse can't be damaged until 2.5 s after the landing.
  - Each landing logs its touchdown speed and health before and after.
- **The camp no longer runs dry.**
  - While the game's entity lists read empty, the mod asks the game for the peds near Arthur and adds the camp's respawned people and
    horses (`POOL fallback topped up N`).
  - Things already in the grip are kept for up to 30 s instead of 3 s.
- **Tree demo:** the tornado now travels in a straight line aimed at the middle tree, and the log reports how many came out. The tree
  scan skips snowy models (no more white firs in a green meadow).
- **Drone camera:** without a tornado, Low angle (1 m up, 12 m out) and Over the shoulder (behind Arthur, looking ahead) no longer
  look like Orbit.
- **Hour:** the "+1 hour / −1 hour" items are gone ("−1 hour" fired 13 times in 4 s). Pick the hour, then select.
- Ground-height jumps are eased over 1-2 s instead of snapping.

**New**
- **Fling and chase** (your idea, 53:57): after 15-30 s in the wall, Arthur (and his horse) are thrown out, soft landing catches
  them, and the tornado keeps coming. Not re-grabbed for 10 s. Main menu toggle, on by default.
- **HUD: Tracker** (the new default). One slim plate at the top centre, out of the webcam corner:
  - a compass strip with a marker for where the tornado is relative to where you look (`<<` / `>>` when it's behind you)
  - the zone you're in (or TORNADO), the distance, and "closing N m/s", "moving away" or "holding"
  - a danger edge coloured by zone
  - no in-world style label; HUD **Full** brings back the dev info box, labels and zone banner, and **Off** hides it all
- **Finish-line run** (Tests & demos, hands-free, about 2 min): storm, touchdown 170 m out, drone orbit, low angle, then it comes for
  you with Grab Arthur and Fling and chase on. Ride it, get thrown, land soft, then it dies down. The log keeps score (`FINISH-LINE RUN
  result`).
- **Drop test** (12 s): lifts Arthur 40 m and lets go. `DROPTEST PASS/FAIL` with the touchdown speed and health.
- **Style tour:** 25 s per style, "Style 2 of 6: name" on the card, and a **next-step key** (`N` / pad RB + A) to move on early.
- **Self-test** (about 1.5 min, about 34 checks):
  - tornado A runs the default world-space engine; its fault injection removes a looped effect, which must restart
  - tornado B runs the legacy anchors; one of its anchors is deleted, which must be re-created
  - new: a barrel lying on the ground must not move (the regression check for the 836 teleports)
  - stress cycles alternate the two engines
- Eye of the tornado moved to Advanced as "(experimental)".
- Offline harness: **31 fixed, 0 still broken** (20 earlier + 11 new). The mock self-test runs clean (33 passed).

## v0.4.0 "LEVEL UP" (2026-09-27, built from playtest 4) — playtest 5 (2026-09-28)
**Controller and menu**
- Controller support: RB + D-pad right opens the menu; D-pad moves/changes, A selects, B goes back. Quick combos for spawn, despawn,
  cinematic mode, drone camera and notes. All rebindable in `[Controller]`. Reads XInput directly and falls back to the game's own
  control actions (for pads RDR2 reads natively); the log says which source worked.
- The camera and walking/riding stay live while the menu is open (only the controls the menu needs are blocked).
- Pad hints in the menu footer once you use the pad; long help text wraps instead of running off the box.
- New main-menu items: **Speed** (slow 3.5 / normal 6 / fast 10 m/s) and **Drone camera**.

**Spawning and movement**
- User spawns land where the **camera** looks, just **outside the tornado's reach** (+40 m), so you watch it come.
- A tornado chasing Arthur slows down once it has him (it lingers instead of racing off).
- An arrow at the screen edge shows where an off-screen tornado is.
- A warning when you spawn during a story mission (playtest 4 failed one).

**Looks**
- Styles are now built from layers: a slow, dark, dense **core** inside a faster **wall**. New flagship **S+: Layered Supercell**.
  Dust Rope and Smoke Wedge (which washed out against the bright sky) are rebuilt as **R: Rope** and **W: Wedge** with cores.
  D: Hybrid is retired.
- One-shot puffs: 45 → 70 per second, in 3–4 tight helix bands that climb and turn with the funnel. The rate backs off by itself if the
  game starts refusing effects.
- The core is tinted darker and the wall lighter, for depth.
- Tours (style, strength, tree demo, render check) keep the storm locked for their whole run: no more sky flashing between styles.
- Experiments: anchor draw distance raised to 1500 m (`AnchorLodDist`), and a **world-space** funnel engine with no anchor props
  (Advanced → Funnel engine).

**Physics**
- Fixed: planted trees were never uprooted ("uprooted 0" all of playtest 4). The "rip fixed props loose" rule reached them first.
- New scripted uproot: tear out with a dirt burst, a frantic tumble while riding up the wall, thrown at a random height, then the game's
  physics takes over, or (for trees without physics) the mod flies it and lays it down where it lands.
- Props tumble harder when they're big and when Strength is higher.
- **Floating-prop sweeper**: props the tornado let go of that hang in mid-air get their physics woken; if that fails twice they're set
  down on the ground. Things resting on tables, roofs or wagons are left alone.
- **Soft landing** for Arthur (on by default): for 12 s after the tornado had him, he's slowed just before hitting the ground.
- When the game's entity lists come back empty (playtest 4: 0/0/0 for about 70 s around a death and respawn in Rhodes), the last good
  list is kept and the read is retried within 0.1 s. Logged as `POOL EMPTY` / `POOL back after`.

**Death, reloads, UI**
- Tornadoes die down when Arthur dies (toggle in Advanced). All mod UI hides while dead, paused or faded out.
- After a story reload (Script Hook restarts the script), every old handle is forgotten without touching the game.
- Sandstorm and Hurricane (a sunny sky) removed from the weather lists.
- Cloud experiments apply as soon as you change them (in playtest 4 they were changed but never applied). Pick-then-select items
  (hour, weather, teleport) say so in their help line.

**Tests and tools**
- **Render check**: a hands-free, 12-step test of why looped smoke sometimes vanishes; you just say what you see.
- **Controller check**: logs which buttons reach the mod and through which source.
- **Self-test** now also checks the tree uproot and the floating-prop sweeper.
- **Drone camera**: orbit, low angle, over Arthur's shoulder (for B-roll).
- Offline harness: 20 scenarios (12 carried over + 8 for v0.4), all passing. Control flow only; no game involved.

## v0.3.0 (2026-09-26) — playtest 4
Everything built after playtest 3 (internal dev builds called v0.3, v0.4, v0.5 and v0.5.1 in chat and in Astra's audit) plus all six
Astra audit fixes: eye / wall / inflow zones, weather lock, touchdown and dissipate with fade, a shared world pool, the anchor
registry, self-healing effects, the self-test, auto tests, FX Lab, tree tools, the zone banner, cinematic mode, and the build log line.
See `AUDIT_NOTES.md`.

## v0.2 (2026-09-25) — playtests 2 and 3
The particle budget, fixed ground following, the Dark Column style, puffs, bigger reach.

## v0.1 (2026-09-25) — playtest 1
First playable tornado: looped smoke on spinning anchors, a vortex that pulls people, wagons and props, and a menu.
