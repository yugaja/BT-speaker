#include "options.h"
#if (TS_MODEL!=TS_MODEL_UNDEFINED) && (DSP_MODEL!=DSP_DUMMY)
#include "Arduino.h"
#include "touchscreen.h"
#include "config.h"
#include "controls.h"
#include "display.h"
// #include "player.h"

#ifndef TS_X_MIN
  #define TS_X_MIN              400
#endif
#ifndef TS_X_MAX
  #define TS_X_MAX              3800
#endif
#ifndef TS_Y_MIN
  #define TS_Y_MIN              260
#endif
#ifndef TS_Y_MAX
  #define TS_Y_MAX              3800
#endif
#ifndef TS_STEPS
  #define TS_STEPS              40
#endif

#if TS_MODEL==TS_MODEL_XPT2046
  #ifdef TS_SPIPINS
    SPIClass  TSSPI(HSPI);
  #endif
  #include <XPT2046_Touchscreen.h>
  XPT2046_Touchscreen ts(TS_CS);
  typedef TS_Point TSPoint;
#elif TS_MODEL==TS_MODEL_GT911
  #include "../GT911_Touchscreen/TAMC_GT911.h"
  TAMC_GT911 ts = TAMC_GT911(TS_SDA, TS_SCL, TS_INT, TS_RST, 0, 0);
  typedef TP_Point TSPoint;
#elif TS_MODEL==TS_MODEL_CST8xx			//=====================Touchscreen CST8xx======================
  #include "CST820.h"
  //#include "TouchDrvCSTXXX.hpp"
  CST820 ts(TS_SDA, TS_SCL, TS_RST, TS_INT);
  //TouchDrvCSTXXX ts;
#endif
#include <src/dual_boot.h>

void TouchScreen::init(uint16_t w, uint16_t h){
  
#if TS_MODEL==TS_MODEL_XPT2046
  #ifdef TS_SPIPINS
    TSSPI.begin(TS_SPIPINS);
    ts.begin(TSSPI);
  #else
    #if TS_HSPI
      ts.begin(SPI2);
    #else
      ts.begin();
    #endif
  #endif
  ts.setRotation(config.store.fliptouch?3:1);
#endif
#if TS_MODEL==TS_MODEL_GT911
  ts.begin();
  ts.setRotation(config.store.fliptouch?0:2);
#endif
  _width  = w;
  _height = h;
#if TS_MODEL==TS_MODEL_GT911
  ts.setResolution(_width, _height);
#endif  
#if TS_MODEL==TS_MODEL_CST8xx				//=====================Touchscreen CST8xx======================
    ts.begin();
    ts.setMaxCoordinates(_width,_height);// Размер экрана, для зеркализации
    ts.setMirrorXY(!config.store.fliptouch,!config.store.fliptouch); //зеркализируем оси X и Y 
    ts.setSwapXY(true); //Меняем оси XY местами
#endif
}

tsDirection_e TouchScreen::_tsDirection(uint16_t x, uint16_t y) {
  int16_t dX = x - _oldTouchX;
  int16_t dY = y - _oldTouchY;
  if (abs(dX) > 20 || abs(dY) > 20) {
    if (abs(dX) > abs(dY)) {
      if (dX > 0) {
        return TSD_RIGHT;
      } else {
        return TSD_LEFT;
      }
    } else {
      if (dY > 0) {
        return TSD_DOWN;
      } else {
        return TSD_UP;
      }
    }
  } else {
    return TDS_REQUEST;
  }
}

void TouchScreen::flip(){
#if TS_MODEL==TS_MODEL_XPT2046
  ts.setRotation(config.store.fliptouch?3:1);
#endif
#if TS_MODEL==TS_MODEL_GT911
  ts.setRotation(config.store.fliptouch?0:2);
#endif
#if TS_MODEL==TS_MODEL_CST8xx                //=====================Touchscreen CST8xx======================
    ts.setMirrorXY(!config.store.fliptouch,!config.store.fliptouch); //зеркализируем X и Y 
    ts.setSwapXY(true);	
#endif

}

/**
 * @brief Main touchscreen loop handler.
 *
 * Clean separation of swipe, short tap and long press.
 * Long press is detected only if finger does not move.
 * Swipe always has priority and cancels tap/long press.
 */
void TouchScreen::loop() {
    static bool wasTouched = false;
    static bool swipeDetected = false;
    static bool longPressFired = false;
    static bool movedTooMuch = false;

    static uint32_t touchDownTime = 0;
    static uint32_t lastSwipeTime = 0;
    static uint32_t lastTapTime = 0;

    static uint16_t startX = 0, startY = 0;
    static uint16_t lastX = 0, lastY = 0;

    const uint32_t MIN_TOUCH_DURATION   = 50;
    const uint32_t SHORT_TAP_MAX        = 300;
    const uint32_t SHORT_TAP_DEBOUNCE   = 250;
    const uint32_t LONG_PRESS_TIME     = 3000;

    const int MOVE_TOLERANCE_X = 10;
    const int MOVE_TOLERANCE_Y = 10;

    if (!_checklpdelay(20, _touchdelay)) return;

    uint16_t touchX = 0, touchY = 0;
    bool isTouched = false;

#if TS_MODEL == TS_MODEL_XPT2046
    TSPoint p = ts.getPoint();
    touchX = map(p.x, TS_X_MIN, TS_X_MAX, 0, _width);
    touchY = map(p.y, TS_Y_MIN, TS_Y_MAX, 0, _height);
#endif

#if TS_MODEL == TS_MODEL_GT911
    ts.read();
#endif

#if TS_MODEL == TS_MODEL_CST8xx
    static uint8_t gesture = 0;
    isTouched = ts.getTouch(&touchX, &touchY, &gesture);
#else
    isTouched = _istouched();
#endif

    /* ---------------- TOUCH DOWN / HOLD ---------------- */
    if (isTouched) {

        if (!wasTouched) {
            wasTouched = true;
            swipeDetected = false;
            longPressFired = false;
            movedTooMuch = false;

            touchDownTime = millis();
            startX = lastX = touchX;
            startY = lastY = touchY;
            return;
        }

        int dxTotal = (int)touchX - (int)startX;
        int dyTotal = (int)touchY - (int)startY;

        if (abs(dxTotal) > MOVE_TOLERANCE_X || abs(dyTotal) > MOVE_TOLERANCE_Y) {
            movedTooMuch = true;
        }

        /* ---------- SWIPE DETECTION ---------- */
        int dx = (int)touchX - (int)lastX;
        int dy = (int)touchY - (int)lastY;

        if ((abs(dx) > 10 || abs(dy) > 20) && millis() - lastSwipeTime > 180) {
            swipeDetected = true;

            if (abs(dx) > abs(dy)) {
                if (display.mode() == PLAYER || display.mode() == VOL) {
                    // display.putRequest(NEWMODE, VOL);
                    // int volDelta = dx / 10;
                    // volDelta = constrain(volDelta, -10, 10);
                    // controlsEvent(dx > 0, (int8_t)volDelta);
                }
            } else {
                // if (display.mode() == PLAYER || display.mode() == STATIONS) {
                //     display.putRequest(NEWMODE, STATIONS);
                //     controlsEvent(dy < 0, 0);
                // }
            }

            lastSwipeTime = millis();
            lastX = touchX;
            lastY = touchY;
            return;
        }

        /* ---------- LONG PRESS DETECTION ---------- */
        if (!swipeDetected && !movedTooMuch && !longPressFired) {
            if (millis() - touchDownTime >= LONG_PRESS_TIME) {
                longPressFired = true;
                reboot_to_wifi();
                // display.putRequest(
                //     NEWMODE,
                //     display.mode() == PLAYER ? STATIONS : PLAYER
                // );

                if (config.store.dbgtouch)
                    Serial.println(F("[TOUCH] Long press fired"));
            }
        }

        return;
    }

    /* ---------------- TOUCH RELEASE ---------------- */
    if (wasTouched) {
        wasTouched = false;

        uint32_t pressTime = millis() - touchDownTime;

        if (pressTime < MIN_TOUCH_DURATION) return;

        if (!swipeDetected && !longPressFired && !movedTooMuch) {
            if (pressTime < SHORT_TAP_MAX) {
                uint32_t now = millis();
                if (now - lastTapTime > SHORT_TAP_DEBOUNCE) {
                    lastTapTime = now;
                    onBtnClick(EVT_BTNCENTER);

                    if (config.store.dbgtouch)
                        Serial.println(F("[TOUCH] Short tap"));
                }
            }
        }
    }
}


bool TouchScreen::_checklpdelay(int m, uint32_t &tstamp) {
  if (millis() - tstamp > m) {
    tstamp = millis();
    return true;
  } else {
    return false;
  }
}

bool TouchScreen::_istouched(){
#if TS_MODEL==TS_MODEL_XPT2046
  return ts.touched();
#elif TS_MODEL==TS_MODEL_GT911
  return ts.isTouched;
#elif TS_MODEL==TS_MODEL_CST8xx  //=====================Touchscreen CST8xx======================
return true; //для совместимости
#endif
}

#endif  // TS_MODEL!=TS_MODEL_UNDEFINED
