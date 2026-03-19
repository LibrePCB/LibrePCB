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
#include <librepcb/editor/utils/slinthelpers.h>

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

class SlintHelpersTest : public ::testing::Test {
  Q_DECLARE_TR_FUNCTIONS(ui::SlintHelpersTest)

protected:
  std::unique_ptr<QTranslator> mTranslator;

  SlintHelpersTest() : mTranslator(new QTranslator(qApp)) {
    if (!mTranslator->load(TEST_DATA_DIR "/i18n/unittests_de.qm")) {
      throw std::runtime_error("Failed to load unittests_de.qm");
    }
    qApp->installTranslator(mTranslator.get());

    // Declare translated strings to be picked up by lupdate. The alias notr()
    // is defined for the test translation process, but it's not picked up for
    // the production translation process.
    notr("Untranslated String");
    notr("Translated String");
    notr("Translated String ‒ With Unicode ☺");
    notr("Translated String %1 of %2");
    notr("Translated %n String(s)", nullptr, 5);
  }

  ~SlintHelpersTest() {
    if (!qApp->removeTranslator(mTranslator.get())) {
      qWarning() << "Failed to remove translator.";
    }
  }

  static const char* notr(const char* s, const char* c = nullptr,
                          int n = 0) noexcept {
    Q_UNUSED(c);
    Q_UNUSED(n);
    return s;
  }
};

/*******************************************************************************
 *  Test Methods
 ******************************************************************************/

TEST_F(SlintHelpersTest, testTranslationQt) {
  // Sanity check that the translation has been successfully loaded.
  // Important: Use variables to avoid these string picked up in the production
  // translation process.
  const char* context = "ui::SlintHelpersTest";
  const char* key = "Translated String";
  const QString out = QCoreApplication::translate(context, key);
  EXPECT_EQ("Übersetzter Text", out.toStdString());
}

TEST_F(SlintHelpersTest, testTranslationUntranslated) {
  SlintTranslator tr;
  slint::SharedString out =
      tr.translate("Untranslated String", "SlintHelpersTest");
  EXPECT_EQ("Untranslated String", static_cast<std::string_view>(out));
}

TEST_F(SlintHelpersTest, testTranslationSingular) {
  SlintTranslator tr;
  slint::SharedString out =
      tr.translate("Translated String", "SlintHelpersTest");
  EXPECT_EQ("Übersetzter Text", static_cast<std::string_view>(out));
}

TEST_F(SlintHelpersTest, testTranslationPluralZero) {
  SlintTranslator tr;
  slint::SharedString out =
      tr.ntranslate(0, "", "Translated %n String(s)", "SlintHelpersTest");
  EXPECT_EQ("Übersetzte 0 Texte", static_cast<std::string_view>(out));
}

TEST_F(SlintHelpersTest, testTranslationPluralOne) {
  SlintTranslator tr;
  slint::SharedString out =
      tr.ntranslate(1, "", "Translated %n String(s)", "SlintHelpersTest");
  EXPECT_EQ("Übersetzter 1 Text", static_cast<std::string_view>(out));
}

TEST_F(SlintHelpersTest, testTranslationPluralFive) {
  SlintTranslator tr;
  slint::SharedString out =
      tr.ntranslate(5, "", "Translated %n String(s)", "SlintHelpersTest");
  EXPECT_EQ("Übersetzte 5 Texte", static_cast<std::string_view>(out));
}

TEST_F(SlintHelpersTest, testTranslationPlaceholders) {
  SlintTranslator tr;
  slint::SharedString out =
      tr.translate("Translated String %1 of %2", "SlintHelpersTest");
  EXPECT_EQ("Übersetzter Text {0} von {1}", static_cast<std::string_view>(out));
}

TEST_F(SlintHelpersTest, testTranslationWithUnicode) {
  SlintTranslator tr;
  slint::SharedString out =
      tr.translate("Translated String ‒ With Unicode ☺", "SlintHelpersTest");
  EXPECT_EQ("Übersetzter Text ‒ Mit Unicode ☺",
            static_cast<std::string_view>(out));
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace editor
}  // namespace librepcb
