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

#define MIN_FREQ 500000
#define MAX_FREQ 29999990

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

#pragma pack(push, 1)

// 5 bytes
typedef struct {
  int32_t freq;
  uint8_t mode;
} VFO;

// 12 bytes
typedef struct {
  VFO vfos[2]; // VFO A/B
  uint8_t active_vfo;
  uint8_t split;
} Channel;

#pragma pack(pop)

class Rig {
public:
  void init();

  void rigChanged();

  void setFreq(int32_t freq, bool need_update = true);

  int32_t getRxFreq();

  int32_t getTxFreq();

  int32_t getFreq();

  int32_t getFreqAnother();

  bool setMode(uint8_t mode, bool need_update = true);

  uint8_t getRxMode();

  uint8_t getTxMode();

  uint8_t getMode();

  uint8_t getModeAnother();

  void setTx(uint8_t tx);

  uint8_t getTx();

  uint8_t getVfo();

  void exchangeVfo(bool need_update = true);

  void equalizeVfo(bool need_update = true);

  void setVfo(uint8_t idx, bool need_update = true);

  void setSplit(uint8_t val, bool need_update = true);

  uint8_t getSplit();

  void setDialLock(uint8_t val, bool need_update = true);

  uint8_t getDialLock();

  void selectVfo(bool need_update = true);

  void selectMem(bool need_update = true);

  bool selectMemCh(int8_t ch, bool need_update = true);

  int8_t getMemCh();

  void writeMemory(int8_t ch_idx = -1, bool need_update = true);

  void memoryToVfo(int8_t ch_idx = -1, bool need_update = true);

  void clearMemory(bool need_update = true);

  bool isVfo();

  bool isMemOk();
  bool isMemOk(int8_t ch_idx);

  int8_t getPrevMemOkCh(int8_t ch_idx);
  int8_t getNextMemOkCh(int8_t ch_idx);

  void resetAll();

  void saveVfoCh();
  void setFreqAdjBase(int32_t base);
  int32_t getFreqAdjBase();

  void setItuRegion(uint8_t rgn);
  uint8_t getItuRegion();

  void getAutokeyTextCh(uint8_t idx, char &ch);
  void getCallsign(char *callsign);

  bool writeEepromBcd(uint16_t addr, uint8_t len, const uint8_t *data);
  bool readEepromBcd(uint16_t addr, uint8_t len, uint8_t *data);

  void serialSetup();
private:
  uint8_t _tx;
  uint8_t _dial_lock;
  bool _is_vfo;
  int32_t _freq_adj_base;
  uint8_t _rgn;

  Channel *_working_ch;
  Channel _vfo_ch;
  Channel _vfo_ch_saved;
  //Channel _mem[MEM_SIZE];
  Channel _mem_ch;
  int8_t _ch_idx;

  void updateDeviceFreqMode();
};

#define CW_KEY_STRAIGHT (0)
#define CW_KEY_IAMBIC_A_L (1)
#define CW_KEY_IAMBIC_A_R (2)
#define CW_KEY_IAMBIC_B_L (3)
#define CW_KEY_IAMBIC_B_R (4)

class Device {
public:
  Device();
  static void init();
  static void resetAll();

  static void setFreqMode(int32_t freq, uint8_t mode, uint8_t tx);

  static void updateHardware();

  static void setCwTone(int16_t cwTone);
  static int16_t getCwTone();

  static void setCwWpm(uint8_t wpm);
  static uint8_t getCwWpm();
  static uint16_t getCwSpeed();

  static void setCwDelay(uint16_t cwDelay);
  static uint16_t getCwDelay();

  static void setCwKey(uint8_t cwKey);
  static uint8_t getCwKey();

  static void cwKeyDown();
  static void cwKeyUp();

  static void cwTone(uint8_t cwToneState);

  static void startCalibrate10M();
  static void updateCalibrate10M();
  static void stopCalibrate10M(bool save = true);

  static void startCalibrate0beat();
  static void updateCalibrate0beat();
  static void stopCalibrate0beat(bool save = true);

  static void startCalibrateBfo();
  static void updateCalibrateBfo();
  static void stopCalibrateBfo(bool save = true);

  static void loadSettings();
  static void saveSettings();
private:
  static uint32_t _ssbBfo, _cwBfo;
  static uint16_t _cwTone;
  static uint8_t _cwWpm;
  static uint16_t _cwSpeed;
  static uint16_t _cwDelay;
  static uint8_t _cwKey;

  static int32_t _freq;
  static uint8_t _mode, _tx;

  static void setTxFilters(int32_t freq);
};

extern uint32_t usbCarrier;
extern int32_t calibration;

void si5351bx_setfreq(uint8_t clknum, uint32_t fout);
void si5351_set_calibration(int32_t cal);
void initOscillators();

#endif // __RIG_H__
