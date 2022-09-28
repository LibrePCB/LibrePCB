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
#include "items/si_symbol.h"
#include "items/si_symbolpin.h"
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
                     const Version& fileFormat, bool create,
                     const QString& newName)
  : QObject(&project),
    AttributeProvider(),
    mProject(project),
    mDirectory(std::move(directory)),
    mIsAddedToProject(false),
    mUuid(Uuid::createRandom()),
    mName("New Page") {
  try {
    mGraphicsScene.reset(new GraphicsScene());

    // try to open/create the schematic file
    if (create) {
      // set attributes
      mName = ElementName(newName);  // can throw

      // load default grid properties
      mGridProperties.reset(new GridProperties());
    } else {
      SExpression root = SExpression::parse(
          mDirectory->read(getFilePath().getFilename()), getFilePath());

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
        if (getSymbolByUuid(symbol->getUuid())) {
          throw RuntimeError(
              __FILE__, __LINE__,
              QString("There is already a symbol with the UUID \"%1\"!")
                  .arg(symbol->getUuid().toStr()));
        }
        mSymbols.append(symbol);
      }

      // Load all netsegments
      foreach (const SExpression& node, root.getChildren("netsegment")) {
        SI_NetSegment* netsegment = new SI_NetSegment(*this, node, fileFormat);
        if (getNetSegmentByUuid(netsegment->getUuid())) {
          throw RuntimeError(
              __FILE__, __LINE__,
              QString("There is already a netsegment with the UUID \"%1\"!")
                  .arg(netsegment->getUuid().toStr()));
        }
        mNetSegments.append(netsegment);
      }
    }

    // emit the "attributesChanged" signal when the project has emitted it
    connect(&mProject, &Project::attributesChanged, this,
            &Schematic::attributesChanged);
  } catch (...) {
    // free the allocated memory in the reverse order of their allocation...
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

FilePath Schematic::getFilePath() const noexcept {
  return mDirectory->getAbsPath("schematic.lp");
}

bool Schematic::isEmpty() const noexcept {
  return (mSymbols.isEmpty() && mNetSegments.isEmpty());
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

SI_Symbol* Schematic::getSymbolByUuid(const Uuid& uuid) const noexcept {
  foreach (SI_Symbol* symbol, mSymbols) {
    if (symbol->getUuid() == uuid) return symbol;
  }
  return nullptr;
}

void Schematic::addSymbol(SI_Symbol& symbol) {
  if ((!mIsAddedToProject) || (mSymbols.contains(&symbol)) ||
      (&symbol.getSchematic() != this)) {
    throw LogicError(__FILE__, __LINE__);
  }
  // check if there is no symbol with the same uuid in the list
  if (getSymbolByUuid(symbol.getUuid())) {
    throw RuntimeError(
        __FILE__, __LINE__,
        QString("There is already a symbol with the UUID \"%1\"!")
            .arg(symbol.getUuid().toStr()));
  }
  // add to schematic
  symbol.addToSchematic();  // can throw
  mSymbols.append(&symbol);
  emit symbolAdded(symbol);
}

void Schematic::removeSymbol(SI_Symbol& symbol) {
  if ((!mIsAddedToProject) || (!mSymbols.contains(&symbol))) {
    throw LogicError(__FILE__, __LINE__);
  }
  // remove from schematic
  symbol.removeFromSchematic();  // can throw
  mSymbols.removeOne(&symbol);
  emit symbolRemoved(symbol);
}

/*******************************************************************************
 *  NetSegment Methods
 ******************************************************************************/

SI_NetSegment* Schematic::getNetSegmentByUuid(const Uuid& uuid) const noexcept {
  foreach (SI_NetSegment* netsegment, mNetSegments) {
    if (netsegment->getUuid() == uuid) return netsegment;
  }
  return nullptr;
}

void Schematic::addNetSegment(SI_NetSegment& netsegment) {
  if ((!mIsAddedToProject) || (mNetSegments.contains(&netsegment)) ||
      (&netsegment.getSchematic() != this)) {
    throw LogicError(__FILE__, __LINE__);
  }
  // check if there is no netsegment with the same uuid in the list
  if (getNetSegmentByUuid(netsegment.getUuid())) {
    throw RuntimeError(
        __FILE__, __LINE__,
        QString("There is already a netsegment with the UUID \"%1\"!")
            .arg(netsegment.getUuid().toStr()));
  }
  // add to schematic
  netsegment.addToSchematic();  // can throw
  mNetSegments.append(&netsegment);
}

void Schematic::removeNetSegment(SI_NetSegment& netsegment) {
  if ((!mIsAddedToProject) || (!mNetSegments.contains(&netsegment))) {
    throw LogicError(__FILE__, __LINE__);
  }
  // remove from schematic
  netsegment.removeFromSchematic();  // can throw
  mNetSegments.removeOne(&netsegment);
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void Schematic::addToProject() {
  if (mIsAddedToProject) {
    throw LogicError(__FILE__, __LINE__);
  }

  ScopeGuardList sgl(mSymbols.count() + mNetSegments.count());
  foreach (SI_Symbol* symbol, mSymbols) {
    symbol->addToSchematic();  // can throw
    sgl.add([symbol]() { symbol->removeFromSchematic(); });
  }
  foreach (SI_NetSegment* segment, mNetSegments) {
    segment->addToSchematic();  // can throw
    sgl.add([segment]() { segment->removeFromSchematic(); });
  }

  mIsAddedToProject = true;
  updateIcon();
  sgl.dismiss();
}

void Schematic::removeFromProject() {
  if (!mIsAddedToProject) {
    throw LogicError(__FILE__, __LINE__);
  }

  ScopeGuardList sgl(mSymbols.count() + mNetSegments.count());
  foreach (SI_NetSegment* segment, mNetSegments) {
    segment->removeFromSchematic();  // can throw
    sgl.add([segment]() { segment->addToSchematic(); });
  }
  foreach (SI_Symbol* symbol, mSymbols) {
    symbol->removeFromSchematic();  // can throw
    sgl.add([symbol]() { symbol->addToSchematic(); });
  }

  mIsAddedToProject = false;
  sgl.dismiss();
}

void Schematic::save() {
  if (mIsAddedToProject) {
    // save schematic file
    SExpression doc(serializeToDomElement("librepcb_schematic"));  // can throw
    mDirectory->write(getFilePath().getFilename(),
                      doc.toByteArray());  // can throw
  } else {
    mDirectory->removeDirRecursively();  // can throw
  }
}

void Schematic::selectAll() noexcept {
  foreach (SI_Symbol* symbol, mSymbols) { symbol->setSelected(true); }
  foreach (SI_NetSegment* segment, mNetSegments) { segment->selectAll(); }
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
  }
}

void Schematic::clearSelection() const noexcept {
  foreach (SI_Symbol* symbol, mSymbols) { symbol->setSelected(false); }
  foreach (SI_NetSegment* segment, mNetSegments) { segment->clearSelection(); }
}

void Schematic::updateAllNetLabelAnchors() noexcept {
  foreach (SI_NetSegment* netsegment, mNetSegments) {
    netsegment->updateAllNetLabelAnchors();
  }
}

std::unique_ptr<SchematicSelectionQuery> Schematic::createSelectionQuery() const
    noexcept {
  return std::unique_ptr<SchematicSelectionQuery>(new SchematicSelectionQuery(
      mSymbols, mNetSegments, const_cast<Schematic*>(this)));
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

void Schematic::serialize(SExpression& root) const {
  root.appendChild(mUuid);
  root.ensureLineBreak();
  root.appendChild("name", mName);
  root.ensureLineBreak();
  root.appendChild(mGridProperties->serializeToDomElement("grid"));
  root.ensureEmptyLine();
  serializePointerContainerUuidSorted(root, mSymbols, "symbol");
  root.ensureEmptyLine();
  serializePointerContainerUuidSorted(root, mNetSegments, "netsegment");
  root.ensureEmptyLine();
}

/*******************************************************************************
 *  Static Methods
 ******************************************************************************/

Schematic* Schematic::create(Project& project,
                             std::unique_ptr<TransactionalDirectory> directory,
                             const ElementName& name) {
  return new Schematic(project, std::move(directory),
                       qApp->getFileFormatVersion(), true, *name);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
