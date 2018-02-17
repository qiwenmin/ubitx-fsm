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

#include <avr/pgmspace.h>

#include "ui_tasks.h"
#include "menu.h"
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

  Menu_Item mi;
  if (_current_state == MENU_MAIN) {
    memcpy_P(&mi, &menu[_menu_idx], sizeof(mi));
  }

  if (fbtn_change) {
    if (_last_fbutton_state == FBTN_UP) {
      if (_current_state == MENU_MAIN) {        
        if (mi.submenu_count != 0 && _submenu_idx == -1) {
          if (mi.get_menu_value_f != NULL) {
            _submenu_idx = (*mi.get_menu_value_f)();
          }

          update_display(this);
        } else {
          if (mi.select_menu_f != NULL) {
            (*mi.select_menu_f)(_submenu_idx);
          }
          gotoState(MENU_NONE);
        }
      } else if (_current_state == MENU_NONE) {
        if (fbtn_from_state == FBTN_DOWN) {
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
      } else if (_current_state == MENU_MAIN) {
        gotoState(MENU_NONE);
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
  int8_t d_idx = 0;

  switch (state) {
  case MENU_NONE:
    if (rig.getDialLock() != ON && enc_val != 0) {
      if (rig.isVfo()) {
        int8_t times = enc_val > 0 ? enc_val : -enc_val;
        if (times < 3) times = 1;
        else times -= 2;

        rig.setFreq(rig.getFreq() + enc_val * times * _freq_adj_base, false);
        update_display(this);
      } else {
        if (enc_val > 1) {
          int8_t ch = rig.getNextMemOkCh(rig.getMemCh());
          if (ch != -1) {
            rig.selectMemCh(ch, false);
            rig.selectMem(false);
            need_update = true;
          }
        } else if (enc_val < -1) {
          int8_t ch = rig.getPrevMemOkCh(rig.getMemCh());
          if (ch != -1) {
            rig.selectMemCh(ch, false);
            rig.selectMem(false);
            need_update = true;
          }
        }

        if (need_update) {
          update_display(this);
          delay(400, MENU_NONE);
        }
      }
    }
    break;

  case MENU_MAIN:
    if (enc_val != 0) {
      if (enc_val > 1) {
        d_idx = 1;
      } else if (enc_val < -1) {
        d_idx = -1;
      }

      if (d_idx != 0) {
        if (_submenu_idx == -1) {
          _menu_idx += d_idx;
          if (_menu_idx >= menu_item_count) _menu_idx = menu_item_count - 1;
          else if (_menu_idx < 0) _menu_idx = 0;
          else need_update = true;
        } else {
          if (mi.get_next_menu_value_f != NULL) {
            _submenu_idx = (*mi.get_next_menu_value_f)(_submenu_idx, d_idx > 0);
          } else {
            _submenu_idx += d_idx;
            if (_submenu_idx == mi.submenu_count) _submenu_idx = 0;
            else if (_submenu_idx == -1) _submenu_idx = mi.submenu_count - 1;
          }
          need_update = true;
        }
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

  freq = rig.getFreqAnother();
  sprintf(_buf, "%2" PRIu32 ".%05" PRIi32, freq / 1000000, (freq % 1000000) / 10);
  displayTask.print(8, 0, _buf);

  // mode
  format_mode(_buf, rig.getMode());
  displayTask.print(4, 1, _buf);

  format_mode(_buf, rig.getModeAnother());
  displayTask.print(4, 0, _buf);

  // SPLIT?
  displayTask.print(2, 0, rig.getSplit() == ON ? "S" : " ");

  // Lock?
  displayTask.print(0, 0, rig.getDialLock() == ON ? '\x00' : ' ');

  // VFO? MEM?
  // Ch#
  sprintf(_buf, "%02d", rig.getMemCh());
  displayTask.print(0, 1, rig.isVfo() ? "V-" : _buf);

  // VFO A? B?
  displayTask.print(2, 1, rig.getVfo() == VFO_A ? "A" : "B");

  // TX?
  displayTask.print(14, 0, rig.getTx() == ON ? "\x04\x05" : "");
}

void UiTask::update_menu_display() {
  char menu_text[12], menu_value_text[6], menu_fulltext[14];
  char n = ' ';
  int8_t v = _submenu_idx;

  Menu_Item mi;
  memcpy_P(&mi, &menu[_menu_idx], sizeof(mi));

  if (_menu_idx < 10) {
    n = '0' + _menu_idx;
  } else {
    n = 'a' + (_menu_idx - 10);
  }

  if (mi.format_menu_f != NULL) {
    (*mi.format_menu_f)(menu_text);
  } else {
    sprintf(menu_text, mi.text);
  }

  if (v == -1 && mi.get_menu_value_f != NULL) {
    v = (*mi.get_menu_value_f)();
  }

  if (mi.format_menu_value_f != NULL) {
    (*mi.format_menu_value_f)(menu_value_text, v);
  } else {
    sprintf(menu_value_text, "%02d", v);
  }

  if (mi.submenu_count == 0) {
    sprintf(menu_fulltext, "%c.%s", n, menu_text);
  } else {
    if (_submenu_idx == -1) {
      sprintf(menu_fulltext, "%c.%s: %s", n, menu_text, menu_value_text);
    } else {
      sprintf(menu_fulltext, "%c.%s:\x01%s\x02", n, menu_text, menu_value_text);
    }
  }

  displayTask.print0("                "); // clear the first line
  displayTask.print0(menu_fulltext);
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

  displayTask.print(p, 0, "\x03");
}

void UiTask::format_mode(char *output, uint8_t mode) {
  switch (mode) {
  case MODE_CW:
    sprintf(output, "CW");
    break;
  case MODE_CWR:
    sprintf(output, "CWR");
    break;
  case MODE_LSB:
    sprintf(output, "LSB");
    break;
  case MODE_USB:
    sprintf(output, "USB");
    break;
  default:
    sprintf(output, "N/A");
    break;
  }
}

