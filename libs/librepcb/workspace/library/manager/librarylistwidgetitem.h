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

#ifndef LIBREPCB_WORKSPACE_LIBRARYLISTWIDGETITEM_H
#define LIBREPCB_WORKSPACE_LIBRARYLISTWIDGETITEM_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include <QtWidgets>
#include <librepcbcommon/exceptions.h>

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {

namespace library {
class Library;
}

namespace workspace {

class Workspace;

namespace Ui {
class LibraryListWidgetItem;
}

/*****************************************************************************************
 *  Class LibraryListWidgetItem
 ****************************************************************************************/

/**
 * @brief The LibraryListWidgetItem class
 *
 * @author ubruhin
 * @date 2016-08-03
 */
class LibraryListWidgetItem final : public QWidget
{
        Q_OBJECT

    public:

        // Constructors / Destructor
        LibraryListWidgetItem() noexcept;
        LibraryListWidgetItem(const LibraryListWidgetItem& other) = delete;
        LibraryListWidgetItem(const Workspace& ws, QSharedPointer<library::Library> lib) noexcept;
        ~LibraryListWidgetItem() noexcept;

        // Getters
        QSharedPointer<library::Library> getLibrary() const noexcept {return mLib;}
        QString getName() const noexcept;
        bool isRemoteLibrary() const noexcept;

        // Operator Overloadings
        LibraryListWidgetItem& operator=(const LibraryListWidgetItem& rhs) = delete;


    private: // Data

        QScopedPointer<Ui::LibraryListWidgetItem> mUi;
        QSharedPointer<library::Library> mLib;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace workspace
} // namespace librepcb

#endif // LIBREPCB_WORKSPACE_LIBRARYLISTWIDGETITEM_H
