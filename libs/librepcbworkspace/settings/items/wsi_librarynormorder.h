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

#ifndef LIBREPCB_WSI_LIBRARYNORMORDER_H
#define LIBREPCB_WSI_LIBRARYNORMORDER_H

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
 *  Class WSI_LibraryNormOrder
 ****************************************************************************************/

/**
 * @brief The WSI_LibraryNormOrder class contains a list of norms which should be used
 *        for all library elements (in the specified order)
 *
 * @author ubruhin
 * @date 2014-11-01
 */
class WSI_LibraryNormOrder final : public WSI_Base
{
        Q_OBJECT

    public:

        // Constructors / Destructor
        explicit WSI_LibraryNormOrder(WorkspaceSettings& settings);
        ~WSI_LibraryNormOrder();

        // Getters
        const QStringList& getNormOrder() const {return mList;}

        // Getters: Widgets
        QString getLabelText() const {return tr("Preferred Norms:\n(Highest priority at top)");}
        QWidget* getWidget() const {return mWidget;}

        // General Methods
        void restoreDefault();
        void apply();
        void revert();


    public slots:

        // Public Slots
        void btnUpClicked();
        void btnDownClicked();
        void btnAddClicked();
        void btnRemoveClicked();


    private:

        // make some methods inaccessible...
        WSI_LibraryNormOrder();
        WSI_LibraryNormOrder(const WSI_LibraryNormOrder& other);
        WSI_LibraryNormOrder& operator=(const WSI_LibraryNormOrder& rhs);


        // Private Methods
        void updateListWidgetItems();


        // General Attributes

        /**
         * @brief The list of norms (like "DIN EN 81346") in the right order
         *
         * The norm which should be used first is at index 0 of the list.
         */
        QStringList mList;
        QStringList mListTmp;

        // Widgets
        QWidget* mWidget;
        QListWidget* mListWidget;
        QComboBox* mComboBox;
        QToolButton* mBtnUp;
        QToolButton* mBtnDown;
        QToolButton* mBtnAdd;
        QToolButton* mBtnRemove;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace workspace
} // namespace librepcb

#endif // LIBREPCB_WSI_LIBRARYNORMORDER_H
