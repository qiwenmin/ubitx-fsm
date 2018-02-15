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
private:
  int8_t _current_state;

  uint8_t _last_fbutton_state;

  int8_t _menu_idx, _submenu_idx;

  void update_rig_display();
  void update_menu_display();

  void _print_rig_mode(uint8_t mode, uint8_t col, uint8_t row);
};

#endif // __UI_TASKS_H__
