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

#ifndef __CAT_TASK_H__
#define __CAT_TASK_H__

#include <fsmos.h>
#include "objs.h"
#include "rig.h"

#define FBC 0xFE // Frame begin char
#define FEC 0xFD // Frame end char
#define NGR 0xFA // Not Good Resp
#define OKR 0xFB // OK Resp
#define CMD(c) c

inline size_t serialReadBytes(byte *c, size_t len) {
#ifdef BOARD_generic_stm32f103c
  return Serial.readBytes((char *)c, len);
#else
  return Serial.readBytes(c, len);
#endif // BOARD_generic_stm32f103c
};


enum CatState {
  CAT_FRAME_BEGIN = (FSM_STATE_USERDEF + 1),
  CAT_ADDRESS,
  CAT_DATA_END,
  CAT_EXEC_CMD,
  CAT_SEND_RESP
};

#define BUF_SIZE (64)

class CatTask : public FsmTask {
public:
  virtual void init() {
    _disabled = false;

    Serial.begin(19200, SERIAL_8N1);
    Serial.flush();
    this->gotoState(CAT_FRAME_BEGIN);
  };

  virtual bool on_state_change(int8_t new_state, int8_t) {
    switch (new_state) {
    case CAT_FRAME_BEGIN:
      _buf_pos = 0;
      break;
    case CAT_EXEC_CMD:
      execCmd();
      break;
    case CAT_SEND_RESP:
      _sent_pos = 0;
    default:
      break;
    }

    return true;
  };

  virtual void in_state(int8_t state) {
    switch (state) {
    case CAT_FRAME_BEGIN:
      readFrameBegin();
      break;
    case CAT_ADDRESS:
      readAddress();
      break;
    case CAT_DATA_END:
      readDataAndEnd();
      break;
    case CAT_SEND_RESP:
      sendResp();
      break;
    default:
      break;
    }
  };

  void setDisabled(bool disabled) {
    _disabled = disabled;
  };

private:
  uint8_t _buf_pos;
  byte _buf[BUF_SIZE * 2]; // for request and for response
  uint8_t _sent_pos;
  bool _disabled;

  void readFrameBegin() {
    while (Serial.available()) {
      byte c;
      serialReadBytes(&c, 1);

      if (c != FBC) {
        // wrong byte
        gotoState(CAT_FRAME_BEGIN);
        break;
      }

      _buf[_buf_pos ++] = c;

      if (_buf_pos == 2) {
        gotoState(CAT_ADDRESS);
        break;
      }
    }
  };

  void readAddress() {
    while (Serial.available()) {
      byte c;
      serialReadBytes(&c, 1);

      _buf[_buf_pos ++] = c;

      if (_buf_pos == 4) {
        gotoState(CAT_DATA_END);
        break;
      }
    }
  };

  void readDataAndEnd() {
    while (Serial.available()) {
      byte c;
      serialReadBytes(&c, 1);

      _buf[_buf_pos ++] = c;

      if (c == FEC) {
        gotoState(CAT_EXEC_CMD);
        break;
      }

      if (_buf_pos == BUF_SIZE) {
        // frame is too long!
        gotoState(CAT_FRAME_BEGIN);
        break;
      }
    }
  };

  void execCmd() {
    if (_disabled) {
      sendNg();
      return;
    }

    switch (CMD(_buf[4])) {
    case 0x00: // set freq - no resp
      break;
    case 0x01: // set mode - no resp
      break;
    case 0x02: // read lower / upper freq
      readFreqRange();
      break;
    case 0x03: // read operating freq
      readOpFreq();
      break;
    case 0x04: // read operating mode
      readOpMode();
      break;
    case 0x05: // set freq
      setOpFreq();
      break;
    case 0x06: // set mode and filter
      setOpMode();
      break;
    case 0x07: // select VFO mode or VFO_A/VFO_B
      setVfo();
      break;
    case 0x08: // set memory
      setMemory();
      break;
    case 0x09: // memory write
      writeMemory();
      break;
    case 0x0A: // memory to vfo
      memoryToVfo();
      break;
    case 0x0B: // memory clear
      clearMemory();
      break;
    case 0x0F: // split
      setSplit();
      break;
    case 0x14: // Level settings
      setLevels();
      break;
    case 0x15: // Read Levels and Status
      readLevels();
      break;
    case 0x16: // Set Various Parameters
      setParams();
      break;
    case 0x19: // read rig id
      getRigId();
      break;
    case 0x1A: // Various rig spec cmds
      do1aCmd();
      break;
    case 0x1C: // transmit on / off
      transmitOnOff();
      break;
    case 0x7F: // ubitx own command
      ubitxCmd();
      break;
    default:
      sendNg();
      break;
    }
  };

  void sendNg() {
    _buf[_buf_pos ++] = FBC;
    _buf[_buf_pos ++] = FBC;
    _buf[_buf_pos ++] = _buf[3];
    _buf[_buf_pos ++] = _buf[2];

    _buf[_buf_pos ++] = NGR;
    _buf[_buf_pos ++] = FEC;

    gotoState(CAT_SEND_RESP);
  };

  void sendOk() {
    _buf[_buf_pos ++] = FBC;
    _buf[_buf_pos ++] = FBC;
    _buf[_buf_pos ++] = _buf[3];
    _buf[_buf_pos ++] = _buf[2];

    _buf[_buf_pos ++] = OKR;
    _buf[_buf_pos ++] = FEC;

    gotoState(CAT_SEND_RESP);
  };

  void sendResp() {
    Serial.write(_buf, _buf_pos);
    gotoState(CAT_FRAME_BEGIN);
  };

  void readFreqRange() {
    _buf[_buf_pos ++] = FBC;
    _buf[_buf_pos ++] = FBC;
    _buf[_buf_pos ++] = _buf[3];
    _buf[_buf_pos ++] = _buf[2];
    _buf[_buf_pos ++] = _buf[4];

    // MIN_FREQ - MAX_FREQ
    freq2bcd(MIN_FREQ, &_buf[_buf_pos]);
    _buf_pos += 5;

    _buf[_buf_pos ++] = '-';

    freq2bcd(MAX_FREQ, &_buf[_buf_pos]);
    _buf_pos += 5;

    _buf[_buf_pos ++] = FEC;

    gotoState(CAT_SEND_RESP);
  };

  void readOpFreq() {
    _buf[_buf_pos ++] = FBC;
    _buf[_buf_pos ++] = FBC;
    _buf[_buf_pos ++] = _buf[3];
    _buf[_buf_pos ++] = _buf[2];
    _buf[_buf_pos ++] = _buf[4];

    freq2bcd(rig.getFreq(), &_buf[_buf_pos]);
    _buf_pos += 5;

    _buf[_buf_pos ++] = FEC;

    gotoState(CAT_SEND_RESP);
  };

  void setOpFreq() {
    rig.setFreq(bcd2freq(&_buf[5]));

    sendOk();
  };

  void readOpMode() {
    _buf[_buf_pos ++] = FBC;
    _buf[_buf_pos ++] = FBC;
    _buf[_buf_pos ++] = _buf[3];
    _buf[_buf_pos ++] = _buf[2];
    _buf[_buf_pos ++] = _buf[4];

    // USB - wide filter
    _buf[_buf_pos ++] = rig.getMode();
    _buf[_buf_pos ++] = FILTER_NORMAL;
    _buf[_buf_pos ++] = FEC;

    gotoState(CAT_SEND_RESP);
  };

  void setOpMode() {
    if (rig.setMode(_buf[5])) {
      sendOk();
    } else {
      sendNg();
    }
  };

  void setVfo() {
    bool isDone = true;

    if (_buf[5] == FEC) {
      rig.selectVfo();
    } else if (_buf[5] == 0x00 || _buf[5] == 0x01) {
      rig.setVfo(_buf[5]);
    } else if (_buf[5] == 0xB0) {
      rig.exchangeVfo();
    } else if (_buf[5] == 0xA0) {
      rig.equalizeVfo();
    } else {
      isDone = false;
    }

    if (isDone) sendOk(); else sendNg();
  };

  void setMemory() {
    bool isDone = true;

    if (_buf[5] == FEC) {
      rig.selectMem();
    } else if (_buf[5] == 0xA0) {
      // select memory bank. ubitx does not support it.
      isDone = false;
    } else {
      // select memory channel
      if (_buf[6] != FEC) {
        // more than 99. ubitx does not support it.
        isDone = false;
      } else {
        uint8_t ch_idx = (_buf[5] & 0x0F) + (_buf[5] >> 4) * 10;

        isDone = rig.selectMemCh(ch_idx);
      }
      isDone = false;
    }

    if (isDone) sendOk(); else sendNg();
  };

  void writeMemory() {
    rig.writeMemory();
    sendOk();
  };

  void memoryToVfo() {
    rig.memoryToVfo();
    sendOk();
  };

  void clearMemory() {
    rig.clearMemory();
    sendOk();
  };

  void setSplit() {
    bool isDone = true;

    if (_buf[5] == 0x00 || _buf[5] == 0x01) {
      rig.setSplit(_buf[5]);
    } else {
      isDone = false;
    }

    if (isDone) sendOk(); else sendNg();
  };

  void setLevels() {
    bool shouldNg = false;

    if (_buf[6] != FEC) {
      // set level, not read. ubitx does not support.
      shouldNg = true;
    } else if (_buf[5] == 0x01 || _buf[5] == 0x02 || _buf[5] == 0x0A || _buf[5] == 0x0B) {
      // AF Level, RF Level, RF Power, MIC Gain - all are 100% (255)
      _buf[_buf_pos ++] = FBC;
      _buf[_buf_pos ++] = FBC;
      _buf[_buf_pos ++] = _buf[3];
      _buf[_buf_pos ++] = _buf[2];
      _buf[_buf_pos ++] = _buf[4];
      _buf[_buf_pos ++] = _buf[5];

      _buf[_buf_pos ++] = 0x02;
      _buf[_buf_pos ++] = 0x55;

      _buf[_buf_pos ++] = FEC;
    } else {
      // Other level settings are not readable.
      shouldNg = true;
    }

    if (shouldNg) {
      sendNg();
    } else {
      gotoState(CAT_SEND_RESP);
    }
  };

  void readLevels() {
    bool shouldNg = false;

    if (_buf[5] == 0x11) {
      // read rf power meter. ubitx always uses 100% power (255).
      _buf[_buf_pos ++] = FBC;
      _buf[_buf_pos ++] = FBC;
      _buf[_buf_pos ++] = _buf[3];
      _buf[_buf_pos ++] = _buf[2];
      _buf[_buf_pos ++] = _buf[4];
      _buf[_buf_pos ++] = _buf[5];

      _buf[_buf_pos ++] = 0x02;
      _buf[_buf_pos ++] = 0x55;

      _buf[_buf_pos ++] = FEC;
    } else {
      shouldNg = true;
    }

    if (shouldNg) {
      sendNg();
    } else {
      gotoState(CAT_SEND_RESP);
    }
  };

  void setParams() {
    bool isDone = true;

    if (_buf[5] == 0x50 && (_buf[6] == 0x00 || _buf[6] == 0x01)) {
      rig.setDialLock(_buf[6]);
    } else {
      isDone = false;
    }

    if (isDone) sendOk(); else sendNg();
  };

  void getRigId() {
    _buf[_buf_pos ++] = FBC;
    _buf[_buf_pos ++] = FBC;
    _buf[_buf_pos ++] = _buf[3];
    _buf[_buf_pos ++] = _buf[2];
    _buf[_buf_pos ++] = _buf[4];

    // returns the address in the request
    _buf[_buf_pos ++] = _buf[2];
    _buf[_buf_pos ++] = FEC;

    gotoState(CAT_SEND_RESP);
  };

  void transmitOnOff() {
    if (_buf[5] == 0x00) { // TX ON/OFF
      if (_buf[6] != FEC) { // Set TX mode
        rig.setTx(_buf[6]);
        sendOk();
      } else { // Read
        _buf[_buf_pos ++] = FBC;
        _buf[_buf_pos ++] = FBC;
        _buf[_buf_pos ++] = _buf[3];
        _buf[_buf_pos ++] = _buf[2];
        _buf[_buf_pos ++] = _buf[4];
        _buf[_buf_pos ++] = _buf[5];

        _buf[_buf_pos ++] = rig.getTx();
        _buf[_buf_pos ++] = FEC;

        gotoState(CAT_SEND_RESP);
      }
    } else  {
      sendNg();
    }
  };

  void do1aCmd() {
    if (_buf[5] == 0x05 && _buf[6] == 0x00 && _buf[7] == 0x92 && _buf[8] == 0x00) {
      // 1A 05 00 92 00- OmniRig init command. It expects OK.
      sendOk();
    } else if (_buf[5] == 0x03 && _buf[6] == FEC) {
      // IF Filter width
      _buf[_buf_pos ++] = FBC;
      _buf[_buf_pos ++] = FBC;
      _buf[_buf_pos ++] = _buf[3];
      _buf[_buf_pos ++] = _buf[2];
      _buf[_buf_pos ++] = _buf[4];
      _buf[_buf_pos ++] = _buf[5];

      _buf[_buf_pos ++] = 0x31; // 2.7KHz
      _buf[_buf_pos ++] = FEC;

      gotoState(CAT_SEND_RESP);
    } else {
      sendNg();
    }
  };

  void ubitxCmd() {
    // 05 06 07      08      09             10 + LEN * 3
    // F5 05 ADDR_HI ADDR_LO LEN {CONTENTS} FEC
    // Contents is in BCD
    // BUF_SIZE is 64. So the max of LEN is ((64 - 22) / 2) = 21
    // we choose 16 as the max length

    if (_buf[5] == 0xF5 && _buf[6] == 0x05) { // $F505 - magic number
      uint16_t addr = 0;
      addr += (_buf[7] & 0x0F) * 100;
      addr += (_buf[8] >> 4) * 10;
      addr += (_buf[8] & 0x0F);
      uint8_t len = _buf[9];

      bool isRead = _buf[10] == FEC;

      if (isRead) { // read eeprom
        if (rig.readEepromBcd(addr, len, _buf + 21)) {
          _buf[_buf_pos ++] = FBC;
          _buf[_buf_pos ++] = FBC;
          _buf[_buf_pos ++] = _buf[3];
          _buf[_buf_pos ++] = _buf[2];
          _buf[_buf_pos ++] = _buf[4];
          _buf[_buf_pos ++] = _buf[5];
          _buf[_buf_pos ++] = _buf[6];
          _buf[_buf_pos ++] = _buf[7];
          _buf[_buf_pos ++] = _buf[8];
          _buf[_buf_pos ++] = _buf[9];

          _buf_pos += (len * 2);

          _buf[_buf_pos ++] = FEC;

          gotoState(CAT_SEND_RESP);
        } else {
          sendNg();
        }
      } else { // write eeprom
        if (rig.writeEepromBcd(addr, len, _buf + 10)) {
          sendOk();
        } else {
          sendNg();
        }
      }
    } else {
      sendNg();
    }
  }

  void freq2bcd(int32_t freq, byte *bcd) {
    uint8_t lo, hi;
    for (int8_t i = 0; i < 5; i ++) {
      lo = freq % 10;
      freq /= 10;
      hi = freq % 10;
      freq /= 10;

      bcd[i] = (hi << 4) + lo;
    }
  };

  int32_t bcd2freq(const byte *bcd) {
    int32_t ret_val = 0;
    uint8_t lo, hi;
    for (int8_t i = 4; i >= 0; i --) {
      lo = bcd[i] & 0x0F;
      hi = (bcd[i] >> 4) & 0x0F;

      ret_val = ret_val * 100 + hi * 10 + lo;
    }

    return ret_val;
  };
};

#endif // __CAT_TASK_H__
