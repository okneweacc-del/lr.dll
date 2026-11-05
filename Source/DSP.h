// TK VOCAL STUDIO - lõi DSP (header-only, không phụ thuộc JUCE để có thể test offline)
// Không cấp phát bộ nhớ trong process(); mọi buffer được cấp phát trong prepare().
#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <vector>

namespace TKdsp
{
constexpr double kPi = 3.14159265358979323846;
inline float dbToLin (float d) { return std::pow (10.0f, d * 0.05f); }
inline float linToDb (float x) { return 20.0f * std::log10 (std::max (x, 1.0e-9f)); }
inline float coefFor (double ms, double sr) { return (float) std::exp (-1.0 / (std::max (ms, 0.01) * 0.001 * sr)); }

// ---------------------------------------------------------------- Biquad (RBJ, TDF-II, double)
struct Biquad
{
    double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
    void reset() { z1 = z2 = 0; }
    inline float process (float x)
    {
        const double y = b0 * x + z1;
        z1 = b1 * x - a1 * y + z2;
        z2 = b2 * x - a2 * y;
        return (float) y;
    }
    void set (double nb0, double nb1, double nb2, double a0, double na1, double na2)
    {
        b0 = nb0 / a0; b1 = nb1 / a0; b2 = nb2 / a0; a1 = na1 / a0; a2 = na2 / a0;
    }
    static double cf (double f, double sr) { return std::clamp (f, 10.0, sr * 0.45); }
    void setPeak (double sr, double f, double q, double gdB)
    {
        f = cf (f, sr);
        const double A = std::pow (10.0, gdB / 40.0), w = 2 * kPi * f / sr, al = std::sin (w) / (2 * q), c = std::cos (w);
        set (1 + al * A, -2 * c, 1 - al * A, 1 + al / A, -2 * c, 1 - al / A);
    }
    void setLowShelf (double sr, double f, double q, double gdB)
    {
        f = cf (f, sr);
        const double A = std::pow (10.0, gdB / 40.0), w = 2 * kPi * f / sr, c = std::cos (w), s = 2 * std::sqrt (A) * (std::sin (w) / (2 * q));
        set (A * ((A + 1) - (A - 1) * c + s), 2 * A * ((A - 1) - (A + 1) * c), A * ((A + 1) - (A - 1) * c - s),
             (A + 1) + (A - 1) * c + s, -2 * ((A - 1) + (A + 1) * c), (A + 1) + (A - 1) * c - s);
    }
    void setHighShelf (double sr, double f, double q, double gdB)
    {
        f = cf (f, sr);
        const double A = std::pow (10.0, gdB / 40.0), w = 2 * kPi * f / sr, c = std::cos (w), s = 2 * std::sqrt (A) * (std::sin (w) / (2 * q));
        set (A * ((A + 1) + (A - 1) * c + s), -2 * A * ((A - 1) + (A + 1) * c), A * ((A + 1) + (A - 1) * c - s),
             (A + 1) - (A - 1) * c + s, 2 * ((A - 1) - (A + 1) * c), (A + 1) - (A - 1) * c - s);
    }
    void setNotch (double sr, double f, double q)
    {
        f = cf (f, sr);
        const double w = 2 * kPi * f / sr, al = std::sin (w) / (2 * q), c = std::cos (w);
        set (1, -2 * c, 1, 1 + al, -2 * c, 1 - al);
    }
    void setHP (double sr, double f, double q)
    {
        f = cf (f, sr);
        const double w = 2 * kPi * f / sr, al = std::sin (w) / (2 * q), c = std::cos (w);
        set ((1 + c) / 2, -(1 + c), (1 + c) / 2, 1 + al, -2 * c, 1 - al);
    }
    void setLP (double sr, double f, double q)
    {
        f = cf (f, sr);
        const double w = 2 * kPi * f / sr, al = std::sin (w) / (2 * q), c = std::cos (w);
        set ((1 - c) / 2, 1 - c, (1 - c) / 2, 1 + al, -2 * c, 1 - al);
    }
    void setBP (double sr, double f, double q)
    {
        f = cf (f, sr);
        const double w = 2 * kPi * f / sr, al = std::sin (w) / (2 * q), c = std::cos (w);
        set (al, 0, -al, 1 + al, -2 * c, 1 - al);
    }
    void setAP (double sr, double f, double q)
    {
        f = cf (f, sr);
        const double w = 2 * kPi * f / sr, al = std::sin (w) / (2 * q), c = std::cos (w);
        set (1 - al, -2 * c, 1 + al, 1 + al, -2 * c, 1 - al);
    }
    // |H(e^jw)| tại tần số f (dùng cho đường cong EQ trên UI)
    double magnitudeAt (double f, double sr) const
    {
        const double w = 2 * kPi * f / sr, c1 = std::cos (w), s1 = std::sin (w), c2 = std::cos (2 * w), s2 = std::sin (2 * w);
        const double nr = b0 + b1 * c1 + b2 * c2, ni = -(b1 * s1 + b2 * s2);
        const double dr = 1 + a1 * c1 + a2 * c2, di = -(a1 * s1 + a2 * s2);
        return std::sqrt ((nr * nr + ni * ni) / (dr * dr + di * di + 1e-30));
    }
};

// Butterworth bậc 4 (2 biquad cascade) cho HPF/LPF 24 dB/oct
struct Filter4
{
    Biquad s[2];
    void setHP (double sr, double f) { s[0].setHP (sr, f, 0.5412); s[1].setHP (sr, f, 1.3066); }
    void setLP (double sr, double f) { s[0].setLP (sr, f, 0.5412); s[1].setLP (sr, f, 1.3066); }
    inline float process (float x) { return s[1].process (s[0].process (x)); }
    void reset() { s[0].reset(); s[1].reset(); }
    double magnitudeAt (double f, double sr) const { return s[0].magnitudeAt (f, sr) * s[1].magnitudeAt (f, sr); }
};

struct Smooth
{
    double v = 1, t = 1, c = 0.001;
    void init (float val, double ms, double sr) { v = t = val; c = 1.0 - (double) coefFor (ms, sr); }
    // double precision: không bị "kẹt" gần đích như float khi hệ số làm mượt nhỏ (192 kHz)
    inline float next() { const double d = t - v; if (std::abs (d) < 1.0e-9) v = t; else v += d * c; return (float) v; }
};

// ---------------------------------------------------------------- Gain computer + compressor (feed-forward, linked)
inline float compGainDb (float levelDb, float thr, float ratio, float knee)
{
    const float over = levelDb - thr, slope = 1.0f / ratio - 1.0f;
    if (knee > 0 && 2 * over < -knee) return 0;
    if (knee > 0 && std::abs (2 * over) <= knee) { const float x = over + knee * 0.5f; return slope * x * x / (2 * knee); }
    return over > 0 ? slope * over : 0.0f;
}

struct Comp
{
    float grDb = 0, att = 0.9f, rel = 0.99f;
    void setTimes (float aMs, float rMs, double sr) { att = coefFor (aMs, sr); rel = coefFor (rMs, sr); }
    void reset() { grDb = 0; }
    // trả về gain tuyến tính (<=1), chưa gồm makeup
    inline float process (float peak, float thr, float ratio, float knee)
    {
        const float target = compGainDb (linToDb (peak), thr, ratio, knee);
        const float c = target < grDb ? att : rel;
        grDb = target + (grDb - target) * c;
        return dbToLin (grDb);
    }
};

struct DelayLine
{
    std::vector<float> buf; int n = 0, pos = 0;
    void prepare (int delay) { n = delay; buf.assign ((size_t) std::max (delay, 1) + 1, 0.0f); pos = 0; }
    void reset() { std::fill (buf.begin(), buf.end(), 0.0f); pos = 0; }
    inline float process (float x)
    {
        if (n <= 0) return x;
        buf[(size_t) pos] = x;
        int rp = pos - n; if (rp < 0) rp += (int) buf.size();
        pos = (pos + 1) % (int) buf.size();
        return buf[(size_t) rp];
    }
};

// ---------------------------------------------------------------- Settings / Meters
struct BandSettings { bool on = true; float freq = 1000, gain = 0, q = 0.9f; int type = 0; };
struct ToneSettings { bool on = false; float amt = 0.4f; };

struct Settings
{
    float inGainDb = 0, outGainDb = 0; bool bypass = false, levelMatch = false;
    float leftDb = 0, rightDb = 0, balance = 0, width = 1, midDb = 0, sideDb = 0, separation = 0;
    bool monoCheck = false, invL = false, invR = false;
    bool eqOn = true, hpfOn = true, lpfOn = false; float hpfFreq = 80, lpfFreq = 20000;
    BandSettings band[7];
    ToneSettings clarity, presence, warmth, air;
    bool deessOn = false; float deessFreq = 6500, deessThr = -28, deessRatio = 4;
    bool nrOn = false; float nrThr = -55, nrRange = 10, nrRelease = 180;
    bool compOn = false; float compThr = -18, compRatio = 2.5f, compAtt = 15, compRel = 160, compKnee = 8, compMakeup = 0; bool compAuto = false;
    bool mbOn = false; float mbX1 = 250, mbX2 = 3500, mbThr = -20, mbRatio = 2, mbLow = 0, mbMid = 0, mbHigh = 0;
    bool dynOn = false; float dynFreq = 3000, dynQ = 2, dynThr = -25, dynRange = -4;
    bool excOn = false; float excFreq = 4000, excAmt = 0.25f;
    bool trOn = false; float trAttack = 0, trSustain = 0;
    bool limOn = true; float limCeiling = -0.3f, limRelease = 80; bool ppOn = true;
};

struct Meters
{
    float inPeakL = 0, inPeakR = 0, outPeakL = 0, outPeakR = 0; // peak lớn nhất trong block (tuyến tính)
    float loudness = -100, correlation = 1, monoLossDb = 0, grComp = 0, grLimiter = 0, grDeEss = 0;
};

// ---------------------------------------------------------------- VocalChain
class VocalChain
{
public:
    static constexpr double lookaheadSeconds = 0.0015;

    void prepare (double sampleRate)
    {
        sr = sampleRate;
        look = (int) std::lround (lookaheadSeconds * sr);
        for (auto& d : dry) d.prepare (look);
        limBuf[0].prepare (look); limBuf[1].prepare (look);
        gainRing.assign ((size_t) look + 1, 1.0f); ringPos = 0;
        auto ini = [&] (Smooth& s, float v, double ms) { s.init (v, ms, sr); };
        ini (inG, 1, 20); ini (outG, 1, 20); ini (lG, 1, 20); ini (rG, 1, 20); ini (midG, 1, 20); ini (sideG, 1, 20);
        ini (widthG, 1, 20); ini (balL, 1, 20); ini (balR, 1, 20); ini (sepG, 0, 20); ini (byp, 0, 10);
        trAtt = 1 - coefFor (2, sr);
        const float p3 = coefFor (3000, sr); (void) p3;
        kA = coefFor (3000, sr); lmPow = coefFor (400, sr); lmGain = 1.0f - coefFor (300, sr);
        corrC = coefFor (300, sr);
        for (auto& ch : kw) { ch[0].setHP (sr, 38.0, 0.5); ch[1].setHighShelf (sr, 1500.0, 0.707, 4.0); }
        mbComp[0].setTimes (12, 150, sr); mbComp[1].setTimes (8, 110, sr); mbComp[2].setTimes (3, 70, sr);
        dyn.setGainSmoothing (10, sr);
        reset();
        forceUpdate = true;
    }

    int getLatency() const { return look; }

    void reset()
    {
        for (auto& d : dry) d.reset();
        limBuf[0].reset(); limBuf[1].reset();
        std::fill (gainRing.begin(), gainRing.end(), 1.0f);
        limGain = 1; lmG = 1; powIn = powOut = 0;
        for (int c = 0; c < 2; ++c)
        {
            hpf[c].reset(); lpf[c].reset();
            for (auto& b : bq[c]) b.reset();
            for (auto& t : tone) for (auto& b : t[c]) b.reset();
            dsBP[c].reset(); dsEnvState = 0; excHP[c].reset(); excHP2[c].reset();
            for (auto& b : kw[c]) b.reset();
            xo1LP[c][0].reset(); xo1LP[c][1].reset(); xo1HP[c][0].reset(); xo1HP[c][1].reset();
            xo2LP[c][0].reset(); xo2LP[c][1].reset(); xo2HP[c][0].reset(); xo2HP[c][1].reset(); xo2AP[c].reset();
            dynBP[c].reset(); dynPk[c].reset();
        }
        for (auto& c : mbComp) c.reset();
        nrEnv = nrGain = 1; dsGr = 0; trFast = trSlow = 0; trGain = 1; dynEnv = 0; dynGainDb = 0;
        kMsL = kMsR = 0; sumLL = sumRR = sumLR = 0;
        compUnit.reset(); forceUpdate = true;
    }

    void process (float* L, float* R, int n, const Settings& s)
    {
        updateCoeffs (s);
        float inPkL = 0, inPkR = 0, outPkL = 0, outPkR = 0, minLim = 1, minComp = 0, minDs = 0;
        double eL = 0, eR = 0, eM = 0;
        const float ceil = dbToLin (s.limCeiling), relC = coefFor (s.limRelease, sr), attC = coefFor (lookaheadSeconds * 1000.0 / 5.0, sr);
        const float nrRel = coefFor (s.nrRelease, sr);
        const float comMk = dbToLin (s.compMakeup + (s.compAuto ? std::clamp (-s.compThr * (1.0f - 1.0f / s.compRatio) * 0.4f, 0.0f, 12.0f) : 0.0f));
        const float mbG[3] = { dbToLin (s.mbLow), dbToLin (s.mbMid), dbToLin (s.mbHigh) };
        const float tcol = std::clamp ((s.excAmt), 0.0f, 1.0f);

        for (int i = 0; i < n; ++i)
        {
            const float dryL = L[i], dryR = R[i];
            inPkL = std::max (inPkL, std::abs (dryL)); inPkR = std::max (inPkR, std::abs (dryR));
            const float dl = dry[0].process (dryL), dr = dry[1].process (dryR);

            const float g = inG.next();
            float l = dryL * g, r = dryR * g;
            const float refPow = 0.5f * (l * l + r * r);
            powIn = refPow + (powIn - refPow) * lmPow;

            // ----- Stereo L/R -----
            l *= lG.next(); r *= rG.next();
            if (s.invL) l = -l;
            if (s.invR) r = -r;
            const float sep = sepG.next();
            if (sep > 1e-4f) { const float a = 0.5f * sep, k = 1.0f / (1.0f - a); const float nl = (l - a * r) * k, nr = (r - a * l) * k; l = nl; r = nr; }
            { const float m = (l + r) * 0.5f * midG.next(), sd = (l - r) * 0.5f * sideG.next() * widthG.next(); l = m + sd; r = m - sd; }
            l *= balL.next(); r *= balR.next();

            // ----- HPF / LPF -----
            if (s.hpfOn) { l = hpf[0].process (l); r = hpf[1].process (r); }
            if (s.lpfOn) { l = lpf[0].process (l); r = lpf[1].process (r); }

            // ----- Noise reduction (downward expander, linked) -----
            if (s.nrOn)
            {
                const float pk = std::max (std::abs (l), std::abs (r));
                nrEnv = pk > nrEnv ? pk + (nrEnv - pk) * coefFor (1.0, sr) : pk + (nrEnv - pk) * coefFor (40.0, sr);
                const float lv = linToDb (nrEnv);
                const float redDb = lv < s.nrThr ? std::min (s.nrRange, (s.nrThr - lv) * 1.0f) : 0.0f;
                const float tgt = dbToLin (-redDb);
                nrGain = tgt > nrGain ? tgt + (nrGain - tgt) * coefFor (4.0, sr) : tgt + (nrGain - tgt) * nrRel;
                l *= nrGain; r *= nrGain;
            }

            // ----- EQ bands -----
            if (s.eqOn)
                for (int b = 0; b < 7; ++b)
                    if (bandActive[b]) { l = bq[0][b].process (l); r = bq[1][b].process (r); }

            // ----- Vocal tone modules -----
            for (int t = 0; t < 4; ++t)
                if (toneN[t] > 0)
                    for (int k = 0; k < toneN[t]; ++k) { l = tone[t][0][k].process (l); r = tone[t][1][k].process (r); }

            // ----- De-esser (chỉ giảm dải sibilance) -----
            if (s.deessOn)
            {
                const float bl = dsBP[0].process (l), br = dsBP[1].process (r);
                const float pk = std::max (std::abs (bl), std::abs (br));
                dsEnvState = pk > dsEnvState ? pk + (dsEnvState - pk) * coefFor (0.5, sr) : pk + (dsEnvState - pk) * coefFor (40.0, sr);
                const float tgt = compGainDb (linToDb (dsEnvState), s.deessThr, s.deessRatio, 0.0f);
                dsGr = tgt < dsGr ? tgt + (dsGr - tgt) * coefFor (0.8, sr) : tgt + (dsGr - tgt) * coefFor (35.0, sr);
                const float gg = dbToLin (dsGr);
                l = l - bl * (1.0f - gg); r = r - br * (1.0f - gg);
                minDs = std::min (minDs, dsGr);
            }

            // ----- Dynamic EQ -----
            if (s.dynOn)
            {
                const float bl = dynBP[0].process (l), br = dynBP[1].process (r);
                const float pk = std::max (std::abs (bl), std::abs (br));
                dynEnv = pk > dynEnv ? pk + (dynEnv - pk) * coefFor (3.0, sr) : pk + (dynEnv - pk) * coefFor (60.0, sr);
                const float amount = std::clamp ((linToDb (dynEnv) - s.dynThr) / 10.0f, 0.0f, 1.0f);
                const float tgt = s.dynRange * amount;
                dynGainDb = tgt + (dynGainDb - tgt) * dyn.c;
                if (++dynCount >= 16) { dynCount = 0; if (std::abs (dynGainDb - dynApplied) > 0.01f) { dynApplied = dynGainDb; for (int c = 0; c < 2; ++c) dynPk[c].setPeak (sr, s.dynFreq, s.dynQ, dynApplied); } }
                l = dynPk[0].process (l); r = dynPk[1].process (r);
            }

            // ----- Transient control -----
            if (s.trOn)
            {
                const float pk = std::max (std::abs (l), std::abs (r));
                trFast = pk > trFast ? pk + (trFast - pk) * coefFor (0.5, sr) : pk + (trFast - pk) * coefFor (25.0, sr);
                trSlow = pk > trSlow ? pk + (trSlow - pk) * coefFor (25.0, sr) : pk + (trSlow - pk) * coefFor (120.0, sr);
                const float ratio = trFast / (trSlow + 1e-6f);
                const float a = std::clamp (ratio - 1.0f, 0.0f, 1.0f), su = std::clamp (1.0f - ratio, 0.0f, 1.0f);
                const float tgt = dbToLin (s.trAttack * a + s.trSustain * su);
                trGain += (tgt - trGain) * trAtt;
                l *= trGain; r *= trGain;
            }

            // ----- Gentle compressor -----
            if (s.compOn)
            {
                const float gc = compUnit.process (std::max (std::abs (l), std::abs (r)), s.compThr, s.compRatio, s.compKnee);
                minComp = std::min (minComp, compUnit.grDb);
                l *= gc * comMk; r *= gc * comMk;
            }

            // ----- Multiband compressor (LR4, 3 băng) -----
            if (s.mbOn)
            {
                float lo[2], mi[2], hi[2], in[2] = { l, r };
                for (int c = 0; c < 2; ++c)
                {
                    lo[c] = xo1LP[c][1].process (xo1LP[c][0].process (in[c]));
                    const float rest = xo1HP[c][1].process (xo1HP[c][0].process (in[c]));
                    mi[c] = xo2LP[c][1].process (xo2LP[c][0].process (rest));
                    hi[c] = xo2HP[c][1].process (xo2HP[c][0].process (rest));
                    lo[c] = xo2AP[c].process (lo[c]); // bù pha để 3 băng cộng lại phẳng
                }
                float* bands[3] = { lo, mi, hi };
                for (int k = 0; k < 3; ++k)
                {
                    const float gk = mbComp[k].process (std::max (std::abs (bands[k][0]), std::abs (bands[k][1])), s.mbThr, s.mbRatio, 6.0f) * mbG[k];
                    bands[k][0] *= gk; bands[k][1] *= gk;
                }
                l = lo[0] + mi[0] + hi[0]; r = lo[1] + mi[1] + hi[1];
            }

            // ----- Harmonic exciter (chỉ thêm hài bậc cao) -----
            if (s.excOn)
            {
                const float hl = excHP[0].process (l), hr = excHP[1].process (r);
                const float sl = excHP2[0].process (std::tanh (3.0f * hl)), sr_ = excHP2[1].process (std::tanh (3.0f * hr));
                l += tcol * 0.5f * sl; r += tcol * 0.5f * sr_;
            }

            // ----- Level match (chỉ chỉnh về mức vào, giới hạn ±12 dB, rất chậm để tránh pumping) -----
            {
                const float pw = 0.5f * (l * l + r * r);
                powOut = pw + (powOut - pw) * lmPow;
                float tgt = 1.0f;
                if (s.levelMatch && powIn > 1e-8f && powOut > 1e-9f) tgt = std::clamp (std::sqrt (powIn / powOut), 0.25f, 4.0f);
                else if (s.levelMatch) tgt = lmG; // giữ nguyên khi im lặng
                lmG += (tgt - lmG) * lmGain;
                l *= lmG; r *= lmG;
            }

            const float og = outG.next();
            l *= og; r *= og;

            // ----- Limiter có lookahead -----
            {
                const float dL = limBuf[0].process (l), dR = limBuf[1].process (r);
                const float pk = std::max (std::abs (l), std::abs (r));
                const float req = (s.limOn && pk > ceil) ? ceil / pk : 1.0f;
                gainRing[(size_t) ringPos] = req;
                ringPos = (ringPos + 1) % (int) gainRing.size();
                float tgt = 1.0f;
                if (s.limOn) for (float v : gainRing) tgt = std::min (tgt, v);
                limGain = tgt < limGain ? tgt + (limGain - tgt) * attC : tgt + (limGain - tgt) * relC;
                minLim = std::min (minLim, limGain);
                l = dL * limGain; r = dR * limGain;
                if (s.ppOn) { l = softClip (l, ceil); r = softClip (r, ceil); }
            }

            // ----- Bypass crossfade (dry đã được delay để khớp độ trễ) -----
            const float bm = byp.next();
            if (bm > 0.0f) { l = l * (1 - bm) + dl * bm; r = r * (1 - bm) + dr * bm; }
            if (s.monoCheck) { const float m = 0.5f * (l + r); l = r = m; }

            L[i] = l; R[i] = r;

            // ----- Meters -----
            outPkL = std::max (outPkL, std::abs (l)); outPkR = std::max (outPkR, std::abs (r));
            const float kl = kw[0][1].process (kw[0][0].process (l)), kr = kw[1][1].process (kw[1][0].process (r));
            kMsL = kl * kl + (kMsL - kl * kl) * kA; kMsR = kr * kr + (kMsR - kr * kr) * kA;
            sumLL = l * l + (sumLL - l * l) * corrC; sumRR = r * r + (sumRR - r * r) * corrC; sumLR = l * r + (sumLR - l * r) * corrC;
            eL += l * l; eR += r * r; eM += 0.25 * (l + r) * (l + r);
        }

        meters.inPeakL = inPkL; meters.inPeakR = inPkR; meters.outPeakL = outPkL; meters.outPeakR = outPkR;
        meters.loudness = -0.691f + 10.0f * std::log10 (std::max (kMsL + kMsR, 1.0e-10f));
        meters.correlation = (sumLL > 1e-10f && sumRR > 1e-10f) ? sumLR / std::sqrt (sumLL * sumRR) : 1.0f;
        meters.monoLossDb = (eL + eR) > 1e-12 ? (float) (10.0 * std::log10 (std::max (2.0 * eM / (eL + eR), 1e-6))) : 0.0f;
        meters.grComp = minComp; meters.grLimiter = linToDb (minLim); meters.grDeEss = minDs;
    }

    Meters meters;

    // Dùng cho UI: biquad tương ứng 1 band EQ
    static void configureBand (Biquad& b, double sr, const BandSettings& bs)
    {
        switch (bs.type)
        {
            case 1: b.setLowShelf (sr, bs.freq, 0.707, bs.gain); break;
            case 2: b.setHighShelf (sr, bs.freq, 0.707, bs.gain); break;
            case 3: b.setNotch (sr, bs.freq, bs.q); break;
            default: b.setPeak (sr, bs.freq, bs.q, bs.gain); break;
        }
    }

private:
    static float softClip (float x, float c)
    {
        const float t = std::abs (x) / c;
        if (t <= 0.9f) return x;
        const float o = c * (0.9f + 0.1f * std::tanh ((t - 0.9f) / 0.1f));
        return x < 0 ? -o : o;
    }

    struct DynSmooth { float c = 0.99f; void setGainSmoothing (double ms, double sr) { c = coefFor (ms, sr); } };

    void updateCoeffs (const Settings& s)
    {
        auto ini = [] (Smooth& sm, float v) { sm.t = v; };
        ini (inG, dbToLin (s.inGainDb)); ini (outG, dbToLin (s.outGainDb));
        ini (lG, dbToLin (s.leftDb)); ini (rG, dbToLin (s.rightDb));
        ini (midG, dbToLin (s.midDb)); ini (sideG, dbToLin (s.sideDb)); ini (widthG, s.width);
        ini (balL, s.balance > 0 ? 1.0f - s.balance : 1.0f); ini (balR, s.balance < 0 ? 1.0f + s.balance : 1.0f);
        ini (sepG, s.separation); ini (byp, s.bypass ? 1.0f : 0.0f);

        if (forceUpdate || s.hpfFreq != pHpf) { for (auto& f : hpf) f.setHP (sr, s.hpfFreq); pHpf = s.hpfFreq; }
        if (forceUpdate || s.lpfFreq != pLpf) { for (auto& f : lpf) f.setLP (sr, s.lpfFreq); pLpf = s.lpfFreq; }

        for (int b = 0; b < 7; ++b)
        {
            const auto& bs = s.band[b];
            const bool changed = forceUpdate || bs.freq != pBand[b].freq || bs.gain != pBand[b].gain || bs.q != pBand[b].q || bs.type != pBand[b].type;
            if (changed) { for (int c = 0; c < 2; ++c) configureBand (bq[c][b], sr, bs); pBand[b] = bs; }
            bandActive[b] = bs.on && (bs.type == 3 || std::abs (bs.gain) > 0.01f);
        }

        const ToneSettings* ts[4] = { &s.clarity, &s.presence, &s.warmth, &s.air };
        for (int t = 0; t < 4; ++t)
        {
            const float a = ts[t]->on ? ts[t]->amt : 0.0f;
            if (forceUpdate || a != pTone[t])
            {
                pTone[t] = a; toneN[t] = a > 0.001f ? (t == 0 || t == 2 ? 2 : 1) : 0;
                for (int c = 0; c < 2; ++c)
                {
                    if (t == 0) { tone[t][c][0].setPeak (sr, 3200, 0.8, 4.0 * a); tone[t][c][1].setPeak (sr, 300, 1.0, -2.5 * a); }
                    if (t == 1) tone[t][c][0].setPeak (sr, 4800, 0.9, 5.0 * a);
                    if (t == 2) { tone[t][c][0].setLowShelf (sr, 180, 0.707, 4.0 * a); tone[t][c][1].setHighShelf (sr, 9000, 0.707, -1.5 * a); }
                    if (t == 3) tone[t][c][0].setHighShelf (sr, 11000, 0.707, 6.0 * a);
                }
            }
        }

        if (forceUpdate || s.deessFreq != pDs) { for (auto& f : dsBP) f.setBP (sr, s.deessFreq, 1.6); pDs = s.deessFreq; }
        if (forceUpdate || s.excFreq != pExc) { for (auto& f : excHP) f.setHP (sr, s.excFreq, 0.707); for (auto& f : excHP2) f.setHP (sr, s.excFreq, 0.707); pExc = s.excFreq; }
        if (forceUpdate || s.dynFreq != pDynF || s.dynQ != pDynQ)
        {
            for (auto& f : dynBP) f.setBP (sr, s.dynFreq, s.dynQ);
            for (auto& f : dynPk) f.setPeak (sr, s.dynFreq, s.dynQ, dynApplied);
            pDynF = s.dynFreq; pDynQ = s.dynQ;
        }
        if (forceUpdate || s.mbX1 != pX1)
        {
            for (int c = 0; c < 2; ++c) { for (int k = 0; k < 2; ++k) { xo1LP[c][k].setLP (sr, s.mbX1, 0.7071); xo1HP[c][k].setHP (sr, s.mbX1, 0.7071); } }
            pX1 = s.mbX1;
        }
        if (forceUpdate || s.mbX2 != pX2)
        {
            for (int c = 0; c < 2; ++c) { for (int k = 0; k < 2; ++k) { xo2LP[c][k].setLP (sr, s.mbX2, 0.7071); xo2HP[c][k].setHP (sr, s.mbX2, 0.7071); } xo2AP[c].setAP (sr, s.mbX2, 0.7071); }
            pX2 = s.mbX2;
        }
        if (forceUpdate || s.compAtt != pCA || s.compRel != pCR) { compUnit.setTimes (s.compAtt, s.compRel, sr); pCA = s.compAtt; pCR = s.compRel; }
        forceUpdate = false;
    }

    double sr = 48000; int look = 72; bool forceUpdate = true;
    DelayLine dry[2], limBuf[2];
    std::vector<float> gainRing; int ringPos = 0; float limGain = 1;
    Smooth inG, outG, lG, rG, midG, sideG, widthG, balL, balR, sepG, byp;
    Filter4 hpf[2], lpf[2];
    Biquad bq[2][7]; bool bandActive[7] {}; BandSettings pBand[7];
    Biquad tone[4][2][2]; int toneN[4] {}; float pTone[4] { -1, -1, -1, -1 };
    Biquad dsBP[2], dynBP[2], dynPk[2], excHP[2], excHP2[2], xo2AP[2], kw[2][2];
    Biquad xo1LP[2][2], xo1HP[2][2], xo2LP[2][2], xo2HP[2][2];
    Comp compUnit, mbComp[3]; DynSmooth dyn;
    float nrEnv = 1, nrGain = 1, dsEnvState = 0, dsGr = 0, trFast = 0, trSlow = 0, trGain = 1, trAtt = 0.01f;
    float dynEnv = 0, dynGainDb = 0, dynApplied = 0; int dynCount = 0;
    float lmG = 1, powIn = 0, powOut = 0, kA = 0.99f, lmPow = 0.99f, lmGain = 0.001f, corrC = 0.99f;
    float kMsL = 0, kMsR = 0, sumLL = 0, sumRR = 0, sumLR = 0;
    float pHpf = -1, pLpf = -1, pDs = -1, pExc = -1, pDynF = -1, pDynQ = -1, pX1 = -1, pX2 = -1, pCA = -1, pCR = -1;
};
} // namespace TKdsp
