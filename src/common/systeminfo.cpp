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

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>
#include <QHostInfo>
#include "systeminfo.h"
#include "exceptions.h"

/*****************************************************************************************
 *  Static Methods
 ****************************************************************************************/

QString SystemInfo::getUsername()
{
    QString username("");

    // this line should work for most UNIX, Linux, Mac and Windows systems
    username = QString(qgetenv("USERNAME")).remove("\n").trimmed();

    // if the environment variable "USERNAME" is not set, we will try "USER"
    if (username.isEmpty())
        username = QString(qgetenv("USER")).remove("\n").trimmed();

    if (username.isEmpty())
        qWarning() << "Could not determine the system's username!";

    return username;
}

QString SystemInfo::getFullUsername()
{
    QString username("");

#if (defined(Q_OS_UNIX) || defined(Q_OS_LINUX)) && (!defined(Q_OS_MACX)) // For UNIX and Linux
    QProcess process;
    QString command("grep \"^$USER:\" /etc/passwd | awk -F: '{print $5}'");
    process.start("sh", QStringList() << "-c" << command);
    process.waitForFinished(1000);
    username = QString(process.readAllStandardOutput()).remove("\n").trimmed();
#elif defined(Q_OS_MACX) // For Mac OS X
    // TODO
#elif defined(Q_OS_WIN) // For Windows
    // TODO
#else
    throw LogicError("Unknown operating system!", __FILE__, __LINE__);
#endif

    if (username.isEmpty())
        qWarning() << "Could not determine the system's full username!";

    return username;
}

QString SystemInfo::getHostname()
{
    QString hostname = QHostInfo::localHostName();

    if (hostname.isEmpty())
        qWarning() << "Could not determine the system's hostname!";

    return hostname;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/
