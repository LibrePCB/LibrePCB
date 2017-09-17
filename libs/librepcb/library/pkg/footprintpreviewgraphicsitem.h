/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
 * http://librepcb.org/
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

#ifndef LIBREPCB_LIBRARY_FOOTPRINTPREVIEWGRAPHICSITEM_H
#define LIBREPCB_LIBRARY_FOOTPRINTPREVIEWGRAPHICSITEM_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include <QtWidgets>
#include <librepcb/common/uuid.h>
#include <librepcb/common/attributes/attributeprovider.h>

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {

class Text;
class GraphicsLayer;
class IF_GraphicsLayerProvider;

namespace library {

class Component;
//class Device;
class Package;
class Footprint;

/*****************************************************************************************
 *  Class FootprintPreviewGraphicsItem
 ****************************************************************************************/

/**
 * @brief The FootprintPreviewGraphicsItem class
 *
 * @author ubruhin
 * @date 2015-04-21
 */
class FootprintPreviewGraphicsItem final : public QGraphicsItem, public AttributeProvider
{
    public:

        // Constructors / Destructor
        explicit FootprintPreviewGraphicsItem(const IF_GraphicsLayerProvider& layerProvider,
            const QStringList& localeOrder, const Footprint& footprint,
            const Package* package = nullptr, /*const Device* device = nullptr,*/
            const Component* component = nullptr,
            const AttributeProvider* attrProvider = nullptr) noexcept;
        ~FootprintPreviewGraphicsItem() noexcept;

        // Setters
        void setDrawBoundingRect(bool enable) noexcept;

        // General Methods
        void updateCacheAndRepaint() noexcept;

        // Inherited from QGraphicsItem
        QRectF boundingRect() const noexcept {return mBoundingRect;}
        QPainterPath shape() const noexcept {return mShape;}
        void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = 0);


    signals:

        /// @copydoc AttributeProvider::attributesChanged()
        void attributesChanged() {}


    private:

        // make some methods inaccessible...
        FootprintPreviewGraphicsItem() = delete;
        FootprintPreviewGraphicsItem(const FootprintPreviewGraphicsItem& other) = delete;
        FootprintPreviewGraphicsItem& operator=(const FootprintPreviewGraphicsItem& rhs) = delete;


        // private methods
        bool getAttributeValue(const QString& attrNS, const QString& attrKey,
                               bool passToParents, QString& value) const noexcept;


        // Types

        struct CachedTextProperties_t {
            QString text;
            qreal fontSize;
            bool rotate180;
            Qt::Alignment align;
            QRectF textRect;
        };


        // General Attributes
        const IF_GraphicsLayerProvider& mLayerProvider;
        const Footprint& mFootprint;
        const Package* mPackage;
        //const Device* mDevice;
        const Component* mComponent;
        const AttributeProvider* mAttributeProvider;
        QFont mFont;
        bool mDrawBoundingRect;
        QStringList mLocaleOrder;

        // Cached Attributes
        QRectF mBoundingRect;
        QPainterPath mShape;
        QHash<const Text*, CachedTextProperties_t> mCachedTextProperties;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace library
} // namespace librepcb

#endif // LIBREPCB_LIBRARY_FOOTPRINTPREVIEWGRAPHICSITEM_H
