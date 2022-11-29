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
#include "schematic.h"

#include "../../application.h"
#include "../../exceptions.h"
#include "../../geometry/polygon.h"
#include "../../graphics/graphicsscene.h"
#include "../../library/sym/symbolpin.h"
#include "../../serialization/sexpression.h"
#include "../../types/gridproperties.h"
#include "../../utils/scopeguardlist.h"
#include "../project.h"
#include "items/si_netlabel.h"
#include "items/si_netline.h"
#include "items/si_netpoint.h"
#include "items/si_netsegment.h"
#include "items/si_polygon.h"
#include "items/si_symbol.h"
#include "items/si_symbolpin.h"
#include "items/si_text.h"
#include "schematicselectionquery.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

Schematic::Schematic(Project& project,
                     std::unique_ptr<TransactionalDirectory> directory,
                     const QString& directoryName, const Version& fileFormat,
                     bool create, const QString& newName)
  : QObject(&project),
    AttributeProvider(),
    mProject(project),
    mDirectoryName(directoryName),
    mDirectory(std::move(directory)),
    mIsAddedToProject(false),
    mUuid(Uuid::createRandom()),
    mName("New Page") {
  if (mDirectoryName.isEmpty()) {
    throw LogicError(__FILE__, __LINE__);
  }

  try {
    mGraphicsScene.reset(new GraphicsScene());

    // try to open/create the schematic file
    if (create) {
      // set attributes
      mName = ElementName(newName);  // can throw

      // load default grid properties
      mGridProperties.reset(new GridProperties());
    } else {
      const QString fp = "schematic.lp";
      const SExpression root =
          SExpression::parse(mDirectory->read(fp), mDirectory->getAbsPath(fp));

      // the schematic seems to be ready to open, so we will create all needed
      // objects

      mUuid = deserialize<Uuid>(root.getChild("@0"), fileFormat);
      mName = deserialize<ElementName>(root.getChild("name/@0"), fileFormat);

      // Load grid properties
      mGridProperties.reset(
          new GridProperties(root.getChild("grid"), fileFormat));

      // Load all symbols
      foreach (const SExpression& node, root.getChildren("symbol")) {
        SI_Symbol* symbol = new SI_Symbol(*this, node, fileFormat);
        if (mSymbols.contains(symbol->getUuid())) {
          throw RuntimeError(
              __FILE__, __LINE__,
              QString("There is already a symbol with the UUID \"%1\"!")
                  .arg(symbol->getUuid().toStr()));
        }
        mSymbols.insert(symbol->getUuid(), symbol);
      }

      // Load all netsegments
      foreach (const SExpression& node, root.getChildren("netsegment")) {
        SI_NetSegment* netsegment = new SI_NetSegment(*this, node, fileFormat);
        if (mNetSegments.contains(netsegment->getUuid())) {
          throw RuntimeError(
              __FILE__, __LINE__,
              QString("There is already a netsegment with the UUID \"%1\"!")
                  .arg(netsegment->getUuid().toStr()));
        }
        mNetSegments.insert(netsegment->getUuid(), netsegment);
      }

      // Load all polygons
      // Note: Support for polygons was added in file format v0.2. However,
      // there is no need to check the file format here - v0.1 simply doesn't
      // contain polygons.
      foreach (const SExpression& node, root.getChildren("polygon")) {
        SI_Polygon* polygon = new SI_Polygon(*this, node, fileFormat);
        if (mPolygons.contains(polygon->getUuid())) {
          throw RuntimeError(
              __FILE__, __LINE__,
              QString("There is already a polygon with the UUID \"%1\"!")
                  .arg(polygon->getUuid().toStr()));
        }
        mPolygons.insert(polygon->getUuid(), polygon);
      }

      // Load all texts
      // Note: Support for texts was added in file format v0.2. However, there
      // is no need to check the file format here - v0.1 simply doesn't contain
      // texts.
      foreach (const SExpression& node, root.getChildren("text")) {
        SI_Text* text = new SI_Text(*this, node, fileFormat);
        if (mTexts.contains(text->getUuid())) {
          throw RuntimeError(
              __FILE__, __LINE__,
              QString("There is already a text with the UUID \"%1\"!")
                  .arg(text->getUuid().toStr()));
        }
        mTexts.insert(text->getUuid(), text);
      }
    }

    // emit the "attributesChanged" signal when the project has emitted it
    connect(&mProject, &Project::attributesChanged, this,
            &Schematic::attributesChanged);
  } catch (...) {
    // free the allocated memory in the reverse order of their allocation...
    qDeleteAll(mTexts);
    mTexts.clear();
    qDeleteAll(mPolygons);
    mPolygons.clear();
    qDeleteAll(mNetSegments);
    mNetSegments.clear();
    qDeleteAll(mSymbols);
    mSymbols.clear();
    mGridProperties.reset();
    mGraphicsScene.reset();
    throw;  // ...and rethrow the exception
  }
}

Schematic::~Schematic() noexcept {
  Q_ASSERT(!mIsAddedToProject);

  // delete all items
  qDeleteAll(mTexts);
  mTexts.clear();
  qDeleteAll(mPolygons);
  mPolygons.clear();
  qDeleteAll(mNetSegments);
  mNetSegments.clear();
  qDeleteAll(mSymbols);
  mSymbols.clear();

  mGridProperties.reset();
  mGraphicsScene.reset();
}

/*******************************************************************************
 *  Getters: General
 ******************************************************************************/

bool Schematic::isEmpty() const noexcept {
  return (mSymbols.isEmpty() && mNetSegments.isEmpty() && mPolygons.isEmpty() &&
          mTexts.isEmpty());
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void Schematic::setGridProperties(const GridProperties& grid) noexcept {
  *mGridProperties = grid;
}

void Schematic::setName(const ElementName& name) noexcept {
  mName = name;
  emit mProject.attributesChanged();
}

/*******************************************************************************
 *  Symbol Methods
 ******************************************************************************/

void Schematic::addSymbol(SI_Symbol& symbol) {
  if ((!mIsAddedToProject) || (mSymbols.values().contains(&symbol)) ||
      (&symbol.getSchematic() != this)) {
    throw LogicError(__FILE__, __LINE__);
  }
  if (mSymbols.contains(symbol.getUuid())) {
    throw RuntimeError(
        __FILE__, __LINE__,
        QString("There is already a symbol with the UUID \"%1\"!")
            .arg(symbol.getUuid().toStr()));
  }
  symbol.addToSchematic();  // can throw
  mSymbols.insert(symbol.getUuid(), &symbol);
  emit symbolAdded(symbol);
}

void Schematic::removeSymbol(SI_Symbol& symbol) {
  if ((!mIsAddedToProject) || (mSymbols.value(symbol.getUuid()) != &symbol)) {
    throw LogicError(__FILE__, __LINE__);
  }
  symbol.removeFromSchematic();  // can throw
  mSymbols.remove(symbol.getUuid());
  emit symbolRemoved(symbol);
}

/*******************************************************************************
 *  NetSegment Methods
 ******************************************************************************/

void Schematic::addNetSegment(SI_NetSegment& netsegment) {
  if ((!mIsAddedToProject) || (mNetSegments.values().contains(&netsegment)) ||
      (&netsegment.getSchematic() != this)) {
    throw LogicError(__FILE__, __LINE__);
  }
  if (mNetSegments.contains(netsegment.getUuid())) {
    throw RuntimeError(
        __FILE__, __LINE__,
        QString("There is already a netsegment with the UUID \"%1\"!")
            .arg(netsegment.getUuid().toStr()));
  }
  netsegment.addToSchematic();  // can throw
  mNetSegments.insert(netsegment.getUuid(), &netsegment);
}

void Schematic::removeNetSegment(SI_NetSegment& netsegment) {
  if ((!mIsAddedToProject) ||
      (mNetSegments.value(netsegment.getUuid()) != &netsegment)) {
    throw LogicError(__FILE__, __LINE__);
  }
  netsegment.removeFromSchematic();  // can throw
  mNetSegments.remove(netsegment.getUuid());
}

/*******************************************************************************
 *  Polygon Methods
 ******************************************************************************/

void Schematic::addPolygon(SI_Polygon& polygon) {
  if ((!mIsAddedToProject) || (mPolygons.values().contains(&polygon)) ||
      (&polygon.getSchematic() != this)) {
    throw LogicError(__FILE__, __LINE__);
  }
  if (mPolygons.contains(polygon.getUuid())) {
    throw RuntimeError(
        __FILE__, __LINE__,
        QString("There is already a polygon with the UUID \"%1\"!")
            .arg(polygon.getUuid().toStr()));
  }
  polygon.addToSchematic();  // can throw
  mPolygons.insert(polygon.getUuid(), &polygon);
}

void Schematic::removePolygon(SI_Polygon& polygon) {
  if ((!mIsAddedToProject) ||
      (mPolygons.value(polygon.getUuid()) != &polygon)) {
    throw LogicError(__FILE__, __LINE__);
  }
  polygon.removeFromSchematic();  // can throw
  mPolygons.remove(polygon.getUuid());
}

/*******************************************************************************
 *  Text Methods
 ******************************************************************************/

void Schematic::addText(SI_Text& text) {
  if ((!mIsAddedToProject) || (mTexts.values().contains(&text)) ||
      (&text.getSchematic() != this)) {
    throw LogicError(__FILE__, __LINE__);
  }
  if (mTexts.contains(text.getUuid())) {
    throw RuntimeError(__FILE__, __LINE__,
                       QString("There is already a text with the UUID \"%1\"!")
                           .arg(text.getUuid().toStr()));
  }
  text.addToSchematic();  // can throw
  mTexts.insert(text.getUuid(), &text);
}

void Schematic::removeText(SI_Text& text) {
  if ((!mIsAddedToProject) || (mTexts.value(text.getUuid()) != &text)) {
    throw LogicError(__FILE__, __LINE__);
  }
  text.removeFromSchematic();  // can throw
  mTexts.remove(text.getUuid());
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void Schematic::addToProject() {
  if (mIsAddedToProject) {
    throw LogicError(__FILE__, __LINE__);
  }

  ScopeGuardList sgl(mSymbols.count() + mNetSegments.count() +
                     mPolygons.count() + mTexts.count());
  foreach (SI_Symbol* symbol, mSymbols) {
    symbol->addToSchematic();  // can throw
    sgl.add([symbol]() { symbol->removeFromSchematic(); });
  }
  foreach (SI_NetSegment* segment, mNetSegments) {
    segment->addToSchematic();  // can throw
    sgl.add([segment]() { segment->removeFromSchematic(); });
  }
  foreach (SI_Polygon* polygon, mPolygons) {
    polygon->addToSchematic();  // can throw
    sgl.add([polygon]() { polygon->removeFromSchematic(); });
  }
  foreach (SI_Text* text, mTexts) {
    text->addToSchematic();  // can throw
    sgl.add([text]() { text->removeFromSchematic(); });
  }

  // Move directory atomically (last step which could throw an exception).
  if (mDirectory->getFileSystem() != mProject.getDirectory().getFileSystem()) {
    TransactionalDirectory dst(mProject.getDirectory(),
                               "schematics/" % mDirectoryName);
    mDirectory->moveTo(dst);  // can throw
  }

  mIsAddedToProject = true;
  updateIcon();
  sgl.dismiss();
}

void Schematic::removeFromProject() {
  if (!mIsAddedToProject) {
    throw LogicError(__FILE__, __LINE__);
  }

  ScopeGuardList sgl(mSymbols.count() + mNetSegments.count() +
                     mPolygons.count() + mTexts.count());
  foreach (SI_Text* text, mTexts) {
    text->removeFromSchematic();  // can throw
    sgl.add([text]() { text->addToSchematic(); });
  }
  foreach (SI_Polygon* polygon, mPolygons) {
    polygon->removeFromSchematic();  // can throw
    sgl.add([polygon]() { polygon->addToSchematic(); });
  }
  foreach (SI_NetSegment* segment, mNetSegments) {
    segment->removeFromSchematic();  // can throw
    sgl.add([segment]() { segment->addToSchematic(); });
  }
  foreach (SI_Symbol* symbol, mSymbols) {
    symbol->removeFromSchematic();  // can throw
    sgl.add([symbol]() { symbol->addToSchematic(); });
  }

  // Move directory atomically (last step which could throw an exception).
  TransactionalDirectory tmp;
  mDirectory->moveTo(tmp);  // can throw

  mIsAddedToProject = false;
  sgl.dismiss();
}

void Schematic::save() {
  SExpression root = SExpression::createList("librepcb_schematic");
  root.appendChild(mUuid);
  root.ensureLineBreak();
  root.appendChild("name", mName);
  root.ensureLineBreak();
  mGridProperties->serialize(root.appendList("grid"));
  root.ensureLineBreak();
  for (const SI_Symbol* obj : mSymbols) {
    root.ensureLineBreak();
    obj->serialize(root.appendList("symbol"));
  }
  root.ensureLineBreak();
  for (const SI_NetSegment* obj : mNetSegments) {
    root.ensureLineBreak();
    obj->serialize(root.appendList("netsegment"));
  }
  root.ensureLineBreak();
  for (const SI_Polygon* obj : mPolygons) {
    root.ensureLineBreak();
    obj->getPolygon().serialize(root.appendList("polygon"));
  }
  root.ensureLineBreak();
  for (const SI_Text* obj : mTexts) {
    root.ensureLineBreak();
    obj->getText().serialize(root.appendList("text"));
  }
  root.ensureLineBreak();
  mDirectory->write("schematic.lp", root.toByteArray());
}

void Schematic::selectAll() noexcept {
  foreach (SI_Symbol* symbol, mSymbols) { symbol->setSelected(true); }
  foreach (SI_NetSegment* segment, mNetSegments) { segment->selectAll(); }
  foreach (SI_Polygon* polygon, mPolygons) { polygon->setSelected(true); }
  foreach (SI_Text* text, mTexts) { text->setSelected(true); }
}

void Schematic::setSelectionRect(const Point& p1, const Point& p2,
                                 bool updateItems) noexcept {
  mGraphicsScene->setSelectionRect(p1, p2);
  if (updateItems) {
    QRectF rectPx = QRectF(p1.toPxQPointF(), p2.toPxQPointF()).normalized();
    foreach (SI_Symbol* symbol, mSymbols) {
      bool selectSymbol = symbol->getGrabAreaScenePx().intersects(rectPx);
      symbol->setSelected(selectSymbol);
      foreach (SI_SymbolPin* pin, symbol->getPins()) {
        bool selectPin = pin->getGrabAreaScenePx().intersects(rectPx);
        pin->setSelected(selectSymbol || selectPin);
      }
    }
    foreach (SI_NetSegment* segment, mNetSegments) {
      segment->setSelectionRect(rectPx);
    }
    foreach (SI_Polygon* polygon, mPolygons) {
      bool select = polygon->getGrabAreaScenePx().intersects(rectPx);
      polygon->setSelected(select);
    }
    foreach (SI_Text* text, mTexts) {
      bool selectText = text->getGrabAreaScenePx().intersects(rectPx);
      text->setSelected(selectText);
    }
  }
}

void Schematic::clearSelection() const noexcept {
  foreach (SI_Symbol* symbol, mSymbols) { symbol->setSelected(false); }
  foreach (SI_NetSegment* segment, mNetSegments) { segment->clearSelection(); }
  foreach (SI_Polygon* polygon, mPolygons) { polygon->setSelected(false); }
  foreach (SI_Text* text, mTexts) { text->setSelected(false); }
}

void Schematic::updateAllNetLabelAnchors() noexcept {
  foreach (SI_NetSegment* netsegment, mNetSegments) {
    netsegment->updateAllNetLabelAnchors();
  }
}

std::unique_ptr<SchematicSelectionQuery> Schematic::createSelectionQuery() const
    noexcept {
  return std::unique_ptr<SchematicSelectionQuery>(new SchematicSelectionQuery(
      mSymbols, mNetSegments, mPolygons, mTexts, const_cast<Schematic*>(this)));
}

/*******************************************************************************
 *  Inherited from AttributeProvider
 ******************************************************************************/

QString Schematic::getBuiltInAttributeValue(const QString& key) const noexcept {
  if (key == QLatin1String("SHEET")) {
    return *mName;
  } else if (key == QLatin1String("PAGE")) {
    return QString::number(mProject.getSchematicIndex(*this) + 1);
  } else {
    return QString();
  }
}

QVector<const AttributeProvider*> Schematic::getAttributeProviderParents() const
    noexcept {
  return QVector<const AttributeProvider*>{&mProject};
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void Schematic::updateIcon() noexcept {
  mIcon = QIcon(mGraphicsScene->toPixmap(QSize(297, 210), Qt::white));
}

/*******************************************************************************
 *  Static Methods
 ******************************************************************************/

Schematic* Schematic::create(Project& project,
                             std::unique_ptr<TransactionalDirectory> directory,
                             const QString& directoryName,
                             const ElementName& name) {
  return new Schematic(project, std::move(directory), directoryName,
                       qApp->getFileFormatVersion(), true, *name);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
