#include "ui_tasks.h"

#include "version.h"
#include "display_task.h"
#include "rig.h"
#include "objs.h"

#define ANALOG_KEYER (A6)

#define BTN_WAIT_DOWN (FSM_STATE_USERDEF + 1)
#define BTN_WAIT_UP (FSM_STATE_USERDEF + 2)

ButtonInputTask::ButtonInputTask(uint8_t pin) {
  _pin = pin;
  _state = HIGH;
  _press_at = 0;
}

void ButtonInputTask::init() {
  pinMode(_pin, INPUT_PULLUP);
  gotoState(BTN_WAIT_DOWN);
}

bool ButtonInputTask::on_state_change(int8_t, int8_t) {
  return true;
}

void ButtonInputTask::in_state(int8_t state) {
  switch (state) {
  case BTN_WAIT_DOWN:
    if (digitalRead(_pin) == LOW) {
      _press_at = millis();
      _state = LOW;
      this->delay(50, BTN_WAIT_UP);
    }
    break;

  case BTN_WAIT_UP:
    if (digitalRead(_pin) == HIGH) {
      _state = HIGH;
      this->delay(50, BTN_WAIT_DOWN);
    }
    break;

  default:
    break;
  };
}

uint8_t ButtonInputTask::getButtonState() {
  return _state;
}

unsigned long ButtonInputTask::getButtonPressAt() {
  return _press_at;
}

#define ENCODER_WAIT_NEW_VALUE (FSM_STATE_USERDEF + 1)
#define ENCODER_DEAL_NEW_VALUE (FSM_STATE_USERDEF + 2)

EncoderTask::EncoderTask(uint8_t pin_a, uint8_t pin_b) {
  _pin_a = pin_a;
  _pin_b = pin_b;

  _enc_state = _new_enc_state = 3;

  _value = _current_value = 0;

  _calc_at = millis();
}

void EncoderTask::init() {
  pinMode(_pin_a, INPUT_PULLUP);
  pinMode(_pin_b, INPUT_PULLUP);

  gotoState(ENCODER_WAIT_NEW_VALUE);
}

bool EncoderTask::on_state_change(int8_t new_state, int8_t) {
  switch (new_state) {
  case ENCODER_DEAL_NEW_VALUE:
    if (read_encoder() == _new_enc_state && _new_enc_state != _enc_state) {
      if ((_enc_state == 0 && _new_enc_state == 2) ||
        (_enc_state == 2 && _new_enc_state == 3) ||
        (_enc_state == 3 && _new_enc_state == 1) ||
        (_enc_state == 1 && _new_enc_state == 0)) {
        _current_value --;
      }
      if ((_enc_state == 0 && _new_enc_state == 1) ||
        (_enc_state == 1 && _new_enc_state == 3) ||
        (_enc_state == 3 && _new_enc_state == 2) ||
        (_enc_state == 2 && _new_enc_state == 0)) {
        _current_value ++;
      }

      _enc_state = _new_enc_state;
    }
    delay(2, ENCODER_WAIT_NEW_VALUE);
    break;

  default:
    break;
  }
  return true;
}

void EncoderTask::in_state(int8_t state) {
  if (millis() - _calc_at >= 100) {
    _value = _current_value;
    _current_value = 0;
    _calc_at = millis();
  }

  switch (state) {
  case ENCODER_WAIT_NEW_VALUE:
    _new_enc_state = read_encoder();
    if (_new_enc_state != _enc_state) {
      delay(2, ENCODER_DEAL_NEW_VALUE);
    }
    break;

  default:
    break;
  }
}

int8_t EncoderTask::get_value() {
  return _value;
}

void EncoderTask::reset_value() {
  _value = 0;
}

uint8_t EncoderTask::read_encoder() {
  return (analogRead(_pin_a) > 500 ? 1 : 0) + (analogRead(_pin_b) > 500 ? 2: 0);
}

#define MENU_WELCOME (FSM_STATE_USERDEF + 1)
#define MENU_NONE (FSM_STATE_USERDEF + 2)
#define MENU_MAIN (FSM_STATE_USERDEF + 3)
#define MENU_FREQ_ADJ_BASE (FSM_STATE_USERDEF + 4)

#define FBTN_UP 0
#define FBTN_DOWN 1
#define FBTN_DOWN_LONG 2
#define FBTN_DOWN_LONG_LONG 3

void UiTask::init() {
  _menu_idx = _submenu_idx = 0;

  _last_fbutton_state = FBTN_UP;

  _freq_adj_base = 100;

  gotoState(MENU_WELCOME);
}

bool UiTask::on_state_change(int8_t new_state, int8_t old_state) {
  _current_state = new_state;

  switch (new_state) {
  case MENU_WELCOME:
    update_display(this);
    delay(2000, MENU_NONE);
    break;
  case MENU_NONE:
    update_display(this);
    break;
  case MENU_MAIN:
    if (old_state != MENU_MAIN) {
      _menu_idx = 0;
      _submenu_idx = -1;
    }
    update_display(this);
    break;
  case MENU_FREQ_ADJ_BASE:
    update_display(this);
    break;
  default:
    break;
  }

  return true;
}

void UiTask::in_state(int8_t state) {
  bool fbtn_change = false;
  uint8_t fbtn_from_state = _last_fbutton_state;

  if (_last_fbutton_state == FBTN_UP) {
    // up to down?
    if (fbuttonTask.getButtonState() == LOW) {
      fbtn_change = true;
      _last_fbutton_state = FBTN_DOWN;
    }
  } else {
    // down to up?
    if (fbuttonTask.getButtonState() == HIGH) {
      fbtn_change = true;
      _last_fbutton_state = FBTN_UP;
    } else {
      // keep down
      unsigned long d = millis() - fbuttonTask.getButtonPressAt();

      if (d >= 800 && fbtn_from_state == FBTN_DOWN) {
        // down_long
        fbtn_change = true;
        _last_fbutton_state = FBTN_DOWN_LONG;
      } else if (d >= 1600 && fbtn_from_state == FBTN_DOWN_LONG) {
        // down_long_long
        fbtn_change = true;
        _last_fbutton_state = FBTN_DOWN_LONG_LONG;
      }
    }
  }
  
  if (fbtn_change) {
    if (_last_fbutton_state == FBTN_UP) {
      if (_current_state == MENU_MAIN) {
        gotoState(MENU_NONE);
      } else if (_current_state == MENU_NONE) {
        if (fbtn_from_state != FBTN_DOWN_LONG_LONG) {
          gotoState(MENU_MAIN);
        }
      } else if (_current_state == MENU_FREQ_ADJ_BASE) {
        if (fbtn_from_state != FBTN_DOWN_LONG) {
          gotoState(MENU_NONE);
        }
      }
    } else if (_last_fbutton_state == FBTN_DOWN_LONG) {
      if (_current_state == MENU_NONE && rig.isVfo()) {
        gotoState(MENU_FREQ_ADJ_BASE);
      }
    } else if (_last_fbutton_state == FBTN_DOWN_LONG_LONG) {
      rig.setDialLock(rig.getDialLock() == ON ? OFF : ON);
      gotoState(MENU_NONE);
    }
  }

  int8_t enc_val = encoderTask.get_value();
  encoderTask.reset_value();

  if (_last_fbutton_state != FBTN_UP) {
    enc_val = 0;
  }

  bool need_update = false;

  switch (state) {
  case MENU_NONE:
    if (rig.getDialLock() != ON && enc_val != 0) {
      if (rig.isVfo()) {
        int8_t times = enc_val > 0 ? enc_val : -enc_val;
        if (times < 3) times = 1;
        else times -= 2;

        rig.setFreq(rig.getFreq() + enc_val * times * _freq_adj_base);
        update_display(this);
      }
    }
    break;

  case MENU_MAIN:
    if (enc_val != 0) {
      if (enc_val > 1) {
        _menu_idx ++;
        if (_menu_idx > 8) _menu_idx = 8;
        else need_update = true;
      } else if (enc_val < -1) {
        _menu_idx --;
        if (_menu_idx < 0) _menu_idx = 0;
        else need_update = true;
      }

      if (need_update) {
        update_display(this);
        delay(400, MENU_MAIN);
      }
    }
    break;

  case MENU_FREQ_ADJ_BASE:
    if (enc_val != 0) {
      if (enc_val > 1) {
        if (_freq_adj_base > 10) {
          _freq_adj_base /= 10;
          need_update = true;
        }
      } else if (enc_val < -1) {
        if (_freq_adj_base < 1000000) {
          _freq_adj_base *= 10;
          need_update = true;
        }
      }

      if (need_update) {
        update_display(this);
        delay(400, MENU_FREQ_ADJ_BASE);
      }
    }
    break;
  default:
    break;
  }
}

void UiTask::update_display(void */*sender*/) {
  switch (_current_state) {
  case MENU_WELCOME:
    displayTask.clear();
    displayTask.print0("=[ uBitx  FMS ]=");
    displayTask.print1("     v." FW_VERSION "     ");
    break;
  case MENU_MAIN:
    displayTask.clear();
    update_rig_display();
    update_menu_display();
    break;
  case MENU_FREQ_ADJ_BASE:
    displayTask.clear();
    update_rig_display();
    update_freq_adj_base();
    break;
  default:
    displayTask.clear();
    update_rig_display();
    break;
  }
}

void UiTask::update_rig_display() {
  char _buf[17];

  // freq
  int32_t freq = rig.getFreq();
  sprintf(_buf, "%2" PRIu32 ".%05" PRIi32, freq / 1000000, (freq % 1000000) / 10);
  displayTask.print(8, 1, _buf);

  // mode
  _print_rig_mode(rig.getMode(), 4, 1);

  // SPLIT?
  displayTask.print(8, 0, rig.getSplit() == ON ? "SPL" : "   ");

  // Lock?
  displayTask.print(4, 0, rig.getDialLock() == ON ? "LCK" : "   ");

  // VFO? MEM?
  displayTask.print(0, 1, rig.isVfo() ? "V-" : "M-");

  // VFO A? B?
  displayTask.print(2, 1, rig.getVfo() == VFO_A ? "A" : "B");

  // Mem OK?
  displayTask.print(0, 0, rig.isMemOk() ? "M" : "?");

  // Ch#
  sprintf(_buf, "%02d", rig.getMemCh());
  displayTask.print(1, 0, _buf);

  // TX?
  displayTask.print(14, 0, rig.getTx() == ON ? "TX" : "  ");
}

void UiTask::update_menu_display() {
  displayTask.print0("             "); // clear the first 13 chars
  switch (_menu_idx) {
  case 0:
    displayTask.print0("0.Mode: ");
    _print_rig_mode(rig.getMode(), 8, 0);
    break;
  case 1:
    displayTask.print0("1.A/B");
    break;
  case 2:
    displayTask.print0("2.A=B");
    break;
  case 3:
    displayTask.print0("3.Split: ");
    displayTask.print(9, 0, rig.getSplit() == ON ? "Y" : "N");
    break;
  case 4:
    displayTask.print0("4.V/M");
    break;
  case 5:
    displayTask.print0("5.M\x7eV");
    break;
  case 6:
    displayTask.print0("6.MW");
    break;
  case 7:
    displayTask.print0("7.MC");
    break;
  case 8:
    displayTask.print0("8.Exit Menu");
    break;
  default:
    displayTask.print0("###ERR###");
    break;
  }
}

void UiTask::update_freq_adj_base() {
  displayTask.print(8, 0, "        ");

  uint8_t p = 10;

  if (_freq_adj_base == 10) p = 15;
  else if (_freq_adj_base == 100) p = 14;
  else if (_freq_adj_base == 1000) p = 13;
  else if (_freq_adj_base == 10000) p = 12;
  else if (_freq_adj_base == 100000) p = 11;
  else if (_freq_adj_base == 1000000) p = 9;

  displayTask.print(p, 0, "_");
}

void UiTask::_print_rig_mode(uint8_t mode, uint8_t col, uint8_t row) {
  switch (mode) {
  case MODE_CW:
    displayTask.print(col, row, "CW ");
    break;
  case MODE_CWR:
    displayTask.print(col, row, "CWR");
    break;
  case MODE_LSB:
    displayTask.print(col, row, "LSB");
    break;
  case MODE_USB:
    displayTask.print(col, row, "USB");
    break;
  }
}

