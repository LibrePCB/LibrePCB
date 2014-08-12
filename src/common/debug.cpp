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
#include "debug.h"
#include "filepath.h"

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

Debug::Debug() :
    mDebugLevelStderr(All), mDebugLevelLogFile(Nothing),
    mStderrStream(new QTextStream(stderr)), mLogFilepath(), mLogFile(0)
{
    // determine the filename of the log file which will be used if logging is enabled
    QString datetime = QDateTime::currentDateTime().toString("yyyy-MM-dd_hh-mm-ss");
    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::DataLocation);
    if (!dataDir.isEmpty())
        mLogFilepath.setPath(dataDir % "/logs/" % datetime % ".log");

    // install the message handler for Qt's debug functions (qDebug(), ...)
    qInstallMessageHandler(messageHandler);
}

Debug::~Debug()
{
    delete mStderrStream;
    mStderrStream = 0;

    if (mLogFile)
    {
        mLogFile->close();
        delete mLogFile;
        mLogFile = 0;
    }
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void Debug::setDebugLevelStderr(DebugLevel level)
{
    mDebugLevelStderr = level;
}

void Debug::setDebugLevelLogFile(DebugLevel level)
{
    if (level == mDebugLevelLogFile)
        return;

    if ((mDebugLevelLogFile == Nothing) && (level != Nothing))
    {
        // enable logging to file
        mLogFilepath.getParentDir().mkPath();
        mLogFile = new QFile(mLogFilepath.toStr());
        bool success = mLogFile->open(QFile::WriteOnly);
        if (success)
        {
            mDebugLevelLogFile = level; // activate logging to file immediately!
            qDebug() << "enabled logging to file" << mLogFilepath.toNative();
            qDebug() << "Qt version:" << qVersion();
        }
        else
        {
            qWarning() << "cannot enable logging to file" << mLogFilepath.toNative();
            qWarning() << "error message:" << mLogFile->errorString();
            delete mLogFile;
            mLogFile = 0;
        }
    }
    else if ((mDebugLevelLogFile != Nothing) && (level == Nothing) && (mLogFile))
    {
        // disable logging to file
        mLogFile->close();
        delete mLogFile;
        mLogFile = 0;
    }

    mDebugLevelLogFile = level;
}

Debug::DebugLevel Debug::getDebugLevelStderr() const
{
    return mDebugLevelStderr;
}

Debug::DebugLevel Debug::getDebugLevelLogFile() const
{
    return mDebugLevelLogFile;
}

const FilePath& Debug::getLogFilepath() const
{
    return mLogFilepath;
}

void Debug::print(DebugLevel level, const QString& msg, const char* file, int line)
{
    if ((mDebugLevelStderr < level) && ((mDebugLevelLogFile < level) || (!mLogFile)))
        return; // if there is nothing to print, we will return immediately from this function

    const char* levelStr = "---------"; // the debug level string has always 9 characters
    switch (level)
    {
        case DebugMsg:
            levelStr = "DEBUG-MSG";
            break;
        case Warning:
            levelStr = " WARNING ";
            break;
        case Exception:
            levelStr = "EXCEPTION";
            break;
        case Critical:
            levelStr = "CRITICAL ";
            break;
        case Fatal:
            levelStr = "  FATAL  ";
            break;
        default:
            break;
    }

    QString logMsg = QString("[%1] %2 (%3:%4)").arg(levelStr, msg.toLocal8Bit().constData(),
                                                    file).arg(line);

    if (mDebugLevelStderr >= level)
    {
        // write to stderr
        *mStderrStream << logMsg << endl;
    }

    if ((mDebugLevelLogFile >= level) && (mLogFile))
    {
        // write to the log file
        QTextStream logFileStream(mLogFile);
        logFileStream << logMsg << endl;
    }
}

/*****************************************************************************************
 *  The message handler for qDebug(), qWarning(), qCritical() and qFatal()
 ****************************************************************************************/

void Debug::messageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
    switch (type)
    {
        case QtDebugMsg:
            instance()->print(DebugMsg, msg, context.file, context.line);
            break;

        case QtWarningMsg:
            instance()->print(Warning, msg, context.file, context.line);
            break;

        case QtCriticalMsg:
            instance()->print(Critical, msg, context.file, context.line);
            break;

        case QtFatalMsg:
            instance()->print(Fatal, msg, context.file, context.line);
            abort(); // fatal error --> quit the whole application!
    }
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/
