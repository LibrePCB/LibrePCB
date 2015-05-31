/*
 * EDA4U - Professional EDA for everyone!
 * Copyright (C) 2013 Urban Bruhin
 * http://eda4u.ubruhin.ch/
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

#ifndef WSI_APPLOCALE_H
#define WSI_APPLOCALE_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include "wsi_base.h"

/*****************************************************************************************
 *  Class WSI_AppLocale
 ****************************************************************************************/

/**
 * @brief The WSI_AppLocale class represents the application's locale settings
 *        (for translation and localization)
 *
 * @author ubruhin
 * @date 2014-10-04
 */
class WSI_AppLocale final : public WSI_Base
{
        Q_OBJECT

    public:

        // Constructors / Destructor
        explicit WSI_AppLocale(WorkspaceSettings& settings);
        ~WSI_AppLocale();

        // Getters
        const QString& getAppLocaleName() const {return mAppLocale;}

        // Getters: Widgets
        QString getLabelText() const {return tr("Application Language:");}
        QWidget* getWidget() const {return mWidget;}

        // General Methods
        void restoreDefault();
        void apply();
        void revert();


    public slots:

        // Public Slots
        void comboBoxIndexChanged(int index);


    private:

        // make some methods inaccessible...
        WSI_AppLocale();
        WSI_AppLocale(const WSI_AppLocale& other);
        WSI_AppLocale& operator=(const WSI_AppLocale& rhs);


        // Private Methods
        void updateComboBoxIndex();


        // General Attributes

        /**
         * @brief The locale name
         *
         * Examples:
         *  - QString("de_CH") for German/Switzerland
         *  - QString("") or QString() means "use system locale"
         *
         * Default: QString()
         */
        QString mAppLocale;
        QString mAppLocaleTmp;

        QList<QTranslator*> mInstalledTranslators; ///< see constructor/destructor code

        // Widgets
        QWidget* mWidget;
        QComboBox* mComboBox;
};

#endif // WSI_APPLOCALE_H
