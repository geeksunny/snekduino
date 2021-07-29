#ifndef SNEKDUINO_INCLUDE_QMK_H_
#define SNEKDUINO_INCLUDE_QMK_H_

#include <stdint.h>

#if defined(__GNUC__)
#    define PACKED __attribute__((__packed__))
#else
#    define PACKED
#endif

typedef struct PACKED {
  uint8_t x;
  uint8_t y;
} point_t;

//class qmk {
//
//};

#endif //SNEKDUINO_INCLUDE_QMK_H_
