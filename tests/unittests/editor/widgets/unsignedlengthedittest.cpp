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
#include <librepcb/editor/widgets/unsignedlengthedit.h>

#include <QtTest/QtTest>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {
namespace tests {

/*******************************************************************************
 *  Test Class
 ******************************************************************************/
class UnsignedLengthEditTest : public ::testing::Test, public QObject {
protected:
  void startListening() {
    connect(&edit, &UnsignedLengthEdit::valueChanged, this,
            &UnsignedLengthEditTest::valueChanged);
  }

  void valueChanged(const UnsignedLength& value) noexcept {
    emittedValues.push_back(value);
  }

  UnsignedLengthEdit edit;
  std::vector<UnsignedLength> emittedValues;
};

/*******************************************************************************
 *  Test Methods
 ******************************************************************************/

TEST_F(UnsignedLengthEditTest, testStep) {
  edit.setSteps({
      PositiveLength(100000),  // 0.1mm
      PositiveLength(254000),  // 0.254mm
      PositiveLength(1000000),  // 1mm
      PositiveLength(2540000),  // 2.54mm
  });
  edit.setValue(UnsignedLength(3000000));  // 3mm
  startListening();

  // Step down from 3mm to 0mm
  std::vector<UnsignedLength> expectedValues = {
      UnsignedLength(2000000), UnsignedLength(1000000), UnsignedLength(900000),
      UnsignedLength(800000),  UnsignedLength(700000),  UnsignedLength(600000),
      UnsignedLength(500000),  UnsignedLength(400000),  UnsignedLength(300000),
      UnsignedLength(200000),  UnsignedLength(100000),  UnsignedLength(0),
  };
  for (size_t i = 0; i < expectedValues.size(); ++i) {
    edit.stepDown();
    EXPECT_EQ(expectedValues[i]->toNm(), edit.getValue()->toNm());
    ASSERT_EQ(i + 1, emittedValues.size());
    EXPECT_EQ(expectedValues[i]->toNm(), emittedValues[i]->toNm());
  }
  emittedValues.clear();

  // Step down one more time -> must do nothing, i.e. value stays at 0mm.
  edit.stepDown();
  EXPECT_EQ(0, edit.getValue()->toNm());

  // Step up from 0mm to 3mm
  expectedValues = {
      UnsignedLength(100000),  UnsignedLength(200000),  UnsignedLength(300000),
      UnsignedLength(400000),  UnsignedLength(500000),  UnsignedLength(600000),
      UnsignedLength(700000),  UnsignedLength(800000),  UnsignedLength(900000),
      UnsignedLength(1000000), UnsignedLength(2000000), UnsignedLength(3000000),
  };
  for (size_t i = 0; i < expectedValues.size(); ++i) {
    edit.stepUp();
    EXPECT_EQ(expectedValues[i]->toNm(), edit.getValue()->toNm());
    ASSERT_EQ(i + 1, emittedValues.size());
    EXPECT_EQ(expectedValues[i]->toNm(), emittedValues[i]->toNm());
  }
}

TEST_F(UnsignedLengthEditTest, testValueChangedWhileTyping) {
  edit.selectAll();
  startListening();
  QTest::keyClicks(&edit, "12+3um");
  QTest::keyClick(&edit, Qt::Key_Enter);

  std::vector<UnsignedLength> expectedValues = {
      UnsignedLength(1000000),  // "1" -> 1mm
      UnsignedLength(12000000),  // "12" -> 12mm
      UnsignedLength(15000000),  // "12+3" -> 15mm
      UnsignedLength(15000),  // "12+3um" -> 15 um
  };

  ASSERT_EQ(expectedValues.size(), emittedValues.size());
  for (size_t i = 0; i < expectedValues.size(); ++i) {
    EXPECT_EQ(expectedValues[i]->toNm(), emittedValues[i]->toNm());
  }
  EXPECT_EQ(expectedValues.back()->toNm(), edit.getValue()->toNm());
}

TEST_F(UnsignedLengthEditTest, testUnitUpdatedWhileTyping) {
  edit.selectAll();
  QTest::keyClicks(&edit, "12+3um");
  EXPECT_EQ(LengthUnit::micrometers(), edit.getDisplayedUnit());
}

TEST_F(UnsignedLengthEditTest, testTextReplacedAfterPressingEnter) {
  edit.selectAll();

  QTest::keyClicks(&edit, " (1/2) in ");
  EXPECT_EQ(12700000, edit.getValue()->toNm());
  EXPECT_EQ(" (1/2) in ", edit.text().toStdString());

  QTest::keyClick(&edit, Qt::Key_Enter);
  EXPECT_EQ(12700000, edit.getValue()->toNm());
  EXPECT_EQ("0.5 ″", edit.text().toStdString());
}

TEST_F(UnsignedLengthEditTest, testDivisionByZero) {
  edit.selectAll();
  QTest::keyClicks(&edit, "5/0");
  EXPECT_EQ(5000000, edit.getValue()->toNm());
  // Note: It results in 5mm because the term "5" was the last valid value
  // entered in the text field.
}

TEST_F(UnsignedLengthEditTest, testTooSmallValue) {
  edit.setValue(UnsignedLength(1000000));
  edit.selectAll();

  QTest::keyClicks(&edit, "-5");
  EXPECT_EQ("-5", edit.text().toStdString());  // text entered...
  EXPECT_EQ(1000000, edit.getValue()->toNm());  // ...but value not updated

  QTest::keyClick(&edit, Qt::Key_Enter);
  EXPECT_EQ("1.0 mm", edit.text().toStdString());  // text reverted...
  EXPECT_EQ(1000000, edit.getValue()->toNm());  // ...to the actual value
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace editor
}  // namespace librepcb
