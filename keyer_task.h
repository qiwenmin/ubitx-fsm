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

#ifndef __KEYER_H__
#define __KEYER_H__

#include <fsmos.h>

#include "rig.h"
#include "objs.h"

#define PADDLE_NONE 0
#define PADDLE_DOT 1
#define PADDLE_DASH 2
#define PADDLE_BOTH 3
#define PADDLE_STRAIGHT 4

#define KEY_READY (FSM_STATE_USERDEF + 1)
#define KEY_AUTOTEXT (FSM_STATE_USERDEF + 2)

#define ET_IDLE (0)
#define ET_DOT (1)
#define ET_DASH (2)
#define ET_IG (3)

#define SS_IDLE (0)
#define SS_CH (1)
#define SS_ICG (2)
#define SS_IWG (3)

class KeyerTask : public FsmTask {
public:
  KeyerTask(uint8_t pin) {
    _pin = pin;
    _is_key_down = false;
    _key_up_at = 0;

    _autotext_mode = false;

    _element_at = 0;
    _element_type = ET_IDLE;
    _expect_key = _next_key = PADDLE_NONE;

    _sending_state = SS_IDLE;
    _sending_ch_idx = 0;
    _sending_m = 0x80;
    _wait_paddle_release = false;
  };

  virtual void init() {
    pinMode(_pin, INPUT_PULLUP);

    gotoState(KEY_READY);
  };

  virtual bool on_state_change(int8_t new_state, int8_t) {
    _autotext_mode = (new_state == KEY_AUTOTEXT);

    if (_is_key_down) keyUp();

    switch (new_state) {
    case KEY_READY:
      _element_type = ET_IDLE;
      _expect_key = _next_key = PADDLE_NONE;
      break;
    case KEY_AUTOTEXT:
      _sending_state = SS_IDLE;
      _sending_ch_idx = 0;
      _wait_paddle_release = false;
      break;
    default:
      break;
    }
    return true;
  };

  virtual void in_state(int8_t state) {
    if (_disabled) return;

    if (rig.getTxMode() != MODE_CW && rig.getTxMode() != MODE_CWR) return;

    if ((!_is_key_down) && rig.getTx() == ON && millis() - _key_up_at >= Device::getCwDelay()) {
      rig.setTx(OFF);
    }

    if (state == KEY_READY) {
      in_ready_state();
    } else if (state == KEY_AUTOTEXT) {
      in_autotext_state();
    }
  };

  void setDisabled(bool disabled) {
    _disabled = disabled;
  };

  bool isAutoTextMode() {
    return _autotext_mode;
  };

  void setAutoTextMode(bool atm) {
    if (atm) {
      gotoState(KEY_AUTOTEXT);
    } else {
      gotoState(KEY_READY);
    }
  };

private:
  uint8_t _pin;
  bool _disabled;
  bool _autotext_mode;

  uint8_t getPaddle() {
    int paddle = analogRead(_pin);

    if (paddle > 800) return PADDLE_NONE;
    if (paddle > 600) return PADDLE_DASH;
    if (paddle > 300) return PADDLE_DOT;
    if (paddle > 50) return PADDLE_BOTH;
    return PADDLE_STRAIGHT;
  };


  bool _is_key_down;

  void keyDown() {
    uint8_t mode = rig.getTxMode();
    if (mode != MODE_CW && mode != MODE_CWR) return;

    if (rig.getTx() != ON) {
      rig.setTx(ON);
    }

    _is_key_down = true;
    Device::cwKeyDown();
  };

  void keyUp() {
    _is_key_down = false;
    Device::cwKeyUp();

    _key_up_at = millis();
  };


  unsigned long _key_up_at;
  unsigned long _element_at;
  uint8_t _element_type;
  uint8_t _expect_key, _next_key;

  void in_ready_state(uint8_t paddle_key = PADDLE_NONE) {
    uint8_t cwKey = Device::getCwKey();
    uint8_t k = getPaddle();

    if (paddle_key != PADDLE_NONE) {
      // simulate paddle
      cwKey = CW_KEY_IAMBIC_A_R;
      _next_key = PADDLE_NONE;
      k = paddle_key;
    }

    if (cwKey== CW_KEY_STRAIGHT) {
      if (!_is_key_down) {
        if (k == PADDLE_DOT || k == PADDLE_BOTH || k == PADDLE_STRAIGHT) {
          keyDown();
        }
      } else {
        if (k == PADDLE_NONE || k == PADDLE_DASH) {
          keyUp();
        }
      }
    } else {
      // iambic a/b l/r
      if (cwKey == CW_KEY_IAMBIC_A_L || cwKey == CW_KEY_IAMBIC_B_L) {
        if (k == PADDLE_DOT) k = PADDLE_DASH;
        else if (k == PADDLE_DASH) k = PADDLE_DOT;
      }

      switch (_element_type) {
      case ET_IDLE:
        if (cwKey == CW_KEY_IAMBIC_B_L || cwKey == CW_KEY_IAMBIC_B_R) {
          // Iambic B
          if (_next_key != PADDLE_NONE) {
            k = _next_key;
          }
        }

        _next_key = PADDLE_NONE;

        if (k == PADDLE_BOTH) {
          if (_expect_key != PADDLE_NONE) {
            k = _expect_key;
          } else {
            k = PADDLE_DASH; // Dash first
          }
          _next_key = k == PADDLE_DASH ? PADDLE_DOT : PADDLE_DASH;
        }

        if (k == PADDLE_DASH) {
          _expect_key = PADDLE_DOT;
          _element_type = ET_DASH;
          _element_at = millis();
          keyDown();
        } else if (k == PADDLE_DOT) {
          _expect_key = PADDLE_DASH;
          _element_type = ET_DOT;
          _element_at = millis();
          keyDown();
        } else {
          _expect_key = PADDLE_NONE;
        }
        break;
      case ET_DOT:
        if (k == PADDLE_BOTH || k == _expect_key) _next_key = _expect_key;
        if (millis() - _element_at >= Device::getCwSpeed()) {
          _element_type = ET_IG;
          _element_at = millis();
          keyUp();
        }
        break;
      case ET_DASH:
        if (k == PADDLE_BOTH || k == _expect_key) _next_key = _expect_key;
        if (millis() - _element_at >= Device::getCwSpeed() * 3) {
          _element_type = ET_IG;
          _element_at = millis();
          keyUp();
        }
        break;
      case ET_IG:
        if (k == PADDLE_BOTH || k == _expect_key) _next_key = _expect_key;
        if (millis() - _element_at >= Device::getCwSpeed()) {
          _element_type = ET_IDLE;
        }
        break;
      default:
        break;
      }
    }
  };


  uint8_t _sending_m;
  uint8_t _sending_state;
  uint8_t _sending_ch_idx;
  bool _wait_paddle_release;

  void in_autotext_state() {
    if (_wait_paddle_release) {
      if (getPaddle() == PADDLE_NONE) {
        _wait_paddle_release = false;
        setAutoTextMode(false);
      }
      return;
    }

    if (getPaddle() != PADDLE_NONE) {
      if (_is_key_down) keyUp();

      _wait_paddle_release = true;
      return;
    }

    static const uint8_t morse_table[] PROGMEM = {
      0b10101110, // ! _._.__
      0b01001010, // " ._.._.
      0b10000000, // # ??????
      0b00010011, // $ ..._.._
      0b10000000, // % ??????
      0b01000100, // & ._...
      0b01111010, // ' .____.
      0b10110100, // ( _.__.
      0b10110110, // ) _.__._
      0b10000000, // * ??????
      0b01010100, // + ._._.
      0b11001110, // , __..__
      0b10000110, // - _...._
      0b01010110, // . ._._._
      0b10010100, // / _.._.
      0b11111100, // 0 _____
      0b01111100, // 1 .____
      0b00111100, // 2 ..___
      0b00011100, // 3 ...__
      0b00001100, // 4 ...._
      0b00000100, // 5 .....
      0b10000100, // 6 _....
      0b11000100, // 7 __...
      0b11100100, // 8 ___..
      0b11110100, // 9 ____.
      0b11100010, // : ___...
      0b10101010, // ; _._._.
      0b10000000, // < ??????
      0b10001100, // = _..._
      0b10000000, // > ??????
      0b00110010, // ? ..__..
      0b01101010, // @ .__._.
      0b01100000, // A ._
      0b10001000, // B _...
      0b10101000, // C _._.
      0b10010000, // D _..
      0b01000000, // E .
      0b00101000, // F .._.
      0b11010000, // G __.
      0b00001000, // H ....
      0b00100000, // I ..
      0b01111000, // J .___
      0b10110000, // K _._
      0b01001000, // L ._..
      0b11100000, // M __
      0b10100000, // N _.
      0b11110000, // O ___
      0b01101000, // P .__.
      0b11011000, // Q __._
      0b01010000, // R ._.
      0b00010000, // S ...
      0b11000000, // T _
      0b00110000, // U .._
      0b00011000, // V ..._
      0b01110000, // W .__
      0b01101000, // X .__.
      0b10111000, // Y _.__
      0b11001000, // Z __..

      0b00110110 // _ ..__._
    };

    static const uint8_t under_score_index = sizeof(morse_table) - 1;

    char ch;
    int8_t m_idx;

    switch (_sending_state) {
    case SS_IDLE:
      rig.getAutokeyTextCh(_sending_ch_idx, ch);
      _sending_ch_idx ++;

      if (ch == 0) {
        // end of the text
        _sending_ch_idx = 0;
        setAutoTextMode(false);
      } else {

        if (ch >= 'a' && ch <= 'z') ch = ch - 'a' + 'A';

        m_idx = -1;
        if (ch >= '!' && ch <= 'Z') {
          m_idx = ch - '!';
        } else if (ch == '_') {
          m_idx = under_score_index;
        }

        if (m_idx != -1) {
          memcpy_P(&_sending_m, &morse_table[m_idx], 1);
        }

        if (m_idx == -1 || _sending_m == 0x80) {
          // unknown char, or space
          _element_at = millis();
          _sending_state = SS_IWG;
        } else {
          _element_type = ET_IDLE;
          _sending_state = SS_CH;
        }
      }
      break;
    case SS_CH:
      if (_sending_m == 0x80) {
        // finished char!
        _element_at = millis();
        _sending_state = SS_ICG;
      } else {
        in_ready_state(_sending_m & 0x80 ? PADDLE_DASH : PADDLE_DOT);

        if (_element_type == ET_IDLE) {
          // finished dot/dash
          _sending_m <<= 1;
        }
      }
      break;
    case SS_ICG:
      if (millis() - _element_at >= Device::getCwSpeed() * 2) {
        _sending_state = SS_IDLE;
      }
      break;
    case SS_IWG:
      if (millis() - _element_at >= Device::getCwSpeed() * 4) {
        _sending_state = SS_IDLE;
      }
      break;
    default:
      break;
    }
  };
};

#endif // __KEYER_H__
