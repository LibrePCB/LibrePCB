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

#ifndef LIBREPCB_WORKSPACE_WSI_USER_H
#define LIBREPCB_WORKSPACE_WSI_USER_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include "wsi_base.h"

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {
namespace workspace {

/*****************************************************************************************
 *  Class WSI_User
 ****************************************************************************************/

/**
 * @brief The WSI_User class contains the default author used for projects and libraries
 */
class WSI_User final : public WSI_Base
{
        Q_OBJECT

    public:

        // Constructors / Destructor
        WSI_User() = delete;
        WSI_User(const WSI_User& other) = delete;
        explicit WSI_User(const SExpression& node);
        ~WSI_User() noexcept;

        // Direct Access
        void setName(const QString& name) noexcept;
        const QString& getName() const noexcept {return mName;}

        // Getters: Widgets
        QString getLabelText() const noexcept {return tr("User Name:");}
        QWidget* getWidget() const noexcept {return mWidget.data();}

        // General Methods
        void restoreDefault() noexcept override;
        void apply() noexcept override;
        void revert() noexcept override;

        /// @copydoc librepcb::SerializableObject::serialize()
        void serialize(SExpression& root) const override;

        // Operator Overloadings
        WSI_User& operator=(const WSI_User& rhs) = delete;


    private: // Data
        QString mName;

        // Widgets
        QScopedPointer<QWidget> mWidget;
        QScopedPointer<QLineEdit> mNameEdit;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace workspace
} // namespace librepcb

#endif // LIBREPCB_WORKSPACE_WSI_USER_H
