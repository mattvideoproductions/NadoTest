# Tornado Redemption: a tornado mod for Red Dead Redemption 2 (story mode)

*Formerly NadoTest (the name you'll see in the video). Same mod, final version.*

Red Dead Redemption 2 has no tornadoes, so this mod builds one. A dark funnel reaches down out of the storm clouds. A ring of dust
blasts out when it touches down. Planks, barrels, wheels, whole logs and real trees spiral up its cone, and it pulls in people,
horses, wagons and props, whirls them round and hurls them across the map. Arthur too, if you let it.

It opens with a cinematic cutscene with the Van der Linde gang that runs straight into gameplay, plus:
- **voices**: Arthur, the gang and passers-by react out loud with the game's own lines ("Thanks for the lift.")
- **twelve kinds of tornado**: a Firenado, a Ghost Twister, a Snow Devil, a Dust Devil, a Waterspout, a Multi-vortex, a Junknado
  made of flying junk, and a cartoon **Toon Twister**
- a **tornado gallery** to see them side by side, and a texture gallery of the smoke they're made of
- a jet-powered hot air balloon, with a Storm chaser challenge
- **Storm season**, wild tornadoes that turn up on their own
- a tornado gun
- the tornado's own roar, and a storm report when it's over
- a menu in the game's own style

Made by **MattVidPro** with **Claude** (Anthropic's AI). Claude wrote the code from recordings of every playtest, and a second AI
(**Astra**) audited it between rounds. A video about how it was made is coming to MattVidPro's YouTube channel.

## ⬇️ Download
**[Get the latest release here](https://github.com/mattvideoproductions/TornadoRedemption/releases/latest)** and download `TornadoRedemption-v1.7.3.zip` (or just `TornadoRedemption.asi` and `TornadoRedemption.ini`).
`TornadoRedemption.asi` and `TornadoRedemption.ini` are also at the top of this repo, so the green **Code → Download ZIP** button
gets you a working copy too.

**Had NadoTest before?** Delete `NadoTest.asi` from your game folder (both would run at once). Your best scores and settings carry over.

> ⚠️ **Story mode only.** Script Hook RDR2 shuts the game down if you go online. Don't use this in Red Dead Online.
>
> ⚠️ **Back up your saves first.** Whatever the tornado throws stays where it lands, in your world. It can carry Arthur off in the
> middle of a story mission and fail it (you get a warning when you spawn one during a mission).
>
> ⚠️ **It's heavy in crowded towns.** The default **Performance: Balanced** suits most PCs; pick **Low PC** if it stutters, **High**
> on a fast machine.

## Quick start (5 minutes)
1. **Install Script Hook RDR2** (by Alexander Blade) from its official page, http://www.dev-c.com/rdr2/scripthookrdr2/. Copy
   `ScriptHookRDR2.dll` and `dinput8.dll` into your Red Dead Redemption 2 folder (the one with `RDR2.exe`).
   - Steam: right-click the game → Manage → Browse local files.
   - Rockstar launcher: Settings → My installed games → Open folder.
2. Download **`TornadoRedemption-v1.7.3.zip`** from the [latest release](https://github.com/mattvideoproductions/TornadoRedemption/releases/latest) and
   copy **`TornadoRedemption.asi`** and **`TornadoRedemption.ini`** from it into the same folder.
3. Start **Story Mode**. After loading you'll see *"Tornado Redemption v1.7.3 loaded"* at the bottom of the screen.
4. Press **`\`** (or **hold RB and press D-pad right** on a controller) to open the menu, then pick **Spawn tornado**. It touches down
   where you're looking, and the camera swings round to show you.
5. Run. Or don't. Or open **Modes & toys** and play the intro, **Storm Chasers**.

**Uninstall:** delete `TornadoRedemption.asi` and `TornadoRedemption.ini`, plus the `TornadoRedemption*.log`, `TornadoRedemption_*.txt`
and `TornadoRedemption_*.wav` files it writes.

## What's in it
- **The intro: Storm Chasers** (Modes & toys): a cutscene, then a run for your life.
  - From anywhere, the game takes you to the gang's camp at Horseshoe Overlook on a sunny morning (Rockstar's own camp, the gang at
    their own chores). Dutch, John, Micah and Arthur tell Susan Grimshaw and Abigail they're off; Micah and Arthur bicker.
  - They ride out, past Valentine, and pull up on a ridge above Emerald Ranch as the weather turns. Thunder. The tornado comes down
    on the ranch, and then it comes for them. John and Dutch run. Arthur, standing next to Micah, has a better idea.
  - Then it's yours: reach the hot air balloon that's appeared behind you (the dog's already in it), the tornado on your heels.
  - Every line is a real line from the game's story, in the characters' own voices, with the game's own subtitles.
  - The first time, the screen stays black a few seconds longer while it finds a road out of Valentine and a ridge over the
    ranch in your game (remembered in `TornadoRedemption_intro.txt`). Backspace (B) skips.
- **Jet balloon:** a hot air balloon with jets, right where you stand. It flies where you look, and the tornado leaves you alone
  while you're in it.
- **Storm chaser:** 90 seconds in the balloon. Get as close to the funnel as you dare; the closer, the faster the points. Touch the
  wall and it has you. Your best score is saved.
- **Survive the storm:** it hunts you and gets faster every 30 s. The clock stops when it lifts you off the ground.
- **Storm season** (main menu): tornadoes turn up on their own. The sky darkens, a warning says where, and a wild one touches down
  out on the map and wanders your way. Choose Rare, Regular or Frequent.
- **Tornado gun:** every shot you fire spawns a pocket twister where it lands, about as tall as a person. It wanders about for
  30 seconds, spinning up whatever's close (up to three at once). Shoot someone and they're spun round it, then knocked back.
- **Arthur's own page:**
  - Invincible on or off.
  - How the tornado treats him (Immune / Tugged / Grabbable / Easy prey).
  - **Landings:** Soft, Mixed or Real. Mixed is the default, and never fatal. A throw might be:
    - caught soft, or a hard landing that hurts;
    - to the stratosphere, or sky-high, 300 m up (never over a town);
    - a far throw, or a long, gentle glide down;
    - slammed down, or a meteor straight into the ground that doesn't hurt.
  - Things it throws at him.
  - Heal.
  - **Mash to get up:** knocked down by a twister? Mash A (or Space) to get back on your feet.
  - **The wide ride camera:** every other time it lifts you, the camera eases out to show the funnel round you, then back. You
    can steer it, and a throw ends it.
- **Town safety** (on by default): in the big towns the street furniture stays put. People, horses, wagons and loose things
  still fly. Saint Denis crashed the game without it.
- **Voices:** Arthur, the gang and passers-by react out loud with the game's own lines, chained into little exchanges.
  - Someone screams as it takes them and Arthur says goodbye.
  - One of the gang flies past; Arthur greets him by name and hears it's going badly.
  - It drops Arthur and he thanks it for the lift.
  - His horse is taken: he whistles, and nothing comes.

  When a storm is over, a **storm report** says what it did.
- **Twelve looks:**
  - **V Dark Vortex**: tall and near-black.
  - **W Wedge** (default): wide and dark, the classic.
  - **C Dark Column**
  - **R Rope**: a thin, snaking, old-timey twister.
  - **T Toon Twister**: a cartoon tube with stripes climbing it and a whipping, hopping tip.
  - **D Dust Devil**: short, tan and fast.
  - **F Firenado**: flame inside black smoke, glowing, throwing embers.
  - **G Ghost Twister**: pale and lit cold blue.
  - **S Snow Devil**
  - **H Waterspout**
  - **M Multi-vortex**: three funnels circling inside one.
  - **J Junknado**: its wall is made of flying junk.
  - **E**: a custom slot (pick its smoke in the texture gallery).

  The seven new ones use effects no playtest has tried in a funnel yet: judge them in the gallery.
- **Tornado gallery:** rooms of tornadoes side by side on the horizon, harmless and labelled: the classics, the elements, the odd
  ones, and one tornado built five different ways. A **texture gallery** shows every smoke a funnel can be made of.
- **Real trees** (experimental): it finds the map's trees in its path, makes the real one vanish and tears out a stand-in. Grass and
  bushes are flattened along its track.
- **Sound:**
  - **The tornado's roar:** a deep freight-train rumble with gusts and cracks, louder as it gets closer and from the side it's on. The
    mod makes it; there are no samples. Quiet by default (Off, Quiet, Normal, Loud).
  - **Optional mission music:** the game's own.
- **Arthur:**
  - **Levels:** Immune / Tugged / Grabbable / Easy prey.
  - **Fling and chase:** it throws him out and keeps coming.
  - **Landings:** eight kinds, from caught soft to a meteor into the ground (Mixed). He isn't invincible by default.
  - A **ride camera** circles him while he's up there.

## Controls (all rebindable in `TornadoRedemption.ini`)
| | Keyboard | Controller |
|---|---|---|
| Open / close the menu | `\` | hold **RB**, press **D-pad right** |
| Move / change / select / back (in the menu) | arrows / Enter / Backspace | D-pad / A / B |
| Spawn a tornado | `]` | RB + D-pad up |
| Despawn everything | `[` | RB + D-pad down |
| Cinematic mode (hide every label and the game's HUD) | `'` | RB + D-pad left |
| Camera: drone orbit → low angle → over the shoulder → normal | `.` | RB + Y |
| Skip to the next step of a test / cycle the balloon's jet flames | `N` | RB + A |
| Save a note to the log | `;` | RB + X |
| Skip the intro | Backspace | B |

**In the jet balloon:**

| | Keyboard | Controller |
|---|---|---|
| Fly (where you look) | W A S D | left stick |
| Up / down | Space / Ctrl | RT / LT |
| Jet boost | Shift | A |
| Bail out (the soft landing catches you) | F | Y |

**In a gallery:** left / right browse, up / down another room, Enter spawns that one (or, in the texture gallery, builds style E
out of it), Backspace (B) leaves.

You can still look around and walk or ride while the menu is open.

## The menu, in plain words
Every item explains itself at the bottom of the menu. The pages:
- **Main:** Spawn tornado, **Style**, **Arthur** (his own page), **Storm season**, Performance, Let it die down, Despawn everything.
- **Modes & toys:** The intro, Jet balloon, Storm chaser, Survive the storm, Tornado gallery, Texture gallery, Tornado gun, A wild
  storm now.
- **The tornado:**
  - Strength, Funnel size, Speed, Movement, Reach, Lifetime
  - Multiple tornadoes, Rips up real trees, Flattens grass, Debris cone, Impact bursts, Touchdown animation, People flee
  - **What it grabs:** people, animals, wagons, props
- **Arthur:** Invincible, In the tornado, Landings, Fling and chase, Things thrown at him, Ride camera, Heal Arthur
- **Weather, time & sound:**
  - Weather override (Storm clouds by default: a dark thunder sky, no rain), Dark sky, Lightning, Heavy rain, Wind
  - Tornado roar, Mission music, Voices, Chatter, Voice subtitles, Storm report
  - **Time & weather now:** set the hour, lock a weather, teleport. A weather you lock yourself is never changed by a tornado.
- **Camera & HUD:**
  - The **storm tracker**: a compass at the top that shows where it is, how far, and whether it's closing in.
  - Drone shots, Cinematic mode, Touchdown and Ride cameras, Camera shake, Map marker
  - **Menu look:** RDR2, or Simple if anything looks wrong on your setup.
- **Tests & tools:** a hands-free showcase, the self-test, a systems check, a drop test, the tree demo, a map-tree check, style
  tours, developer tools (including a voice audition that plays every reaction line).

**Set your own defaults** in `TornadoRedemption.ini`:
- `[Defaults]`: style, strength, size, speed, movement, reach, Arthur, fling, invincible, landings, things thrown at Arthur, cameras,
  real trees, flattened grass, Storm season
- `[Performance]`: Mode
- `[Storm]`: Weather, WindRoar, DarkSky
- `[Sound]`: TornadoRoar, RoarVolume, MissionMusic, Voices, Chatter, VoiceSubtitles, StormReport
- `[Intro]`: CampMap (Rockstar's camp, its base layout only, or the mod's own props)
- `[UI]`: Style

## The intro's lines
All eleven lines in the intro are the characters' real lines from the game's story, played by name as one line of one of
Rockstar's own conversations, so the words are exact (turn the game's Subtitles option on to see them). If the game won't play
one, an ambient line in the same voice plays in its place.

## How it works (short version)
- **The funnel:** looping smoke effects placed in the world and moved every frame along a cone that leans and sways with the tornado,
  plus one-shot dust, smoke and debris bursts.
  - Each plume sits at its own height, a golden angle round from the one below, so the column has no gaps or stripes.
  - Fill puffs plug what's left.
- **The spin:**
  - real props carried round the cone in three spiral arms
  - train-smoke streamers aimed along the spin
- **The vortex:** three zones.
  - **pull**: drags things in
  - **wall**: whirls them up the cone and throws them out
  - **eye**: a calm centre, experimental
- **Props with dead physics:** anything whose physics won't wake up (trees it rips out, wreckage it leaves hanging) is flown by the
  mod itself and laid down on the ground.
- **Real trees:**
  1. Rays from the funnel find narrow, tall, vertical trunks.
  2. The map's tree models loaded nearby are hidden in a small circle there.
  3. A spawnable stand-in tree of the same family is torn out instead.
- **The intro:** the camp is the map pieces Rockstar's camp script switches on for Horseshoe Overlook (put back afterwards), the
  gang take Rockstar's own camp spots, and every shot has a camera director that moves the camera until it can see who it's
  meant to show. The riders are moved between the three places at the cuts, each cut waiting for the next place to load.
- **The balloon:** the game's own balloon, steered every frame with the balloon natives Rockstar's balloon mission uses. If those
  don't take, it falls back to moving the balloon directly.
- **New ways to draw a funnel** (v1.2): each style can carry its own smoke colour or keep the effects' own (fire, steam, snow);
  point lights spiral up inside it (the Firenado's glow); a multi-vortex deals its plumes round three sub-funnels that circle the
  centre; a Junknado's wall is props the mod flies round the cone.
- **Voices:** the game's own ambient lines (`PLAY_PED_AMBIENT_SPEECH_NATIVE`), checked against each speaker's voice first, chained the
  way Rockstar's scripts chain them: the next line waits for the last speaker to finish.
- **The roar:** synthesized live (brown-noise rumble, a gusting band of wind, a howl, the odd crack) and played through Windows.
- **Built-in tests:**
  - The self-test spawns tornadoes, injects faults and checks for leaks.
  - The systems check covers the balloon, the gun, the Storm season warning, Arthur's voice lines and a Junknado in your game.
  - `TornadoRedemption.log` records everything; `TornadoRedemption_findings.txt` records results and your notes.

## Known issues and limits
- **Real trees are experimental.** Whether the hidden map tree's collision goes too, and whether every tree type hides, is still
  being checked. Turn it off under The tornado if it misbehaves.
- **The new looks are untested in a funnel.** The seven v1.2 types use smoke, fire, steam, snow and mist effects that haven't been
  seen in a moving funnel yet. If one draws badly, the gallery shows it straight away; tell us which.
- **Voices** use Rockstar's ambient lines, so what exactly is said varies; not every voice has every line (the mod falls back or
  stays quiet). Characters can't say "Matt".
- **The intro's places are found in your game** the first time: if its ridge isn't a good one, delete
  `TornadoRedemption_intro.txt` and it looks again. If Rockstar's camp pieces look wrong, set `[Intro] CampMap=Base` or `None`.
- **The balloon:** the game's balloon is known to sometimes appear without its cloth envelope (a game quirk); you can still fly the
  basket.
- **Mission music is Rockstar's score:** it may be claimed if you upload footage. It's off by default.
- **Trains are left alone** on purpose.
- **The smoke can't cast a shadow.** Smoke reads best in overcast weather, at dusk or at night; the default Storm clouds and Dark sky
  take care of that.

## FAQ
- **Does it work online?** No, and it can't: Script Hook closes the game if you go online.
- **Performance?** About 60 fps on a high-end PC with one tornado on High; less in crowded towns. Balanced and Low PC trade a little
  of the look for frames.
- **Updates?** In MattVidPro's words: no plans to maintain it, "but I will leave it completely open for you guys to modify". Fork it,
  change it, re-release it, make it yours.

## Building from source
1. **Visual Studio 2019 or newer** with "Desktop development with C++". `build.bat` finds it with vswhere (or set `VCVARS` to your
   `vcvars64.bat`).
2. **The Script Hook RDR2 SDK** from http://www.dev-c.com/rdr2/scripthookrdr2/. Its licence doesn't allow redistribution, so it isn't
   in this repository. Extract it into **`sdk\`** next to `build.bat`, so that you have `sdk\inc\main.h` and
   `sdk\lib\ScriptHookRDR2.lib` (or set the `SDK` environment variable to wherever it is).
3. Run **`build.bat`**. It produces `bin\TornadoRedemption.asi` and copies `TornadoRedemption.ini` next to it.
4. **Tests:** `tools\harness\run_harness.bat` runs the offline regression scenarios against a mock made from your SDK (control flow
   only, no game needed). Exit code 0 = all pass.
5. `src\nat.h` and `tools\harness\hashes.h` are generated by `tools\gen_natives.py` from alloc8or's rdr3-nativedb data
   (`research\natives.json`, not included; get it from the rdr3-nativedb-data repository).

| Folder | What's in it |
|---|---|
| `src\script.cpp` | menu, controller, HUD, cameras, tests, storm control, the main loop |
| `src\intro.inl`, `src\balloon.inl`, `src\season.inl`, `src\gun.inl`, `src\music.inl`, `src\checks.inl` | the intro and the modes, compiled as part of script.cpp |
| `src\voices.inl`, `src\gallery.inl` | v1.2: the voices and the storm report; the tornado and texture galleries (also part of script.cpp) |
| `src\tornado.cpp` | the funnel, puffs, debris cone, physics zones, flights (carried props and trees), real map trees, flattened grass, the floating-prop sweeper, soft landing |
| `src\ui.cpp` | the RDR2-style UI kit: the game's menu art, fonts and sounds, subtitles, letterbox, toasts |
| `src\roar.cpp` | the tornado's synthesized roar |
| `src\pad.cpp`, `src\input.cpp` | controller (XInput or the game's own controls) and rebindable keys |
| `tools\harness\` | the offline regression harness |

## Credits
- **MattVidPro**: idea, direction, every playtest
- **Claude** (Anthropic): code, analysis of the playtest recordings
- **Astra**: code audit
- **Alexander Blade**: Script Hook RDR2 and its SDK
- **alloc8or and contributors**: the RDR3 native database
- **femga and contributors** (rdr3_discoveries): the lists of models, effects, animations, scenarios and textures this mod is built on
- **Halen84**: the RDR2 native menu base (whose layout and textures the menu follows) and the published decompiled game scripts used
  as reference

## License
MIT. See `LICENSE`. Free to use, modify and redistribute. The Script Hook RDR2 SDK has its own licence and is not part of this
repository.
