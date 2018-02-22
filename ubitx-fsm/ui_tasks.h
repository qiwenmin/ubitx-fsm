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

#ifndef __UI_TASKS_H__
#define __UI_TASKS_H__

#include <fsmos.h>

class ButtonInputTask : public FsmTask {
public:
  ButtonInputTask(uint8_t pin);

  virtual void init();
  virtual bool on_state_change(int8_t, int8_t);
  virtual void in_state(int8_t);

  uint8_t getButtonState();
  unsigned long getButtonPressAt();

  uint8_t getRawState();
private:
  uint8_t _pin;
  uint8_t _state;
  unsigned long _press_at;
};

class EncoderTask : public FsmTask {
public:
  EncoderTask(uint8_t pin_a, uint8_t pin_b);

  virtual void init();
  virtual bool on_state_change(int8_t, int8_t);
  virtual void in_state(int8_t);

  int8_t get_value();
  void reset_value();
private:
  uint8_t _pin_a, _pin_b;
  uint8_t _enc_state, _new_enc_state;
  int8_t _value, _current_value;

  unsigned long _calc_at;

  uint8_t read_encoder();
};

class UiTask : public FsmTask {
public:
  virtual void init();
  virtual bool on_state_change(int8_t, int8_t);
  virtual void in_state(int8_t);

  void update_display(void *);
  void gotoSysMenu();
  bool isMenuMode();
private:
  int8_t _current_state;

  uint8_t _last_fbutton_state;
  uint8_t _last_ptt_state;

  int8_t _menu_idx;
  bool _menu_change_val;
  int16_t _menu_val;

  uint8_t _last_tx;
  unsigned long _tx_flashing_at;
  unsigned long _save_vfo_ch_at;

  void update_rig_display();
  void update_menu_display();
  void update_freq_adj_base();

  void format_mode(char *, uint8_t);

  void in_state_menu_none(bool fbtn_change, uint8_t fbtn_from_state, int8_t enc_val);
  void in_state_menu_main(bool fbtn_change, uint8_t fbtn_from_state, int8_t enc_val);
  void in_state_menu_freq_adj_base(bool fbtn_change, uint8_t fbtn_from_state, int8_t enc_val);
};

#endif // __UI_TASKS_H__
