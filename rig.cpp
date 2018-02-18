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

#include "rig.h"

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

Rig::Rig() {
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
}

void Rig::rigChanged() {
  uiTask.update_display(this);
}

void Rig::setFreq(int32_t freq, bool need_update) {
  if (getTx() == ON) return;

  if ((!rig.isVfo()) && rig.isMemOk()) {
    copy_channel(&_vfo_ch, &_mem[_ch_idx]);
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
      copy_channel(&_vfo_ch, &_mem[_ch_idx]);
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
    copy_channel(&_vfo_ch, &_mem[_ch_idx]);
    selectVfo(false);
  }

  _working_ch->active_vfo = _working_ch->active_vfo == VFO_A ? VFO_B : VFO_A;

  if (need_update) rigChanged();
}

void Rig::equalizeVfo(bool need_update) {
  if (getTx() == ON) return;

  if ((!rig.isVfo()) && rig.isMemOk()) {
    copy_channel(&_vfo_ch, &_mem[_ch_idx]);
    selectVfo(false);
  }

  _working_ch->vfos[0].freq = _working_ch->vfos[1].freq = getRxFreq();
  _working_ch->vfos[0].mode = _working_ch->vfos[1].mode = getRxMode();

  if (need_update) rigChanged();
}

void Rig::setVfo(uint8_t idx, bool need_update) {
  if (getTx() == ON) return;

  if ((!rig.isVfo()) && rig.isMemOk()) {
    copy_channel(&_vfo_ch, &_mem[_ch_idx]);
    selectVfo(false);
  }

  _working_ch->active_vfo = idx;
  if (need_update) rigChanged();
}

void Rig::setSplit(uint8_t val, bool need_update) {
  if (getTx() == ON) return;

  if ((!rig.isVfo()) && rig.isMemOk()) {
    copy_channel(&_vfo_ch, &_mem[_ch_idx]);
    selectVfo(false);
  }

  _working_ch->split = val;
  if (need_update) rigChanged();
}

uint8_t Rig::getSplit() { return _working_ch->split; };

void Rig::setDialLock(uint8_t val, bool need_update) {
  if (getTx() == ON) return;

  _dial_lock = val;
  if (need_update) rigChanged();
}

uint8_t Rig::getDialLock() { return _dial_lock; };

void Rig::selectVfo(bool need_update) {
  if (getTx() == ON) return;

  _working_ch = &_vfo_ch;
  if (need_update) rigChanged();
}

void Rig::selectMem(bool need_update) {
  if (getTx() == ON) return;

  if (isMemOk()) {
    _working_ch = &_mem[_ch_idx];
    if (need_update) rigChanged();
  }
}

bool Rig::selectMemCh(int8_t ch, bool need_update) {
  if (getTx() == ON) return false;

  if (ch < 0 || ch >= MEM_SIZE) {
    return false;
  } else {
    _ch_idx = ch;
    if (!isMemOk()) selectVfo(false);
    // selectMem(false);
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
    copy_channel(&_mem[ch_idx == -1 ? _ch_idx : ch_idx], _working_ch);
    if (need_update) rigChanged();
  }
}

void Rig::memoryToVfo(int8_t ch_idx, bool need_update) {
  if (getTx() == ON) return;

  if (isMemOk(ch_idx == -1 ? _ch_idx : ch_idx)) {
    copy_channel(&_vfo_ch, &_mem[ch_idx == -1 ? _ch_idx : ch_idx]);
    if (need_update) rigChanged();
  }
}

void Rig::clearMemory(bool need_update) {
  if (getTx() == ON) return;

  selectVfo(false);
  memset(&_mem[_ch_idx], 0, sizeof(Channel));
  if (need_update) rigChanged();
}

bool Rig::isVfo() {
  return _working_ch == (&_vfo_ch);
}

bool Rig::isMemOk() {
  return isMemOk(_ch_idx);
}

bool Rig::isMemOk(int8_t ch_idx) {
  return _mem[ch_idx].vfos[_mem[ch_idx].active_vfo].freq != 0;
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
