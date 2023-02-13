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

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "bgi_footprintpad.h"

#include "../../../application.h"
#include "../../../library/pkg/footprint.h"
#include "../../../library/pkg/package.h"
#include "../../circuit/netsignal.h"
#include "../../project.h"
#include "../../projectsettings.h"
#include "../board.h"
#include "../boardlayerstack.h"
#include "../items/bi_device.h"
#include "../items/bi_footprintpad.h"

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

BGI_FootprintPad::BGI_FootprintPad(BI_FootprintPad& pad) noexcept
  : BGI_Base(),
    mPad(pad),
    mLibPad(pad.getLibPad()),
    mCopperLayer(nullptr),
    mContents(),
    mOnLayerEditedSlot(*this, &BGI_FootprintPad::layerEdited) {
  mFont = qApp->getDefaultSansSerifFont();
  mFont.setPixelSize(1);

  updateCacheAndRepaint();
}

BGI_FootprintPad::~BGI_FootprintPad() noexcept {
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

bool BGI_FootprintPad::isSelectable() const noexcept {
  return mCopperLayer && mCopperLayer->isVisible();
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void BGI_FootprintPad::updateCacheAndRepaint() noexcept {
  prepareGeometryChange();

  setToolTip(mPad.getDisplayText());

  // Set Z value.
  if (mPad.getComponentSide() == FootprintPad::ComponentSide::Bottom) {
    setZValue(Board::ZValue_FootprintPadsBottom);
  } else {
    setZValue(Board::ZValue_FootprintPadsTop);
  }

  // Determine layers to draw something on (in stackup order).
  QStringList layers{
      GraphicsLayer::sBotSolderPaste,
      GraphicsLayer::sBotStopMask,
      GraphicsLayer::sBotCopper,
  };
  if (mPad.getLibPad().isTht()) {
    for (int i = 1; i <= mPad.getBoard().getLayerStack().getInnerLayerCount();
         ++i) {
      layers.append(GraphicsLayer::getInnerLayerName(i));
    }
  }
  layers += {
      GraphicsLayer::sTopCopper,
      GraphicsLayer::sBoardPadsTht,
      GraphicsLayer::sTopStopMask,
      GraphicsLayer::sTopSolderPaste,
  };

  // Determine content to draw on each layer.
  disconnectLayerEditedSlots();
  mCopperLayer = getLayer(mPad.getLayerName());
  mContents.clear();
  foreach (const QString& layerName, layers) {
    if (GraphicsLayer* layer = getLayer(layerName)) {
      foreach (const PadGeometry& geometry,
               mPad.getGeometryOnLayer(layerName)) {
        mContents.append(LayerContent{
            layer,
            layer->isCopperLayer() ? mCopperLayer : layer,
            geometry.toQPainterPathPx(),
        });
      }
    }
  }
  connectLayerEditedSlots();
  updateVisibility();

  // Set bounding rect and shape.
  mBoundingRect = QRectF();
  foreach (const LayerContent& content, mContents) {
    mBoundingRect |= content.path.boundingRect();
  }
  mShape = mLibPad.getGeometry().toFilledQPainterPathPx();

  update();
}

/*******************************************************************************
 *  Inherited from QGraphicsItem
 ******************************************************************************/

void BGI_FootprintPad::paint(QPainter* painter,
                             const QStyleOptionGraphicsItem* option,
                             QWidget* widget) {
  Q_UNUSED(option);
  Q_UNUSED(widget);

  const NetSignal* netsignal = mPad.getCompSigInstNetSignal();
  bool highlight =
      mPad.isSelected() || (netsignal && netsignal->isHighlighted());

  // Draw bottom non-copper layers.
  foreach (const LayerContent& content, mContents) {
    if ((content.drawLayer != mCopperLayer) &&
        (content.drawLayer->isBottomLayer()) &&
        (content.drawLayer->isVisible())) {
      painter->setPen(Qt::NoPen);
      painter->setBrush(content.drawLayer->getColor(highlight));
      painter->drawPath(content.path);
    }
  }

  // Draw filled copper layers.
  QVector<QPainterPath> filledCopperPaths;
  foreach (const LayerContent& content, mContents) {
    if ((content.drawLayer == mCopperLayer) && content.drawLayer->isVisible() &&
        content.visibilityLayer->isEnabled() &&
        content.visibilityLayer->isVisible() &&
        (!filledCopperPaths.contains(content.path))) {
      painter->setPen(Qt::NoPen);
      painter->setBrush(content.drawLayer->getColor(highlight));
      painter->drawPath(content.path);
      filledCopperPaths.append(content.path);
    }
  }

  // Draw outline copper layers.
  QVector<QPainterPath> outlineCopperPaths;
  foreach (const LayerContent& content, mContents) {
    if ((content.drawLayer == mCopperLayer) && content.drawLayer->isVisible() &&
        content.visibilityLayer->isEnabled() &&
        (!content.visibilityLayer->isVisible()) &&
        (!filledCopperPaths.contains(content.path)) &&
        (!outlineCopperPaths.contains(content.path))) {
      painter->setPen(QPen(content.drawLayer->getColor(highlight), 0));
      painter->setBrush(Qt::NoBrush);
      painter->drawPath(content.path);
      outlineCopperPaths.append(content.path);
    }
  }

  // Draw text.
  if (mCopperLayer && mCopperLayer->isVisible()) {
    painter->setFont(mFont);
    painter->setPen(mCopperLayer->getColor(highlight).lighter(150));
    painter->drawText(mShape.boundingRect(), Qt::AlignCenter,
                      mPad.getDisplayText());
  }

  // Draw top non-copper layers.
  foreach (const LayerContent& content, mContents) {
    if ((content.drawLayer != mCopperLayer) &&
        (content.drawLayer->isTopLayer()) && (content.drawLayer->isVisible())) {
      painter->setPen(Qt::NoPen);
      painter->setBrush(content.drawLayer->getColor(highlight));
      painter->drawPath(content.path);
    }
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

GraphicsLayer* BGI_FootprintPad::getLayer(const QString& name) const noexcept {
  return mPad.getDevice().getBoard().getLayerStack().getLayer(name);
}

QSet<GraphicsLayer*> BGI_FootprintPad::getAllInvolvedLayers() const noexcept {
  QSet<GraphicsLayer*> layers;
  foreach (const LayerContent& content, mContents) {
    layers.insert(content.visibilityLayer);
    layers.insert(content.drawLayer);
  }
  return layers;
}

void BGI_FootprintPad::connectLayerEditedSlots() noexcept {
  foreach (GraphicsLayer* layer, getAllInvolvedLayers()) {
    layer->onEdited.attach(mOnLayerEditedSlot);
  }
}

void BGI_FootprintPad::disconnectLayerEditedSlots() noexcept {
  foreach (GraphicsLayer* layer, getAllInvolvedLayers()) {
    layer->onEdited.detach(mOnLayerEditedSlot);
  }
}

void BGI_FootprintPad::layerEdited(const GraphicsLayer& layer,
                                   GraphicsLayer::Event event) noexcept {
  Q_UNUSED(layer);

  switch (event) {
    case GraphicsLayer::Event::ColorChanged:
      update();
      break;
    case GraphicsLayer::Event::HighlightColorChanged:
      update();
      break;
    case GraphicsLayer::Event::VisibleChanged:
    case GraphicsLayer::Event::EnabledChanged:
      updateVisibility();
      break;
    default:
      break;
  }
}

void BGI_FootprintPad::updateVisibility() noexcept {
  bool visible = (mCopperLayer && mCopperLayer->isVisible());
  if (!visible) {
    foreach (const LayerContent& content, mContents) {
      if (content.visibilityLayer->isVisible() &&
          content.drawLayer->isVisible()) {
        visible = true;
        break;
      }
    }
  }
  setVisible(visible);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
