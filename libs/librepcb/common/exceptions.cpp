/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
 * https://librepcb.org/
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

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "exceptions.h"

#include "debug.h"
#include "fileio/filepath.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class Exception
 ******************************************************************************/

Exception::Exception(const char* file, int line, const QString& msg) noexcept
  : mMsg(msg), mFile(file), mLine(line) {
  // print out the exception to the console/log output
  Debug::instance()->print(Debug::DebugLevel_t::Exception, mMsg, file, line);
}

Exception::Exception(const Exception& other) noexcept
  : mMsg(other.mMsg), mFile(other.mFile), mLine(other.mLine) {
}

const char* Exception::what() const noexcept {
  if (mMsgUtf8.isNull()) {
    mMsgUtf8 = mMsg.toUtf8();
  }
  return mMsgUtf8.constData();
}

/*******************************************************************************
 *  Class LogicError
 ******************************************************************************/

LogicError::LogicError(const char* file, int line, const QString& msg) noexcept
  : Exception(file, line, msg) {
}

LogicError::LogicError(const LogicError& other) noexcept : Exception(other) {
}

/*******************************************************************************
 *  Class RuntimeError
 ******************************************************************************/

RuntimeError::RuntimeError(const char* file, int line,
                           const QString& msg) noexcept
  : Exception(file, line, msg) {
}

RuntimeError::RuntimeError(const RuntimeError& other) noexcept
  : Exception(other) {
}

/*******************************************************************************
 *  Class RangeError
 ******************************************************************************/

RangeError::RangeError(const char* file, int line, const QString& msg) noexcept
  : RuntimeError(file, line, msg) {
}

RangeError::RangeError(const RangeError& other) noexcept : RuntimeError(other) {
}

/*******************************************************************************
 *  Class FileParseError
 ******************************************************************************/

FileParseError::FileParseError(const char* file, int line,
                               const FilePath& filePath, int fileLine,
                               int            fileColumn,
                               const QString& invalidFileContent,
                               const QString& msg) noexcept
  : RuntimeError(file, line,
                 QString("File parse error: %1\n\nFile: %2\nLine,Column: "
                         "%3,%4\nInvalid Content: \"%5\"")
                     .arg(msg)
                     .arg(filePath.toNative())
                     .arg(fileLine)
                     .arg(fileColumn)
                     .arg(invalidFileContent)) {
}

FileParseError::FileParseError(const FileParseError& other) noexcept
  : RuntimeError(other) {
}

/*******************************************************************************
 *  Class UserCanceled
 ******************************************************************************/

UserCanceled::UserCanceled(const char* file, int line,
                           const QString& msg) noexcept
  : Exception(file, line, msg) {
}

UserCanceled::UserCanceled(const UserCanceled& other) noexcept
  : Exception(other) {
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
