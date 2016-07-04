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

#ifndef LIBREPCB_APPLICATION_H
#define LIBREPCB_APPLICATION_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include <QApplication>
#include "version.h"

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {

/*****************************************************************************************
 *  Class Application
 ****************************************************************************************/

/**
 * @brief The Application class extends the QApplication with the exception-save method
 *        #notify()
 *
 * @author ubruhin
 * @date 2014-10-23
 */
class Application final : public QApplication
{
        Q_OBJECT

    public:

        // Constructors / Destructor
        Application(int& argc, char** argv);
        ~Application();

        // Reimplemented from QApplication
        bool notify(QObject* receiver, QEvent* e);

        // Static Methods
        static void setApplicationVersion(const Version& version) noexcept;
        static Version applicationVersion() noexcept;
        static int majorVersion() noexcept {return applicationVersion().getNumbers().first();}


    private:

        // make some methods inaccessible...
        Application();
        Application(const Application& other);
        Application& operator=(const Application& rhs);
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace librepcb

#endif // LIBREPCB_APPLICATION_H
