// Tornado Redemption v1.1 - the tornado's roar. The user, after v1.0: "wind or tornado sfx... can we add that in at all during a
// tornado... like during a regular nado experience". The game has no tornado sound, so this one is synthesized live (no
// samples): a deep rumble, a gusting whoosh, a wind howl and the odd crack of debris, louder the closer the funnel, panned to
// where it is. It plays through Windows (it mixes with the game's own sound and goes quiet when the game is paused).
#pragma once

namespace Roar
{
	// once per script frame. volume 0..1 (0 = silent), pan -1 (left) .. 1 (right), intensity 0..1 (how violent)
	void Update(float volume, float pan, float intensity);
	void Shutdown();
	extern bool g_mute;      // the harness
	extern float g_master;   // [Sound] RoarVolume
	float Level();           // the volume it's playing at (for the log)
	extern float g_lastAsk[3];   // the last volume / pan / intensity asked for (the harness reads it)
}
