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

}  // namespace Prefs
