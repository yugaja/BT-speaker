// Módosítva! v0.9.710
#include "../../core/options.h"
#if DSP_MODEL != DSP_DUMMY
#ifdef NAMEDAYS_FILE
#include "../../core/namedays.h"
#endif

#include "../../core/config.h"
// #include "../../core/network.h" //  for Clock widget
// #include "../../core/player.h"  //  for VU widget
#include "../dspcore.h"
#include "../tools/l10n.h"
#include "../tools/psframebuffer.h"
#include "Arduino.h"
#include "widgets.h"
#include "../fonts/VT_DIGI_34x19.h"
#include "../fonts/VT_DIGI_68x38.h"

// clang-format off
/************************
      FILL WIDGET
 ************************/
void FillWidget::init(FillConfig conf, uint16_t bgcolor){
  Widget::init(conf.widget, bgcolor, bgcolor);
  _width = conf.width;
  _height = conf.height;
  
}

void FillWidget::_draw(){
  if(!_active) return;
  dsp.fillRect(_config.left, _config.top, _width, _height, _bgcolor);
}

void FillWidget::setHeight(uint16_t newHeight){
  _height = newHeight;
  //_draw();
}
/************************
      TEXT WIDGET
 ************************/
TextWidget::~TextWidget() {
  free(_text);
  free(_oldtext);
}

void TextWidget::_charSize(uint8_t textsize, uint8_t& width, uint16_t& height){
#ifndef DSP_LCD
  width = textsize * CHARWIDTH;
  height = textsize * CHARHEIGHT;
#else
  width = 1;
  height = 1;
#endif
}

void TextWidget::init(WidgetConfig wconf, uint16_t buffsize, bool uppercase, uint16_t fgcolor, uint16_t bgcolor) {
  Widget::init(wconf, fgcolor, bgcolor);
  _buffsize = buffsize;
  _text = (char *) malloc(sizeof(char) * _buffsize);
  memset(_text, 0, _buffsize);
  _oldtext = (char *) malloc(sizeof(char) * _buffsize);
  memset(_oldtext, 0, _buffsize);
  _charSize(_config.textsize, _charWidth, _textheight);
  _textwidth = _oldtextwidth = _oldleft = 0;
  _uppercase = uppercase;
}

void TextWidget::setText(const char* txt) {
  strlcpy(_text, utf8To(txt, _uppercase), _buffsize);
  _textwidth = strlen(_text) * _charWidth;
  if (strcmp(_oldtext, _text) == 0) return;
  if (_active) dsp.fillRect(_oldleft == 0 ? _realLeft() : min(_oldleft, _realLeft()),  _config.top, max(_oldtextwidth, _textwidth), _textheight, _bgcolor);
  _oldtextwidth = _textwidth;
  _oldleft = _realLeft();
  if (_active) _draw();
}

void TextWidget::setText(int val, const char *format){
  char buf[_buffsize];
  snprintf(buf, _buffsize, format, val);
  setText(buf);
}

void TextWidget::setText(const char* txt, const char *format){
  char buf[_buffsize];
  snprintf(buf, _buffsize, format, txt);
  setText(buf);
}

uint16_t TextWidget::_realLeft(bool w_fb) {
  uint16_t realwidth = (_width>0 && w_fb)?_width:dsp.width();
  switch (_config.align) {
    case WA_CENTER: return (realwidth - _textwidth) / 2; break;
    case WA_RIGHT: return (realwidth - _textwidth - (!w_fb?_config.left:0)); break;
    default: return !w_fb?_config.left:0; break;
  }
}

void TextWidget::_draw() {
  if(!_active) return;
  dsp.setTextColor(_fgcolor, _bgcolor);
  dsp.setCursor(_realLeft(), _config.top);
  dsp.setFont();
  dsp.setTextSize(_config.textsize);
  dsp.print(_text);
  strlcpy(_oldtext, _text, _buffsize);
}

/************************
      SCROLL WIDGET
 ************************/
ScrollWidget::ScrollWidget(const char* separator, ScrollConfig conf, uint16_t fgcolor, uint16_t bgcolor) {
  init(separator, conf, fgcolor, bgcolor);
}

ScrollWidget::~ScrollWidget() {
  free(_fb);
  free(_sep);
  free(_window);
}

void ScrollWidget::init(const char* separator, ScrollConfig conf, uint16_t fgcolor, uint16_t bgcolor) {
  TextWidget::init(conf.widget, conf.buffsize, conf.uppercase, fgcolor, bgcolor);
  _sep = (char *) malloc(sizeof(char) * 4);
  memset(_sep, 0, 4);
  snprintf(_sep, 4, " %.*s ", 1, separator);
  _x = conf.widget.left;
  _startscrolldelay = conf.startscrolldelay;
  _scrolldelta = conf.scrolldelta;
  _scrolltime = conf.scrolltime;
  _charSize(_config.textsize, _charWidth, _textheight);
  _sepwidth = strlen(_sep) * _charWidth;
  _width = conf.width;
  _backMove.width = _width;
  _window = (char *) malloc(sizeof(char) * (MAX_WIDTH / _charWidth + 1));
  memset(_window, 0, (MAX_WIDTH / _charWidth + 1));  // +1?
  _doscroll = false;
  #ifdef PSFBUFFER
  _fb = new psFrameBuffer(dsp.width(), dsp.height());
  uint16_t _rl = (_config.align==WA_CENTER)?(dsp.width()-_width)/2:_config.left;
  _fb->begin(&dsp, _rl, _config.top, _width, _textheight, _bgcolor);
  #endif
}

void ScrollWidget::_setTextParams() {
  if (_config.textsize == 0) return;
  if(_fb->ready()){
  #ifdef PSFBUFFER
    _fb->setTextSize(_config.textsize);
    _fb->setTextColor(_fgcolor, _bgcolor);
  #endif
  }else{
    dsp.setTextSize(_config.textsize);
    dsp.setTextColor(_fgcolor, _bgcolor);
  }
}

bool ScrollWidget::_checkIsScrollNeeded() {
  return _textwidth > _width;
}

void ScrollWidget::setText(const char* txt) {
  strlcpy(_text, utf8To(txt, _uppercase), _buffsize - 1);
  if (strcmp(_oldtext, _text) == 0) return;
  _textwidth = strlen(_text) * _charWidth;
  _x = _fb->ready()?0:_config.left;
  _doscroll = _checkIsScrollNeeded();
  if (dsp.getScrollId() == this) dsp.setScrollId(NULL);
  _scrolldelay = millis();
  if (_active) {
    _setTextParams();
    if (_doscroll) {
      if(_fb->ready()){
      #ifdef PSFBUFFER
        _fb->fillRect(0, 0, _width, _textheight, _bgcolor);
        _fb->setCursor(0, 0);
        snprintf(_window, _width / _charWidth + 1, "%s", _text); //TODO
        _fb->print(_window);
        _fb->display();
      #endif
      } else {
        dsp.fillRect(_config.left,  _config.top, _width, _textheight, _bgcolor);
        dsp.setCursor(_config.left, _config.top);
        snprintf(_window, _width / _charWidth + 1, "%s", _text); //TODO
        dsp.setClipping({_config.left, _config.top, _width, _textheight});
        dsp.print(_window);
        dsp.clearClipping();
      }
    } else {
      if(_fb->ready()){
      #ifdef PSFBUFFER
        _fb->fillRect(0, 0, _width, _textheight, _bgcolor);
        _fb->setCursor(_realLeft(true), 0);
        _fb->print(_text);
        _fb->display();
      #endif
      } else {
        dsp.fillRect(_config.left, _config.top, _width, _textheight, _bgcolor);
        dsp.setCursor(_realLeft(), _config.top);
        //dsp.setClipping({_config.left, _config.top, _width, _textheight});
        dsp.print(_text);
        //dsp.clearClipping();
      }
    }
    strlcpy(_oldtext, _text, _buffsize);
  }
}

void ScrollWidget::setText(const char* txt, const char *format){
  char buf[_buffsize];
  snprintf(buf, _buffsize, format, txt);
  setText(buf);
}

void ScrollWidget::loop() {
  if(_locked) return;
  if (!_doscroll || _config.textsize == 0 || (dsp.getScrollId() != NULL && dsp.getScrollId() != this)) return;
  uint16_t fbl = _fb->ready()?0:_config.left;
  if (_checkDelay(_x == fbl ? _startscrolldelay : _scrolltime, _scrolldelay)) {
    _calcX();
    if (_active) _draw();
  }
}

void ScrollWidget::_clear(){
  if(_fb->ready()){
    #ifdef PSFBUFFER
    _fb->fillRect(0, 0, _width, _textheight, _bgcolor);
    _fb->display();
    #endif
  } else {
    dsp.fillRect(_config.left, _config.top, _width, _textheight, _bgcolor);
  }
}

void ScrollWidget::_draw() {
  if(!_active || _locked) return;
  _setTextParams();
  if (_doscroll) {
    uint16_t fbl = _fb->ready()?0:_config.left;
    uint16_t _newx = fbl - _x;
    const char* _cursor = _text + _newx / _charWidth;
    uint16_t hiddenChars = _cursor - _text;
    uint8_t addChars = _fb->ready()?2:1;
    if (hiddenChars < strlen(_text)) {
    //TODO
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wformat-truncation="
      snprintf(_window, _width / _charWidth + addChars, "%s%s%s", _cursor, _sep, _text);
    #pragma GCC diagnostic pop
    } else {
      const char* _scursor = _sep + (_cursor - (_text + strlen(_text)));
      snprintf(_window, _width / _charWidth + addChars, "%s%s", _scursor, _text);
    }
    if(_fb->ready()){
    #ifdef PSFBUFFER
      _fb->fillRect(0, 0, _width, _textheight, _bgcolor);
      _fb->setCursor(_x + hiddenChars * _charWidth, 0);
      _fb->print(_window);
      _fb->display();
    #endif
    } else {
      dsp.setCursor(_x + hiddenChars * _charWidth, _config.top);
      dsp.setClipping({_config.left, _config.top, _width, _textheight});
      dsp.print(_window);
      #ifndef DSP_LCD
        dsp.print(" ");
      #endif
      dsp.clearClipping();
    }
  } else {
    if(_fb->ready()){
    #ifdef PSFBUFFER
      _fb->fillRect(0, 0, _width, _textheight, _bgcolor);
      _fb->setCursor(_realLeft(true), 0);
      _fb->print(_text);
      _fb->display();
    #endif
    } else {
      dsp.fillRect(_config.left, _config.top, _width, _textheight, _bgcolor);
      dsp.setCursor(_realLeft(), _config.top);
      dsp.setClipping({_realLeft(), _config.top, _width, _textheight});
      dsp.print(_text);
      dsp.clearClipping();
    }
  }
}

void ScrollWidget::_calcX() {
  if (!_doscroll || _config.textsize == 0) return;
  _x -= _scrolldelta;
  uint16_t fbl = _fb->ready()?0:_config.left;
  if (-_x > _textwidth + _sepwidth - fbl) {
    _x = fbl;
    dsp.setScrollId(NULL);
  } else {
    dsp.setScrollId(this);
  }
}

bool ScrollWidget::_checkDelay(int m, uint32_t &tstamp) {
  if (millis() - tstamp > m) {
    tstamp = millis();
    return true;
  } else {
    return false;
  }
}

void ScrollWidget::_reset(){
  dsp.setScrollId(NULL);
  _x = _fb->ready()?0:_config.left;
  _scrolldelay = millis();
  _doscroll = _checkIsScrollNeeded();
  #ifdef PSFBUFFER
  _fb->freeBuffer();
  uint16_t _rl = (_config.align==WA_CENTER)?(dsp.width()-_width)/2:_config.left;
  _fb->begin(&dsp, _rl, _config.top, _width, _textheight, _bgcolor);
  #endif
}

/************************
      SLIDER WIDGET
 ************************/
void SliderWidget::init(FillConfig conf, uint16_t fgcolor, uint16_t bgcolor, uint32_t maxval, uint16_t oucolor) {
  Widget::init(conf.widget, fgcolor, bgcolor);
  _width = conf.width; _height = conf.height; _outlined = conf.outlined; _oucolor = oucolor, _max = maxval;
  _oldvalwidth = _value = 0;
}

void SliderWidget::setValue(uint32_t val) {
  _value = val;
  if (_active && !_locked) _drawslider();

}

void SliderWidget::_drawslider() {
  uint16_t valwidth = map(_value, 0, _max, 0, _width - _outlined * 2);
  if (_oldvalwidth == valwidth) return;
  dsp.fillRect(_config.left + _outlined + min(valwidth, _oldvalwidth), _config.top + _outlined, abs(_oldvalwidth - valwidth), _height - _outlined * 2, _oldvalwidth > valwidth ? _bgcolor : _fgcolor);
  _oldvalwidth = valwidth;
}

void SliderWidget::_draw() {
  if(_locked) return;
  _clear();
  if(!_active) return;
  if (_outlined) dsp.drawRect(_config.left, _config.top, _width, _height, _oucolor);
  uint16_t valwidth = map(_value, 0, _max, 0, _width - _outlined * 2);
  dsp.fillRect(_config.left + _outlined, _config.top + _outlined, valwidth, _height - _outlined * 2, _fgcolor);
}

void SliderWidget::_clear() {
//  _oldvalwidth = 0;
  dsp.fillRect(_config.left, _config.top, _width, _height, _bgcolor);
}
void SliderWidget::_reset() {
  _oldvalwidth = 0;
}

// clang-format on
/************************
      VU WIDGET
 ************************/
#if !defined(DSP_LCD) && !defined(DSP_OLED)
VuWidget::~VuWidget() {
  if (_canvas)
    free(_canvas);
}

void VuWidget::init(WidgetConfig wconf, VUBandsConfig bands, uint16_t vumaxcolor, uint16_t vumidcolor, uint16_t vumincolor, uint16_t bgcolor) {
  Widget::init(wconf, bgcolor, bgcolor);
  _vumaxcolor = vumaxcolor;
  _vumidcolor = vumidcolor; // Módosítás új sor.
  _vumincolor = vumincolor;
  _bands = bands;
  //  _canvas = new Canvas(_bands.width * 2 + _bands.space, _bands.height);
  // _canvas = new Canvas(_bands.width, _bands.height * 2 + _bands.space);
#ifndef BOOMBOX_STYLE
  // két VU egymás alatt
  _canvas = new Canvas(_bands.width, _bands.height * 2 + _bands.space);
#else
  // két VU egymás mellett
  _canvas = new Canvas(_bands.width * 2 + _bands.space, _bands.height);
#endif
}

/* A _labelsDrawn változó true értéke esetén rajzolja meg a VU méter előtti L R feliratot.
A  BitrateWidget::_clear() -ben kap false értéket.*/
bool VuWidget::_labelsDrawn = false; // Módosítás

void VuWidget::setLabelsDrawn(bool value) { // Saját
  _labelsDrawn = value;
}

bool VuWidget::isLabelsDrawn() { // Saját
  return _labelsDrawn;
}

#include "..\..\AudioDsp.h"
extern AudioDSP audio_dsp;

/**
 * @brief Linear to log-like lookup table for VU scaling.
 * Maps 0..255 linear input to 0..255 compressed output.
 */
static const uint8_t lin2log[256] = {
    0, 20, 28, 34, 39, 43, 46, 49,
    52, 54, 57, 59, 61, 63, 65, 66,
    68, 70, 71, 73, 74, 76, 77, 78,
    80, 81, 82, 83, 84, 86, 87, 88,
    89, 90, 91, 92, 93, 94, 95, 96,
    97, 98, 99, 100, 101, 102, 103, 104,
    105, 106, 106, 107, 108, 109, 110, 111,
    111, 112, 113, 114, 115, 115, 116, 117,
    118, 119, 119, 120, 121, 122, 122, 123,
    124, 125, 125, 126, 127, 128, 128, 129,
    130, 131, 131, 132, 133, 134, 134, 135,
    136, 137, 137, 138, 139, 140, 140, 141,
    142, 143, 143, 144, 145, 146, 146, 147,
    148, 149, 149, 150, 151, 152, 152, 153,
    154, 155, 155, 156, 157, 158, 158, 159,
    160, 161, 161, 162, 163, 164, 164, 165,
    166, 167, 167, 168, 169, 170, 170, 171,
    172, 173, 173, 174, 175, 176, 176, 177,
    178, 179, 179, 180, 181, 182, 182, 183,
    184, 185, 185, 186, 187, 188, 188, 189,
    190, 191, 191, 192, 193, 194, 194, 195,
    196, 197, 197, 198, 199, 200, 200, 201,
    202, 203, 203, 204, 205, 206, 206, 207,
    208, 209, 209, 210, 211, 212, 212, 213,
    214, 215, 215, 216, 217, 218, 218, 219,
    220, 221, 221, 222, 223, 224, 224, 225,
    226, 227, 227, 228, 229, 230, 230, 231,
    232, 233, 233, 234, 235, 236, 236, 237,
    238, 239, 239, 240, 241, 242, 242, 243,
    244, 245, 245, 246, 247, 248, 248, 249,
    250, 251, 251, 252, 253, 254, 254, 255
};


/**
 * @brief Draws the VU meter widget.
 *
 * This function renders the stereo VU meters with optional peak-hold indicators.
 * It performs refresh rate limiting, level smoothing (fade-back),
 * peak detection with hold and decay, and color-segment rendering.
 *
 * The visual behavior is intentionally decoupled from DSP timing.
 */
void VuWidget::_draw()
{
    if (!_active || _locked)
        return;

    static uint16_t displayL = 0;
    static uint16_t displayR = 0;
    static uint32_t lastDrawTime = 0;

    const uint8_t  refreshTimeMs = 30;
    const uint32_t now = millis();

    if (now - lastDrawTime < refreshTimeMs)
        return;

    lastDrawTime = now;

    const bool played = true; // player.isRunning()

    const uint16_t dimension = _config.align ? _bands.width : _bands.height;

    const uint16_t vuPacked = audio_dsp.getVU(dimension);
    uint16_t levelL = (vuPacked >> 8) & 0xFF;
    uint16_t levelR = vuPacked & 0xFF;

    // Compress small signals
    // uint16_t levelL = lin2log[(vuPacked >> 8) & 0xFF];
    // uint16_t levelR = lin2log[vuPacked & 0xFF];

    // Safety clamp
    if (levelL > dimension) levelL = dimension;
    if (levelR > dimension) levelR = dimension;

    /* ---------------- Level smoothing (fade-back) ---------------- */

    if (played) {
        if (levelL < displayL)
            displayL = (displayL > _bands.fadespeed) ? displayL - _bands.fadespeed : levelL;
        else
            displayL = levelL;

        if (levelR < displayR)
            displayR = (displayR > _bands.fadespeed) ? displayR - _bands.fadespeed : levelR;
        else
            displayR = levelR;
    } else {
        if (displayL > 0)
            displayL = (displayL > _bands.fadespeed) ? displayL - _bands.fadespeed : 0;
        if (displayR > 0)
            displayR = (displayR > _bands.fadespeed) ? displayR - _bands.fadespeed : 0;
    }

    /* ---------------- Peak hold logic ---------------- */
#ifdef VU_PEAK
    static uint16_t peakL = 0;
    static uint16_t peakR = 0;
    static uint32_t peakTimeL = 0;
    static uint32_t peakTimeR = 0;

    const uint16_t peakHoldMs = 200;
    const uint8_t  peakDecay = 3;

#ifndef BOOMBOX_STYLE
    const uint16_t peakPosL = dimension - displayL;
    const uint16_t peakPosR = dimension - displayR;

    if (peakPosL > peakL) {
        peakL = peakPosL;
        peakTimeL = now;
    } else if (now - peakTimeL > peakHoldMs && peakL > peakDecay) {
        peakL -= peakDecay;
    }

    if (peakPosR > peakR) {
        peakR = peakPosR;
        peakTimeR = now;
    } else if (now - peakTimeR > peakHoldMs && peakR > peakDecay) {
        peakR -= peakDecay;
    }
#else
    if (displayL < peakL) {
        peakL = displayL;
        peakTimeL = now;
    } else if (now - peakTimeL > peakHoldMs && peakL + peakDecay < dimension) {
        peakL += peakDecay;
    }

    if (displayR < peakR) {
        peakR = displayR;
        peakTimeR = now;
    } else if (now - peakTimeR > peakHoldMs && peakR + peakDecay < dimension) {
        peakR += peakDecay;
    }
#endif
#endif // VU_PEAK

    /* ---------------- Background clear ---------------- */
#ifndef BOOMBOX_STYLE
    _canvas->fillRect(
        0, 0,
        _bands.width,
        _bands.height * 2 + _bands.space,
        _bgcolor
    );
#else
    _canvas->fillRect(
        0, 0,
        _bands.width * 2 + _bands.space,
        _bands.height,
        _bgcolor
    );
#endif

    /* ---------------- Color segmented bar rendering ---------------- */
    const uint16_t greenEnd  = (_bands.width * 65) / 100;
    const uint16_t yellowEnd = (_bands.width * 85) / 100;
    const uint8_t  segmentW  = (dimension / _bands.perheight) - _bands.vspace;

    for (uint16_t i = 0; i < dimension; i += (dimension / _bands.perheight)) {

        uint16_t color;
        if (i < greenEnd)
            color = _vumincolor;
        else if (i < yellowEnd)
            color = _vumidcolor;
        else
            color = _vumaxcolor;

#ifndef BOOMBOX_STYLE
        _canvas->fillRect(i, 0, segmentW, _bands.height, color);
        _canvas->fillRect(i, _bands.height + _bands.space, segmentW, _bands.height, color);
#else
        _canvas->fillRect(i, 0, segmentW, _bands.height, color);
        _canvas->fillRect(i + _bands.width + _bands.space, 0, segmentW, _bands.height, color);
#endif
    }

    /* ---------------- Level erase ---------------- */
#ifndef BOOMBOX_STYLE
    _canvas->fillRect(
        _bands.width - displayL,
        0,
        displayL,
        _bands.height,
        _bgcolor
    );
    _canvas->fillRect(
        _bands.width - displayR,
        _bands.height + _bands.space,
        displayR,
        _bands.height,
        _bgcolor
    );
#else
    _canvas->fillRect(0, 0, _bands.width - displayL, _bands.height, _bgcolor);
    _canvas->fillRect(
        _bands.width * 2 + _bands.space - displayR,
        0,
        displayR,
        _bands.height,
        _bgcolor
    );
#endif

    /* ---------------- Peak drawing ---------------- */
#ifdef VU_PEAK
    const uint16_t peakColor = 0xFFFF;
    const uint16_t peakGlow  = 0xF7FF;

#ifndef BOOMBOX_STYLE
    if (peakL < _bands.width) {
        _canvas->fillRect(peakL - 1, 0, 3, _bands.height, peakGlow);
        _canvas->fillRect(peakL, 0, 1, _bands.height, peakColor);
    }
    if (peakR < _bands.width) {
        _canvas->fillRect(peakR - 1, _bands.height + _bands.space, 3, _bands.height, peakGlow);
        _canvas->fillRect(peakR, _bands.height + _bands.space, 1, _bands.height, peakColor);
    }
#else
    if (peakL < _bands.width) {
        _canvas->fillRect(peakL - 1, 0, 3, _bands.height, peakGlow);
        _canvas->fillRect(peakL, 0, 1, _bands.height, peakColor);
    }
    if (peakR < _bands.width) {
        _canvas->fillRect(
            _bands.width + _bands.space + peakR - 1,
            0,
            3,
            _bands.height,
            peakGlow
        );
        _canvas->fillRect(
            _bands.width + _bands.space + peakR,
            0,
            1,
            _bands.height,
            peakColor
        );
    }
#endif
#endif // VU_PEAK

    /* ---------------- Final blit ---------------- */
#ifndef BOOMBOX_STYLE
    dsp.drawRGBBitmap(
        _config.left,
        _config.top,
        _canvas->getBuffer(),
        _bands.width,
        _bands.height * 2 + _bands.space
    );
#else
    dsp.startWrite();
    dsp.setAddrWindow(
        _config.left,
        _config.top,
        _bands.width * 2 + _bands.space,
        _bands.height
    );
    dsp.writePixels(
        (uint16_t *)_canvas->getBuffer(),
        (_bands.width * 2 + _bands.space) * _bands.height
    );
    dsp.endWrite();
#endif
/* --------------------------------------------------------------------------
 * Channel labels (L / R)
 * Drawn once when playback is active.
 * -------------------------------------------------------------------------- */
#ifndef BOOMBOX_STYLE
    if (played && !_labelsDrawn) {

        const int labelWidth  = _bands.height + 15;
        const int labelHeight = _bands.height + 4;
        const int labelOffset = labelWidth + 4;
        const int labelLeft   = _config.left - labelOffset;

        if (labelLeft >= 0) {

            // Left channel label background
            dsp.fillRect(
                labelLeft,
                _config.top - 4,
                labelWidth,
                labelHeight,
                0x7BEF
            );

            // Right channel label background
            dsp.fillRect(
                labelLeft,
                _config.top + _bands.height + _bands.space,
                labelWidth,
                labelHeight - 1,
                0x7BEF
            );

            dsp.setTextSize(1);
            dsp.setFont();
            dsp.setTextColor(0xFFFF);

            const int textX = labelLeft + (labelWidth - 6) / 2;

            const int textYL =
                (_config.top - 2) + (labelHeight - 10) / 2;
            const int textYR =
                _config.top + _bands.height + _bands.space +
                (labelHeight - 8) / 2;

            dsp.setCursor(textX, textYL);
            dsp.print("L");

            dsp.setCursor(textX, textYR);
            dsp.print("R");

            _labelsDrawn = true;
        }
    }
#else
    if (played && !_labelsDrawn) {

        const int labelWidth  = _bands.height + 15;
        const int labelHeight = _bands.height + 4;

        // Total width of both labels including spacing
        const int totalWidth = (2 * labelWidth) + 6;

        // Center labels horizontally
        const int centerLeft = (dsp.width() - totalWidth) / 2;

        const int labelLeftL = centerLeft;
        const int labelLeftR = centerLeft + labelWidth + 6;

        dsp.setTextSize(1);
        dsp.setFont();
        dsp.setTextColor(0xFFFF);

        // Left channel label
        dsp.fillRect(
            labelLeftL,
            _config.top - 4,
            labelWidth,
            labelHeight,
            0x7BEF
        );

        dsp.setCursor(
            labelLeftL + (labelWidth - 6) / 2,
            (_config.top - 2) + (labelHeight - 10) / 2
        );
        dsp.print("L");

        // Right channel label
        dsp.fillRect(
            labelLeftR,
            _config.top - 4,
            labelWidth,
            labelHeight,
            0x7BEF
        );

        dsp.setCursor(
            labelLeftR + (labelWidth - 6) / 2,
            (_config.top - 2) + (labelHeight - 10) / 2
        );
        dsp.print("R");

        _labelsDrawn = true;
    }
#endif

}


void VuWidget::loop() {
  if (_active || !_locked)
    _draw();
}

void VuWidget::_clear() {
  // dsp.fillRect(_config.left, _config.top, _bands.width * 2 + _bands.space, _bands.height, _bgcolor);
  dsp.fillRect(0, _config.top - 4, 479, 24, _bgcolor);
  _labelsDrawn = false; // L és R meg keljen rajzolni. Módosítás.
  // Serial.println("widget.cpp -> VuWidget::_clear()");
}
#else // DSP_LCD
VuWidget::~VuWidget() {}
void VuWidget::init(WidgetConfig wconf, VUBandsConfig bands, uint16_t vumaxcolor, uint16_t vumincolor, uint16_t bgcolor) {
  Widget::init(wconf, bgcolor, bgcolor);
}
void VuWidget::_draw() {}
void VuWidget::loop() {}
void VuWidget::_clear() {}
#endif

// clang-format off
/************************
      NUM & CLOCK
 ************************/
#if !defined(DSP_LCD)
  #if TIME_SIZE<19 //19->NOKIA
  const GFXfont* Clock_GFXfontPtr = nullptr;
  #define CLOCKFONT5x7
  #else
  const GFXfont* Clock_GFXfontPtr = &Clock_GFXfont;
  #endif
#endif //!defined(DSP_LCD)

#if !defined(CLOCKFONT5x7) && !defined(DSP_LCD)
  inline GFXglyph *pgm_read_glyph_ptr(const GFXfont *gfxFont, uint8_t c) {
    return gfxFont->glyph + c;
  }
  uint8_t _charWidth(unsigned char c){
    GFXglyph *glyph = pgm_read_glyph_ptr(Clock_GFXfontPtr, c - 0x20);
    return pgm_read_byte(&glyph->xAdvance);
  }
  uint16_t _textHeight(){
    GFXglyph *glyph = pgm_read_glyph_ptr(Clock_GFXfontPtr, '8' - 0x20);
    return pgm_read_byte(&glyph->height);
  }
#else //!defined(CLOCKFONT5x7) && !defined(DSP_LCD)
  uint8_t _charWidth(unsigned char c){
  #ifndef DSP_LCD
    return CHARWIDTH * TIME_SIZE;
  #else
    return 1;
  #endif
  }
  uint16_t _textHeight(){
    return CHARHEIGHT * TIME_SIZE;
  }
#endif
uint16_t _textWidth(const char *txt){
  uint16_t w = 0, l=strlen(txt);
  for(uint16_t c=0;c<l;c++) w+=_charWidth(txt[c]);
//  #if DSP_MODEL==DSP_ILI9225
//  return w+l;
//  #else
  return w;
//  #endif
}

/************************
      NUM WIDGET
 ************************/
void NumWidget::init(WidgetConfig wconf, uint16_t buffsize, bool uppercase, uint16_t fgcolor, uint16_t bgcolor) {
  Widget::init(wconf, fgcolor, bgcolor);
  _buffsize = buffsize;
  _text = (char *) malloc(sizeof(char) * _buffsize);
  memset(_text, 0, _buffsize);
  _oldtext = (char *) malloc(sizeof(char) * _buffsize);
  memset(_oldtext, 0, _buffsize);
  _textwidth = _oldtextwidth = _oldleft = 0;
  _uppercase = uppercase;
  _textheight = TIME_SIZE/*wconf.textsize*/;
}

void NumWidget::setText(const char* txt) {
  strlcpy(_text, txt, _buffsize);
  _getBounds();
  if (strcmp(_oldtext, _text) == 0) return;
  uint16_t realth = _textheight;
#if defined(DSP_OLED) && DSP_MODEL!=DSP_SSD1322
  if(Clock_GFXfontPtr==nullptr) realth = _textheight * 8; //CHARHEIGHT
#endif
  if (_active)
  #ifndef CLOCKFONT5x7
    dsp.fillRect(_oldleft == 0 ? _realLeft() : min(_oldleft, _realLeft()),  _config.top-_textheight+1, max(_oldtextwidth, _textwidth), realth, _bgcolor);
  #else
    dsp.fillRect(_oldleft == 0 ? _realLeft() : min(_oldleft, _realLeft()),  _config.top, max(_oldtextwidth, _textwidth), realth, _bgcolor);
  #endif

  _oldtextwidth = _textwidth;
  _oldleft = _realLeft();
  if (_active) _draw();
}

void NumWidget::setText(int val, const char *format){
  char buf[_buffsize];
  snprintf(buf, _buffsize, format, val);
  setText(buf);
}

void NumWidget::_getBounds() {
  _textwidth= _textWidth(_text);
}

void NumWidget::_draw() {
#ifndef DSP_LCD
  if(!_active || TIME_SIZE<2) return;
  dsp.setTextSize(Clock_GFXfontPtr==nullptr?TIME_SIZE:1);
  dsp.setFont(Clock_GFXfontPtr);
  dsp.setTextColor(_fgcolor, _bgcolor);
#endif
  if(!_active) return;
  dsp.setCursor(_realLeft(), _config.top);
  dsp.print(_text);
  strlcpy(_oldtext, _text, _buffsize);
  dsp.setFont();
}

/**************************
      PROGRESS WIDGET
 **************************/
void ProgressWidget::_progress() {
  char buf[_width + 1];
  snprintf(buf, _width, "%*s%.*s%*s", _pg <= _barwidth ? 0 : _pg - _barwidth, "", _pg <= _barwidth ? _pg : 5, ".....", _width - _pg, "");
  _pg++; if (_pg >= _width + _barwidth) _pg = 0;
  setText(buf);
}

bool ProgressWidget::_checkDelay(int m, uint32_t &tstamp) {
  if (millis() - tstamp > m) {
    tstamp = millis();
    return true;
  } else {
    return false;
  }
}

void ProgressWidget::loop() {
  if (_checkDelay(_speed, _scrolldelay)) {
    _progress();
  }
}

/**************************
      CLOCK WIDGET
 **************************/
void ClockWidget::init(WidgetConfig wconf, uint16_t fgcolor, uint16_t bgcolor){
  Widget::init(wconf, fgcolor, bgcolor);
  _timeheight = _textHeight();
  _fullclock = TIME_SIZE>35 || DSP_MODEL==DSP_ILI9225;
  if(_fullclock) _superfont = TIME_SIZE / 17; //magick
  else if(TIME_SIZE==19 || TIME_SIZE==2) _superfont=1;
  else _superfont=0;
  _space = (5*_superfont)/2; //magick
  #ifndef HIDE_DATE
  if(_fullclock){
    _dateheight = _superfont<4?1:2;
    _clockheight = _timeheight + _space + CHARHEIGHT * _dateheight;
  } else {
    _clockheight = _timeheight;
  }
  #else
    _clockheight = _timeheight;
  #endif
  _getTimeBounds();
#ifdef PSFBUFFER
  _fb = new psFrameBuffer(dsp.width(), dsp.height());
  _begin();
#endif
}

void ClockWidget::_begin(){
#ifdef PSFBUFFER
  _fb->begin(&dsp, _clockleft, _config.top-_timeheight, _clockwidth, _clockheight+1, config.theme.background);
#endif
}

bool ClockWidget::_getTime(){
  // strftime(_timebuffer, sizeof(_timebuffer), "%H:%M", &network.timeinfo);
  // bool ret = network.timeinfo.tm_sec==0 || _forceflag!=network.timeinfo.tm_year;
  // _forceflag = network.timeinfo.tm_year;
  // return ret;
}

uint16_t ClockWidget::_left(){
  if(_fb->ready()) return 0; else return _clockleft;
}
uint16_t ClockWidget::_top(){
  if(_fb->ready()) return _timeheight; else return _config.top;
}

void ClockWidget::_getTimeBounds() {
  _timewidth = _textWidth(_timebuffer);
  uint8_t fs = _superfont>0?_superfont:TIME_SIZE;
  uint16_t rightside = CHARWIDTH * fs * 2; // seconds
  if(_fullclock){
    rightside += _space*2+1; //2space+vline
    _clockwidth = _timewidth+rightside;
  } else {
    if(_superfont==0)
      _clockwidth = _timewidth;
    else
      _clockwidth = _timewidth + rightside;
  }
  switch(_config.align){
    case WA_LEFT: _clockleft = _config.left; break;
    case WA_RIGHT: _clockleft = dsp.width()-_clockwidth-_config.left; break;
    default:
      _clockleft = (dsp.width()/2 - _clockwidth/2)+_config.left;
      break;
  }
  char buf[4];
  // strftime(buf, 4, "%H", &network.timeinfo);
  _dotsleft=_textWidth(buf);
}

#ifndef DSP_LCD

Adafruit_GFX& ClockWidget::getRealDsp(){
#ifdef PSFBUFFER
  if (_fb && _fb->ready()) return *_fb;
#endif
  return dsp;
}

void ClockWidget::_printClock(bool force){
  auto& gfx = getRealDsp();
  gfx.setTextSize(Clock_GFXfontPtr==nullptr?TIME_SIZE:1);
  gfx.setFont(Clock_GFXfontPtr);
  bool clockInTitle=!config.isScreensaver && _config.top<_timeheight; //DSP_SSD1306x32
  if(force){
    _clearClock();
    _getTimeBounds();
    #ifndef DSP_OLED
    if(CLOCKFONT_MONO) {
      gfx.setTextColor(config.theme.clockbg, config.theme.background);
      gfx.setCursor(_left(), _top());
      gfx.print("88:88");
    }
    #endif
    if(clockInTitle)
      gfx.setTextColor(config.theme.meta, config.theme.metabg);
    else
      gfx.setTextColor(config.theme.clock, config.theme.background);
    gfx.setCursor(_left(), _top());
    gfx.print(_timebuffer);
    if(_fullclock){
      // lines, date & dow
      bool fullClockOnScreensaver = (!config.isScreensaver || (_fb->ready() && FULL_SCR_CLOCK));
      _linesleft = _left()+_timewidth+_space;
      if(fullClockOnScreensaver){
        gfx.drawFastVLine(_linesleft, _top()-_timeheight, _timeheight, config.theme.div);
        gfx.drawFastHLine(_linesleft, _top()-(_timeheight)/2, CHARWIDTH * _superfont * 2 + _space, config.theme.div);
        gfx.setFont();
        gfx.setTextSize(_superfont);
        gfx.setCursor(_linesleft+_space+1, _top()-CHARHEIGHT * _superfont);
        gfx.setTextColor(config.theme.dow, config.theme.background);
        // gfx.print(utf8To(LANG::dow[network.timeinfo.tm_wday], false));
        // sprintf(_tmp, "%2d %s %d", network.timeinfo.tm_mday,LANG::mnths[network.timeinfo.tm_mon], network.timeinfo.tm_year+1900);
        #ifndef HIDE_DATE
        strlcpy(_datebuf, utf8To(_tmp, true), sizeof(_datebuf));
        uint16_t _datewidth = strlen(_datebuf) * CHARWIDTH*_dateheight;
        gfx.setTextSize(_dateheight);
        #if DSP_MODEL==DSP_GC9A01A
        gfx.setCursor((dsp.width()-_datewidth)/2, _top() + _space);
        #else
        gfx.setCursor(_left()+_clockwidth-_datewidth, _top() + _space);
        #endif
        gfx.setTextColor(config.theme.date, config.theme.background);
        gfx.print(_datebuf);
        #endif
      }
    }
  }
  if(_fullclock || _superfont>0){
    gfx.setFont();
    gfx.setTextSize(_superfont);
    if(!_fullclock){
      #ifndef CLOCKFONT5x7
      gfx.setCursor(_left()+_timewidth+_space, _top()-_timeheight+_space);
      #else
      gfx.setCursor(_left()+_timewidth+_space, _top());
      #endif
    }else{
      gfx.setCursor(_linesleft+_space+1, _top()-_timeheight);
    }
    gfx.setTextColor(config.theme.seconds, config.theme.background);
    // sprintf(_tmp, "%02d", network.timeinfo.tm_sec);
    gfx.print(_tmp);
  }
  gfx.setTextSize(Clock_GFXfontPtr==nullptr?TIME_SIZE:1);
  gfx.setFont(Clock_GFXfontPtr);
  #ifndef DSP_OLED
  gfx.setTextColor(dots ? config.theme.clock : (CLOCKFONT_MONO?config.theme.clockbg:config.theme.background), config.theme.background);
  #else
  if(clockInTitle)
    gfx.setTextColor(dots ? config.theme.meta:config.theme.metabg, config.theme.metabg);
  else
    gfx.setTextColor(dots ? config.theme.clock:config.theme.background, config.theme.background);
  #endif
  dots=!dots;
  gfx.setCursor(_left()+_dotsleft, _top());
  gfx.print(":");
  gfx.setFont();
  if(_fb->ready()) _fb->display();
}

/*********************  A névnapok kiírása. *****************************/
#ifdef NAMEDAYS_FILE
void ClockWidget::getNamedayUpper(char *dest, size_t len) { // commongfx.h - ban van deklarálva.
  const char *nameday = getNameDay(network.timeinfo.tm_mon + 1, network.timeinfo.tm_mday);
  char        tmp[32];
  strlcpy(tmp, nameday, sizeof(tmp));
  for (int i = 0; tmp[i]; i++) {
    tmp[i] = toupper((unsigned char)tmp[i]);
  }
  strlcpy(dest, utf8To(tmp, true), len);
}

void ClockWidget::_printNameday() {
  // Névnap nevének területének törlése a kijelzőről.
  if (_oldnamedayleft > 0) {
    int clearWidth = max(_oldnamedaywidth, _namedaywidth); // A régi és az új név közül a szélesebb szélessége.
    dsp.fillRect(_oldnamedayleft, _config.top - 14, clearWidth, CHARHEIGHT * 2, config.theme.background);
  }
  // Névnapi terület törlése letiltás esetén
  //  if (_oldnamedayleft > 0 && _oldnamedaywidth > 0) {
  //    dsp.fillRect(_oldnamedayleft, clockTop - 14, _oldnamedaywidth, CHARHEIGHT * 2, config.theme.background);
  // }
  // Rajzold le a nyelvfüggő "Névnap:" szót fehér színnel.
  dsp.setTextColor(config.theme.date, config.theme.background);
  dsp.setCursor(_namedayleft, _config.top - 37); // egy sorral feljebb
  dsp.setTextSize(2);
  if (!config.isScreensaver)
    dsp.print(utf8To(nameday_label, false)); // <<< Itt már a headerből jön

  // Csak a neveket rajzolja arany színnel
  dsp.setTextColor(config.theme.nameday, config.theme.background); // szürke 0x8410
  dsp.setCursor(_namedayleft, _config.top - 14);
  dsp.setTextSize(2);
  if (!config.isScreensaver)
    dsp.print(_namedayBuf);

  strlcpy(_oldNamedayBuf, _namedayBuf, sizeof(_namedayBuf));
  _oldnamedaywidth = _namedaywidth;
  _oldnamedayleft = _namedayleft;
}
#endif //NAMEDAYS_FILE

void ClockWidget::_clearClock(){
#ifdef PSFBUFFER
  if(_fb->ready()) _fb->clear();
  else
#endif
#ifndef CLOCKFONT5x7
//dsp.fillRect(_left(), _top()-_timeheight, _clockwidth, _clockheight+1, 0x8410);
  dsp.fillRect(_left(), _top()-_timeheight, _clockwidth, _clockheight+1, config.theme.background);
// Serial.println("Törlés");
#else
  dsp.fillRect(_left(), _top(), _clockwidth+1, _clockheight+1, config.theme.background);
#endif
}

void ClockWidget::draw(bool force){
  if(!_active) return;
  _printClock(_getTime() || force);
}

void ClockWidget::_draw(){
  if(!_active) return;
  _printClock(true);
}

void ClockWidget::_reset(){
#ifdef PSFBUFFER
  if(_fb->ready()) {
    _fb->freeBuffer();
    _getTimeBounds();
    _begin();
  }
#endif
}

void ClockWidget::_clear(){
  _clearClock();
}
#else //#ifndef DSP_LCD

void ClockWidget::_printClock(bool force){
  strftime(_timebuffer, sizeof(_timebuffer), "%H:%M", &network.timeinfo);
  if(force){
    dsp.setCursor(dsp.width()-5, 0);
    dsp.print(_timebuffer);
  }
  dsp.setCursor(dsp.width()-5+2, 0);
  dsp.print((network.timeinfo.tm_sec % 2 == 0)?":":" ");
}

void ClockWidget::_clearClock(){}

void ClockWidget::draw(bool force){
  if(!_active) return;
  _printClock(true);
}
void ClockWidget::_draw(){
  if(!_active) return;
  _printClock(true);
}
void ClockWidget::_reset(){}
void ClockWidget::_clear(){}
#endif //#ifndef DSP_LCD

/**************************
      BITRATE WIDGET
 **************************/
void BitrateWidget::init(BitrateConfig bconf, uint16_t fgcolor, uint16_t bgcolor){
  Widget::init(bconf.widget, fgcolor, bgcolor);
  _dimension = bconf.dimension;
  _bitrate = 0;
  _format = BF_UNKNOWN;
  _charSize(bconf.widget.textsize, _charWidth, _textheight);
  memset(_buf, 0, 6);
}

void BitrateWidget::setBitrate(uint16_t bitrate){
  _bitrate = bitrate;
  if(_bitrate>999) _bitrate=999;
  _draw();
}

void BitrateWidget::setFormat(BitrateFormat format){
  _format = format;
  _draw();
}

//TODO move to parent
void BitrateWidget::_charSize(uint8_t textsize, uint8_t& width, uint16_t& height){
#ifndef DSP_LCD
  width = textsize * CHARWIDTH;
  height = textsize * CHARHEIGHT;
#else
  width = 1;
  height = 1;
#endif
}

void BitrateWidget::_draw(){
  _clear();
  if(!_active || _format == BF_UNKNOWN || _bitrate==0) return;
  dsp.drawRect(_config.left, _config.top, _dimension, _dimension, _fgcolor);
  dsp.fillRect(_config.left, _config.top + _dimension/2, _dimension, _dimension/2, _fgcolor);
  dsp.setFont();
  dsp.setTextSize(_config.textsize);
  dsp.setTextColor(_fgcolor, _bgcolor);
  snprintf(_buf, 6, "%d", _bitrate);
  dsp.setCursor(_config.left + _dimension/2 - _charWidth*strlen(_buf)/2 + 1, _config.top + _dimension/4 - _textheight/2+1);
  dsp.print(_buf);
  dsp.setTextColor(_bgcolor, _fgcolor);
  dsp.setCursor(_config.left + _dimension/2 - _charWidth*3/2 + 1, _config.top + _dimension - _dimension/4 - _textheight/2);
  switch(_format){
    case BF_MP3:  dsp.print("MP3"); break;
    case BF_AAC:  dsp.print("AAC"); break;
    case BF_FLAC: dsp.print("FLC"); break;
    case BF_OGG:  dsp.print("OGG"); break;
    case BF_WAV:  dsp.print("WAV"); break;
    default:                        break;
  }
}

void BitrateWidget::_clear() {
  dsp.fillRect(_config.left, _config.top, _dimension, _dimension, _bgcolor);
}


/**************************
      PLAYLIST WIDGET
 **************************/
void PlayListWidget::init(ScrollWidget* current){
  Widget::init({0, 0, 0, WA_LEFT}, 0, 0);
  _current = current;
  #ifndef DSP_LCD
  _plItemHeight = playlistConf.widget.textsize*(CHARHEIGHT-1)+playlistConf.widget.textsize*4;
  _plTtemsCount = round((float)dsp.height()/_plItemHeight);
  if(_plTtemsCount%2==0) _plTtemsCount++;
  _plCurrentPos = _plTtemsCount/2;
  _plYStart = (dsp.height() / 2 - _plItemHeight / 2) - _plItemHeight * (_plTtemsCount - 1) / 2 + playlistConf.widget.textsize*2;
  #else
  _plTtemsCount = PLMITEMS;
  _plCurrentPos = 1;
  #endif
}

uint8_t PlayListWidget::_fillPlMenu(int from, uint8_t count) {
  int     ls      = from;
  uint8_t c       = 0;
  bool    finded  = false;
  if (config.playlistLength() == 0) {
    return 0;
  }
  File playlist = config.SDPLFS()->open(REAL_PLAYL, "r");
  File index = config.SDPLFS()->open(REAL_INDEX, "r");
  while (true) {
    if (ls < 1) {
      ls++;
      _printPLitem(c, "");
      c++;
      continue;
    }
    if (!finded) {
      index.seek((ls - 1) * 4, SeekSet);
      uint32_t pos;
      index.readBytes((char *) &pos, 4);
      finded = true;
      index.close();
      playlist.seek(pos, SeekSet);
    }
    bool pla = true;
    while (pla) {
      pla = playlist.available();
      String stationName = playlist.readStringUntil('\n');
      stationName = stationName.substring(0, stationName.indexOf('\t'));
      if(config.store.numplaylist && stationName.length()>0) stationName = String(from+c)+" "+stationName;
      _printPLitem(c, stationName.c_str());
      c++;
      if (c >= count) break;
    }
    break;
  }
  playlist.close();
  return c;
}
#ifndef DSP_LCD
void PlayListWidget::drawPlaylist(uint16_t currentItem) {
  uint8_t lastPos = _fillPlMenu(currentItem - _plCurrentPos, _plTtemsCount);
  if(lastPos<_plTtemsCount){
    dsp.fillRect(0, lastPos*_plItemHeight+_plYStart, dsp.width(), dsp.height()/2, config.theme.background);
  }
}

void PlayListWidget::_printPLitem(uint8_t pos, const char* item){
  dsp.setTextSize(playlistConf.widget.textsize);
  if (pos == _plCurrentPos) {
    _current->setText(item);
  } else {
    uint8_t plColor = (abs(pos - _plCurrentPos)-1)>4?4:abs(pos - _plCurrentPos)-1;
    dsp.setTextColor(config.theme.playlist[plColor], config.theme.background);
    dsp.setCursor(TFT_FRAMEWDT, _plYStart + pos * _plItemHeight);
    dsp.fillRect(0, _plYStart + pos * _plItemHeight - 1, dsp.width(), _plItemHeight - 2, config.theme.background);
    dsp.print(utf8To(item, true));
  }
}
#else
void PlayListWidget::_printPLitem(uint8_t pos, const char* item){
  if (pos == _plCurrentPos) {
    _current->setText(item);
  } else {
    dsp.setCursor(1, pos);
    char tmp[dsp.width()] = {0};
    strlcpy(tmp, utf8To(item, true), dsp.width());
    dsp.print(tmp);
  }
}

void PlayListWidget::drawPlaylist(uint16_t currentItem) {
  dsp.clear();
  _fillPlMenu(currentItem - _plCurrentPos, _plTtemsCount);
  dsp.setCursor(0,1);
  dsp.write(uint8_t(126));
}
#endif //DSP_LCD


#endif // #if DSP_MODEL!=DSP_DUMMY