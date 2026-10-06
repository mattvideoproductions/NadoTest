// NadoTest v1.1 - the tornado's roar (see roar.h).
// No thread: the script refills a queue of eight ~70 ms Windows audio buffers every frame. If the script stalls (a loading
// wait, the pause menu) the queue simply runs dry and it goes quiet.
#include "roar.h"
#include "common.h"
#include <cstdint>
#include <cstring>

namespace Roar
{
bool g_mute = false;
float g_master = 0.8f;
float g_lastAsk[3] = { 0, 0, 0 };

typedef MMRESULT(WINAPI* OpenFn)(LPHWAVEOUT, UINT, LPCWAVEFORMATEX, DWORD_PTR, DWORD_PTR, DWORD);
typedef MMRESULT(WINAPI* HdrFn)(HWAVEOUT, LPWAVEHDR, UINT);
typedef MMRESULT(WINAPI* DevFn)(HWAVEOUT);
static OpenFn pOpen; static HdrFn pPrepare, pUnprepare, pWrite; static DevFn pReset, pClose;
static bool g_tried = false, g_open = false;
static HWAVEOUT g_dev = nullptr;
static const int kRate = 22050, kFrames = 1536, kBufs = 8;
static int16_t g_pcm[kBufs][kFrames * 2];
static WAVEHDR g_hdr[kBufs];
static bool g_queued[kBufs];

// synth state
static float g_vol = 0, g_pan = 0, g_int = 0;          // smoothed
static float g_brown = 0, g_lp1 = 0, g_lp2 = 0, g_lp3 = 0, g_bp1 = 0, g_bp2 = 0, g_gust = 0, g_gustT = 0, g_crack = 0;
static double g_clk = 0, g_howlPh = 0;   // seconds played (double: a float clock stalls after ~17 min) and the howl's phase
static uint32_t g_seed = 0x1234567;
static inline float Noise() { g_seed = g_seed * 1664525u + 1013904223u; return ((g_seed >> 9) & 0x7FFFFF) / (float)0x3FFFFF - 1.0f; }
float Level() { return g_vol; }

static bool Open()
{
	if (g_open) return true;
	if (g_tried) return false;
	g_tried = true;
	HMODULE m = LoadLibraryA("winmm.dll");
	if (!m) return false;
	pOpen = (OpenFn)GetProcAddress(m, "waveOutOpen");
	pPrepare = (HdrFn)GetProcAddress(m, "waveOutPrepareHeader");
	pUnprepare = (HdrFn)GetProcAddress(m, "waveOutUnprepareHeader");
	pWrite = (HdrFn)GetProcAddress(m, "waveOutWrite");
	pReset = (DevFn)GetProcAddress(m, "waveOutReset");
	pClose = (DevFn)GetProcAddress(m, "waveOutClose");
	if (!pOpen || !pPrepare || !pUnprepare || !pWrite || !pReset || !pClose) return false;
	WAVEFORMATEX f = {};
	f.wFormatTag = WAVE_FORMAT_PCM; f.nChannels = 2; f.nSamplesPerSec = kRate; f.wBitsPerSample = 16;
	f.nBlockAlign = 4; f.nAvgBytesPerSec = kRate * 4;
	if (pOpen(&g_dev, WAVE_MAPPER, &f, 0, 0, CALLBACK_NULL) != MMSYSERR_NOERROR) { Log("roar: no audio device"); return false; }
	for (int i = 0; i < kBufs; i++)
	{
		memset(&g_hdr[i], 0, sizeof(WAVEHDR));
		g_hdr[i].lpData = (LPSTR)g_pcm[i];
		g_hdr[i].dwBufferLength = sizeof(g_pcm[i]);
		pPrepare(g_dev, &g_hdr[i], sizeof(WAVEHDR));
		g_queued[i] = false;
	}
	g_open = true;
	Log("roar: audio open (%d Hz stereo, %d x %.0f ms buffers)", kRate, kBufs, kFrames * 1000.0f / kRate);
	return true;
}

static void Fill(int16_t* out, float vol, float pan, float inten)
{
	// parameters glide across the buffer (no zipper noise)
	float v0 = g_vol, p0 = g_pan, i0 = g_int;
	for (int n = 0; n < kFrames; n++)
	{
		float k = (float)n / kFrames;
		float v = v0 + (vol - v0) * k, p = p0 + (pan - p0) * k, in = i0 + (inten - i0) * k;
		float w = Noise();
		// deep rumble: brown noise, then two low-pass stages (~60 Hz) and a high-pass (~18 Hz). v1.1 audit 2: it was mostly
		// below 20 Hz and drove the tanh into the clip half the time (an offline render: 56% of samples at the ceiling, now 0.1%)
		g_brown = g_brown * 0.98f + w * 0.06f;
		g_lp1 += (g_brown - g_lp1) * 0.018f;
		g_lp2 += (g_lp1 - g_lp2) * 0.018f;
		g_lp3 += (g_lp2 - g_lp3) * 0.005f;
		float rumble = (g_lp2 - g_lp3) * 6.0f;
		// gusts: a slow wobble plus the odd surge
		g_clk += 1.0 / kRate;
		const float g_lfo = (float)fmod(g_clk, 4096.0);   // slow LFOs (a seam every 68 min is inaudible under the noise)
		g_gustT -= 1.0f / kRate;
		if (g_gustT <= 0) { g_gustT = 0.6f + (Noise() + 1.0f) * 1.4f; g_gust = 0.35f + (Noise() + 1.0f) * 0.45f; }
		float surge = 0.65f + 0.35f * sinf(g_lfo * 1.3f) * sinf(g_lfo * 0.37f + 1.0f) + g_gust * 0.4f;
		g_gust *= 0.99995f;
		// whoosh: band of noise around 250-500 Hz that breathes with the gusts
		float fc = 0.06f + 0.04f * sinf(g_lfo * 0.8f);
		g_bp1 += (w - g_bp1) * fc;
		g_bp2 += (g_bp1 - g_bp2) * fc;
		float whoosh = (g_bp1 - g_bp2) * 2.2f * surge;
		// howl: a narrow, wandering whistle that comes in with intensity
		double hf = 180.0 + 60.0 * sin(g_clk * 0.5);
		g_howlPh += 2.0 * 3.14159265358979 * hf / kRate;
		if (g_howlPh > 6.28318530718) g_howlPh -= 6.28318530718;
		float howl = sinf((float)g_howlPh) * 0.08f * in * surge * (0.5f + 0.5f * w * 0.2f);
		// debris: rare cracks when it's violent
		if (g_crack < 0.001f && Noise() > 1.0f - 0.00012f * in) g_crack = 0.6f;
		float crack = g_crack * Noise();
		g_crack *= 0.992f;
		float s = (rumble * (0.7f + 0.3f * surge) + whoosh * (0.4f + 0.6f * in) + howl + crack * 0.5f) * v;
		s = tanhf(s * 1.2f) * 0.85f;
		float l = s * (p > 0 ? 1.0f - p * 0.6f : 1.0f), r = s * (p < 0 ? 1.0f + p * 0.6f : 1.0f);
		out[n * 2] = (int16_t)(Clamp(l, -1, 1) * 30000);
		out[n * 2 + 1] = (int16_t)(Clamp(r, -1, 1) * 30000);
	}
	g_vol = vol; g_pan = pan; g_int = inten;
}

void Update(float volume, float pan, float intensity)
{
	g_lastAsk[0] = volume; g_lastAsk[1] = pan; g_lastAsk[2] = intensity;
	if (g_mute) return;
	volume = Clamp(volume * g_master, 0, 1);
	if (!g_open && volume < 0.005f) return;   // nothing to say yet: don't even open the device
	if (!Open()) return;
	for (int i = 0; i < kBufs; i++)
	{
		if (g_queued[i] && !(g_hdr[i].dwFlags & WHDR_DONE)) continue;
		if (volume < 0.002f && g_vol < 0.002f) { g_queued[i] = false; continue; }   // silent: let the queue run dry
		Fill(g_pcm[i], volume, Clamp(pan, -1, 1), Clamp(intensity, 0, 1));
		g_hdr[i].dwFlags &= ~WHDR_DONE;
		g_queued[i] = pWrite(g_dev, &g_hdr[i], sizeof(WAVEHDR)) == MMSYSERR_NOERROR;
	}
}

void Shutdown()
{
	if (!g_open) return;
	pReset(g_dev);
	for (int i = 0; i < kBufs; i++) pUnprepare(g_dev, &g_hdr[i], sizeof(WAVEHDR));
	pClose(g_dev);
	g_open = false;
	g_dev = nullptr;
}
}
