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
#define MAX_FREQ 29999999

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

class Rig {
public:
  Rig();

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
private:
  uint8_t _tx;
  uint8_t _dial_lock;

  Channel *_working_ch;
  Channel _vfo_ch;
  Channel _mem[MEM_SIZE];
  int8_t _ch_idx;
};

#endif // __RIG_H__
