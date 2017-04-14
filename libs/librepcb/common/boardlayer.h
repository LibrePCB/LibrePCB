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

#ifndef LIBREPCB_BOARDLAYER_H
#define LIBREPCB_BOARDLAYER_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include <QtWidgets>
#include "fileio/serializableobject.h"

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {

/*****************************************************************************************
 *  Class BoardLayer
 ****************************************************************************************/

/**
 * @brief The BoardLayer class
 *
 * @note See @ref doc_board_layers for more information about the whole concept.
 *
 * @author ubruhin
 * @date 2015-06-05
 */
class BoardLayer final : public QObject, public SerializableObject
{
        Q_OBJECT

    public:

        /// All Board Layer IDs
        enum LayerID {

            // Special Layers
            Grid                        = 1,
            Unrouted                    = 3,

            // General Layers
            BoardOutlines               = 10,
            Drills                      = 20, // NPTH
            Vias                        = 30, // PTH
            ViaRestrict                 = 40,
            ThtPads                     = 50, // PTH

            // Top Layers (Note: Layer IDs must be mirrored from bottom layers!)
            _TOP_LAYERS_START           = 200, ///< required for mirroring layers
            TopDeviceOutlines           = 200,
            TopDeviceOriginCrosses      = 201,
            TopDeviceGrabAreas          = 202,
            TopTestPoints               = 210,
            TopGlue                     = 220,
            TopPaste                    = 230,
            TopOverlayNames             = 240,
            TopOverlayValues            = 250,
            TopOverlay                  = 260,
            TopStopMask                 = 270,
            TopDeviceKeepout            = 280,
            TopCopperRestrict           = 290,
            _TOP_LAYERS_END             = 300, ///< required for mirroring layers

            // Copper Layers
            _COPPER_LAYERS_START        = 300,
            TopCopper                   = 300,
            InnerCopper1                = 301,
            InnerCopper2                = 302,
            InnerCopper3                = 303,
            InnerCopper4                = 304,
            InnerCopper5                = 305,
            InnerCopper6                = 306,
            InnerCopper7                = 307,
            InnerCopper8                = 308,
            InnerCopper9                = 309,
            InnerCopper10               = 310,
            InnerCopper11               = 311,
            InnerCopper12               = 312,
            InnerCopper13               = 313,
            InnerCopper14               = 314,
            InnerCopper15               = 315,
            InnerCopper16               = 316,
            InnerCopper17               = 317,
            InnerCopper18               = 318,
            InnerCopper19               = 319,
            InnerCopper20               = 320,
            InnerCopper21               = 321,
            InnerCopper22               = 322,
            InnerCopper23               = 323,
            InnerCopper24               = 324,
            InnerCopper25               = 325,
            InnerCopper26               = 326,
            InnerCopper27               = 327,
            InnerCopper28               = 328,
            InnerCopper29               = 329,
            InnerCopper30               = 330,
            InnerCopper31               = 331,
            InnerCopper32               = 332,
            InnerCopper33               = 333,
            InnerCopper34               = 334,
            InnerCopper35               = 335,
            InnerCopper36               = 336,
            InnerCopper37               = 337,
            InnerCopper38               = 338,
            InnerCopper39               = 339,
            InnerCopper40               = 340,
            InnerCopper41               = 341,
            InnerCopper42               = 342,
            InnerCopper43               = 343,
            InnerCopper44               = 344,
            InnerCopper45               = 345,
            InnerCopper46               = 346,
            InnerCopper47               = 347,
            InnerCopper48               = 348,
            InnerCopper49               = 349,
            InnerCopper50               = 350,
            InnerCopper51               = 351,
            InnerCopper52               = 352,
            InnerCopper53               = 353,
            InnerCopper54               = 354,
            InnerCopper55               = 355,
            InnerCopper56               = 356,
            InnerCopper57               = 357,
            InnerCopper58               = 358,
            InnerCopper59               = 359,
            InnerCopper60               = 360,
            InnerCopper61               = 361,
            InnerCopper62               = 362,
            InnerCopper63               = 363,
            InnerCopper64               = 364,
            InnerCopper65               = 365,
            InnerCopper66               = 366,
            InnerCopper67               = 367,
            InnerCopper68               = 368,
            InnerCopper69               = 369,
            InnerCopper70               = 370,
            InnerCopper71               = 371,
            InnerCopper72               = 372,
            InnerCopper73               = 373,
            InnerCopper74               = 374,
            InnerCopper75               = 375,
            InnerCopper76               = 376,
            InnerCopper77               = 377,
            InnerCopper78               = 378,
            InnerCopper79               = 379,
            InnerCopper80               = 380,
            InnerCopper81               = 381,
            InnerCopper82               = 382,
            InnerCopper83               = 383,
            InnerCopper84               = 384,
            InnerCopper85               = 385,
            InnerCopper86               = 386,
            InnerCopper87               = 387,
            InnerCopper88               = 388,
            InnerCopper89               = 389,
            InnerCopper90               = 390,
            InnerCopper91               = 391,
            InnerCopper92               = 392,
            InnerCopper93               = 393,
            InnerCopper94               = 394,
            InnerCopper95               = 395,
            InnerCopper96               = 396,
            InnerCopper97               = 397,
            InnerCopper98               = 398,
            InnerCopper99               = 399,
            BottomCopper                = 400,
            _COPPER_LAYERS_END          = 400,

            // Bottom Layers (Note: Layer IDs must be mirrored from top layers!)
            _BOTTOM_LAYERS_START        = 400, ///< required for mirroring layers
            BottomCopperRestrict        = 410,
            BottomDeviceKeepout         = 420,
            BottomStopMask              = 430,
            BottomOverlay               = 440,
            BottomOverlayValues         = 450,
            BottomOverlayNames          = 460,
            BottomPaste                 = 470,
            BottomGlue                  = 480,
            BottomTestPoints            = 490,
            BottomDeviceGrabAreas       = 498,
            BottomDeviceOriginCrosses   = 499,
            BottomDeviceOutlines        = 500,
            _BOTTOM_LAYERS_END          = 500, ///< required for mirroring layers

#ifdef QT_DEBUG
            // IDs 900-999: debug layers (for developers)
            DEBUG_GraphicsItemsBoundingRects         = 900,
            DEBUG_GraphicsItemsTextsBoundingRects    = 901,
#endif

            // Begin of User defined Layers
            UserDefinedBaseId   = 1000
        };

        // Constructors / Destructor
        BoardLayer() = delete;
        explicit BoardLayer(const BoardLayer& other) throw (Exception);
        explicit BoardLayer(const XmlDomElement& domElement) throw (Exception);
        explicit BoardLayer(int id);
        ~BoardLayer();

        // Getters
        int getId() const {return mId;}
        const QString& getName() const {return mName;}
        const QColor& getColor(bool highlighted = false) const;
        bool isVisible() const noexcept {return mIsVisible;}
        bool isCopperLayer() const noexcept {return isCopperLayer(mId);}
        int getMirroredLayerId() const noexcept {return getMirroredLayerId(mId);}

        // Setters
        void setVisible(bool visible) noexcept {mIsVisible = visible; emit attributesChanged();}

        // General Methods
        /// @copydoc librepcb::SerializableObject::serialize()
        void serialize(XmlDomElement& root) const throw (Exception) override;

        // Operator Overloadings
        BoardLayer& operator=(const BoardLayer& rhs) = delete;

        // Static Methods
        static bool isCopperLayer(int id) noexcept;
        static int getMirroredLayerId(int id) noexcept;


    signals:

        void attributesChanged();


    private:

        // Attributes
        int mId;
        QString mName;
        QColor mColor;
        QColor mColorHighlighted;
        bool mIsVisible;
};

/*****************************************************************************************
 *  Doxygen Documentation
 ****************************************************************************************/

/**
    @page doc_board_layers Board Layers Documentation

    @tableofcontents

    @section doc_board_layer_class Class BoardLayer
        The class #BoardLayer is used to representate footprint/board layers. for
        each layer you need to create an object of that type.

    @section doc_board_layer_attributes Layer Attributes
        Each board layer has the following attibutes:
            - ID: An unsigned integer which identifies the layer (must be unique)
            - Name: The name of the layer (translated into the user's language)
            - Color: The color which is used to draw elements of that layer
            - Color (highlighted): The color for highlighted (selected) elements
            - Visible: Defines whether the layer is visible (true) or not (false)

    @section doc_board_layer_ids Layer IDs
        All available layers are listed in BoardLayer#LayerID.

    @todo Add more details

*/

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace librepcb

#endif // LIBREPCB_BOARDLAYER_H
