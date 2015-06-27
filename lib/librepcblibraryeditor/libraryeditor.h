/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 Urban Bruhin
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

#ifndef LIBRARYEDITOR_LIBRARYEDITOR_H
#define LIBRARYEDITOR_LIBRARYEDITOR_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include <QtWidgets>

/*****************************************************************************************
 *  Forward Declarations
 ****************************************************************************************/

namespace Ui {
class LibraryEditor;
}

/*****************************************************************************************
 *  Class LibraryEditor
 ****************************************************************************************/

namespace library_editor {

/**
 * @brief The LibraryEditor class
 *
 * @todo this is only a stub class...
 */
class LibraryEditor : public QMainWindow
{
        Q_OBJECT

    public:

        // Constructors / Destructor
        LibraryEditor();
        ~LibraryEditor();

    private:

        // make some methods inaccessible...
        LibraryEditor(const LibraryEditor& other);
        LibraryEditor& operator=(const LibraryEditor& rhs);

        // Attributes
        Ui::LibraryEditor* ui;
};

} // namespace library_editor

#endif // LIBRARYEDITOR_LIBRARYEDITOR_H
