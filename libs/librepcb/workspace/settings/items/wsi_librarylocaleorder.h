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

#ifndef LIBREPCB_WSI_LIBRARYLOCALEORDER_H
#define LIBREPCB_WSI_LIBRARYLOCALEORDER_H

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
 *  Class WSI_LibraryLocaleOrder
 ******************************************************************************/

/**
 * @brief The WSI_LibraryLocaleOrder class contains a list of locales which
 * should be used for all (translatable) strings in library elements (in the
 * specified order)
 */
class WSI_LibraryLocaleOrder final : public WSI_Base {
  Q_OBJECT

public:
  // Constructors / Destructor
  WSI_LibraryLocaleOrder()                                    = delete;
  WSI_LibraryLocaleOrder(const WSI_LibraryLocaleOrder& other) = delete;
  explicit WSI_LibraryLocaleOrder(const SExpression& node);
  ~WSI_LibraryLocaleOrder() noexcept;

  // Getters
  const QStringList& getLocaleOrder() const noexcept { return mList; }

  // Getters: Widgets
  QString getLabelText() const noexcept {
    return tr("Preferred Languages:\n(Highest priority at top)");
  }
  QWidget* getWidget() const noexcept { return mWidget.data(); }

  // General Methods
  void restoreDefault() noexcept override;
  void apply() noexcept override;
  void revert() noexcept override;

  /// @copydoc librepcb::SerializableObject::serialize()
  void serialize(SExpression& root) const override;

  // Operator Overloadings
  WSI_LibraryLocaleOrder& operator=(const WSI_LibraryLocaleOrder& rhs) = delete;

private:  // Methods
  void btnUpClicked() noexcept;
  void btnDownClicked() noexcept;
  void btnAddClicked() noexcept;
  void btnRemoveClicked() noexcept;
  void updateListWidgetItems() noexcept;

private:  // Data
  /**
   * @brief The list of locales (like "de_CH") in the right order
   *
   * The locale which should be used first is at index 0 of the list. If no
   * translation strings are found for all locales in this list, the fallback
   * locale "en_US" will be used automatically, so the list do not have to
   * contain "en_US". An empty list is also valid, then the fallback locale
   * "en_US" will be used.
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

#endif  // LIBREPCB_WSI_LIBRARYLOCALEORDER_H
