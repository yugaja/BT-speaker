#pragma once
#include "Arduino.h"
#include "esp_system.h"
#include "dual_boot.h"
#include "BluetoothA2DPSink.h"

/**
 * @brief Class to manage ESP32 Bluetooth A2DP audio sink
 */
class BluetoothAudio {
public:

    BluetoothA2DPSink& sink() { return a2dp_sink; }
    /* ===================== USER CALLBACK TYPES ===================== */

    typedef void (*TitleCallback)(const char* title);
    typedef void (*ArtistCallback)(const char* artist);
    typedef void (*AlbumCallback)(const char* album);
    typedef void (*VolumeCallback)(uint8_t volume); // 0..127 AVRCP scale

    /**
    * @brief DSP stream callback type.
    *        Called with decoded PCM16 stereo samples before I2S.
    */
    typedef void (*DSPCallback)(const uint8_t* data, uint32_t len);


    /* ===================== REGISTRATION METHODS ===================== */

    /**
     * @brief Register callback for new track title
     */
    void onTitleChanged(TitleCallback cb) { title_cb = cb; }

    /**
     * @brief Register callback for new artist
     */
    void onArtistChanged(ArtistCallback cb) { artist_cb = cb; }

    /**
     * @brief Register callback for new album
     */
    void onAlbumChanged(AlbumCallback cb) { album_cb = cb; }

    /**
     * @brief Register callback for volume change
     */
    void onVolumeChanged(VolumeCallback cb) { volume_cb = cb; }


    /**
    * @brief Register DSP processing callback.
    */
    void onAudioDSP(DSPCallback cb) { dsp_cb = cb; }


    /* ===================== SETUP ===================== */

    /**
     * @brief Initialize the Bluetooth audio sink with I2S pins and configuration
     * @param name Device name for Bluetooth
     */
    void setup(const char* name = "yugaja") {

        static i2s_config_t i2s_config = {
            .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
            .sample_rate = 48000,
            .bits_per_sample = (i2s_bits_per_sample_t)32,
            .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
            .communication_format = I2S_COMM_FORMAT_STAND_I2S,
            .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
            .dma_buf_count = 8,
            .dma_buf_len = 512,
            .use_apll = true,
            .tx_desc_auto_clear = true
        };

        static i2s_pin_config_t pin_config = {
            .mck_io_num = I2S_PIN_NO_CHANGE,
            .bck_io_num = 22,
            .ws_io_num = 4,
            .data_out_num = 17,
            .data_in_num = I2S_PIN_NO_CHANGE
        };

        a2dp_sink.set_pin_config(pin_config);
        a2dp_sink.set_i2s_config(i2s_config);

        a2dp_sink.set_avrc_metadata_attribute_mask(
            ESP_AVRC_MD_ATTR_TITLE |
            ESP_AVRC_MD_ATTR_ARTIST |
            ESP_AVRC_MD_ATTR_ALBUM |
            ESP_AVRC_MD_ATTR_PLAYING_TIME
        );

        a2dp_sink.set_avrc_metadata_callback(metadata_callback_static);
        a2dp_sink.set_avrc_rn_volumechange(volume_callback_static);
        // a2dp_sink.set_auto_reconnect(true);

        // a2dp_sink.set_stream_reader(audio_stream_callback_static, false);
        instance = this;
        a2dp_sink.start(name);
    }

    /**
     * @brief Call this in Arduino loop()
     */
    void loop() {
        if (a2dp_sink.get_audio_state() == ESP_A2D_AUDIO_STATE_STARTED) {
            //delay(1);
        }
    }

private:
    BluetoothA2DPSink a2dp_sink;

    /* ===================== CALLBACK STORAGE ===================== */

    TitleCallback  title_cb  = nullptr;
    ArtistCallback artist_cb = nullptr;
    AlbumCallback  album_cb  = nullptr;
    VolumeCallback volume_cb = nullptr;
    DSPCallback dsp_cb = nullptr;


    static BluetoothAudio* instance;

    /* ===================== AVRCP PARSER ===================== */

    static void metadata_callback_static(uint8_t id, const uint8_t* text) {
        if (!instance || !text) return;

        const char* str = (const char*)text;

        switch (id) {
            case ESP_AVRC_MD_ATTR_TITLE:
                if (instance->title_cb) instance->title_cb(str);
                break;

            case ESP_AVRC_MD_ATTR_ARTIST:
                if (instance->artist_cb) instance->artist_cb(str);
                break;

            case ESP_AVRC_MD_ATTR_ALBUM:
                if (instance->album_cb) instance->album_cb(str);
                break;

            case ESP_AVRC_MD_ATTR_PLAYING_TIME:
                /* Ignored here, already handled elsewhere if needed */
                break;

            default:
                break;
        }
    }

    /* ===================== VOLUME CALLBACK ===================== */

    static void volume_callback_static(int volume) {
        if (!instance || !instance->volume_cb) return;
        instance->volume_cb((uint8_t)volume);
    }

    // static void audio_stream_callback_static(const uint8_t* data, uint32_t len) {
    //     if (!instance || !instance->dsp_cb) return;
    //     instance->dsp_cb(data, len);
    // }

};

/* ===================== STATIC INSTANCE ===================== */
BluetoothAudio* BluetoothAudio::instance = nullptr;
