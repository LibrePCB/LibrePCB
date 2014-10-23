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

#ifndef EXCEPTIONS_H
#define EXCEPTIONS_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>

/*****************************************************************************************
 *  Class Exception
 ****************************************************************************************/

/**
 * @brief The Exception class
 *
 * This is an exception base class. It inherits from the QException class, which implements
 * an exception class which can be transferred accross threads. QException inherits from
 * std::exception. There are several subclasses of the class Exception, see inheritance
 * diagram.
 *
 * @note Exceptions must always be thrown by value and caught by reference!
 *
 * For more information about the QException class read the Qt documentation.
 *
 * Each Exception object holds following attributes:
 *  - a debug message (#mDebugMsg): contains debugging information (in english, do not
 *    need to be a complete sentence, could consists only of values of variables)
 *  - a user message (#mUserMsg): error message in the user's language (can be printed
 *    directly to a QMessageBox or similar)
 *  - the filename of the source file where the exception was thrown (#mFile)
 *  - the number of the line where the exception was thrown (#mLine)
 *
 * @note Every exception will automatically print a debug message (see #Debug) of type
 *       Debug#Exception (see Debug#DebugLevel).
 *
 * Example how to use exceptions:
 *
 * @code
 * void foo(int i)
 * {
 *     if (i < 0)
 *         throw Exception(__FILE__, __LINE__, QString("i=%1").arg(i), tr("Invalid argument!"));
 * }
 *
 * void bar()
 * {
 *     try
 *     {
 *         foo(-5);
 *     }
 *     catch (Exception& e)
 *     {
 *         QMessageBox::critical(0, tr("Error"), e.getUserMsg());
 *     }
 * }
 * @endcode
 *
 * @warning Please read the "Exception Safety" notes from the Qt Project documentation
 * before writing source code which throws exceptions! There are some important things
 * to know! http://qt-project.org/doc/qt-5/exceptionsafety.html
 */
class Exception : public QException
{
    public:

        // Constructor

        /**
         * @brief The constructor which is used to throw an exception
         *
         * @param file      The source file where the exception was thrown (use __FILE__)
         * @param line      The line number where the exception was thrown (use __LINE__)
         * @param debugMsg  Debugging information which will be written only to the debug
         *                  log (see class Debug). The user will never see this
         *                  information, so it should be always in english!
         * @param userMsg   An error message in the user's language (use QObject::tr()!).
         *                  This message can be used in message boxes. Do not include too
         *                  much technical information about the exception here, use the
         *                  parameter debugMsg instead.
         */
        Exception(const char* file, int line, const QString& debugMsg = QString(),
                  const QString& userMsg = QString("Exception"));

        /**
         * @brief The copy constructor (needed for #clone())
         */
        Exception(const Exception& other);


        // Getters

        /**
         * @brief Get the debug error message (always in english)
         * @return The debug error message / technical information about the exception
         */
        const QString&  getDebugMsg()   const {return mDebugMsg;}

        /**
         * @brief Get the user error message (translated)
         * @return The user error message in the user's language
         */
        const QString&  getUserMsg()    const {return mUserMsg;}

        /**
         * @brief Get the source file where the exception was thrown
         * @return The filename
         */
        const QString&  getFile()       const {return mFile;}

        /**
         * @brief Get the line number where the exception was thrown
         * @return The line number
         */
        int             getLine()       const {return mLine;}

        /**
         * @brief reimplemented from std::exception::what()
         *
         * @warning This method is only for compatibility reasons with the base class
         *          std::exception. Normally, you should not use this method. Use
         *          #getDebugMsg() or #getUserMsg() instead!
         *
         * @return the user error message as a C-string (const char*) in the local encoding
         */
        const char* what() const noexcept override;


        // Inherited from QException (see QException documentation for more details)
        virtual void raise() const {throw *this;}
        virtual Exception* clone() const {return new Exception(*this);}

    protected:

        // Attributes
        QString mDebugMsg;  ///< the debug message (in english)
        QString mUserMsg;   ///< the user message (translated)
        QString mFile;      ///< the source filename where the exception was thrown
        int mLine;          ///< the line number where the exception was thrown

    private:

        /**
         * @brief The default constructor
         */
        Exception();
};

/*****************************************************************************************
 *  Class LogicError
 ****************************************************************************************/

/**
 * @brief The LogicError class
 *
 * This exception class is used for exceptions related to the internal logic of the
 * program (a throwed LogicError means that there is a bug in the source code!).
 *
 * @see Exception
 */
class LogicError : public Exception
{
    public:

        /**
         * @copydoc Exception::Exception
         */
        LogicError(const char* file, int line, const QString& debugMsg = QString(),
                   const QString& userMsg = QString("Logic Error"));

        /**
         * @brief The copy constructor (needed for #clone())
         */
        LogicError(const LogicError& other);

        // Inherited from Exception
        virtual void raise() const {throw *this;}
        virtual LogicError* clone() const {return new LogicError(*this);}

    private:

        /// @brief make the default constructor inaccessible
        LogicError();
};

/*****************************************************************************************
 *  Class RuntimeError
 ****************************************************************************************/

/**
 * @brief The RuntimeError class
 *
 * This exception class is used for exceptions detected during runtime, but are not
 * produced because of bugs in the source code. For example if you want to write to a file
 * but the user has no write permissions, this can be a runtime error.
 *
 * @see Exception
 */
class RuntimeError : public Exception
{
    public:

        /**
         * @copydoc Exception::Exception
         */
        RuntimeError(const char* file, int line, const QString& debugMsg = QString(),
                     const QString& userMsg = QString("Runtime Error"));

        /**
         * @brief The copy constructor (needed for #clone())
         */
        RuntimeError(const RuntimeError& other);

        // Inherited from Exception
        virtual void raise() const {throw *this;}
        virtual RuntimeError* clone() const {return new RuntimeError(*this);}

    private:

        /// @brief make the default constructor inaccessible
        RuntimeError();
};

/*****************************************************************************************
 *  Class RangeError
 ****************************************************************************************/

/**
 * @brief The RangeError class
 *
 * This exception class is used for range under-/overflows, for example in units.cpp.
 *
 * @see Exception
 */
class RangeError : public Exception
{
    public:

        /**
         * @copydoc Exception::Exception
         */
        RangeError(const char* file, int line, const QString& debugMsg = QString(),
                   const QString& userMsg = QString("Range Error"));

        /**
         * @brief The copy constructor (needed for #clone())
         */
        RangeError(const RangeError& other);

        // Inherited from Exception
        virtual void raise() const {throw *this;}
        virtual RangeError* clone() const {return new RangeError(*this);}

    private:

        /// @brief make the default constructor inaccessible
        RangeError();
};

/*****************************************************************************************
 *  Class UserCanceled
 ****************************************************************************************/

/**
 * @brief The UserCanceled class
 *
 * This exception class is used to interrupt an action which was canceled by the user.
 * This type of exception is useful if the exception catcher do not need to show a
 * message box with the error message. For example, if a project is opened, the project's
 * constructor will throw an exception in case of an error. Then the caller (the catcher
 * of the exception) will show a message box with the error message. But the constructor
 * can also throw an exception if the user has canceled opening the project (for example
 * in a message box "restore project?" --> YES|NO|CANCEL). But then the error message box
 * do not need to appear! So we can throw an exception of type "UserCanceled" to indicate
 * that this was a user's decision and the catcher will not show an error message box.
 *
 * @note    Normally, a UserCanceled exception do not need the attribute #mUserMsg, so you
 *          do not need to pass the parameter "userMsg" to the constructor. This is because
 *          such an exception will never produce a message box with the error message
 *          (as there is not really an error - the user has simply canceled something).
 *
 * @see Exception
 */
class UserCanceled : public Exception
{
    public:

        /**
         * @copydoc Exception::Exception
         */
        UserCanceled(const char* file, int line, const QString& debugMsg = QString(),
                   const QString& userMsg = QString("User Canceled"));

        /**
         * @brief The copy constructor (needed for #clone())
         */
        UserCanceled(const UserCanceled& other);

        // Inherited from Exception
        virtual void raise() const {throw *this;}
        virtual UserCanceled* clone() const {return new UserCanceled(*this);}

    private:

        /// @brief make the default constructor inaccessible
        UserCanceled();
};

#endif // EXCEPTIONS_H
