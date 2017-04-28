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
#include <gtest/gtest.h>
#include <librepcb/common/systeminfo.h>
#include <librepcb/project/project.h>

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace project {
namespace tests {

/*****************************************************************************************
 *  Test Class
 ****************************************************************************************/

class ProjectTest : public ::testing::Test
{
    protected:
        FilePath mProjectDir;
        FilePath mProjectFile;

        ProjectTest() {
            mProjectDir = FilePath::getRandomTempPath().getPathTo("test project dir");
            mProjectFile = mProjectDir.getPathTo("test project.lpp");
            // the whitespaces in the path are there to make the test even stronger ;)
        }

        virtual ~ProjectTest() {
            QDir(mProjectDir.getParentDir().toStr()).removeRecursively();
        }
};

/*****************************************************************************************
 *  Test Methods
 ****************************************************************************************/

TEST_F(ProjectTest, testCreateCloseOpen)
{
    QDateTime datetime = QDateTime::currentDateTime();

    // create new project
    QScopedPointer<Project> project(Project::create(mProjectFile));
    EXPECT_EQ(mProjectFile, project->getFilepath());
    EXPECT_EQ(mProjectDir, project->getPath());
    EXPECT_FALSE(project->isReadOnly());
    EXPECT_FALSE(project->isRestored());
    EXPECT_EQ(mProjectFile.getCompleteBasename(), project->getName());
    EXPECT_EQ(SystemInfo::getFullUsername(), project->getAuthor());
    EXPECT_EQ(QString("v1"), project->getVersion());
    EXPECT_NEAR(datetime.toMSecsSinceEpoch(),
                project->getCreated().toMSecsSinceEpoch(), 5000);
    EXPECT_NEAR(datetime.toMSecsSinceEpoch(),
                project->getLastModified().toMSecsSinceEpoch(), 5000);
    EXPECT_EQ(0, project->getSchematics().count());
    EXPECT_EQ(0, project->getBoards().count());

    // close project
    project.reset();

    // check existence of files
    EXPECT_TRUE(mProjectDir.isExistingDir());
    EXPECT_FALSE(mProjectDir.isEmptyDir());
    EXPECT_TRUE(mProjectFile.isExistingFile());
    EXPECT_TRUE(mProjectDir.getPathTo(".librepcb-project").isExistingFile());
    EXPECT_TRUE(mProjectDir.getPathTo("core/circuit.xml").isExistingFile());
    EXPECT_TRUE(mProjectDir.getPathTo("core/settings.xml").isExistingFile());
    EXPECT_TRUE(mProjectDir.getPathTo("core/erc.xml").isExistingFile());

    // open project again
    project.reset(new Project(mProjectFile, false));
    EXPECT_EQ(mProjectFile, project->getFilepath());
    EXPECT_EQ(mProjectDir, project->getPath());
    EXPECT_FALSE(project->isReadOnly());
    EXPECT_FALSE(project->isRestored());
    EXPECT_EQ(mProjectFile.getCompleteBasename(), project->getName());
    EXPECT_EQ(SystemInfo::getFullUsername(), project->getAuthor());
    EXPECT_EQ(QString("v1"), project->getVersion());
    EXPECT_NEAR(datetime.toMSecsSinceEpoch(),
                project->getCreated().toMSecsSinceEpoch(), 5000);
    EXPECT_NEAR(datetime.toMSecsSinceEpoch(),
                project->getLastModified().toMSecsSinceEpoch(), 5000);
    EXPECT_EQ(0, project->getSchematics().count());
    EXPECT_EQ(0, project->getBoards().count());
}

TEST_F(ProjectTest, testSave)
{
    // create new project
    QScopedPointer<Project> project(Project::create(mProjectFile));

    // save project
    project->save(false);
    project->save(true);

    // close and re-open project
    project.reset();
    project.reset(new Project(mProjectFile, false));

    // save project
    project->save(false);
    project->save(true);

    // close and re-open project
    project.reset();
    project.reset(new Project(mProjectFile, false));
}

TEST_F(ProjectTest, testIfLastModifiedDateTimeIsUpdatedOnSave)
{
    // create new project
    QScopedPointer<Project> project(Project::create(mProjectFile));
    qint64 datetimeAfterCreating = project->getLastModified().toMSecsSinceEpoch();

    // check if datetime has not changed
    QThread::msleep(1000);
    EXPECT_EQ(datetimeAfterCreating, project->getLastModified().toMSecsSinceEpoch());

    // save project and verify that datetime has changed
    QThread::msleep(1000);
    project->save(true);
    qint64 datetimeAfterSaving = project->getLastModified().toMSecsSinceEpoch();
    EXPECT_NEAR(QDateTime::currentMSecsSinceEpoch(), datetimeAfterSaving, 1000); // +/- 1s
    EXPECT_NE(datetimeAfterCreating, datetimeAfterSaving);
}

TEST_F(ProjectTest, testSettersGetters)
{
    // create new project
    QScopedPointer<Project> project(Project::create(mProjectFile));

    // set properties
    QString name = "test name 1234";
    QString author = "test author 1234";
    QString version = "test version 1234";
    project->setName(name);
    project->setAuthor(author);
    project->setVersion(version);

    // get properties
    EXPECT_EQ(name, project->getName());
    EXPECT_EQ(author, project->getAuthor());
    EXPECT_EQ(version, project->getVersion());

    // save project
    project->save(true);

    // close and re-open project (read-only)
    project.reset();
    project.reset(new Project(mProjectFile, true));

    // get properties
    EXPECT_EQ(name, project->getName());
    EXPECT_EQ(author, project->getAuthor());
    EXPECT_EQ(version, project->getVersion());
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace tests
} // namespace project
} // namespace librepcb
