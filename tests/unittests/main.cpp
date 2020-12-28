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

#include <gmock/gmock.h>
#include <librepcb/common/application.h>
#include <librepcb/common/debug.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
using namespace librepcb;

/*******************************************************************************
 *  The Unit Testing Program
 ******************************************************************************/

int main(int argc, char* argv[]) {
  // Initialize a common locale for all tests
  QLocale::setDefault(QLocale(QLocale::English, QLocale::UnitedStates));

  // many classes rely on a QApplication instance, so we create it here
  Application app(argc, argv);
  Application::setOrganizationName("LibrePCB");
  Application::setOrganizationDomain("librepcb.org");
  Application::setApplicationName("LibrePCB-UnitTests");

  // disable the whole debug output (we want only the output from gtest)
  Debug::instance()->setDebugLevelLogFile(Debug::DebugLevel_t::Nothing);
  Debug::instance()->setDebugLevelStderr(Debug::DebugLevel_t::Nothing);

  // init gmock and run all tests
  ::testing::InitGoogleMock(&argc, argv);
  return RUN_ALL_TESTS();
}
