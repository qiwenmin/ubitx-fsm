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

#ifndef __DISPLAY_TASK_H__
#define __DISPLAY_TASK_H__

#include <fsmos.h>

class DisplayTask : public FsmTask {
public:
  DisplayTask() {
    memset(_buf, ' ', 32);
    memset(_printed, ' ', 32);
  };

  virtual void init();

  virtual bool on_state_change(int8_t, int8_t);

  virtual void in_state(int8_t) {};

  void clear() {
    memset(_buf, ' ', 32);
  };

  void clear0() {
    memset(&(_buf[0][0]), ' ', 16);
  };

  void clear1() {
    memset(&(_buf[1][0]), ' ', 16);
  };

  void print0(const char *str) { print(0, str); };
  void print1(const char *str) { print(1, str); };
  void print(uint8_t row, const char *str) { print(0, row, str); };
  void print(uint8_t col, uint8_t row, const char *str) {
    uint8_t c = col & 0x0F;
    uint8_t r = row & 0x01;

    for (uint8_t i = c; i < 16; i ++) {
      if (str[i - c] != '\0') {
        _buf[r][i] = str[i - c];
      } else {
        break;
      }
    }
  };

  void print(uint8_t col, uint8_t row, char ch) {
    _buf[row & 0x01][col & 0x0F] = ch;
  }

private:
  char _buf[2][16];
  char _printed[2][16];

  void updateDisplay();
};

#endif // __DISPLAY_TASK_H__
