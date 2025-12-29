#pragma once

#include "A2DPVolumeControl.h"
#include "AudioDSP.h"

/**
 * @brief A2DP DSP hook integrated into ESP32-A2DP pipeline
 *
 * Processes decoded PCM audio before I2S output.
 */
class A2DPAudioDSP : public A2DPVolumeControl {
public:
    /**
     * @brief Construct DSP adapter
     * @param dsp Pointer to AudioDSP engine
     */
    explicit A2DPAudioDSP(AudioDSP* dsp)
        : m_dsp(dsp) {}

    /**
     * @brief Process audio frames before I2S output
     *
     * @param data Pointer to stereo PCM frames
     * @param frameCount Number of frames
     */
    void update_audio_data(Frame* data, uint16_t frameCount) override {
        if (!m_dsp) return;

        for (uint16_t i = 0; i < frameCount; i++) {

            int16_t s16[2] = {
                data[i].channel1,
                data[i].channel2
            };

            m_dsp->computeVU(s16);

            int32_t* s32 = m_dsp->bassEnhancer(s16, 1.5f, false);

            for (uint8_t f = 0; f < 3; f++) {
                s32 = m_dsp->iirProcess(f, s32, false);
            }

            int32_t packed = m_dsp->applyGain(s32);

            data[i].channel1 = (int16_t)(packed >> 16);
            data[i].channel2 = (int16_t)(packed & 0xFFFF);
        }
    }

    /**
     * @brief Set volume from AVRCP (0–127)
     */
    void set_volume(uint8_t volume) override {
        m_volume = volume;
        if (m_dsp) {
            m_dsp->setVolume(volume << 1); // map 0–127 → 0–254
        }
    }



private:
    AudioDSP* m_dsp;
    uint8_t   m_volume = 127;
};
