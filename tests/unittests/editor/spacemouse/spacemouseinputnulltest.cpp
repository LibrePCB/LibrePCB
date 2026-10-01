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
#include <librepcb/editor/spacemouse/spacemouseinputnull.h>

#include <QtCore>
#include <QtTest>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {
namespace tests {

/*******************************************************************************
 *  Test Methods
 *
 *  Note: SpaceMouseInputNull is the backend used when SpaceMouse support is
 *  not available in a build, so it is the only backend which can be tested
 *  without real hardware.
 ******************************************************************************/

TEST(SpaceMouseInputNullTest, testNeverConnected) {
  SpaceMouseInputNull backend;
  EXPECT_FALSE(backend.isDeviceConnected());
}

TEST(SpaceMouseInputNullTest, testSetLedEnabledIsNoOp) {
  SpaceMouseInputNull backend;
  QSignalSpy motionSpy(&backend, &IF_SpaceMouseInputBackend::motionEvent);
  QSignalSpy connectedSpy(&backend,
                          &IF_SpaceMouseInputBackend::deviceConnectedChanged);

  backend.setLedEnabled(true);
  backend.setLedEnabled(false);

  EXPECT_FALSE(backend.isDeviceConnected());
  EXPECT_EQ(0, motionSpy.count());
  EXPECT_EQ(0, connectedSpy.count());
}

TEST(SpaceMouseInputNullTest, testNeverEmitsSignals) {
  SpaceMouseInputNull backend;
  QSignalSpy motionSpy(&backend, &IF_SpaceMouseInputBackend::motionEvent);
  QSignalSpy connectedSpy(&backend,
                          &IF_SpaceMouseInputBackend::deviceConnectedChanged);

  // Let any (unexpectedly) queued events get delivered.
  QCoreApplication::processEvents();

  EXPECT_EQ(0, motionSpy.count());
  EXPECT_EQ(0, connectedSpy.count());
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace editor
}  // namespace librepcb
