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
 *  Forward Declarations
 ****************************************************************************************/

class Workspace;

/*****************************************************************************************
 *  Class SchematicLayer
 ****************************************************************************************/

/**
 * @brief The SchematicLayer class
 */
class SchematicLayer : public QObject
{
        Q_OBJECT

    public:

        // Constructors / Destructor
        explicit SchematicLayer(Workspace& workspace);
        virtual ~SchematicLayer();

        // Getters
        unsigned int getId() const {return mId;}
        const QString& getName() const {return mName;}
        const QPen& getPen(bool selected) const {return selected ? mPenSelected : mPen;}
        const QBrush& getBrush(bool selected) const {return selected ? mBrushSelected : mBrush;}

        // Setters
        //void setColor(const QColor& color);
        //void setSelectedColor(const QColor& color);
        //void setWidth(qreal width);
        //void setBrushStyle(Qt::BrushStyle brushStyle);

    protected:

        // General
        Workspace& mWorkspace;

        // Attributes
        unsigned int mId;
        QString mName;
        QPen mPen;
        QPen mPenSelected;
        QBrush mBrush;
        QBrush mBrushSelected;

    private:

        // make some methods inaccessible...
        SchematicLayer();
        SchematicLayer(const SchematicLayer& other);
        SchematicLayer& operator=(const SchematicLayer& rhs);
};

/*****************************************************************************************
 *  Doxygen Documentation
 ****************************************************************************************/

/**
    @page doc_layers Layers Documentation
    @tableofcontents

    @section layers_overwiew Overview
        There are three different Types of Layers:
            - @ref layers_global "Global Layers (hardcoded)"
            - @ref layers_workspace "Workspace Layers (stored in the workspace directory)"
            - @ref layers_project "Project Layers (stored in the project directory)"

    @section layers_global Global Layers
        Alle notwendigen Layer sind fest in die Software einprogrammiert.
        Die Nummern 1-899 sind für globale Layer reserviert.
        Es wird zwischen Board- und Schaltplan-Layer unterschieden.
        Die Farben und Schraffuren dieser Layer werden im Workspace gespeichert.

        Es gibt folgende Board-Layer (Footprints/Layout):
            - 1-99:   Kupferlagen (1=Top, 99=Bottom)

            - 100:    Pads
            - 101:    Vias

            - 110:    Bohrungen durchkontaktiert ("drills")
            - 111:    Bohrungen nicht durchkontaktiert ("holes")

            - 120:    Platinenumriss
            - 121:    Ausfräsungen (innen)

            - 130:    Stoplack Oben
            - 131:    Stoplack Unten
            - 132:    Finish Oben
            - 133:    Finish Unten
            - 134:    Kleber Oben
            - 135:    Kleber Unten
            - 136:    Schablone Oben
            - 137:    Schablone Unten
            - 138:    Testpunkte Oben
            - 139:    Testpunkte Unten

            - 150:    Bauteil-Referenzen Oben (Koordinaten-Kreuz)
            - 151:    Bauteil-Referenzen Unten (Koordinaten-Kreuz)
            - 152:    Bauteil-Umrisse Oben
            - 153:    Bauteil-Umrisse Unten
            - 154:    Bauteil-Ansicht Oben (Bauteile vervollständigt für einen Ausdruck)
            - 155:    Bauteil-Ansicht Unten (Bauteile vervollständigt für einen Ausdruck)
            - 156:    Bauteil-Namen Oben
            - 157:    Bauteil-Namen Unten
            - 158:    Bauteil-Werte Oben
            - 159:    Bauteil-Werte Unten

            - 170:    Keepout Oben (keine Bauteile)
            - 171:    Keepout Unten (keine Bauteile)
            - 172:    Restrict Oben (kein Kupfer)
            - 173:    Restrict Unten (kein Kupfer)
            - 174:    Restrict Vias (keine Vias)

            - 190:    Unrouted
            - 191:    Bemassungen
            - 192:    Dokumentation??
            - 193:    Referenz??
            - 194:    DXF??

        Es gibt folgende Schaltplan-Layer (Symbole/Schaltplan):
            - 1:      Netze
            - 2:      Busse
            - 3:      Pins

            - 10:     Bauteil-Referenzen (Koordinaten-Kreuz)
            - 11:     Bauteil-Umrisse
            - 12:     Bauteil-Namen
            - 13:     Bauteil-Werte
            - 14:     Bauteil-Package-Namen

            - 20:     (Gruppen-Rahmen)
            - 21:     (Gruppen-Beschriftungen)

            - 30:     Text

        @todo: Layer für Frontplatten-Ausschnitte usw.

    @section layers_workspace Workspace Layers
        Für Workspace-Layer sind die Nummern 900-999 reserviert.
        Sie werden im Workspace-Verzeichnis gespeichert.

    @section layers_project Project Layers
        Beim Anlegen eines neuen Projektes werden alle Workspace-Layer ins Projekt kopiert.
        Von da an werden für das geöffnete Projekt die Workspace-Layer nicht mehr berücksichtigt,
        er wird nur noch mit den Layern des Projektes gearbeitet.
        Verändert man die Workspace-Layer, hat dies auf das Projekt keinen Einfluss mehr.
        Die Projekt-Layer haben damit automatisch auch den Nummernbereich 900-999.

*/

#endif // SCHEMATICLAYER_H
