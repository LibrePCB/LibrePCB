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

#ifndef LIBREPCB_PROJECT_BOARD_H
#define LIBREPCB_PROJECT_BOARD_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include <QtWidgets>
#include <librepcbcommon/if_attributeprovider.h>
#include <librepcbcommon/fileio/if_xmlserializableobject.h>
#include <librepcbcommon/units/all_length_units.h>
#include <librepcbcommon/fileio/filepath.h>
#include <librepcbcommon/exceptions.h>
#include <librepcbcommon/uuid.h>
#include "../erc/if_ercmsgprovider.h"

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {

class GridProperties;
class GraphicsView;
class GraphicsScene;
class SmartXmlFile;

namespace project {

class Project;
class DeviceInstance;
class BI_Base;
class BI_Polygon;
class BoardLayerStack;

/*****************************************************************************************
 *  Class Board
 ****************************************************************************************/

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
            ZValue_FootprintsBottom,    ///< Z value for #project#BI_Footprint items
            ZValue_FootprintPadsBottom, ///< Z value for #project#BI_FootprintPad items
            ZValue_FootprintPadsTop,    ///< Z value for #project#BI_FootprintPad items
            ZValue_FootprintsTop,       ///< Z value for #project#BI_Footprint items
        };

        // Constructors / Destructor
        explicit Board(Project& project, const FilePath& filepath, bool restore, bool readOnly) throw (Exception) :
            Board(project, filepath, restore, readOnly, false, QString()) {}
        ~Board() noexcept;

        // Getters: General
        Project& getProject() const noexcept {return mProject;}
        const FilePath& getFilePath() const noexcept {return mFilePath;}
        const GridProperties& getGridProperties() const noexcept {return *mGridProperties;}
        BoardLayerStack& getLayerStack() noexcept {return *mLayerStack;}
        bool isEmpty() const noexcept;
        QList<BI_Base*> getSelectedItems(bool footprintPads
                                         /*bool floatingPoints,
                                         bool attachedPoints,
                                         bool floatingPointsFromFloatingLines,
                                         bool attachedPointsFromFloatingLines,
                                         bool floatingPointsFromAttachedLines,
                                         bool attachedPointsFromAttachedLines,
                                         bool attachedPointsFromSymbols,
                                         bool floatingLines,
                                         bool attachedLines,
                                         bool attachedLinesFromFootprints*/) const noexcept;
        QList<BI_Base*> getItemsAtScenePos(const Point& pos) const noexcept;
        //QList<SI_NetPoint*> getNetPointsAtScenePos(const Point& pos) const noexcept;
        //QList<SI_NetLine*> getNetLinesAtScenePos(const Point& pos) const noexcept;
        //QList<SI_SymbolPin*> getPinsAtScenePos(const Point& pos) const noexcept;

        // Setters: General
        void setGridProperties(const GridProperties& grid) noexcept;

        // Getters: Attributes
        const Uuid& getUuid() const noexcept {return mUuid;}
        const QString& getName() const noexcept {return mName;}
        const QIcon& getIcon() const noexcept {return mIcon;}

        // DeviceInstance Methods
        const QMap<Uuid, DeviceInstance*>& getDeviceInstances() const noexcept {return mDeviceInstances;}
        DeviceInstance* getDeviceInstanceByComponentUuid(const Uuid& uuid) const noexcept;
        //DeviceInstance* createDeviceInstance() throw (Exception);
        void addDeviceInstance(DeviceInstance& instance) throw (Exception);
        void removeDeviceInstance(DeviceInstance& instance) throw (Exception);

        // Polygon Methods
        const QList<BI_Polygon*>& getPolygons() const noexcept {return mPolygons;}
        //BI_Polygon* createPolygon() throw (Exception);
        void addPolygon(BI_Polygon& polygon) throw (Exception);
        void removePolygon(BI_Polygon& polygon) throw (Exception);

        // General Methods
        void addToProject() throw (Exception);
        void removeFromProject() throw (Exception);
        bool save(bool toOriginal, QStringList& errors) noexcept;
        void showInView(GraphicsView& view) noexcept;
        void saveViewSceneRect(const QRectF& rect) noexcept {mViewRect = rect;}
        const QRectF& restoreViewSceneRect() const noexcept {return mViewRect;}
        void setSelectionRect(const Point& p1, const Point& p2, bool updateItems) noexcept;
        void clearSelection() const noexcept;

        // Helper Methods
        bool getAttributeValue(const QString& attrNS, const QString& attrKey,
                               bool passToParents, QString& value) const noexcept;

        // Static Methods
        static Board* create(Project& project, const FilePath& filepath,
                             const QString& name) throw (Exception);


    signals:

        /// @copydoc IF_AttributeProvider#attributesChanged()
        void attributesChanged();

        void deviceAdded(DeviceInstance& comp);
        void deviceRemoved(DeviceInstance& comp);


    private:

        // make some methods inaccessible...
        Board() = delete;
        Board(const Board& other) = delete;
        Board& operator=(const Board& rhs) = delete;

        // Private Methods
        explicit Board(Project& project, const FilePath& filepath, bool restore,
                       bool readOnly, bool create, const QString& newName) throw (Exception);
        void updateIcon() noexcept;

        /// @copydoc IF_XmlSerializableObject#checkAttributesValidity()
        bool checkAttributesValidity() const noexcept override;

        void updateErcMessages() noexcept;

        /// @copydoc IF_XmlSerializableObject#serializeToXmlDomElement()
        XmlDomElement* serializeToXmlDomElement() const throw (Exception) override;


        // General
        Project& mProject; ///< A reference to the Project object (from the ctor)
        FilePath mFilePath; ///< the filepath of the schematic *.xml file (from the ctor)
        SmartXmlFile* mXmlFile;
        bool mAddedToProject;

        BoardLayerStack* mLayerStack;
        GraphicsScene* mGraphicsScene;
        QRectF mViewRect;
        GridProperties* mGridProperties;

        // Attributes
        Uuid mUuid;
        QString mName;
        QIcon mIcon;

        // ERC messages
        QHash<Uuid, ErcMsg*> mErcMsgListUnplacedComponentInstances;

        // items
        QMap<Uuid, DeviceInstance*> mDeviceInstances;
        QList<BI_Polygon*> mPolygons;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb

#endif // LIBREPCB_PROJECT_BOARD_H
