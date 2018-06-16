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

#ifndef LIBREPCB_CIRCLEPROPERTIESDIALOG_H
#define LIBREPCB_CIRCLEPROPERTIESDIALOG_H

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
class Circle;
class GraphicsLayer;

namespace Ui {
class CirclePropertiesDialog;
}

/*****************************************************************************************
 *  Class CirclePropertiesDialog
 ****************************************************************************************/

/**
 * @brief The CirclePropertiesDialog class
 */
class CirclePropertiesDialog final : public QDialog
{
        Q_OBJECT

    public:

        // Constructors / Destructor
        CirclePropertiesDialog() = delete;
        CirclePropertiesDialog(const CirclePropertiesDialog& other) = delete;
        CirclePropertiesDialog(Circle& circle, UndoStack& undoStack,
                                QList<GraphicsLayer*> layers, QWidget* parent = nullptr) noexcept;
        ~CirclePropertiesDialog() noexcept;

        // Operator Overloadings
        CirclePropertiesDialog& operator=(const CirclePropertiesDialog& rhs) = delete;


    private: // GUI Events
        void buttonBoxClicked(QAbstractButton* button) noexcept;

    private: // Methods
        bool applyChanges() noexcept;
        void selectLayerNameInCombobox(const QString& name) noexcept;


    private: // Data
        Circle& mCircle;
        UndoStack& mUndoStack;
        QScopedPointer<Ui::CirclePropertiesDialog> mUi;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace librepcb

#endif // LIBREPCB_CIRCLEPROPERTIESDIALOG_H
