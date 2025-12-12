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

#ifndef LIBREPCB_CORE_SI_BUSSEGMENT_H
#define LIBREPCB_CORE_SI_BUSSEGMENT_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../../types/point.h"
#include "../../../types/uuid.h"
#include "si_base.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Bus;
class SI_BusJunction;
class SI_BusLabel;
class SI_BusLine;
class SI_NetSegment;

/*******************************************************************************
 *  Class SI_BusSegment
 ******************************************************************************/

/**
 * @brief The SI_BusSegment class
 *
 * @todo Do not allow to create empty bus segments!
 */
class SI_BusSegment final : public SI_Base {
  Q_OBJECT

public:
  // Constructors / Destructor
  SI_BusSegment() = delete;
  SI_BusSegment(const SI_BusSegment& other) = delete;
  SI_BusSegment(Schematic& schematic, const Uuid& uuid, Bus& bus);
  ~SI_BusSegment() noexcept;

  // Getters
  const Uuid& getUuid() const noexcept { return mUuid; }
  Bus& getBus() const noexcept { return *mBus; }
  bool isUsed() const noexcept;
  Point calcNearestPoint(const Point& p) const noexcept;
  QSet<SI_NetSegment*> getAttachedNetSegments() const noexcept;

  // Setters
  void setBus(Bus& bus);

  // Element Getters
  const QMap<Uuid, SI_BusJunction*>& getJunctions() const noexcept {
    return mJunctions;
  }
  const QMap<Uuid, SI_BusLine*>& getLines() const noexcept { return mLines; }

  // Junction/Line Methods
  void addJunctionsAndLines(const QList<SI_BusJunction*>& junctions,
                            const QList<SI_BusLine*>& lines);
  void removeJunctionsAndLines(const QList<SI_BusJunction*>& junctions,
                               const QList<SI_BusLine*>& lines);

  // Label Methods
  const QMap<Uuid, SI_BusLabel*>& getLabels() const noexcept { return mLabels; }
  void addLabel(SI_BusLabel& label);
  void removeLabel(SI_BusLabel& label);
  void updateAllLabelAnchors() noexcept;

  // General Methods
  void addToSchematic() override;
  void removeFromSchematic() override;

  /**
   * @brief Serialize into ::librepcb::SExpression node
   *
   * @param root    Root node to serialize into.
   */
  void serialize(SExpression& root) const;

  // Operator Overloadings
  SI_BusSegment& operator=(const SI_BusSegment& rhs) = delete;
  bool operator==(const SI_BusSegment& rhs) noexcept { return (this == &rhs); }
  bool operator!=(const SI_BusSegment& rhs) noexcept { return (this != &rhs); }

signals:
  void junctionsAndLinesAdded(const QList<SI_BusJunction*>& junctions,
                              const QList<SI_BusLine*>& lines);
  void junctionsAndLinesRemoved(const QList<SI_BusJunction*>& junctions,
                                const QList<SI_BusLine*>& lines);
  void labelAdded(SI_BusLabel& label);
  void labelRemoved(SI_BusLabel& label);

private:
  bool checkAttributesValidity() const noexcept;
  bool areAllJunctionsConnectedTogether() const noexcept;
  void findAllConnectedJunctions(const SI_BusJunction& np,
                                 QSet<const SI_BusJunction*>& points,
                                 QSet<const SI_BusLine*>& lines) const noexcept;

  // Attributes
  Uuid mUuid;
  Bus* mBus;

  // Items
  QMap<Uuid, SI_BusJunction*> mJunctions;
  QMap<Uuid, SI_BusLine*> mLines;
  QMap<Uuid, SI_BusLabel*> mLabels;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
