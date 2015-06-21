#include "pebble.h"
  
static const GPathInfo MINUTE_HAND_POINTS = {
  4,
  (GPoint []) {
    { -3, 16 },
    { 5, 16 },
    { 4, -66 },
    { -2, -66 }
  }
};

static const GPathInfo HOUR_HAND_POINTS = {
  4,
  (GPoint []) {
    { -3, 16 },
    { 5, 16 },
    { 4, -45 },
    { -2, -45 }
  }
};
