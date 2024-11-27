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

#ifndef LIBREPCB_EDITOR_KICADLIBRARYIMPORTWIZARDPAGE_SELECTELEMENTS_H
#define LIBREPCB_EDITOR_KICADLIBRARYIMPORTWIZARDPAGE_SELECTELEMENTS_H

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
namespace editor {

class KiCadLibraryImportWizardContext;

namespace Ui {
class KiCadLibraryImportWizardPage_SelectElements;
}

/*******************************************************************************
 *  Class KiCadLibraryImportWizardPage_SelectElements
 ******************************************************************************/

/**
 * @brief The KiCadLibraryImportWizardPage_SelectElements class
 */
class KiCadLibraryImportWizardPage_SelectElements final : public QWizardPage {
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
  KiCadLibraryImportWizardPage_SelectElements() = delete;
  KiCadLibraryImportWizardPage_SelectElements(
      const KiCadLibraryImportWizardPage_SelectElements& other) = delete;
  KiCadLibraryImportWizardPage_SelectElements(
      std::shared_ptr<KiCadLibraryImportWizardContext> context,
      QWidget* parent = nullptr) noexcept;
  ~KiCadLibraryImportWizardPage_SelectElements() noexcept;

  // General Methods
  virtual void initializePage() override;
  virtual bool isComplete() const override;

  // Operator Overloadings
  KiCadLibraryImportWizardPage_SelectElements& operator=(
      const KiCadLibraryImportWizardPage_SelectElements& rhs) = delete;

private:  // Methods
  void treeItemChanged(QTreeWidgetItem* item) noexcept;
  void applyChangedCheckState(bool checked) noexcept;
  void updateItemCheckState(ElementType elementType, const QString& libName,
                            const QString& name, Qt::CheckState state) noexcept;
  void updateRootNodes() noexcept;

private:  // Data
  QScopedPointer<Ui::KiCadLibraryImportWizardPage_SelectElements> mUi;
  std::shared_ptr<KiCadLibraryImportWizardContext> mContext;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
