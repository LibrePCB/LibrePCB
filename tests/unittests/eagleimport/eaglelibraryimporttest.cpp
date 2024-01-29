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
#include <librepcb/core/utils/messagelogger.h>
#include <librepcb/eagleimport/eaglelibraryimport.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace eagleimport {
namespace tests {

/*******************************************************************************
 *  Test Class
 ******************************************************************************/

class EagleLibraryImportTest : public ::testing::Test {};

/*******************************************************************************
 *  Test Methods
 ******************************************************************************/

TEST_F(EagleLibraryImportTest, testImport) {
  FilePath src(TEST_DATA_DIR "/unittests/eagleimport/resistor.lbr");
  FilePath dst = FilePath::getRandomTempPath();

  EagleLibraryImport import(dst);

  // Connect signals by hand because QSignalSpy is not threadsafe!
  int signalFinished = 0;
  QObject::connect(&import, &EagleLibraryImport::finished,
                   [&signalFinished]() { ++signalFinished; });

  QStringList parseErrors = import.open(src);
  EXPECT_EQ(1, import.getSymbols().count());
  EXPECT_EQ(1, import.getPackages().count());
  EXPECT_EQ(1, import.getComponents().count());
  EXPECT_EQ(1, import.getDevices().count());
  EXPECT_EQ(0, parseErrors.count());

  import.start();
  EXPECT_TRUE(import.wait(10000));
  EXPECT_EQ(1, signalFinished);
  EXPECT_EQ(0, import.getLogger()->getMessages().count());
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace eagleimport
}  // namespace librepcb
