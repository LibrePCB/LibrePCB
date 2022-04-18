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

#ifndef LIBREPCB_EDITOR_FOOTPRINTPREVIEWGRAPHICSITEM_H
#define LIBREPCB_EDITOR_FOOTPRINTPREVIEWGRAPHICSITEM_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/core/attribute/attributeprovider.h>
#include <librepcb/core/geometry/stroketext.h>
#include <librepcb/core/types/uuid.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Component;
class Footprint;
class GraphicsLayer;
class IF_GraphicsLayerProvider;
class Package;

namespace editor {

/*******************************************************************************
 *  Class FootprintPreviewGraphicsItem
 ******************************************************************************/

/**
 * @brief The FootprintPreviewGraphicsItem class
 */
class FootprintPreviewGraphicsItem final : public QGraphicsItem,
                                           public AttributeProvider {
public:
  // Constructors / Destructor
  FootprintPreviewGraphicsItem() = delete;
  FootprintPreviewGraphicsItem(const FootprintPreviewGraphicsItem& other) =
      delete;
  explicit FootprintPreviewGraphicsItem(
      const IF_GraphicsLayerProvider& layerProvider,
      const QStringList& localeOrder, const Footprint& footprint,
      const Package* package = nullptr, /*const Device* device = nullptr,*/
      const Component* component = nullptr,
      const AttributeProvider* attrProvider = nullptr) noexcept;
  ~FootprintPreviewGraphicsItem() noexcept;

  // General Methods
  void updateCacheAndRepaint() noexcept;

  // Inherited from QGraphicsItem
  QRectF boundingRect() const noexcept override { return mBoundingRect; }
  QPainterPath shape() const noexcept override { return mShape; }
  void paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
             QWidget* widget = 0) noexcept override;

  // Operator Overloadings
  FootprintPreviewGraphicsItem& operator=(
      const FootprintPreviewGraphicsItem& rhs) = delete;

signals:

  /// @copydoc AttributeProvider::attributesChanged()
  void attributesChanged() override {}

private:
  // Inherited from AttributeProvider
  /// @copydoc ::librepcb::AttributeProvider::getBuiltInAttributeValue()
  QString getBuiltInAttributeValue(const QString& key) const noexcept override;

  // General Attributes
  const IF_GraphicsLayerProvider& mLayerProvider;
  const Footprint& mFootprint;
  const Package* mPackage;
  // const Device* mDevice;
  const Component* mComponent;
  const AttributeProvider* mAttributeProvider;
  QStringList mLocaleOrder;
  StrokeTextList mStrokeTexts;

  // Cached Attributes
  QRectF mBoundingRect;
  QPainterPath mShape;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
