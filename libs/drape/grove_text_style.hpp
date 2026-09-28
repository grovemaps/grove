#pragma once

#include "drape/glyph_manager.hpp"

#include <cstdint>
#include <string>
#include <string_view>

namespace grove
{
// Label typography that Organic Maps' drawing rules can't express: semibold, italic and spaced capitals.
// The style travels as one private-use character (U+F0000 + flags) in front of a caption's text. That way it reaches
// GlyphManager::ShapeText, and its metrics cache key, without new parameters through every text layout class.
enum TextStyle : uint8_t
{
  kPlain = 0,
  kSemibold = 1,
  // Upper case with letter spacing. Applied only to text in a script with letter case.
  kSpacedCaps = 2,
  kItalic = 4,
};

// Returns the text to put in a caption: the style marker, then the text, upper-cased for kSpacedCaps.
std::string ApplyTextStyle(uint8_t style, std::string_view text);

// Removes a style marker from the front of utf8 and returns the style (kPlain if there was no marker).
uint8_t TakeTextStyle(std::string_view & utf8);

// Letter spacing for kSpacedCaps, added to shaped glyph advances.
void ApplyTracking(uint8_t style, dp::text::TextMetrics & metrics);

// Semibold and italic twins of the regular label font. They are loaded like any other font but serve no unicode block
// themselves: styled text swaps them in for the regular font.
class FontVariants
{
public:
  // Returns true for a twin, whose characters must not be assigned to unicode blocks.
  bool OnFontLoaded(std::string_view fileName, int fontIndex);

  // Spaces stay in the regular font: text layout finds line breaks by comparing against its space glyph.
  int Pick(int fontIndex, char32_t c, uint8_t style) const
  {
    if (fontIndex < 0 || fontIndex != m_regular || c == U' ')
      return fontIndex;
    if ((style & kItalic) && m_italic >= 0)
      return m_italic;
    if ((style & kSemibold) && m_semibold >= 0)
      return m_semibold;
    return fontIndex;
  }

private:
  int m_regular = -1;
  int m_semibold = -1;
  int m_italic = -1;
};
}  // namespace grove
