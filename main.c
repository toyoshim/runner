// Copyright 2021 Takashi Toyoshima <toyoshim@gmail.com>. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdbool.h>
#include <stdint.h>

#include "chlib/ch559.h"
#include "chlib/io.h"
#include "chlib/serial.h"
#include "chlib/timer3.h"

//          | DIP1 | DIP2 | DIP3 | DIP4
//----------+------+------+------+------
// SYNC     |      |      |      |
//   CSYNC  |  OFF |      |      |
//   Timer  |  ON  |      |      |
// Mode     |      |      |      |
//   Single |      |  OFF |      |
//   Double |      |  ON  |      |
// Speed    |      |      |      |
//   10     |      |      |  OFF |  OFF
//   15     |      |      |  OFF |  ON
//   20     |      |      |  ON  |  OFF
//   30     |      |      |  ON  |  ON
static uint16_t last_msec = 0;

uint8_t dipsw() {
  return P0 >> 4;
}

void setup() {
  pinMode(0, 1, INPUT);  // SYNC

  pinMode(0, 4, INPUT_PULLUP);  // DIPSW1
  pinMode(0, 5, INPUT_PULLUP);  // DIPSW2
  pinMode(0, 6, INPUT_PULLUP);  // DIPSW3
  pinMode(0, 7, INPUT_PULLUP);  // DIPSW4

  // Port 1 as P1 inputs w/pull-ups
  P1_PU = 0xff;
  P1_DIR = 0x00;

  // Port 2 as P1 hi-z / output low
  P2_PU = 0x00;
  P2_DIR = 0x00;
  P2 = 0x00;

  // Port 3 as P2 inputs w/pull-ups
  P3_PU = 0xff;
  P3_DIR = 0x00;

  // Port 4 as P2 hi-z / output low
  P4_PU = 0x00;
  P4_DIR = 0x00;
  P4_OUT = 0x00;

  // Initialize rapid-fire timer
  timer3_tick_init();
  last_msec = timer3_tick_msec();
}

bool rapid_fire() {
  static bool on = false;
  uint8_t sw = dipsw();
  if (sw & 1) {
    // CSYNC mode
    static uint8_t csync_count = 0;
    static uint8_t csync_value = 0;
    static uint8_t csync_lpf_count = 0;
    static uint8_t csync_lpf_value = 0;
    if (P0 & 2)
      csync_lpf_value++;
    csync_lpf_count++;
    if (csync_lpf_count == 48) {
      uint8_t csync_new_value = (csync_lpf_value < 24) ? 0 : 1;
      csync_lpf_value = 0;
      csync_lpf_count = 0;
      if (csync_value == 1 && csync_new_value == 0) {
        csync_count += 2;
        uint8_t threshold;
        switch (sw & 0xc) {
          case 0x00:
            threshold = 2;
            break;
          case 0x40:
            threshold = 3;
            break;
          case 0x80:
            threshold = 4;
            break;
          default:
            threshold = 6;
            break;
        }
        if (csync_count >= threshold) {
          csync_count -= threshold;
          on = !on;
        }
      }
      csync_value = csync_new_value;
    }
  } else {
    // Timer mode
    uint16_t step;
    switch (sw & 0xc) {
      case 0x00:
        step = 17;
        break;
      case 0x40:
        step = 25;
        break;
      case 0x80:
        step = 33;
        break;
      default:
        step = 50;
        break;
    }
    if (!timer3_tick_msec_between(last_msec, last_msec + step)) {
      last_msec += step;
      on = !on;
    }
  }
  return on;
}

void main() {
  initialize();
  setup();

  for (;;) {
    uint8_t player1_buttons = P1;
    uint8_t player2_buttons = P3;
    uint8_t mode = dipsw() & 2;
    if (rapid_fire()) {
      if (mode) {
        // Hi-Z, all-off
        P2_DIR = 0x00;
        P4_DIR = 0x00;
      } else {
        // 1-3(1-3 front) off, 4-7(1-3 back) through
        P2_DIR = (~player1_buttons & 0x07) << 3;
        P4_DIR = (~player2_buttons & 0x07) << 3;
      }
    } else {
      if (mode) {
        // All-through
        P2_DIR = ~player1_buttons & 0x3f;
        P4_DIR = ~player2_buttons & 0x3f;
      } else {
        // 1-3(1-3 front) through, 4-7(1-3 back) off
        P2_DIR = ~player1_buttons & 0x07;
        P4_DIR = ~player2_buttons & 0x07;
      }
    }
  }
}