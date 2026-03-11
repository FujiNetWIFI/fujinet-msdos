#include <stdint.h>

typedef struct {
  uint8_t query;
  char signature[4];
  uint8_t unit;
} fuji_ioctl_query;
