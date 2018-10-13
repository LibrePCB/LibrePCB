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
class UndoCommandGroup;

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
 *
 * @todo This class is very provisional and may contain dangerous bugs...
 */
class UnplacedComponentsDock final : public QDockWidget {
  Q_OBJECT

public:
  // Constructors / Destructor
  explicit UnplacedComponentsDock(ProjectEditor& editor);
  ~UnplacedComponentsDock();

  // Getters
  int getUnplacedComponentsCount() const noexcept;

  // Setters
  void setBoard(Board* board);

signals:

  void unplacedComponentsCountChanged(int count);
  void addDeviceTriggered(ComponentInstance& cmp, const Uuid& deviceUuid,
                          Uuid footprintUuid);

private slots:

  void on_lstUnplacedComponents_currentItemChanged(QListWidgetItem* current,
                                                   QListWidgetItem* previous);
  void on_cbxSelectedDevice_currentIndexChanged(int index);
  void on_cbxSelectedFootprint_currentIndexChanged(int index);
  void on_btnAdd_clicked();
  void on_pushButton_clicked();
  void on_btnAddAll_clicked();

private:
  // make some methods inaccessible...
  UnplacedComponentsDock();
  UnplacedComponentsDock(const UnplacedComponentsDock& other);
  UnplacedComponentsDock& operator=(const UnplacedComponentsDock& rhs);

  // Private Methods
  void updateComponentsList() noexcept;
  void setSelectedComponentInstance(ComponentInstance* cmp) noexcept;
  void setSelectedDeviceAndPackage(const library::Device*  device,
                                   const library::Package* package) noexcept;
  void setSelectedFootprintUuid(const tl::optional<Uuid>& uuid) noexcept;
  void beginUndoCmdGroup() noexcept;
  void addNextDeviceToCmdGroup(
      ComponentInstance& cmp, const Uuid& deviceUuid,
      const tl::optional<Uuid>& footprintUuid) noexcept;
  void commitUndoCmdGroup() noexcept;
  void addDeviceManually(ComponentInstance& cmp, const Uuid& deviceUuid,
                         Uuid footprintUuid) noexcept;

  // General
  ProjectEditor&                         mProjectEditor;
  Project&                               mProject;
  Board*                                 mBoard;
  Ui::UnplacedComponentsDock*            mUi;
  GraphicsScene*                         mFootprintPreviewGraphicsScene;
  library::FootprintPreviewGraphicsItem* mFootprintPreviewGraphicsItem;
  ComponentInstance*                     mSelectedComponent;
  const library::Device*                 mSelectedDevice;
  const library::Package*                mSelectedPackage;
  tl::optional<Uuid>                     mSelectedFootprintUuid;
  QMetaObject::Connection                mCircuitConnection1;
  QMetaObject::Connection                mCircuitConnection2;
  QMetaObject::Connection                mBoardConnection1;
  QMetaObject::Connection                mBoardConnection2;
  Point                                  mNextPosition;
  bool                                   mDisableListUpdate;
  QHash<Uuid, Uuid>                      mLastDeviceOfComponent;
  QHash<Uuid, tl::optional<Uuid>>        mLastFootprintOfDevice;
  QScopedPointer<UndoCommandGroup>       mCurrentUndoCmdGroup;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace project
}  // namespace librepcb

#endif  // LIBREPCB_PROJECT_UNPLACEDCOMPONENTSDOCK_H
