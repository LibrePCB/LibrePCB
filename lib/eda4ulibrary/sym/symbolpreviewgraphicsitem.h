/*
 * EDA4U - Professional EDA for everyone!
 * Copyright (C) 2013 Urban Bruhin
 * http://eda4u.ubruhin.ch/
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

#ifndef LIBRARY_SYMBOLPREVIEWGRAPHICSITEM_H
#define LIBRARY_SYMBOLPREVIEWGRAPHICSITEM_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include <QtWidgets>
#include <eda4ucommon/graphics/graphicsitem.h>
#include <eda4ucommon/if_attributeprovider.h>

/*****************************************************************************************
 *  Forward Declarations
 ****************************************************************************************/

class SchematicLayer;
class IF_SchematicLayerProvider;

namespace library {
class Symbol;
class SymbolText;
class GenericComponent;
class GenCompSymbVarItem;
}

/*****************************************************************************************
 *  Class SymbolPreviewGraphicsItem
 ****************************************************************************************/

namespace library {

/**
 * @brief The SymbolPreviewGraphicsItem class
 *
 * @author ubruhin
 * @date 2015-04-21
 */
class SymbolPreviewGraphicsItem final : public GraphicsItem, public IF_AttributeProvider
{
    public:

        // Constructors / Destructor
        explicit SymbolPreviewGraphicsItem(const IF_SchematicLayerProvider& layerProvider,
                                           const QStringList& localeOrder,
                                           const Symbol& symbol,
                                           const GenericComponent* genComp = nullptr,
                                           const QUuid& symbVarUuid = QUuid(),
                                           const QUuid& symbVarItemUuid = QUuid()) noexcept;
        ~SymbolPreviewGraphicsItem() noexcept;

        // Setters
        void setDrawBoundingRect(bool enable) noexcept;

        // General Methods
        void updateCacheAndRepaint() noexcept;

        // Inherited from QGraphicsItem
        QRectF boundingRect() const noexcept {return mBoundingRect;}
        QPainterPath shape() const noexcept {return mShape;}
        void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = 0);


    signals:

        /// @copydoc IF_AttributeProvider#attributesChanged()
        void attributesChanged() {}


    private:

        // make some methods inaccessible...
        SymbolPreviewGraphicsItem() = delete;
        SymbolPreviewGraphicsItem(const SymbolPreviewGraphicsItem& other) = delete;
        SymbolPreviewGraphicsItem& operator=(const SymbolPreviewGraphicsItem& rhs) = delete;


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
        const IF_SchematicLayerProvider& mLayerProvider;
        const Symbol& mSymbol;
        const GenericComponent* mGenComp;
        const GenCompSymbVarItem* mSymbVarItem;
        QFont mFont;
        bool mDrawBoundingRect;
        QStringList mLocaleOrder;

        // Cached Attributes
        QRectF mBoundingRect;
        QPainterPath mShape;
        QHash<const SymbolText*, CachedTextProperties_t> mCachedTextProperties;
};

} // namespace project

#endif // LIBRARY_SYMBOLPREVIEWGRAPHICSITEM_H
