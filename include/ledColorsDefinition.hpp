#pragma once
#include <FastLED.h>

namespace Prefs
{
  // LED_COLOR_XXX uint32 in hex: RED-GREEN-BLUE
  constexpr uint32_t LED_COLOR_WHITE{ 0xFFFFFF };       //! LED WHITE
  constexpr uint32_t LED_COLOR_BLACK{ 0x0 };            //! LED black
  constexpr uint32_t LED_COLOR_ALERT{ 0xFF0000 };       //! LES RED - ALERT
  constexpr uint32_t LED_COLOR_WARNING{ 0xFAF7A2 };     //! LED ywellow-orange - WARNUNG
  constexpr uint32_t LED_COLOR_FORMATTING{ 0xFFB000 };  //! LED orange - format SPIFFS

  constexpr uint32_t LED_COLOR_WLAN_DISCONN{ 0x323200 };
  constexpr uint32_t LED_COLOR_WLAN_SEARCH{ 0x664106 };
  constexpr uint32_t LED_COLOR_WLAN_CONNECT{ 0x020200 };
  constexpr uint32_t LED_COLOR_WLAN_CONNECT_AND_SYNC{ 0x000200 };
  constexpr uint32_t LED_COLOR_WLAN_FAIL{ 0x0a0000 };
  constexpr uint32_t LED_COLOR_MEASURE_UNKNOWN{ 0x808000 };
  constexpr uint32_t LED_COLOR_MEASURE_ACTION{ 0x705000 };
  constexpr uint32_t LED_COLOR_MEASURE_NOMINAL{ LED_COLOR_WLAN_CONNECT_AND_SYNC };
  constexpr uint32_t LED_COLOR_MEASURE_WARN{ 0xff4000 };
  constexpr uint32_t LED_COLOR_MEASURE_ERROR{ 0x800000 };
  constexpr uint32_t LED_COLOR_MEASURE_CRIT{ 0xff1010 };
  constexpr uint32_t LED_COLOR_HTTP_ACTIVE{ 0x808080 };

}  // namespace Prefs
