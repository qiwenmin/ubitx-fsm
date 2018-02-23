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

#include "ui_tasks.h"
#include "menu.h"
#include "version.h"
#include "display_task.h"
#include "keyer_task.h"
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

uint8_t ButtonInputTask::getRawState() {
  return digitalRead(_pin);
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
#define MENU_SYSTEM (FSM_STATE_USERDEF + 5)

#define FBTN_UP 0
#define FBTN_DOWN 1
#define FBTN_DOWN_LONG 2
#define FBTN_DOWN_LONG_LONG 3

void UiTask::init() {
  _menu_idx = 0;
  _menu_val = -1;
  _menu_change_val = false;

  _last_fbutton_state = FBTN_UP;
  _last_ptt_state = FBTN_UP;

  _last_tx = OFF;
  _tx_flashing_at = 0;
  _save_vfo_ch_at = millis();

  _ptt_to_rx = false;

  rig.init();

  gotoState(MENU_WELCOME);
}

void UiTask::gotoSysMenu() {
  _menu_idx = 0; _menu_val = -1; _menu_change_val = false;

  displayTask.clear();
  displayTask.print(0, 0, F(" System Menu... "));
  delay(2000, MENU_SYSTEM);
}

bool UiTask::isMenuMode() {
  return _current_state == MENU_MAIN;
}

bool UiTask::on_state_change(int8_t new_state, int8_t old_state) {
  _current_state = new_state;

  switch (new_state) {
  case MENU_WELCOME:
    if (fbuttonTask.getRawState() == LOW) {
      rig.serialSetup();
    } else {
      update_display(this);
      delay(2000, MENU_NONE);
    }
    break;
  case MENU_NONE:
    update_display(this);
    break;
  case MENU_MAIN:
  case MENU_SYSTEM:
    keyerTask.clearChar();
    if (new_state == MENU_SYSTEM) {
      active_system_menu();
      displayTask.clear1();
    } else {
      active_main_menu();
    }
    if (old_state != MENU_MAIN && old_state != MENU_SYSTEM) {
      _menu_idx = 0;
      _menu_val = -1;
      _menu_change_val = false;
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
  // read inputs
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

  int8_t enc_val = encoderTask.get_value();
  encoderTask.reset_value();

  if (_last_fbutton_state != FBTN_UP) {
    enc_val = 0;
  }

  // Calibrate?
  if (state == MENU_SYSTEM) {
    in_state_menu_main(fbtn_change, fbtn_from_state, enc_val);
    return;
  }

  // ptt?
  if (rig.getTxMode() == MODE_LSB || rig.getTxMode() == MODE_USB) {
    if (pttTask.getButtonState() == HIGH && rig.getTx() == ON && _ptt_to_rx) {
      rig.setTx(OFF);
      _ptt_to_rx = false;
    }

    if (pttTask.getButtonState() == LOW) {
      _ptt_to_rx = true;

      if (rig.getTx() == OFF) {
        rig.setTx(ON);
      }
    }
  } else {
    if (pttTask.getButtonState() == HIGH) {
      if (_last_ptt_state == FBTN_DOWN) {
        if (keyerTask.setAutoTextMode(!keyerTask.isAutoTextMode())) {
          gotoState(MENU_NONE);
        }
      }
      _last_ptt_state = FBTN_UP;
    } else {
      _last_ptt_state = FBTN_DOWN;
    }
  }

  // TX?
  if (rig.getTx() == ON) {
    if (_current_state != MENU_NONE) gotoStateForce(MENU_NONE);

    if (_last_tx == OFF) {
      _tx_flashing_at = millis();
      _last_tx = ON;
    }

    displayTask.print(0, 0, ((millis() - _tx_flashing_at) / 500) & 0x01 ? '\x02' : '\x03');
    return;
  }

  _last_tx = OFF;

  // Save VFO Ch every 10 seconds
  if (millis() - _save_vfo_ch_at > 10000) {
    rig.saveVfoCh();
    _save_vfo_ch_at = millis();
  }

  // cw key inputs?
  if (state == MENU_MAIN) {
    char ch;
    if (keyerTask.getChar(ch)) {
      keyerTask.clearChar();

      if (ch == 'E' || ch == 'T') {
        if (keyerTask.setAutoTextMode(true)) {
          gotoState(MENU_NONE);
        }
      }
    }
  }

  // long long press --> dial lock and goto menu_none
  if (fbtn_change && (_last_fbutton_state == FBTN_DOWN_LONG_LONG)) {
    rig.setDialLock(rig.getDialLock() == ON ? OFF : ON);
    gotoState(MENU_NONE);
    return;
  }

  switch (state) {
  case MENU_NONE:
    in_state_menu_none(fbtn_change, fbtn_from_state, enc_val);
    break;
  case MENU_MAIN:
    in_state_menu_main(fbtn_change, fbtn_from_state, enc_val);
    break;
  case MENU_FREQ_ADJ_BASE:
    in_state_menu_freq_adj_base(fbtn_change, fbtn_from_state, enc_val);
    break;
  default:
    break;
  }
}

void UiTask::in_state_menu_main(bool fbtn_change, uint8_t fbtn_from_state, int8_t enc_val) {
  Menu_Item mi;
  memcpy_P(&mi, &menu[_menu_idx], sizeof(mi));

  if (fbtn_change) {
    if (_last_fbutton_state == FBTN_UP && fbtn_from_state == FBTN_DOWN) {
      if (mi.submenu_count != 0 && (!_menu_change_val)) {
        if (mi.get_menu_value_f != NULL) {
          _menu_val = (*mi.get_menu_value_f)();
          _menu_change_val = true;
        }

        update_display(this);
      } else {
        if (mi.select_menu_f != NULL) {
          if ((*mi.select_menu_f)(_menu_val, true)) {
            gotoState(MENU_NONE);
          } else {
            _menu_val = -1;
            _menu_change_val = false;

            update_display(this);
          }
        } else {
          gotoState(MENU_NONE);
        }
      }
    } else if (_last_fbutton_state == FBTN_DOWN_LONG) {
      if (_current_state == MENU_MAIN) gotoState(MENU_NONE);
      else if (_current_state == MENU_SYSTEM) {
        if (_menu_change_val) {
          if (mi.select_menu_f != NULL) {
            (*mi.select_menu_f)(_menu_val, false);
          }
          _menu_change_val = false;
          _menu_val = -1;
          update_display(this);
        }
      }
    }
  } else if (enc_val != 0) {
    bool need_update = false;
    int8_t d_idx = 0;

    if (enc_val > 1) {
      d_idx = 1;
    } else if (enc_val < -1) {
      d_idx = -1;
    }

    if (enc_val != 0) {
      if (!_menu_change_val) {
        if (d_idx != 0) {
          _menu_idx += d_idx;
          if (_menu_idx >= menu_item_count) _menu_idx = menu_item_count - 1;
          else if (_menu_idx < 0) _menu_idx = 0;
          else need_update = true;
        }
      } else {
        if (mi.get_next_menu_value_f != NULL) {
          if (d_idx != 0) {
            _menu_val = (*mi.get_next_menu_value_f)(_menu_val, d_idx > 0);
          }
        } else {
          if (mi.submenu_count > 0) {
            if (d_idx != 0) {
              _menu_val += d_idx;

              if (_menu_val == mi.submenu_count) _menu_val = 0;
              else if (_menu_val == -1) _menu_val = mi.submenu_count - 1;
            }
          } else if (mi.submenu_count == -1) {
            int8_t times = enc_val >= 0 ? enc_val : -enc_val;
            if (times < 3) times = 1;
            else times -= 2;

            _menu_val += (enc_val * times);
          }
        }
        need_update = true;
      }
    }

    if (need_update) {
      update_display(this);
      delay(400, _current_state);
    }
  }
}

void UiTask::in_state_menu_none(bool fbtn_change, uint8_t fbtn_from_state, int8_t enc_val) {
  if (fbtn_change) {
    if (_last_fbutton_state == FBTN_UP) {
      if (_current_state == MENU_NONE) {
        if (fbtn_from_state == FBTN_DOWN) {
          gotoState(MENU_MAIN);
        }
      }
    } else if (_last_fbutton_state == FBTN_DOWN_LONG) {
      if (_current_state == MENU_NONE && rig.isVfo()) {
        gotoState(MENU_FREQ_ADJ_BASE);
      }
    }
  } else if (enc_val != 0 && rig.getDialLock() != ON) {
    if (rig.isVfo()) {
      int8_t times = enc_val > 0 ? enc_val : -enc_val;
      if (times < 3) times = 1;
      else times -= 2;

      rig.setFreq(rig.getFreq() + enc_val * times * rig.getFreqAdjBase(), false);
      update_display(this);
    } else {
      bool need_update = false;

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
}

void UiTask::in_state_menu_freq_adj_base(bool fbtn_change, uint8_t fbtn_from_state, int8_t enc_val) {
  if (fbtn_change && _last_fbutton_state == FBTN_UP && fbtn_from_state != FBTN_DOWN_LONG) {
    gotoState(MENU_NONE);
  } else if (enc_val != 0) {
    bool need_update = false;
    int32_t base = rig.getFreqAdjBase();

    if (enc_val > 1) {
      if (base > 10) {
        base /= 10;
        rig.setFreqAdjBase(base);
        need_update = true;
      }
    } else if (enc_val < -1) {
      if (base < 1000000) {
        base *= 10;
        rig.setFreqAdjBase(base);
        need_update = true;
      }
    }

    if (need_update) {
      update_display(this);
      delay(400, MENU_FREQ_ADJ_BASE);
    }
  }
}

void UiTask::update_display(void */*sender*/) {
  char callsign[16];

  switch (_current_state) {
  case MENU_WELCOME:
    displayTask.clear();
    rig.getCallsign(callsign);
    if (callsign[0] != 0) {
      displayTask.print0(F("uBitx FMS v." FW_VERSION));
      uint8_t x = (16 - strlen(callsign)) / 2;
      displayTask.clear1();
      displayTask.print(x, 1, callsign);
    } else {
      displayTask.print0(F("=[ uBitx  FMS ]="));
      displayTask.print1(F("     v." FW_VERSION "     "));
    }
    break;
  case MENU_MAIN:
    displayTask.clear();
    update_rig_display();
    update_menu_display();
    break;
  case MENU_SYSTEM:
    displayTask.clear0();
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
  displayTask.print(2, 0, rig.getSplit() == ON ? F("S") : F(" "));

  // Lock?
  displayTask.print(0, 0, rig.getDialLock() == ON ? '\x00' : ' ');

  // VFO? MEM?
  // Ch#
  sprintf(_buf, "%02d", rig.getMemCh());
  displayTask.print(0, 1, rig.isVfo() ? "V-" : _buf);

  // VFO A? B?
  displayTask.print(2, 1, rig.getVfo() == VFO_A ? F("A") : F("B"));

  // TX?
  displayTask.print(0, 0, rig.getTx() == ON ? F("\x03\x04") : F(""));
}

void UiTask::update_menu_display() {
  char menu_text[15], menu_value_text[13], menu_fulltext[15];
  char n = ' ';
  int16_t v = _menu_val;

  Menu_Item mi;
  memcpy_P(&mi, &menu[_menu_idx], sizeof(mi));

  if (_menu_idx < 10) {
    n = '0' + _menu_idx;
  } else {
    n = 'a' + (_menu_idx - 10);
  }

  if (mi.format_menu_f != NULL) {
    (*mi.format_menu_f)(menu_text, mi.text, _menu_change_val, _menu_val);
  } else {
    sprintf(menu_text, mi.text);
  }

  if (!_menu_change_val && mi.get_menu_value_f != NULL) {
    v = (*mi.get_menu_value_f)();
  }

  if ((mi.submenu_count == 0) || ((!_menu_change_val) && (mi.format_menu_f != NULL))) {
    sprintf(menu_fulltext, "%c.%s", n, menu_text);
  } else {
    if (mi.format_menu_value_f != NULL) {
      (*mi.format_menu_value_f)(menu_value_text, v);
    } else {
      sprintf(menu_value_text, "%02d", v);
    }

    if (!_menu_change_val) {
      sprintf(menu_fulltext, "%c.%s:%s", n, menu_text, menu_value_text);
    } else {
      sprintf(menu_fulltext, "%c.%s\xa2%s\xa3", n, menu_text, menu_value_text);
    }
  }

  displayTask.clear0();
  displayTask.print0(menu_fulltext);
}

void UiTask::update_freq_adj_base() {
  displayTask.print(8, 0, F("        "));

  uint8_t p = 10;

  int32_t base = rig.getFreqAdjBase();

  if (base == 10) p = 15;
  else if (base == 100) p = 14;
  else if (base == 1000) p = 13;
  else if (base == 10000) p = 12;
  else if (base == 100000) p = 11;
  else if (base == 1000000) p = 9;

  displayTask.print(p, 0, "\x01");
}

void UiTask::format_mode(char *output, uint8_t mode) {
  switch (mode) {
  case MODE_CW:
    strcpy_P(output, PSTR("CW"));
    break;
  case MODE_CWR:
    strcpy_P(output, PSTR("CWR"));
    break;
  case MODE_LSB:
    strcpy_P(output, PSTR("LSB"));
    break;
  case MODE_USB:
    strcpy_P(output, PSTR("USB"));
    break;
  default:
    strcpy_P(output, PSTR("N/A"));
    break;
  }
}

