#pragma once

#include "pebble.h"

#define NUM_CLOCK_TICKS 11
#ifdef PBL_PLATFORM_EMERY
  #define DAY_LABEL_FONT FONT_KEY_GOTHIC_24
  #define NUM_LABEL_FONT FONT_KEY_GOTHIC_24_BOLD
#else
  #define DAY_LABEL_FONT FONT_KEY_GOTHIC_18
  #define NUM_LABEL_FONT FONT_KEY_GOTHIC_18_BOLD
#endif
//All base points defined for 144x168 screen, will be transformed for other screen sizes in code
#define BASE_DAY_LABEL_OFFSET_x -30
#define BASE_DAY_LABEL_OFFSET_y 30
#define BASE_NUM_LABEL_OFFSET_x 1
#define BASE_NUM_LABEL_OFFSET_y 30
#define BASE_LABEL_WIDTH_DAY 27
#define BASE_LABEL_HEIGHT 20
#define BASE_LABEL_WIDTH_NUM 18

static const struct GPathInfo ANALOG_BG_POINTS[] = {
  { 4,
    (GPoint []) {
      {68, 0},
      {71, 0},
      {71, 12},
      {68, 12}
    }
  },
  { 4, (GPoint []){
      {72, 0},
      {75, 0},
      {75, 12},
      {72, 12}
    }
  },
  { 4, (GPoint []){
      {112, 10},
      {114, 12},
      {108, 23},
      {106, 21}
    }
  },
  { 4, (GPoint []){
      {132, 47},
      {144, 40},
      {144, 44},
      {135, 49}
    }
  },
  { 4, (GPoint []){
      {135, 118},
      {144, 123},
      {144, 126},
      {132, 120}
    }
  },
  { 4, (GPoint []){
      {108, 144},
      {114, 154},
      {112, 157},
      {106, 147}
    }
  },
  { 4, (GPoint []){
      {70, 155},
      {73, 155},
      {73, 167},
      {70, 167}
    }
  },
  { 4, (GPoint []){
      {32, 10},
      {30, 12},
      {36, 23},
      {38, 21}
    }
  },
  { 4, (GPoint []){
      {12, 47},
      {-1, 40},
      {-1, 44},
      {9, 49}
    }
  },
  { 4, (GPoint []){
      {9, 118},
      {-1, 123},
      {-1, 126},
      {12, 120}
    }
  },
  { 4, (GPoint []){
      {36, 144},
      {30, 154},
      {32, 157},
      {38, 147}
    }
  },

};

static const GPathInfo DATE_BOX_POINTS = {
  4,
  (GPoint []) {
    { 71,116 },
    { 91, 116 },
    { 91, 136 },
    { 71, 136}
  }
};

static const GPathInfo MINUTE_HAND_POINTS = {
  3,
  (GPoint []) {
    { -8, 20 },
    { 8, 20 },
    { 0, -80 }
  }
};

static const GPathInfo HOUR_HAND_POINTS = {
  3, (GPoint []){
    {-6, 20},
    {6, 20},
    {0, -60}
  }
};