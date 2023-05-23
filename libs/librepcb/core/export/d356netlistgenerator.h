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
 *
 * @see https://www.downstreamtech.com/downloads/IPCD356_Simplified.pdf
 * @see
 * https://web.pa.msu.edu/hep/atlas/l1calo/hub/hardware/components/circuit_board/ipc_356a_net_list.pdf
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
  void throughVia(const QString& netName, const Point& position,
                  const PositiveLength& width, const PositiveLength& height,
                  const Angle& rotation, const PositiveLength& drillDiameter,
                  bool solderMaskCovered);
  void blindVia(const QString& netName, const Point& position,
                const PositiveLength& width, const PositiveLength& height,
                const Angle& rotation, const PositiveLength& drillDiameter,
                int startLayer, int endLayer, bool solderMaskCovered);
  void buriedVia(const QString& netName, const Point& position,
                 const PositiveLength& drillDiameter, int startLayer,
                 int endLayer);
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
    Continuation = 27,
    BlindOrBuriedVia = 307,
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
    tl::optional<QString> signalName;
    QString componentName;
    QString padName;
    bool midPoint;
    tl::optional<std::pair<PositiveLength, bool>> hole;
    tl::optional<int> accessCode;
    Point position;
    tl::optional<PositiveLength> width;
    tl::optional<PositiveLength> height;
    tl::optional<Angle> rotation;
    tl::optional<SolderMask> solderMask;
    tl::optional<int> startLayer;
    tl::optional<int> endLayer;
  };

  QStringList mComments;
  QList<Record> mRecords;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
