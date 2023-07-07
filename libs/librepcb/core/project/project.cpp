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
#include "project.h"

#include "../application.h"
#include "../exceptions.h"
#include "../fileio/directorylock.h"
#include "../fileio/fileutils.h"
#include "../fileio/versionfile.h"
#include "../font/strokefontpool.h"
#include "../serialization/sexpression.h"
#include "board/board.h"
#include "board/items/bi_polygon.h"
#include "circuit/circuit.h"
#include "circuit/netclass.h"
#include "projectlibrary.h"
#include "schematic/schematic.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

Project::Project(std::unique_ptr<TransactionalDirectory> directory,
                 const QString& filename)
  : QObject(nullptr),
    mDirectory(std::move(directory)),
    mFilename(filename),
    mUuid(Uuid::createRandom()),
    mName("Unnamed"),
    mAuthor(),
    mVersion(),
    mCreated(QDateTime::currentDateTime()),
    mDateTime(QDateTime::currentDateTime()),
    mLocaleOrder(),
    mNormOrder(),
    mCustomBomAttributes(),
    mPrimaryBoard(nullptr) {
  // Check if the file extension is correct
  if (!mFilename.endsWith(".lpp")) {
    throw RuntimeError(__FILE__, __LINE__,
                       tr("The suffix of the project file must be \"lpp\"!"));
  }

  // Load stroke fonts.
  mStrokeFontPool.reset(new StrokeFontPool(
      TransactionalDirectory(*mDirectory, "resources/fontobene")));

  // Load project library.
  mProjectLibrary.reset(
      new ProjectLibrary(std::unique_ptr<TransactionalDirectory>(
          new TransactionalDirectory(*mDirectory, "library"))));

  // Initialize circuit.
  mCircuit.reset(new Circuit(*this));
}

Project::~Project() noexcept {
  // free the allocated memory in the reverse order of their allocation

  // delete all boards and schematics (and catch all thrown exceptions)
  foreach (Board* board, mBoards) {
    try {
      removeBoard(*board, true);
    } catch (...) {
    }
  }
  qDeleteAll(mRemovedBoards);
  mRemovedBoards.clear();
  foreach (Schematic* schematic, mSchematics) {
    try {
      removeSchematic(*schematic, true);
    } catch (...) {
    }
  }
  qDeleteAll(mRemovedSchematics);
  mRemovedSchematics.clear();

  qDebug().nospace() << "Closed project " << getFilepath().toNative() << ".";
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void Project::setUuid(const Uuid& newUuid) noexcept {
  if (newUuid != mUuid) {
    mUuid = newUuid;
    emit attributesChanged();
  }
}

void Project::setName(const ElementName& newName) noexcept {
  if (newName != mName) {
    mName = newName;
    emit attributesChanged();
  }
}

void Project::setAuthor(const QString& newAuthor) noexcept {
  if (newAuthor != mAuthor) {
    mAuthor = newAuthor;
    emit attributesChanged();
  }
}

void Project::setVersion(const QString& newVersion) noexcept {
  if (newVersion != mVersion) {
    mVersion = newVersion;
    emit attributesChanged();
  }
}

void Project::setCreated(const QDateTime& newCreated) noexcept {
  if (newCreated != mCreated) {
    mCreated = newCreated;
    emit attributesChanged();
  }
}

void Project::updateDateTime() noexcept {
  mDateTime = QDateTime::currentDateTime();
  emit attributesChanged();
}

void Project::setAttributes(const AttributeList& newAttributes) noexcept {
  if (newAttributes != mAttributes) {
    mAttributes = newAttributes;
    emit attributesChanged();
  }
}

void Project::setLocaleOrder(const QStringList& newLocales) noexcept {
  if (newLocales != mLocaleOrder) {
    mLocaleOrder = newLocales;
    emit attributesChanged();
  }
}

void Project::setNormOrder(const QStringList& newNorms) noexcept {
  if (newNorms != mNormOrder) {
    mNormOrder = newNorms;
    emit attributesChanged();
    emit normOrderChanged();
  }
}

void Project::setCustomBomAttributes(const QStringList& newKeys) noexcept {
  if (newKeys != mCustomBomAttributes) {
    mCustomBomAttributes = newKeys;
  }
}

bool Project::setErcMessageApprovals(
    const QSet<SExpression>& approvals) noexcept {
  if (approvals != mErcMessageApprovals) {
    mErcMessageApprovals = approvals;
    emit ercMessageApprovalsChanged(mErcMessageApprovals);
    return true;
  } else {
    return false;
  }
}

/*******************************************************************************
 *  Schematic Methods
 ******************************************************************************/

int Project::getSchematicIndex(const Schematic& schematic) const noexcept {
  return mSchematics.indexOf(const_cast<Schematic*>(&schematic));
}

Schematic* Project::getSchematicByUuid(const Uuid& uuid) const noexcept {
  foreach (Schematic* schematic, mSchematics) {
    if (schematic->getUuid() == uuid) return schematic;
  }
  return nullptr;
}

Schematic* Project::getSchematicByName(const QString& name) const noexcept {
  foreach (Schematic* schematic, mSchematics) {
    if (schematic->getName() == name) return schematic;
  }
  return nullptr;
}

void Project::addSchematic(Schematic& schematic, int newIndex) {
  if ((mSchematics.contains(&schematic)) || (&schematic.getProject() != this)) {
    throw LogicError(__FILE__, __LINE__);
  }
  if (getSchematicByUuid(schematic.getUuid())) {
    throw RuntimeError(
        __FILE__, __LINE__,
        QString("There is already a schematic with the UUID \"%1\"!")
            .arg(schematic.getUuid().toStr()));
  }
  if (getSchematicByName(*schematic.getName())) {
    throw RuntimeError(__FILE__, __LINE__,
                       tr("There is already a schematic with the name \"%1\"!")
                           .arg(*schematic.getName()));
  }
  foreach (const Schematic* s, mSchematics) {
    if (s->getDirectoryName() == schematic.getDirectoryName()) {
      throw RuntimeError(
          __FILE__, __LINE__,
          tr("There is already a schematic with the directory name \"%1\"!")
              .arg(schematic.getDirectoryName()));
    }
  }

  if ((newIndex < 0) || (newIndex > mSchematics.count())) {
    newIndex = mSchematics.count();
  }

  schematic.addToProject();  // can throw
  mSchematics.insert(newIndex, &schematic);

  if (mRemovedSchematics.contains(&schematic)) {
    mRemovedSchematics.removeOne(&schematic);
  }

  emit schematicAdded(newIndex);
  emit attributesChanged();
}

void Project::removeSchematic(Schematic& schematic, bool deleteSchematic) {
  if ((!mSchematics.contains(&schematic)) ||
      (mRemovedSchematics.contains(&schematic))) {
    throw LogicError(__FILE__, __LINE__);
  }
  if ((!deleteSchematic) && (!schematic.isEmpty())) {
    throw RuntimeError(__FILE__, __LINE__,
                       tr("There are still elements in the schematic \"%1\"!")
                           .arg(*schematic.getName()));
  }

  int index = getSchematicIndex(schematic);
  Q_ASSERT(index >= 0);

  schematic.removeFromProject();  // can throw
  mSchematics.removeAt(index);

  emit schematicRemoved(index);
  emit attributesChanged();

  if (deleteSchematic) {
    delete &schematic;
  } else {
    mRemovedSchematics.append(&schematic);
  }
}

/*******************************************************************************
 *  Board Methods
 ******************************************************************************/

int Project::getBoardIndex(const Board& board) const noexcept {
  return mBoards.indexOf(const_cast<Board*>(&board));
}

Board* Project::getBoardByUuid(const Uuid& uuid) const noexcept {
  foreach (Board* board, mBoards) {
    if (board->getUuid() == uuid) return board;
  }
  return nullptr;
}

Board* Project::getBoardByName(const QString& name) const noexcept {
  foreach (Board* board, mBoards) {
    if (board->getName() == name) return board;
  }
  return nullptr;
}

void Project::addBoard(Board& board, int newIndex) {
  if ((mBoards.contains(&board)) || (&board.getProject() != this)) {
    throw LogicError(__FILE__, __LINE__);
  }
  if (getBoardByUuid(board.getUuid())) {
    throw RuntimeError(__FILE__, __LINE__,
                       QString("There is already a board with the UUID \"%1\"!")
                           .arg(board.getUuid().toStr()));
  }
  if (getBoardByName(*board.getName())) {
    throw RuntimeError(__FILE__, __LINE__,
                       tr("There is already a board with the name \"%1\"!")
                           .arg(*board.getName()));
  }
  foreach (const Board* b, mBoards) {
    if (b->getDirectoryName() == board.getDirectoryName()) {
      throw RuntimeError(
          __FILE__, __LINE__,
          tr("There is already a board with the directory name \"%1\"!")
              .arg(board.getDirectoryName()));
    }
  }

  if ((newIndex < 0) || (newIndex > mBoards.count())) {
    newIndex = mBoards.count();
  }

  board.addToProject();  // can throw
  mBoards.insert(newIndex, &board);

  if (mRemovedBoards.contains(&board)) {
    mRemovedBoards.removeOne(&board);
  }

  emit boardAdded(newIndex);
  updatePrimaryBoard();
  emit attributesChanged();
}

void Project::removeBoard(Board& board, bool deleteBoard) {
  if ((!mBoards.contains(&board)) || (mRemovedBoards.contains(&board))) {
    throw LogicError(__FILE__, __LINE__);
  }

  int index = getBoardIndex(board);
  Q_ASSERT(index >= 0);

  board.removeFromProject();  // can throw
  mBoards.removeAt(index);

  emit boardRemoved(index);
  updatePrimaryBoard();
  emit attributesChanged();

  if (deleteBoard) {
    delete &board;
  } else {
    mRemovedBoards.append(&board);
  }
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void Project::save() {
  qDebug() << "Save project files to transactional file system...";

  // Version file.
  mDirectory->write(
      ".librepcb-project",
      VersionFile(Application::getFileFormatVersion()).toByteArray());

  // Project file.
  mDirectory->write(mFilename, "LIBREPCB-PROJECT");

  // Metadata.
  {
    SExpression root = SExpression::createList("librepcb_project_metadata");
    root.appendChild(mUuid);
    root.ensureLineBreak();
    root.appendChild("name", mName);
    root.ensureLineBreak();
    root.appendChild("author", mAuthor);
    root.ensureLineBreak();
    root.appendChild("version", mVersion);
    root.ensureLineBreak();
    root.appendChild("created", mCreated);
    root.ensureLineBreak();
    mAttributes.serialize(root);
    root.ensureLineBreak();
    mDirectory->write("project/metadata.lp", root.toByteArray());
  }

  // Settings.
  {
    SExpression root = SExpression::createList("librepcb_project_settings");
    root.ensureLineBreak();
    {
      SExpression& node = root.appendList("library_locale_order");
      foreach (const QString& locale, mLocaleOrder) {
        node.ensureLineBreak();
        node.appendChild("locale", locale);
      }
      node.ensureLineBreak();
    }
    root.ensureLineBreak();
    {
      SExpression& node = root.appendList("library_norm_order");
      foreach (const QString& norm, mNormOrder) {
        node.ensureLineBreak();
        node.appendChild("norm", norm);
      }
      node.ensureLineBreak();
    }
    root.ensureLineBreak();
    {
      SExpression& node = root.appendList("custom_bom_attributes");
      foreach (const QString& key, mCustomBomAttributes) {
        node.ensureLineBreak();
        node.appendChild("attribute", key);
      }
      node.ensureLineBreak();
    }
    root.ensureLineBreak();
    mDirectory->write("project/settings.lp", root.toByteArray());
  }

  // Circuit.
  {
    SExpression root = SExpression::createList("librepcb_circuit");
    mCircuit->serialize(root);
    mDirectory->write("circuit/circuit.lp", root.toByteArray());
  }

  // ERC.
  {
    SExpression root = SExpression::createList("librepcb_erc");
    foreach (const SExpression& node,
             Toolbox::sortedQSet(mErcMessageApprovals)) {
      root.ensureLineBreak();
      root.appendChild(node);
    }
    root.ensureLineBreak();
    mDirectory->write("circuit/erc.lp", root.toByteArray());
  }

  // Schematics.
  {
    SExpression root = SExpression::createList("librepcb_schematics");
    foreach (Schematic* schematic, mSchematics) {
      root.ensureLineBreak();
      root.appendChild(
          "schematic",
          "schematics/" + schematic->getDirectoryName() + "/schematic.lp");
      schematic->save();
    }
    root.ensureLineBreak();
    mDirectory->write("schematics/schematics.lp", root.toByteArray());
  }

  // Boards.
  {
    SExpression root = SExpression::createList("librepcb_boards");
    foreach (Board* board, mBoards) {
      root.ensureLineBreak();
      root.appendChild("board",
                       "boards/" + board->getDirectoryName() + "/board.lp");
      board->save();
    }
    root.ensureLineBreak();
    mDirectory->write("boards/boards.lp", root.toByteArray());
  }

  // Update the datetime attribute of the project.
  updateDateTime();
}

/*******************************************************************************
 *  Static Methods
 ******************************************************************************/

std::unique_ptr<Project> Project::create(
    std::unique_ptr<TransactionalDirectory> directory,
    const QString& filename) {
  Q_ASSERT(directory);
  qDebug().nospace() << "Create project "
                     << directory->getAbsPath(filename).toNative() << "...";

  // Check if there isn't already a project in the selected directory.
  if (directory->fileExists(".librepcb-project") ||
      directory->fileExists(filename)) {
    throw RuntimeError(
        __FILE__, __LINE__,
        QString(tr("The directory \"%1\" already contains a LibrePCB project."))
            .arg(directory->getAbsPath().toNative()));
  }

  // Populate with stroke fonts.
  TransactionalDirectory fontobeneDir(*directory, "resources/fontobene");
  FilePath src = Application::getResourcesDir().getPathTo("fontobene");
  foreach (const FilePath& fp,
           FileUtils::getFilesInDirectory(src, {"*.bene"})) {
    if (fp.getSuffix() == "bene") {
      fontobeneDir.write(fp.getFilename(), FileUtils::readFile(fp));
    }
  }

  // Create empty project.
  std::unique_ptr<Project> p(new Project(std::move(directory), filename));

  // Add default netclass with name "default".
  {
    NetClass* netclass = new NetClass(p->getCircuit(), Uuid::createRandom(),
                                      ElementName("default"));
    p->getCircuit().addNetClass(*netclass);
  }

  // Done!
  return p;
}

bool Project::isFilePathInsideProjectDirectory(const FilePath& fp) noexcept {
  FilePath parent = fp.getParentDir();
  if (isProjectDirectory(parent)) {
    return true;
  } else if (parent.isValid() && !parent.isRoot()) {
    return isFilePathInsideProjectDirectory(parent);
  } else {
    return false;
  }
}

bool Project::isProjectFile(const FilePath& file) noexcept {
  return file.getSuffix() == "lpp" && file.isExistingFile() &&
      isProjectDirectory(file.getParentDir());
}

bool Project::isProjectDirectory(const FilePath& dir) noexcept {
  return dir.getPathTo(".librepcb-project").isExistingFile();
}

Version Project::getProjectFileFormatVersion(const FilePath& dir) {
  QByteArray content = FileUtils::readFile(dir.getPathTo(".librepcb-project"));
  VersionFile file = VersionFile::fromByteArray(content);
  return file.getVersion();
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void Project::updatePrimaryBoard() {
  Board* primary = mBoards.value(0);
  if (mPrimaryBoard != primary) {
    mPrimaryBoard = primary;
    primaryBoardChanged(mPrimaryBoard);
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
