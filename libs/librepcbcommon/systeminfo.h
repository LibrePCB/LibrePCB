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

#ifndef LIBREPCB_SYSTEMINFO_H
#define LIBREPCB_SYSTEMINFO_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {

/*****************************************************************************************
 *  Class SystemInfo
 ****************************************************************************************/

/**
 * @brief This class provides some methods to get information from the operating system
 *
 * For example, this class is used to get the name of the user which is logged in and the
 * hostname of the computer to create a lock file (see class FileLock).
 *
 * @note Only static methods are available. You cannot create objects of this class.
 *
 * @author ubruhin
 * @date 2014-07-28
 */
class SystemInfo final
{
    public:

        /**
         * @brief Get the name of the user which is logged in (like "homer")
         *
         * @return The username (in case of an error, this string can be empty!)
         *
         * @todo test this method on windows and mac!
         */
        static QString getUsername() noexcept;

        /**
         * @brief Get the full name of the user which is logged in (like "Homer Simpson")
         *
         * @return The full user name (in case of an error, this string can be empty!)
         *
         * @todo complete this method for windows systems!
         */
        static QString getFullUsername() noexcept;

        /**
         * @brief Get the hostname of the computer (like "homer-workstation")
         *
         * @return The hostname (in case of an error, this string can be empty!)
         *
         * @todo test this method on windows and mac!
         */
        static QString getHostname() noexcept;


    private:

        SystemInfo(); ///< make the default constructor inaccessible
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace librepcb

#endif // LIBREPCB_SYSTEMINFO_H
