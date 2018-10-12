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

#ifndef LIBREPCB_LIBRARY_FOOTPRINT_H
#define LIBREPCB_LIBRARY_FOOTPRINT_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "footprintpad.h"

#include <librepcb/common/fileio/cmd/cmdlistelementinsert.h>
#include <librepcb/common/fileio/cmd/cmdlistelementremove.h>
#include <librepcb/common/fileio/cmd/cmdlistelementsswap.h>
#include <librepcb/common/fileio/serializablekeyvaluemap.h>
#include <librepcb/common/fileio/serializableobjectlist.h>
#include <librepcb/common/geometry/circle.h>
#include <librepcb/common/geometry/hole.h>
#include <librepcb/common/geometry/polygon.h>
#include <librepcb/common/geometry/stroketext.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace library {

class FootprintGraphicsItem;

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
class Footprint final : public SerializableObject,
                        private FootprintPadList::IF_Observer,
                        private PolygonList::IF_Observer,
                        private CircleList::IF_Observer,
                        private StrokeTextList::IF_Observer,
                        private HoleList::IF_Observer {
  Q_DECLARE_TR_FUNCTIONS(Footprint)

public:
  // Constructors / Destructor
  Footprint() = delete;
  Footprint(const Footprint& other) noexcept;
  Footprint(const Uuid& uuid, const ElementName& name_en_US,
            const QString& description_en_US);
  explicit Footprint(const SExpression& node);
  ~Footprint() noexcept;

  // Getters: General
  const Uuid&              getUuid() const noexcept { return mUuid; }
  LocalizedNameMap&        getNames() noexcept { return mNames; }
  const LocalizedNameMap&  getNames() const noexcept { return mNames; }
  LocalizedDescriptionMap& getDescriptions() noexcept { return mDescriptions; }
  const LocalizedDescriptionMap& getDescriptions() const noexcept {
    return mDescriptions;
  }

  // Getters: Geometry
  const FootprintPadList& getPads() const noexcept { return mPads; }
  FootprintPadList&       getPads() noexcept { return mPads; }
  const PolygonList&      getPolygons() const noexcept { return mPolygons; }
  PolygonList&            getPolygons() noexcept { return mPolygons; }
  const CircleList&       getCircles() const noexcept { return mCircles; }
  CircleList&             getCircles() noexcept { return mCircles; }
  const StrokeTextList& getStrokeTexts() const noexcept { return mStrokeTexts; }
  StrokeTextList&       getStrokeTexts() noexcept { return mStrokeTexts; }
  const HoleList&       getHoles() const noexcept { return mHoles; }
  HoleList&             getHoles() noexcept { return mHoles; }

  // General Methods
  void setStrokeFontForAllTexts(const StrokeFont* font) noexcept;
  void registerGraphicsItem(FootprintGraphicsItem& item) noexcept;
  void unregisterGraphicsItem(FootprintGraphicsItem& item) noexcept;

  // General Methods

  /// @copydoc librepcb::SerializableObject::serialize()
  void serialize(SExpression& root) const override;

  // Operator Overloadings
  bool operator==(const Footprint& rhs) const noexcept;
  bool operator!=(const Footprint& rhs) const noexcept {
    return !(*this == rhs);
  }
  Footprint& operator=(const Footprint& rhs) noexcept;

private:  // Methods
  void listObjectAdded(
      const FootprintPadList& list, int newIndex,
      const std::shared_ptr<FootprintPad>& ptr) noexcept override;
  void listObjectAdded(const PolygonList& list, int newIndex,
                       const std::shared_ptr<Polygon>& ptr) noexcept override;
  void listObjectAdded(const CircleList& list, int newIndex,
                       const std::shared_ptr<Circle>& ptr) noexcept override;
  void listObjectAdded(
      const StrokeTextList& list, int newIndex,
      const std::shared_ptr<StrokeText>& ptr) noexcept override;
  void listObjectAdded(const HoleList& list, int newIndex,
                       const std::shared_ptr<Hole>& ptr) noexcept override;
  void listObjectRemoved(
      const FootprintPadList& list, int oldIndex,
      const std::shared_ptr<FootprintPad>& ptr) noexcept override;
  void listObjectRemoved(const PolygonList& list, int oldIndex,
                         const std::shared_ptr<Polygon>& ptr) noexcept override;
  void listObjectRemoved(const CircleList& list, int oldIndex,
                         const std::shared_ptr<Circle>& ptr) noexcept override;
  void listObjectRemoved(
      const StrokeTextList& list, int oldIndex,
      const std::shared_ptr<StrokeText>& ptr) noexcept override;
  void listObjectRemoved(const HoleList& list, int oldIndex,
                         const std::shared_ptr<Hole>& ptr) noexcept override;

private:  // Data
  Uuid                    mUuid;
  LocalizedNameMap        mNames;
  LocalizedDescriptionMap mDescriptions;
  FootprintPadList        mPads;
  PolygonList             mPolygons;
  CircleList              mCircles;
  StrokeTextList          mStrokeTexts;
  HoleList                mHoles;

  const StrokeFont*      mStrokeFont;
  FootprintGraphicsItem* mRegisteredGraphicsItem;
};

/*******************************************************************************
 *  Class FootprintList
 ******************************************************************************/

struct FootprintListNameProvider {
  static constexpr const char* tagname = "footprint";
};
using FootprintList =
    SerializableObjectList<Footprint, FootprintListNameProvider>;
using CmdFootprintInsert =
    CmdListElementInsert<Footprint, FootprintListNameProvider>;
using CmdFootprintRemove =
    CmdListElementRemove<Footprint, FootprintListNameProvider>;
using CmdFootprintsSwap =
    CmdListElementsSwap<Footprint, FootprintListNameProvider>;

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace library
}  // namespace librepcb

#endif  // LIBREPCB_LIBRARY_FOOTPRINT_H
