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
#include "exceptions.h"
#include "debug.h"

/*****************************************************************************************
 *  Class Exception
 ****************************************************************************************/

Exception::Exception(const QString& msg, const char* file, int line) :
    mMsg(msg), mFile(file), mLine(line)
{
    // the filename and line number will be added in the Debug class, not here!
    Debug::instance()->print(Debug::Exception, msg, file, line);
}

QString Exception::getDebugString() const
{
    return QString("%1 (thrown at %2:%3)").arg(mMsg, mFile).arg(mLine);
}

const char* Exception::what() const noexcept
{
    static QByteArray utf8string;
    utf8string = mMsg.toLocal8Bit();
    return utf8string.constData();
}

/*****************************************************************************************
 *  Class LogicError
 ****************************************************************************************/

LogicError::LogicError(const QString& msg, const char* file, int line) :
    Exception(msg, file, line)
{
}

/*****************************************************************************************
 *  Class RuntimeError
 ****************************************************************************************/

RuntimeError::RuntimeError(const QString& msg, const char* file, int line) :
    Exception(msg, file, line)
{
}

/*****************************************************************************************
 *  Class RangeError
 ****************************************************************************************/

RangeError::RangeError(const QString& msg, const char* file, int line) :
    Exception(msg, file, line)
{
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/
