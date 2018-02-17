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
