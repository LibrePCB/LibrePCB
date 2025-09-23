/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
 * https://librepcb.org/
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <gtest/gtest.h>
#include <librepcb/editor/guiapplication.h>

#include <QtTest>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {
namespace tests {

/*******************************************************************************
 *  Test Class
 ******************************************************************************/

class GuiApplicationTest : public ::testing::Test {
  Q_DECLARE_TR_FUNCTIONS(ui::GuiApplicationTest)

protected:
  QTranslator* mTranslator = nullptr;

  GuiApplicationTest() : mTranslator(new QTranslator(qApp)) {
    if (!mTranslator->load(TEST_DATA_DIR "/i18n/unittests_de.qm")) {
      throw std::runtime_error("Failed to load unittests_de.qm");
    }
    qApp->installTranslator(mTranslator);

    // Declare translated strings to be picked up by lupdate. The alias notr()
    // is defined for the test translation process, but it's not picked up for
    // the production translation process.
    notr("Untranslated String");
    notr("Translated String");
    notr("Translated String ‒ With Unicode ☺");
    notr("Translated String %1 of %2");
    notr("Translated %n String(s)");
  }

  ~GuiApplicationTest() {
    if (!qApp->removeTranslator(mTranslator)) {
      qWarning() << "Failed to remove translator.";
    }
  }

  static const char* notr(const char* s) noexcept { return s; }

  static slint::private_api::Slice<uint8_t> toSlice(const char* s) noexcept {
    return slint::private_api::Slice<uint8_t>{
        reinterpret_cast<uint8_t*>(const_cast<char*>(s)), std::strlen(s)};
  }
};

/*******************************************************************************
 *  Test Methods
 ******************************************************************************/

TEST_F(GuiApplicationTest, testTranslationQt) {
  // Sanity check that the translation has been successfully loaded.
  // Important: Use variables to avoid these string picked up in the production
  // translation process.
  const char* context = "ui::GuiApplicationTest";
  const char* key = "Translated String";
  const QString out = QCoreApplication::translate(context, key);
  EXPECT_EQ("Übersetzter Text", out.toStdString());
}

TEST_F(GuiApplicationTest, testTranslationUntranslated) {
  slint::SharedString out;
  slintTr(toSlice("Untranslated String"), toSlice("GuiApplicationTest"),
          toSlice("Domain"), 0, toSlice(""), &out);
  EXPECT_EQ("Untranslated String", static_cast<std::string_view>(out));
}

TEST_F(GuiApplicationTest, testTranslationSingular) {
  slint::SharedString out;
  slintTr(toSlice("Translated String"), toSlice("GuiApplicationTest"),
          toSlice("Domain"), 0, toSlice(""), &out);
  EXPECT_EQ("Übersetzter Text", static_cast<std::string_view>(out));
}

TEST_F(GuiApplicationTest, testTranslationPluralZero) {
  slint::SharedString out;
  slintTr(toSlice("Translated %n String(s)"), toSlice("GuiApplicationTest"),
          toSlice("Domain"), 0, toSlice("Translated %n String(s)"), &out);
  EXPECT_EQ("Übersetzte 0 Texte", static_cast<std::string_view>(out));
}

TEST_F(GuiApplicationTest, testTranslationPluralOne) {
  slint::SharedString out;
  slintTr(toSlice("Translated %n String(s)"), toSlice("GuiApplicationTest"),
          toSlice("Domain"), 1, toSlice("Translated %n String(s)"), &out);
  EXPECT_EQ("Übersetzter 1 Text", static_cast<std::string_view>(out));
}

TEST_F(GuiApplicationTest, testTranslationPluralFive) {
  slint::SharedString out;
  slintTr(toSlice("Translated %n String(s)"), toSlice("GuiApplicationTest"),
          toSlice("Domain"), 5, toSlice("Translated %n String(s)"), &out);
  EXPECT_EQ("Übersetzte 5 Texte", static_cast<std::string_view>(out));
}

TEST_F(GuiApplicationTest, testTranslationPlaceholders) {
  slint::SharedString out;
  slintTr(toSlice("Translated String %1 of %2"), toSlice("GuiApplicationTest"),
          toSlice("Domain"), 0, toSlice(""), &out);
  EXPECT_EQ("Übersetzter Text {0} von {1}", static_cast<std::string_view>(out));
}

TEST_F(GuiApplicationTest, testTranslationWithUnicode) {
  slint::SharedString out;
  slintTr(toSlice("Translated String ‒ With Unicode ☺"),
          toSlice("GuiApplicationTest"), toSlice("Domain"), 0, toSlice(""),
          &out);
  EXPECT_EQ("Übersetzter Text ‒ Mit Unicode ☺",
            static_cast<std::string_view>(out));
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace editor
}  // namespace librepcb
