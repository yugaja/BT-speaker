#ifndef myoptions_h
#define myoptions_h


#define L10N_LANGUAGE		SRB
#define DSP_MODEL			DSP_ST7789
#define DSP_HSPI			true

#define LED_BUILTIN         255

// #define I2S_INTERNAL true
//#define PLAYER_FORCE_MONO   true

#define TFT_DC			    2
#define TFT_CS			    15
#define BRIGHTNESS_PIN		27
#define I2S_DOUT			17
#define I2S_BCLK			22
#define I2S_LRC			    4

#define TS_MODEL			TS_MODEL_CST8xx
#define TS_INT              -1
#define TS_RST              25

#define TS_STEPS            40

//#define LIGHT_SENSOR        255 
//#define AUTOBACKLIGHT_MAX   512

#define IR_PIN              255

#define HIDE_VOLBAR
//#define HIDE_WEATHER
//#define HIDE_VU

#define SMOOTH_FONT

#define DSP_TASK_DELAY      10
#define DSP_TASK_PRIORITY   2

#define CLOCKFONT_MONO      false
#define CLOCK_TTS_ENABLED   true      // Enabled (true) or disabled (false)
#define CLOCK_TTS_LANGUAGE "EN"       // Language ( EN, HU, PL, NL, DE, RU ,FR, EL)
#define CLOCK_TTS_INTERVAL_MINUTES 60 // How many times a minute does it say.
// #define NAMEDAYS_FILE EN // HU, PL, NL
//#define WEATHER_FMT_SHORT
//#define BOOMBOX_STYLE
#define VU_PEAK
#define RSSI_DIGIT true
#define TITLE1_FMT_SHORT
#define TITLE2_FMT_SHORT

#define DEFAULT_SPI_FREQ        40000000


//#define DEBUG_V


//#define SDC_CS			    5


#endif
