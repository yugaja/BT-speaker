
#include "Arduino.h"
#include "core/options.h"
#include "core/config.h"
#include "core/display.h"
#include "core/controls.h"
#include "core/optionschecker.h"
#include "core/timekeeper.h"
#include "BluetoothAudio.h"
#include "AudioDsp.h"
#include "A2DPAudioDSP.h"

BluetoothAudio BT;

#if DSP_HSPI || TS_HSPI || VS_HSPI
SPIClass  SPI2(HSPI);
#endif

void setTitle(const char* title){
  config.setTitle(title);
}
void setArtist(const char* title){
   config.setUrl(title);
}

AudioDSP audio_dsp(I2S_NUM_0);  
A2DPAudioDSP bt_dsp(&audio_dsp);

void setup() {
  Serial.begin(115200);
  config.init();

  //restore  tone settings
  audio_dsp.setBalance(config.store.balance);
  audio_dsp.setTone(config.store.bass, config.store.middle, config.store.trebble);
  audio_dsp.setVolume(254);

  BT.setup("yugaja_mali");  
  BT.onTitleChanged(setTitle);
  BT.onArtistChanged(setArtist);
  BT.sink().set_volume_control(&bt_dsp);
  
  display.init();


  initControls();
  display.putRequest(DSP_START);
  while(!display.ready()) delay(10);    
  config.setStation("BT SPEAKER");
  display.putRequest(NEWSTATION);

}

void loop() {
    BT.loop();
    loopControls();
}
