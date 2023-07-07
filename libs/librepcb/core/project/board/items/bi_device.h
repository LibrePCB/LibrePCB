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

#ifndef LIBREPCB_CORE_BI_DEVICE_H
#define LIBREPCB_CORE_BI_DEVICE_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../../attribute/attribute.h"
#include "../../../geometry/stroketext.h"
#include "../../../types/uuid.h"
#include "../../../utils/signalslot.h"
#include "bi_base.h"
#include "bi_stroketext.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class BI_FootprintPad;
class Board;
class ComponentInstance;
class Device;
class Footprint;
class Package;
class PackageModel;
class Project;

/*******************************************************************************
 *  Class BI_Device
 ******************************************************************************/

/**
 * @brief The BI_Device class
 */
class BI_Device final : public BI_Base {
  Q_OBJECT

public:
  // Signals
  enum class Event {
    BoardLayersChanged,
    PositionChanged,
    RotationChanged,
    MirroredChanged,
    StopMaskOffsetsChanged,
  };
  Signal<BI_Device, Event> onEdited;
  typedef Slot<BI_Device, Event> OnEditedSlot;

  // Constructors / Destructor
  BI_Device() = delete;
  BI_Device(const BI_Device& other) = delete;
  BI_Device(Board& board, ComponentInstance& compInstance,
            const Uuid& deviceUuid, const Uuid& footprintUuid,
            const Point& position, const Angle& rotation, bool mirror,
            bool locked, bool loadInitialStrokeTexts);
  ~BI_Device() noexcept;

  // Getters
  const Uuid& getComponentInstanceUuid() const noexcept;
  ComponentInstance& getComponentInstance() const noexcept {
    return mCompInstance;
  }
  const Device& getLibDevice() const noexcept { return *mLibDevice; }
  const Package& getLibPackage() const noexcept { return *mLibPackage; }
  const Footprint& getLibFootprint() const noexcept { return *mLibFootprint; }
  const PackageModel* getLibModel() const noexcept { return mLibModel; }
  tl::optional<Uuid> getLibModelUuid() const noexcept;
  tl::optional<Uuid> getDefaultLibModelUuid() const noexcept;
  const Point& getPosition() const noexcept { return mPosition; }
  const Angle& getRotation() const noexcept { return mRotation; }
  bool getMirrored() const noexcept { return mMirrored; }
  bool isLocked() const noexcept { return mLocked; }
  const AttributeList& getAttributes() const noexcept { return mAttributes; }
  BI_FootprintPad* getPad(const Uuid& padUuid) const noexcept {
    return mPads.value(padUuid);
  }
  const QMap<Uuid, BI_FootprintPad*>& getPads() const noexcept { return mPads; }
  const QHash<Uuid, tl::optional<Length>>& getHoleStopMasks() const noexcept {
    return mHoleStopMaskOffsets;
  }
  bool isUsed() const noexcept;

  // Setters
  void setPosition(const Point& pos) noexcept;
  void setRotation(const Angle& rot) noexcept;
  void setMirrored(bool mirror);
  void setLocked(bool locked) noexcept;
  void setAttributes(const AttributeList& attributes) noexcept;
  void setModel(const tl::optional<Uuid>& uuid);

  // StrokeText Methods
  StrokeTextList getDefaultStrokeTexts() const noexcept;
  const QMap<Uuid, BI_StrokeText*>& getStrokeTexts() const noexcept {
    return mStrokeTexts;
  }
  void addStrokeText(BI_StrokeText& text);
  void removeStrokeText(BI_StrokeText& text);

  // General Methods
  void addToBoard() override;
  void removeFromBoard() override;

  /**
   * @brief Serialize into ::librepcb::SExpression node
   *
   * @param root    Root node to serialize into.
   */
  void serialize(SExpression& root) const;

  // Operator Overloadings
  BI_Device& operator=(const BI_Device& rhs) = delete;

signals:
  void attributesChanged();

  void strokeTextAdded(BI_StrokeText& strokeText);
  void strokeTextRemoved(BI_StrokeText& strokeText);

private:
  bool checkAttributesValidity() const noexcept;
  void updateHoleStopMaskOffsets() noexcept;
  const QStringList& getLocaleOrder() const noexcept;

  // General
  ComponentInstance& mCompInstance;
  const Device* mLibDevice;
  const Package* mLibPackage;
  const Footprint* mLibFootprint;
  const PackageModel* mLibModel;  ///< `nullptr` if no model available/selected

  // Attributes
  Point mPosition;
  Angle mRotation;
  bool mMirrored;
  bool mLocked;
  AttributeList mAttributes;  ///< Not used yet, but specified in file format

  QMap<Uuid, BI_FootprintPad*> mPads;  ///< key: footprint pad UUID
  QMap<Uuid, BI_StrokeText*> mStrokeTexts;
  QHash<Uuid, tl::optional<Length>> mHoleStopMaskOffsets;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
