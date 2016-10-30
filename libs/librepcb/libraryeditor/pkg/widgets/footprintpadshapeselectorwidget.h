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

#ifndef LIBREPCB_LIBRARY_EDITOR_FOOTPRINTPADSHAPESELECTORWIDGET_H
#define LIBREPCB_LIBRARY_EDITOR_FOOTPRINTPADSHAPESELECTORWIDGET_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include <QtWidgets>
#include <librepcb/library/pkg/footprintpad.h>

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {
namespace library {
namespace editor {

/*****************************************************************************************
 *  Class FootprintPadShapeSelectorWidget
 ****************************************************************************************/

/**
 * @brief The FootprintPadShapeSelectorWidget class
 *
 * @author ubruhin
 * @date 2017-08-17
 */
class FootprintPadShapeSelectorWidget final : public QWidget
{
        Q_OBJECT

    public:
        // Constructors / Destructor
        explicit FootprintPadShapeSelectorWidget(QWidget* parent = nullptr) noexcept;
        FootprintPadShapeSelectorWidget(const FootprintPadShapeSelectorWidget& other) = delete;
        ~FootprintPadShapeSelectorWidget() noexcept;

        // Getters
        FootprintPad::Shape getCurrentShape() const noexcept;

        // Setters
        void setCurrentShape(FootprintPad::Shape shape) noexcept;

        // Operator Overloadings
        FootprintPadShapeSelectorWidget& operator=(const FootprintPadShapeSelectorWidget& rhs) = delete;


    signals:
        void currentShapeChanged(FootprintPad::Shape shape);


    private: // Methods
        void btnRoundToggled(bool checked) noexcept;
        void btnRectToggled(bool checked) noexcept;
        void btnOctagonToggled(bool checked) noexcept;


    private: // Data
        QToolButton* mBtnRound;
        QToolButton* mBtnRect;
        QToolButton* mBtnOctagon;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace editor
} // namespace library
} // namespace librepcb

#endif // LIBREPCB_LIBRARY_EDITOR_FOOTPRINTPADSHAPESELECTORWIDGET_H
