// NadoTest v1.1 - the RDR2-style UI kit: the game's own menu textures, fonts (markup), sounds and feed notifications, plus the
// cutscene layer (letterbox, subtitles, chapter card, objective, honor toast) and a couple of sounds of our own.
// Sources: research\ui_world_research.md (Halen84's native menu base, Rockstar's scripts, femga's lists).
#pragma once
#include "common.h"

namespace UI
{
	enum Align { LEFT, CENTRE, RIGHT };
	extern bool g_simple;                  // [UI] Style=Simple: the plain v1.0 look (if the game's textures or fonts misbehave)
	extern bool g_clean;                   // Cinematic mode: Draw() keeps only the letterbox, the chapter card and subtitles

	void Preload();                        // asks for the game's texture dictionaries once at startup
	void Frame(float dt);                  // once per frame, before anything is drawn
	void Draw();                           // the timed widgets (letterbox, objective, help tip, shard, honor, toasts) - once per frame, last

	// text in an RDR2 face ("body" - the menu serif, "title" - the condensed caps header face, "Font5" - Redemption)
	void Text(const char* s, float x, float y, float scale, int r, int g, int b, int a, Align al = LEFT, const char* face = "body", bool shadow = false);
	float TextWidth(const char* s, float scale);    // rough width (screen fraction) for layout
	bool Sprite(const char* dict, const char* tex, float cx, float cy, float w, float h, int r = 255, int g = 255, int b = 255, int a = 255, float rot = 0.0f);
	void Panel(float x, float y, float w, float h, int a);   // the game's dark ink-roller panel (top-left anchored)
	void Sound(const char* name, const char* set);
	float Aspect();                        // width / height of the game window (for square sprites)

	// cutscene layer
	void Letterbox(float amount);          // 0..1, eased toward
	void Subtitle(const char* speaker, const char* text, float alpha);   // this frame only
	void ChapterCard(const char* title, const char* sub, float alpha);   // this frame only
	void PlaceCard(const char* place, const char* sub, float alpha);     // v1.4: bottom left, this frame only (where and when a scene is)
	void PromptMeter(const char* text, float fill);                       // v1.5: a button prompt with a fill bar, this frame only
	void Objective(const char* text, float seconds);
	void HonorLost(bool hudHidden = false);   // the game's toast, or (HUD hidden, or refused) the mod's own sting - one of them
	// notifications
	void HelpTip(const char* text, float seconds);   // top-left, RDR2 help-text style ("" clears)
	void Toast(const char* title, const char* text, float seconds);
	void Shard(const char* title, const char* sub, float seconds);   // big centred title card
	void ScorePlate(const char* title, const char* big, const char* sub, const char* right, float meter);   // this frame only
	int FeedToastIcon(const char* title, const char* sub, const char* dict, const char* tex, int ms);   // the game's own toast (0 = refused)

	// sounds of our own (Windows audio, no samples - synthesized at load) and optional voice files for the intro
	void PlayBoom(bool soft = false);     // v1.6.1: soft = 14 dB down (the intro's title)
	void PlayWhoosh(bool soft = false);   // v1.2: the jet burners lighting (v1.6.1: soft = 8 dB down, the intro's lift-off)
	void PlayVoiceFile(const char* id);    // NadoTest_intro\<id>.wav if it exists
	extern bool g_mute;                    // the harness mutes audio
}
