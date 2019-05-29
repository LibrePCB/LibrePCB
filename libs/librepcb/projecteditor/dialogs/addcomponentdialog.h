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

#ifndef LIBREPCB_PROJECT_ADDCOMPONENTDIALOG_H
#define LIBREPCB_PROJECT_ADDCOMPONENTDIALOG_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/common/exceptions.h>
#include <librepcb/common/fileio/filepath.h>
#include <librepcb/common/uuid.h>
#include <librepcb/workspace/library/cat/categorytreemodel.h>

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
class FootprintPreviewGraphicsItem;
class Component;
class ComponentSymbolVariant;
class Symbol;
class SymbolPreviewGraphicsItem;
class ComponentCategory;
}  // namespace library

namespace workspace {
class Workspace;
}

namespace project {

class Project;

namespace editor {

namespace Ui {
class AddComponentDialog;
}

/*******************************************************************************
 *  Class AddComponentDialog
 ******************************************************************************/

/**
 * @brief The AddComponentDialog class
 */
class AddComponentDialog final : public QDialog {
  Q_OBJECT

  // Types
  struct SearchResultDevice {
    QString  name;
    FilePath pkgFp;
    QString  pkgName;
    bool     match = false;
  };

  struct SearchResultComponent {
    QString                             name;
    QHash<FilePath, SearchResultDevice> devices;
    bool                                match = false;
  };

  typedef QHash<FilePath, SearchResultComponent> SearchResult;

public:
  // Constructors / Destructor
  explicit AddComponentDialog(workspace::Workspace& workspace, Project& project,
                              QWidget* parent = nullptr);
  ~AddComponentDialog() noexcept;

  // Getters
  tl::optional<Uuid> getSelectedComponentUuid() const noexcept;
  tl::optional<Uuid> getSelectedSymbVarUuid() const noexcept;
  tl::optional<Uuid> getSelectedDeviceUuid() const noexcept;

private slots:
  void searchEditTextChanged(const QString& text) noexcept;
  void treeCategories_currentItemChanged(const QModelIndex& current,
                                         const QModelIndex& previous) noexcept;
  void treeComponents_currentItemChanged(QTreeWidgetItem* current,
                                         QTreeWidgetItem* previous) noexcept;
  void treeComponents_itemDoubleClicked(QTreeWidgetItem* item,
                                        int              column) noexcept;
  void on_cbxSymbVar_currentIndexChanged(int index) noexcept;

private:
  // Private Methods
  void         searchComponents(const QString& input);
  SearchResult searchComponentsAndDevices(const QString& input);
  void         setSelectedCategory(const tl::optional<Uuid>& categoryUuid);
  void         setSelectedComponent(const library::Component* cmp);
  void setSelectedSymbVar(const library::ComponentSymbolVariant* symbVar);
  void setSelectedDevice(const library::Device* dev);
  void accept() noexcept;

  // General
  workspace::Workspace&                        mWorkspace;
  Project&                                     mProject;
  Ui::AddComponentDialog*                      mUi;
  GraphicsScene*                               mComponentPreviewScene;
  GraphicsScene*                               mDevicePreviewScene;
  QScopedPointer<DefaultGraphicsLayerProvider> mGraphicsLayerProvider;
  workspace::ComponentCategoryTreeModel*       mCategoryTreeModel;

  // Attributes
  tl::optional<Uuid>                         mSelectedCategoryUuid;
  const library::Component*                  mSelectedComponent;
  const library::ComponentSymbolVariant*     mSelectedSymbVar;
  const library::Device*                     mSelectedDevice;
  const library::Package*                    mSelectedPackage;
  QList<library::SymbolPreviewGraphicsItem*> mPreviewSymbolGraphicsItems;
  library::FootprintPreviewGraphicsItem*     mPreviewFootprintGraphicsItem;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace project
}  // namespace librepcb

#endif  // LIBREPCB_PROJECT_ADDCOMPONENTDIALOG_H
