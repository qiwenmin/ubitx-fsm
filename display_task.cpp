#include "display_task.h"
#include "rig.h"
#include "objs.h"

#ifdef ARDUINO_GENERIC_STM32F103C

#include <Adafruit_SSD1306_STM32.h>
static Adafruit_SSD1306 _oled(4);

#endif // ARDUINO_GENERIC_STM32F103C

static int counter = 0;

#define UPDATE_DISPLAY (FSM_STATE_USERDEF + 1)

void DisplayTask::init() {

#ifdef ARDUINO_GENERIC_STM32F103C
  _oled.begin(SSD1306_SWITCHCAPVCC, 0x3C);

  _oled.clearDisplay();
  _oled.display();

  _oled.setTextSize(1);
  _oled.setTextColor(WHITE);  
  _oled.setTextSize(1);

#endif // ARDUINO_GENERIC_STM32F103C

  print0("----- --- --- --");
  print1("--- --.----- ---");

  rig.refreshDisplay();

  delay(20, UPDATE_DISPLAY);
}

bool DisplayTask::on_state_change(int8_t new_state, int8_t) {
  if (new_state == UPDATE_DISPLAY) {
    counter ++;
    if (counter > 999) counter = 0;

    char cntMsg[4];
    sprintf(cntMsg, "%03d", counter);

#ifdef ARDUINO_GENERIC_STM32F103C

    bool need_update = false;

    _oled.writeFillRect(93, 49, 20, 10, WHITE);
    _oled.setCursor(94, 50);
    _oled.setTextColor(BLACK);
    _oled.print(cntMsg);
    _oled.setTextColor(WHITE);
    need_update = true;

    for (uint8_t row = 0; row < 2; row ++) {
      for (uint8_t col = 0; col < 16; col ++) {
        if (_buf[row][col] != _printed[row][col]) {
          int16_t x = 16 + 6 * col;
          int16_t y = 20 + row * 10;
          _oled.writeFillRect(x, y, 6, 8, BLACK);
          _oled.setCursor(x, y);
          _oled.print(_buf[row][col]);

          _printed[row][col] = _buf[row][col];
          need_update = true;
        }
      }
    }

    if (need_update) {
      _oled.display();
    }

#endif // ARDUINO_GENERIC_STM32F103C

    delay(20, UPDATE_DISPLAY);

  }
  return true;
}

