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
      Vertex(Point(1000, 0)),
      Vertex(Point(1000, 1000)),
      Vertex(Point(0, 0)),
  })};
  QVector<Path> expected = {Path(QVector<Vertex>{
      Vertex(Point(0, 0)),
      Vertex(Point(1000, 0)),
      Vertex(Point(1000, 1000)),
      Vertex(Point(0, 0)),
  })};
  QVector<Path> output = TangentPathJoiner::join(input);
  EXPECT_EQ(str(expected), str(output)) << debug(expected, output);
}

TEST_F(TangentPathJoinerTest, testOneOpenPath) {
  QVector<Path> input = {Path(QVector<Vertex>{
      Vertex(Point(0, 0)),
      Vertex(Point(1000, 0)),
      Vertex(Point(1000, 1000)),
  })};
  QVector<Path> expected = {Path(QVector<Vertex>{
      Vertex(Point(0, 0)),
      Vertex(Point(1000, 0)),
      Vertex(Point(1000, 1000)),
  })};
  QVector<Path> output = TangentPathJoiner::join(input);
  EXPECT_EQ(str(expected), str(output)) << debug(expected, output);
}

TEST_F(TangentPathJoinerTest, testTwoTangentPaths) {
  QVector<Path> input = {
      Path(QVector<Vertex>{
          Vertex(Point(0, 0), Angle::deg90()),
          Vertex(Point(1000, 0)),
      }),
      Path(QVector<Vertex>{
          Vertex(Point(1000, 0), Angle::deg180()),
          Vertex(Point(1000, 1000)),
      }),
  };
  QVector<Path> expected = {Path(QVector<Vertex>{
      Vertex(Point(0, 0), Angle::deg90()),
      Vertex(Point(1000, 0), Angle::deg180()),
      Vertex(Point(1000, 1000)),
  })};
  QVector<Path> output = TangentPathJoiner::join(input);
  EXPECT_EQ(str(expected), str(output)) << debug(expected, output);
}

TEST_F(TangentPathJoinerTest, testTwoTangentPathsFirstReversed) {
  QVector<Path> input = {
      Path(QVector<Vertex>{
          Vertex(Point(1000, 0), Angle::deg90()),
          Vertex(Point(0, 0)),
      }),
      Path(QVector<Vertex>{
          Vertex(Point(1000, 0), Angle::deg180()),
          Vertex(Point(1000, 1000)),
      }),
  };
  QVector<Path> expected = {Path(QVector<Vertex>{
      Vertex(Point(0, 0), -Angle::deg90()),
      Vertex(Point(1000, 0), Angle::deg180()),
      Vertex(Point(1000, 1000)),
  })};
  QVector<Path> output = TangentPathJoiner::join(input);
  EXPECT_EQ(str(expected), str(output)) << debug(expected, output);
}

TEST_F(TangentPathJoinerTest, testTwoTangentPathsSecondReversed) {
  QVector<Path> input = {
      Path(QVector<Vertex>{
          Vertex(Point(0, 0), Angle::deg90()),
          Vertex(Point(1000, 0)),
      }),
      Path(QVector<Vertex>{
          Vertex(Point(1000, 1000), Angle::deg180()),
          Vertex(Point(1000, 0)),
      }),
  };
  QVector<Path> expected = {Path(QVector<Vertex>{
      Vertex(Point(0, 0), Angle::deg90()),
      Vertex(Point(1000, 0), -Angle::deg180()),
      Vertex(Point(1000, 1000)),
  })};
  QVector<Path> output = TangentPathJoiner::join(input);
  EXPECT_EQ(str(expected), str(output)) << debug(expected, output);
}

TEST_F(TangentPathJoinerTest, testTwoTangentPathsBothReversed) {
  QVector<Path> input = {
      Path(QVector<Vertex>{
          Vertex(Point(1000, 0), Angle::deg90()),
          Vertex(Point(0, 0)),
      }),
      Path(QVector<Vertex>{
          Vertex(Point(1000, 1000), Angle::deg180()),
          Vertex(Point(1000, 0)),
      }),
  };
  QVector<Path> expected = {Path(QVector<Vertex>{
      Vertex(Point(1000, 1000), Angle::deg180()),
      Vertex(Point(1000, 0), Angle::deg90()),
      Vertex(Point(0, 0)),
  })};
  QVector<Path> output = TangentPathJoiner::join(input);
  EXPECT_EQ(str(expected), str(output)) << debug(expected, output);
}

TEST_F(TangentPathJoinerTest, testTwoNestedRects) {
  QVector<Path> input = {
      Path(QVector<Vertex>{
          Vertex(Point(0, 0)),
          Vertex(Point(0, 1000)),
          Vertex(Point(1000, 1000)),
      }),
      Path(QVector<Vertex>{
          Vertex(Point(0, 0)),
          Vertex(Point(1000, 0)),
      }),
      Path(QVector<Vertex>{
          Vertex(Point(1000, 0)),
          Vertex(Point(1000, 1000)),
      }),
      Path(QVector<Vertex>{
          Vertex(Point(1000, 0)),
          Vertex(Point(2000, 0)),
          Vertex(Point(2000, 1000)),
          Vertex(Point(1000, 1000)),
      }),
  };
  QVector<Path> expected = {
      Path(QVector<Vertex>{
          Vertex(Point(1000, 1000)),
          Vertex(Point(0, 1000)),
          Vertex(Point(0, 0)),
          Vertex(Point(1000, 0)),
          Vertex(Point(2000, 0)),
          Vertex(Point(2000, 1000)),
          Vertex(Point(1000, 1000)),
      }),
      Path(QVector<Vertex>{
          Vertex(Point(1000, 0)),
          Vertex(Point(1000, 1000)),
      }),
  };
  QVector<Path> output = TangentPathJoiner::join(input);
  EXPECT_EQ(str(expected), str(output)) << debug(expected, output);
}

TEST_F(TangentPathJoinerTest, testSeveralTangentAndNonTangentPaths) {
  QVector<Path> input = {
      // Path 2, Segment 2
      Path(QVector<Vertex>{
          Vertex(Point(1000, 0)),
          Vertex(Point(1000, 1000), Angle::deg90()),
          Vertex(Point(2000, 1000)),
      }),
      // Path 1 (closed)
      Path(QVector<Vertex>{
          Vertex(Point(0, 0), Angle::deg90()),
          Vertex(Point(1000, 0)),
          Vertex(Point(1000, 1000)),
          Vertex(Point(0, 0)),
      }),
      // Path 3 (open)
      Path(QVector<Vertex>{
          Vertex(Point(5000, 5000), Angle::deg90()),
          Vertex(Point(6000, 6000)),
          Vertex(Point(7000, 7000)),
      }),
      // Path 2, Segment 1
      Path(QVector<Vertex>{
          Vertex(Point(0, 0)),
          Vertex(Point(1000, 0)),
      }),
      // Path 2, Segment 4 (reversed)
      Path(QVector<Vertex>{
          Vertex(Point(4000, 1000)),
          Vertex(Point(3000, 1000), Angle::deg90()),
          Vertex(Point(3000, 0)),
          Vertex(Point(2000, 0)),
      }),
      // Path 2, Segment 3
      Path(QVector<Vertex>{
          Vertex(Point(2000, 1000), Angle::deg90()),
          Vertex(Point(2000, 0)),
      }),
  };
  QVector<Path> expected = {
      // Path 1 (closed)
      Path(QVector<Vertex>{
          Vertex(Point(0, 0), Angle::deg90()),
          Vertex(Point(1000, 0)),
          Vertex(Point(1000, 1000)),
          Vertex(Point(0, 0)),
      }),
      // Path 2 (joined)
      Path(QVector<Vertex>{
          Vertex(Point(0, 0)),
          Vertex(Point(1000, 0)),
          Vertex(Point(1000, 1000), Angle::deg90()),
          Vertex(Point(2000, 1000), Angle::deg90()),
          Vertex(Point(2000, 0)),
          Vertex(Point(3000, 0), -Angle::deg90()),
          Vertex(Point(3000, 1000)),
          Vertex(Point(4000, 1000)),
      }),
      // Path 3 (open)
      Path(QVector<Vertex>{
          Vertex(Point(5000, 5000), Angle::deg90()),
          Vertex(Point(6000, 6000)),
          Vertex(Point(7000, 7000)),
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
        Vertex(Point(i, 0)),     Vertex(Point(i, 1), Angle::deg90()),
        Vertex(Point(i, 2000)),  Vertex(Point(i, 3000)),
        Vertex(Point(i, 4000)),  Vertex(Point(i, 5000)),
        Vertex(Point(i, 6000)),  Vertex(Point(i, 7000)),
        Vertex(Point(i, 8000)),  Vertex(Point(i, 9000)),
        Vertex(Point(i, 10000)), Vertex(Point(i, 11000)),
        Vertex(Point(i, 12000)), Vertex(Point(i, 13000)),
        Vertex(Point(i, 14000)), Vertex(Point(i, 15000)),
        Vertex(Point(i, 16000)), Vertex(Point(i, 17000)),
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
        Vertex(Point(i, 2000)),  Vertex(Point(i, 3000)),
        Vertex(Point(i, 4000)),  Vertex(Point(i, 5000)),
        Vertex(Point(i, 6000)),  Vertex(Point(i, 7000)),
        Vertex(Point(i, 8000)),  Vertex(Point(i, 9000)),
        Vertex(Point(i, 10000)), Vertex(Point(i, 11000)),
        Vertex(Point(i, 12000)), Vertex(Point(i, 13000)),
        Vertex(Point(i, 14000)), Vertex(Point(i, 15000)),
        Vertex(Point(i, 16000)), Vertex(Point(i, 17000)),
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
