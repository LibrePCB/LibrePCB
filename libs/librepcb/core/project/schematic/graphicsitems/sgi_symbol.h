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

#ifndef LIBREPCB_CORE_SGI_SYMBOL_H
#define LIBREPCB_CORE_SGI_SYMBOL_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "sgi_base.h"

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class GraphicsLayer;
class SI_Symbol;
class Symbol;
class Text;

/*******************************************************************************
 *  Class SGI_Symbol
 ******************************************************************************/

/**
 * @brief The SGI_Symbol class
 */
class SGI_Symbol final : public SGI_Base {
public:
  // Constructors / Destructor
  SGI_Symbol() = delete;
  SGI_Symbol(const SGI_Symbol& other) = delete;
  explicit SGI_Symbol(SI_Symbol& symbol) noexcept;
  ~SGI_Symbol() noexcept;

  // General Methods
  void updateCacheAndRepaint() noexcept;

  // Inherited from QGraphicsItem
  QRectF boundingRect() const noexcept { return mBoundingRect; }
  QPainterPath shape() const noexcept { return mShape; }
  void paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
             QWidget* widget = 0);

  // Operator Overloadings
  SGI_Symbol& operator=(const SGI_Symbol& rhs) = delete;

private:
  // Private Methods
  GraphicsLayer* getLayer(const QString& name) const noexcept;

  // Types

  struct CachedTextProperties_t {
    QString text;
    int fontPixelSize;
    qreal scaleFactor;
    bool rotate180;
    bool mirrored;
    int flags;
    QRectF textRect;  // not scaled
  };

  // General Attributes
  SI_Symbol& mSymbol;
  const Symbol& mLibSymbol;
  QFont mFont;

  // Cached Attributes
  QRectF mBoundingRect;
  QPainterPath mShape;
  QHash<const Text*, CachedTextProperties_t> mCachedTextProperties;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
