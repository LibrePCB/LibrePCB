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

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

Debug::Debug() :
    mDebugLevelStderr(All), mDebugLevelLogFile(Nothing),
    mStderrStream(new QTextStream(stderr)), mLogFile(0)
{
    // determine the filename of the log file which will be used if logging is enabled
    QDir dataDir(QStandardPaths::writableLocation(QStandardPaths::DataLocation));
    QDir logDir(dataDir.absoluteFilePath("logs"));
    logDir.mkpath(logDir.absolutePath());
    mLogFilename = QDir::toNativeSeparators(logDir.absoluteFilePath(
                        QDateTime::currentDateTime().toString(Qt::ISODate) % ".log"));

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
        mLogFile = new QFile(mLogFilename);
        bool success = mLogFile->open(QFile::WriteOnly);
        if (success)
            qDebug() << "enabled logging to file" << mLogFilename;
        else
        {
            delete mLogFile;
            mLogFile = 0;
            qWarning() << "cannot enable logging to file" << mLogFilename;
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

const QString& Debug::getLogFilename() const
{
    return mLogFilename;
}

void Debug::print(DebugLevel level, const QString& msg, const char* file, int line)
{
    if ((mDebugLevelStderr < level) && ((mDebugLevelLogFile < level) || (!mLogFile)))
        return; // if there is nothing to print, we will return immediately from this function

    const char* levelStr = "";
    switch (level)
    {
        case DebugMsg:
            levelStr = "DEBUG";
            break;
        case Warning:
            levelStr = "WARNING";
            break;
        case Critical:
            levelStr = "CRITICAL";
            break;
        case Fatal:
            levelStr = "FATAL";
            break;
        default:
            break;
    }

    QString logMsg = QString("[%1] %2 (%3:%4)\n").arg(levelStr, msg.toLocal8Bit().constData(), file).arg(line);

    if (mDebugLevelStderr >= level)
    {
        // write to stderr
        *mStderrStream << logMsg;
        mStderrStream->flush();
    }

    if ((mDebugLevelLogFile >= level) && (mLogFile))
    {
        // write to the log file
        mLogFile->write(logMsg.toLocal8Bit());
        mLogFile->flush();
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
