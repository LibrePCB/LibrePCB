/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 Urban Bruhin
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

#ifndef BOARDLAYER_H
#define BOARDLAYER_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include <QtWidgets>

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
class BoardLayer final : public QObject
{
        Q_OBJECT

    public:

        // All Board Layer IDs
        enum LayerID {

            // Special Layers
            Grid                    = 1,
            OriginCrosses           = 2,
            Unrouted                = 3,
            FootprintGrabAreas      = 4,

            // General Layers
            BoardOutline            = 10,
            Drills                  = 20,
            Vias                    = 30,

            // Top Layers
            TopKeepout              = 100,
            TopDeviceOutlines       = 110,
            TopTestPoint            = 120,
            TopGlue                 = 130,
            TopPaste                = 140,
            TopOverlayNames         = 150,
            TopOverlayValues        = 160,
            TopOverlay              = 170,
            TopStopMask             = 180,

            // Copper Layers
            TopCopper               = 200,
            InnerCopper1            = 201,
            InnerCopper2            = 202,
            InnerCopper3            = 203,
            InnerCopper4            = 204,
            InnerCopper5            = 205,
            InnerCopper6            = 206,
            InnerCopper7            = 207,
            InnerCopper8            = 208,
            InnerCopper9            = 209,
            InnerCopper10           = 210,
            InnerCopper11           = 211,
            InnerCopper12           = 212,
            InnerCopper13           = 213,
            InnerCopper14           = 214,
            InnerCopper15           = 215,
            InnerCopper16           = 216,
            InnerCopper17           = 217,
            InnerCopper18           = 218,
            InnerCopper19           = 219,
            InnerCopper20           = 220,
            InnerCopper21           = 221,
            InnerCopper22           = 222,
            InnerCopper23           = 223,
            InnerCopper24           = 224,
            InnerCopper25           = 225,
            InnerCopper26           = 226,
            InnerCopper27           = 227,
            InnerCopper28           = 228,
            InnerCopper29           = 229,
            InnerCopper30           = 230,
            InnerCopper31           = 231,
            InnerCopper32           = 232,
            InnerCopper33           = 233,
            InnerCopper34           = 234,
            InnerCopper35           = 235,
            InnerCopper36           = 236,
            InnerCopper37           = 237,
            InnerCopper38           = 238,
            InnerCopper39           = 239,
            InnerCopper40           = 240,
            InnerCopper41           = 241,
            InnerCopper42           = 242,
            InnerCopper43           = 243,
            InnerCopper44           = 244,
            InnerCopper45           = 245,
            InnerCopper46           = 246,
            InnerCopper47           = 247,
            InnerCopper48           = 248,
            InnerCopper49           = 249,
            InnerCopper50           = 250,
            InnerCopper51           = 251,
            InnerCopper52           = 252,
            InnerCopper53           = 253,
            InnerCopper54           = 254,
            InnerCopper55           = 255,
            InnerCopper56           = 256,
            InnerCopper57           = 257,
            InnerCopper58           = 258,
            InnerCopper59           = 259,
            InnerCopper60           = 260,
            InnerCopper61           = 261,
            InnerCopper62           = 262,
            InnerCopper63           = 263,
            InnerCopper64           = 264,
            InnerCopper65           = 265,
            InnerCopper66           = 266,
            InnerCopper67           = 267,
            InnerCopper68           = 268,
            InnerCopper69           = 269,
            InnerCopper70           = 270,
            InnerCopper71           = 271,
            InnerCopper72           = 272,
            InnerCopper73           = 273,
            InnerCopper74           = 274,
            InnerCopper75           = 275,
            InnerCopper76           = 276,
            InnerCopper77           = 277,
            InnerCopper78           = 278,
            InnerCopper79           = 279,
            InnerCopper80           = 280,
            InnerCopper81           = 281,
            InnerCopper82           = 282,
            InnerCopper83           = 283,
            InnerCopper84           = 284,
            InnerCopper85           = 285,
            InnerCopper86           = 286,
            InnerCopper87           = 287,
            InnerCopper88           = 288,
            InnerCopper89           = 289,
            InnerCopper90           = 290,
            InnerCopper91           = 291,
            InnerCopper92           = 292,
            InnerCopper93           = 293,
            InnerCopper94           = 294,
            InnerCopper95           = 295,
            InnerCopper96           = 296,
            InnerCopper97           = 297,
            InnerCopper98           = 298,
            BottomCopper            = 299,

            // Bottom Layers
            BottomStopMask          = 310,
            BottomOverlay           = 320,
            BottomOverlayValues     = 330,
            BottomOverlayNames      = 340,
            BottomPaste             = 350,
            BottomGlue              = 360,
            BottomTestPoint         = 370,
            BottomDeviceOutlines    = 380,
            BootomKeepout           = 390,

            // TODO: keepout, restrict, ...

            // Begin of User defined Layers
            UserDefinedBaseId   = 1000
        };

        // Constructors / Destructor
        explicit BoardLayer(uint id);
        ~BoardLayer();

        // Getters
        uint getId() const {return mId;}
        const QString& getName() const {return mName;}
        const QColor& getColor(bool highlighted = false) const;


    private:

        // make some methods inaccessible...
        BoardLayer();
        BoardLayer(const BoardLayer& other);
        BoardLayer& operator=(const BoardLayer& rhs);


        // Attributes
        uint mId;
        QString mName;
        QColor mColor;
        QColor mColorHighlighted;
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

    @todo Add more details. See https://github.com/ubruhin/LibrePCB/issues/4

*/

#endif // BOARDLAYER_H
