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

#ifndef LIBREPCB_WORKSPACE_WSI_REPOSITORIES_H
#define LIBREPCB_WORKSPACE_WSI_REPOSITORIES_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include "wsi_base.h"

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {

class Repository;

namespace workspace {

/*****************************************************************************************
 *  Class WSI_Repositories
 ****************************************************************************************/

/**
 * @brief The WSI_Repositories class contains a list of used repositories
 *
 * @author ubruhin
 * @date 2016-08-10
 */
class WSI_Repositories final : public WSI_Base
{
        Q_OBJECT

    public:

        // Constructors / Destructor
        WSI_Repositories() = delete;
        WSI_Repositories(const WSI_Repositories& other) = delete;
        explicit WSI_Repositories(const SExpression& node);
        ~WSI_Repositories() noexcept;

        // Getters
        const QList<const Repository*>& getRepositories() const noexcept {return mList;}

        // Getters: Widgets
        QWidget* getWidget() const noexcept {return mWidget.data();}

        // General Methods
        void restoreDefault() noexcept override;
        void apply() noexcept override;
        void revert() noexcept override;

        /// @copydoc librepcb::SerializableObject::serialize()
        void serialize(SExpression& root) const override;

        // Operator Overloadings
        WSI_Repositories& operator=(const WSI_Repositories& rhs) = delete;


    private: // Methods
        void btnUpClicked() noexcept;
        void btnDownClicked() noexcept;
        void btnAddClicked() noexcept;
        void btnRemoveClicked() noexcept;
        void updateListWidgetItems() noexcept;


    private: // Data

        /**
         * @brief The list of repositories in the right order
         *
         * The repository with the highest priority is at index 0 of the list. In case of
         * version conflicts, the repository with the higher priority will be used.
         */
        QList<const Repository*> mList;
        QList<const Repository*> mListTmp;

        // Widgets
        QScopedPointer<QWidget> mWidget;
        QScopedPointer<QListWidget> mListWidget;
        QScopedPointer<QLineEdit> mLineEdit;
        QScopedPointer<QToolButton> mBtnUp;
        QScopedPointer<QToolButton> mBtnDown;
        QScopedPointer<QToolButton> mBtnAdd;
        QScopedPointer<QToolButton> mBtnRemove;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace workspace
} // namespace librepcb

#endif // LIBREPCB_WORKSPACE_WSI_REPOSITORIES_H
