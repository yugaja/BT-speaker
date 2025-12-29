#include "AudioDSP.h"
#include "core/config.h"

AudioDSP::AudioDSP(i2s_port_t port)
: m_i2s(port)
{
    updateGain();
    vuRef = 0; // start value, not zero
}

/* ================= STREAM ================= */

/* ================= DSP ================= */

int32_t* AudioDSP::bassEnhancer(int16_t in[2], float intensity, bool clear)
{
    static int32_t out[2];
    static float mem[2] = {};
    static float prev[2] = {};

    const float fc = 100.0f;
    const float fs = getSampleRate();
    const float alpha = 1.0f / (1.0f + (1.0f / (2.0f * 3.14159265f * fc)) * fs);

    if(clear) memset(mem, 0, sizeof(mem));

    for(int ch = 0; ch < 2; ch++) {
        float x = in[ch] / 32768.0f;
        mem[ch] = prev[ch] + alpha * (x - prev[ch]);
        prev[ch] = mem[ch];
        mem[ch] = tanhf(mem[ch] * intensity);
        out[ch] = (int32_t)((x + mem[ch]) * 32768.0f);
    }
    return out;
}

int32_t* AudioDSP::iirProcess(uint8_t idx, const int32_t in[2], bool clear)
{
    static int32_t out[2];
    if(clear) {
        memset(m_filterState[idx], 0, sizeof(m_filterState[idx]));
        return out;
    }

    for(uint8_t ch = 0; ch < 2; ch++) {
        float x = in[ch] / 32768.0f;
        float y =
            m_filter[idx].a0 * x +
            m_filter[idx].a1 * m_filterState[idx][0][0][ch] +
            m_filter[idx].a2 * m_filterState[idx][1][0][ch] -
            m_filter[idx].b1 * m_filterState[idx][0][1][ch] -
            m_filter[idx].b2 * m_filterState[idx][1][1][ch];

        m_filterState[idx][1][0][ch] = m_filterState[idx][0][0][ch];
        m_filterState[idx][0][0][ch] = x;
        m_filterState[idx][1][1][ch] = m_filterState[idx][0][1][ch];
        m_filterState[idx][0][1][ch] = y;

        out[ch] = (int32_t)(y * 32768.0f);
    }
    return out;
}

int32_t AudioDSP::applyGain(int32_t in[2])
{
    int32_t L = (in[LEFT]  * m_gainL) >> 8;
    int32_t R = (in[RIGHT] * m_gainR) >> 8;

    if(L > 32767) L = 32767; else if(L < -32768) L = -32768;
    if(R > 32767) R = 32767; else if(R < -32768) R = -32768;

    return (L << 16) | (R & 0xFFFF);
}

/* ================= CONTROL ================= */

void AudioDSP::setVolume(uint8_t v)
{
    if(v > 254) v = 254;
    m_vol = v;
    updateGain();
}

void AudioDSP::setBalance(int8_t b)
{
    if(b < -16) b = -16;
    if(b > 16)  b = 16;
    m_balance = b;
    updateGain();
}

void AudioDSP::updateGain()
{
    int32_t base = ((int32_t)m_vol * 256) / 254;
    m_gainL = base;
    m_gainR = base;

    if(m_balance < 0)
        m_gainR = base * (16 + m_balance) / 16;
    else if(m_balance > 0)
        m_gainL = base * (16 - m_balance) / 16;
}

/* ================= VU ================= */

void AudioDSP::computeVU(int16_t sample[2])
{  
    if(!config.store.vumeter) return;
    auto avg = [&](uint8_t* arr) -> uint16_t {
        uint16_t s = 0;
        for(int i = 0; i < 8; i++) s += arr[i];
        return s >> 3;
    };

    auto largest = [&](uint8_t* arr) -> uint16_t {
        uint16_t m = 0;
        for(int i = 0; i < 8; i++)
            if(arr[i] > m) m = arr[i];
        return m;
    };

    if(cnt0 == 64) { cnt0 = 0; cnt1++; }
    if(cnt1 == 8)  { cnt1 = 0; cnt2++; }
    if(cnt2 == 8)  { cnt2 = 0; cnt3++; }
    if(cnt3 == 8)  { cnt3 = 0; cnt4++; f_vu = true; }
    if(cnt4 == 8)  { cnt4 = 0; }

    if(!cnt0) {
        m_sampleArray[LEFT][0][cnt1]  = abs(sample[LEFT])  >> 7;
        m_sampleArray[RIGHT][0][cnt1] = abs(sample[RIGHT]) >> 7;
    }

    if(!cnt1) {
        m_sampleArray[LEFT][1][cnt2]  = largest(m_sampleArray[LEFT][0]);
        m_sampleArray[RIGHT][1][cnt2] = largest(m_sampleArray[RIGHT][0]);
    }

    if(!cnt2) {
        m_sampleArray[LEFT][2][cnt3]  = largest(m_sampleArray[LEFT][1]);
        m_sampleArray[RIGHT][2][cnt3] = largest(m_sampleArray[RIGHT][1]);
    }

    if(!cnt3) {
        m_sampleArray[LEFT][3][cnt4]  = avg(m_sampleArray[LEFT][2]);
        m_sampleArray[RIGHT][3][cnt4] = avg(m_sampleArray[RIGHT][2]);
    }

    if(f_vu) {
        f_vu = false;

        m_vuLeft  = avg(m_sampleArray[LEFT][3]);
        m_vuRight = avg(m_sampleArray[RIGHT][3]);


        if(m_vuLeft > config.vuThreshold)  config.vuThreshold = m_vuLeft;
        if(m_vuRight > config.vuThreshold) config.vuThreshold = m_vuRight;
    }

    cnt1++;
}

// uint16_t AudioDSP::getVU(uint16_t dimension){
// //   if(!config.store.vumeter || config.vuThreshold==0) return 0;
//   uint8_t L = map(m_vuLeft, 255,/*config.vuThreshold,*/ 0, 0, dimension);
//   uint8_t R = map(m_vuRight, 255,/*config.vuThreshold*/ 0, 0, dimension);
//   return (L << 8) | R;
// }
void AudioDSP::updateVURef()
{
    uint16_t v = m_vuLeft > m_vuRight ? m_vuLeft : m_vuRight;

    if(v > vuRef) {
        vuRef += (v - vuRef) / ATTACK;   // fast attack
    } else {
        vuRef -= (vuRef - v) / RELEASE;  // slow release
    }

    if(vuRef < VU_FLOOR) vuRef = VU_FLOOR;
    if(vuRef > VU_MAX)   vuRef = VU_MAX;
}


uint16_t AudioDSP::getVU(uint16_t dimension)
{
    updateVURef();

    if(!config.store.vumeter) return 0;

    uint8_t L = map(m_vuLeft,  vuRef, 0, 0, dimension);
    uint8_t R = map(m_vuRight, vuRef, 0, 0, dimension);

    return (L << 8) | R;
}

void AudioDSP::setTone(int8_t gainLowPass, int8_t gainBandPass, int8_t gainHighPass){
    // see https://www.earlevel.com/main/2013/10/13/biquad-calculator-v2/
    // values can be between -40 ... +6 (dB)

    IIR_calculateCoefficients(gainLowPass, gainBandPass, gainHighPass);
}
#include <math.h>

/**
 * @brief Internal helper for computing a single filter's biquad coefficients.
 */
void AudioDSP::IIR_computeOne(int idx, const FilterDef& f)
{
    float Fc = f.freq / (float)getSampleRate();
    float K  = tanf((float)PI * Fc);
    float V  = powf(10.0f, fabsf(f.gain) / 20.0f);
    float Q  = (f.Q <= 0.0f ? 1.0f : f.Q);
    float norm = 1.0f;

    switch (f.type) {
        case FILTER_LOW_SHELF:
            if (f.gain >= 0) {
                norm = 1.0f / (1.0f + sqrtf(2.0f) * K + K * K);
                m_filter[idx].a0 = (1 + sqrtf(2 * V) * K + V * K * K) * norm;
                m_filter[idx].a1 = 2 * (V * K * K - 1) * norm;
                m_filter[idx].a2 = (1 - sqrtf(2 * V) * K + V * K * K) * norm;
                m_filter[idx].b1 = 2 * (K * K - 1) * norm;
                m_filter[idx].b2 = (1 - sqrtf(2.0f) * K + K * K) * norm;
            } else {
                norm = 1.0f / (1.0f + sqrtf(2 * V) * K + V * K * K);
                m_filter[idx].a0 = (1 + sqrtf(2.0f) * K + K * K) * norm;
                m_filter[idx].a1 = 2 * (K * K - 1) * norm;
                m_filter[idx].a2 = (1 - sqrtf(2.0f) * K + K * K) * norm;
                m_filter[idx].b1 = 2 * (V * K * K - 1) * norm;
                m_filter[idx].b2 = (1 - sqrtf(2 * V) * K + V * K * K) * norm;
            }
            break;

        case FILTER_PEAK_EQ:
            if (f.gain >= 0) {
                norm = 1.0f / (1.0f + 1.0f/Q * K + K * K);
                m_filter[idx].a0 = (1 + V/Q * K + K * K) * norm;
                m_filter[idx].a1 = 2 * (K * K - 1) * norm;
                m_filter[idx].a2 = (1 - V/Q * K + K * K) * norm;
                m_filter[idx].b1 = m_filter[idx].a1;
                m_filter[idx].b2 = (1 - 1.0f/Q * K + K * K) * norm;
            } else {
                norm = 1.0f / (1.0f + V/Q * K + K * K);
                m_filter[idx].a0 = (1 + 1.0f/Q * K + K * K) * norm;
                m_filter[idx].a1 = 2 * (K * K - 1) * norm;
                m_filter[idx].a2 = (1 - 1.0f/Q * K + K * K) * norm;
                m_filter[idx].b1 = m_filter[idx].a1;
                m_filter[idx].b2 = (1 - V/Q * K + K * K) * norm;
            }
            break;

        case FILTER_HIGH_SHELF:
            if (f.gain >= 0) {
                norm = 1.0f / (1.0f + sqrtf(2.0f) * K + K * K);
                m_filter[idx].a0 = (V + sqrtf(2 * V) * K + K * K) * norm;
                m_filter[idx].a1 = 2 * (K * K - V) * norm;
                m_filter[idx].a2 = (V - sqrtf(2 * V) * K + K * K) * norm;
                m_filter[idx].b1 = 2 * (K * K - 1) * norm;
                m_filter[idx].b2 = (1 - sqrtf(2.0f) * K + K * K) * norm;
            } else {
                norm = 1.0f / (V + sqrtf(2 * V) * K + K * K);
                m_filter[idx].a0 = (1 + sqrtf(2.0f) * K + K * K) * norm;
                m_filter[idx].a1 = 2 * (K * K - 1) * norm;
                m_filter[idx].a2 = (1 - sqrtf(2.0f) * K + K * K) * norm;
                m_filter[idx].b1 = 2 * (K * K - V) * norm;
                m_filter[idx].b2 = (V - sqrtf(2 * V) * K + K * K) * norm;
            }
            break;

        default:
            m_filter[idx].a0 = 1.0f;
            m_filter[idx].a1 = m_filter[idx].a2 = m_filter[idx].b1 = m_filter[idx].b2 = 0.0f;
            break;
    }
}

/**
 * @brief New flexible API that supports arbitrary number of filters.
 */
void AudioDSP::IIR_calculateCoefficientsExt(const FilterDef* filters, int count)
{
    // if (getSampleRate() < 1000 || !filters || count <= 0)
    //     return;

    for (int i = 0; i < count; ++i)
        IIR_computeOne(i, filters[i]);
}

/**
 * @brief Wrapper — provides legacy API using modern function underneath.
 */
void AudioDSP::IIR_calculateCoefficients(int8_t G0, int8_t G1, int8_t G2)
{
    if (G0>16){
        G0=16;
    }
    if (G1>16){
        G1=16;
    }
    if (G2>16){
        G2=16;
    }
    ///custom filter set incl small speaker box model
    FilterDef defaultChain[4] = {
        { FILTER_LOW_SHELF,  60.0f,  -10.0f, 2.5f },
        { FILTER_PEAK_EQ,   170.0f,  (float)G0, 1.0f   },
        { FILTER_PEAK_EQ,   900.0f,  (float)G1, 1.0f   },
        { FILTER_HIGH_SHELF, 4200.0f, (float)G2, 2.3f }
    };

    IIR_calculateCoefficientsExt(defaultChain, 3);
}

uint32_t AudioDSP::getSampleRate(){
    return 44100;
}
