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

#ifndef PROJECT_BOARD_H
#define PROJECT_BOARD_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include <QtWidgets>
#include <eda4ucommon/if_attributeprovider.h>
#include <eda4ucommon/fileio/if_xmlserializableobject.h>
#include <eda4ucommon/units/all_length_units.h>
#include <eda4ucommon/fileio/filepath.h>
#include <eda4ucommon/exceptions.h>
#include "../erc/if_ercmsgprovider.h"

/*****************************************************************************************
 *  Forward Declarations
 ****************************************************************************************/

class GridProperties;
class GraphicsView;
class GraphicsScene;
class SmartXmlFile;

namespace project {
class Project;
class ComponentInstance;
}

/*****************************************************************************************
 *  Class Board
 ****************************************************************************************/

namespace project {

/**
 * @brief The Board class represents a PCB of a project and is always part of a circuit
 */
class Board final : public QObject, public IF_AttributeProvider,
                    public IF_ErcMsgProvider, public IF_XmlSerializableObject
{
        Q_OBJECT
        DECLARE_ERC_MSG_CLASS_NAME(Board)

    public:

        // Types

        /**
         * @brief Z Values of all items in a board scene (to define the stacking order)
         *
         * These values are used for QGraphicsItem::setZValue() to define the stacking
         * order of all items in a board QGraphicsScene. We use integer values, even
         * if the z-value of QGraphicsItem is a qreal attribute...
         *
         * Low number = background, high number = foreground
         */
        enum ItemZValue {
            ZValue_Default = 0,         ///< this is the default value (behind all other items)
            ZValue_FootprintsBottom,    ///< Z value for project#BI_Footprint items
            ZValue_FootprintsTop,       ///< Z value for project#BI_Footprint items
        };

        // Constructors / Destructor
        explicit Board(Project& project, const FilePath& filepath, bool restore, bool readOnly) throw (Exception) :
            Board(project, filepath, restore, readOnly, false, QString()) {}
        ~Board() noexcept;

        // Getters: General
        Project& getProject() const noexcept {return mProject;}
        const FilePath& getFilePath() const noexcept {return mFilePath;}
        const GridProperties& getGridProperties() const noexcept {return *mGridProperties;}
        bool isEmpty() const noexcept;

        // Setters: General
        void setGridProperties(const GridProperties& grid) noexcept;

        // Getters: Attributes
        const QUuid& getUuid() const noexcept {return mUuid;}
        const QString& getName() const noexcept {return mName;}
        const QIcon& getIcon() const noexcept {return mIcon;}

        // ComponentInstance Methods
        const QHash<QUuid, ComponentInstance*>& getComponentInstances() const noexcept {return mComponentInstances;}
        ComponentInstance* getCompInstanceByGenCompUuid(const QUuid& uuid) const noexcept;
        ComponentInstance* createComponentInstance() throw (Exception);
        void addComponentInstance(ComponentInstance& componentInstance) throw (Exception);
        void removeComponentInstance(ComponentInstance& componentInstance) throw (Exception);

        // General Methods
        void addToProject() throw (Exception);
        void removeFromProject() throw (Exception);
        bool save(bool toOriginal, QStringList& errors) noexcept;
        void showInView(GraphicsView& view) noexcept;
        void saveViewSceneRect(const QRectF& rect) noexcept {mViewRect = rect;}
        const QRectF& restoreViewSceneRect() const noexcept {return mViewRect;}

        // Helper Methods
        bool getAttributeValue(const QString& attrNS, const QString& attrKey,
                               bool passToParents, QString& value) const noexcept;

        // Static Methods
        static Board* create(Project& project, const FilePath& filepath,
                             const QString& name) throw (Exception);


    signals:

        /// @copydoc IF_AttributeProvider#attributesChanged()
        void attributesChanged();


    private:

        // make some methods inaccessible...
        Board() = delete;
        Board(const Board& other) = delete;
        Board& operator=(const Board& rhs) = delete;

        // Private Methods
        explicit Board(Project& project, const FilePath& filepath, bool restore,
                       bool readOnly, bool create, const QString& newName) throw (Exception);
        void updateIcon() noexcept;

        bool checkAttributesValidity() const noexcept;
        void updateErcMessages() noexcept;

        /**
         * @copydoc IF_XmlSerializableObject#serializeToXmlDomElement()
         */
        XmlDomElement* serializeToXmlDomElement() const throw (Exception);


        // General
        Project& mProject; ///< A reference to the Project object (from the ctor)
        FilePath mFilePath; ///< the filepath of the schematic *.xml file (from the ctor)
        SmartXmlFile* mXmlFile;
        bool mAddedToProject;

        GraphicsScene* mGraphicsScene;
        QRectF mViewRect;
        GridProperties* mGridProperties;

        // Attributes
        QUuid mUuid;
        QString mName;
        QIcon mIcon;

        // ERC messages
        QHash<QUuid, ErcMsg*> mErcMsgListUnplacedGenCompInstances;

        // items
        QHash<QUuid, ComponentInstance*> mComponentInstances;
};

} // namespace project

#endif // PROJECT_BOARD_H
