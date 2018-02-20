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

//#define ITU_REGION_1
//#define ITU_REGION_2
#define ITU_REGION_3
//#define ITU_REGION_ALL

// https://en.wikipedia.org/wiki/WARC_bands
const uint16_t ham_band_range[10][2] PROGMEM = {
#if defined(ITU_REGION_1)
  { 1800, 1850 },
#elif defined(ITU_REGION_2) || defined(ITU_REGION_3) || defined(ITU_REGION_ALL)
  { 1800, 2000 },
#endif

#if defined(ITU_REGION_1)
  { 3500, 3800 },
#elif defined(ITU_REGION_2) || defined(ITU_REGION_ALL)
  { 3500, 4000 },
#elif defined(ITU_REGION_3)
  { 3500, 3900 },
#endif

  { 5351, 5367 },

#if defined(ITU_REGION_1) || defined(ITU_REGION_3)
  { 7000, 7200 },
#elif defined(ITU_REGION_2) || defined(ITU_REGION_ALL)
  { 7000, 7300 },
#endif

  { 10100, 10150 },
  { 14000, 14350 },
  { 18068, 18168 },
  { 21000, 21450 },
  { 24890, 24990 },
  { 28000, 29700 }
};

#define HAM_BAND_RANGE_LEN (10)

static bool in_ham_band_range(int32_t freq) {
  uint16_t fk = (freq / 1000);
  for (uint8_t i = 0; i < HAM_BAND_RANGE_LEN; i ++) {
    uint16_t fr[2];
    memcpy_P(fr, &ham_band_range[i][0], sizeof(fr));
    if (fr[0] <= fk && fk < fr[1]) return true;
  }

  return false;
}

#define EEPROM_MAGIC_NUMBER (0xF505)
#define EEPROM_VERSION_NO (1)
#define CHANNEL_SIZE (0x0010)

#define ADDR_MAGIC_NUMBER (0x0000)
#define ADDR_VERSION (0x0002)

#define ADDR_LOCK (0x0004)
#define ADDR_IS_VFO (0x0005)
#define ADDR_MEM_CH_IDX (0x0006)
#define ADDR_FREQ_ADJ_BASE (0x0007)

#define ADDR_CW_TONE (0x000B)
#define ADDR_CW_WPM (0x000C)
#define ADDR_CW_DELAY (0x000D)
#define ADDR_CW_KEY (0x000E)

// 0x000F - 0x000F reserved

#define ADDR_MASTER_CALI (0x0010)
#define ADDR_SSB_BFO (0x0014)
#define ADDR_CW_BFO (0x0018)

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

void eeprom_write_cw_tone(uint16_t tone) {
  int8_t n = (tone - 400) / 50;

  if (n < 0) n = 0;
  else if (n > 32) n = 32;

  EEPROM.put(ADDR_CW_TONE, n);
}

void eeprom_read_cw_tone(uint16_t &tone) {
  int8_t n;

  EEPROM.get(ADDR_CW_TONE, n);

  if (n < 0) n = 0;
  else if (n > 32) n = 32;

  tone = n * 50 + 400;
}

void eeprom_write_cw_wpm(uint8_t wpm) {
  if (wpm < 5) wpm = 5;
  else if (wpm > 60) wpm = 60;

  wpm -=5;

  EEPROM.put(ADDR_CW_WPM, wpm);
}

void eeprom_read_cw_wpm(uint8_t &wpm) {
  EEPROM.get(ADDR_CW_WPM, wpm);

  wpm += 5;

  if (wpm < 5) wpm = 5;
  else if (wpm > 60) wpm = 60;
}

void eeprom_write_cw_delay(uint16_t delay) {
  uint8_t n = delay / 100;

  EEPROM.put(ADDR_CW_DELAY, n);
}

void eeprom_read_cw_delay(uint16_t &delay) {
  uint8_t n;

  EEPROM.get(ADDR_CW_DELAY, n);

  delay = n * 100;
}

void eeprom_write_cw_key(uint8_t key) {
  EEPROM.put(ADDR_CW_KEY, key);
}

void eeprom_read_cw_key(uint8_t &key) {
  EEPROM.get(ADDR_CW_KEY, key);

  if (key > 4) key = 4;
}

void eeprom_write_master_cali(int32_t cali) {
  EEPROM.put(ADDR_MASTER_CALI, cali);
}

void eeprom_read_master_cali(int32_t &cali) {
  EEPROM.get(ADDR_MASTER_CALI, cali);
}

void eeprom_write_ssb_bfo(uint32_t bfo) {
  EEPROM.put(ADDR_SSB_BFO, bfo);
}

void eeprom_read_ssb_bfo(uint32_t &bfo) {
  EEPROM.get(ADDR_SSB_BFO, bfo);
}

void eeprom_write_cw_bfo(uint32_t bfo) {
  EEPROM.put(ADDR_CW_BFO, bfo);
}

void eeprom_read_cw_bfo(uint32_t &bfo) {
  EEPROM.get(ADDR_CW_BFO, bfo);
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

void Rig::init() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  _tx = OFF;

  if (eeprom_ok()) {
    Device::loadSettings();

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

  Device::init();

  updateDeviceFreqMode();
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

  Device::resetAll();
  Device::saveSettings();

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

  updateDeviceFreqMode();

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

    updateDeviceFreqMode();

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
  if ((tx == ON || tx == OFF) && (tx != _tx)) {

    if (tx == ON && (!in_ham_band_range(getTxFreq()))) return;

    _tx = tx;

    digitalWrite(LED_BUILTIN, tx == ON ? HIGH : LOW);

    updateDeviceFreqMode();

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

  updateDeviceFreqMode();

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

  updateDeviceFreqMode();

  if (need_update) rigChanged();
}

void Rig::setVfo(uint8_t idx, bool need_update) {
  if (getTx() == ON) return;

  if ((!rig.isVfo()) && rig.isMemOk()) {
    copy_channel(&_vfo_ch, &_mem_ch);
    selectVfo(false);
  }

  _working_ch->active_vfo = idx;

  updateDeviceFreqMode();

  if (need_update) rigChanged();
}

void Rig::setSplit(uint8_t val, bool need_update) {
  if (getTx() == ON) return;

  if ((!rig.isVfo()) && rig.isMemOk()) {
    copy_channel(&_vfo_ch, &_mem_ch);
    selectVfo(false);
  }

  _working_ch->split = val;

  updateDeviceFreqMode();

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

    updateDeviceFreqMode();

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

    updateDeviceFreqMode();

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

    updateDeviceFreqMode();

    if (need_update) rigChanged();
  }
}

void Rig::clearMemory(bool need_update) {
  if (getTx() == ON) return;

  selectVfo(false);
  memset(&_mem_ch, 0, sizeof(Channel));
  eeprom_write_mem_ch(_ch_idx, _mem_ch);

  updateDeviceFreqMode();

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

void Rig::updateDeviceFreqMode() {
  if (getTx() == OFF) {
    Device::setFreqMode(getRxFreq(), getRxMode(), getTx());
  } else {
    Device::setFreqMode(getTxFreq(), getTxMode(), getTx());
  }
}

///////////
// Device

#define TX_RX (7)
#define CW_TONE (6)
#define TX_LPF_A (5)
#define TX_LPF_B (4)
#define TX_LPF_C (3)
#define CW_KEY (2)

#define SECOND_OSC_USB (56995000l)
#define SECOND_OSC_LSB (32995000l)

int32_t Device::_freq = -1;
uint8_t Device::_mode = 0;
uint8_t Device::_tx = 0;

uint32_t Device::_ssbBfo = 11995000L;
uint32_t Device::_cwBfo = 11995000L;
uint16_t Device::_cwTone = 700;
uint8_t Device::_cwWpm = 15;
uint16_t Device::_cwSpeed = 1200 / Device::_cwWpm;
uint16_t Device::_cwDelay = 500;
uint8_t Device::_cwKey = CW_KEY_IAMBIC_B_R;

Device::Device() {
  calibration = 0;
}

void Device::init() {
  analogReference(DEFAULT);

  // TX_RX
  pinMode(TX_RX, OUTPUT);
  digitalWrite(TX_RX, 0);

  // CW_TONE
  pinMode(CW_TONE, OUTPUT);
  digitalWrite(CW_TONE, 0);

  // CW_KEY
  pinMode(CW_KEY, OUTPUT);
  digitalWrite(CW_KEY, 0);

  // LPF
  pinMode(TX_LPF_A, OUTPUT);
  pinMode(TX_LPF_B, OUTPUT);
  pinMode(TX_LPF_C, OUTPUT);

  digitalWrite(TX_LPF_A, 0);
  digitalWrite(TX_LPF_B, 0);
  digitalWrite(TX_LPF_C, 0);

  // si5351bx
  initOscillators();
}

void Device::resetAll() {
  Device::_ssbBfo = 11995000L;
  Device::_cwBfo = 11995000L;
  Device::_cwTone = 700;
  Device::_cwWpm = 15;
  Device::_cwSpeed = 1200 / Device::_cwWpm;
  Device::_cwDelay = 500;
  Device::_cwKey = CW_KEY_IAMBIC_B_R;
}

void Device::loadSettings() {
  eeprom_read_cw_tone(Device::_cwTone);
  eeprom_read_cw_wpm(Device::_cwWpm);
  Device::_cwSpeed = 1200 / Device::_cwWpm;
  eeprom_read_cw_delay(Device::_cwDelay);
  eeprom_read_cw_key(Device::_cwKey);

  eeprom_read_master_cali(calibration);
  eeprom_read_ssb_bfo(Device::_ssbBfo);
  eeprom_read_cw_bfo(Device::_cwBfo);

}

void Device::saveSettings() {
  eeprom_write_cw_tone(Device::_cwTone);
  eeprom_write_cw_wpm(Device::_cwWpm);
  eeprom_write_cw_delay(Device::_cwDelay);
  eeprom_write_cw_key(Device::_cwKey);

  eeprom_write_master_cali(calibration);
  eeprom_write_ssb_bfo(Device::_ssbBfo);
  eeprom_write_cw_bfo(Device::_cwBfo);
}

void Device::setFreqMode(int32_t freq, uint8_t mode, uint8_t tx) {
  _freq = freq; _mode = mode; _tx = tx;

  Device::updateHardware();
}

void Device::updateHardware() {
  Device::setTxFilters(_freq);

  digitalWrite(TX_RX, _tx == ON ? HIGH : LOW);

  if (_mode == MODE_CW || _mode == MODE_CWR) {
    usbCarrier = Device::_cwBfo;
  } else {
    usbCarrier = Device::_ssbBfo;
  }

  if (_tx == OFF) {
    si5351bx_setfreq(0, usbCarrier);

    int32_t f = _freq;
    if (_mode == MODE_CW) f = _freq - Device::_cwTone;
    else if (_mode == MODE_CWR) f = _freq + Device::_cwTone;

    if (_mode == MODE_USB || _mode == MODE_CW) {
      si5351bx_setfreq(2, SECOND_OSC_USB - usbCarrier + f);
      si5351bx_setfreq(1, SECOND_OSC_USB);
    } else {
      si5351bx_setfreq(2, SECOND_OSC_LSB + usbCarrier + f);
      si5351bx_setfreq(1, SECOND_OSC_LSB);
    }
  } else {
    if (_mode == MODE_USB) {
      si5351bx_setfreq(0, usbCarrier);
      si5351bx_setfreq(2, SECOND_OSC_USB - usbCarrier + _freq);
      si5351bx_setfreq(1, SECOND_OSC_USB);
    } else if (_mode == MODE_LSB) {
      si5351bx_setfreq(0, usbCarrier);
      si5351bx_setfreq(2, SECOND_OSC_LSB + usbCarrier + _freq);
      si5351bx_setfreq(1, SECOND_OSC_LSB);
    } else {
      si5351bx_setfreq(0, usbCarrier);
      si5351bx_setfreq(2, _freq);
      si5351bx_setfreq(1, 0);
    }
  }
}

void Device::setCwTone(int16_t cwTone) {
  Device::_cwTone = cwTone;

  Device::updateHardware();

  eeprom_write_cw_tone(Device::_cwTone);
}

int16_t Device::getCwTone() {
  return Device::_cwTone;
}

void Device::setCwWpm(uint8_t wpm) {
  Device::_cwWpm = wpm;
  Device::_cwSpeed = 1200 / Device::_cwWpm;

  eeprom_write_cw_wpm(Device::_cwWpm);
}

uint8_t Device::getCwWpm() {
  return Device::_cwWpm;
}

uint16_t Device::getCwSpeed() {
  return Device::_cwSpeed;
}

void Device::setCwDelay(uint16_t cwDelay) {
  Device::_cwDelay = cwDelay;

  eeprom_write_cw_delay(Device::_cwDelay);
}

uint16_t Device::getCwDelay() {
  return Device::_cwDelay;
}

void Device::setCwKey(uint8_t key) {
  Device::_cwKey = key;

  eeprom_write_cw_key(Device::_cwKey);
}

uint8_t Device::getCwKey() {
  return Device::_cwKey;
}

void Device::cwKeyDown() {
  digitalWrite(CW_KEY, 1);
  tone(CW_TONE, Device::_cwTone);
}

void Device::cwKeyUp() {
  digitalWrite(CW_KEY, 0);
  noTone(CW_TONE);
}

void Device::startCalibrate10M() {
  Device::setTxFilters(10000000L);

  calibration = 0;

  si5351_set_calibration(calibration);

  digitalWrite(TX_RX, 1);

  si5351bx_setfreq(0, 0);
  si5351bx_setfreq(1, 0);
  si5351bx_setfreq(2, 10000000L); 

  digitalWrite(CW_KEY, 1);
}

void Device::updateCalibrate10M() {
  si5351_set_calibration(calibration);
  si5351bx_setfreq(2, 10000000L);
}

void Device::stopCalibrate10M(bool save) {
  if (save) eeprom_write_master_cali(calibration);

  digitalWrite(CW_KEY, 0);
  digitalWrite(TX_RX, 0);

  si5351_set_calibration(calibration);
  Device::updateHardware();
}

void Device::startCalibrateBfo() {
  Device::updateHardware();
}

void Device::updateCalibrateBfo() {
  uint8_t mode = rig.getRxMode();

  if (mode == MODE_CW || mode == MODE_CWR) {
    Device::_cwBfo = usbCarrier;
  } else {
    Device::_ssbBfo = usbCarrier;
  }

  Device::updateHardware();
}

void Device::stopCalibrateBfo(bool save) {
  if (save) {
    uint8_t mode = rig.getRxMode();

    if (mode == MODE_CW || mode == MODE_CWR) {
      eeprom_write_cw_bfo(Device::_cwBfo);
    } else {
      eeprom_write_ssb_bfo(Device::_ssbBfo);      
    }
  }
  
  Device::updateHardware();
}

/**
 * Select the properly tx harmonic filters
 * The four harmonic filters use only three relays
 * the four LPFs cover 30-21 Mhz, 18 - 14 Mhz, 7-10 MHz and 3.5 to 5 Mhz
 * Briefly, it works like this, 
 * - When KT1 is OFF, the 'off' position routes the PA output through the 30 MHz LPF
 * - When KT1 is ON, it routes the PA output to KT2. Which is why you will see that
 *   the KT1 is on for the three other cases.
 * - When the KT1 is ON and KT2 is off, the off position of KT2 routes the PA output
 *   to 18 MHz LPF (That also works for 14 Mhz) 
 * - When KT1 is On, KT2 is On, it routes the PA output to KT3
 * - KT3, when switched on selects the 7-10 Mhz filter
 * - KT3 when switched off selects the 3.5-5 Mhz filter
 * See the circuit to understand this
 */

void Device::setTxFilters(int32_t freq) {
  if (freq > 21000000L) {  // the default filter is with 35 MHz cut-off
    digitalWrite(TX_LPF_A, 0);
    digitalWrite(TX_LPF_B, 0);
    digitalWrite(TX_LPF_C, 0);
  } else if (freq >= 14000000L) { //thrown the KT1 relay on, the 30 MHz LPF is bypassed and the 14-18 MHz LPF is allowd to go through
    digitalWrite(TX_LPF_A, 1);
    digitalWrite(TX_LPF_B, 0);
    digitalWrite(TX_LPF_C, 0);
  } else if (freq > 7000000L) {
    digitalWrite(TX_LPF_A, 1);
    digitalWrite(TX_LPF_B, 1);
    digitalWrite(TX_LPF_C, 0);
  } else {
    digitalWrite(TX_LPF_A, 1);
    digitalWrite(TX_LPF_B, 1);
    digitalWrite(TX_LPF_C, 1);
  }
}
