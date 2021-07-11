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

#ifndef LIBREPCB_PROJECT_UNPLACEDCOMPONENTSDOCK_H
#define LIBREPCB_PROJECT_UNPLACEDCOMPONENTSDOCK_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/common/units/all_length_units.h>
#include <librepcb/common/uuid.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class GraphicsScene;
class DefaultGraphicsLayerProvider;

namespace library {
class Device;
class Package;
class Footprint;
class FootprintPreviewGraphicsItem;
}  // namespace library

namespace project {

class Project;
class Board;
class ComponentInstance;

namespace editor {

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

    /// Whether this device is pre-selected in schematics
    bool selectedInSchematic;
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
  void setSelectedDeviceAndPackage(const tl::optional<Uuid>& deviceUuid,
                                   const library::Package* package,
                                   bool packageOwned) noexcept;
  void setSelectedFootprintUuid(const tl::optional<Uuid>& uuid) noexcept;
  void setSelectedDeviceAsDefault() noexcept;
  void addSelectedDeviceToBoard() noexcept;
  void addSimilarDevicesToBoard() noexcept;
  void addPreSelectedDevicesToBoard() noexcept;
  void addAllDevicesToBoard() noexcept;
  void autoAddDevicesToBoard(
      bool onlyWithPreSelectedDevice,
      const tl::optional<Uuid>& libCmpUuidFilter) noexcept;

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
  tl::optional<Uuid> getSuggestedFootprint(const Uuid& libPkgUuid) const
      noexcept;

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
  tl::optional<Uuid> mSelectedDeviceUuid;
  const library::Package* mSelectedPackage;
  bool mSelectedPackageOwned;
  tl::optional<Uuid> mSelectedFootprintUuid;

  // Preview graphics scene
  QScopedPointer<DefaultGraphicsLayerProvider> mGraphicsLayerProvider;
  QScopedPointer<GraphicsScene> mPreviewGraphicsScene;
  QScopedPointer<library::FootprintPreviewGraphicsItem> mPreviewGraphicsItem;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace project
}  // namespace librepcb

#endif  // LIBREPCB_PROJECT_UNPLACEDCOMPONENTSDOCK_H
