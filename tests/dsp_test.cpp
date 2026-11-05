// Test offline cho lõi DSP (không cần JUCE): latency, clipping, mono, pha, sample rate, CPU, preset-like settings.
#include "../Source/DSP.h"
#include <chrono>
#include <cstdio>
using namespace TKdsp;

static std::vector<float> sine (double f, double sr, int n, float amp) { std::vector<float> v (n); for (int i = 0; i < n; ++i) v[i] = amp * (float) std::sin (2 * kPi * f * i / sr); return v; }
static double rms (const std::vector<float>& v, int a, int b) { double s = 0; for (int i = a; i < b; ++i) s += v[i] * v[i]; return std::sqrt (s / (b - a)); }
static int fails = 0;
#define CHECK(c, msg) do { if (!(c)) { ++fails; std::printf ("  FAIL: %s\n", msg); } else std::printf ("  ok: %s\n", msg); } while (0)

int main()
{
    const double rates[] = { 44100, 48000, 88200, 96000, 192000 };
    for (double sr : rates)
    {
        std::printf ("== Sample rate %.0f ==\n", sr);
        VocalChain ch; ch.prepare (sr);
        Settings s; // mặc định: EQ phẳng, HPF 80Hz, limiter -0.3, mọi module off
        s.hpfOn = false;
        const int n = (int) sr; auto x = sine (1000, sr, n, 0.3f); auto L = x, R = x;
        for (int p = 0; p < n; p += 512) { int m = std::min (512, n - p); ch.process (L.data() + p, R.data() + p, m, s); }
        const int lat = ch.getLatency();
        double rin = rms (x, n / 2, n - lat), rout = rms (L, n / 2 + lat, n);
        std::printf ("  latency=%d samples (%.2f ms)  in=%.4f out=%.4f\n", lat, 1000.0 * lat / sr, rin, rout);
        CHECK (std::abs (20 * std::log10 (rout / rin)) < 0.05, "chuỗi mặc định giữ nguyên mức (±0.05 dB)");
        double dL = 0; for (int i = n / 2; i < n; ++i) dL = std::max (dL, (double) std::abs (L[i] - R[i]));
        CHECK (dL == 0.0, "L==R khi vào mono (không lệch kênh)");

        // Preset nặng: bật tất cả module
        Settings h = s; h.hpfOn = true; h.nrOn = h.deessOn = h.compOn = h.mbOn = h.dynOn = h.excOn = h.trOn = true;
        h.clarity.on = h.presence.on = h.warmth.on = h.air.on = true; h.band[4].gain = 3; h.band[6].gain = 4; h.band[2].gain = -3;
        h.compMakeup = 3; h.mbLow = 1; h.outGainDb = 6;
        auto hot = sine (220, sr, n, 0.9f); auto hv = sine (6500, sr, n, 0.3f); for (int i = 0; i < n; ++i) hot[i] += hv[i];
        auto HL = hot, HR = hot; ch.reset();
        auto t0 = std::chrono::steady_clock::now();
        for (int p = 0; p < n; p += 256) { int m = std::min (256, n - p); ch.process (HL.data() + p, HR.data() + p, m, h); }
        double secs = std::chrono::duration<double> (std::chrono::steady_clock::now() - t0).count();
        float pk = 0; bool nan = false; for (int i = 0; i < n; ++i) { pk = std::max ({ pk, std::abs (HL[i]), std::abs (HR[i]) }); if (!std::isfinite (HL[i])) nan = true; }
        std::printf ("  peak out (all modules, +6 dB out, hot input) = %.4f (ceiling %.4f)  CPU=%.2f%% of 1 core\n", pk, dbToLin (h.limCeiling), 100 * secs);
        CHECK (!nan, "không có NaN/Inf");
        CHECK (pk <= dbToLin (h.limCeiling) + 1e-4f, "không vượt ceiling limiter (không clipping)");

        // Mono compatibility + correlation: stereo thật, width 1 → correlation 1 với nguồn mono; width 2 giảm
        Settings w = s; w.width = 1.0f; ch.reset();
        std::vector<float> A (n), B (n); unsigned seed = 12345; auto rnd = [&] { seed = seed * 1664525u + 1013904223u; return ((seed >> 8) & 0xFFFF) / 32768.0f - 1.0f; };
        for (int i = 0; i < n; ++i) { A[i] = 0.2f * rnd(); B[i] = 0.2f * rnd(); }
        ch.process (A.data(), B.data(), n, w); // một block 1 s để đo mono loss ổn định
        std::printf ("  uncorrelated: corr=%.2f monoLoss=%.2f dB\n", ch.meters.correlation, ch.meters.monoLossDb);
        CHECK (std::abs (ch.meters.monoLossDb + 3.0f) < 0.8f, "mono loss ≈ -3 dB cho tín hiệu không tương quan");
        // Out-of-phase: invL => correlation -1 và mono loss rất lớn
        Settings ip = s; ip.invL = true; ch.reset();
        auto C = sine (500, sr, n, 0.3f), D = C;
        for (int p = 0; p < n; p += 512) { int m = std::min (512, n - p); ch.process (C.data() + p, D.data() + p, m, ip); }
        CHECK (ch.meters.correlation < -0.95f && ch.meters.monoLossDb < -20.f, "phát hiện ngược pha (corr≈-1, mono loss lớn)");
        // Width 0 => mono hoàn toàn
        Settings w0 = s; w0.width = 0; ch.reset();
        auto E = sine (500, sr, n, 0.3f), F = sine (700, sr, n, 0.3f);
        for (int p = 0; p < n; p += 512) { int m = std::min (512, n - p); ch.process (E.data() + p, F.data() + p, m, w0); }
        double dd = 0; for (int i = n / 2; i < n; ++i) dd = std::max (dd, (double) std::abs (E[i] - F[i]));
        CHECK (dd < 1e-4, "Width=0 cho kết quả mono (L==R)");
        // Balance
        Settings bl = s; bl.balance = -1.0f; ch.reset(); auto G = sine (1000, sr, n, 0.3f), H = G;
        for (int p = 0; p < n; p += 512) { int m = std::min (512, n - p); ch.process (G.data() + p, H.data() + p, m, bl); }
        CHECK (rms (H, n / 2, n) < 1e-4 && rms (G, n / 2, n) > 0.1, "Balance -1 tắt kênh phải, giữ nguyên kênh trái");
        // Bypass
        Settings by = h; by.bypass = true; ch.reset(); auto I = sine (440, sr, n, 0.5f), J = I; auto I0 = I;
        for (int p = 0; p < n; p += 512) { int m = std::min (512, n - p); ch.process (I.data() + p, J.data() + p, m, by); }
        double be = 0; for (int i = n / 2; i < n - lat; ++i) be = std::max (be, (double) std::abs (I[i + lat] - I0[i]));
        CHECK (be < 1e-6, "Bypass trả về tín hiệu gốc (đã bù latency)");
        // Level match
        Settings lm = s; lm.compOn = true; lm.compThr = -30; lm.compRatio = 6; lm.compMakeup = 0; lm.levelMatch = true; ch.reset();
        auto K = sine (300, sr, 4 * n, 0.2f); auto K2 = K; auto K0 = K;
        for (size_t p = 0; p < K.size(); p += 512) { int m = (int) std::min<size_t> (512, K.size() - p); ch.process (K.data() + p, K2.data() + p, m, lm); }
        double dr = 20 * std::log10 (rms (K, 3 * n, 4 * n - lat) / rms (K0, 3 * n, 4 * n - lat));
        std::printf ("  level match: out-in = %.2f dB\n", dr);
        CHECK (std::abs (dr) < 0.6, "Level Match giữ mức ra ≈ mức vào");
    }
    std::printf ("\n%s (%d fail)\n", fails ? "FAILED" : "ALL PASSED", fails);
    return fails ? 1 : 0;
}
