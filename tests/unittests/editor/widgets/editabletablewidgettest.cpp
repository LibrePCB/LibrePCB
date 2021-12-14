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

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "editabletablewidgetreceiver.h"

#include <gtest/gtest.h>
#include <librepcb/common/widgets/editabletablewidget.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace tests {

/*******************************************************************************
 *  Test Class
 ******************************************************************************/
class EditableTableWidgetTest : public ::testing::Test {
protected:
  static void connect(EditableTableWidget& widget,
                      EditableTableWidgetReceiver& receiver) noexcept {
    QObject::connect(&widget, &EditableTableWidget::btnAddClicked, &receiver,
                     &EditableTableWidgetReceiver::btnAddClicked);
    QObject::connect(&widget, &EditableTableWidget::btnRemoveClicked, &receiver,
                     &EditableTableWidgetReceiver::btnRemoveClicked);
    QObject::connect(&widget, &EditableTableWidget::btnCopyClicked, &receiver,
                     &EditableTableWidgetReceiver::btnCopyClicked);
    QObject::connect(&widget, &EditableTableWidget::btnEditClicked, &receiver,
                     &EditableTableWidgetReceiver::btnEditClicked);
    QObject::connect(&widget, &EditableTableWidget::btnMoveUpClicked, &receiver,
                     &EditableTableWidgetReceiver::btnMoveUpClicked);
    QObject::connect(&widget, &EditableTableWidget::btnMoveDownClicked,
                     &receiver,
                     &EditableTableWidgetReceiver::btnMoveDownClicked);
    QObject::connect(&widget, &EditableTableWidget::btnBrowseClicked, &receiver,
                     &EditableTableWidgetReceiver::btnBrowseClicked);
  }

  static QToolButton* getBtnRemove(QWidget* indexWidget) {
    if (!indexWidget) throw std::exception();  // abort test immediately
    QToolButton* btn = indexWidget->findChild<QToolButton*>("btnRemove");
    if (!btn) throw std::exception();  // abort test immediately
    return btn;
  }
};

/*******************************************************************************
 *  Test Methods
 ******************************************************************************/

TEST_F(EditableTableWidgetTest, testIfDataGetsUpdated) {
  QStringListModel model({"a", "b", "c"});
  QModelIndex index = model.index(1);
  model.setData(index, "foo", Qt::EditRole);

  EditableTableWidget widget;
  widget.setModel(&model);
  EditableTableWidgetReceiver receiver;
  connect(widget, receiver);

  QToolButton* btn = getBtnRemove(widget.indexWidget(index));
  btn->click();
  EXPECT_EQ("foo", receiver.mRemoveData.toString().toStdString());

  // Now change the underlying data and click again to see if the callback gets
  // called with the new data.
  model.setData(index, "bar", Qt::EditRole);
  btn->click();
  EXPECT_EQ("bar", receiver.mRemoveData.toString().toStdString());
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace librepcb
