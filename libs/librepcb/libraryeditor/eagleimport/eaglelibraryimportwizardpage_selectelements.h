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

#ifndef LIBREPCB_LIBRARY_EDITOR_EAGLELIBRARYIMPORTWIZARDPAGE_SELECTELEMENTS_H
#define LIBREPCB_LIBRARY_EDITOR_EAGLELIBRARYIMPORTWIZARDPAGE_SELECTELEMENTS_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <QtCore>
#include <QtWidgets>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace library {
namespace editor {

class EagleLibraryImportWizardContext;

namespace Ui {
class EagleLibraryImportWizardPage_SelectElements;
}

/*******************************************************************************
 *  Class EagleLibraryImportWizardPage_SelectElements
 ******************************************************************************/

/**
 * @brief The EagleLibraryImportWizardPage_SelectElements class
 */
class EagleLibraryImportWizardPage_SelectElements final : public QWizardPage {
  Q_OBJECT

  enum ElementType {
    _Unknown = 0,
    Device,
    Component,
    Symbol,
    Package,
  };

public:
  // Constructors / Destructor
  EagleLibraryImportWizardPage_SelectElements() = delete;
  EagleLibraryImportWizardPage_SelectElements(
      const EagleLibraryImportWizardPage_SelectElements& other) = delete;
  EagleLibraryImportWizardPage_SelectElements(
      std::shared_ptr<EagleLibraryImportWizardContext> context,
      QWidget* parent = nullptr) noexcept;
  ~EagleLibraryImportWizardPage_SelectElements() noexcept;

  // General Methods
  virtual void initializePage() override;
  virtual bool isComplete() const override;

  // Operator Overloadings
  EagleLibraryImportWizardPage_SelectElements& operator=(
      const EagleLibraryImportWizardPage_SelectElements& rhs) = delete;

private:  // Methods
  void treeItemChanged(QTreeWidgetItem* item) noexcept;
  void updateItemCheckState(ElementType elementType, const QString& name,
                            Qt::CheckState state) noexcept;
  void updateRootNodes() noexcept;

private:  // Data
  QScopedPointer<Ui::EagleLibraryImportWizardPage_SelectElements> mUi;
  std::shared_ptr<EagleLibraryImportWizardContext> mContext;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace library
}  // namespace librepcb

#endif
