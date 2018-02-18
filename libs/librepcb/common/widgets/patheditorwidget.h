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

#ifndef LIBREPCB_PATHEDITORWIDGET_H
#define LIBREPCB_PATHEDITORWIDGET_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include <QtWidgets>
#include "../geometry/path.h"

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {

/*****************************************************************************************
 *  Class PathEditorWidget
 ****************************************************************************************/

/**
 * @brief The PathEditorWidget class
 */
class PathEditorWidget final : public QWidget
{
        Q_OBJECT

    public:
        // Constructors / Destructor
        explicit PathEditorWidget(QWidget* parent = nullptr) noexcept;
        PathEditorWidget(const PathEditorWidget& other) = delete;
        ~PathEditorWidget() noexcept;

        // General Methods
        void setPath(const Path& path) noexcept;
        Path getPath() const;

        // Operator Overloadings
        PathEditorWidget& operator=(const PathEditorWidget& rhs) = delete;


    private:
        void setRowContent(int row, const QString& x, const QString& y,
                           const QString& angle, bool isLastRow) noexcept;
        void btnAddRemoveClicked() noexcept;
        int getRowOfTableCellWidget(const QWidget* widget) const noexcept;
        QString cellText(int row, int column, const QString& fallback = QString()) const noexcept;


    signals:
        void toggled(bool checked);
        void clicked(bool checked);
        void stateChanged(int state);


    private: // Data
        QTableWidget* mTable; // ownership by Qt's parent-child-mechanism
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace librepcb

#endif // LIBREPCB_PATHEDITORWIDGET_H
