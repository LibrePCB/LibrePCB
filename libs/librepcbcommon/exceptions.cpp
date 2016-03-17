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
#include "exceptions.h"
#include "debug.h"
#include "fileio/filepath.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {

/*****************************************************************************************
 *  Class Exception
 ****************************************************************************************/

Exception::Exception(const char* file, int line, const QString& debugMsg,
                     const QString& userMsg) noexcept :
    mDebugMsg(debugMsg), mUserMsg(userMsg), mFile(file), mLine(line)
{
    // print out the exception to the console/log output
    Debug::instance()->print(Debug::DebugLevel_t::Exception,
                             QString("%1 {%2}").arg(mUserMsg, mDebugMsg), file, line);
}

Exception::Exception(const Exception& other) noexcept :
    mDebugMsg(other.mDebugMsg), mUserMsg(other.mUserMsg), mFile(other.mFile),
    mLine(other.mLine)
{
}

const char* Exception::what() const noexcept
{
    if (mUserMsgUtf8.isNull()) {
        mUserMsgUtf8 = mUserMsg.toUtf8();
    }
    return mUserMsgUtf8.constData();
}

/*****************************************************************************************
 *  Class LogicError
 ****************************************************************************************/

LogicError::LogicError(const char* file, int line, const QString& debugMsg,
                       const QString& userMsg) noexcept :
    Exception(file, line, debugMsg, userMsg)
{
}

LogicError::LogicError(const LogicError& other) noexcept :
    Exception(other)
{
}

/*****************************************************************************************
 *  Class RuntimeError
 ****************************************************************************************/

RuntimeError::RuntimeError(const char* file, int line, const QString& debugMsg,
                           const QString& userMsg) noexcept :
    Exception(file, line, debugMsg, userMsg)
{
}

RuntimeError::RuntimeError(const RuntimeError& other) noexcept :
    Exception(other)
{
}

/*****************************************************************************************
 *  Class RangeError
 ****************************************************************************************/

RangeError::RangeError(const char* file, int line, const QString& debugMsg,
                       const QString& userMsg) noexcept :
    RuntimeError(file, line, debugMsg, userMsg)
{
}

RangeError::RangeError(const RangeError& other) noexcept :
    RuntimeError(other)
{
}

/*****************************************************************************************
 *  Class FileParseError
 ****************************************************************************************/

FileParseError::FileParseError(const char* file, int line, const FilePath& filePath,
                               int fileLine, int fileColumn,
                               const QString& invalidFileContent, const QString& userMsg) noexcept :
    RuntimeError(file, line, invalidFileContent,
        QString("File parse error: %1\n\nFile: %2\nLine,Column: %3,%4\nInvalid Content: \"%5\"")
        .arg(userMsg).arg(filePath.toNative()).arg(fileLine).arg(fileColumn).arg(invalidFileContent))
{
}

FileParseError::FileParseError(const FileParseError& other) noexcept :
    RuntimeError(other)
{
}

/*****************************************************************************************
 *  Class UserCanceled
 ****************************************************************************************/

UserCanceled::UserCanceled(const char* file, int line, const QString& debugMsg,
                           const QString& userMsg) noexcept :
    Exception(file, line, debugMsg, userMsg)
{
}

UserCanceled::UserCanceled(const UserCanceled& other) noexcept :
    Exception(other)
{
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace librepcb
