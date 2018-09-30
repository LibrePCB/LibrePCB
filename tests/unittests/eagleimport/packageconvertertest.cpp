/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
 * http://librepcb.org/
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
#include <librepcb/eagleimport/converterdb.h>
#include <librepcb/eagleimport/packageconverter.h>
#include <librepcb/library/pkg/package.h>
#include <parseagle/library.h>

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

class PackageConverterTest : public ::testing::Test {};

/*******************************************************************************
 *  Test Methods
 ******************************************************************************/

TEST_F(PackageConverterTest, testConversion) {
  FilePath testDataDir(TEST_DATA_DIR "/unittests/eagleimport");

  // load eagle package
  FilePath           eagleLibFp = testDataDir.getPathTo("resistor.lbr");
  parseagle::Library eagleLibrary(eagleLibFp.toStr());
  ASSERT_EQ(1, eagleLibrary.getPackages().count());
  const parseagle::Package& eaglePackage = eagleLibrary.getPackages().first();

  // load converter database
  ConverterDb db(testDataDir.getPathTo("db.ini"));

  // convert package
  PackageConverter                  converter(eaglePackage, db);
  std::unique_ptr<library::Package> package = converter.generate();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace eagleimport
}  // namespace librepcb
