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

#define PADDLE_NONE 0
#define PADDLE_DOT 1
#define PADDLE_DASH 2
#define PADDLE_BOTH 3
#define PADDLE_STRAIGHT 4

#define KEY_READY (FSM_STATE_USERDEF + 1)

#define ET_IDLE (0)
#define ET_DOT (1)
#define ET_DASH (2)
#define ET_IG (3)

class KeyerTask : public FsmTask {
public:
  KeyerTask(uint8_t pin) {
    _pin = pin;
    _is_key_down = false;
    _key_up_at = 0;

    _element_at = 0;
    _element_type = ET_IDLE;
    _expect_key = _next_key = PADDLE_NONE;
  };

  virtual void init() {
    pinMode(_pin, INPUT_PULLUP);

    gotoState(KEY_READY);
  };

  virtual bool on_state_change(int8_t, int8_t) { return true; };

  virtual void in_state(int8_t state) {
    if (_disabled) return;

    if (state == KEY_READY) {
      if ((!_is_key_down) && rig.getTx() == ON && millis() - _key_up_at >= Device::getCwDelay()) {
        rig.setTx(OFF);
      }

      uint8_t k = getPaddle();

      if (Device::getCwKey() == CW_KEY_STRAIGHT) {
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
        uint8_t cwKey = Device::getCwKey();
        if (cwKey == CW_KEY_IAMBIC_A_L || cwKey == CW_KEY_IAMBIC_B_L) {
          if (k == PADDLE_DOT) k = PADDLE_DASH;
          else if (k == PADDLE_DASH) k = PADDLE_DOT;
        }

        switch (_element_type) {
        case ET_IDLE:
          if (cwKey == CW_KEY_IAMBIC_B_L || cwKey == CW_KEY_IAMBIC_B_R) {
            // Iambic B
            if (k == PADDLE_NONE) {
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
    }
  };

  void setDisabled(bool disabled) {
    _disabled = disabled;
  };
private:
  uint8_t _pin;
  bool _is_key_down;
  unsigned long _key_up_at;
  unsigned long _element_at;
  uint8_t _element_type;
  uint8_t _expect_key, _next_key;
  bool _disabled;

  uint8_t getPaddle() {
    int paddle = analogRead(_pin);

    if (paddle > 800) return PADDLE_NONE;
    if (paddle > 600) return PADDLE_DASH;
    if (paddle > 300) return PADDLE_DOT;
    if (paddle > 50) return PADDLE_BOTH;
    return PADDLE_STRAIGHT;
  };

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
};

#endif // __KEYER_H__
