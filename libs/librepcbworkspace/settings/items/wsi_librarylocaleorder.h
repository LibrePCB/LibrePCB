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

#ifndef LIBREPCB_WSI_LIBRARYLOCALEORDER_H
#define LIBREPCB_WSI_LIBRARYLOCALEORDER_H

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
 *  Class WSI_LibraryLocaleOrder
 ****************************************************************************************/

/**
 * @brief The WSI_LibraryLocaleOrder class contains a list of locales which should be used
 *        for all (translatable) strings in library elements (in the specified order)
 *
 * @author ubruhin
 * @date 2014-10-05
 */
class WSI_LibraryLocaleOrder final : public WSI_Base
{
        Q_OBJECT

    public:

        // Constructors / Destructor
        explicit WSI_LibraryLocaleOrder(WorkspaceSettings& settings);
        ~WSI_LibraryLocaleOrder();

        // Getters
        const QStringList& getLocaleOrder() const {return mList;}

        // Getters: Widgets
        QString getLabelText() const {return tr("Preferred Languages:\n(Highest priority at top)");}
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
        WSI_LibraryLocaleOrder();
        WSI_LibraryLocaleOrder(const WSI_LibraryLocaleOrder& other);
        WSI_LibraryLocaleOrder& operator=(const WSI_LibraryLocaleOrder& rhs);


        // Private Methods
        void updateListWidgetItems();


        // General Attributes

        /**
         * @brief The list of locales (like "de_CH") in the right order
         *
         * The locale which should be used first is at index 0 of the list. If no
         * translation strings are found for all locales in this list, the fallback locale
         * "en_US" will be used automatically, so the list do not have to contain "en_US".
         * An empty list is also valid, then the fallback locale "en_US" will be used.
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

#endif // LIBREPCB_WSI_LIBRARYLOCALEORDER_H
