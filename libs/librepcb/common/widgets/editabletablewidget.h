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

#ifndef LIBREPCB_EDITABLETABLEWIDGET_H
#define LIBREPCB_EDITABLETABLEWIDGET_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class EditableTableWidget
 ******************************************************************************/

/**
 * @brief A QTableView subclass which adds buttons to edit the underlying model
 */
class EditableTableWidget : public QTableView {
  Q_OBJECT

  typedef void (EditableTableWidget::*Signal)(const QVariant& data);

public:
  // Constructors / Destructor
  explicit EditableTableWidget(QWidget* parent = nullptr) noexcept;
  EditableTableWidget(const EditableTableWidget& other) = delete;
  virtual ~EditableTableWidget() noexcept;

  // Setters
  void setShowCopyButton(bool show) noexcept { mShowCopyButton = show; }
  void setShowEditButton(bool show) noexcept { mShowEditButton = show; }
  void setShowMoveButtons(bool show) noexcept { mShowMoveButtons = show; }
  void setBrowseButtonColumn(int col) noexcept { mBrowseButtonColumn = col; }

  // Inherited
  virtual void reset() override;

  // Operator Overloadings
  EditableTableWidget& operator=(const EditableTableWidget& rhs) = delete;

protected:
  virtual void currentChanged(const QModelIndex& current,
                              const QModelIndex& previous) override;
  virtual void rowsInserted(const QModelIndex& parent, int start,
                            int end) override;

signals:
  void currentRowChanged(int row);
  void btnAddClicked(const QVariant& data);
  void btnRemoveClicked(const QVariant& data);
  void btnCopyClicked(const QVariant& data);
  void btnEditClicked(const QVariant& data);
  void btnMoveUpClicked(const QVariant& data);
  void btnMoveDownClicked(const QVariant& data);
  void btnBrowseClicked(const QVariant& data);

private:
  void         installButtons(int row) noexcept;
  QToolButton* createButton(const QIcon& icon, const QString& text,
                            const QString& toolTip, int width, int height,
                            const QVariant& data,
                            Signal          clickedSignal) noexcept;

  bool mShowCopyButton;
  bool mShowEditButton;
  bool mShowMoveButtons;
  int  mBrowseButtonColumn;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif  // LIBREPCB_EDITABLETABLEWIDGET_H
