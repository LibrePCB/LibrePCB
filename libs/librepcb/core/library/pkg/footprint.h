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

#ifndef LIBREPCB_CORE_FOOTPRINT_H
#define LIBREPCB_CORE_FOOTPRINT_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../geometry/circle.h"
#include "../../geometry/hole.h"
#include "../../geometry/polygon.h"
#include "../../geometry/stroketext.h"
#include "../../geometry/zone.h"
#include "../../serialization/serializablekeyvaluemap.h"
#include "../../serialization/serializableobjectlist.h"
#include "footprintpad.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class Footprint
 ******************************************************************************/

/**
 * @brief The Footprint class represents one footprint variant of a package
 *
 * Following information is considered as the "interface" of a footprint and
 * must therefore never be changed:
 *  - UUID
 *  - Footprint pads (neither adding nor removing pads is allowed)
 *    - UUID
 */
class Footprint final {
  Q_DECLARE_TR_FUNCTIONS(Footprint)

public:
  // Signals
  enum class Event {
    UuidChanged,
    NamesEdited,
    DescriptionsEdited,
    ModelPositionChanged,
    ModelRotationChanged,
    ModelsChanged,
    PadsEdited,
    PolygonsEdited,
    CirclesEdited,
    StrokeTextsEdited,
    HolesEdited,
    ZonesEdited,
  };
  Signal<Footprint, Event> onEdited;
  typedef Slot<Footprint, Event> OnEditedSlot;

  // Constructors / Destructor
  Footprint() = delete;
  Footprint(const Footprint& other) noexcept;
  Footprint(const Uuid& uuid, const ElementName& name_en_US,
            const QString& description_en_US);
  explicit Footprint(const SExpression& node);
  ~Footprint() noexcept;

  // Getters: General
  const Uuid& getUuid() const noexcept { return mUuid; }
  LocalizedNameMap& getNames() noexcept { return mNames; }
  const LocalizedNameMap& getNames() const noexcept { return mNames; }
  LocalizedDescriptionMap& getDescriptions() noexcept { return mDescriptions; }
  const LocalizedDescriptionMap& getDescriptions() const noexcept {
    return mDescriptions;
  }
  const Point3D& getModelPosition() const noexcept { return mModelPosition; }
  const Angle3D& getModelRotation() const noexcept { return mModelRotation; }
  const QSet<Uuid>& getModels() const noexcept { return mModels; }

  // Getters: Geometry
  const FootprintPadList& getPads() const noexcept { return mPads; }
  FootprintPadList& getPads() noexcept { return mPads; }
  const PolygonList& getPolygons() const noexcept { return mPolygons; }
  PolygonList& getPolygons() noexcept { return mPolygons; }
  const CircleList& getCircles() const noexcept { return mCircles; }
  CircleList& getCircles() noexcept { return mCircles; }
  const StrokeTextList& getStrokeTexts() const noexcept { return mStrokeTexts; }
  StrokeTextList& getStrokeTexts() noexcept { return mStrokeTexts; }
  const ZoneList& getZones() const noexcept { return mZones; }
  ZoneList& getZones() noexcept { return mZones; }
  const HoleList& getHoles() const noexcept { return mHoles; }
  HoleList& getHoles() noexcept { return mHoles; }

  // Setters: General
  bool setModelPosition(const Point3D& position) noexcept;
  bool setModelRotation(const Angle3D& rotation) noexcept;
  bool setModels(const QSet<Uuid>& models) noexcept;

  // General Methods

  /**
   * @brief Serialize into ::librepcb::SExpression node
   *
   * @param root    Root node to serialize into.
   */
  void serialize(SExpression& root) const;

  // Operator Overloadings
  bool operator==(const Footprint& rhs) const noexcept;
  bool operator!=(const Footprint& rhs) const noexcept {
    return !(*this == rhs);
  }
  Footprint& operator=(const Footprint& rhs) noexcept;

private:  // Methods
  void namesEdited(const LocalizedNameMap& names, const QString& key,
                   LocalizedNameMap::Event event) noexcept;
  void descriptionsEdited(const LocalizedDescriptionMap& names,
                          const QString& key,
                          LocalizedDescriptionMap::Event event) noexcept;
  void padsEdited(const FootprintPadList& list, int index,
                  const std::shared_ptr<const FootprintPad>& pad,
                  FootprintPadList::Event event) noexcept;
  void polygonsEdited(const PolygonList& list, int index,
                      const std::shared_ptr<const Polygon>& polygon,
                      PolygonList::Event event) noexcept;
  void circlesEdited(const CircleList& list, int index,
                     const std::shared_ptr<const Circle>& circle,
                     CircleList::Event event) noexcept;
  void strokeTextsEdited(const StrokeTextList& list, int index,
                         const std::shared_ptr<const StrokeText>& text,
                         StrokeTextList::Event event) noexcept;
  void zonesEdited(const ZoneList& list, int index,
                   const std::shared_ptr<const Zone>& zone,
                   ZoneList::Event event) noexcept;
  void holesEdited(const HoleList& list, int index,
                   const std::shared_ptr<const Hole>& hole,
                   HoleList::Event event) noexcept;

private:  // Data
  Uuid mUuid;
  LocalizedNameMap mNames;
  LocalizedDescriptionMap mDescriptions;
  Point3D mModelPosition;
  Angle3D mModelRotation;
  QSet<Uuid> mModels;
  FootprintPadList mPads;
  PolygonList mPolygons;
  CircleList mCircles;
  StrokeTextList mStrokeTexts;
  ZoneList mZones;
  HoleList mHoles;

  // Slots
  LocalizedNameMap::OnEditedSlot mNamesEditedSlot;
  LocalizedDescriptionMap::OnEditedSlot mDescriptionsEditedSlot;
  FootprintPadList::OnEditedSlot mPadsEditedSlot;
  PolygonList::OnEditedSlot mPolygonsEditedSlot;
  CircleList::OnEditedSlot mCirclesEditedSlot;
  StrokeTextList::OnEditedSlot mStrokeTextsEditedSlot;
  ZoneList::OnEditedSlot mZonesEditedSlot;
  HoleList::OnEditedSlot mHolesEditedSlot;
};

/*******************************************************************************
 *  Class FootprintList
 ******************************************************************************/

struct FootprintListNameProvider {
  static constexpr const char* tagname = "footprint";
};
using FootprintList =
    SerializableObjectList<Footprint, FootprintListNameProvider,
                           Footprint::Event>;

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
