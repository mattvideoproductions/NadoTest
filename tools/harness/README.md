# Offline regression harness

Based on **Astra's** v0.5 audit harness (`ASTRA_AUDIT_EVIDENCE.zip`, 2026-09-26). It compiles the real mod `.cpp` files
against mocked Script Hook natives (a mock of the SDK: the same headers with `IMPORT` emptied, made at run time because the SDK may not be redistributed) and checks **control
flow only**: no rendering, physics, streaming, pools or game lifecycle. Passing here does not mean the mod works in RDR2.

Run: `run_harness.bat` (copies the current `src` into `work\`, makes the mock from your Script Hook RDR2 SDK - `sdk\` or the `SDK`
environment variable, see the main README - builds, runs). Exit code 0 = all scenarios FIXED.

Scenarios 1–9 are Astra's reproductions with expectations flipped; 10–12 cover v0.3.0 changes; 13–20 cover v0.4.0 (the tree
uproot, empty entity lists, the floating-prop sweeper, the menu keeping look/move, spawning outside the reach, dying down on death,
script-restart safety, soft landing); 21–31 cover v0.5.0 (world-space engine + heal, the sweeper on high ground / stuck floaters / its rate cap,
fling-and-chase, the empty-read top-up, tracker bearing maths, the style tour card and skip key, the hour picker, drone framings,
and a clean mock self-test); 32–41 cover v0.6.0; 42–49 cover v0.7.0; 50–54 cover v1.0.0 (Survive the storm, performance levels,
small hops, debris through the flight system, the ini [Defaults]); 55-72 cover v1.1.0 (the funnel density layout, the Toon stripes, real
map trees on a mock forest of trunks and walls, flattened grass, the intro end to end plus its skip and an abort, the jet balloon and
its drive fallbacks, Storm chaser, Storm season, the tornado gun, the storm/weather fixes, survival, the menu, the roar, UI text
markup); 73-75 are regressions for the v1.1 audit (a manual storm with the season Off, despawning a balloon high up, the gun
leaving the big tornado its smoke); 76-98 cover audit 2 (the intro's grade handoff at 144 fps, the intro's funnel, no streaming
wait mid-scene, an offline render of the roar, the minis, cinematic mode, Storm season, music in missions, poles vs trunks, the
hide cap, the intro's leftovers, the script restart and more); 99-110 cover v1.2 (voices: a scream then Arthur's answer, the landing
lines and fallbacks, gang small talk, pacing, Voices Off, the touchdown remark and the riding line; the storm report, also with
Voices Off; the new types' smoke, glow, junk walls and sub-funnels; both galleries, over a drone camera and switching; the intro's
gags and a skip); 111 reads the game's memory; 112-116 cover v1.3 (landings, things thrown at Arthur, the gun's minis, map
trees as entities, invincibility with minis and the menu; the intro scenarios follow the new screenplay); 117-121 cover the v1.3
audit (both throw paths give the landing picked, Real still catches a bail; thrown things capped and never at a wagon or a shielded
Arthur; the intro's story lines, one at a time, and their fallbacks; the skips; the real-tree scan leaving the mod's own trees
alone); 122-124 cover v1.4 (the gun's pocket twister; no stratosphere over a town and the 60 m/s cap; the intro's camera director
keeping Arthur's and Dutch's close-ups clear of trees planted in the way); 125-130 cover v1.5 (town safety on a Saint Denis street;
a shot person spun then knocked back; the pocket twister's whirls; mash to get up; the wide steerable ride camera; the intro's ears -
a second after every cut, held cuts within budget, far lines beside the camera, rolling subtitles); 131-132 cover v1.6 (the new throws
through the catch - sky-high climbs, far soft glides, the meteor dives and costs nothing, none of the far ones in town; the intro's
story-mode gestures, Micah knocked flat by the balloon, Dutch's pass across the frame, close-ups at Arthur's face, the gang going up
from the camp's edge); 133 covers v1.6.1 (the posed riders on carriers, and the camera handed back looking the way he escapes); 134 covers v1.6.2 (no object
list in Saint Denis, a back-off after an empty read; v1.7: the intro takes you out of the city).
**v1.7 (the new intro, "Storm Chasers").** The scenarios that tested the old intro were rewritten in place (same numbers): 59 the play-through
to the handoff, 60 an early skip, 61 death mid-scene, 72 the weather handed back, 76 the scene's sky and lightning, 77 the 10 s shield,
grabbable, boarding and the end, 78 no streaming waits, 88 / 107 what Despawn everything clears (and a lost balloon), 91 the push and
Micah into the funnel, 94b a script restart in the scene / the run for the balloon, 96 Arthur's horse and the 150 s time-out, 98 the
fallback camp and the anim dictionaries, 110 / 120 skips firing nothing late, 119 the story lines (over every row of kIntroLines)
and their fallbacks, 124 the camera director, 130 / 130b the ears, held cuts and the wait for a new place's ground, 132 the places
file (TornadoRedemption_intro.txt, backed up and put back), 133 the camp's map pieces (IPLs; v1.7.1: kept through the scene, put back once
it's over and Arthur is 350 m+ away, taken over by a scene started again); v1.7.1 adds 119b (a wrong take from a random set drawn
again) and turns 130b into the dips to black (waiting in the dark for the ground round Arthur). The mock grew IPLs (requested / removed /
active, or never coming up), collision per place and round an entity, mounts (a rider - Arthur too - is where its horse is, and is left there when he gets off), the take a conversation drew, entity blips, PAUSE_CLOCK, Arthur's
teleports, visibility and the intro's clock on every line; the mod's GetTickCount moves on 50 ms per mock WAIT, so a blocking
build loop with a real-time timeout ends quickly.
The mock keeps attached things where their parent is. The mock now has peds, speech (what was said, by whom, when; lines a voice lacks), scripted conversations (created, voices
added, the single line asked for, playing or not), vehicles, seats, attachments and a small
shape-test world, and records the timecycle modifier, music events, ragdoll / invincible / config flags, released models and
anim dictionaries; a scenario can count streaming waits instead of failing on one. Current result: see the last line of a run (137 checks since v1.7.1). The mock has crude physics (gravity + velocity for unfrozen objects) so the self-test can run
end to end: expect `clean mock run: 0 failures`. `hashes.h` is generated together with `src\nat.h` by
`tools\gen_natives.py`, so every native the mod uses is available to the mock.
