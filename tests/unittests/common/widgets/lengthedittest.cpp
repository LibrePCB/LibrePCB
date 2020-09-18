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
#include <librepcb/common/widgets/lengthedit.h>

#include <QtTest/QtTest>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace tests {

/*******************************************************************************
 *  Test Class
 ******************************************************************************/
class LengthEditTest : public ::testing::Test, public QObject {
protected:
  void startListening() {
    connect(&edit, &LengthEdit::valueChanged, this,
            &LengthEditTest::valueChanged);
  }

  void valueChanged(const Length& value) noexcept {
    emittedValues.append(value);
  }

  LengthEdit edit;
  QVector<Length> emittedValues;
};

/*******************************************************************************
 *  Test Methods
 ******************************************************************************/

TEST_F(LengthEditTest, testStep) {
  edit.setSteps({
      PositiveLength(100000),  // 0.1mm
      PositiveLength(254000),  // 0.254mm
      PositiveLength(1000000),  // 1mm
      PositiveLength(2540000),  // 2.54mm
  });
  edit.setValue(Length(3000000));  // 3mm
  startListening();

  // Step down from 3mm to -3mm
  QVector<Length> expectedValues = {
      Length(2000000), Length(1000000),  Length(900000),   Length(800000),
      Length(700000),  Length(600000),   Length(500000),   Length(400000),
      Length(300000),  Length(200000),   Length(100000),   Length(0),
      Length(-100000), Length(-200000),  Length(-300000),  Length(-400000),
      Length(-500000), Length(-600000),  Length(-700000),  Length(-800000),
      Length(-900000), Length(-1000000), Length(-2000000), Length(-3000000),
  };
  for (int i = 0; i < expectedValues.count(); ++i) {
    edit.stepDown();
    EXPECT_EQ(expectedValues[i].toNm(), edit.getValue().toNm());
    ASSERT_EQ(1, emittedValues.count());
    EXPECT_EQ(expectedValues[i].toNm(), emittedValues.takeLast().toNm());
  }

  // Step up from -3mm to 3mm
  expectedValues = {
      Length(-2000000), Length(-1000000), Length(-900000), Length(-800000),
      Length(-700000),  Length(-600000),  Length(-500000), Length(-400000),
      Length(-300000),  Length(-200000),  Length(-100000), Length(0),
      Length(100000),   Length(200000),   Length(300000),  Length(400000),
      Length(500000),   Length(600000),   Length(700000),  Length(800000),
      Length(900000),   Length(1000000),  Length(2000000), Length(3000000),
  };
  for (int i = 0; i < expectedValues.count(); ++i) {
    edit.stepUp();
    EXPECT_EQ(expectedValues[i].toNm(), edit.getValue().toNm());
    ASSERT_EQ(1, emittedValues.count());
    EXPECT_EQ(expectedValues[i].toNm(), emittedValues.takeLast().toNm());
  }
}

TEST_F(LengthEditTest, testValueChangedWhileTyping) {
  edit.selectAll();
  startListening();
  QTest::keyClicks(&edit, "12+3um");
  QTest::keyClick(&edit, Qt::Key_Enter);

  QVector<Length> expectedValues = {
      Length(1000000),  // "1" -> 1mm
      Length(12000000),  // "12" -> 12mm
      Length(15000000),  // "12+3" -> 15mm
      Length(15000),  // "12+3um" -> 15 um
  };

  ASSERT_EQ(expectedValues.count(), emittedValues.count());
  for (int i = 0; i < expectedValues.count(); ++i) {
    EXPECT_EQ(expectedValues[i].toNm(), emittedValues[i].toNm());
  }
  EXPECT_EQ(expectedValues.last().toNm(), edit.getValue().toNm());
}

TEST_F(LengthEditTest, testUnitUpdatedWhileTyping) {
  edit.selectAll();
  QTest::keyClicks(&edit, "12+3um");
  EXPECT_EQ(LengthUnit::micrometers(), edit.getDisplayedUnit());
}

TEST_F(LengthEditTest, testTextReplacedAfterPressingEnter) {
  edit.selectAll();

  QTest::keyClicks(&edit, " (-1/2) in ");
  EXPECT_EQ(-12700000, edit.getValue().toNm());
  EXPECT_EQ(" (-1/2) in ", edit.text().toStdString());

  QTest::keyClick(&edit, Qt::Key_Enter);
  EXPECT_EQ(-12700000, edit.getValue().toNm());
  EXPECT_EQ("-0.5 ″", edit.text().toStdString());
}

TEST_F(LengthEditTest, testDivisionByZero) {
  edit.selectAll();
  QTest::keyClicks(&edit, "5/0");
  EXPECT_EQ(5000000, edit.getValue().toNm());
  // Note: It results in 5mm because the term "5" was the last valid value
  // entered in the text field.
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace librepcb
