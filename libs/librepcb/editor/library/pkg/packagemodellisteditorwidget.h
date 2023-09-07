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

#ifndef LIBREPCB_EDITOR_PACKAGEMODELLISTEDITORWIDGET_H
#define LIBREPCB_EDITOR_PACKAGEMODELLISTEDITORWIDGET_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/core/library/pkg/packagemodel.h>

#include <QtCore>
#include <QtWidgets>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Footprint;
class Package;

namespace editor {

class EditableTableWidget;
class PackageModelListModel;
class SortFilterProxyModel;
class UndoStack;

/*******************************************************************************
 *  Class PackageModelListEditorWidget
 ******************************************************************************/

/**
 * @brief The PackageModelListEditorWidget class
 */
class PackageModelListEditorWidget final : public QWidget {
  Q_OBJECT

public:
  // Constructors / Destructor
  explicit PackageModelListEditorWidget(QWidget* parent = nullptr) noexcept;
  PackageModelListEditorWidget(const PackageModelListEditorWidget& other) =
      delete;
  ~PackageModelListEditorWidget() noexcept;

  // Setters
  void setFrameStyle(int style) noexcept;
  void setReadOnly(bool readOnly) noexcept;
  void setReferences(Package* package, UndoStack* stack) noexcept;
  void setCurrentFootprint(std::shared_ptr<Footprint> footprint) noexcept;

  // Operator Overloadings
  PackageModelListEditorWidget& operator=(
      const PackageModelListEditorWidget& rhs) = delete;

signals:
  void currentIndexChanged(int index);

private:
  QPointer<Package> mCurrentPackage;
  QScopedPointer<PackageModelListModel> mModel;
  QScopedPointer<SortFilterProxyModel> mProxy;
  QScopedPointer<EditableTableWidget> mView;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
