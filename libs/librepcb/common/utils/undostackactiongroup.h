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

#ifndef LIBREPCB_UNDOSTACKACTIONGROUP_H
#define LIBREPCB_UNDOSTACKACTIONGROUP_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include <QtWidgets>

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {

class UndoStack;

/*****************************************************************************************
 *  Class UndoStackActionGroup
 ****************************************************************************************/

/**
 * @brief The UndoStackActionGroup class groups an undo-QAction and redo-QAction together
 *        and optionally connects them with a librepcb::UndoStack
 *
 * @author ubruhin
 * @date 2016-12-04
 */
class UndoStackActionGroup final : public QObject
{
        Q_OBJECT

    public:

        // Constructors / Destructor
        UndoStackActionGroup() = delete;
        UndoStackActionGroup(const UndoStackActionGroup& other) = delete;
        UndoStackActionGroup(QAction& undo, QAction& redo, QAction* save,
                             UndoStack* stack, QWidget* msgBoxParent) noexcept;
        ~UndoStackActionGroup() noexcept;

        // General Methods
        void setUndoStack(UndoStack* stack) noexcept;

        // Operator Overloadings
        UndoStackActionGroup& operator=(const UndoStackActionGroup& rhs) = delete;


    private: // Methods
        void undoTriggered() noexcept;
        void redoTriggered() noexcept;
        void unregisterFromStack() noexcept;
        void registerToStack(UndoStack* stack) noexcept;


    private: // Data
        QAction& mUndo;
        QAction& mRedo;
        QAction* mSave;
        UndoStack* mStack;
        QWidget* mMsgBoxParent;
        QList<QMetaObject::Connection> mConnections;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace librepcb

#endif // LIBREPCB_UNDOSTACKACTIONGROUP_H
