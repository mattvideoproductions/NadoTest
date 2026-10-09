# The intro: "Storm Chasers"

An in-game cutscene (about 75 s with its handles) that runs straight into gameplay. Pick **Menu → Modes & toys → The intro: Storm Chasers** from
anywhere in story mode (not during a story mission). The screen goes black, the game takes you to the gang's camp at Horseshoe
Overlook near Valentine, and the scene plays. Backspace (or B on a controller) skips to the end.

The first time you play it, the screen stays black a few seconds longer: the mod finds a road out of Valentine and a ridge over
Emerald Ranch in your game and remembers them in `TornadoRedemption_intro.txt`. Delete that file to make it look again.

**The premise:** a sunny morning before the great storm. Arthur, Dutch, Micah and John ride out to watch the weather over Emerald
Ranch. The weather comes to watch them.

## Where it comes from
- **v1.3 to v1.6** were "Arthur Had a Feeling" (Arthur escaping camp in a balloon; 100 s at its longest). What carries over:
  - every line is the character's own voice, with the game's own subtitles;
  - the camera director: a shot that can't see who it's meant to show swings round, lifts or pulls back;
  - the ears: no line starts in the first moments after a cut, a cut waits for a line still being said, and a far-off speaker
    is played from beside the camera.
- **v1.7** (for the video's cold open): "30s to 40s MAX AND LEGIBLE COMEDY", the real camp on a sunny morning, the four at the
  camp's edge telling Susan Grimshaw and Abigail they're off, Micah and Arthur bickering, the ride out past Valentine in worsening
  weather, a ridge over Emerald Ranch, thunder once, the tornado on the ranch, "it's coming this way", the others run, and Arthur
  pushes Micah off the ridge into it. Then you run for the hot air balloon. Cain's in it.
  - The camp is **Rockstar's own**: the map pieces their camp script switches on for Horseshoe Overlook, put back the way they were
    once the scene has left the camp, with the gang on Rockstar's own camp spots doing their own chores. (If the game won't switch
    them on, the mod's own props stand in. In a chapter 2 save the real camp is already there and is used as it is.)
  - Every line is a real line from the game's story, found in its dialogue (`research\intro_v17_lines.md`).

## Beat sheet (seconds)
A line starts no sooner than 0.7 s after a cut. Times can slip a little: a cut waits for a line still being said (up to 2.2 s),
for the next place's ground to load (up to 2.5 s), and the fall waits for the push.

Every shot has a lead-in and a tail for the edit (v1.7.2), so the scene runs about 75 s; cut it down in the edit.

| Time | Shot | What happens | Line (real, by name) |
|---|---|---|---|
| 0.0 | Over the camp | Sunny morning at Horseshoe Overlook, the gang about their chores, down to the four at the camp's edge. Big card: *The Van der Linde Gang. Valentine, 1899. The morning before the great storm.* | |
| 5.5 | Over Dutch's shoulder | Dutch, in front of the line, asks Arthur; John beside him is upset. | **Dutch** (one of four takes): "Come on, let's get out of here for a bit." / "Are you coming?" / "Let's go. You need some recreation, my boy." |
| 10.3 | Over John's shoulder, at the women | Susan shoos them off and walks back into camp. | **Susan:** "Well, hurry it along." · **Abigail** (to John): "Will you go rest, please?" |
| 15.6 | Over Arthur's shoulder | Micah, smug. | **Micah:** "You sure you got the lungs for this, Morgan?" |
| 20.2 | Over Micah's shoulder | Arthur, angry. | **Arthur:** "Shut the hell up." |
| 23.6 | Riding out | Out of camp the clear way, in pairs. Clouds. | |
| (dip) | | | |
| 27.6 | Leaving Valentine | The town behind them; they come up and past. Card: *Valentine. Late morning.* | |
| (dip) | | | |
| 32.0 | The ridge | The ranch below, then the four ride up past the camera and pull up. Card: *Emerald Ranch. That afternoon.* | |
| 36.5 | Over Arthur's shoulder | Micah. | **Micah:** "Weather don't worry me. In fact, I like it." |
| 41.3 | Over Micah's shoulder | Arthur. | **Arthur:** "Funny, I didn't think lizards survived in the mountains." |
| 46.4 | Thunder | One flash far off; all four look up and flinch. | |
| 48.4 | Over their shoulders | The funnel comes down on the ranch; the camera leans in. | |
| 52.4 | Down at the ranch | The hero shot: the funnel on the ranch, its cattle and farmhands going up. | |
| 57.0 | Behind the four | The funnel coming up the valley at them. John points. | **John:** "It's right on us, come on!" |
| 60.6 | Dutch | John's already running; Dutch says his piece, running before he's finished. | **Dutch:** "We have a plan. My friends." |
| 63.6 | Over Arthur's shoulder | Micah turns to Arthur, his back to the drop, the funnel behind him. | **Micah:** "Well this is fun, ain't it?" |
| 67.3 | Closer, the same side | Arthur shoves him from the front, backwards off the ridge. | **Arthur** (one of four takes): "Don't fall off." / "Boo." / "Am I bothering you?" / "Don't lose your focus." |
| 70.2 | From the ridge | Micah goes over, falls, and the funnel takes him up. | |
| 73.4 | Gameplay | *Reach the hot air balloon.* | |

**After the handoff:**
- The tornado comes up over the ridge after you. It can't take you for the first 10 s; after that it can, until you're in the
  balloon (then your own Arthur setting is back).
- Walk into the balloon and you're in: **Arthur:** "Okay, here goes nothing." *Storm Chasers: mission complete.* Fly it as usual.
- Dutch and John keep running; the tornado may well have them. Micah is thrown out of it after a while.

## The places
| Place | Where | How it's found |
|---|---|---|
| The camp | Horseshoe Overlook, (-125.9, -40.0); the four meet at Rockstar's muster spot (-111.1, -24.1) | fixed (Rockstar's camp script) |
| The road | the nearest road point 130 m out of Valentine toward the ranch, and which way it runs | the game's road network, once |
| The ridge | 100-250 m from Emerald Ranch (1332, 300): higher than the ranch, a drop in front toward it, flat enough to stand on, room for the balloon 26 m behind, a clear view of the ranch | a ground scan, once |

## Settings (`TornadoRedemption.ini`, `[Intro]`)
- `CampMap=All` (default): Rockstar's camp pieces; `Base`: only the base layout; `None`: the mod's own props.

## Logged (`TornadoRedemption.log`, `INTRO` lines)
The places found, which camp pieces came up, who took one of Rockstar's camp spots, every line (`STORY root[index] "words"`,
whether it played and how soon, or the stand-in), every held cut, cameras the director had to move, the push and how far out the
funnel was, when the funnel has Micah, the balloon, and the run (boarding, or why it ended).
