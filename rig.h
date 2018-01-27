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
#include "display_task.h"
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
  uint32_t freq;
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

  void refreshDisplay() {
    char _buf[17];

    // freq
    uint32_t freq = getFreq();
    sprintf(_buf, "%2lu.%05lu", freq / 1000000, (freq % 1000000) / 10);
    displayTask.print(4, 1, _buf);

    // mode
    uint8_t mode = getMode();
    switch (mode) {
    case MODE_CW:
      displayTask.print(13, 1, " CW");
      break;
    case MODE_CWR:
      displayTask.print(13, 1, "CWR");
      break;
    case MODE_LSB:
      displayTask.print(13, 1, "LSB");
      break;
    case MODE_USB:
      displayTask.print(13, 1, "USB");
      break;
    }

    // SPLIT?
    displayTask.print(6, 0, getSplit() == ON ? "SPL" : "   ");

    // Lock?
    displayTask.print(10, 0, getDialLock() == ON ? "LCK" : "   ");

    // VFO? MEM?
    displayTask.print(0, 0, isVfo() ? "VFO-" : "MEM-");

    // VFO A? B?
    displayTask.print(4, 0, getVfo() == VFO_A ? "A" : "B");

    // Mem OK?
    displayTask.print(0, 1, isMemOk() ? "M" : "-");

    // Ch#
    sprintf(_buf, "%02d", getMemCh());
    displayTask.print(1, 1, _buf);

    // TX?
    displayTask.print(14, 0, getTx() == ON ? "TX" : "  ");
  };

  void setFreq(uint32_t freq) {
    selectVfo();
    if (freq < MIN_FREQ)
      _working_ch->vfos[_working_ch->active_vfo].freq = MIN_FREQ;
    else if (freq > MAX_FREQ)
      _working_ch->vfos[_working_ch->active_vfo].freq = MAX_FREQ;
    else
      _working_ch->vfos[_working_ch->active_vfo].freq = freq;

    refreshDisplay();
  };

  uint32_t getRxFreq() { return _working_ch->vfos[_working_ch->active_vfo].freq; };

  uint32_t getTxFreq() {
    uint8_t v = _working_ch->active_vfo;
    if (getSplit() == ON) {
      v = (v == VFO_A ? VFO_B : VFO_A);
    }

    return _working_ch->vfos[v].freq;
  };

  uint32_t getFreq() { return getTx() == ON ? getTxFreq() : getRxFreq(); };

  bool setMode(uint8_t mode) {
    if (mode == MODE_LSB || mode == MODE_USB || mode == MODE_CW || mode == MODE_CWR) {
      selectVfo();
      _working_ch->vfos[_working_ch->active_vfo].mode = mode;

      refreshDisplay();

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
      refreshDisplay();
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
    _working_ch->active_vfo = _working_ch->active_vfo == VFO_A ? VFO_B : VFO_A;

    refreshDisplay();
  };

  void equalizeVfo() {
    _working_ch->vfos[0].freq = _working_ch->vfos[1].freq = getRxFreq();
    _working_ch->vfos[0].mode = _working_ch->vfos[1].mode = getRxMode();

    refreshDisplay();
  };

  void setVfo(uint8_t idx) {
    _working_ch->active_vfo = idx;
    refreshDisplay();
  };

  void setSplit(uint8_t val) {
    selectVfo();
    _working_ch->split = val;
    refreshDisplay();
  };

  uint8_t getSplit() { return _working_ch->split; };

  void setDialLock(uint8_t val) {
    _dial_lock = val;
    refreshDisplay();
  };

  uint8_t getDialLock() { return _dial_lock; };

  void selectVfo() {
    _working_ch = &_vfo_ch;
    refreshDisplay();
  };

  void selectMem() {
    if (isMemOk()) {
      _working_ch = &_mem[_ch_idx];
      refreshDisplay();
    }
  };

  bool selectMemCh(uint8_t ch) {
    if (ch >= MEM_SIZE || (!isMemOk())) {
      return false;
    } else {
      _ch_idx = ch;
      refreshDisplay();
      return true;
    }
  };

  uint8_t getMemCh() { return _ch_idx; };

  void writeMemory() {
    if (isVfo()) {
      copy_channel(&_mem[_ch_idx], _working_ch);
      refreshDisplay();
    }
  };

  void memoryToVfo() {
    if (isMemOk()) {
      copy_channel(&_vfo_ch, &_mem[_ch_idx]);
      refreshDisplay();
    }
  };

  void clearMemory() {
    selectVfo();
    memset(&_mem[_ch_idx], 0, sizeof(Channel));
    refreshDisplay();
  };

  bool isVfo() { return _working_ch == (&_vfo_ch); };
  bool isMemOk() { return _mem[_ch_idx].vfos[_mem[_ch_idx].active_vfo].freq != 0; };
private:
  uint8_t _tx;
  uint8_t _dial_lock;

  Channel *_working_ch;
  Channel _vfo_ch;
  Channel _mem[MEM_SIZE];
  uint8_t _ch_idx;
};

#endif // __RIG_H__
