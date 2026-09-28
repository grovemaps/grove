#include "testing/testing.hpp"

#include "drape/glyph_manager.hpp"
#include "drape/grove_text_style.hpp"

#include "coding/string_utf8_multilang.hpp"

#include "platform/platform.hpp"

#include "base/file_name_utils.hpp"

#include <string>
#include <string_view>

namespace grove_text_style_tests
{
using namespace grove;

uint8_t Take(std::string const & styled, std::string_view & text)
{
  text = styled;
  return TakeTextStyle(text);
}

UNIT_TEST(GroveTextStyle_Marker)
{
  std::string_view text;

  TEST_EQUAL(ApplyTextStyle(kPlain, "Amsterdam"), "Amsterdam", ());
  TEST_EQUAL(Take("Amsterdam", text), kPlain, ());
  TEST_EQUAL(text, "Amsterdam", ());

  std::string const semibold = ApplyTextStyle(kSemibold, "Amsterdam");
  TEST_NOT_EQUAL(semibold, "Amsterdam", ());
  TEST_EQUAL(Take(semibold, text), kSemibold, ());
  TEST_EQUAL(text, "Amsterdam", ());

  TEST_EQUAL(Take(ApplyTextStyle(kItalic, "Het IJ"), text), kItalic, ());
  TEST_EQUAL(text, "Het IJ", ());

  TEST_EQUAL(Take(ApplyTextStyle(kSpacedCaps, "Jordaan"), text), kSpacedCaps, ());
  TEST_EQUAL(text, "JORDAAN", ());

  TEST_EQUAL(Take(ApplyTextStyle(kSemibold | kSpacedCaps, "Straße"), text), kSemibold | kSpacedCaps, ());
  TEST_EQUAL(text, "STRASSE", ());

  TEST_EQUAL(Take(ApplyTextStyle(kSpacedCaps, "Ерево"), text), kSpacedCaps, ());
  TEST_EQUAL(text, "ЕРЕВО", ());
}

UNIT_TEST(GroveTextStyle_CapsOnlyForCasedScripts)
{
  // Spacing would break connected scripts, and there are no capitals to set.
  TEST_EQUAL(ApplyTextStyle(kSpacedCaps, "الكرادة"), "الكرادة", ());
  TEST_EQUAL(ApplyTextStyle(kSpacedCaps, "中关村"), "中关村", ());

  std::string_view text;
  TEST_EQUAL(Take(ApplyTextStyle(kSemibold | kSpacedCaps, "中关村"), text), kSemibold, ());
  TEST_EQUAL(text, "中关村", ());
}

UNIT_TEST(GroveTextStyle_GeistOnlyForAscii)
{
  std::string_view text;
  TEST_EQUAL(Take(ApplyTextStyle(kGeist, "A10"), text), kGeist, ());
  TEST_EQUAL(text, "A10", ());

  // A Cyrillic road number or a house number with a letter Geist may lack stays in the label font.
  TEST_EQUAL(ApplyTextStyle(kGeist, "М10"), "М10", ());
  TEST_EQUAL(ApplyTextStyle(kGeist, "12ä"), "12ä", ());
}

UNIT_TEST(GroveTextStyle_Shaping)
{
  dp::GlyphManager::Params args;
  args.m_uniBlocks = base::JoinPath("fonts", "unicode_blocks.txt");
  args.m_whitelist = base::JoinPath("fonts", "whitelist.txt");
  args.m_blacklist = base::JoinPath("fonts", "blacklist.txt");
  GetPlatform().GetFontNames(args.m_fonts);
  dp::GlyphManager mng(args);

  auto const lang = StringUtf8Multilang::kUnsupportedLanguageCode;

  auto const plain = mng.ShapeText("Den Haag", lang);
  auto const semibold = mng.ShapeText(ApplyTextStyle(kSemibold, "Den Haag"), lang);
  TEST_EQUAL(plain.m_glyphs.size(), 8, ());
  TEST_EQUAL(semibold.m_glyphs.size(), plain.m_glyphs.size(), ());
  for (size_t i = 0; i < plain.m_glyphs.size(); ++i)
  {
    // The space stays regular, so that long labels still wrap on it.
    if (i == 3)
      TEST(semibold.m_glyphs[i].m_key == plain.m_glyphs[i].m_key, ());
    else
      TEST_NOT_EQUAL(semibold.m_glyphs[i].m_key.m_fontIndex, plain.m_glyphs[i].m_key.m_fontIndex, (i));
  }
  auto const italic = mng.ShapeText(ApplyTextStyle(kItalic, "Den Haag"), lang);
  TEST_NOT_EQUAL(italic.m_glyphs[0].m_key.m_fontIndex, plain.m_glyphs[0].m_key.m_fontIndex, ());
  TEST_NOT_EQUAL(italic.m_glyphs[0].m_key.m_fontIndex, semibold.m_glyphs[0].m_key.m_fontIndex, ());

  auto const number = mng.ShapeText("A10", lang);
  auto const geist = mng.ShapeText(ApplyTextStyle(kGeist, "A10"), lang);
  TEST_EQUAL(geist.m_glyphs.size(), 3, ());
  for (size_t i = 0; i < 3; ++i)
  {
    TEST_NOT_EQUAL(geist.m_glyphs[i].m_key.m_fontIndex, number.m_glyphs[i].m_key.m_fontIndex, (i));
    TEST_NOT_EQUAL(geist.m_glyphs[i].m_key.m_fontIndex, semibold.m_glyphs[0].m_key.m_fontIndex, (i));
  }

  // The plain text's cached metrics must not be affected by the styled one.
  TEST(mng.ShapeText("Den Haag", lang).m_glyphs[0].m_key == plain.m_glyphs[0].m_key, ());

  auto const upper = mng.ShapeText("JORDAAN", lang);
  auto const spaced = mng.ShapeText(ApplyTextStyle(kSpacedCaps, "Jordaan"), lang);
  TEST_EQUAL(spaced.m_glyphs.size(), 7, ());
  TEST(spaced.m_glyphs[0].m_key == upper.m_glyphs[0].m_key, ());
  TEST_GREATER(spaced.m_lineWidthInPixels, upper.m_lineWidthInPixels + 6, ());
  TEST_EQUAL(spaced.m_glyphs.back().m_xAdvance, upper.m_glyphs.back().m_xAdvance, ());
}
}  // namespace grove_text_style_tests
