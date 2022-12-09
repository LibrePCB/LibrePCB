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
#include "../../../attribute/attributeprovider.h"
#include "../../../types/uuid.h"
#include "../../erc/if_ercmsgprovider.h"
#include "../graphicsitems/bgi_device.h"
#include "bi_base.h"
#include "bi_stroketext.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Board;
class ComponentInstance;
class Device;
class Footprint;
class Package;
class Project;

/*******************************************************************************
 *  Class BI_Device
 ******************************************************************************/

/**
 * @brief The BI_Device class
 */
class BI_Device final : public BI_Base,
                        public AttributeProvider,
                        public IF_ErcMsgProvider {
  Q_OBJECT
  DECLARE_ERC_MSG_CLASS_NAME(BI_Device)

public:
  // Types
  enum class MountType { Tht, Smt, Fiducial, Other, None };

  // Constructors / Destructor
  BI_Device() = delete;
  BI_Device(const BI_Device& other) = delete;
  BI_Device(Board& board, ComponentInstance& compInstance,
            const Uuid& deviceUuid, const Uuid& footprintUuid,
            const Point& position, const Angle& rotation, bool mirror,
            bool loadInitialStrokeTexts);
  ~BI_Device() noexcept;

  // Getters
  const Uuid& getComponentInstanceUuid() const noexcept;
  ComponentInstance& getComponentInstance() const noexcept {
    return mCompInstance;
  }
  const Device& getLibDevice() const noexcept { return *mLibDevice; }
  const Package& getLibPackage() const noexcept { return *mLibPackage; }
  const Footprint& getLibFootprint() const noexcept { return *mLibFootprint; }
  const Point& getPosition() const noexcept { return mPosition; }
  const Angle& getRotation() const noexcept { return mRotation; }
  bool getMirrored() const noexcept { return mMirrored; }
  const AttributeList& getAttributes() const noexcept { return mAttributes; }
  BI_FootprintPad* getPad(const Uuid& padUuid) const noexcept {
    return mPads.value(padUuid);
  }
  const QMap<Uuid, BI_FootprintPad*>& getPads() const noexcept { return mPads; }
  bool isSelectable() const noexcept override;
  bool isUsed() const noexcept;
  QRectF getBoundingRect() const noexcept;
  BGI_Device& getGraphicsItem() noexcept { return *mGraphicsItem; }

  /**
   * @brief Determine the mount (assembly) type of this device
   *
   * By default, this is automatically detected from the footprint pads (THT or
   * SMT). If there are no pads, ::librepcb::BI_Device::MountType::None is
   * returned since probably it is just a logo or so.
   *
   * However, the user can manually override this mechanism by adding a
   * device property `MOUNT_TYPE` set to `THT`, `SMT`, `FIDUCIAL`, `OTHER` or
   * `NONE`.
   *
   * Note that this mechanism should be refactored in v0.2, see
   * https://github.com/LibrePCB/LibrePCB/issues/1001 for details.
   *
   * @return The determined device mount type.
   */
  MountType determineMountType() const noexcept;

  // Setters
  void setPosition(const Point& pos) noexcept;
  void setRotation(const Angle& rot) noexcept;
  void setMirrored(bool mirror);
  void setAttributes(const AttributeList& attributes) noexcept;

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

  // Inherited from AttributeProvider
  /// @copydoc ::librepcb::AttributeProvider::getUserDefinedAttributeValue()
  QString getUserDefinedAttributeValue(const QString& key) const
      noexcept override;
  /// @copydoc ::librepcb::AttributeProvider::getBuiltInAttributeValue()
  QString getBuiltInAttributeValue(const QString& key) const noexcept override;
  /// @copydoc ::librepcb::AttributeProvider::getAttributeProviderParents()
  QVector<const AttributeProvider*> getAttributeProviderParents() const
      noexcept override;

  // Inherited from BI_Base
  Type_t getType() const noexcept override { return BI_Base::Type_t::Device; }
  QPainterPath getGrabAreaScenePx() const noexcept override;
  void setSelected(bool selected) noexcept override;

  // Operator Overloadings
  BI_Device& operator=(const BI_Device& rhs) = delete;

signals:
  /// @copydoc AttributeProvider::attributesChanged()
  void attributesChanged() override;

private:
  bool checkAttributesValidity() const noexcept;
  void updateGraphicsItemTransform() noexcept;
  const QStringList& getLocaleOrder() const noexcept;

  // General
  ComponentInstance& mCompInstance;
  const Device* mLibDevice;
  const Package* mLibPackage;
  const Footprint* mLibFootprint;

  // Attributes
  Point mPosition;
  Angle mRotation;
  bool mMirrored;
  AttributeList
      mAttributes;  ///< not yet used, but already specified in file format

  QMap<Uuid, BI_FootprintPad*> mPads;  ///< key: footprint pad UUID
  QMap<Uuid, BI_StrokeText*> mStrokeTexts;

  QScopedPointer<BGI_Device> mGraphicsItem;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
