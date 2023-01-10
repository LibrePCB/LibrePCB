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

#ifndef LIBREPCB_CORE_D356NETLISTGENERATOR_H
#define LIBREPCB_CORE_D356NETLISTGENERATOR_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../types/angle.h"
#include "../types/point.h"

#include <optional/tl/optional.hpp>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class D356NetlistGenerator
 ******************************************************************************/

/**
 * @brief The D356NetlistGenerator class
 */
class D356NetlistGenerator final {
  Q_DECLARE_TR_FUNCTIONS(D356NetlistGenerator)

public:
  // Constructors / Destructor
  D356NetlistGenerator() = delete;
  D356NetlistGenerator(const D356NetlistGenerator& other) = delete;
  D356NetlistGenerator(const QString& projName, const QString& projRevision,
                       const QString& brdName,
                       const QDateTime& generationDate) noexcept;
  ~D356NetlistGenerator() noexcept;

  // General Methods
  void smtPad(const QString& netName, const QString& cmpName,
              const QString& padName, const Point& position,
              const PositiveLength& width, const PositiveLength& height,
              const Angle& rotation, int layer);
  void thtPad(const QString& netName, const QString& cmpName,
              const QString& padName, const Point& position,
              const PositiveLength& width, const PositiveLength& height,
              const Angle& rotation, const PositiveLength& drillDiameter);
  void via(const QString& netName, const Point& position,
           const PositiveLength& width, const PositiveLength& height,
           const Angle& rotation, const PositiveLength& drillDiameter,
           bool solderMaskCovered);
  QByteArray generate() const;

  // Operator Overloadings
  D356NetlistGenerator& operator=(const D356NetlistGenerator& rhs) = delete;

private:  // Methods
  static QString cleanString(QString str) noexcept;
  static QString checkedComponentName(const QString& name) noexcept;
  static QString formatLength(const Length& value, bool isSigned,
                              int digits) noexcept;

private:  // Data
  enum class OperationCode : int {
    ThroughHole = 317,
    SurfaceMount = 327,
  };

  enum class SolderMask : int {
    None = 0,
    PrimarySide = 1,
    SecondarySide = 2,
    BothSides = 3,
  };

  struct Record {
    OperationCode code;
    QString signalName;
    QString componentName;
    QString padName;
    bool midPoint;
    tl::optional<std::pair<PositiveLength, bool>> hole;
    int accessCode;
    Point position;
    PositiveLength width;
    PositiveLength height;
    Angle rotation;
    SolderMask solderMask;
  };

  QStringList mComments;
  QList<Record> mRecords;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
