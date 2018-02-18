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

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include <QtWidgets>
#include "patheditorwidget.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

PathEditorWidget::PathEditorWidget(QWidget* parent) noexcept :
    QWidget(parent), mTable(nullptr)
{
    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0,0,0,0);
    mTable = new QTableWidget(this);
    mTable->setColumnCount(4);
    mTable->setHorizontalHeaderItem(0, new QTableWidgetItem(tr("Pos. X")));
    mTable->setHorizontalHeaderItem(1, new QTableWidgetItem(tr("Pos Y.")));
    mTable->setHorizontalHeaderItem(2, new QTableWidgetItem(tr("Angle")));
    mTable->setHorizontalHeaderItem(3, new QTableWidgetItem(""));
    mTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    mTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    mTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    mTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    mTable->verticalHeader()->setDefaultSectionSize(20);
    mTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    mTable->setSelectionMode(QAbstractItemView::SingleSelection);
    mTable->setCornerButtonEnabled(false);
    layout->addWidget(mTable);
}

PathEditorWidget::~PathEditorWidget() noexcept
{
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void PathEditorWidget::setPath(const Path& path) noexcept
{
    mTable->setRowCount(path.getVertices().count() + 1);
    for (int i = 0; i < path.getVertices().count(); ++i) {
        const Vertex& v = path.getVertices().at(i);
        setRowContent(i, v.getPos().getX().toMmString(), v.getPos().getY().toMmString(),
                      v.getAngle().toDegString(), false);
    }
    setRowContent(mTable->rowCount() - 1, QString(), QString(), QString(), true);
}

Path PathEditorWidget::getPath() const
{
    Path path;
    for (int i = 0; i < mTable->rowCount() - 1; ++i) {
        QString x = cellText(i, 0, "0");
        QString y = cellText(i, 1, "0");
        QString a = cellText(i, 2, "0");
        Point pos = Point(Length::fromMm(x), Length::fromMm(y)); // can throw
        Angle angle = Angle::fromDeg(a); // can throw
        path.addVertex(Vertex(pos, angle));
    }
    return path;
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

void PathEditorWidget::setRowContent(int row, const QString& x, const QString& y,
                                     const QString& angle, bool isLastRow) noexcept
{
    // vertex
    mTable->setItem(row, 0, new QTableWidgetItem(x));
    mTable->setItem(row, 1, new QTableWidgetItem(y));
    mTable->setItem(row, 2, new QTableWidgetItem(angle));

    // button
    int btnSize = 23; // TODO: can we determine this value dynamically?
    QToolButton* btnAddRemove = new QToolButton(this);
    btnAddRemove->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    btnAddRemove->setFixedSize(btnSize, btnSize);
    btnAddRemove->setIconSize(QSize(btnSize - 6, btnSize - 6));
    connect(btnAddRemove, &QToolButton::clicked,
            this, &PathEditorWidget::btnAddRemoveClicked);
    if (isLastRow) {
        btnAddRemove->setIcon(QIcon(":/img/actions/add.png"));
    } else {
        btnAddRemove->setIcon(QIcon(":/img/actions/minus.png"));
    }
    mTable->setCellWidget(row, 3, btnAddRemove);

    // adjust the height of the row according to the size of the contained widgets
    mTable->verticalHeader()->resizeSection(row, btnSize);
}

void PathEditorWidget::btnAddRemoveClicked() noexcept
{
    int row = getRowOfTableCellWidget(dynamic_cast<QWidget*>(sender()));
    if (row == mTable->rowCount() - 1) {
        QString x = cellText(row, 0, "0");
        QString y = cellText(row, 1, "0");
        QString a = cellText(row, 2, "0");
        mTable->insertRow(row);
        setRowContent(row, x, y, a, false);
    } else if (row >= 0) {
        mTable->removeRow(row);
    }
}

int PathEditorWidget::getRowOfTableCellWidget(const QWidget* widget) const noexcept
{
    for (int row = 0; row < mTable->rowCount(); ++row) {
        if (mTable->cellWidget(row, 3) == widget) {
            return row;
        }
    }
    return -1;
}

QString PathEditorWidget::cellText(int row, int column, const QString& fallback) const noexcept
{
    const QTableWidgetItem* item = mTable->item(row, column);
    QString text = item ? item->text().trimmed() : QString();
    if (text.isEmpty()) {
        return fallback;
    } else {
        return text;
    }
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace librepcb
