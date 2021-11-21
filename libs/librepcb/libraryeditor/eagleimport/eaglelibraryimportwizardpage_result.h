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

#ifndef LIBREPCB_LIBRARY_EDITOR_EAGLELIBRARYIMPORTWIZARDPAGE_RESULT_H
#define LIBREPCB_LIBRARY_EDITOR_EAGLELIBRARYIMPORTWIZARDPAGE_RESULT_H

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
class EagleLibraryImportWizardPage_Result;
}

/*******************************************************************************
 *  Class EagleLibraryImportWizardPage_Result
 ******************************************************************************/

/**
 * @brief The EagleLibraryImportWizardPage_Result class
 */
class EagleLibraryImportWizardPage_Result final : public QWizardPage {
  Q_OBJECT

public:
  // Constructors / Destructor
  EagleLibraryImportWizardPage_Result() = delete;
  EagleLibraryImportWizardPage_Result(
      const EagleLibraryImportWizardPage_Result& other) = delete;
  EagleLibraryImportWizardPage_Result(
      std::shared_ptr<EagleLibraryImportWizardContext> context,
      QWidget* parent = nullptr) noexcept;
  ~EagleLibraryImportWizardPage_Result() noexcept;

  // General Methods
  virtual void initializePage() override;
  virtual bool isComplete() const override;

  // Operator Overloadings
  EagleLibraryImportWizardPage_Result& operator=(
      const EagleLibraryImportWizardPage_Result& rhs) = delete;

private:  // Methods
  void importFinished(const QStringList& errors) noexcept;

private:  // Data
  QScopedPointer<Ui::EagleLibraryImportWizardPage_Result> mUi;
  std::shared_ptr<EagleLibraryImportWizardContext> mContext;
  QVector<QMetaObject::Connection> mProgressBarConnections;
  bool mIsCompleted;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace library
}  // namespace librepcb

#endif
