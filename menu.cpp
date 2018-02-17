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

#include "menu.h"
#include "rig.h"
#include "objs.h"

// menu mode
void select_menu_mode(int8_t val) {
  switch (val) {
  case 0:
    rig.setMode(MODE_CW);
    break;
  case 1:
    rig.setMode(MODE_CWR);
    break;
  case 2:
    rig.setMode(MODE_LSB);
    break;
  case 3:
    rig.setMode(MODE_USB);
    break;
  default:
    break;
  }
}

void format_menu_value_mode(char *buf, int8_t val) {
  switch (val) {
  case 0:
    sprintf(buf, "CW");
    break;
  case 1:
    sprintf(buf, "CWR");
    break;
  case 2:
    sprintf(buf, "LSB");
    break;
  case 3:
    sprintf(buf, "USB");
    break;
  default:
    sprintf(buf, "N/A");
    break;
  }
}

int8_t get_menu_value_mode() {
  int8_t result = -1;

  switch (rig.getMode()) {
  case MODE_CW:
    result = 0;
    break;
  case MODE_CWR:
    result = 1;
    break;
  case MODE_LSB:
    result = 2;
    break;
  case MODE_USB:
    result = 3;
    break;
  default:
    break;
  }

  return result;
}

// menu A/B
void select_menu_exchange_vfo(int8_t) {
  rig.exchangeVfo(false);
}

// menu A=B
void select_menu_equalize_vfo(int8_t) {
  rig.equalizeVfo(false);
}

// menu split
void select_menu_split(int8_t) {
  rig.setSplit(rig.getSplit() == ON ? OFF : ON, false);
}

void format_menu_split(char *buf) {
  sprintf(buf, "Split %s", rig.getSplit() == ON ? "OFF" : "ON");
}

// menu V/M
void select_menu_vm(int8_t) {
  int8_t ch;
  if (rig.isVfo()) {
    if (!rig.isMemOk()) {
      ch = rig.getNextMemOkCh(rig.getMemCh());
      if (ch != -1) {
        rig.selectMemCh(ch, false);
        rig.selectMem(false);
      }
    }
    rig.selectMem(false);
  } else {
    rig.selectVfo(false);
  }
}

// menu M->V, MW, MC
void select_menu_mem_to_vfo(int8_t val) {
  rig.selectMemCh(val, false);
  rig.memoryToVfo(val, false);
  rig.selectVfo(false);
}

void select_menu_mem_write(int8_t val) {
  rig.selectMemCh(val, false);
  rig.writeMemory(val, false);
}

void select_menu_mem_clear(int8_t val) {
  rig.selectMemCh(val, false);
  rig.clearMemory(false);
}

int8_t get_menu_value_mem_ch() {
  return rig.getMemCh();
}

int8_t get_menu_value_mem_ok_ch() {
  int8_t result;
  if (rig.isMemOk()) {
    result = rig.getMemCh();
  } else {
    result = rig.getNextMemOkCh(rig.getMemCh());
  }

  return result;
}

void format_menu_value_mem_ch(char *buf, int8_t val) {
  if (val >= 0)
    sprintf(buf, rig.isMemOk(val) ? "M%02d" : "?%02d", val);
  else
    sprintf(buf, "N/A");
}

int8_t get_next_menu_value_mem_ok_ch(int8_t val, bool forward) {
  int8_t result;

  if (forward) result = rig.getNextMemOkCh(val);
  else result = rig.getPrevMemOkCh(val);

  return result;
}

const Menu_Item menu[] PROGMEM = {
// text submenu_count  select_menu_f             format_menu_f      get_menu_value_f          format_menu_value_f       get_next_menu_value_f
  {"Mode",          4, select_menu_mode,         NULL,              get_menu_value_mode,      format_menu_value_mode,   NULL                         },
  {"A/B",           0, select_menu_exchange_vfo, NULL,              NULL,                     NULL,                     NULL                         },
  {"A=B",           0, select_menu_equalize_vfo, NULL,              NULL,                     NULL,                     NULL                         },
  {"Split",         0, select_menu_split,        format_menu_split, NULL,                     NULL,                     NULL                         },
  {"V/M",           0, select_menu_vm,           NULL,              NULL,                     NULL,                     NULL                         },
  {"M\x7eV", MEM_SIZE, select_menu_mem_to_vfo,   NULL,              get_menu_value_mem_ok_ch, format_menu_value_mem_ch, get_next_menu_value_mem_ok_ch},
  {"MW",     MEM_SIZE, select_menu_mem_write,    NULL,              get_menu_value_mem_ch,    format_menu_value_mem_ch, NULL                         },
  {"MC",     MEM_SIZE, select_menu_mem_clear,    NULL,              get_menu_value_mem_ok_ch, format_menu_value_mem_ch, get_next_menu_value_mem_ok_ch},
  {"Exit Menu",     0, NULL,                     NULL,              NULL,                     NULL,                     NULL                         }
};

const uint8_t menu_item_count = sizeof(menu) / sizeof(menu[0]);

