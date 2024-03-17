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
#include <librepcb/core/geometry/path.h>
#include <librepcb/core/serialization/sexpression.h>
#include <librepcb/core/utils/tangentpathjoiner.h>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace tests {

/*******************************************************************************
 *  Test Class
 ******************************************************************************/

class TangentPathJoinerTest : public ::testing::Test {
protected:
  std::string str(QVector<Path> paths, const QString& name = "paths") const {
    std::unique_ptr<SExpression> root = SExpression::createList(name);
    foreach (const Path& p, paths) {
      root->ensureLineBreak();
      p.serialize(root->appendList("path"));
    }
    root->ensureLineBreak();
    return root->toByteArray().toStdString();
  }

  std::string debug(const QVector<Path>& expected,
                    const QVector<Path>& actual) const {
    return str(expected, "expected") + "\n" + str(actual, "actual");
  }
};

/*******************************************************************************
 *  Test Methods
 ******************************************************************************/

TEST_F(TangentPathJoinerTest, testEmptyInput) {
  QVector<Path> input = {};
  QVector<Path> expected = {};
  QVector<Path> output = TangentPathJoiner::join(input);
  EXPECT_EQ(str(expected), str(output)) << debug(expected, output);
}

TEST_F(TangentPathJoinerTest, testNoVertices) {
  QVector<Path> input = {Path()};
  QVector<Path> expected = {};
  QVector<Path> output = TangentPathJoiner::join(input);
  EXPECT_EQ(str(expected), str(output)) << debug(expected, output);
}

TEST_F(TangentPathJoinerTest, testOnlyOneVertex) {
  QVector<Path> input = {Path(QVector<Vertex>{Vertex(Point(0, 0))})};
  QVector<Path> expected = {};
  QVector<Path> output = TangentPathJoiner::join(input);
  EXPECT_EQ(str(expected), str(output)) << debug(expected, output);
}

TEST_F(TangentPathJoinerTest, testOneClosedPath) {
  QVector<Path> input = {Path(QVector<Vertex>{
      Vertex(Point(0, 0)),
      Vertex(Point(1, 0)),
      Vertex(Point(1, 1)),
      Vertex(Point(0, 0)),
  })};
  QVector<Path> expected = {Path(QVector<Vertex>{
      Vertex(Point(0, 0)),
      Vertex(Point(1, 0)),
      Vertex(Point(1, 1)),
      Vertex(Point(0, 0)),
  })};
  QVector<Path> output = TangentPathJoiner::join(input);
  EXPECT_EQ(str(expected), str(output)) << debug(expected, output);
}

TEST_F(TangentPathJoinerTest, testOneOpenPath) {
  QVector<Path> input = {Path(QVector<Vertex>{
      Vertex(Point(0, 0)),
      Vertex(Point(1, 0)),
      Vertex(Point(1, 1)),
  })};
  QVector<Path> expected = {Path(QVector<Vertex>{
      Vertex(Point(0, 0)),
      Vertex(Point(1, 0)),
      Vertex(Point(1, 1)),
  })};
  QVector<Path> output = TangentPathJoiner::join(input);
  EXPECT_EQ(str(expected), str(output)) << debug(expected, output);
}

TEST_F(TangentPathJoinerTest, testTwoTangentPaths) {
  QVector<Path> input = {
      Path(QVector<Vertex>{
          Vertex(Point(0, 0), Angle::deg90()),
          Vertex(Point(1, 0)),
      }),
      Path(QVector<Vertex>{
          Vertex(Point(1, 0), Angle::deg180()),
          Vertex(Point(1, 1)),
      }),
  };
  QVector<Path> expected = {Path(QVector<Vertex>{
      Vertex(Point(0, 0), Angle::deg90()),
      Vertex(Point(1, 0), Angle::deg180()),
      Vertex(Point(1, 1)),
  })};
  QVector<Path> output = TangentPathJoiner::join(input);
  EXPECT_EQ(str(expected), str(output)) << debug(expected, output);
}

TEST_F(TangentPathJoinerTest, testTwoTangentPathsFirstReversed) {
  QVector<Path> input = {
      Path(QVector<Vertex>{
          Vertex(Point(1, 0), Angle::deg90()),
          Vertex(Point(0, 0)),
      }),
      Path(QVector<Vertex>{
          Vertex(Point(1, 0), Angle::deg180()),
          Vertex(Point(1, 1)),
      }),
  };
  QVector<Path> expected = {Path(QVector<Vertex>{
      Vertex(Point(0, 0), -Angle::deg90()),
      Vertex(Point(1, 0), Angle::deg180()),
      Vertex(Point(1, 1)),
  })};
  QVector<Path> output = TangentPathJoiner::join(input);
  EXPECT_EQ(str(expected), str(output)) << debug(expected, output);
}

TEST_F(TangentPathJoinerTest, testTwoTangentPathsSecondReversed) {
  QVector<Path> input = {
      Path(QVector<Vertex>{
          Vertex(Point(0, 0), Angle::deg90()),
          Vertex(Point(1, 0)),
      }),
      Path(QVector<Vertex>{
          Vertex(Point(1, 1), Angle::deg180()),
          Vertex(Point(1, 0)),
      }),
  };
  QVector<Path> expected = {Path(QVector<Vertex>{
      Vertex(Point(0, 0), Angle::deg90()),
      Vertex(Point(1, 0), -Angle::deg180()),
      Vertex(Point(1, 1)),
  })};
  QVector<Path> output = TangentPathJoiner::join(input);
  EXPECT_EQ(str(expected), str(output)) << debug(expected, output);
}

TEST_F(TangentPathJoinerTest, testTwoTangentPathsBothReversed) {
  QVector<Path> input = {
      Path(QVector<Vertex>{
          Vertex(Point(1, 0), Angle::deg90()),
          Vertex(Point(0, 0)),
      }),
      Path(QVector<Vertex>{
          Vertex(Point(1, 1), Angle::deg180()),
          Vertex(Point(1, 0)),
      }),
  };
  QVector<Path> expected = {Path(QVector<Vertex>{
      Vertex(Point(1, 1), Angle::deg180()),
      Vertex(Point(1, 0), Angle::deg90()),
      Vertex(Point(0, 0)),
  })};
  QVector<Path> output = TangentPathJoiner::join(input);
  EXPECT_EQ(str(expected), str(output)) << debug(expected, output);
}

TEST_F(TangentPathJoinerTest, testTwoNestedRects) {
  QVector<Path> input = {
      Path(QVector<Vertex>{
          Vertex(Point(0, 0)),
          Vertex(Point(0, 1)),
          Vertex(Point(1, 1)),
      }),
      Path(QVector<Vertex>{
          Vertex(Point(0, 0)),
          Vertex(Point(1, 0)),
      }),
      Path(QVector<Vertex>{
          Vertex(Point(1, 0)),
          Vertex(Point(1, 1)),
      }),
      Path(QVector<Vertex>{
          Vertex(Point(1, 0)),
          Vertex(Point(2, 0)),
          Vertex(Point(2, 1)),
          Vertex(Point(1, 1)),
      }),
  };
  QVector<Path> expected = {
      Path(QVector<Vertex>{
          Vertex(Point(1, 1)),
          Vertex(Point(0, 1)),
          Vertex(Point(0, 0)),
          Vertex(Point(1, 0)),
          Vertex(Point(2, 0)),
          Vertex(Point(2, 1)),
          Vertex(Point(1, 1)),
      }),
      Path(QVector<Vertex>{
          Vertex(Point(1, 0)),
          Vertex(Point(1, 1)),
      }),
  };
  QVector<Path> output = TangentPathJoiner::join(input);
  EXPECT_EQ(str(expected), str(output)) << debug(expected, output);
}

TEST_F(TangentPathJoinerTest, testSeveralTangentAndNonTangentPaths) {
  QVector<Path> input = {
      // Path 2, Segment 2
      Path(QVector<Vertex>{
          Vertex(Point(1, 0)),
          Vertex(Point(1, 1), Angle::deg90()),
          Vertex(Point(2, 1)),
      }),
      // Path 1 (closed)
      Path(QVector<Vertex>{
          Vertex(Point(0, 0), Angle::deg90()),
          Vertex(Point(1, 0)),
          Vertex(Point(1, 1)),
          Vertex(Point(0, 0)),
      }),
      // Path 3 (open)
      Path(QVector<Vertex>{
          Vertex(Point(5, 5), Angle::deg90()),
          Vertex(Point(6, 6)),
          Vertex(Point(7, 7)),
      }),
      // Path 2, Segment 1
      Path(QVector<Vertex>{
          Vertex(Point(0, 0)),
          Vertex(Point(1, 0)),
      }),
      // Path 2, Segment 4 (reversed)
      Path(QVector<Vertex>{
          Vertex(Point(4, 1)),
          Vertex(Point(3, 1), Angle::deg90()),
          Vertex(Point(3, 0)),
          Vertex(Point(2, 0)),
      }),
      // Path 2, Segment 3
      Path(QVector<Vertex>{
          Vertex(Point(2, 1), Angle::deg90()),
          Vertex(Point(2, 0)),
      }),
  };
  QVector<Path> expected = {
      // Path 1 (closed)
      Path(QVector<Vertex>{
          Vertex(Point(0, 0), Angle::deg90()),
          Vertex(Point(1, 0)),
          Vertex(Point(1, 1)),
          Vertex(Point(0, 0)),
      }),
      // Path 2 (joined)
      Path(QVector<Vertex>{
          Vertex(Point(0, 0)),
          Vertex(Point(1, 0)),
          Vertex(Point(1, 1), Angle::deg90()),
          Vertex(Point(2, 1), Angle::deg90()),
          Vertex(Point(2, 0)),
          Vertex(Point(3, 0), -Angle::deg90()),
          Vertex(Point(3, 1)),
          Vertex(Point(4, 1)),
      }),
      // Path 3 (open)
      Path(QVector<Vertex>{
          Vertex(Point(5, 5), Angle::deg90()),
          Vertex(Point(6, 6)),
          Vertex(Point(7, 7)),
      }),
  };
  QVector<Path> output = TangentPathJoiner::join(input);
  EXPECT_EQ(str(expected), str(output)) << debug(expected, output);
}

// For testing performance with huge input.
TEST_F(TangentPathJoinerTest, testManyIndependentPaths) {
  QVector<Path> input;
  for (int i = 0; i < 1000; ++i) {
    QVector<Vertex> vertices{
        Vertex(Point(i, 0)),  Vertex(Point(i, 1), Angle::deg90()),
        Vertex(Point(i, 2)),  Vertex(Point(i, 3)),
        Vertex(Point(i, 4)),  Vertex(Point(i, 5)),
        Vertex(Point(i, 6)),  Vertex(Point(i, 7)),
        Vertex(Point(i, 8)),  Vertex(Point(i, 9)),
        Vertex(Point(i, 10)), Vertex(Point(i, 11)),
        Vertex(Point(i, 12)), Vertex(Point(i, 13)),
        Vertex(Point(i, 14)), Vertex(Point(i, 15)),
        Vertex(Point(i, 16)), Vertex(Point(i, 17)),
    };
    input.append(Path(vertices));
  }
  QVector<Path> expected = input;
  QVector<Path> output = TangentPathJoiner::join(input);
  EXPECT_EQ(str(expected), str(output)) << debug(expected, output);
}

// For testing performance with huge input.
TEST_F(TangentPathJoinerTest, testManyTangentPaths) {
  QVector<Path> input;
  QVector<Vertex> expectedVertices;
  for (int i = 0; i < 1000; ++i) {
    QVector<Vertex> vertices{
        Vertex(Point(i, 0)),     Vertex(Point(i, 1), Angle::deg90()),
        Vertex(Point(i, 2)),     Vertex(Point(i, 3)),
        Vertex(Point(i, 4)),     Vertex(Point(i, 5)),
        Vertex(Point(i, 6)),     Vertex(Point(i, 7)),
        Vertex(Point(i, 8)),     Vertex(Point(i, 9)),
        Vertex(Point(i, 10)),    Vertex(Point(i, 11)),
        Vertex(Point(i, 12)),    Vertex(Point(i, 13)),
        Vertex(Point(i, 14)),    Vertex(Point(i, 15)),
        Vertex(Point(i, 16)),    Vertex(Point(i, 17)),
        Vertex(Point(i + 1, 0)),
    };
    input.append(Path(vertices));
    if (!expectedVertices.isEmpty()) {
      expectedVertices.removeLast();
    }
    expectedVertices.append(vertices);
  }
  QVector<Path> expected = {Path(expectedVertices)};
  QVector<Path> output = TangentPathJoiner::join(input);
  EXPECT_EQ(str(expected), str(output)) << debug(expected, output);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace librepcb
