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

#include "../../testhelpers.h"

#include <gtest/gtest.h>
#include <librepcb/core/graphics/graphicslayer.h>
#include <librepcb/core/types/lengthunit.h>
#include <librepcb/editor/dialogs/dxfimportdialog.h>
#include <librepcb/editor/widgets/doublespinbox.h>
#include <librepcb/editor/widgets/lengthedit.h>
#include <librepcb/editor/widgets/unsignedlengthedit.h>

#include <QtTest>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {
namespace tests {

using librepcb::tests::TestHelpers;

/*******************************************************************************
 *  Test Class
 ******************************************************************************/

class DxfImportDialogTest : public ::testing::Test {
protected:
  QList<GraphicsLayer*> mLayers;

  DxfImportDialogTest() {
    mLayers.append(new GraphicsLayer(GraphicsLayer::sBoardOutlines));
    mLayers.append(new GraphicsLayer(GraphicsLayer::sBoardComments));
    mLayers.append(new GraphicsLayer(GraphicsLayer::sTopPlacement));
    mLayers.append(new GraphicsLayer(GraphicsLayer::sTopDocumentation));
    QSettings().clear();
  }
};

/*******************************************************************************
 *  Test Methods
 ******************************************************************************/

TEST_F(DxfImportDialogTest, testLayerName) {
  const int defaultIndex = 1;
  const int newIndex = 2;

  {
    DxfImportDialog dialog(mLayers,
                           GraphicsLayerName(mLayers[defaultIndex]->getName()),
                           true, LengthUnit::millimeters(), "test");

    // Check if the layer combobox contains all layers.
    QComboBox& cbx =
        TestHelpers::getChild<QComboBox>(dialog, "cbxLayer/QComboBox");
    EXPECT_EQ(4, cbx.count());
    for (int i = 0; i < mLayers.count(); ++i) {
      EXPECT_EQ(mLayers[i]->getNameTr().toStdString(),
                cbx.itemText(i).toStdString());
    }

    // Check the default value.
    EXPECT_EQ(defaultIndex, cbx.currentIndex());
    EXPECT_EQ(mLayers[defaultIndex]->getNameTr().toStdString(),
              cbx.currentText().toStdString());
    EXPECT_EQ(mLayers[defaultIndex]->getName().toStdString(),
              dialog.getLayerName()->toStdString());

    // Check if the value can be changed.
    cbx.setCurrentIndex(newIndex);
    EXPECT_EQ(mLayers[newIndex]->getNameTr().toStdString(),
              cbx.currentText().toStdString());
    EXPECT_EQ(mLayers[newIndex]->getName().toStdString(),
              dialog.getLayerName()->toStdString());
  }

  // Check if the setting is saved and restored automatically.
  {
    DxfImportDialog dialog(mLayers,
                           GraphicsLayerName(mLayers[defaultIndex]->getName()),
                           true, LengthUnit::millimeters(), "test");
    QComboBox& cbx =
        TestHelpers::getChild<QComboBox>(dialog, "cbxLayer/QComboBox");
    EXPECT_EQ(newIndex, cbx.currentIndex());
    EXPECT_EQ(mLayers[newIndex]->getNameTr().toStdString(),
              cbx.currentText().toStdString());
    EXPECT_EQ(mLayers[newIndex]->getName().toStdString(),
              dialog.getLayerName()->toStdString());
  }
}

TEST_F(DxfImportDialogTest, testCirclesAsDrills) {
  const bool defaultValue = false;
  const bool newValue = true;

  {
    DxfImportDialog dialog(mLayers, GraphicsLayerName(mLayers[0]->getName()),
                           true, LengthUnit::millimeters(), "test");

    // Check the default value.
    QCheckBox& cbx =
        TestHelpers::getChild<QCheckBox>(dialog, "cbxCirclesAsDrills");
    EXPECT_EQ(defaultValue, cbx.isChecked());
    EXPECT_EQ(defaultValue, dialog.getImportCirclesAsDrills());

    // Check if the value can be changed.
    cbx.setChecked(newValue);
    EXPECT_EQ(newValue, dialog.getImportCirclesAsDrills());
  }

  // Check if the setting is saved and restored automatically.
  {
    DxfImportDialog dialog(mLayers, GraphicsLayerName(mLayers[0]->getName()),
                           true, LengthUnit::millimeters(), "test");
    QCheckBox& cbx =
        TestHelpers::getChild<QCheckBox>(dialog, "cbxCirclesAsDrills");
    EXPECT_EQ(newValue, cbx.isChecked());
    EXPECT_EQ(newValue, dialog.getImportCirclesAsDrills());
  }
}

TEST_F(DxfImportDialogTest, testJoinTangentPolylines) {
  const bool defaultValue = true;
  const bool newValue = false;

  {
    DxfImportDialog dialog(mLayers, GraphicsLayerName(mLayers[0]->getName()),
                           true, LengthUnit::millimeters(), "test");

    // Check the default value.
    QCheckBox& cbx =
        TestHelpers::getChild<QCheckBox>(dialog, "cbxJoinTangentPolylines");
    EXPECT_EQ(defaultValue, cbx.isChecked());
    EXPECT_EQ(defaultValue, dialog.getJoinTangentPolylines());

    // Check if the value can be changed.
    cbx.setChecked(newValue);
    EXPECT_EQ(newValue, dialog.getImportCirclesAsDrills());
  }

  // Check if the setting is saved and restored automatically.
  {
    DxfImportDialog dialog(mLayers, GraphicsLayerName(mLayers[0]->getName()),
                           true, LengthUnit::millimeters(), "test");
    QCheckBox& cbx =
        TestHelpers::getChild<QCheckBox>(dialog, "cbxJoinTangentPolylines");
    EXPECT_EQ(newValue, cbx.isChecked());
    EXPECT_EQ(newValue, dialog.getJoinTangentPolylines());
  }
}

TEST_F(DxfImportDialogTest, testLineWidth) {
  const UnsignedLength defaultValue(0);
  const UnsignedLength newValue(1230000);

  {
    DxfImportDialog dialog(mLayers, GraphicsLayerName(mLayers[0]->getName()),
                           true, LengthUnit::millimeters(), "test");

    // Check the default value.
    UnsignedLengthEdit& edt =
        TestHelpers::getChild<UnsignedLengthEdit>(dialog, "edtLineWidth");
    EXPECT_EQ(defaultValue, edt.getValue());
    EXPECT_EQ(defaultValue, dialog.getLineWidth());

    // Check if the value can be changed.
    edt.setValue(newValue);
    EXPECT_EQ(newValue, dialog.getLineWidth());
  }

  // Check if the setting is saved and restored automatically.
  {
    DxfImportDialog dialog(mLayers, GraphicsLayerName(mLayers[0]->getName()),
                           true, LengthUnit::millimeters(), "test");
    UnsignedLengthEdit& edt =
        TestHelpers::getChild<UnsignedLengthEdit>(dialog, "edtLineWidth");
    EXPECT_EQ(newValue, edt.getValue());
    EXPECT_EQ(newValue, dialog.getLineWidth());
  }
}

TEST_F(DxfImportDialogTest, testScaleFactor) {
  const qreal defaultValue = 1;
  const qreal newValue = 0.5;

  {
    DxfImportDialog dialog(mLayers, GraphicsLayerName(mLayers[0]->getName()),
                           true, LengthUnit::millimeters(), "test");

    // Check the default value.
    DoubleSpinBox& spbx =
        TestHelpers::getChild<DoubleSpinBox>(dialog, "spbxScaleFactor");
    EXPECT_EQ(defaultValue, spbx.value());
    EXPECT_EQ(defaultValue, dialog.getScaleFactor());

    // Check if the value can be changed.
    spbx.setValue(newValue);
    EXPECT_EQ(newValue, dialog.getScaleFactor());
  }

  // Check if the setting is saved and restored automatically.
  {
    DxfImportDialog dialog(mLayers, GraphicsLayerName(mLayers[0]->getName()),
                           true, LengthUnit::millimeters(), "test");
    DoubleSpinBox& spbx =
        TestHelpers::getChild<DoubleSpinBox>(dialog, "spbxScaleFactor");
    EXPECT_EQ(newValue, spbx.value());
    EXPECT_EQ(newValue, dialog.getScaleFactor());
  }
}

TEST_F(DxfImportDialogTest, testPlacementPosition) {
  const tl::optional<Point> defaultValue = tl::nullopt;
  const tl::optional<Point> newValue = Point(1000000, 2000000);

  {
    DxfImportDialog dialog(mLayers, GraphicsLayerName(mLayers[0]->getName()),
                           true, LengthUnit::millimeters(), "test");

    // Check the default value.
    QCheckBox& cbxInteractive =
        TestHelpers::getChild<QCheckBox>(dialog, "cbxInteractivePlacement");
    LengthEdit& edtX = TestHelpers::getChild<LengthEdit>(dialog, "edtPosX");
    LengthEdit& edtY = TestHelpers::getChild<LengthEdit>(dialog, "edtPosY");
    EXPECT_EQ(!defaultValue.has_value(), cbxInteractive.isChecked());
    EXPECT_EQ(defaultValue.has_value(), edtX.isEnabled());
    EXPECT_EQ(defaultValue.has_value(), edtY.isEnabled());
    EXPECT_EQ(Length(0), edtX.getValue());
    EXPECT_EQ(Length(0), edtY.getValue());
    EXPECT_EQ(defaultValue, dialog.getPlacementPosition());

    // Check if the value can be changed.
    edtX.setValue(newValue->getX());
    edtY.setValue(newValue->getY());
    cbxInteractive.setChecked(!newValue.has_value());
    EXPECT_EQ(newValue, dialog.getPlacementPosition());
  }

  // Check if the setting is saved and restored automatically.
  {
    DxfImportDialog dialog(mLayers, GraphicsLayerName(mLayers[0]->getName()),
                           true, LengthUnit::millimeters(), "test");
    QCheckBox& cbxInteractive =
        TestHelpers::getChild<QCheckBox>(dialog, "cbxInteractivePlacement");
    LengthEdit& edtX = TestHelpers::getChild<LengthEdit>(dialog, "edtPosX");
    LengthEdit& edtY = TestHelpers::getChild<LengthEdit>(dialog, "edtPosY");
    EXPECT_EQ(!newValue.has_value(), cbxInteractive.isChecked());
    EXPECT_EQ(newValue.has_value(), edtX.isEnabled());
    EXPECT_EQ(newValue.has_value(), edtY.isEnabled());
    EXPECT_EQ(newValue->getX(), edtX.getValue());
    EXPECT_EQ(newValue->getY(), edtY.getValue());
    EXPECT_EQ(newValue, dialog.getPlacementPosition());
  }
}

TEST_F(DxfImportDialogTest, testHolesSupport) {
  for (bool enable : {true, false}) {
    DxfImportDialog dialog(mLayers, GraphicsLayerName(mLayers[0]->getName()),
                           enable, LengthUnit::millimeters(), "test");
    QCheckBox& cbx =
        TestHelpers::getChild<QCheckBox>(dialog, "cbxCirclesAsDrills");
    EXPECT_EQ(enable, cbx.isVisibleTo(&dialog));
  }
}

TEST_F(DxfImportDialogTest, testTabOrder) {
  DxfImportDialog dialog(mLayers, GraphicsLayerName(mLayers[0]->getName()),
                         true, LengthUnit::millimeters(), "test");
  TestHelpers::testTabOrder(dialog);
}

TEST_F(DxfImportDialogTest, testThrowNoObjectsImportedError) {
  EXPECT_THROW(DxfImportDialog::throwNoObjectsImportedError(), Exception);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace editor
}  // namespace librepcb
