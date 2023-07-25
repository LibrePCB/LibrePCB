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

#ifndef LIBREPCB_EDITOR_COMPONENTASSEMBLYOPTIONLISTEDITORWIDGET_H
#define LIBREPCB_EDITOR_COMPONENTASSEMBLYOPTIONLISTEDITORWIDGET_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/core/project/circuit/componentassemblyoption.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class ComponentInstance;
class Project;
class Workspace;

namespace editor {

class EditableTableWidget;
class PartListModel;

/*******************************************************************************
 *  Class ComponentAssemblyOptionListEditorWidget
 ******************************************************************************/

/**
 * @brief The ComponentAssemblyOptionListEditorWidget class
 */
class ComponentAssemblyOptionListEditorWidget final : public QWidget {
  Q_OBJECT

  enum Column {
    COLUMN_DEVICE,
    COLUMN_MPN,
    COLUMN_MANUFACTURER,
    COLUMN_ATTRIBUTES,
    COLUMN_MOUNT,
    _COLUMN_COUNT,
  };

public:
  // Constructors / Destructor
  explicit ComponentAssemblyOptionListEditorWidget(
      QWidget* parent = nullptr) noexcept;
  ComponentAssemblyOptionListEditorWidget(
      const ComponentAssemblyOptionListEditorWidget& other) = delete;
  ~ComponentAssemblyOptionListEditorWidget() noexcept;

  // Getters
  const ComponentAssemblyOptionList& getOptions() const noexcept {
    return mOptions;
  }

  // Setters
  void setFrameStyle(int style) noexcept;
  void setReferences(const Workspace* ws, const Project* project,
                     ComponentInstance* component) noexcept;

  // Operator Overloadings
  ComponentAssemblyOptionListEditorWidget& operator=(
      const ComponentAssemblyOptionListEditorWidget& rhs) = delete;

signals:
  void selectedPartChanged(std::shared_ptr<Part> part);

private:
  void addOption() noexcept;
  bool addPart() noexcept;
  void editOptionOrPart() noexcept;
  void removeOptionOrPart() noexcept;
  void itemChanged(QTreeWidgetItem* item, int column) noexcept;
  void itemSelectionChanged() noexcept;
  std::pair<int, int> getIndices(QTreeWidgetItem* item) noexcept;
  void optionListEdited(
      const ComponentAssemblyOptionList& list, int index,
      const std::shared_ptr<const ComponentAssemblyOption>& obj,
      ComponentAssemblyOptionList::Event event) noexcept;

private:  // Data
  QPointer<const Workspace> mWorkspace;
  QPointer<const Project> mProject;
  QPointer<ComponentInstance> mComponent;
  bool mMultiAssemblyVariantMode;
  ComponentAssemblyOptionList mOptions;
  QScopedPointer<QTreeWidget> mTreeWidget;
  QScopedPointer<QToolButton> mAddOptionButton;
  QScopedPointer<QToolButton> mAddPartButton;
  QScopedPointer<QToolButton> mEditButton;
  QScopedPointer<QToolButton> mRemoveButton;

  // Slots
  ComponentAssemblyOptionList::OnEditedSlot mOnListEditedSlot;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
