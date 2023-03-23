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
#include "../../types/lengthunit.h"
#include "../../types/uuid.h"
#include "../../types/version.h"

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
class BoardDesignRuleCheckSettings;
class BoardDesignRules;
class BoardFabricationOutputSettings;
class Layer;
class NetSignal;
class Project;

/*******************************************************************************
 *  Class Board
 ******************************************************************************/

/**
 * @brief The Board class represents a PCB of a project and is always part of a
 * circuit
 */
class Board final : public QObject, public AttributeProvider {
  Q_OBJECT

public:
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
  const BoardDesignRules& getDesignRules() const noexcept {
    return *mDesignRules;
  }
  const BoardDesignRuleCheckSettings& getDrcSettings() const noexcept {
    return *mDrcSettings;
  }
  BoardFabricationOutputSettings& getFabricationOutputSettings() noexcept {
    return *mFabricationOutputSettings;
  }
  const BoardFabricationOutputSettings& getFabricationOutputSettings() const
      noexcept {
    return *mFabricationOutputSettings;
  }
  bool isEmpty() const noexcept;
  QList<BI_Base*> getAllItems() const noexcept;

  // Getters: Attributes
  const Uuid& getUuid() const noexcept { return mUuid; }
  const ElementName& getName() const noexcept { return mName; }
  const QString& getDefaultFontName() const noexcept {
    return mDefaultFontFileName;
  }
  const PositiveLength& getGridInterval() const noexcept {
    return mGridInterval;
  }
  const LengthUnit& getGridUnit() const noexcept { return mGridUnit; }
  int getInnerLayerCount() const noexcept { return mInnerLayerCount; }
  const QSet<const Layer*> getCopperLayers() const noexcept {
    return mCopperLayers;
  }
  const QMap<QString, bool>& getLayersVisibility() const noexcept {
    return mLayersVisibility;
  }

  // Setters
  void setName(const ElementName& name) noexcept { mName = name; }
  void setDefaultFontName(const QString& name) noexcept {
    mDefaultFontFileName = name;
  }
  void setGridInterval(const PositiveLength& interval) noexcept {
    mGridInterval = interval;
  }
  void setGridUnit(const LengthUnit& unit) noexcept { mGridUnit = unit; }
  void setInnerLayerCount(int count) noexcept;
  void setLayersVisibility(const QMap<QString, bool>& visibility) noexcept {
    mLayersVisibility = visibility;
  }
  void setDesignRules(const BoardDesignRules& rules) noexcept;
  void setDrcSettings(const BoardDesignRuleCheckSettings& settings) noexcept;

  // DRC Message Approval Methods
  const QSet<SExpression>& getDrcMessageApprovals() const noexcept {
    return mDrcMessageApprovals;
  }
  void loadDrcMessageApprovals(const Version& version,
                               const QSet<SExpression>& approvals) noexcept;
  bool updateDrcMessageApprovals(QSet<SExpression> approvals,
                                 bool partialRun) noexcept;
  void setDrcMessageApproved(const SExpression& approval,
                             bool approved) noexcept;

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

  void designRulesModified();
  void innerLayerCountChanged();

  void deviceAdded(BI_Device& device);
  void deviceRemoved(BI_Device& device);
  void netSegmentAdded(BI_NetSegment& netSegment);
  void netSegmentRemoved(BI_NetSegment& netSegment);
  void planeAdded(BI_Plane& plane);
  void planeRemoved(BI_Plane& plane);
  void polygonAdded(BI_Polygon& polygon);
  void polygonRemoved(BI_Polygon& polygon);
  void strokeTextAdded(BI_StrokeText& strokeText);
  void strokeTextRemoved(BI_StrokeText& strokeText);
  void holeAdded(BI_Hole& hole);
  void holeRemoved(BI_Hole& hole);
  void airWireAdded(BI_AirWire& airWire);
  void airWireRemoved(BI_AirWire& airWire);

private:
  // General
  Project& mProject;  ///< A reference to the Project object (from the ctor)
  const QString mDirectoryName;
  std::unique_ptr<TransactionalDirectory> mDirectory;
  bool mIsAddedToProject;

  QScopedPointer<BoardDesignRules> mDesignRules;
  QScopedPointer<BoardDesignRuleCheckSettings> mDrcSettings;
  QScopedPointer<BoardFabricationOutputSettings> mFabricationOutputSettings;
  QSet<NetSignal*> mScheduledNetSignalsForAirWireRebuild;

  // Attributes
  Uuid mUuid;
  ElementName mName;
  QString mDefaultFontFileName;
  PositiveLength mGridInterval;
  LengthUnit mGridUnit;
  int mInnerLayerCount;
  QSet<const Layer*> mCopperLayers;
  QMap<QString, bool> mLayersVisibility;

  // DRC
  Version mDrcMessageApprovalsVersion;
  QSet<SExpression> mDrcMessageApprovals;
  QSet<SExpression> mSupportedDrcMessageApprovals;

  // items
  QMap<Uuid, BI_Device*> mDeviceInstances;
  QMap<Uuid, BI_NetSegment*> mNetSegments;
  QMap<Uuid, BI_Plane*> mPlanes;
  QMap<Uuid, BI_Polygon*> mPolygons;
  QMap<Uuid, BI_StrokeText*> mStrokeTexts;
  QMap<Uuid, BI_Hole*> mHoles;
  QMultiHash<NetSignal*, BI_AirWire*> mAirWires;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
