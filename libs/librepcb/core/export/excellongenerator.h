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

#ifndef LIBREPCB_CORE_EXCELLONGENERATOR_H
#define LIBREPCB_CORE_EXCELLONGENERATOR_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../fileio/filepath.h"
#include "../geometry/path.h"
#include "../types/length.h"
#include "gerberattribute.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Point;

/*******************************************************************************
 *  Class ExcellonGenerator
 ******************************************************************************/

/**
 * @brief The ExcellonGenerator class
 */
class ExcellonGenerator final {
  Q_DECLARE_TR_FUNCTIONS(ExcellonGenerator)

public:
  // Types
  enum class Plating { Yes, No, Mixed };
  using Function = GerberAttribute::ApertureFunction;

  // Constructors / Destructor
  ExcellonGenerator() = delete;
  ExcellonGenerator(const ExcellonGenerator& other) = delete;
  ExcellonGenerator(const QDateTime& creationDate, const QString& projName,
                    const Uuid& projUuid, const QString& projRevision,
                    Plating plating, int fromLayer, int toLayer) noexcept;
  ~ExcellonGenerator() noexcept;

  // Setters
  void setUseG85Slots(bool use) noexcept { mUseG85Slots = use; }

  // Getters
  const QString& toStr() const noexcept { return mOutput; }

  // General Methods
  void drill(const Point& pos, const PositiveLength& dia, bool plated,
             Function function) noexcept;
  void drill(const NonEmptyPath& path, const PositiveLength& dia, bool plated,
             Function function) noexcept;
  void generate();
  void saveToFile(const FilePath& filepath) const;

  // Operator Overloadings
  ExcellonGenerator& operator=(const ExcellonGenerator& rhs) = delete;

private:
  void printHeader() noexcept;
  void printToolList() noexcept;
  void printDrills();
  void printPath(const NonEmptyPath& path);
  void printDrill(const Point& pos) noexcept;
  void printSlot(const NonEmptyPath& path);
  void printRout(const NonEmptyPath& path) noexcept;
  void printMoveTo(const Point& pos) noexcept;
  void printLinearInterpolation(const Point& pos) noexcept;
  void printCircularInterpolation(const Point& from, const Point& to,
                                  const Angle& angle) noexcept;
  void printFooter() noexcept;

  // Types
  typedef std::tuple<Length, bool, Function> Tool;

  // Metadata
  Plating mPlating;
  QVector<GerberAttribute> mFileAttributes;

  // Configuration
  bool mUseG85Slots;

  // Excellon Data
  QString mOutput;
  QMultiMap<Tool, NonEmptyPath> mDrillList;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
