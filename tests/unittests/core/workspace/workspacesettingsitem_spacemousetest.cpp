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
#include <librepcb/core/serialization/sexpression.h>
#include <librepcb/core/workspace/workspacesettingsitem_spacemouse.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace tests {

using Axis = WorkspaceSettingsItem_SpaceMouse::Axis;
using AxisSettings = WorkspaceSettingsItem_SpaceMouse::AxisSettings;

/*******************************************************************************
 *  Test Methods
 ******************************************************************************/

TEST(WorkspaceSettingsItemSpaceMouseTest, testDefaults) {
  WorkspaceSettingsItem_SpaceMouse obj(nullptr);
  EXPECT_EQ(6, obj.get().count());
  EXPECT_EQ(AxisSettings(), obj.get(Axis::TranslationX));
  EXPECT_EQ(AxisSettings(), obj.get(Axis::RotationZ));
}

TEST(WorkspaceSettingsItemSpaceMouseTest, testStoreAndLoad) {
  WorkspaceSettingsItem_SpaceMouse obj1(nullptr);
  obj1.set(Axis::TranslationY, {1.5, true});
  obj1.set(Axis::RotationX, {0.25, false});

  std::unique_ptr<SExpression> root = SExpression::createList("spacemouse");
  obj1.serialize(*root);

  WorkspaceSettingsItem_SpaceMouse obj2(nullptr);
  obj2.load(*root);
  EXPECT_EQ(obj1.get(), obj2.get());
  EXPECT_EQ((AxisSettings{1.5, true}), obj2.get(Axis::TranslationY));
}

TEST(WorkspaceSettingsItemSpaceMouseTest, testLoadIgnoresInvalidValues) {
  std::unique_ptr<SExpression> root = SExpression::parse(
      "(spacemouse\n"
      " (axis translation_x (sensitivity 0.0) (invert true))\n"
      " (axis translation_y (sensitivity -2.0) (invert false))\n"
      " (axis rotation_z (sensitivity abc) (invert false))\n"
      " (axis unknown_axis (sensitivity 2.0) (invert true))\n"
      " (axis rotation_x (sensitivity 2.0) (invert true))\n"
      ")",
      FilePath());

  WorkspaceSettingsItem_SpaceMouse obj(nullptr);
  obj.load(*root);
  // Invalid sensitivities fall back to the default, but other values of the
  // same axis are still loaded.
  EXPECT_EQ((AxisSettings{1.0, true}), obj.get(Axis::TranslationX));
  EXPECT_EQ((AxisSettings{1.0, false}), obj.get(Axis::TranslationY));
  EXPECT_EQ((AxisSettings{1.0, false}), obj.get(Axis::RotationZ));
  EXPECT_EQ((AxisSettings{2.0, true}), obj.get(Axis::RotationX));
  EXPECT_EQ(6, obj.get().count());  // Unknown axis was ignored.
}

TEST(WorkspaceSettingsItemSpaceMouseTest, testRestoreDefault) {
  WorkspaceSettingsItem_SpaceMouse obj(nullptr);
  obj.set(Axis::RotationY, {3.0, true});
  obj.restoreDefault();
  EXPECT_EQ(AxisSettings(), obj.get(Axis::RotationY));
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace librepcb
