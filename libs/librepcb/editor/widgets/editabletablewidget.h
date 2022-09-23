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

#ifndef LIBREPCB_EDITOR_EDITABLETABLEWIDGET_H
#define LIBREPCB_EDITOR_EDITABLETABLEWIDGET_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

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
  void setReadOnly(bool readOnly) noexcept;
  void setShowCopyButton(bool show) noexcept { mShowCopyButton = show; }
  void setShowEditButton(bool show) noexcept { mShowEditButton = show; }
  void setShowMoveButtons(bool show) noexcept { mShowMoveButtons = show; }
  void setBrowseButtonColumn(int col) noexcept { mBrowseButtonColumn = col; }

  // Inherited
  using QAbstractItemView::edit;  // Required to override edit() overload below.
  virtual void reset() override;

  // Operator Overloadings
  EditableTableWidget& operator=(const EditableTableWidget& rhs) = delete;

protected:
  virtual bool edit(const QModelIndex& index, EditTrigger trigger,
                    QEvent* event) override;
  virtual void currentChanged(const QModelIndex& current,
                              const QModelIndex& previous) override;
  virtual void rowsInserted(const QModelIndex& parent, int start,
                            int end) override;

signals:
  void readOnlyChanged(bool readOnly);
  void currentRowChanged(int row);
  void btnAddClicked(const QVariant& data);
  void btnRemoveClicked(const QVariant& data);
  void btnCopyClicked(const QVariant& data);
  void btnEditClicked(const QVariant& data);
  void btnMoveUpClicked(const QVariant& data);
  void btnMoveDownClicked(const QVariant& data);
  void btnBrowseClicked(const QVariant& data);

private:
  void installButtons(int row) noexcept;
  QToolButton* createButton(const QString& objectName, const QIcon& icon,
                            const QString& text, const QString& toolTip,
                            int width, int height, Signal clickedSignal,
                            const QPersistentModelIndex& index,
                            bool doesModify) noexcept;
  void buttonClickedHandler(Signal clickedSignal,
                            const QPersistentModelIndex& index) noexcept;

  bool mShowCopyButton;
  bool mShowEditButton;
  bool mShowMoveButtons;
  int mBrowseButtonColumn;
  bool mReadOnly;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
