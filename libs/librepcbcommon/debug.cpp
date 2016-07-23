/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
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
#include "debug.h"
#include "fileio/filepath.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

Debug::Debug() :
    mDebugLevelStderr(DebugLevel_t::All), mDebugLevelLogFile(DebugLevel_t::Nothing),
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

void Debug::setDebugLevelStderr(DebugLevel_t level)
{
    mDebugLevelStderr = level;
}

void Debug::setDebugLevelLogFile(DebugLevel_t level)
{
    if (level == mDebugLevelLogFile)
        return;

    if ((mDebugLevelLogFile == DebugLevel_t::Nothing) && (level != DebugLevel_t::Nothing))
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
    else if ((mDebugLevelLogFile != DebugLevel_t::Nothing)
             && (level == DebugLevel_t::Nothing) && (mLogFile))
    {
        // disable logging to file
        mLogFile->close();
        delete mLogFile;
        mLogFile = 0;
    }

    mDebugLevelLogFile = level;
}

Debug::DebugLevel_t Debug::getDebugLevelStderr() const
{
    return mDebugLevelStderr;
}

Debug::DebugLevel_t Debug::getDebugLevelLogFile() const
{
    return mDebugLevelLogFile;
}

const FilePath& Debug::getLogFilepath() const
{
    return mLogFilepath;
}

void Debug::print(DebugLevel_t level, const QString& msg, const char* file, int line)
{
    if ((mDebugLevelStderr < level) && ((mDebugLevelLogFile < level) || (!mLogFile)))
        return; // if there is nothing to print, we will return immediately from this function

    const char* levelStr = "---------"; // the debug level string has always 9 characters
    switch (level)
    {
        case DebugLevel_t::DebugMsg:
            levelStr = "DEBUG-MSG";
            break;
        case DebugLevel_t::Info:
            levelStr = "  INFO   ";
            break;
        case DebugLevel_t::Warning:
            levelStr = " WARNING ";
            break;
        case DebugLevel_t::Exception:
            levelStr = "EXCEPTION";
            break;
        case DebugLevel_t::Critical:
            levelStr = "CRITICAL ";
            break;
        case DebugLevel_t::Fatal:
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
            instance()->print(DebugLevel_t::DebugMsg, msg, context.file, context.line);
            break;
#if (QT_VERSION >= QT_VERSION_CHECK(5, 5, 0))
        case QtInfoMsg:
            instance()->print(DebugLevel_t::Info, msg, context.file, context.line);
            break;
#endif
        case QtWarningMsg:
            instance()->print(DebugLevel_t::Warning, msg, context.file, context.line);
            break;

        case QtCriticalMsg:
            instance()->print(DebugLevel_t::Critical, msg, context.file, context.line);
            break;

        case QtFatalMsg:
            instance()->print(DebugLevel_t::Fatal, msg, context.file, context.line);
            abort(); // fatal error --> quit the whole application!

        default:
            Q_ASSERT(false);
            instance()->print(DebugLevel_t::Critical, QString("unhandled case: %1").arg(type), __FILE__, __LINE__);
            break;
    }
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace librepcb
