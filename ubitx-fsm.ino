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

#include <fsmos.h>
#include "objs.h"
#include "cat_task.h"
#include "display_task.h"
#include "rig.h"
#include "led_def.h"

FsmOs fsmOs(2);

void setup() {
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LED_OFF_VALUE);

  fsmOs.addTask(&displayTask);
  fsmOs.addTask(&catTask);

  fsmOs.init();
}

void loop() {
  fsmOs.loop();
}
