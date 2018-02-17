#include "display_task.h"
#include "rig.h"
#include "objs.h"

#include <LiquidCrystal.h>
LiquidCrystal lcd(8, 9, 10, 11, 12, 13);

const PROGMEM uint8_t cust_char_table[] = {
  // 0 - lock
  0b01110,
  0b10001,
  0b10001,
  0b11111,
  0b11011,
  0b11011,
  0b11111,
  0b00000,

  // 1 - Left arrow
  0b00001,
  0b00011,
  0b00111,
  0b01111,
  0b00111,
  0b00011,
  0b00001,
  0b00000,

  // 2 - Right arrow
  0b10000,
  0b11000,
  0b11100,
  0b11110,
  0b11100,
  0b11000,
  0b10000,
  0b00000,

  // 3 - Down arrow
  0b00000,
  0b00000,
  0b00000,
  0b00000,
  0b00000,
  0b11111,
  0b01110,
  0b00100,

  // 4, 5, 6 - TX
  0b00000,
  0b00000,
  0b00000,
  0b00000,
  0b00001,
  0b00101,
  0b10101,
  0b00000,

  0b11100,
  0b01000,
  0b01000,
  0b01000,
  0b00001,
  0b00101,
  0b10101,
  0b00000,

  0b00100,
  0b00100,
  0b10100,
  0b10100,
  0b10100,
  0b10100,
  0b10100,
  0b00000
};

const uint8_t cust_char_table_len = sizeof(cust_char_table) / sizeof(cust_char_table[0]);

#define UPDATE_DISPLAY (FSM_STATE_USERDEF + 1)

void DisplayTask::init() {
  lcd.begin(16, 2);

  uint8_t cust_char[8];

  for (uint8_t i = 0; i < cust_char_table_len / 8; i ++) {
    memcpy_P(&cust_char, &cust_char_table[i * 8], 8);
    lcd.createChar(i, cust_char);
  }

  delay(20, UPDATE_DISPLAY);
}

bool DisplayTask::on_state_change(int8_t new_state, int8_t) {
  if (new_state == UPDATE_DISPLAY) {
    for (uint8_t row = 0; row < 2; row ++) {
      for (uint8_t col = 0; col < 16; col ++) {
        if (_buf[row][col] != _printed[row][col]) {
          lcd.setCursor(col, row);
          lcd.write(_buf[row][col]);

          _printed[row][col] = _buf[row][col];
        }
      }
    }

    delay(20, UPDATE_DISPLAY);

  }
  return true;
}

