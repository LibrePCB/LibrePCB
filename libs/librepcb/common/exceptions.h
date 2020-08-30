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

#ifndef LIBREPCB_EXCEPTIONS_H
#define LIBREPCB_EXCEPTIONS_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class FilePath;

/*******************************************************************************
 *  Class Exception
 ******************************************************************************/

/**
 * @brief The Exception class
 *
 * This is an exception base class. It inherits from the QException class, which
 * implements an exception class which can be transferred accross threads.
 * QException inherits from std::exception. There are several subclasses of the
 * class Exception, see inheritance diagram.
 *
 * @note Exceptions must always be thrown by value and caught by (const)
 * reference!
 *
 * For more information about the QException class read the Qt documentation.
 *
 * Each Exception object holds following attributes:
 *  - a message (#mMsg): error message in the user's language (can be printed
 * directly to a QMessageBox or similar)
 *  - the filename of the source file where the exception was thrown (#mFile)
 *  - the line number where the exception was thrown (#mLine)
 *
 * @note Every exception will automatically print a debug message (see
 * librepcb::Debug) of type librepcb::Debug::DebugLevel_t::Exception.
 *
 * Example how to use exceptions:
 *
 * @code
 * void foo(int i) {
 *   if (i < 0) {
 *     throw Exception(__FILE__, __LINE__, tr("Invalid argument: %1").arg(i));
 *   }
 * }
 *
 * void bar() noexcept {
 *   try {
 *     foo(-5);
 *   } catch (const Exception& e) {
 *     QMessageBox::critical(0, tr("Error"), e.getMsg());
 *   }
 * }
 * @endcode
 *
 * @warning Please read the "Exception Safety" notes from the Qt Project
 * documentation before writing source code which throws exceptions, there are
 * some important things to know:
 * http://qt-project.org/doc/qt-5/exceptionsafety.html
 */
class Exception : public QException {
public:
  // Constructors / Destructor

  /**
   * @brief The default constructor
   */
  Exception() = delete;

  /**
   * @brief The copy constructor (needed for #clone())
   */
  Exception(const Exception& other) noexcept;

  /**
   * @brief The constructor which is used to throw an exception
   *
   * @param file      The source file where the exception was thrown (use
   * __FILE__)
   * @param line      The line number where the exception was thrown (use
   * __LINE__)
   * @param msg       An error message in the user's language (use
   * QObject::tr()). This message can be used in message boxes.
   */
  Exception(const char* file, int line,
            const QString& msg = QString("Exception")) noexcept;

  /**
   * @brief The destructor
   */
  virtual ~Exception() noexcept {}

  // Getters

  /**
   * @brief Get the error message (translated)
   *
   * @return The error message in the user's language
   */
  const QString& getMsg() const { return mMsg; }

  /**
   * @brief Get the source file where the exception was thrown
   *
   * @return The filename
   */
  const QString& getFile() const { return mFile; }

  /**
   * @brief Get the line number where the exception was thrown
   *
   * @return The line number
   */
  int getLine() const { return mLine; }

  /**
   * @brief reimplemented from std::exception::what()
   *
   * @warning This method is only for compatibility reasons with the base class
   *          std::exception. Normally, you should not use this method.
   *          Use #getMsg() instead.
   *
   * @note The returned pointer is valid as long as the exception object exists
   *
   * @return the error message as a C-string (const char*) in the local encoding
   */
  const char* what() const noexcept override;

  // Inherited from QException (see QException documentation for more details)
  virtual void       raise() const override { throw *this; }
  virtual Exception* clone() const override { return new Exception(*this); }

private:
  // Attributes
  QString mMsg;   ///< the error message (translated)
  QString mFile;  ///< the source filename where the exception was thrown
  int     mLine;  ///< the line number where the exception was thrown

  // Cached Attributes
  mutable QByteArray mMsgUtf8;  ///< the message as an UTF8 byte array
};

/*******************************************************************************
 *  Class LogicError
 ******************************************************************************/

/**
 * @brief The LogicError class
 *
 * This exception class is used for exceptions related to the internal logic of
 * the program (a thrown LogicError means that there is a bug in the source
 * code).
 *
 * @see Exception
 */
class LogicError final : public Exception {
public:
  /**
   * @brief Default Constructor
   */
  LogicError() = delete;

  /**
   * @copydoc Exception::Exception
   */
  LogicError(const char* file, int line,
             const QString& msg = QString("Logic Error")) noexcept;

  /**
   * @brief The copy constructor (needed for #clone())
   */
  LogicError(const LogicError& other) noexcept;

  // Inherited from Exception
  virtual void        raise() const override { throw *this; }
  virtual LogicError* clone() const override { return new LogicError(*this); }
};

/*******************************************************************************
 *  Class RuntimeError
 ******************************************************************************/

/**
 * @brief The RuntimeError class
 *
 * This exception class is used for exceptions detected during runtime, but are
 * not produced because of bugs in the source code. For example if you want to
 * write to a file but the user has no write permissions, this can be a runtime
 * error.
 *
 * @see Exception
 */
class RuntimeError : public Exception {
public:
  /**
   * @brief Default Constructor
   */
  RuntimeError() = delete;

  /**
   * @copydoc Exception::Exception
   */
  RuntimeError(const char* file, int line,
               const QString& msg = QString("Runtime Error")) noexcept;

  /**
   * @brief The copy constructor (needed for #clone())
   */
  RuntimeError(const RuntimeError& other) noexcept;

  /**
   * @brief Destructor
   */
  virtual ~RuntimeError() noexcept {}

  // Inherited from Exception
  virtual void          raise() const override { throw *this; }
  virtual RuntimeError* clone() const override {
    return new RuntimeError(*this);
  }
};

/*******************************************************************************
 *  Class RangeError
 ******************************************************************************/

/**
 * @brief The RangeError class
 *
 * This exception class is used for range under-/overflows.
 *
 * @see Exception
 */
class RangeError final : public RuntimeError {
public:
  /**
   * @brief Default Constructor
   */
  RangeError() = delete;

  /**
   * @copydoc Exception::Exception
   */
  RangeError(const char* file, int line,
             const QString& msg = QString("Range Error")) noexcept;

  /**
   * Constructor which produces a message like "42 not in range [13..37]"
   *
   * @param file                  See ::librepcb::Exception::Exception()
   * @param line                  See ::librepcb::Exception::Exception()
   * @param value                 The invalid value
   * @param min                   The lower value limit
   * @param max                   The upper value limit
   */
  template <typename Tval, typename Tmin, typename Tmax>
  RangeError(const char* file, int line, const Tval& value, const Tmin& min,
             const Tmax& max) noexcept
    : RangeError(file, line,
                 QString("Range error: %1 not in [%2..%3]")
                     .arg(value)
                     .arg(min)
                     .arg(max)) {}

  /**
   * @brief The copy constructor (needed for #clone())
   */
  RangeError(const RangeError& other) noexcept;

  // Inherited from RuntimeError
  virtual void        raise() const override { throw *this; }
  virtual RangeError* clone() const override { return new RangeError(*this); }
};

/*******************************************************************************
 *  Class FileParseError
 ******************************************************************************/

/**
 * @brief The FileParseError class
 *
 * This exception class is used for errors while parsing files due to invalid
 * file content (for example invalid syntax).
 *
 * @see Exception
 */
class FileParseError final : public RuntimeError {
public:
  /**
   * @brief Default Constructor
   */
  FileParseError() = delete;

  /**
   * @brief The constructor which is used to throw an exception
   *
   * @param file                  See ::librepcb::Exception::Exception()
   * @param line                  See ::librepcb::Exception::Exception()
   * @param filePath              The path to the parsed file (optional)
   * @param fileLine              The line number of the parse error (-1 if
   * unknown)
   * @param fileColumn            The column of the parse error (-1 if unknown)
   * @param invalidFileContent    The parsed string which is invalid (optional)
   * @param msg                   See Exception#Exception()
   */
  FileParseError(const char* file, int line, const FilePath& filePath,
                 int fileLine = -1, int fileColumn = -1,
                 const QString& invalidFileContent = QString(),
                 const QString& msg = QString("File Parse Error")) noexcept;

  /**
   * @brief The copy constructor (needed for #clone())
   */
  FileParseError(const FileParseError& other) noexcept;

  // Inherited from RuntimeError
  virtual void            raise() const override { throw *this; }
  virtual FileParseError* clone() const override {
    return new FileParseError(*this);
  }
};

/*******************************************************************************
 *  Class UserCanceled
 ******************************************************************************/

/**
 * @brief The UserCanceled class
 *
 * This exception class is used to interrupt an action which was canceled by the
 * user. This type of exception is useful if the exception catcher do not need
 * to show a message box with the error message. For example, if a project is
 * opened, the project's constructor will throw an exception in case of an
 * error. Then the caller (the catcher of the exception) will show a message box
 * with the error message. But the constructor can also throw an exception if
 * the user has canceled opening the project (for example in a message box
 * "restore project?" --> YES|NO|CANCEL). But then the error message box do not
 * need to appear! So we can throw an exception of type "UserCanceled" to
 * indicate that this was a user's decision and the catcher will not show an
 * error message box.
 *
 * @note    Normally, a UserCanceled exception do not need the attribute #mMsg,
 * so you do not need to pass the parameter "msg" to the constructor. This is
 * because such an exception will never produce a message box with the error
 * message (as there is not really an error - the user has simply canceled
 * something).
 *
 * @see Exception
 */
class UserCanceled final : public Exception {
public:
  /**
   * @brief Default Constructor
   */
  UserCanceled() = delete;

  /**
   * @copydoc Exception::Exception
   */
  UserCanceled(const char* file, int line,
               const QString& msg = QString("User Canceled")) noexcept;

  /**
   * @brief The copy constructor (needed for #clone())
   */
  UserCanceled(const UserCanceled& other) noexcept;

  // Inherited from Exception
  virtual void          raise() const override { throw *this; }
  virtual UserCanceled* clone() const override {
    return new UserCanceled(*this);
  }
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif  // LIBREPCB_EXCEPTIONS_H
