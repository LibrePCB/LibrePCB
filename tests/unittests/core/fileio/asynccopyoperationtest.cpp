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

#include <gtest/gtest.h>
#include <librepcb/core/fileio/asynccopyoperation.h>
#include <librepcb/core/fileio/fileutils.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace tests {

/*******************************************************************************
 *  Test Class
 ******************************************************************************/

class AsyncCopyOperationTest : public ::testing::Test {
protected:
  FilePath mTmpDir;
  FilePath mNonExistingDir;
  FilePath mEmptyDir;
  FilePath mPopulatedDir;
  FilePath mDestinationDir;

  int mSignalStarted;
  QStringList mSignalProgressStatus;
  QList<int> mSignalProgressPercent;
  int mSignalSucceeded;
  QStringList mSignalFailed;
  int mSignalFinished;
  QObject mContext;

  AsyncCopyOperationTest()
    : mTmpDir(FilePath::getRandomTempPath()),
      mNonExistingDir(mTmpDir.getPathTo("non existing")),
      mEmptyDir(mTmpDir.getPathTo("empty directory")),
      mPopulatedDir(mTmpDir.getPathTo("populated directory")),
      mDestinationDir(mTmpDir.getPathTo("destination directory")),
      mSignalStarted(0),
      mSignalProgressStatus(),
      mSignalProgressPercent(),
      mSignalSucceeded(0),
      mSignalFailed(),
      mSignalFinished(0),
      mContext() {
    FileUtils::makePath(mEmptyDir);
    FileUtils::writeFile(mPopulatedDir.getPathTo("foo/a dir/f"), "A");
    FileUtils::writeFile(mPopulatedDir.getPathTo(".dotfile"), "B");
  }

  virtual ~AsyncCopyOperationTest() {
    QDir(mTmpDir.toStr()).removeRecursively();
  }

  bool run(AsyncCopyOperation& copy, unsigned long timeout) {
    // Connect all signals by hand because QSignalSpy is not threadsafe!
    QObject::connect(
        &copy, &AsyncCopyOperation::started, &mContext,
        [this]() { ++mSignalStarted; }, Qt::QueuedConnection);
    QObject::connect(
        &copy, &AsyncCopyOperation::progressStatus, &mContext,
        [this](const QString& status) {
          std::cout << "STATUS: " << status.toStdString() << std::endl;
          mSignalProgressStatus.append(status);
        },
        Qt::QueuedConnection);
    QObject::connect(
        &copy, &AsyncCopyOperation::progressPercent, &mContext,
        [this](int percent) {
          std::cout << "PROGRESS: " << percent << std::endl;
          mSignalProgressPercent.append(percent);
        },
        Qt::QueuedConnection);
    QObject::connect(
        &copy, &AsyncCopyOperation::succeeded, &mContext,
        [this]() { ++mSignalSucceeded; }, Qt::QueuedConnection);
    QObject::connect(
        &copy, &AsyncCopyOperation::failed, &mContext,
        [this](const QString& error) {
          std::cout << "ERROR: " << error.toStdString() << std::endl;
          mSignalFailed.append(error);
        },
        Qt::QueuedConnection);
    QObject::connect(
        &copy, &AsyncCopyOperation::finished, &mContext,
        [this]() { ++mSignalFinished; }, Qt::QueuedConnection);

    copy.start();
    const bool success = copy.wait(timeout);
    // Enforce processing the emitted signals. Needs to be called twice, see
    // https://github.com/LibrePCB/LibrePCB/issues/1061.
    qApp->processEvents();
    qApp->processEvents();
    return success;
  }
};

/*******************************************************************************
 *  Test Methods
 ******************************************************************************/

TEST_F(AsyncCopyOperationTest, testEmptySourceDir) {
  // Perform copy operation.
  AsyncCopyOperation copy(mEmptyDir, mDestinationDir);
  EXPECT_TRUE(run(copy, 5000));

  // Verify emitted signals.
  EXPECT_EQ(mSignalStarted, 1);
  EXPECT_GE(mSignalProgressStatus.count(), 1);
  EXPECT_GE(mSignalProgressPercent.count(), 1);
  EXPECT_EQ(mSignalSucceeded, 1);
  EXPECT_EQ(mSignalFailed.count(), 0);
  EXPECT_EQ(mSignalFinished, 1);

  // Verify copied directoy.
  EXPECT_TRUE(mDestinationDir.isEmptyDir());
}

TEST_F(AsyncCopyOperationTest, testPopulatedSourceDir) {
  // Perform copy operation.
  AsyncCopyOperation copy(mPopulatedDir, mDestinationDir);
  EXPECT_TRUE(run(copy, 5000));

  // Verify emitted signals.
  EXPECT_EQ(mSignalStarted, 1);
  EXPECT_GE(mSignalProgressStatus.count(), 1);
  EXPECT_GE(mSignalProgressPercent.count(), 1);
  EXPECT_EQ(mSignalSucceeded, 1);
  EXPECT_EQ(mSignalFailed.count(), 0);
  EXPECT_EQ(mSignalFinished, 1);

  // Verify copied directoy.
  EXPECT_EQ(FileUtils::readFile(mDestinationDir.getPathTo("foo/a dir/f")), "A");
  EXPECT_EQ(FileUtils::readFile(mDestinationDir.getPathTo(".dotfile")), "B");
}

TEST_F(AsyncCopyOperationTest, testNonExistentSourceDir) {
  // Perform copy operation.
  AsyncCopyOperation copy(mNonExistingDir, mDestinationDir);
  EXPECT_TRUE(run(copy, 5000));

  // Verify emitted signals.
  EXPECT_EQ(mSignalStarted, 1);
  EXPECT_GE(mSignalProgressStatus.count(), 1);
  EXPECT_GE(mSignalProgressPercent.count(), 0);
  EXPECT_EQ(mSignalSucceeded, 0);
  EXPECT_EQ(mSignalFailed.count(), 1);
  EXPECT_EQ(mSignalFinished, 1);

  // Verify copied directoy.
  EXPECT_FALSE(mDestinationDir.isExistingDir());
}

TEST_F(AsyncCopyOperationTest, testExistingDestinationDir) {
  // Perform copy operation.
  AsyncCopyOperation copy(mEmptyDir, mPopulatedDir);
  EXPECT_TRUE(run(copy, 5000));

  // Verify emitted signals.
  EXPECT_EQ(mSignalStarted, 1);
  EXPECT_GE(mSignalProgressStatus.count(), 1);
  EXPECT_GE(mSignalProgressPercent.count(), 0);
  EXPECT_EQ(mSignalSucceeded, 0);
  EXPECT_EQ(mSignalFailed.count(), 1);
  EXPECT_EQ(mSignalFinished, 1);

  // Verify that the already existing destination is not removed.
  EXPECT_TRUE(mPopulatedDir.getPathTo("foo/a dir/f").isExistingFile());
  EXPECT_TRUE(mPopulatedDir.getPathTo(".dotfile").isExistingFile());
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace librepcb
