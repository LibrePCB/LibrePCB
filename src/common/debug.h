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

#ifndef DEBUG_H
#define DEBUG_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/

#include <QtCore>

/*****************************************************************************************
 *  Class Debug
 ****************************************************************************************/

/**
 * @brief The Debug class provides some methods for debugging/logging
 *
 * There will be created a singleton object of this class with Debug::instance().
 * The singleton will be created at the first call to Debug::instance(). The constructor
 * will register the method Debug::messageHandler() as the message handler function for
 * Qt's debug functions qDebug(), qWarning(), qCritical() and qFatal(). You can use
 * these functions in your source code, and the Debug class will handle these messages.
 *
 * This class can write messages to the stderr output and to a log file. You can set
 * seperate debug levels for both. By default, logging to a file is disabled.
 *
 * @author ubruhin
 * @date 2014-07-28
 */
class Debug final
{
    public:

        // Types

        /**
         * Enum for debug levels. Only messages of the current or a higher level are printed.
         */
        enum DebugLevel {
            Nothing = 0,    ///< 0: nothing
            Fatal,          ///< 1: fatal errors [qFatal()] --> this will quit the application!
            Critical,       ///< 2: errors [qCritical()]
            Warning,        ///< 3: warnings [qWarning()]
            DebugMsg,       ///< 4: irrelevant debug messages (a lot of messages!) [qDebug()]
            All,            ///< 5: all
        };


        // General methods

        /**
         * @brief Set the debug level for the stderr output.
         *
         * Only messages of the current or a higher level are printed.
         *
         * @param level     A value from Debug::DebugLevel (inclusive Nothing and All)
         */
        void setDebugLevelStderr(DebugLevel level);

        /**
         * @brief Set the debug level for the log file.
         *
         * Only messages of the current or a higher level are printed.
         *
         * @param level     A value from Debug::DebugLevel (inclusive Nothing and All)
         */
        void setDebugLevelLogFile(DebugLevel level);

        /**
         * @brief Get the current debug level for the stderr output.
         *
         * @return The current debug level
         */
        DebugLevel getDebugLevelStderr() const;

        /**
         * @brief Get the current debug level for the log file.
         *
         * @return The current debug level
         */
        DebugLevel getDebugLevelLogFile() const;

        /**
         * @brief Get the filename of the log file (even if file logging is disabled)
         *
         * @return The filename of the log file (the file may do not exist)
         */
        const QString& getLogFilename() const;

        /**
         * @brief Print a message to stderr/logfile (with respect to the current debug level)
         *
         * @note    You can use this method directly, but as the functions qDebug(), qWarning()
         *          (and so on) are more flexible and do not need the file and line arguments,
         *          it's easier to use the Qt's debug funtions instead of this method.
         *
         * @param level     The debug level of the message (DO NOT USE "Nothing" and "All"!)
         * @param msg       The message
         * @param file      The source file (use the macro __FILE__)
         * @param line      The line number (use the macro __LINE__)
         */
        void print(DebugLevel level, const QString& msg, const char* file, int line);


        // Static methods

        /**
         * @brief Get a pointer to the instance of the singleton Debug object
         *
         * @warning The singleton object will be created when calling this method the
         *          first time. You mustn't call this method before setting the
         *          application's organization and name, as this method will use
         *          these values to determine a filename for the log file!
         *
         * @return A pointer to the singleton object
         */
        static Debug* instance() {static Debug dbg; return &dbg;}

    private:

        // make some methods inaccessible...
        Debug();
        Debug(const Debug& other);
        ~Debug();
        Debug& operator=(const Debug& rhs);


        /**
         * @brief The message handler for qDebug(), qWarning(), qCritical() and qFatal()
         *
         * This method is registered as the message handler for Qt's debug functions.
         * This is done by the function qInstallMessageHandler() in Debug::Debug().
         */
        static void messageHandler(QtMsgType type, const QMessageLogContext& context,
                                   const QString& msg);


        // General Attributes
        DebugLevel mDebugLevelStderr;   ///< the current debug level for the stderr output
        DebugLevel mDebugLevelLogFile;  ///< the current debug level for the log file
        QTextStream* mStderrStream;     ///< the stream to stderr
        QString mLogFilename;           ///< the filename for the log file
        QFile* mLogFile;                ///< NULL if file logging is disabled

};

#endif // DEBUG_H
