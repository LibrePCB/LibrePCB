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
#include <librepcb/common/fileio/fileutils.h>
#include <librepcb/project/project.h>
#include <librepcb/project/boards/board.h>
#include <librepcb/project/boards/items/bi_plane.h>

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace project {
namespace tests {

/*****************************************************************************************
 *  Test Class
 ****************************************************************************************/

/**
 * @brief The BoardPlaneFragmentsBuilderTest checks if board plane fragments are correct
 *
 * In the test data directory is a project containing some planes and a file with the
 * expected paths of all plane fragments. This test then re-calculates all plane fragments
 * and compares them with the expected fragments.
 */
class BoardPlaneFragmentsBuilderTest : public ::testing::Test
{
};

/*****************************************************************************************
 *  Test Methods
 ****************************************************************************************/

TEST(BoardPlaneFragmentsBuilderTest, testFragments)
{
    FilePath testDataDir(TEST_DATA_DIR "/project/boards/BoardPlaneFragmentsBuilderTest");

    // open project from test data directory
    FilePath projectFp = testDataDir.getPathTo("test_project/test_project.lpp");
    QScopedPointer<Project> project(new Project(projectFp, true, false));

    // force planes rebuild
    Board* board = project->getBoards().first();
    board->rebuildAllPlanes();

    // determine actual plane fragments
    QMap<Uuid, QSet<Path>> actualPlaneFragments;
    foreach (const BI_Plane* plane, board->getPlanes()) {
        foreach (const Path& fragment, plane->getFragments()) {
            actualPlaneFragments[plane->getUuid()].insert(fragment);
        }
    }

    // write actual plane fragments into file (useful for debugging purposes)
    SExpression actualSexpr = SExpression::createList("actual");
    foreach (const Uuid& uuid, actualPlaneFragments.keys()) {
        SExpression child = SExpression::createList("plane");
        child.appendChild(uuid);
        foreach (const Path& fragment, actualPlaneFragments[uuid]) {
            child.appendChild(fragment.serializeToDomElement("fragment"), true);
        }
        actualSexpr.appendChild(child, true);
    }
    FileUtils::writeFile(testDataDir.getPathTo("actual.lp"), actualSexpr.toString(0).toUtf8());

    // load expected plane fragments from file
    FilePath expectedFp = testDataDir.getPathTo("expected.lp");
    SExpression expectedSexpr = SExpression::parse(FileUtils::readFile(expectedFp), expectedFp);
    QMap<Uuid, QSet<Path>> expectedPlaneFragments;
    foreach (const SExpression& child, expectedSexpr.getChildren("plane")) {
        Uuid uuid = child.getValueOfFirstChild<Uuid>();
        foreach (const SExpression& fragmentChild, child.getChildren("fragment")) {
            expectedPlaneFragments[uuid].insert(Path(fragmentChild));
        }
    }

    // compare
    EXPECT_EQ(expectedPlaneFragments, actualPlaneFragments);
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace tests
} // namespace project
} // namespace librepcb
