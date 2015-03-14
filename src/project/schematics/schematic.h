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

#ifndef PROJECT_SCHEMATIC_H
#define PROJECT_SCHEMATIC_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include "../../common/if_attributeprovider.h"
#include "../../common/file_io/if_xmlserializableobject.h"
#include "../../common/units/all_length_units.h"
#include "../../common/file_io/filepath.h"
#include "../../common/exceptions.h"
#include "../../common/cadscene.h"

/*****************************************************************************************
 *  Forward Declarations
 ****************************************************************************************/

class SmartXmlFile;

namespace project {
class Project;
class NetSignal;
class GenCompInstance;
class SymbolInstance;
class SymbolPinInstance;
class SchematicNetPoint;
class SchematicNetLine;
class SchematicNetLabel;
}

namespace library {
class SymbolPin;
}

/*****************************************************************************************
 *  Class Schematic
 ****************************************************************************************/

namespace project {

/**
 * @brief The Schematic class represents one schematic page of a project and is always
 * part of a circuit
 *
 * This class inherits from QGraphicsScene (through CADScene). This way, a schematic page
 * can be shown directly in a QGraphicsView (resp. CADView).
 */
class Schematic final : public CADScene, public IF_AttributeProvider,
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
            ZValue_Default = 0, ///< this is the default value (behind all other items)
            ZValue_Symbols,     ///< Z value for project#SymbolInstance items
            ZValue_NetLabels,   ///< Z value for project#SchematicNetLabel items
            ZValue_NetLines,    ///< Z value for project#SchematicNetLine items
            ZValue_NetPoints    ///< Z value for project#SchematicNetPoint items
        };


        // Constructors / Destructor
        explicit Schematic(Project& project, const FilePath& filepath, bool restore, bool readOnly) throw (Exception) :
            Schematic(project, filepath, restore, readOnly, false, QString()) {}
        ~Schematic() noexcept;

        // Getters: Attributes
        const FilePath& getFilePath() const noexcept {return mFilePath;}
        Project& getProject() const noexcept {return mProject;}
        const QUuid& getUuid() const noexcept {return mUuid;}
        const QString& getName() const noexcept {return mName;}
        const QIcon& getIcon() const noexcept {return mIcon;}

        // Getters: General
        bool isEmpty() const noexcept;
        uint getNetPointsAtScenePos(QList<SchematicNetPoint*>& list, const Point& pos) const noexcept;
        uint getNetLinesAtScenePos(QList<SchematicNetLine*>& list, const Point& pos) const noexcept;
        uint getPinsAtScenePos(QList<SymbolPinInstance*>& list, const Point& pos) const noexcept;

        // SymbolInstance Methods
        SymbolInstance* getSymbolByUuid(const QUuid& uuid) const noexcept;
        SymbolInstance* createSymbol(GenCompInstance& genCompInstance,
                                     const QUuid& symbolItem, const Point& position = Point(),
                                     const Angle& angle = Angle()) throw (Exception);
        void addSymbol(SymbolInstance& symbol) throw (Exception);
        void removeSymbol(SymbolInstance& symbol) throw (Exception);

        // SchematicNetPoint Methods
        SchematicNetPoint* getNetPointByUuid(const QUuid& uuid) const noexcept;
        SchematicNetPoint* createNetPoint(NetSignal& netsignal, const Point& position) throw (Exception);
        SchematicNetPoint* createNetPoint(SymbolInstance& symbol, const QUuid& pin) throw (Exception);
        void addNetPoint(SchematicNetPoint& netpoint) throw (Exception);
        void removeNetPoint(SchematicNetPoint& netpoint) throw (Exception);

        // SchematicNetLine Methods
        SchematicNetLine* getNetLineByUuid(const QUuid& uuid) const noexcept;
        SchematicNetLine* createNetLine(SchematicNetPoint& startPoint,
                                        SchematicNetPoint& endPoint,
                                        const Length& width) throw (Exception);
        void addNetLine(SchematicNetLine& netline) throw (Exception);
        void removeNetLine(SchematicNetLine& netline) throw (Exception);

        // SchematicNetLabel Methods
        SchematicNetLabel* getNetLabelByUuid(const QUuid& uuid) const noexcept;
        SchematicNetLabel* createNetLabel(NetSignal& netsignal, const Point& position) throw (Exception);
        void addNetLabel(SchematicNetLabel& netlabel) throw (Exception);
        void removeNetLabel(SchematicNetLabel& netlabel) throw (Exception);

        // General Methods
        void addToProject() throw (Exception);
        void removeFromProject() throw (Exception);
        bool save(bool toOriginal, QStringList& errors) noexcept;
        void saveViewSceneRect(const QRectF& rect) noexcept {mViewRect = rect;}
        const QRectF& restoreViewSceneRect() const noexcept {return mViewRect;}

        // Helper Methods
        bool getAttributeValue(const QString& attrNS, const QString& attrKey,
                               bool passToParents, QString& value) const noexcept;

        // Static Methods
        static Schematic* create(Project& project, const FilePath& filepath,
                                 const QString& name) throw (Exception);

    private:

        // make some methods inaccessible...
        Schematic();
        Schematic(const Schematic& other);
        Schematic& operator=(const Schematic& rhs);

        // Private Methods
        explicit Schematic(Project& project, const FilePath& filepath, bool restore,
                           bool readOnly, bool create, const QString& newName) throw (Exception);
        void updateIcon() noexcept;

        bool checkAttributesValidity() const noexcept;

        /**
         * @copydoc IF_XmlSerializableObject#serializeToXmlDomElement()
         */
        XmlDomElement* serializeToXmlDomElement() const throw (Exception);


        // General
        Project& mProject; ///< A reference to the Project object (from the ctor)
        FilePath mFilePath; ///< the filepath of the schematic *.xml file (from the ctor)
        SmartXmlFile* mXmlFile;
        bool mAddedToProject;

        QRectF mViewRect;

        // Attributes
        QUuid mUuid;
        QString mName;
        QIcon mIcon;

        QHash<QUuid, SymbolInstance*> mSymbols;
        QHash<QUuid, SchematicNetPoint*> mNetPoints;
        QHash<QUuid, SchematicNetLine*> mNetLines;
        QHash<QUuid, SchematicNetLabel*> mNetLabels;
};

} // namespace project

#endif // PROJECT_SCHEMATIC_H
