#pragma once
#include "Arduino.h"
#include <stdint.h>
#include <string.h>
#include <math.h>
#include "driver/i2s.h"

/**
 * @brief Real-time audio DSP processor for ESP32 (A2DP-safe).
 *        Designed to be used from BluetoothA2DPSink stream reader callback.
 */
class AudioDSP {
public:
    static constexpr uint8_t LEFT  = 0;
    static constexpr uint8_t RIGHT = 1;

    AudioDSP(i2s_port_t port);

    /* ===== A2DP ENTRY POINT ===== */

    /**
     * @brief Process incoming PCM16 stereo samples and write to I2S.
     * @param data Pointer to interleaved int16 stereo samples.
     * @param len  Length in bytes.
     */
    void processStream(const uint8_t* data, uint32_t len);

    /* ===== USER CONTROLS ===== */

    void setVolume(uint8_t vol);
    void setBalance(int8_t bal);
    void setTone(int8_t low, int8_t mid, int8_t high);
    uint32_t getSampleRate();
    uint16_t getVU(uint16_t dimension);
    int32_t* bassEnhancer(int16_t in[2], float intensity, bool clear);
    int32_t* iirProcess(uint8_t idx, const int32_t in[2], bool clear);
    int32_t  applyGain(int32_t in[2]);
    void     updateGain();
    void     computeVU(int16_t in[2]);

    typedef enum {
        FILTER_LOW_SHELF,
        FILTER_PEAK_EQ,
        FILTER_HIGH_SHELF
    } FilterType;

    /**
    * @brief Definition of one filter band (for configuration input)
    */
    typedef struct {
        FilterType type;  ///< Filter type (low-shelf, peaking EQ, high-shelf)
        float freq;       ///< Center or cutoff frequency [Hz]
        float gain;       ///< Gain in dB (-40 ... +16)
        float Q;          ///< Quality factor
    } FilterDef;

private:
    /* ===== INTERNAL DSP ===== */


    void IIR_calculateCoefficientsExt(const FilterDef* filters, int count);
    void IIR_calculateCoefficients(int8_t G0, int8_t G1, int8_t G2);
    void IIR_computeOne(int idx, const FilterDef& f);

    void updateVURef();

    /* ===== CONFIG ===== */

    i2s_port_t m_i2s;

    /* ===== GAIN ===== */

    uint8_t m_vol = 255;
    int8_t  m_balance = 0;
    int16_t m_gainL = 256;
    int16_t m_gainR = 256;

    /* ===== DSP BUFFERS ===== */

    static constexpr uint8_t NUM_FILTERS = 3;

    struct Biquad {
        float a0, a1, a2;
        float b1, b2;
    };

    Biquad m_filter[NUM_FILTERS];
    float  m_filterState[NUM_FILTERS][2][2][2] = {}; // [filter][z][in/out][ch]

    /* ===== VU ===== */

    /* ===== VU METER STATE ===== */

uint8_t  m_vuLeft = 0;
uint8_t  m_vuRight = 0;
uint16_t m_vuThreshold = 1;

static constexpr uint16_t VU_MAX   = 255;  // s16 >> 7
static constexpr uint16_t VU_FLOOR = 20;   // prevents boosting noise
static constexpr uint8_t  ATTACK   = 4;    // fast follow
static constexpr uint8_t  RELEASE  = 64;   // slow decay
uint16_t vuRef;

uint8_t  m_sampleArray[2][4][8] = {{{0}}};

uint8_t cnt0 = 0;
uint8_t cnt1 = 0;
uint8_t cnt2 = 0;
uint8_t cnt3 = 0;
uint8_t cnt4 = 0;
bool    f_vu  = false;

};
