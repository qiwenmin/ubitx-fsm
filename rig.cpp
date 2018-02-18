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

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wignored-qualifiers"
#include <EEPROM.h>
#pragma GCC diagnostic pop

#include "rig.h"
#include "version.h"

#define EEPROM_MAGIC_NUMBER (0xF505)
#define EEPROM_VERSION_NO (1)
#define CHANNEL_SIZE (0x0010)

#define ADDR_MAGIC_NUMBER (0x0000)
#define ADDR_VERSION (0x0002)

#define ADDR_LOCK (0x0004)
#define ADDR_IS_VFO (0x0005)
#define ADDR_MEM_CH_IDX (0x0006)
#define ADDR_FREQ_ADJ_BASE (0x0007)

#define ADDR_VFOS (0x00F0) // 0x0100 - 0x0010
#define ADDR_MEM_CH_BEGIN (0x0100)  // MEM: 0x0100 ~ 0x02FF

inline bool eeprom_ok() {
  uint16_t n = 0;

  EEPROM.get(ADDR_MAGIC_NUMBER, n);
  if (n != EEPROM_MAGIC_NUMBER) return false;

  EEPROM.get(ADDR_VERSION, n);
  if (n != EEPROM_VERSION_NO) return false;

  return true;
}

void eeprom_write_head() {
  uint16_t n = EEPROM_MAGIC_NUMBER;
  EEPROM.put(ADDR_MAGIC_NUMBER, n);

  n = EEPROM_VERSION_NO;
  EEPROM.put(ADDR_VERSION, n);
}

void eeprom_write_lock(uint8_t lock) {
  EEPROM.put(ADDR_LOCK, lock);
}

void eeprom_read_lock(uint8_t &lock) {
  EEPROM.get(ADDR_LOCK, lock);
}

void eeprom_write_is_vfo(bool is_vfo) {
  EEPROM.put(ADDR_IS_VFO, is_vfo);
}

void eeprom_read_is_vfo(bool &is_vfo) {
  EEPROM.get(ADDR_IS_VFO, is_vfo);
}

void eeprom_write_mem_ch_idx(int8_t idx) {
  EEPROM.put(ADDR_MEM_CH_IDX, idx);
}

void eeprom_read_mem_ch_idx(int8_t &idx) {
  EEPROM.get(ADDR_MEM_CH_IDX, idx);
}

void eeprom_write_freq_adj_base(int32_t base) {
  uint8_t n = 0;

  if (base == 10) n = 0;
  else if (base == 100) n = 1;
  else if (base == 1000) n = 2;
  else if (base == 10000) n = 3;
  else if (base == 100000) n = 4;
  else if (base == 1000000) n = 5;

  EEPROM.put(ADDR_FREQ_ADJ_BASE, n);
}

void eeprom_read_freq_adj_base(int32_t &base) {
  uint8_t n;

  EEPROM.get(ADDR_FREQ_ADJ_BASE, n);

  if (n == 0) base = 10;
  else if (n == 1) base = 100;
  else if (n == 2) base = 1000;
  else if (n == 3) base = 10000;
  else if (n == 4) base = 100000;
  else if (n == 5) base = 1000000;
}

void eeprom_write_vfos(const Channel &ch) {
  EEPROM.put(ADDR_VFOS, ch);
}

void eeprom_read_vfos(Channel &ch) {
  EEPROM.get(ADDR_VFOS, ch);
}

void eeprom_write_mem_ch(int8_t idx, const Channel &ch) {
  EEPROM.put(ADDR_MEM_CH_BEGIN + CHANNEL_SIZE * idx, ch);
}

void eeprom_read_mem_ch(int8_t idx, Channel &ch) {
  EEPROM.get(ADDR_MEM_CH_BEGIN + CHANNEL_SIZE * idx, ch);
}

inline void init_channel(Channel *ch) {
  ch->vfos[0].freq = 7023000;
  ch->vfos[0].mode = MODE_CW;

  ch->vfos[1].freq = 7050000;
  ch->vfos[1].mode = MODE_LSB;

  ch->active_vfo = VFO_A;

  ch->split = OFF;
}

inline void copy_channel(Channel *dest, const Channel *src) {
  memcpy(dest, src, sizeof(Channel));
}

Rig::Rig() {
  _tx = OFF;

  if (eeprom_ok()) {
    eeprom_read_lock(_dial_lock);
    eeprom_read_is_vfo(_is_vfo);
    eeprom_read_mem_ch_idx(_ch_idx);
    
    eeprom_read_vfos(_vfo_ch);
    eeprom_read_vfos(_vfo_ch_saved);
    eeprom_read_mem_ch(_ch_idx, _mem_ch);
    eeprom_read_freq_adj_base(_freq_adj_base);

    if (_is_vfo) _working_ch = &_vfo_ch;
    else _working_ch = &_mem_ch;
  } else {
    resetAll();
  }
}

void Rig::resetAll() {
  _dial_lock = OFF;

  // init mem with empty channels
  memset(&_mem_ch, 0, sizeof(_mem_ch));

  _ch_idx = 0;
  _freq_adj_base = 100;

  init_channel(&_vfo_ch);
  init_channel(&_vfo_ch_saved);

  _is_vfo = true;
  _working_ch = &_vfo_ch;

  // reset eeprom
  eeprom_write_head();

  eeprom_write_lock(_dial_lock);
  eeprom_write_is_vfo(_is_vfo);
  eeprom_write_mem_ch_idx(_ch_idx);
  eeprom_write_freq_adj_base(_freq_adj_base);

  eeprom_write_vfos(_vfo_ch);

  for (int8_t i = 0; i < MEM_SIZE; i ++) {
    eeprom_write_mem_ch(i, _mem_ch);
  }
}

void Rig::rigChanged() {
  uiTask.update_display(this);
}

void Rig::setFreq(int32_t freq, bool need_update) {
  if (getTx() == ON) return;

  if ((!rig.isVfo()) && rig.isMemOk()) {
    copy_channel(&_vfo_ch, &_mem_ch);
    selectVfo(false);
  }

  if (freq < MIN_FREQ)
    _working_ch->vfos[_working_ch->active_vfo].freq = MIN_FREQ;
  else if (freq > MAX_FREQ)
    _working_ch->vfos[_working_ch->active_vfo].freq = MAX_FREQ;
  else
    _working_ch->vfos[_working_ch->active_vfo].freq = (freq / 10) * 10;

  if (need_update) rigChanged();
}

int32_t Rig::getRxFreq() {
  return _working_ch->vfos[_working_ch->active_vfo].freq;
}

int32_t Rig::getTxFreq() {
  uint8_t v = _working_ch->active_vfo;
  if (getSplit() == ON) {
    v = (v == VFO_A ? VFO_B : VFO_A);
  }

  return _working_ch->vfos[v].freq;
}

int32_t Rig::getFreq() {
  return getTx() == ON ? getTxFreq() : getRxFreq();
}

int32_t Rig::getFreqAnother() {
  uint8_t v = getVfo() == VFO_A ? VFO_B : VFO_A;
  return _working_ch->vfos[v].freq;
}

bool Rig::setMode(uint8_t mode, bool need_update) {
  if (getTx() == ON) return false;

  if (mode == MODE_LSB || mode == MODE_USB || mode == MODE_CW || mode == MODE_CWR) {
    if ((!rig.isVfo()) && rig.isMemOk()) {
      copy_channel(&_vfo_ch, &_mem_ch);
      selectVfo(false);
    }

    _working_ch->vfos[_working_ch->active_vfo].mode = mode;

    if (need_update) rigChanged();

    return true;
  } else {
    return false;
  }
}

uint8_t Rig::getRxMode() {
  return _working_ch->vfos[_working_ch->active_vfo].mode;
}

uint8_t Rig::getTxMode() {
  uint8_t v = _working_ch->active_vfo;
  if (getSplit() == ON) {
    v = (v == VFO_A ? VFO_B : VFO_A);
  }

  return _working_ch->vfos[v].mode;
}

uint8_t Rig::getMode() {
  return getTx() == ON ? getTxMode() : getRxMode();
}

uint8_t Rig::getModeAnother() {
  uint8_t v = getVfo() == VFO_A ? VFO_B : VFO_A;
  return _working_ch->vfos[v].mode;
}

void Rig::setTx(uint8_t tx) {
  if (tx == ON || tx == OFF) {
    _tx = tx;

    digitalWrite(LED_BUILTIN, tx == ON ? HIGH : LOW);
    rigChanged();
  }
}

uint8_t Rig::getTx() {
  return _tx;
}

uint8_t Rig::getVfo() {
  uint8_t v = _working_ch->active_vfo;
  if (getSplit() == ON && getTx() == ON) {
    v = (v == VFO_A ? VFO_B : VFO_A);
  }

  return v;
}

void Rig::exchangeVfo(bool need_update) {
  if (getTx() == ON) return;

  if ((!rig.isVfo()) && rig.isMemOk()) {
    copy_channel(&_vfo_ch, &_mem_ch);
    selectVfo(false);
  }

  _working_ch->active_vfo = _working_ch->active_vfo == VFO_A ? VFO_B : VFO_A;

  if (need_update) rigChanged();
}

void Rig::equalizeVfo(bool need_update) {
  if (getTx() == ON) return;

  if ((!rig.isVfo()) && rig.isMemOk()) {
    copy_channel(&_vfo_ch, &_mem_ch);
    selectVfo(false);
  }

  _working_ch->vfos[0].freq = _working_ch->vfos[1].freq = getRxFreq();
  _working_ch->vfos[0].mode = _working_ch->vfos[1].mode = getRxMode();

  if (need_update) rigChanged();
}

void Rig::setVfo(uint8_t idx, bool need_update) {
  if (getTx() == ON) return;

  if ((!rig.isVfo()) && rig.isMemOk()) {
    copy_channel(&_vfo_ch, &_mem_ch);
    selectVfo(false);
  }

  _working_ch->active_vfo = idx;
  if (need_update) rigChanged();
}

void Rig::setSplit(uint8_t val, bool need_update) {
  if (getTx() == ON) return;

  if ((!rig.isVfo()) && rig.isMemOk()) {
    copy_channel(&_vfo_ch, &_mem_ch);
    selectVfo(false);
  }

  _working_ch->split = val;
  if (need_update) rigChanged();
}

uint8_t Rig::getSplit() { return _working_ch->split; };

void Rig::setDialLock(uint8_t val, bool need_update) {
  if (getTx() == ON) return;

  _dial_lock = val;

  eeprom_write_lock(_dial_lock);

  if (need_update) rigChanged();
}

uint8_t Rig::getDialLock() { return _dial_lock; };

void Rig::selectVfo(bool need_update) {
  if (getTx() == ON) return;

  _working_ch = &_vfo_ch;

  _is_vfo = true;
  eeprom_write_is_vfo(_is_vfo);

  if (need_update) rigChanged();
}

void Rig::selectMem(bool need_update) {
  if (getTx() == ON) return;

  if (isMemOk()) {
    _working_ch = &_mem_ch;

    _is_vfo = false;
    eeprom_write_is_vfo(_is_vfo);

    if (need_update) rigChanged();
  }
}

bool Rig::selectMemCh(int8_t ch, bool need_update) {
  if (getTx() == ON) return false;

  if (ch < 0 || ch >= MEM_SIZE) {
    return false;
  } else {
    _ch_idx = ch;
    eeprom_read_mem_ch(_ch_idx, _mem_ch);
    eeprom_write_mem_ch_idx(_ch_idx);

    if (!isMemOk()) selectVfo(false);
    if (need_update) rigChanged();
    return true;
  }
}

int8_t Rig::getMemCh() {
  return _ch_idx;
}

void Rig::writeMemory(int8_t ch_idx, bool need_update) {
  if (getTx() == ON) return;

  if (isVfo()) {
    copy_channel(&_mem_ch, _working_ch);
    eeprom_write_mem_ch(ch_idx == -1 ? _ch_idx : ch_idx, _mem_ch);
    if (need_update) rigChanged();
  }
}

void Rig::memoryToVfo(int8_t ch_idx, bool need_update) {
  if (getTx() == ON) return;

  if (isMemOk(ch_idx == -1 ? _ch_idx : ch_idx)) {
    eeprom_read_mem_ch(ch_idx == -1 ? _ch_idx : ch_idx, _vfo_ch);
    if (need_update) rigChanged();
  }
}

void Rig::clearMemory(bool need_update) {
  if (getTx() == ON) return;

  selectVfo(false);
  memset(&_mem_ch, 0, sizeof(Channel));
  eeprom_write_mem_ch(_ch_idx, _mem_ch);
  if (need_update) rigChanged();
}

bool Rig::isVfo() {
  return _working_ch == (&_vfo_ch);
}

bool Rig::isMemOk() {
  return isMemOk(_ch_idx);
}

bool Rig::isMemOk(int8_t ch_idx) {
  Channel ch;
  eeprom_read_mem_ch(ch_idx, ch);

  return ch.vfos[ch.active_vfo].freq != 0;
}

int8_t Rig::getPrevMemOkCh(int8_t ch_idx) {
  int8_t result = -1;

  for (int8_t i = ch_idx - 1 + MEM_SIZE; i >= ch_idx; i --) {
    int8_t ch = i % MEM_SIZE;
    if (isMemOk(ch)) {
      result = ch;
      break;
    }
  }

  return result;
}

int8_t Rig::getNextMemOkCh(int8_t ch_idx) {
  int8_t result = -1;

  for (int8_t i = ch_idx + 1; i <= ch_idx + MEM_SIZE; i ++) {
    int8_t ch = i % MEM_SIZE;
    if (isMemOk(ch)) {
      result = ch;
      break;
    }
  }

  return result;
}

void Rig::saveVfoCh() {
  if (
    (_vfo_ch.active_vfo != _vfo_ch_saved.active_vfo) ||
    (_vfo_ch.split != _vfo_ch_saved.split) ||
    (_vfo_ch.vfos[0].freq != _vfo_ch_saved.vfos[0].freq) ||
    (_vfo_ch.vfos[0].mode != _vfo_ch_saved.vfos[0].mode) ||
    (_vfo_ch.vfos[1].freq != _vfo_ch_saved.vfos[1].freq) ||
    (_vfo_ch.vfos[1].mode != _vfo_ch_saved.vfos[1].mode)) {

    eeprom_write_vfos(_vfo_ch);
    copy_channel(&_vfo_ch_saved, &_vfo_ch);
  }
}

void Rig::setFreqAdjBase(int32_t base) {
  _freq_adj_base = base;
  eeprom_write_freq_adj_base(_freq_adj_base);
}

int32_t Rig::getFreqAdjBase() {
  return _freq_adj_base;
}

