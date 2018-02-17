#include "display_task.h"
#include "rig.h"
#include "objs.h"

#include <LiquidCrystal.h>
LiquidCrystal lcd(8, 9, 10, 11, 12, 13);

#define UPDATE_DISPLAY (FSM_STATE_USERDEF + 1)

void DisplayTask::init() {
  lcd.begin(16, 2);

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

