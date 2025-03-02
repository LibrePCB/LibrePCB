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

#ifndef LIBREPCB_EDITOR_SGI_NETLINE_H
#define LIBREPCB_EDITOR_SGI_NETLINE_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/core/project/schematic/items/si_netline.h>

#include <QtCore>
#include <QtWidgets>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

class GraphicsLayer;
class IF_GraphicsLayerProvider;

/*******************************************************************************
 *  Class SGI_NetLine
 ******************************************************************************/

/**
 * @brief The SGI_NetLine class
 */
class SGI_NetLine final : public QGraphicsItem {
public:
  // Constructors / Destructor
  SGI_NetLine() = delete;
  SGI_NetLine(const SGI_NetLine& other) = delete;
  SGI_NetLine(SI_NetLine& netline, const IF_GraphicsLayerProvider& lp,
              std::shared_ptr<const QSet<const NetSignal*>>
                  highlightedNetSignals) noexcept;
  virtual ~SGI_NetLine() noexcept;

  // General Methods
  SI_NetLine& getNetLine() noexcept { return mNetLine; }

  // Inherited from QGraphicsItem
  QRectF boundingRect() const noexcept override { return mBoundingRect; }
  QPainterPath shape() const noexcept override;
  void paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
             QWidget* widget) noexcept override;

  // Operator Overloadings
  SGI_NetLine& operator=(const SGI_NetLine& rhs) = delete;

private:  // Methods
  void netLineEdited(const SI_NetLine& obj, SI_NetLine::Event event) noexcept;
  void updatePositions() noexcept;
  void updateNetSignalName() noexcept;
  std::shared_ptr<GraphicsLayer> getLayer(const QString& name) const noexcept;

private:  // Data
  SI_NetLine& mNetLine;
  std::shared_ptr<const QSet<const NetSignal*>> mHighlightedNetSignals;
  std::shared_ptr<GraphicsLayer> mLayer;

  // Cached Attributes
  QLineF mLineF;
  QRectF mBoundingRect;
  QPainterPath mShape;

  // Slots
  SI_NetLine::OnEditedSlot mOnNetLineEditedSlot;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
