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

#ifndef SCHEMATICLAYER_H
#define SCHEMATICLAYER_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include <QtWidgets>

/*****************************************************************************************
 *  Class SchematicLayer
 ****************************************************************************************/

/**
 * @brief The SchematicLayer class
 *
 * @note See @ref doc_schematic_layers for more information about the whole concept.
 */
class SchematicLayer final : public QObject
{
        Q_OBJECT

    public:

        // All Schematic Layer IDs
        enum LayerID {

            // General
            OriginCrosses       = 1,

            // Symbols
            SymbolOutlines      = 10,
            SymbolGrabAreas     = 11,
            SymbolPinCircles    = 12,
            SymbolPinNames      = 13,

            // Symbols in a Schematic
            ComponentNames      = 20,
            ComponentValues     = 21,
            NetLabels           = 22,

            // Circuit Stuff in a Schematic
            Nets                = 30,
            Busses              = 31,

            // Begin of User defined Layers
            UserDefinedBaseId   = 100
        };

        // Constructors / Destructor
        explicit SchematicLayer(uint id);
        ~SchematicLayer();

        // Getters
        uint getId() const {return mId;}
        const QString& getName() const {return mName;}
        const QColor& getColor(bool highlighted = false) const;

        // Static Methods
        static QList<LayerID> getAllLayerIDs() noexcept;


    private:

        // make some methods inaccessible...
        SchematicLayer();
        SchematicLayer(const SchematicLayer& other);
        SchematicLayer& operator=(const SchematicLayer& rhs);

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
    @page doc_schematic_layers Schematic Layers Documentation
    @tableofcontents

    @section doc_schematic_layer_class Class SchematicLayer
        The class #SchematicLayer is used to representate symbol/schematic layers. for
        each layer you need to create an object of that type.

    @section doc_schematic_layer_attributes Layer Attributes
        Each schematic layer has the following attibutes:
            - ID:   An unsigned integer which identifies the layer (must be unique)
            - Name: The name of the layer (translated into the user's language)
            - Color:    The color which is used to draw elements of that layer
            - Color (highlighted): The color for highlighted (selected) elements

    @section doc_schematic_layer_ids Layer IDs
        The following layers exist:
            - 1:    Origin Crosses
            - 10:   Symbol Outlines
            - 11:   Symbol Grab Areas
            - 12:   Symbol Pin Circles
            - 13:   Symbol Pin Names (Text)
            - 20:   Component Names (Text)
            - 21:   Component Values (Text)
            - 30:   Nets (in Schematics, see project#SchematicNetPoint and project#SchematicNetLine)
            - 31:   Busses (in Schematics)

        @todo Add more layers:
            - Various Texts
            - Group Frames / Group Names
            - Custom Layers (user defined)

    @todo Add more details. See https://github.com/ubruhin/EDA4U/issues/4

*/

#endif // SCHEMATICLAYER_H
