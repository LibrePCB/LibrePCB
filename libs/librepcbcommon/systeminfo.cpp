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

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include <QHostInfo>
#include "systeminfo.h"

#if (defined(Q_OS_UNIX) || defined(Q_OS_LINUX)) && (!defined(Q_OS_MACX)) // For UNIX and Linux
#include <unistd.h>
#include <pwd.h>
#endif

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {

/*****************************************************************************************
 *  Static Methods
 ****************************************************************************************/

QString SystemInfo::getUsername() noexcept
{
    QString username("");

    // this line should work for most UNIX, Linux, Mac and Windows systems
    username = QString(qgetenv("USERNAME")).trimmed();

    // if the environment variable "USERNAME" is not set, we will try "USER"
    if (username.isEmpty())
        username = QString(qgetenv("USER")).trimmed();

    if (username.isEmpty())
        qWarning() << "Could not determine the system's username!";

    return username;
}

QString SystemInfo::getFullUsername() noexcept
{
    QString username("");

#if (defined(Q_OS_UNIX) || defined(Q_OS_LINUX)) && (!defined(Q_OS_MACX)) // For UNIX and Linux
    passwd* userinfo = getpwuid(getuid());
    if (userinfo == NULL) {
        qWarning() << "Could not fetch user info via getpwuid!";
    } else {
        QString gecosString = QString::fromLocal8Bit(userinfo->pw_gecos);
        QStringList gecosParts = gecosString.split(',', QString::SkipEmptyParts);
        if (gecosParts.size() >= 1) {
            username = gecosParts.at(0);
        }
    }
#elif defined(Q_OS_MACX) // For Mac OS X
    QString command("finger `whoami` | awk -F: '{ print $3 }' | head -n1 | sed 's/^ //'");
    QProcess process;
    process.start("sh", QStringList() << "-c" << command);
    process.waitForFinished(500);
    username = QString(process.readAllStandardOutput()).remove("\n").remove("\r").trimmed();
#elif defined(Q_OS_WIN) // For Windows
    // TODO
#else
#error Unknown operating system!
#endif

    if (username.isEmpty())
        qWarning() << "Could not determine the system's full username!";

    return username;
}

QString SystemInfo::getHostname() noexcept
{
    QString hostname = QHostInfo::localHostName();

    if (hostname.isEmpty())
        qWarning() << "Could not determine the system's hostname!";

    return hostname;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace librepcb
