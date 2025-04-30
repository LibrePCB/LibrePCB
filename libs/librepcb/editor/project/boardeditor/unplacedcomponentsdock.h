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

#ifndef LIBREPCB_EDITOR_UNPLACEDCOMPONENTSDOCK_H
#define LIBREPCB_EDITOR_UNPLACEDCOMPONENTSDOCK_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/core/types/point.h>
#include <librepcb/core/types/uuid.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Board;
class ComponentInstance;
class Device;
class Footprint;
class Package;
class Project;

namespace editor {

class FootprintGraphicsItem;
class GraphicsLayerList;
class GraphicsScene;
class ProjectEditor;

namespace Ui {
class UnplacedComponentsDock;
}

/*******************************************************************************
 *  Class SchematicPagesDock
 ******************************************************************************/

/**
 * @brief The UnplacedComponentsDock class
 */
class UnplacedComponentsDock final : public QDockWidget {
  Q_OBJECT

  struct DeviceMetadata {
    /// Device library element UUID
    Uuid deviceUuid;

    /// Device library element name
    QString deviceName;

    /// Package library element UUID
    Uuid packageUuid;

    /// Package library element name
    QString packageName;

    /// Whether this device has been added as a part in the component instance
    /// or not
    bool isListedInComponentInstance;
  };

public:
  // Constructors / Destructor
  UnplacedComponentsDock() = delete;
  explicit UnplacedComponentsDock(ProjectEditor& editor,
                                  QWidget* parent = nullptr) noexcept;
  UnplacedComponentsDock(const UnplacedComponentsDock& other) = delete;
  ~UnplacedComponentsDock() noexcept;

  // Getters
  int getUnplacedComponentsCount() const noexcept;

  // Setters
  void setBoard(Board* board);

  // Operator Overloadings
  UnplacedComponentsDock& operator=(const UnplacedComponentsDock& rhs) = delete;

signals:
  void unplacedComponentsCountChanged(int count);
  void addDeviceTriggered(ComponentInstance& cmp, const Uuid& deviceUuid,
                          Uuid footprintUuid);

private:  // Methods
  void updateComponentsList() noexcept;
  void currentComponentListItemChanged(QListWidgetItem* current,
                                       QListWidgetItem* previous) noexcept;
  void currentDeviceIndexChanged(int index) noexcept;
  void currentFootprintIndexChanged(int index) noexcept;
  void setSelectedComponentInstance(ComponentInstance* cmp) noexcept;
  void setSelectedDeviceAndPackage(const std::optional<Uuid>& deviceUuid,
                                   Package* package,
                                   bool packageOwned) noexcept;
  void setSelectedFootprintUuid(const std::optional<Uuid>& uuid) noexcept;
  void addSelectedDeviceToBoard() noexcept;
  void addSimilarDevicesToBoard() noexcept;
  void addAllDevicesToBoard() noexcept;
  void autoAddDevicesToBoard(
      const std::optional<Uuid>& libCmpUuidFilter) noexcept;

  /**
   * @brief Get all available devices for a specific component instance
   *
   * @param cmp   The desired component instance.
   *
   * @return  Metadata of all available devices, and the list index of the
   *          best match / most relevant device.
   */
  std::pair<QList<DeviceMetadata>, int> getAvailableDevices(
      ComponentInstance& cmp) const noexcept;
  std::optional<Uuid> getSuggestedFootprint(
      const Uuid& libPkgUuid) const noexcept;

private:  // Data
  ProjectEditor& mProjectEditor;
  Project& mProject;
  Board* mBoard;
  QScopedPointer<Ui::UnplacedComponentsDock> mUi;

  // State
  bool mDisableListUpdate;
  Point mNextPosition;
  QHash<Uuid, Uuid> mLastDeviceOfComponent;
  QHash<Uuid, Uuid> mLastFootprintOfPackage;
  QList<DeviceMetadata> mCurrentDevices;

  // Current selection
  ComponentInstance* mSelectedComponent;
  std::optional<Uuid> mSelectedDeviceUuid;
  Package* mSelectedPackage;
  bool mSelectedPackageOwned;
  std::optional<Uuid> mSelectedFootprintUuid;

  // Preview graphics scene
  std::unique_ptr<GraphicsLayerList> mLayers;
  QScopedPointer<GraphicsScene> mPreviewGraphicsScene;
  QScopedPointer<FootprintGraphicsItem> mPreviewGraphicsItem;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
