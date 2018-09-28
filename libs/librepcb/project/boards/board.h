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

#ifndef LIBREPCB_PROJECT_BOARD_H
#define LIBREPCB_PROJECT_BOARD_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../erc/if_ercmsgprovider.h"

#include <librepcb/common/attributes/attributeprovider.h>
#include <librepcb/common/elementname.h>
#include <librepcb/common/exceptions.h>
#include <librepcb/common/fileio/filepath.h>
#include <librepcb/common/fileio/serializableobject.h>
#include <librepcb/common/units/all_length_units.h>
#include <librepcb/common/uuid.h>

#include <QtCore>
#include <QtWidgets>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class GridProperties;
class GraphicsView;
class GraphicsScene;
class SmartSExprFile;
class GraphicsLayer;
class BoardDesignRules;

namespace project {

class NetSignal;
class Project;
class BI_Device;
class BI_Base;
class BI_FootprintPad;
class BI_Via;
class BI_NetSegment;
class BI_NetPoint;
class BI_NetLine;
class BI_Polygon;
class BI_StrokeText;
class BI_Hole;
class BI_Plane;
class BI_AirWire;
class BoardLayerStack;
class BoardFabricationOutputSettings;
class BoardUserSettings;
class BoardSelectionQuery;

/*******************************************************************************
 *  Class Board
 ******************************************************************************/

/**
 * @brief The Board class represents a PCB of a project and is always part of a
 * circuit
 */
class Board final : public QObject,
                    public AttributeProvider,
                    public IF_ErcMsgProvider,
                    public SerializableObject {
  Q_OBJECT
  DECLARE_ERC_MSG_CLASS_NAME(Board)

public:
  // Types

  /**
   * @brief Z Values of all items in a board scene (to define the stacking
   * order)
   *
   * These values are used for QGraphicsItem::setZValue() to define the stacking
   * order of all items in a board QGraphicsScene. We use integer values, even
   * if the z-value of QGraphicsItem is a qreal attribute...
   *
   * Low number = background, high number = foreground
   */
  enum ItemZValue {
    ZValue_Default = 0,  ///< this is the default value (behind all other items)
    ZValue_TextsBottom,  ///< Z value for librepcb::project::BI_StrokeText items
    ZValue_FootprintsBottom,  ///< Z value for librepcb::project::BI_Footprint
                              ///< items
    ZValue_FootprintPadsBottom,  ///< Z value for
                                 ///< librepcb::project::BI_FootprintPad items
    ZValue_CopperBottom,
    ZValue_CopperTop,
    ZValue_FootprintPadsTop,  ///< Z value for
                              ///< librepcb::project::BI_FootprintPad items
    ZValue_FootprintsTop,     ///< Z value for librepcb::project::BI_Footprint
                              ///< items
    ZValue_TextsTop,  ///< Z value for librepcb::project::BI_StrokeText items
    ZValue_Vias,      ///< Z value for librepcb::project::BI_Via items
    ZValue_Texts,     ///< Z value for librepcb::project::BI_StrokeText items
    ZValue_AirWires,  ///< Z value for librepcb::project::BI_AirWire items
  };

  // Constructors / Destructor
  Board()                   = delete;
  Board(const Board& other) = delete;
  Board(const Board& other, const FilePath& filepath, const ElementName& name);
  Board(Project& project, const FilePath& filepath, bool restore, bool readOnly)
    : Board(project, filepath, restore, readOnly, false, QString()) {}
  ~Board() noexcept;

  // Getters: General
  Project&              getProject() const noexcept { return mProject; }
  const FilePath&       getFilePath() const noexcept { return mFilePath; }
  const GridProperties& getGridProperties() const noexcept {
    return *mGridProperties;
  }
  GraphicsScene&   getGraphicsScene() const noexcept { return *mGraphicsScene; }
  BoardLayerStack& getLayerStack() noexcept { return *mLayerStack; }
  const BoardLayerStack& getLayerStack() const noexcept { return *mLayerStack; }
  BoardDesignRules&      getDesignRules() noexcept { return *mDesignRules; }
  const BoardDesignRules& getDesignRules() const noexcept {
    return *mDesignRules;
  }
  BoardFabricationOutputSettings& getFabricationOutputSettings() noexcept {
    return *mFabricationOutputSettings;
  }
  const BoardFabricationOutputSettings& getFabricationOutputSettings() const
      noexcept {
    return *mFabricationOutputSettings;
  }
  bool                isEmpty() const noexcept;
  QList<BI_Base*>     getItemsAtScenePos(const Point& pos) const noexcept;
  QList<BI_Via*>      getViasAtScenePos(const Point&     pos,
                                        const NetSignal* netsignal) const noexcept;
  QList<BI_NetPoint*> getNetPointsAtScenePos(const Point&         pos,
                                             const GraphicsLayer* layer,
                                             const NetSignal* netsignal) const
      noexcept;
  QList<BI_NetLine*> getNetLinesAtScenePos(const Point&         pos,
                                           const GraphicsLayer* layer,
                                           const NetSignal*     netsignal) const
      noexcept;
  QList<BI_FootprintPad*> getPadsAtScenePos(const Point&         pos,
                                            const GraphicsLayer* layer,
                                            const NetSignal* netsignal) const
      noexcept;
  QList<BI_Base*> getAllItems() const noexcept;

  // Setters: General
  void setGridProperties(const GridProperties& grid) noexcept;

  // Getters: Attributes
  const Uuid&        getUuid() const noexcept { return mUuid; }
  const ElementName& getName() const noexcept { return mName; }
  const QIcon&       getIcon() const noexcept { return mIcon; }
  const QString&     getDefaultFontName() const noexcept {
    return mDefaultFontFileName;
  }

  // DeviceInstance Methods
  const QMap<Uuid, BI_Device*>& getDeviceInstances() const noexcept {
    return mDeviceInstances;
  }
  BI_Device* getDeviceInstanceByComponentUuid(const Uuid& uuid) const noexcept;
  void       addDeviceInstance(BI_Device& instance);
  void       removeDeviceInstance(BI_Device& instance);

  // NetSegment Methods
  const QList<BI_NetSegment*>& getNetSegments() const noexcept {
    return mNetSegments;
  }
  BI_NetSegment* getNetSegmentByUuid(const Uuid& uuid) const noexcept;
  void           addNetSegment(BI_NetSegment& netsegment);
  void           removeNetSegment(BI_NetSegment& netsegment);

  // Plane Methods
  const QList<BI_Plane*>& getPlanes() const noexcept { return mPlanes; }
  void                    addPlane(BI_Plane& plane);
  void                    removePlane(BI_Plane& plane);
  void                    rebuildAllPlanes() noexcept;

  // Polygon Methods
  const QList<BI_Polygon*>& getPolygons() const noexcept { return mPolygons; }
  void                      addPolygon(BI_Polygon& polygon);
  void                      removePolygon(BI_Polygon& polygon);

  // StrokeText Methods
  const QList<BI_StrokeText*>& getStrokeTexts() const noexcept {
    return mStrokeTexts;
  }
  void addStrokeText(BI_StrokeText& text);
  void removeStrokeText(BI_StrokeText& text);

  // Hole Methods
  const QList<BI_Hole*>& getHoles() const noexcept { return mHoles; }
  void                   addHole(BI_Hole& hole);
  void                   removeHole(BI_Hole& hole);

  // AirWire Methods
  void scheduleAirWiresRebuild(NetSignal* netsignal) noexcept {
    mScheduledNetSignalsForAirWireRebuild.insert(netsignal);
  }
  void triggerAirWiresRebuild() noexcept;
  void forceAirWiresRebuild() noexcept;

  // General Methods
  void addToProject();
  void removeFromProject();
  bool save(bool toOriginal, QStringList& errors) noexcept;
  void showInView(GraphicsView& view) noexcept;
  void saveViewSceneRect(const QRectF& rect) noexcept { mViewRect = rect; }
  const QRectF& restoreViewSceneRect() const noexcept { return mViewRect; }
  void          setSelectionRect(const Point& p1, const Point& p2,
                                 bool updateItems) noexcept;
  void          clearSelection() const noexcept;
  std::unique_ptr<BoardSelectionQuery> createSelectionQuery() const noexcept;

  // Inherited from AttributeProvider
  /// @copydoc librepcb::AttributeProvider::getBuiltInAttributeValue()
  QString getBuiltInAttributeValue(const QString& key) const noexcept override;
  /// @copydoc librepcb::AttributeProvider::getAttributeProviderParents()
  QVector<const AttributeProvider*> getAttributeProviderParents() const
      noexcept override;

  // Operator Overloadings
  Board& operator=(const Board& rhs) = delete;
  bool   operator==(const Board& rhs) noexcept { return (this == &rhs); }
  bool   operator!=(const Board& rhs) noexcept { return (this != &rhs); }

  // Static Methods
  static Board* create(Project& project, const FilePath& filepath,
                       const ElementName& name);

signals:

  /// @copydoc AttributeProvider::attributesChanged()
  void attributesChanged() override;

  void deviceAdded(BI_Device& comp);
  void deviceRemoved(BI_Device& comp);

private:
  Board(Project& project, const FilePath& filepath, bool restore, bool readOnly,
        bool create, const QString& newName);
  void updateIcon() noexcept;
  void updateErcMessages() noexcept;

  /// @copydoc librepcb::SerializableObject::serialize()
  void serialize(SExpression& root) const override;

  // General
  Project& mProject;   ///< A reference to the Project object (from the ctor)
  FilePath mFilePath;  ///< the filepath of the board.lp file (from the ctor)
  QScopedPointer<SmartSExprFile> mFile;
  bool                           mIsAddedToProject;

  QScopedPointer<GraphicsScene>                  mGraphicsScene;
  QScopedPointer<BoardLayerStack>                mLayerStack;
  QScopedPointer<GridProperties>                 mGridProperties;
  QScopedPointer<BoardDesignRules>               mDesignRules;
  QScopedPointer<BoardFabricationOutputSettings> mFabricationOutputSettings;
  QScopedPointer<BoardUserSettings>              mUserSettings;
  QRectF                                         mViewRect;
  QSet<NetSignal*> mScheduledNetSignalsForAirWireRebuild;

  // Attributes
  Uuid        mUuid;
  ElementName mName;
  QIcon       mIcon;
  QString     mDefaultFontFileName;

  // items
  QMap<Uuid, BI_Device*>              mDeviceInstances;
  QList<BI_NetSegment*>               mNetSegments;
  QList<BI_Plane*>                    mPlanes;
  QList<BI_Polygon*>                  mPolygons;
  QList<BI_StrokeText*>               mStrokeTexts;
  QList<BI_Hole*>                     mHoles;
  QMultiHash<NetSignal*, BI_AirWire*> mAirWires;

  // ERC messages
  QHash<Uuid, ErcMsg*> mErcMsgListUnplacedComponentInstances;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace project
}  // namespace librepcb

#endif  // LIBREPCB_PROJECT_BOARD_H
