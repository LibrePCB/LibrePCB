/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
 * https://librepcb.org/
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

#ifndef LIBREPCB_CORE_BOARD_H
#define LIBREPCB_CORE_BOARD_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../attribute/attributeprovider.h"
#include "../../fileio/filepath.h"
#include "../../fileio/transactionaldirectory.h"
#include "../../types/elementname.h"
#include "../../types/length.h"
#include "../../types/uuid.h"
#include "../erc/if_ercmsgprovider.h"

#include <QtCore>
#include <QtWidgets>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class BI_AirWire;
class BI_Base;
class BI_Device;
class BI_FootprintPad;
class BI_Hole;
class BI_NetLine;
class BI_NetPoint;
class BI_NetSegment;
class BI_Plane;
class BI_Polygon;
class BI_StrokeText;
class BI_Via;
class BoardDesignRules;
class BoardFabricationOutputSettings;
class BoardLayerStack;
class BoardSelectionQuery;
class GraphicsLayer;
class GraphicsScene;
class GridProperties;
class NetSignal;
class Project;

/*******************************************************************************
 *  Class Board
 ******************************************************************************/

/**
 * @brief The Board class represents a PCB of a project and is always part of a
 * circuit
 */
class Board final : public QObject,
                    public AttributeProvider,
                    public IF_ErcMsgProvider {
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
    ZValue_TextsBottom,  ///< Z value for librepcb::BI_StrokeText items
    ZValue_DevicesBottom,  ///< Z value for librepcb::BI_Device items
    ZValue_FootprintPadsBottom,  ///< Z value for
                                 ///< librepcb::BI_FootprintPad items
    ZValue_CopperBottom,
    ZValue_CopperTop,
    ZValue_FootprintPadsTop,  ///< Z value for
                              ///< librepcb::BI_FootprintPad items
    ZValue_DevicesTop,  ///< Z value for librepcb::BI_Device items
    ZValue_TextsTop,  ///< Z value for librepcb::BI_StrokeText items
    ZValue_Vias,  ///< Z value for librepcb::BI_Via items
    ZValue_Texts,  ///< Z value for librepcb::BI_StrokeText items
    ZValue_AirWires,  ///< Z value for librepcb::BI_AirWire items
  };

  // Constructors / Destructor
  Board() = delete;
  Board(const Board& other) = delete;
  Board(Project& project, std::unique_ptr<TransactionalDirectory> directory,
        const QString& directoryName, const Uuid& uuid,
        const ElementName& name);
  ~Board() noexcept;

  // Getters: General
  Project& getProject() const noexcept { return mProject; }
  const QString& getDirectoryName() const noexcept { return mDirectoryName; }
  TransactionalDirectory& getDirectory() noexcept { return *mDirectory; }
  const GridProperties& getGridProperties() const noexcept {
    return *mGridProperties;
  }
  GraphicsScene& getGraphicsScene() const noexcept { return *mGraphicsScene; }
  BoardLayerStack& getLayerStack() noexcept { return *mLayerStack; }
  const BoardLayerStack& getLayerStack() const noexcept { return *mLayerStack; }
  BoardDesignRules& getDesignRules() noexcept { return *mDesignRules; }
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
  bool isEmpty() const noexcept;
  QList<BI_NetPoint*> getNetPointsAtScenePos(
      const Point& pos, const GraphicsLayer* layer = nullptr,
      const QSet<const NetSignal*>& netsignals = {}) const noexcept;
  QList<BI_NetLine*> getNetLinesAtScenePos(
      const Point& pos, const GraphicsLayer* layer = nullptr,
      const QSet<const NetSignal*>& netsignals = {}) const noexcept;
  QList<BI_Base*> getAllItems() const noexcept;

  // Setters: General
  void setGridProperties(const GridProperties& grid) noexcept;

  // Getters: Attributes
  const Uuid& getUuid() const noexcept { return mUuid; }
  const ElementName& getName() const noexcept { return mName; }
  const QIcon& getIcon() const noexcept { return mIcon; }
  const QString& getDefaultFontName() const noexcept {
    return mDefaultFontFileName;
  }

  // Setters
  void setName(const ElementName& name) noexcept { mName = name; }
  void setDefaultFontName(const QString& name) noexcept {
    mDefaultFontFileName = name;
  }

  // DeviceInstance Methods
  const QMap<Uuid, BI_Device*>& getDeviceInstances() const noexcept {
    return mDeviceInstances;
  }
  BI_Device* getDeviceInstanceByComponentUuid(const Uuid& uuid) const noexcept;
  void addDeviceInstance(BI_Device& instance);
  void removeDeviceInstance(BI_Device& instance);

  // NetSegment Methods
  const QMap<Uuid, BI_NetSegment*>& getNetSegments() const noexcept {
    return mNetSegments;
  }
  void addNetSegment(BI_NetSegment& netsegment);
  void removeNetSegment(BI_NetSegment& netsegment);

  // Plane Methods
  const QMap<Uuid, BI_Plane*>& getPlanes() const noexcept { return mPlanes; }
  void addPlane(BI_Plane& plane);
  void removePlane(BI_Plane& plane);
  void rebuildAllPlanes() noexcept;

  // Polygon Methods
  const QMap<Uuid, BI_Polygon*>& getPolygons() const noexcept {
    return mPolygons;
  }
  void addPolygon(BI_Polygon& polygon);
  void removePolygon(BI_Polygon& polygon);

  // StrokeText Methods
  const QMap<Uuid, BI_StrokeText*>& getStrokeTexts() const noexcept {
    return mStrokeTexts;
  }
  void addStrokeText(BI_StrokeText& text);
  void removeStrokeText(BI_StrokeText& text);

  // Hole Methods
  const QMap<Uuid, BI_Hole*>& getHoles() const noexcept { return mHoles; }
  void addHole(BI_Hole& hole);
  void removeHole(BI_Hole& hole);

  // AirWire Methods
  QList<BI_AirWire*> getAirWires() const noexcept { return mAirWires.values(); }
  void scheduleAirWiresRebuild(NetSignal* netsignal) noexcept {
    mScheduledNetSignalsForAirWireRebuild.insert(netsignal);
  }
  void triggerAirWiresRebuild() noexcept;
  void forceAirWiresRebuild() noexcept;

  // General Methods
  void addDefaultContent();
  void copyFrom(const Board& other);
  void addToProject();
  void removeFromProject();
  void save();
  void saveViewSceneRect(const QRectF& rect) noexcept { mViewRect = rect; }
  const QRectF& restoreViewSceneRect() const noexcept { return mViewRect; }
  void selectAll() noexcept;
  void setSelectionRect(const Point& p1, const Point& p2,
                        bool updateItems) noexcept;
  void clearSelection() const noexcept;
  std::unique_ptr<BoardSelectionQuery> createSelectionQuery() const noexcept;

  // Inherited from AttributeProvider
  /// @copydoc ::librepcb::AttributeProvider::getBuiltInAttributeValue()
  QString getBuiltInAttributeValue(const QString& key) const noexcept override;
  /// @copydoc ::librepcb::AttributeProvider::getAttributeProviderParents()
  QVector<const AttributeProvider*> getAttributeProviderParents() const
      noexcept override;

  // Operator Overloadings
  Board& operator=(const Board& rhs) = delete;
  bool operator==(const Board& rhs) noexcept { return (this == &rhs); }
  bool operator!=(const Board& rhs) noexcept { return (this != &rhs); }

signals:

  /// @copydoc AttributeProvider::attributesChanged()
  void attributesChanged() override;

  void deviceAdded(BI_Device& comp);
  void deviceRemoved(BI_Device& comp);

private:
  void updateIcon() noexcept;
  void updateErcMessages() noexcept;

  // General
  Project& mProject;  ///< A reference to the Project object (from the ctor)
  const QString mDirectoryName;
  std::unique_ptr<TransactionalDirectory> mDirectory;
  bool mIsAddedToProject;

  QScopedPointer<GraphicsScene> mGraphicsScene;
  QScopedPointer<BoardLayerStack> mLayerStack;
  QScopedPointer<GridProperties> mGridProperties;
  QScopedPointer<BoardDesignRules> mDesignRules;
  QScopedPointer<BoardFabricationOutputSettings> mFabricationOutputSettings;
  QRectF mViewRect;
  QSet<NetSignal*> mScheduledNetSignalsForAirWireRebuild;

  // Attributes
  Uuid mUuid;
  ElementName mName;
  QIcon mIcon;
  QString mDefaultFontFileName;

  // items
  QMap<Uuid, BI_Device*> mDeviceInstances;
  QMap<Uuid, BI_NetSegment*> mNetSegments;
  QMap<Uuid, BI_Plane*> mPlanes;
  QMap<Uuid, BI_Polygon*> mPolygons;
  QMap<Uuid, BI_StrokeText*> mStrokeTexts;
  QMap<Uuid, BI_Hole*> mHoles;
  QMultiHash<NetSignal*, BI_AirWire*> mAirWires;

  // ERC messages
  QHash<Uuid, ErcMsg*> mErcMsgListUnplacedComponentInstances;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
