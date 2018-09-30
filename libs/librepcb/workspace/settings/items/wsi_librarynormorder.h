/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
 * http://librepcb.org/
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

#ifndef LIBREPCB_WSI_LIBRARYNORMORDER_H
#define LIBREPCB_WSI_LIBRARYNORMORDER_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "wsi_base.h"

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace workspace {

/*******************************************************************************
 *  Class WSI_LibraryNormOrder
 ******************************************************************************/

/**
 * @brief The WSI_LibraryNormOrder class contains a list of norms which should
 * be used for all library elements (in the specified order)
 *
 * @author ubruhin
 * @date 2014-11-01
 */
class WSI_LibraryNormOrder final : public WSI_Base {
  Q_OBJECT

public:
  // Constructors / Destructor
  WSI_LibraryNormOrder()                                  = delete;
  WSI_LibraryNormOrder(const WSI_LibraryNormOrder& other) = delete;
  explicit WSI_LibraryNormOrder(const SExpression& node);
  ~WSI_LibraryNormOrder() noexcept;

  // Getters
  const QStringList& getNormOrder() const noexcept { return mList; }

  // Getters: Widgets
  QString getLabelText() const noexcept {
    return tr("Preferred Norms:\n(Highest priority at top)");
  }
  QWidget* getWidget() const noexcept { return mWidget.data(); }

  // General Methods
  void restoreDefault() noexcept override;
  void apply() noexcept override;
  void revert() noexcept override;

  /// @copydoc librepcb::SerializableObject::serialize()
  void serialize(SExpression& root) const override;

  // Operator Overloadings
  WSI_LibraryNormOrder& operator=(const WSI_LibraryNormOrder& rhs) = delete;

private:  // Methods
  void btnUpClicked() noexcept;
  void btnDownClicked() noexcept;
  void btnAddClicked() noexcept;
  void btnRemoveClicked() noexcept;
  void updateListWidgetItems() noexcept;

private:  // Data
  /**
   * @brief The list of norms (like "DIN EN 81346") in the right order
   *
   * The norm which should be used first is at index 0 of the list.
   */
  QStringList mList;
  QStringList mListTmp;

  // Widgets
  QScopedPointer<QWidget>     mWidget;
  QScopedPointer<QListWidget> mListWidget;
  QScopedPointer<QComboBox>   mComboBox;
  QScopedPointer<QToolButton> mBtnUp;
  QScopedPointer<QToolButton> mBtnDown;
  QScopedPointer<QToolButton> mBtnAdd;
  QScopedPointer<QToolButton> mBtnRemove;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace workspace
}  // namespace librepcb

#endif  // LIBREPCB_WSI_LIBRARYNORMORDER_H
