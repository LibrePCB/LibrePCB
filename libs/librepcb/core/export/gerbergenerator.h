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

#ifndef LIBREPCB_CORE_GERBERGENERATOR_H
#define LIBREPCB_CORE_GERBERGENERATOR_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../fileio/filepath.h"
#include "../types/length.h"
#include "../types/uuid.h"
#include "gerberaperturelist.h"
#include "gerberattribute.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Angle;
class GerberAttributeWriter;
class Path;
class Point;

/*******************************************************************************
 *  Class GerberGenerator
 ******************************************************************************/

/**
 * @brief The GerberGenerator class
 */
class GerberGenerator final {
  Q_DECLARE_TR_FUNCTIONS(GerberGenerator)

public:
  // Public Types
  using Polarity = GerberAttribute::Polarity;
  using BoardSide = GerberAttribute::BoardSide;
  using CopperSide = GerberAttribute::CopperSide;
  using Function = GerberApertureList::Function;
  using MountType = GerberAttribute::MountType;

  // Constructors / Destructor
  GerberGenerator() = delete;
  GerberGenerator(const GerberGenerator& other) = delete;
  GerberGenerator(const QDateTime& creationDate, const QString& projName,
                  const Uuid& projUuid, const QString& projRevision) noexcept;
  ~GerberGenerator() noexcept;

  // Getters
  const QString& toStr() const noexcept { return mOutput; }

  // Plot Methods
  void setFileFunctionOutlines(bool plated) noexcept;
  void setFileFunctionCopper(int layer, CopperSide side,
                             Polarity polarity) noexcept;
  void setFileFunctionSolderMask(BoardSide side, Polarity polarity) noexcept;
  void setFileFunctionLegend(BoardSide side, Polarity polarity) noexcept;
  void setFileFunctionPaste(BoardSide side, Polarity polarity) noexcept;
  void setFileFunctionComponent(int layer, BoardSide side) noexcept;
  void setLayerPolarity(Polarity p) noexcept;
  void drawLine(const Point& start, const Point& end,
                const UnsignedLength& width, Function function,
                const tl::optional<QString>& net,
                const QString& component) noexcept;
  void drawPathOutline(const Path& path, const UnsignedLength& lineWidth,
                       Function function, const tl::optional<QString>& net,
                       const QString& component) noexcept;
  void drawPathArea(const Path& path, Function function,
                    const tl::optional<QString>& net,
                    const QString& component) noexcept;
  void drawComponentOutline(const Path& path, const Angle& rot,
                            const QString& designator, const QString& value,
                            MountType mountType, const QString& manufacturer,
                            const QString& mpn, const QString& footprintName,
                            Function function) noexcept;
  void flashCircle(const Point& pos, const PositiveLength& dia,
                   Function function, const tl::optional<QString>& net,
                   const QString& component, const QString& pin,
                   const QString& signal) noexcept;
  void flashRect(const Point& pos, const PositiveLength& w,
                 const PositiveLength& h, const UnsignedLength& radius,
                 const Angle& rot, Function function,
                 const tl::optional<QString>& net, const QString& component,
                 const QString& pin, const QString& signal) noexcept;
  void flashObround(const Point& pos, const PositiveLength& w,
                    const PositiveLength& h, const Angle& rot,
                    Function function, const tl::optional<QString>& net,
                    const QString& component, const QString& pin,
                    const QString& signal) noexcept;
  void flashOctagon(const Point& pos, const PositiveLength& w,
                    const PositiveLength& h, const UnsignedLength& radius,
                    const Angle& rot, Function function,
                    const tl::optional<QString>& net, const QString& component,
                    const QString& pin, const QString& signal) noexcept;
  void flashComponent(const Point& pos, const Angle& rot,
                      const QString& designator, const QString& value,
                      MountType mountType, const QString& manufacturer,
                      const QString& mpn,
                      const QString& footprintName) noexcept;
  void flashComponentPin(const Point& pos, const Angle& rot,
                         const QString& designator, const QString& value,
                         MountType mountType, const QString& manufacturer,
                         const QString& mpn, const QString& footprintName,
                         const QString& pin, const QString& signal,
                         bool isPin1) noexcept;

  // General Methods
  void generate();
  void saveToFile(const FilePath& filepath) const;

  // Operator Overloadings
  GerberGenerator& operator=(const GerberGenerator& rhs) = delete;

private:
  // Private Methods
  void setCurrentAttributes(
      Function apertureFunction, const tl::optional<QString>& netName,
      const QString& componentDesignator, const QString& pinName,
      const QString& pinSignal, const QString& componentValue,
      const tl::optional<MountType>& componentMountType,
      const QString& componentManufacturer, const QString& componentMpn,
      const QString& componentFootprint,
      const tl::optional<Angle>& componentRotation) noexcept;
  void setCurrentAperture(int number) noexcept;
  void setRegionModeOn() noexcept;
  void setRegionModeOff() noexcept;
  void switchToLinearInterpolationModeG01() noexcept;
  void switchToCircularCwInterpolationModeG02() noexcept;
  void switchToCircularCcwInterpolationModeG03() noexcept;
  void moveToPosition(const Point& pos) noexcept;
  void linearInterpolateToPosition(const Point& pos) noexcept;
  void circularInterpolateToPosition(const Point& start, const Point& center,
                                     const Point& end) noexcept;
  void interpolateBetween(const Vertex& from, const Vertex& to) noexcept;
  void flashAtPosition(const Point& pos) noexcept;
  void printHeader() noexcept;
  void printApertureList() noexcept;
  void printContent() noexcept;
  void printFooter() noexcept;
  QString calcOutputMd5Checksum() const noexcept;

  // Metadata
  QVector<GerberAttribute> mFileAttributes;

  // Gerber Data
  QString mOutput;
  QString mContent;
  QScopedPointer<GerberAttributeWriter> mAttributeWriter;
  QScopedPointer<GerberApertureList> mApertureList;
  int mCurrentApertureNumber;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
