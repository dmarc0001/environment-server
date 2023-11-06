#pragma once
#include <stdint.h>
#include <stdbool.h>

/// RGB color representation
typedef struct
{
  union
  {
    uint8_t r;
    uint8_t red;
  };
  union
  {
    uint8_t g;
    uint8_t green;
  };
  union
  {
    uint8_t b;
    uint8_t blue;
  };
} rgb_t;

/// Create rgb_t color from 24-bit color code 0x00RRGGBB
static inline rgb_t rgb_from_code( uint32_t color_code )
{
  rgb_t res = {
      .r = ( uint8_t ) ( ( color_code >> 16 ) & 0xff ),
      .g = ( uint8_t ) ( ( color_code >> 8 ) & 0xff ),
      .b = ( uint8_t ) ( color_code & 0xff ),
  };
  return res;
}

/// Create rgb_t color from values
static inline rgb_t rgb_from_values( uint8_t r, uint8_t g, uint8_t b )
{
  rgb_t res = {
      .r = r,
      .g = g,
      .b = b,
  };
  return res;
}

/// Convert RGB color to 24-bit color code 0x00RRGGBB
static inline uint32_t rgb_to_code( rgb_t color )
{
  return ( ( uint32_t ) color.r << 16 ) | ( ( uint32_t ) color.g << 8 ) | color.b;
}
