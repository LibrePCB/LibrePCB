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

#ifndef NEWELEMENTWIZARD_H
#define NEWELEMENTWIZARD_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "newelementwizardcontext.h"

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class IF_GraphicsLayerProvider;

namespace workspace {
class Workspace;
}

namespace library {

class Library;

namespace editor {

class NewElementWizardContext;

namespace Ui {
class NewElementWizard;
}

/*******************************************************************************
 *  Class NewElementWizard
 ******************************************************************************/

/**
 * @brief The NewElementWizard class
 *
 * @author ubruhin
 * @date 2017-03-21
 */
class NewElementWizard final : public QWizard {
  Q_OBJECT

public:
  // Constructors / Destructor
  NewElementWizard()                              = delete;
  NewElementWizard(const NewElementWizard& other) = delete;
  NewElementWizard(const workspace::Workspace& ws, Library& lib,
                   const IF_GraphicsLayerProvider& lp,
                   QWidget*                        parent = 0) noexcept;
  ~NewElementWizard() noexcept;

  // Getters
  const NewElementWizardContext& getContext() const noexcept {
    return *mContext;
  }

  // General Methods
  void setNewElementType(NewElementWizardContext::ElementType type) noexcept;
  void setElementToCopy(NewElementWizardContext::ElementType type,
                        const FilePath&                      fp) noexcept;
  bool validateCurrentPage() noexcept override;

  // Operator Overloadings
  NewElementWizard& operator=(const NewElementWizard& rhs) = delete;

private:  // Methods
  void insertPage(int index, QWizardPage* page) noexcept;

private:  // Data
  QScopedPointer<Ui::NewElementWizard>    mUi;
  QScopedPointer<NewElementWizardContext> mContext;
  QList<QWizardPage*>                     mPages;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace library
}  // namespace librepcb

#endif  // NEWELEMENTWIZARD_H
