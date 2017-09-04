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

#ifndef LIBREPCB_WORKSPACE_LIBRARYINFOWIDGET_H
#define LIBREPCB_WORKSPACE_LIBRARYINFOWIDGET_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include <QtWidgets>
#include <librepcb/common/exceptions.h>

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {

namespace workspace {
class Workspace;
}

namespace library {

class Library;

namespace manager {

namespace Ui {
class LibraryInfoWidget;
}

/*****************************************************************************************
 *  Class LibraryInfoWidget
 ****************************************************************************************/

/**
 * @brief The LibraryInfoWidget class
 *
 * @author ubruhin
 * @date 2016-08-03
 */
class LibraryInfoWidget final : public QWidget
{
        Q_OBJECT

    public:

        // Constructors / Destructor
        LibraryInfoWidget() noexcept;
        LibraryInfoWidget(const LibraryInfoWidget& other) = delete;
        LibraryInfoWidget(workspace::Workspace& ws, QSharedPointer<Library> lib) noexcept;
        ~LibraryInfoWidget() noexcept;

        // Getters
        QString getName() const noexcept;

        // Operator Overloadings
        LibraryInfoWidget& operator=(const LibraryInfoWidget& rhs) = delete;


    signals:

        void libraryRemoved(const FilePath& libDir);
        void openLibraryEditorTriggered(QSharedPointer<Library> lib);


    private: // Methods

        void btnOpenLibraryEditorClicked() noexcept;
        void btnRemoveLibraryClicked() noexcept;
        bool isRemoteLibrary() const noexcept;


    private: // Data

        QScopedPointer<Ui::LibraryInfoWidget> mUi;
        workspace::Workspace& mWorkspace;
        QSharedPointer<Library> mLib;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace manager
} // namespace library
} // namespace librepcb

#endif // LIBREPCB_WORKSPACE_LIBRARYINFOWIDGET_H
