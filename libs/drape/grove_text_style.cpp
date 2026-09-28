#include "drape/grove_text_style.hpp"

#include <unicode/locid.h>
#include <unicode/uchar.h>
#include <unicode/unistr.h>

namespace grove
{
namespace
{
// U+F0000 in UTF-8 is F3 B0 80 80; the flags go into the last byte's low 6 bits.
std::string_view constexpr kMarkerLead = "\xF3\xB0\x80";
uint8_t constexpr kMarkerBase = 0x80;

std::string_view constexpr kRegularFont = "fonts/08_inter_medium.ttf";
std::string_view constexpr kSemiboldFont = "fonts/08_inter_semibold.ttf";
std::string_view constexpr kItalicFont = "fonts/08_inter_medium_italic.ttf";

// About 0.09 em at the glyph manager's base font size, like Apple's small spaced capitals.
int32_t constexpr kTrackingPixels = 2;

// Upper-cases text for kSpacedCaps and returns true, or returns false if the text has no letter case.
bool ToCaps(std::string_view text, std::string & caps)
{
  auto s = icu::UnicodeString::fromUTF8(icu::StringPiece(text.data(), static_cast<int32_t>(text.size())));
  for (int32_t i = 0; i < s.length(); i = s.moveIndex32(i, 1))
  {
    if (u_hasBinaryProperty(s.char32At(i), UCHAR_CASED))
    {
      s.toUpper(icu::Locale::getRoot()).toUTF8String(caps);
      return true;
    }
  }
  return false;
}
}  // namespace

std::string ApplyTextStyle(uint8_t style, std::string_view text)
{
  std::string styled;
  if (!(style & kSpacedCaps) || !ToCaps(text, styled))
  {
    style &= ~kSpacedCaps;
    styled = text;
  }

  if (style == kPlain)
    return styled;

  std::string result(kMarkerLead);
  result += static_cast<char>(kMarkerBase | style);
  return result + styled;
}

uint8_t TakeTextStyle(std::string_view & utf8)
{
  if (utf8.size() <= kMarkerLead.size() || !utf8.starts_with(kMarkerLead))
    return kPlain;

  auto const last = static_cast<uint8_t>(utf8[kMarkerLead.size()]);
  if ((last & 0xC0) != kMarkerBase)
    return kPlain;

  utf8.remove_prefix(kMarkerLead.size() + 1);
  return last & 0x3F;
}

void ApplyTracking(uint8_t style, dp::text::TextMetrics & metrics)
{
  if (!(style & kSpacedCaps))
    return;

  // No spacing after the last glyph, nor after zero-width marks, which sit on the previous letter.
  for (size_t i = 0; i + 1 < metrics.m_glyphs.size(); ++i)
  {
    auto & glyph = metrics.m_glyphs[i];
    if (glyph.m_xAdvance > 0)
    {
      glyph.m_xAdvance += kTrackingPixels;
      metrics.m_lineWidthInPixels += kTrackingPixels;
    }
  }
}

bool FontVariants::OnFontLoaded(std::string_view fileName, int fontIndex)
{
  if (fileName == kRegularFont)
    m_regular = fontIndex;
  else if (fileName == kSemiboldFont)
    m_semibold = fontIndex;
  else if (fileName == kItalicFont)
    m_italic = fontIndex;
  else
    return false;

  return fontIndex != m_regular;
}
}  // namespace grove
