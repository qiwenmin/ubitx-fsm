#include <Adafruit_SSD1306_STM32.h>
#include "display_task.h"
#include "rig.h"
#include "objs.h"

static int counter = 0;

static Adafruit_SSD1306 _oled(4);

#define UPDATE_DISPLAY (FSM_STATE_USERDEF + 1)

void DisplayTask::init() {
  _oled.begin(SSD1306_SWITCHCAPVCC, 0x3C);

  _oled.clearDisplay();
  _oled.display();

  _oled.setTextSize(1);
  _oled.setTextColor(WHITE);  
  _oled.setTextSize(1);

  print0("----- --- --- --");
  print1("--- --.----- ---");

  rig.refreshDisplay();

  delay(20, UPDATE_DISPLAY);
}

bool DisplayTask::on_state_change(int8_t new_state, int8_t) {
  counter ++;
  if (counter > 999) counter = 0;

  char cntMsg[4];
  sprintf(cntMsg, "%03d", counter);

  bool need_update = false;

  _oled.writeFillRect(93, 49, 20, 10, WHITE);
  _oled.setCursor(94, 50);
  _oled.setTextColor(BLACK);
  _oled.print(cntMsg);
  _oled.setTextColor(WHITE);
  need_update = true;

  if (new_state == UPDATE_DISPLAY) {
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
  }

  if (need_update) {
    _oled.display();
  }

  delay(20, UPDATE_DISPLAY);

  return true;
}

