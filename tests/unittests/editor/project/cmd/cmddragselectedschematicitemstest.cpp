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

#include "../../../testhelpers.h"

#include <gtest/gtest.h>
#include <librepcb/core/fileio/transactionaldirectory.h>
#include <librepcb/core/fileio/transactionalfilesystem.h>
#include <librepcb/core/library/cmp/component.h>
#include <librepcb/core/library/cmp/componentpinsignalmap.h>
#include <librepcb/core/library/cmp/componentsignal.h>
#include <librepcb/core/library/sym/symbol.h>
#include <librepcb/core/project/circuit/circuit.h>
#include <librepcb/core/project/circuit/componentinstance.h>
#include <librepcb/core/project/circuit/componentsignalinstance.h>
#include <librepcb/core/project/circuit/netclass.h>
#include <librepcb/core/project/circuit/netsignal.h>
#include <librepcb/core/project/project.h>
#include <librepcb/core/project/projectlibrary.h>
#include <librepcb/core/project/schematic/items/si_netline.h>
#include <librepcb/core/project/schematic/items/si_netpoint.h>
#include <librepcb/core/project/schematic/items/si_netsegment.h>
#include <librepcb/core/project/schematic/items/si_symbol.h>
#include <librepcb/core/project/schematic/items/si_symbolpin.h>
#include <librepcb/core/project/schematic/schematic.h>
#include <librepcb/editor/graphics/graphicslayer.h>
#include <librepcb/editor/graphics/graphicslayerlist.h>
#include <librepcb/editor/project/cmd/cmddragselectedschematicitems.h>
#include <librepcb/editor/project/projectcrossprobe.h>
#include <librepcb/editor/project/schematic/graphicsitems/sgi_symbol.h>
#include <librepcb/editor/project/schematic/schematicgraphicsscene.h>

#include <QtCore>

#include <memory>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {
namespace tests {

using ::librepcb::tests::TestHelpers;

/*******************************************************************************
 *  Test Class
 ******************************************************************************/

class CmdDragSelectedSchematicItemsTest : public ::testing::Test {
protected:
  CmdDragSelectedSchematicItemsTest()
    : mUuidFactory(TestHelpers::createDeterministicUuidFactory()),
      mSymbolUuid(mUuidFactory()),
      mPinUuid(mUuidFactory()),
      mComponentUuid(mUuidFactory()),
      mSignalUuid(mUuidFactory()),
      mVariantUuid(mUuidFactory()),
      mGateUuid(mUuidFactory()),
      mIgnorePlacementLocks(false) {}

  ~CmdDragSelectedSchematicItemsTest() noexcept override {
    mScene.reset();
    mLayers.reset();
    mProject.reset();
    if (mProjectDir.isValid()) {
      QDir(mProjectDir.toStr()).removeRecursively();
    }
  }

  Uuid uuid() { return mUuidFactory(); }

  static Length mm(qreal value) { return Length::fromMm(value); }

  static Point point(qreal x, qreal y) { return Point(mm(x), mm(y)); }

  void createProject() {
    mProjectDir = FilePath::getRandomTempPath();
    mProject =
        Project::create(std::make_unique<TransactionalDirectory>(
                            TransactionalFileSystem::openRW(mProjectDir)),
                        "project.lpp", mUuidFactory);

    auto symbol = std::make_unique<Symbol>(
        mSymbolUuid, Version::fromString("1.0"), "LibrePCB Tests",
        TestHelpers::createDeterministicDateTime(), ElementName("Symbol"), "",
        "");
    symbol->getPins().append(std::make_shared<SymbolPin>(
        mPinUuid, CircuitIdentifier("P"), Point(), UnsignedLength(0),
        Angle::deg0(), Point(), Angle::deg0(), PositiveLength(1000000),
        Alignment(HAlign::left(), VAlign::center())));
    mSymbol = symbol.get();
    mProject->getLibrary().addSymbol(*symbol.release());

    auto component = std::make_unique<Component>(
        mComponentUuid, Version::fromString("1.0"), "LibrePCB Tests",
        TestHelpers::createDeterministicDateTime(), ElementName("Component"),
        "", "");
    component->getSignals().append(std::make_shared<ComponentSignal>(
        mSignalUuid, CircuitIdentifier("SIG"), SignalRole::passive(), "", false,
        false, false));
    auto variant = std::make_shared<ComponentSymbolVariant>(
        mVariantUuid, "", ElementName("default"), "");
    auto gate = std::make_shared<ComponentSymbolVariantItem>(
        mGateUuid, mSymbolUuid, Point(), Angle::deg0(), true,
        ComponentSymbolVariantItemSuffix(""));
    gate->getPinSignalMap().append(std::make_shared<ComponentPinSignalMapItem>(
        mPinUuid, std::optional<Uuid>(mSignalUuid),
        CmpSigPinDisplayType::componentSignal()));
    variant->getSymbolItems().append(gate);
    component->getSymbolVariants().append(variant);
    mComponent = component.get();
    mProject->getLibrary().addComponent(*component.release());

    auto schematic = std::make_unique<Schematic>(
        *mProject, std::make_unique<TransactionalDirectory>(), "schematic",
        uuid(), ElementName("Schematic"));
    mSchematic = schematic.get();
    mProject->addSchematic(*schematic.release());
  }

  ComponentInstance& addComponentInstance(const CircuitIdentifier& name) {
    ComponentInstance* instance = new ComponentInstance(
        mProject->getCircuit(), uuid(), *mComponent, mVariantUuid, name);
    mProject->getCircuit().addComponentInstance(*instance);
    return *instance;
  }

  SI_Symbol& addSymbol(ComponentInstance& component, const Point& position) {
    SI_Symbol* symbol = new SI_Symbol(*mSchematic, uuid(), component, mGateUuid,
                                      position, Angle::deg0(), false, false);
    mSchematic->addSymbol(*symbol);
    return *symbol;
  }

  NetSignal& addNetSignal(const CircuitIdentifier& name) {
    NetClass* netclass = mProject->getCircuit().getNetClasses().first();
    NetSignal* netSignal =
        new NetSignal(mProject->getCircuit(), uuid(), *netclass, name, false);
    mProject->getCircuit().addNetSignal(*netSignal);
    return *netSignal;
  }

  SI_NetSegment& addNetSegment(NetSignal& netSignal) {
    SI_NetSegment* segment = new SI_NetSegment(*mSchematic, uuid(), netSignal);
    mSchematic->addNetSegment(*segment);
    return *segment;
  }

  SI_SymbolPin& pin(SI_Symbol& symbol) const {
    return *symbol.getPin(mPinUuid);
  }

  void connect(ComponentInstance& component, NetSignal& netSignal) {
    component.getSignalInstance(mSignalUuid)->setNetSignal(&netSignal);
  }

  void createScene() {
    mLayers = GraphicsLayerList::schematicLayers(nullptr);
    mSceneContext = std::make_shared<SchematicGraphicsScene::Context>(
        SchematicGraphicsScene::Context{
            nullptr, std::make_shared<ProjectCrossProbe>(),
            GraphicsLayer::State::Enabled, mIgnorePlacementLocks});
    mScene = std::make_unique<SchematicGraphicsScene>(*mSchematic, *mLayers,
                                                      mSceneContext);
  }

  void select(SI_Symbol& symbol) {
    mScene->getSymbols().value(&symbol)->setSelected(true);
  }

  static bool isOrthogonal(const SI_NetLine& line) {
    return (line.getP1().getPosition().getX() ==
            line.getP2().getPosition().getX()) ||
        (line.getP1().getPosition().getY() ==
         line.getP2().getPosition().getY());
  }

  static ::testing::AssertionResult allNetLinesOrthogonal(
      const SI_NetSegment& segment) {
    foreach (SI_NetLine* line, segment.getNetLines()) {
      if (!isOrthogonal(*line)) {
        return ::testing::AssertionFailure()
            << "Netline " << line->getUuid().toStr().toStdString()
            << " is diagonal";
      }
    }
    return ::testing::AssertionSuccess();
  }

  static int netPointCountAt(const SI_NetSegment& segment, const Point& pos) {
    int count = 0;
    foreach (SI_NetPoint* netPoint, segment.getNetPoints()) {
      if (netPoint->getPosition() == pos) {
        ++count;
      }
    }
    return count;
  }

  std::function<Uuid()> mUuidFactory;
  Uuid mSymbolUuid;
  Uuid mPinUuid;
  Uuid mComponentUuid;
  Uuid mSignalUuid;
  Uuid mVariantUuid;
  Uuid mGateUuid;
  FilePath mProjectDir;
  std::unique_ptr<Project> mProject;
  Symbol* mSymbol = nullptr;
  Component* mComponent = nullptr;
  Schematic* mSchematic = nullptr;
  bool mIgnorePlacementLocks;
  std::unique_ptr<GraphicsLayerList> mLayers;
  std::shared_ptr<SchematicGraphicsScene::Context> mSceneContext;
  std::unique_ptr<SchematicGraphicsScene> mScene;
};

/*******************************************************************************
 *  Test Methods
 ******************************************************************************/

TEST_F(CmdDragSelectedSchematicItemsTest,
       testDragDirectPinToPinNetlineCreatesOrthogonalBend) {
  createProject();
  ComponentInstance& c1 = addComponentInstance(CircuitIdentifier("U1"));
  ComponentInstance& c2 = addComponentInstance(CircuitIdentifier("U2"));
  NetSignal& net = addNetSignal(CircuitIdentifier("N"));
  connect(c1, net);
  connect(c2, net);
  SI_Symbol& s1 = addSymbol(c1, point(0, 0));
  SI_Symbol& s2 = addSymbol(c2, point(10, 0));
  SI_NetSegment& segment = addNetSegment(net);
  SI_NetLine* line = new SI_NetLine(segment, uuid(), pin(s1), pin(s2),
                                    SI_NetLine::getDefaultWidth());
  segment.addNetPointsAndNetLines({}, {line});
  createScene();
  select(s1);

  CmdDragSelectedSchematicItems cmd(*mScene, Point());
  cmd.setCurrentPosition(point(0, 5));
  EXPECT_TRUE(cmd.execute());

  EXPECT_EQ(1, segment.getNetPoints().count());
  EXPECT_EQ(2, segment.getNetLines().count());
  EXPECT_TRUE(allNetLinesOrthogonal(segment));

  cmd.undo();
  EXPECT_EQ(0, segment.getNetPoints().count());
  EXPECT_EQ(1, segment.getNetLines().count());
  EXPECT_TRUE(allNetLinesOrthogonal(segment));

  cmd.redo();
  EXPECT_EQ(1, segment.getNetPoints().count());
  EXPECT_EQ(2, segment.getNetLines().count());
  EXPECT_TRUE(allNetLinesOrthogonal(segment));
}

TEST_F(CmdDragSelectedSchematicItemsTest,
       testDragThroughNetPointCreatesOrthogonalBendAtFixedPin) {
  createProject();
  ComponentInstance& c1 = addComponentInstance(CircuitIdentifier("U1"));
  ComponentInstance& c2 = addComponentInstance(CircuitIdentifier("U2"));
  NetSignal& net = addNetSignal(CircuitIdentifier("N"));
  connect(c1, net);
  connect(c2, net);
  SI_Symbol& s1 = addSymbol(c1, point(0, 0));
  SI_Symbol& s2 = addSymbol(c2, point(20, 0));
  SI_NetSegment& segment = addNetSegment(net);
  SI_NetPoint* netPoint = new SI_NetPoint(segment, uuid(), point(10, 0));
  SI_NetLine* firstLine = new SI_NetLine(segment, uuid(), pin(s1), *netPoint,
                                         SI_NetLine::getDefaultWidth());
  SI_NetLine* secondLine = new SI_NetLine(segment, uuid(), *netPoint, pin(s2),
                                          SI_NetLine::getDefaultWidth());
  segment.addNetPointsAndNetLines({netPoint}, {firstLine, secondLine});
  createScene();
  select(s1);

  CmdDragSelectedSchematicItems cmd(*mScene, Point());
  cmd.setCurrentPosition(point(0, 5.08));
  EXPECT_TRUE(cmd.execute());

  EXPECT_EQ(2, segment.getNetPoints().count());
  EXPECT_EQ(3, segment.getNetLines().count());
  EXPECT_EQ(point(10, 5.08), netPoint->getPosition());
  EXPECT_EQ(1, netPointCountAt(segment, point(10, 0)));
  EXPECT_EQ(0, netPointCountAt(segment, point(20, 5.08)));
  EXPECT_TRUE(allNetLinesOrthogonal(segment));

  cmd.undo();
  EXPECT_EQ(1, segment.getNetPoints().count());
  EXPECT_EQ(2, segment.getNetLines().count());
  EXPECT_TRUE(allNetLinesOrthogonal(segment));

  cmd.redo();
  EXPECT_EQ(2, segment.getNetPoints().count());
  EXPECT_EQ(3, segment.getNetLines().count());
  EXPECT_TRUE(allNetLinesOrthogonal(segment));
}

TEST_F(CmdDragSelectedSchematicItemsTest,
       testDragThroughVerticalNetPointKeepsUpperSegmentFixed) {
  createProject();
  ComponentInstance& c1 = addComponentInstance(CircuitIdentifier("U1"));
  ComponentInstance& c2 = addComponentInstance(CircuitIdentifier("U2"));
  NetSignal& net = addNetSignal(CircuitIdentifier("N"));
  connect(c1, net);
  connect(c2, net);
  SI_Symbol& s1 = addSymbol(c1, point(0, 0));
  SI_Symbol& s2 = addSymbol(c2, point(0, 20));
  SI_NetSegment& segment = addNetSegment(net);
  SI_NetPoint* netPoint = new SI_NetPoint(segment, uuid(), point(0, 10));
  SI_NetLine* firstLine = new SI_NetLine(segment, uuid(), pin(s1), *netPoint,
                                         SI_NetLine::getDefaultWidth());
  SI_NetLine* secondLine = new SI_NetLine(segment, uuid(), *netPoint, pin(s2),
                                          SI_NetLine::getDefaultWidth());
  segment.addNetPointsAndNetLines({netPoint}, {firstLine, secondLine});
  createScene();
  select(s1);

  CmdDragSelectedSchematicItems cmd(*mScene, Point());
  cmd.setCurrentPosition(point(5.08, 0));
  EXPECT_TRUE(cmd.execute());

  EXPECT_EQ(2, segment.getNetPoints().count());
  EXPECT_EQ(3, segment.getNetLines().count());
  EXPECT_EQ(point(5.08, 10), netPoint->getPosition());
  EXPECT_EQ(1, netPointCountAt(segment, point(0, 10)));
  EXPECT_EQ(0, netPointCountAt(segment, point(5.08, 20)));
  EXPECT_TRUE(allNetLinesOrthogonal(segment));
}

TEST_F(CmdDragSelectedSchematicItemsTest, testNoDragDiscardsCleanly) {
  createProject();
  ComponentInstance& c1 = addComponentInstance(CircuitIdentifier("U1"));
  ComponentInstance& c2 = addComponentInstance(CircuitIdentifier("U2"));
  NetSignal& net = addNetSignal(CircuitIdentifier("N"));
  connect(c1, net);
  connect(c2, net);
  SI_Symbol& s1 = addSymbol(c1, point(0, 0));
  SI_Symbol& s2 = addSymbol(c2, point(10, 0));
  SI_NetSegment& segment = addNetSegment(net);
  SI_NetPoint* netPoint = new SI_NetPoint(segment, uuid(), point(5, 0));
  SI_NetLine* firstLine = new SI_NetLine(segment, uuid(), pin(s1), *netPoint,
                                         SI_NetLine::getDefaultWidth());
  SI_NetLine* secondLine = new SI_NetLine(segment, uuid(), *netPoint, pin(s2),
                                          SI_NetLine::getDefaultWidth());
  segment.addNetPointsAndNetLines({netPoint}, {firstLine, secondLine});
  createScene();
  select(s1);

  CmdDragSelectedSchematicItems cmd(*mScene, Point());
  cmd.setCurrentPosition(Point());
  EXPECT_FALSE(cmd.execute());

  EXPECT_EQ(1, segment.getNetPoints().count());
  EXPECT_EQ(2, segment.getNetLines().count());
  EXPECT_TRUE(allNetLinesOrthogonal(segment));
}

TEST_F(CmdDragSelectedSchematicItemsTest,
       testRotateDuringDragDiscardsStretchEdits) {
  createProject();
  ComponentInstance& c1 = addComponentInstance(CircuitIdentifier("U1"));
  ComponentInstance& c2 = addComponentInstance(CircuitIdentifier("U2"));
  NetSignal& net = addNetSignal(CircuitIdentifier("N"));
  connect(c1, net);
  connect(c2, net);
  SI_Symbol& s1 = addSymbol(c1, point(0, 0));
  SI_Symbol& s2 = addSymbol(c2, point(10, 0));
  SI_NetSegment& segment = addNetSegment(net);
  SI_NetPoint* netPoint = new SI_NetPoint(segment, uuid(), point(5, 0));
  SI_NetLine* firstLine = new SI_NetLine(segment, uuid(), pin(s1), *netPoint,
                                         SI_NetLine::getDefaultWidth());
  SI_NetLine* secondLine = new SI_NetLine(segment, uuid(), *netPoint, pin(s2),
                                          SI_NetLine::getDefaultWidth());
  segment.addNetPointsAndNetLines({netPoint}, {firstLine, secondLine});
  createScene();
  select(s1);

  CmdDragSelectedSchematicItems cmd(*mScene, Point());
  cmd.setCurrentPosition(point(0, 5.08));
  cmd.rotate(Angle::deg90(), false);
  EXPECT_TRUE(cmd.execute());

  EXPECT_EQ(point(5, 0), netPoint->getPosition());
  EXPECT_EQ(1, segment.getNetPoints().count());
  EXPECT_EQ(2, segment.getNetLines().count());
}

TEST_F(CmdDragSelectedSchematicItemsTest,
       testDragReturnToAxisRemovesPreviewBend) {
  createProject();
  ComponentInstance& c1 = addComponentInstance(CircuitIdentifier("U1"));
  ComponentInstance& c2 = addComponentInstance(CircuitIdentifier("U2"));
  NetSignal& net = addNetSignal(CircuitIdentifier("N"));
  connect(c1, net);
  connect(c2, net);
  SI_Symbol& s1 = addSymbol(c1, point(0, 0));
  SI_Symbol& s2 = addSymbol(c2, point(10, 0));
  SI_NetSegment& segment = addNetSegment(net);
  SI_NetLine* line = new SI_NetLine(segment, uuid(), pin(s1), pin(s2),
                                    SI_NetLine::getDefaultWidth());
  segment.addNetPointsAndNetLines({}, {line});
  createScene();
  select(s1);

  CmdDragSelectedSchematicItems cmd(*mScene, Point());
  cmd.setCurrentPosition(point(0, 5.08));
  cmd.setCurrentPosition(point(2.54, 0));
  EXPECT_TRUE(cmd.execute());

  EXPECT_EQ(0, segment.getNetPoints().count());
  EXPECT_EQ(1, segment.getNetLines().count());
  EXPECT_TRUE(allNetLinesOrthogonal(segment));
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace editor
}  // namespace librepcb
