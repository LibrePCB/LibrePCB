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

#ifndef NETWORKREQUESTBASESIGNALRECEIVER_H
#define NETWORKREQUESTBASESIGNALRECEIVER_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <gtest/gtest.h>
#include <librepcb/common/fileio/filepath.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace tests {

/*******************************************************************************
 *  Signal Receiver Class
 ******************************************************************************/

class NetworkRequestBaseSignalReceiver final : public QObject {
  Q_OBJECT

public:
  QThread* mThread;
  int mProgressStateCallCount;
  int mSimpleProgressCallCount;
  int mAdvancedProgressCallCount;
  int mAbortedCallCount;
  int mSucceededCallCount;
  int mErroredCallCount;
  int mFinishedCallCount;
  int mDataReceivedCallCount;
  int mFileDownloadedCallCount;
  int mZipFileExtractedCallCount;
  bool mDestroyed;
  QString mErrorMessage;
  bool mFinishedSuccess;
  QByteArray mReceivedData;
  FilePath mDownloadedToFilePath;
  FilePath mExtractedToFilePath;

  NetworkRequestBaseSignalReceiver()
    : QObject(),
      mThread(QThread::currentThread()),
      mProgressStateCallCount(0),
      mSimpleProgressCallCount(0),
      mAdvancedProgressCallCount(0),
      mAbortedCallCount(0),
      mSucceededCallCount(0),
      mErroredCallCount(0),
      mFinishedCallCount(0),
      mDataReceivedCallCount(0),
      mFileDownloadedCallCount(0),
      mZipFileExtractedCallCount(0),
      mDestroyed(false),
      mErrorMessage(),
      mFinishedSuccess(false) {}

  void progressState(QString state) {
    EXPECT_EQ(mThread, QThread::currentThread());
    EXPECT_FALSE(state.isEmpty());
    mProgressStateCallCount++;
  }

  void progressPercent(int estimatedPercent) {
    EXPECT_EQ(mThread, QThread::currentThread());
    EXPECT_GE(estimatedPercent, 0);
    EXPECT_LE(estimatedPercent, 100);
    mSimpleProgressCallCount++;
  }

  void progress(qint64 bytesReceived, qint64 bytesTotal, int estimatedPercent) {
    EXPECT_EQ(mThread, QThread::currentThread());
    Q_UNUSED(bytesReceived);
    Q_UNUSED(bytesTotal);
    EXPECT_GE(estimatedPercent, 0);
    EXPECT_LE(estimatedPercent, 100);
    mAdvancedProgressCallCount++;
  }

  void aborted() {
    EXPECT_EQ(mThread, QThread::currentThread());
    mAbortedCallCount++;
  }

  void succeeded() {
    EXPECT_EQ(mThread, QThread::currentThread());
    mSucceededCallCount++;
  }

  void errored(QString errorMsg) {
    EXPECT_EQ(mThread, QThread::currentThread());
    mErrorMessage = errorMsg;
    mErroredCallCount++;
  }

  void finished(bool success) {
    EXPECT_EQ(mThread, QThread::currentThread());
    mFinishedSuccess = success;
    mFinishedCallCount++;
  }

  void dataReceived(QByteArray data) {
    mReceivedData = data;
    mDataReceivedCallCount++;
  }

  void fileDownloaded(FilePath filepath) {
    mDownloadedToFilePath = filepath;
    mFileDownloadedCallCount++;
  }

  void zipFileExtracted(FilePath directory) {
    mExtractedToFilePath = directory;
    mZipFileExtractedCallCount++;
  }

  void destroyed(QObject* obj) {
    EXPECT_EQ(mThread, QThread::currentThread());
    EXPECT_FALSE(mDestroyed);
    Q_UNUSED(obj);
    mDestroyed = true;
  }
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace librepcb

#endif  // NETWORKREQUESTBASESIGNALRECEIVER_H
