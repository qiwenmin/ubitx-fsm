/*
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

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

  // 1 - Down arrow
  0b00000,
  0b00000,
  0b00000,
  0b00000,
  0b00000,
  0b11111,
  0b01110,
  0b00100,

  // 2, 3, 4 - TX
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

