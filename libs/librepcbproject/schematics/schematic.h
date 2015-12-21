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

#ifndef PROJECT_SCHEMATIC_H
#define PROJECT_SCHEMATIC_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include <QtWidgets>
#include <librepcbcommon/uuid.h>
#include <librepcbcommon/if_attributeprovider.h>
#include <librepcbcommon/fileio/if_xmlserializableobject.h>
#include <librepcbcommon/units/all_length_units.h>
#include <librepcbcommon/fileio/filepath.h>
#include <librepcbcommon/exceptions.h>

/*****************************************************************************************
 *  Forward Declarations
 ****************************************************************************************/

class GridProperties;
class GraphicsView;
class GraphicsScene;
class SmartXmlFile;

namespace project {
class Project;
class NetSignal;
class ComponentInstance;
class SI_Base;
class SI_Symbol;
class SI_SymbolPin;
class SI_NetPoint;
class SI_NetLine;
class SI_NetLabel;
}

/*****************************************************************************************
 *  Class Schematic
 ****************************************************************************************/

namespace project {

/**
 * @brief The Schematic class represents one schematic page of a project and is always
 * part of a circuit
 *
 * A schematic can contain following items (see project#SI_Base and project#SGI_Base):
 *  - netpoint:         project#SI_NetPoint    + project#SGI_NetPoint
 *  - netline:          project#SI_NetLine     + project#SGI_NetLine
 *  - netlabel:         project#SI_NetLabel    + project#SGI_NetLabel
 *  - symbol:           project#SI_Symbol      + project#SGI_Symbol
 *  - symbol pin:       project#SI_SymbolPin   + project#SGI_SymbolPin
 *  - polygon:          TODO
 *  - ellipse:          TODO
 *  - text:             TODO
 */
class Schematic final : public QObject, public IF_AttributeProvider,
                        public IF_XmlSerializableObject
{
        Q_OBJECT

    public:

        // Types

        /**
         * @brief Z Values of all items in a schematic scene (to define the stacking order)
         *
         * These values are used for QGraphicsItem::setZValue() to define the stacking
         * order of all items in a schematic QGraphicsScene. We use integer values, even
         * if the z-value of QGraphicsItem is a qreal attribute...
         *
         * Low number = background, high number = foreground
         */
        enum ItemZValue {
            ZValue_Default = 0,         ///< this is the default value (behind all other items)
            ZValue_Symbols,             ///< Z value for project#SymbolInstance items
            ZValue_NetLabels,           ///< Z value for project#SchematicNetLabel items
            ZValue_NetLines,            ///< Z value for project#SchematicNetLine items
            ZValue_HiddenNetPoints,     ///< Z value for hidden project#SchematicNetPoint items
            ZValue_VisibleNetPoints,    ///< Z value for visible project#SchematicNetPoint items
        };


        // Constructors / Destructor
        explicit Schematic(Project& project, const FilePath& filepath, bool restore, bool readOnly) throw (Exception) :
            Schematic(project, filepath, restore, readOnly, false, QString()) {}
        ~Schematic() noexcept;

        // Getters: General
        Project& getProject() const noexcept {return mProject;}
        const FilePath& getFilePath() const noexcept {return mFilePath;}
        const GridProperties& getGridProperties() const noexcept {return *mGridProperties;}
        bool isEmpty() const noexcept;
        QList<SI_Base*> getSelectedItems(bool symbolPins,
                                         bool floatingPoints,
                                         bool attachedPoints,
                                         bool floatingPointsFromFloatingLines,
                                         bool attachedPointsFromFloatingLines,
                                         bool floatingPointsFromAttachedLines,
                                         bool attachedPointsFromAttachedLines,
                                         bool attachedPointsFromSymbols,
                                         bool floatingLines,
                                         bool attachedLines,
                                         bool attachedLinesFromSymbols) const noexcept;
        QList<SI_Base*> getItemsAtScenePos(const Point& pos) const noexcept;
        QList<SI_NetPoint*> getNetPointsAtScenePos(const Point& pos) const noexcept;
        QList<SI_NetLine*> getNetLinesAtScenePos(const Point& pos) const noexcept;
        QList<SI_SymbolPin*> getPinsAtScenePos(const Point& pos) const noexcept;

        // Setters: General
        void setGridProperties(const GridProperties& grid) noexcept;

        // Getters: Attributes
        const Uuid& getUuid() const noexcept {return mUuid;}
        const QString& getName() const noexcept {return mName;}
        const QIcon& getIcon() const noexcept {return mIcon;}

        // Symbol Methods
        SI_Symbol* getSymbolByUuid(const Uuid& uuid) const noexcept;
        SI_Symbol* createSymbol(ComponentInstance& genCompInstance, const Uuid& symbolItem,
                                const Point& position = Point(), const Angle& angle = Angle()) throw (Exception);
        void addSymbol(SI_Symbol& symbol) throw (Exception);
        void removeSymbol(SI_Symbol& symbol) throw (Exception);

        // NetPoint Methods
        SI_NetPoint* getNetPointByUuid(const Uuid& uuid) const noexcept;
        SI_NetPoint* createNetPoint(NetSignal& netsignal, const Point& position) throw (Exception);
        SI_NetPoint* createNetPoint(SI_SymbolPin& pin) throw (Exception);
        void addNetPoint(SI_NetPoint& netpoint) throw (Exception);
        void removeNetPoint(SI_NetPoint& netpoint) throw (Exception);

        // NetLine Methods
        SI_NetLine* getNetLineByUuid(const Uuid& uuid) const noexcept;
        SI_NetLine* createNetLine(SI_NetPoint& startPoint, SI_NetPoint& endPoint,
                                  const Length& width) throw (Exception);
        void addNetLine(SI_NetLine& netline) throw (Exception);
        void removeNetLine(SI_NetLine& netline) throw (Exception);

        // NetLabel Methods
        SI_NetLabel* getNetLabelByUuid(const Uuid& uuid) const noexcept;
        SI_NetLabel* createNetLabel(NetSignal& netsignal, const Point& position) throw (Exception);
        void addNetLabel(SI_NetLabel& netlabel) throw (Exception);
        void removeNetLabel(SI_NetLabel& netlabel) throw (Exception);

        // General Methods
        void addToProject() throw (Exception);
        void removeFromProject() throw (Exception);
        bool save(bool toOriginal, QStringList& errors) noexcept;
        void showInView(GraphicsView& view) noexcept;
        void saveViewSceneRect(const QRectF& rect) noexcept {mViewRect = rect;}
        const QRectF& restoreViewSceneRect() const noexcept {return mViewRect;}
        void setSelectionRect(const Point& p1, const Point& p2, bool updateItems) noexcept;
        void clearSelection() const noexcept;
        void renderToQPainter(QPainter& painter) const noexcept;

        // Helper Methods
        bool getAttributeValue(const QString& attrNS, const QString& attrKey,
                               bool passToParents, QString& value) const noexcept;

        // Static Methods
        static Schematic* create(Project& project, const FilePath& filepath,
                                 const QString& name) throw (Exception);


    signals:

        /// @copydoc IF_AttributeProvider#attributesChanged()
        void attributesChanged();


    private:

        // make some methods inaccessible...
        Schematic();
        Schematic(const Schematic& other);
        Schematic& operator=(const Schematic& rhs);

        // Private Methods
        explicit Schematic(Project& project, const FilePath& filepath, bool restore,
                           bool readOnly, bool create, const QString& newName) throw (Exception);
        void updateIcon() noexcept;

        /// @copydoc IF_XmlSerializableObject#checkAttributesValidity()
        bool checkAttributesValidity() const noexcept override;

        /// @copydoc IF_XmlSerializableObject#serializeToXmlDomElement()
        XmlDomElement* serializeToXmlDomElement() const throw (Exception) override;


        // General
        Project& mProject; ///< A reference to the Project object (from the ctor)
        FilePath mFilePath; ///< the filepath of the schematic *.xml file (from the ctor)
        SmartXmlFile* mXmlFile;
        bool mAddedToProject;

        GraphicsScene* mGraphicsScene;
        QRectF mViewRect;
        GridProperties* mGridProperties;

        // Attributes
        Uuid mUuid;
        QString mName;
        QIcon mIcon;

        QList<SI_Symbol*> mSymbols;
        QList<SI_NetPoint*> mNetPoints;
        QList<SI_NetLine*> mNetLines;
        QList<SI_NetLabel*> mNetLabels;
};

} // namespace project

#endif // PROJECT_SCHEMATIC_H
