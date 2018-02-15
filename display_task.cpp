#include "display_task.h"
#include "rig.h"
#include "objs.h"

#ifdef ARDUINO_GENERIC_STM32F103C

#include <U8g2lib.h>

#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif

U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

static int counter = 0;

#endif // ARDUINO_GENERIC_STM32F103C

#ifdef ARDUINO_AVR_NANO

#include <LiquidCrystal.h>
LiquidCrystal lcd(8, 9, 10, 11, 12, 13);

#endif // ARDUINO_AVR_NANO

#define UPDATE_DISPLAY (FSM_STATE_USERDEF + 1)

void DisplayTask::init() {

#ifdef ARDUINO_GENERIC_STM32F103C
  u8g2.begin();
  u8g2.enableUTF8Print();

  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x12_tf);
  u8g2.setDrawColor(1); // WHITE

#endif // ARDUINO_GENERIC_STM32F103C

#ifdef ARDUINO_AVR_NANO
  lcd.begin(16, 2);
#endif // ARDUINO_AVR_NANO

  delay(20, UPDATE_DISPLAY);
}

bool DisplayTask::on_state_change(int8_t new_state, int8_t) {
  if (new_state == UPDATE_DISPLAY) {

#ifdef ARDUINO_GENERIC_STM32F103C

    counter ++;
    if (counter > 999) counter = 0;

    char cntMsg[4];
    sprintf(cntMsg, "%03d", counter);

    bool need_update = false;

    u8g2.setDrawColor(1); // WHITE
    u8g2.drawBox(93, 49, 20, 10);
    u8g2.setCursor(94, 50 + 8);
    u8g2.setDrawColor(0); // BLACK
    u8g2.print(cntMsg);
    u8g2.setDrawColor(1); // WHITE
    need_update = true;

    for (uint8_t row = 0; row < 2; row ++) {
      for (uint8_t col = 0; col < 16; col ++) {
        if (_buf[row][col] != _printed[row][col]) {
          int16_t x = 16 + 6 * col;
          int16_t y = 20 + row * 10;
          u8g2.setDrawColor(0); // BLACK
          u8g2.drawBox(x, y, 6, 10);
          u8g2.setCursor(x, y + 8);
          u8g2.setDrawColor(1); // WHITE
          u8g2.print(_buf[row][col]);

          _printed[row][col] = _buf[row][col];
          need_update = true;
        }
      }
    }

    if (need_update) {
      u8g2.sendBuffer();
    }

#endif // ARDUINO_GENERIC_STM32F103C

#ifdef ARDUINO_AVR_NANO
    for (uint8_t row = 0; row < 2; row ++) {
      for (uint8_t col = 0; col < 16; col ++) {
        if (_buf[row][col] != _printed[row][col]) {
          lcd.setCursor(col, row);
          lcd.write(_buf[row][col]);

          _printed[row][col] = _buf[row][col];
        }
      }
    }
#endif // ARDUINO_AVR_NANO

    delay(20, UPDATE_DISPLAY);

  }
  return true;
}

