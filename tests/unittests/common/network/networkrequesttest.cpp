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
#include "networkrequestbasesignalreceiver.h"

#include <gtest/gtest.h>
#include <librepcb/common/network/networkaccessmanager.h>
#include <librepcb/common/network/networkrequest.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace tests {

/*******************************************************************************
 *  Test Data Type
 ******************************************************************************/

typedef struct {
  QUrl       url;
  QByteArray accept;
  QByteArray contentStart;
  bool       success;
} NetworkRequestTestData;

/*******************************************************************************
 *  Test Class
 ******************************************************************************/

class NetworkRequestTest
  : public ::testing::TestWithParam<NetworkRequestTestData> {
public:
  static void SetUpTestCase() { sDownloadManager = new NetworkAccessManager(); }

  static void TearDownTestCase() { delete sDownloadManager; }

protected:
  NetworkRequestBaseSignalReceiver mSignalReceiver;
  static NetworkAccessManager*     sDownloadManager;
};

NetworkAccessManager* NetworkRequestTest::sDownloadManager = nullptr;

/*******************************************************************************
 *  Test Methods
 ******************************************************************************/

TEST_P(NetworkRequestTest, testConstructorAndSettersAndDestructor) {
  const NetworkRequestTestData& data = GetParam();

  NetworkRequest request(data.url);
  request.setExpectedReplyContentSize(5);
}

TEST_P(NetworkRequestTest, testDownload) {
  const NetworkRequestTestData& data = GetParam();

  // start the request
  NetworkRequest* request = new NetworkRequest(data.url);
  if (!data.accept.isEmpty()) {
    request->setHeaderField("Accept", data.accept);
  }
  QObject::connect(request, &NetworkRequest::progressState, &mSignalReceiver,
                   &NetworkRequestBaseSignalReceiver::progressState);
  QObject::connect(request, &NetworkRequest::progressPercent, &mSignalReceiver,
                   &NetworkRequestBaseSignalReceiver::progressPercent);
  QObject::connect(request, &NetworkRequest::progress, &mSignalReceiver,
                   &NetworkRequestBaseSignalReceiver::progress);
  QObject::connect(request, &NetworkRequest::aborted, &mSignalReceiver,
                   &NetworkRequestBaseSignalReceiver::aborted);
  QObject::connect(request, &NetworkRequest::succeeded, &mSignalReceiver,
                   &NetworkRequestBaseSignalReceiver::succeeded);
  QObject::connect(request, &NetworkRequest::errored, &mSignalReceiver,
                   &NetworkRequestBaseSignalReceiver::errored);
  QObject::connect(request, &NetworkRequest::finished, &mSignalReceiver,
                   &NetworkRequestBaseSignalReceiver::finished);
  QObject::connect(request, &NetworkRequest::dataReceived, &mSignalReceiver,
                   &NetworkRequestBaseSignalReceiver::dataReceived);
  QObject::connect(request, &NetworkRequest::destroyed, &mSignalReceiver,
                   &NetworkRequestBaseSignalReceiver::destroyed);
  request->start();

  // wait until request finished (with timeout)
  qint64 start       = QDateTime::currentDateTime().toMSecsSinceEpoch();
  auto   currentTime = []() {
    return QDateTime::currentDateTime().toMSecsSinceEpoch();
  };
  while ((!mSignalReceiver.mDestroyed) && (currentTime() - start < 30000)) {
    QThread::msleep(100);
    qApp->processEvents();
  }

  // check count and parameters of emited signals
  EXPECT_TRUE(mSignalReceiver.mDestroyed) << "Request timed out!";
  EXPECT_GT(mSignalReceiver.mProgressStateCallCount, 0);
  EXPECT_EQ(mSignalReceiver.mAdvancedProgressCallCount,
            mSignalReceiver.mSimpleProgressCallCount);
  EXPECT_EQ(0, mSignalReceiver.mAbortedCallCount);
  EXPECT_EQ(1, mSignalReceiver.mFinishedCallCount);
  EXPECT_EQ(0, mSignalReceiver.mFileDownloadedCallCount);
  EXPECT_EQ(0, mSignalReceiver.mZipFileExtractedCallCount);
  if (data.success) {
    EXPECT_GE(mSignalReceiver.mSimpleProgressCallCount, 1);
    EXPECT_EQ(1, mSignalReceiver.mSucceededCallCount);
    EXPECT_EQ(0, mSignalReceiver.mErroredCallCount);
    EXPECT_EQ(1, mSignalReceiver.mDataReceivedCallCount);
    EXPECT_TRUE(mSignalReceiver.mErrorMessage.isNull())
        << qPrintable(mSignalReceiver.mErrorMessage);
    EXPECT_TRUE(mSignalReceiver.mFinishedSuccess);
    EXPECT_FALSE(mSignalReceiver.mReceivedData.isEmpty());
  } else {
    EXPECT_GE(mSignalReceiver.mSimpleProgressCallCount, 0);
    EXPECT_EQ(0, mSignalReceiver.mSucceededCallCount);
    EXPECT_EQ(1, mSignalReceiver.mErroredCallCount);
    EXPECT_EQ(0, mSignalReceiver.mDataReceivedCallCount);
    EXPECT_FALSE(mSignalReceiver.mErrorMessage.isEmpty())
        << qPrintable(mSignalReceiver.mErrorMessage);
    EXPECT_FALSE(mSignalReceiver.mFinishedSuccess);
    EXPECT_TRUE(mSignalReceiver.mReceivedData.isEmpty());
  }
  EXPECT_TRUE(
      mSignalReceiver.mReceivedData.trimmed().startsWith(data.contentStart))
      << qPrintable(mSignalReceiver.mReceivedData);
}

/*******************************************************************************
 *  Test Data
 ******************************************************************************/

// clang-format off
INSTANTIATE_TEST_SUITE_P(NetworkRequestTest, NetworkRequestTest, ::testing::Values(
    NetworkRequestTestData({QUrl::fromLocalFile(TEST_DATA_DIR "/unittests/librepcbcommon/NetworkRequestTest/libraries"),
                            QByteArray("application/json"),
                            QByteArray("{"),
                            true}),
    //NetworkRequestTestData({QUrl::fromLocalFile(TEST_DATA_DIR "/common/api/v1/libraries"),
    //                        QByteArray("text/html"),
    //                        QByteArray("<"),
    //                        true}),
    NetworkRequestTestData({QUrl::fromLocalFile("/some-invalid-url"),
                            QByteArray("text/html"),
                            QByteArray(""),
                            false})
));
// clang-format on

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace librepcb
