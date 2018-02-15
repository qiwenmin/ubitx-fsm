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

#ifndef __RIG_H__
#define __RIG_H__

#include <stdint.h>
#include "ui_tasks.h"
#include "objs.h"

#include "led_def.h"

#define MIN_FREQ 500000
#define MAX_FREQ 30000000

#define MODE_LSB 0x00
#define MODE_USB 0x01
#define MODE_CW 0x03
#define MODE_CWR 0x07

#define VFO_A 0
#define VFO_B 1

#define FILTER_NORMAL 0x02

#define OFF 0x00
#define ON 0x01

#define MEM_SIZE 20 // provides channel#00 to channel#19

typedef struct {
  int32_t freq;
  uint8_t mode;
} VFO;

typedef struct {
  VFO vfos[2]; // VFO A/B
  uint8_t active_vfo;
  uint8_t split;
} Channel;

inline void init_channel(Channel *ch) {
  ch->vfos[0].freq = 7074000;
  ch->vfos[0].mode = MODE_USB;

  ch->vfos[1].freq = 14025000;
  ch->vfos[1].mode = MODE_CW;

  ch->active_vfo = VFO_A;

  ch->split = OFF;
}

inline void copy_channel(Channel *dest, const Channel *src) {
  memcpy(dest, src, sizeof(Channel));
}

class Rig {
public:
  Rig() {
    _tx = OFF;
    _dial_lock = OFF;

    init_channel(&_vfo_ch);
    _working_ch = &_vfo_ch;

    // init mem with empty channels
    memset(_mem, 0, sizeof(_mem));
    _ch_idx = 0;

    init_channel(_mem);
    _mem[0].vfos[VFO_A].freq = 14074000;
    _mem[0].vfos[VFO_A].mode = MODE_USB;
  };

  void rigChanged() {
    uiTask.update_display(this);
  };

  void setFreq(int32_t freq) {
    if ((!rig.isVfo()) && rig.isMemOk()) {
      copy_channel(&_vfo_ch, &_mem[_ch_idx]);
      selectVfo();
    }

    if (freq < MIN_FREQ)
      _working_ch->vfos[_working_ch->active_vfo].freq = MIN_FREQ;
    else if (freq > MAX_FREQ)
      _working_ch->vfos[_working_ch->active_vfo].freq = MAX_FREQ;
    else
      _working_ch->vfos[_working_ch->active_vfo].freq = (freq / 10) * 10;

    rigChanged();
  };

  int32_t getRxFreq() { return _working_ch->vfos[_working_ch->active_vfo].freq; };

  int32_t getTxFreq() {
    uint8_t v = _working_ch->active_vfo;
    if (getSplit() == ON) {
      v = (v == VFO_A ? VFO_B : VFO_A);
    }

    return _working_ch->vfos[v].freq;
  };

  int32_t getFreq() { return getTx() == ON ? getTxFreq() : getRxFreq(); };

  bool setMode(uint8_t mode) {
    if (mode == MODE_LSB || mode == MODE_USB || mode == MODE_CW || mode == MODE_CWR) {
      if ((!rig.isVfo()) && rig.isMemOk()) {
        copy_channel(&_vfo_ch, &_mem[_ch_idx]);
        selectVfo();
      }

      _working_ch->vfos[_working_ch->active_vfo].mode = mode;

      rigChanged();

      return true;
    } else {
      return false;
    }
  };

  uint8_t getRxMode() { return _working_ch->vfos[_working_ch->active_vfo].mode; };

  uint8_t getTxMode() {
    uint8_t v = _working_ch->active_vfo;
    if (getSplit() == ON) {
      v = (v == VFO_A ? VFO_B : VFO_A);
    }

    return _working_ch->vfos[v].mode;
  };

  uint8_t getMode() { return getTx() == ON ? getTxMode() : getRxMode(); };

  void setTx(uint8_t tx) {
    if (tx == ON || tx == OFF) {
      _tx = tx;

      digitalWrite(LED_PIN, tx == ON ? LED_ON_VALUE : LED_OFF_VALUE);
      rigChanged();
    }
  };

  uint8_t getTx() { return _tx; };

  uint8_t getVfo() {
    uint8_t v = _working_ch->active_vfo;
    if (getSplit() == ON && getTx() == ON) {
      v = (v == VFO_A ? VFO_B : VFO_A);
    }

    return v;
  };

  void exchangeVfo() {
    if ((!rig.isVfo()) && rig.isMemOk()) {
      copy_channel(&_vfo_ch, &_mem[_ch_idx]);
      selectVfo();
    }

    _working_ch->active_vfo = _working_ch->active_vfo == VFO_A ? VFO_B : VFO_A;

    rigChanged();
  };

  void equalizeVfo() {
    if ((!rig.isVfo()) && rig.isMemOk()) {
      copy_channel(&_vfo_ch, &_mem[_ch_idx]);
      selectVfo();
    }

    _working_ch->vfos[0].freq = _working_ch->vfos[1].freq = getRxFreq();
    _working_ch->vfos[0].mode = _working_ch->vfos[1].mode = getRxMode();

    rigChanged();
  };

  void setVfo(uint8_t idx) {
    if ((!rig.isVfo()) && rig.isMemOk()) {
      copy_channel(&_vfo_ch, &_mem[_ch_idx]);
      selectVfo();
    }

    _working_ch->active_vfo = idx;
    rigChanged();
  };

  void setSplit(uint8_t val) {
    if ((!rig.isVfo()) && rig.isMemOk()) {
      copy_channel(&_vfo_ch, &_mem[_ch_idx]);
      selectVfo();
    }

    _working_ch->split = val;
    rigChanged();
  };

  uint8_t getSplit() { return _working_ch->split; };

  void setDialLock(uint8_t val) {
    _dial_lock = val;
    rigChanged();
  };

  uint8_t getDialLock() { return _dial_lock; };

  void selectVfo() {
    _working_ch = &_vfo_ch;
    rigChanged();
  };

  void selectMem() {
    if (isMemOk()) {
      _working_ch = &_mem[_ch_idx];
      rigChanged();
    }
  };

  bool selectMemCh(uint8_t ch, bool need_update = true) {
    if (ch >= MEM_SIZE) {
      return false;
    } else {
      _ch_idx = ch;
      if (need_update) rigChanged();
      return true;
    }
  };

  uint8_t getMemCh() { return _ch_idx; };

  void writeMemory() {
    if (isVfo()) {
      copy_channel(&_mem[_ch_idx], _working_ch);
      rigChanged();
    }
  };

  void memoryToVfo() {
    if (isMemOk()) {
      copy_channel(&_vfo_ch, &_mem[_ch_idx]);
      rigChanged();
    }
  };

  void clearMemory() {
    selectVfo();
    memset(&_mem[_ch_idx], 0, sizeof(Channel));
    rigChanged();
  };

  bool isVfo() { return _working_ch == (&_vfo_ch); };
  bool isMemOk() { return isMemOk(_ch_idx); };
  bool isMemOk(uint8_t ch_idx) { return _mem[ch_idx].vfos[_mem[ch_idx].active_vfo].freq != 0; };
private:
  uint8_t _tx;
  uint8_t _dial_lock;

  Channel *_working_ch;
  Channel _vfo_ch;
  Channel _mem[MEM_SIZE];
  uint8_t _ch_idx;
};

#endif // __RIG_H__
