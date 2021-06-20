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
#include <librepcb/common/fileio/asynccopyoperation.h>
#include <librepcb/common/fileio/fileutils.h>

#include <QSignalSpy>
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

  AsyncCopyOperationTest() {
    // Temporary dir
    mTmpDir = FilePath::getRandomTempPath();

    // Non-existing dir
    mNonExistingDir = mTmpDir.getPathTo("non existing");

    // Empty dir
    mEmptyDir = mTmpDir.getPathTo("empty directory");
    FileUtils::makePath(mEmptyDir);

    // Populated dir
    mPopulatedDir = mTmpDir.getPathTo("populated directory");
    FileUtils::writeFile(mPopulatedDir.getPathTo("foo/a dir/f"), "A");
    FileUtils::writeFile(mPopulatedDir.getPathTo(".dotfile"), "B");

    // Destination dir
    mDestinationDir = mTmpDir.getPathTo("destination directory");
  }

  virtual ~AsyncCopyOperationTest() {
    QDir(mTmpDir.toStr()).removeRecursively();
  }
};

/*******************************************************************************
 *  Test Methods
 ******************************************************************************/

TEST_F(AsyncCopyOperationTest, testEmptySourceDir) {
  // Start copy operation.
  AsyncCopyOperation copy(mEmptyDir, mDestinationDir);
  QSignalSpy spyStarted(&copy, &AsyncCopyOperation::started);
  QSignalSpy spyProgressStatus(&copy, &AsyncCopyOperation::progressStatus);
  QSignalSpy spyProgressPercent(&copy, &AsyncCopyOperation::progressPercent);
  QSignalSpy spySucceeded(&copy, &AsyncCopyOperation::succeeded);
  QSignalSpy spyFailed(&copy, &AsyncCopyOperation::failed);
  QSignalSpy spyFinished(&copy, &AsyncCopyOperation::finished);
  copy.start();

  // Wait for completion.
  EXPECT_TRUE(spyFinished.wait(5000));

  // Print status and error messages.
  for (const auto& arg : spyProgressStatus) {
    std::cout << "STATUS: " << arg.first().toString().toStdString()
              << std::endl;
  }
  for (const auto& arg : spyFailed) {
    std::cout << "ERROR: " << arg.first().toString().toStdString() << std::endl;
  }

  // Verify emitted signals.
  EXPECT_EQ(spyStarted.count(), 1);
  EXPECT_GE(spyProgressStatus.count(), 1);
  EXPECT_GE(spyProgressPercent.count(), 1);
  EXPECT_EQ(spySucceeded.count(), 1);
  EXPECT_EQ(spyFailed.count(), 0);
  EXPECT_EQ(spyFinished.count(), 1);

  // Verify copied directoy.
  EXPECT_TRUE(mDestinationDir.isEmptyDir());
}

TEST_F(AsyncCopyOperationTest, testPopulatedSourceDir) {
  // Start copy operation.
  AsyncCopyOperation copy(mPopulatedDir, mDestinationDir);
  QSignalSpy spyStarted(&copy, &AsyncCopyOperation::started);
  QSignalSpy spyProgressStatus(&copy, &AsyncCopyOperation::progressStatus);
  QSignalSpy spyProgressPercent(&copy, &AsyncCopyOperation::progressPercent);
  QSignalSpy spySucceeded(&copy, &AsyncCopyOperation::succeeded);
  QSignalSpy spyFailed(&copy, &AsyncCopyOperation::failed);
  QSignalSpy spyFinished(&copy, &AsyncCopyOperation::finished);
  copy.start();

  // Wait for completion.
  EXPECT_TRUE(spyFinished.wait(5000));

  // Print status and error messages.
  for (const auto& arg : spyProgressStatus) {
    std::cout << "STATUS: " << arg.first().toString().toStdString()
              << std::endl;
  }
  for (const auto& arg : spyFailed) {
    std::cout << "ERROR: " << arg.first().toString().toStdString() << std::endl;
  }

  // Verify emitted signals.
  EXPECT_EQ(spyStarted.count(), 1);
  EXPECT_GE(spyProgressStatus.count(), 1);
  EXPECT_GE(spyProgressPercent.count(), 1);
  EXPECT_EQ(spySucceeded.count(), 1);
  EXPECT_EQ(spyFailed.count(), 0);
  EXPECT_EQ(spyFinished.count(), 1);

  // Verify copied directoy.
  EXPECT_EQ(FileUtils::readFile(mDestinationDir.getPathTo("foo/a dir/f")), "A");
  EXPECT_EQ(FileUtils::readFile(mDestinationDir.getPathTo(".dotfile")), "B");
}

TEST_F(AsyncCopyOperationTest, testNonExistentSourceDir) {
  // Start copy operation.
  AsyncCopyOperation copy(mNonExistingDir, mDestinationDir);
  QSignalSpy spyStarted(&copy, &AsyncCopyOperation::started);
  QSignalSpy spyProgressStatus(&copy, &AsyncCopyOperation::progressStatus);
  QSignalSpy spyProgressPercent(&copy, &AsyncCopyOperation::progressPercent);
  QSignalSpy spySucceeded(&copy, &AsyncCopyOperation::succeeded);
  QSignalSpy spyFailed(&copy, &AsyncCopyOperation::failed);
  QSignalSpy spyFinished(&copy, &AsyncCopyOperation::finished);
  copy.start();

  // Wait for completion.
  EXPECT_TRUE(spyFinished.wait(5000));

  // Print status and error messages.
  for (const auto& arg : spyProgressStatus) {
    std::cout << "STATUS: " << arg.first().toString().toStdString()
              << std::endl;
  }
  for (const auto& arg : spyFailed) {
    std::cout << "ERROR: " << arg.first().toString().toStdString() << std::endl;
  }

  // Verify emitted signals.
  EXPECT_EQ(spyStarted.count(), 1);
  EXPECT_GE(spyProgressStatus.count(), 1);
  EXPECT_GE(spyProgressPercent.count(), 0);
  EXPECT_EQ(spySucceeded.count(), 0);
  EXPECT_EQ(spyFailed.count(), 1);
  EXPECT_EQ(spyFinished.count(), 1);

  // Verify copied directoy.
  EXPECT_FALSE(mDestinationDir.isExistingDir());
}

TEST_F(AsyncCopyOperationTest, testExistingDestinationDir) {
  // Start copy operation.
  AsyncCopyOperation copy(mEmptyDir, mPopulatedDir);
  QSignalSpy spyStarted(&copy, &AsyncCopyOperation::started);
  QSignalSpy spyProgressStatus(&copy, &AsyncCopyOperation::progressStatus);
  QSignalSpy spyProgressPercent(&copy, &AsyncCopyOperation::progressPercent);
  QSignalSpy spySucceeded(&copy, &AsyncCopyOperation::succeeded);
  QSignalSpy spyFailed(&copy, &AsyncCopyOperation::failed);
  QSignalSpy spyFinished(&copy, &AsyncCopyOperation::finished);
  copy.start();

  // Wait for completion.
  EXPECT_TRUE(spyFinished.wait(5000));

  // Print status and error messages.
  for (const auto& arg : spyProgressStatus) {
    std::cout << "STATUS: " << arg.first().toString().toStdString()
              << std::endl;
  }
  for (const auto& arg : spyFailed) {
    std::cout << "ERROR: " << arg.first().toString().toStdString() << std::endl;
  }

  // Verify emitted signals.
  EXPECT_EQ(spyStarted.count(), 1);
  EXPECT_GE(spyProgressStatus.count(), 1);
  EXPECT_GE(spyProgressPercent.count(), 0);
  EXPECT_EQ(spySucceeded.count(), 0);
  EXPECT_EQ(spyFailed.count(), 1);
  EXPECT_EQ(spyFinished.count(), 1);

  // Verify that the already existing destination is not removed.
  EXPECT_TRUE(mPopulatedDir.getPathTo("foo/a dir/f").isExistingFile());
  EXPECT_TRUE(mPopulatedDir.getPathTo(".dotfile").isExistingFile());
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace librepcb
