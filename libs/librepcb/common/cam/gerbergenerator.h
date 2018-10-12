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

#ifndef LIBREPCB_GERBERGENERATOR_H
#define LIBREPCB_GERBERGENERATOR_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../exceptions.h"
#include "../fileio/filepath.h"
#include "../units/all_length_units.h"
#include "../uuid.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Circle;
class Path;
class GerberApertureList;

/*******************************************************************************
 *  Class GerberGenerator
 ******************************************************************************/

/**
 * @brief The GerberGenerator class
 *
 * @todo Remove/Escape illegal characters in #mProjectId and #mProjectRevision!
 * @todo Use file/aperture attributes
 *
 * @author ubruhin
 * @date 2016-01-10
 */
class GerberGenerator final {
  Q_DECLARE_TR_FUNCTIONS(GerberGenerator)

public:
  // Public Types
  // enum class FileFunction {Copper, SolderMask, Glue, Paste, KeepOut, Plated,
  //                         NonPlated, Profile, DrillMap};
  enum class LayerPolarity { Positive, Negative };

  // Constructors / Destructor
  GerberGenerator()                             = delete;
  GerberGenerator(const GerberGenerator& other) = delete;
  GerberGenerator(const QString& projName, const Uuid& projUuid,
                  const QString& projRevision) noexcept;
  ~GerberGenerator() noexcept;

  // Getters
  const QString& toStr() const noexcept { return mOutput; }

  // Plot Methods
  void setLayerPolarity(LayerPolarity p) noexcept;
  void drawLine(const Point& start, const Point& end,
                const UnsignedLength& width) noexcept;
  void drawCircleOutline(const Circle& circle) noexcept;
  void drawCircleArea(const Circle& circle) noexcept;
  void drawPathOutline(const Path&           path,
                       const UnsignedLength& lineWidth) noexcept;
  void drawPathArea(const Path& path) noexcept;
  void flashCircle(const Point& pos, const UnsignedLength& dia,
                   const UnsignedLength& hole) noexcept;
  void flashRect(const Point& pos, const UnsignedLength& w,
                 const UnsignedLength& h, const Angle& rot,
                 const UnsignedLength& hole) noexcept;
  void flashObround(const Point& pos, const UnsignedLength& w,
                    const UnsignedLength& h, const Angle& rot,
                    const UnsignedLength& hole) noexcept;
  void flashRegularPolygon(const Point& pos, const UnsignedLength& dia, int n,
                           const Angle&          rot,
                           const UnsignedLength& hole) noexcept;

  // General Methods
  void reset() noexcept;
  void generate();
  void saveToFile(const FilePath& filepath) const;

  // Operator Overloadings
  GerberGenerator& operator=(const GerberGenerator& rhs) = delete;

private:
  // Private Methods
  void    setCurrentAperture(int number) noexcept;
  void    setRegionModeOn() noexcept;
  void    setRegionModeOff() noexcept;
  void    setMultiQuadrantArcModeOn() noexcept;
  void    setMultiQuadrantArcModeOff() noexcept;
  void    switchToLinearInterpolationModeG01() noexcept;
  void    switchToCircularCwInterpolationModeG02() noexcept;
  void    switchToCircularCcwInterpolationModeG03() noexcept;
  void    moveToPosition(const Point& pos) noexcept;
  void    linearInterpolateToPosition(const Point& pos) noexcept;
  void    circularInterpolateToPosition(const Point& start, const Point& center,
                                        const Point& end) noexcept;
  void    flashAtPosition(const Point& pos) noexcept;
  void    printHeader() noexcept;
  void    printApertureList() noexcept;
  void    printContent() noexcept;
  void    printFooter() noexcept;
  QString calcOutputMd5Checksum() const noexcept;

  // Static Methods
  static QString escapeString(const QString& str) noexcept;

  // Metadata
  QString mProjectId;
  Uuid    mProjectUuid;
  QString mProjectRevision;

  // Gerber Data
  QString                            mOutput;
  QString                            mContent;
  QScopedPointer<GerberApertureList> mApertureList;
  int                                mCurrentApertureNumber;
  bool                               mMultiQuadrantArcModeOn;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif  // LIBREPCB_GERBERGENERATOR_H
