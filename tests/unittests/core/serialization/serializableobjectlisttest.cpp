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
#include "serializableobjectmock.h"

#include <gtest/gtest.h>
#include <librepcb/core/serialization/serializableobjectlist.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace tests {

/*******************************************************************************
 *  Types
 ******************************************************************************/

struct SerializableObjectListTagNameProvider {
  static constexpr const char* tagname = "test";
};

using MinimalMock = MinimalSerializableObjectMock;
using Mock = SerializableObjectMock;

using MinimalList =
    SerializableObjectList<MinimalMock, SerializableObjectListTagNameProvider>;
using List =
    SerializableObjectList<Mock, SerializableObjectListTagNameProvider>;

/*******************************************************************************
 *  Test Class
 ******************************************************************************/

class SerializableObjectListTest : public ::testing::Test {
public:
  QList<std::shared_ptr<Mock>> mMocks;

  SerializableObjectListTest() : ::testing::Test() {
    appendMock("c2ceffd2-4cc5-43c6-941c-fc64a341d026", "foo");
    appendMock("4484ba9b-f3f8-4487-9109-10a8e9844fdc", "bar");
    appendMock("162bf1b0-f45e-4175-9656-33b5adc73ed0", "pcb");
  }

private:
  void appendMock(const char* uuid, const char* name) {
    mMocks.append(std::make_shared<Mock>(Uuid::fromString(uuid), name));
  }
};

/*******************************************************************************
 *  Test Methods
 ******************************************************************************/

TEST_F(SerializableObjectListTest, testInstantiationWithMinimalElementClass) {
  std::unique_ptr<const SExpression> sexpr = SExpression::createList("list");

  MinimalList l1;  // default ctor
  MinimalList l2(std::move(l1));  // move ctor
  MinimalList l3(*sexpr);  // SExpression ctor
  l3.append(std::make_shared<MinimalMock>("foo"));
  EXPECT_TRUE(l1.isEmpty());
  EXPECT_EQ(0, l2.count());
  EXPECT_FALSE(l2.contains(0));
  EXPECT_NE(nullptr, l3.value(0));
}

TEST_F(SerializableObjectListTest, testDefaultConstructor) {
  List l;
  EXPECT_EQ(0, l.count());
}

TEST_F(SerializableObjectListTest, testCopyConstructor) {
  List l1{mMocks[0], mMocks[1]};
  List l2(l1);
  EXPECT_EQ(2, l2.count());
  EXPECT_EQ(*mMocks[0], *l2[0]);
  EXPECT_EQ(*mMocks[1], *l2[1]);
}

TEST_F(SerializableObjectListTest, testMoveConstructor) {
  List l1{mMocks[0]};
  List l2(std::move(l1));
  EXPECT_EQ(0, l1.count());
  EXPECT_EQ(1, l2.count());
  EXPECT_EQ(mMocks[0], l2[0]);
}

TEST_F(SerializableObjectListTest, testPointerInitializerListConstructor) {
  List l{mMocks[0], mMocks[1]};
  EXPECT_EQ(2, l.count());
  EXPECT_EQ(mMocks[0], l[0]);
  EXPECT_EQ(mMocks[1], l[1]);
}

TEST_F(SerializableObjectListTest, testValueInitializerListConstructor) {
  List l{std::make_shared<Mock>(Uuid::createRandom(), "foo"),
         std::make_shared<Mock>(Uuid::createRandom(), "bar")};
  EXPECT_EQ(2, l.count());
  EXPECT_EQ("foo", l[0]->mName);
  EXPECT_EQ("bar", l[1]->mName);
}

TEST_F(SerializableObjectListTest, testSExpressionConstructor) {
  std::unique_ptr<SExpression> e = SExpression::createList("list");
  e->ensureLineBreak();
  e->appendChild("test", mMocks[0]->mUuid).appendChild<QString>("name", "foo");
  e->ensureLineBreak();
  e->appendChild("test", mMocks[1]->mUuid).appendChild<QString>("name", "bar");
  e->ensureLineBreak();
  e->appendChild("none", mMocks[2]->mUuid).appendChild<QString>("name", "bar");
  e->ensureLineBreak();
  List l(*e);
  EXPECT_EQ(2, l.count());
  EXPECT_EQ(mMocks[0]->mUuid, l[0]->mUuid);
  EXPECT_EQ(mMocks[1]->mUuid, l[1]->mUuid);
  EXPECT_EQ("foo", l[0]->mName);
  EXPECT_EQ("bar", l[1]->mName);
}

TEST_F(SerializableObjectListTest, testGetUuids) {
  List l{mMocks[0], mMocks[1], mMocks[2], mMocks[2]};
  std::vector<Uuid> vector{mMocks[0]->mUuid, mMocks[1]->mUuid, mMocks[2]->mUuid,
                           mMocks[2]->mUuid};
  QSet<Uuid> set{mMocks[0]->mUuid, mMocks[1]->mUuid, mMocks[2]->mUuid};
  EXPECT_EQ(vector, l.getUuids());
  EXPECT_EQ(set, l.getUuidSet());
}

TEST_F(SerializableObjectListTest, testIndexOfPointer) {
  List l{mMocks[0], mMocks[1], mMocks[2]};
  EXPECT_EQ(0, l.indexOf(mMocks[0].get()));
}

TEST_F(SerializableObjectListTest, testIndexOfUuid) {
  List l{mMocks[0], mMocks[1], mMocks[2]};
  EXPECT_EQ(1, l.indexOf(mMocks[1]->mUuid));
}

TEST_F(SerializableObjectListTest, testIndexOfNameCaseSensitive) {
  List l{mMocks[0], mMocks[1], mMocks[2]};
  EXPECT_EQ(2, l.indexOf("pcb"));
  EXPECT_EQ(2, l.indexOf("pcb", Qt::CaseSensitive));
  EXPECT_EQ(-1, l.indexOf("PCB"));
}

TEST_F(SerializableObjectListTest, testIndexOfNameCaseInsensitive) {
  List l{mMocks[0], mMocks[1], mMocks[2]};
  EXPECT_EQ(2, l.indexOf("pcb", Qt::CaseInsensitive));
  EXPECT_EQ(2, l.indexOf("PCB", Qt::CaseInsensitive));
}

TEST_F(SerializableObjectListTest, testContainsPointer) {
  List l{mMocks[0], mMocks[1], mMocks[2]};
  EXPECT_TRUE(l.contains(mMocks[0].get()));
  EXPECT_FALSE(l.contains(nullptr));
}

TEST_F(SerializableObjectListTest, testContainsUuid) {
  List l{mMocks[0], mMocks[1], mMocks[2]};
  EXPECT_TRUE(l.contains(mMocks[1]->mUuid));
  EXPECT_FALSE(l.contains(Uuid::createRandom()));
}

TEST_F(SerializableObjectListTest, testContainsName) {
  List l{mMocks[0], mMocks[1], mMocks[2]};
  EXPECT_TRUE(l.contains(mMocks[2]->mName));
  EXPECT_FALSE(l.contains(QString()));
}

TEST_F(SerializableObjectListTest, testDataAccess) {
  List l{mMocks[0], mMocks[1], mMocks[2]};
  EXPECT_EQ(mMocks[0], l.first());
  EXPECT_EQ(mMocks[0], l[0]);
  EXPECT_EQ(mMocks[1], l[1]);
  EXPECT_EQ(mMocks[2], l[2]);
  EXPECT_EQ(mMocks[2], l.last());
}

TEST_F(SerializableObjectListTest, testConstDataAccess) {
  const List l{mMocks[0], mMocks[1], mMocks[2]};
  EXPECT_EQ(mMocks[0], l.first());
  EXPECT_EQ(mMocks[0], l[0]);
  EXPECT_EQ(mMocks[1], l[1]);
  EXPECT_EQ(mMocks[2], l[2]);
  EXPECT_EQ(mMocks[2], l.last());
}

TEST_F(SerializableObjectListTest, testIteratorOnEmptyList) {
  List l;
  for (auto it = l.cbegin(); it != l.cend(); ++it) {
    ADD_FAILURE();
  }
}

TEST_F(SerializableObjectListTest, testConstIterator) {
  const List l{mMocks[0], mMocks[1], mMocks[2]};
  int i = 0;
  for (const auto& mock : l) {
    EXPECT_EQ(*mMocks[i], mock);
    ++i;
  }
  EXPECT_EQ(3, i);
}

TEST_F(SerializableObjectListTest, testMutableIterator) {
  List l{mMocks[0], mMocks[1], mMocks[2]};
  int i = 0;
  for (auto& mock : l) {
    mock.mName = QString::number(i++);
  }
  EXPECT_EQ("0", l[0]->mName);
  EXPECT_EQ("1", l[1]->mName);
  EXPECT_EQ("2", l[2]->mName);
}

TEST_F(SerializableObjectListTest, testSwap) {
  List l{mMocks[0], mMocks[1], mMocks[2]};
  l.swap(2, 1);
  EXPECT_EQ(mMocks[0], l[0]);
  EXPECT_EQ(mMocks[2], l[1]);
  EXPECT_EQ(mMocks[1], l[2]);
}

TEST_F(SerializableObjectListTest, testInsert) {
  List l;
  l.insert(0, mMocks[0]);
  l.insert(0, mMocks[1]);
  l.insert(1, mMocks[2]);
  EXPECT_EQ(3, l.count());
  EXPECT_EQ(mMocks[1], l[0]);
  EXPECT_EQ(mMocks[2], l[1]);
  EXPECT_EQ(mMocks[0], l[2]);
}

TEST_F(SerializableObjectListTest, testAppendItem) {
  List l;
  l.append(mMocks[0]);
  l.append(mMocks[1]);
  l.append(mMocks[2]);
  EXPECT_EQ(3, l.count());
  EXPECT_EQ(mMocks[0], l[0]);
  EXPECT_EQ(mMocks[1], l[1]);
  EXPECT_EQ(mMocks[2], l[2]);
}

TEST_F(SerializableObjectListTest, testAppendList) {
  List l1{mMocks[0]};
  List l2{mMocks[1], mMocks[2]};
  l1.append(l2);
  EXPECT_EQ(3, l1.count());
  EXPECT_EQ(mMocks[0], l1[0]);
  EXPECT_EQ(mMocks[1], l1[1]);
  EXPECT_EQ(mMocks[2], l1[2]);
}

TEST_F(SerializableObjectListTest, testRemove) {
  List l{mMocks[0], mMocks[1], mMocks[2]};
  l.remove(1);
  EXPECT_EQ(2, l.count());
  EXPECT_EQ(mMocks[0], l[0]);
  EXPECT_EQ(mMocks[2], l[1]);
}

TEST_F(SerializableObjectListTest, testClear) {
  List l{mMocks[0], mMocks[1], mMocks[2]};
  EXPECT_EQ(3, l.count());
  l.clear();
  EXPECT_EQ(0, l.count());
}

TEST_F(SerializableObjectListTest, testSerialize) {
  std::unique_ptr<SExpression> e = SExpression::createList("list");
  List l{mMocks[0], mMocks[1], mMocks[2]};
  l.serialize(*e);
  // (list
  //  (test c2ceffd2-4cc5-43c6-941c-fc64a341d026
  //    (name "foo")
  //  )
  //  (test 4484ba9b-f3f8-4487-9109-10a8e9844fdc
  //   (name "bar")
  //  )
  //  (test 162bf1b0-f45e-4175-9656-33b5adc73ed0
  //   (name "pcb")
  //  )
  // )
  EXPECT_EQ(7, e->getChildCount());
  for (int i = 0; i < 3; ++i) {
    const SExpression& lineBreak = e->getChild(i * 2);
    EXPECT_EQ(SExpression::Type::LineBreak, lineBreak.getType());
    const SExpression& list = e->getChild(i * 2 + 1);
    EXPECT_EQ(SExpression::Type::List, list.getType());
    EXPECT_EQ("test", list.getName().toStdString());
    EXPECT_EQ(4, list.getChildCount());
    EXPECT_EQ(SExpression::Type::Token, list.getChild(0).getType());
    EXPECT_EQ(mMocks[i]->getUuid().toStr(), list.getChild(0).getValue());
    EXPECT_EQ(SExpression::Type::LineBreak, list.getChild(1).getType());
    EXPECT_EQ(SExpression::Type::List, list.getChild(2).getType());
    EXPECT_EQ("name", list.getChild(2).getName());
    EXPECT_EQ(1, list.getChild(2).getChildCount());
    EXPECT_EQ(mMocks[i]->getName(), list.getChild(2).getChild(0).getValue());
    EXPECT_EQ(SExpression::Type::LineBreak, list.getChild(3).getType());
  }
  EXPECT_EQ(SExpression::Type::LineBreak, e->getChild(6).getType());
}

TEST_F(SerializableObjectListTest, testSerializeEmpty) {
  std::unique_ptr<SExpression> e = SExpression::createList("list");
  List l;
  l.serialize(*e);
  // (list
  // )
  EXPECT_EQ(1, e->getChildCount());
  EXPECT_EQ(SExpression::Type::LineBreak, e->getChild(0).getType());
}

TEST_F(SerializableObjectListTest, testOperatorEqual) {
  EXPECT_TRUE(List() == List());
  EXPECT_TRUE(List({mMocks[0], mMocks[1]}) == List({mMocks[0], mMocks[1]}));
  EXPECT_TRUE(List({mMocks[0], mMocks[1]}) ==
              List({std::make_shared<Mock>(*mMocks[0]),
                    std::make_shared<Mock>(*mMocks[1])}));
  EXPECT_FALSE(List({mMocks[0], mMocks[1]}) == List({mMocks[0], mMocks[2]}));
  EXPECT_FALSE(List({mMocks[0]}) == List({mMocks[0], mMocks[1]}));
}

TEST_F(SerializableObjectListTest, testOperatorUnequal) {
  EXPECT_FALSE(List() != List());
  EXPECT_FALSE(List({mMocks[0], mMocks[1]}) != List({mMocks[0], mMocks[1]}));
  EXPECT_FALSE(List({mMocks[0], mMocks[1]}) !=
               List({std::make_shared<Mock>(*mMocks[0]),
                     std::make_shared<Mock>(*mMocks[1])}));
  EXPECT_TRUE(List({mMocks[0], mMocks[1]}) != List({mMocks[0], mMocks[2]}));
  EXPECT_TRUE(List({mMocks[0]}) != List({mMocks[0], mMocks[1]}));
}

TEST_F(SerializableObjectListTest, testOperatorAssign) {
  List l1{mMocks[0], mMocks[1]};
  List l2{mMocks[2]};
  l2 = l1;
  EXPECT_EQ(2, l1.count());
  EXPECT_EQ(2, l2.count());
  EXPECT_EQ(mMocks[0], l1[0]);
  EXPECT_EQ(mMocks[1], l1[1]);
  EXPECT_NE(mMocks[0], l2[0]);  // pointers have changed...
  EXPECT_NE(mMocks[1], l2[1]);
  EXPECT_EQ(*mMocks[0], *l2[0]);  // ...but values not!
  EXPECT_EQ(*mMocks[1], *l2[1]);
}

TEST_F(SerializableObjectListTest, testOperatorMove) {
  List l1{mMocks[0], mMocks[1]};
  List l2{mMocks[2]};
  l2 = std::move(l1);
  EXPECT_EQ(0, l1.count());
  EXPECT_EQ(2, l2.count());
  EXPECT_EQ(mMocks[0], l2[0]);
  EXPECT_EQ(mMocks[1], l2[1]);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace librepcb
