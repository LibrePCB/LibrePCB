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

#include "boards/board.h"
#include "circuit/circuit.h"
#include "erc/ercmsglist.h"
#include "library/projectlibrary.h"
#include "metadata/projectmetadata.h"
#include "schematics/schematic.h"
#include "schematics/schematiclayerprovider.h"
#include "settings/projectsettings.h"

#include <librepcb/common/application.h>
#include <librepcb/common/exceptions.h>
#include <librepcb/common/fileio/directorylock.h>
#include <librepcb/common/fileio/fileutils.h>
#include <librepcb/common/fileio/sexpression.h>
#include <librepcb/common/fileio/versionfile.h>
#include <librepcb/common/font/strokefontpool.h>

#include <QPrinter>
#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace project {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

Project::Project(std::unique_ptr<TransactionalDirectory> directory,
                 const QString& filename, bool create)
  : QObject(nullptr),
    AttributeProvider(),
    mDirectory(std::move(directory)),
    mFilename(filename) {
  qDebug() << (create ? "create project:" : "open project:")
           << getFilepath().toNative();

  // Check if the file extension is correct
  if (!mFilename.endsWith(".lpp")) {
    throw RuntimeError(__FILE__, __LINE__,
                       tr("The suffix of the project file must be \"lpp\"!"));
  }

  if (create) {
    // Check if there isn't already a project in the selected directory
    if (mDirectory->fileExists(".librepcb-project") ||
        mDirectory->fileExists(mFilename)) {
      throw RuntimeError(
          __FILE__, __LINE__,
          QString(
              tr("The directory \"%1\" already contains a LibrePCB project."))
              .arg(getPath().toNative()));
    }
  } else {
    // check if the project does exist
    if (!mDirectory->fileExists(".librepcb-project")) {
      throw RuntimeError(
          __FILE__, __LINE__,
          QString(
              tr("The directory \"%1\" does not contain a LibrePCB project."))
              .arg(getPath().toNative()));
    }
    if (!mDirectory->fileExists(mFilename)) {
      throw RuntimeError(
          __FILE__, __LINE__,
          tr("The file \"%1\" does not exist.").arg(getFilepath().toNative()));
    }
    // check the project's file format version
    Version version =
        VersionFile::fromByteArray(mDirectory->read(".librepcb-project"))
            .getVersion();
    if (version > qApp->getFileFormatVersion()) {
      throw RuntimeError(
          __FILE__, __LINE__,
          QString(
              tr("This project was created with a newer application version.\n"
                 "You need at least LibrePCB %1 to open it.\n\n%2"))
              .arg(version.toPrettyStr(3))
              .arg(getFilepath().toNative()));
    }
  }

  // OK - the project is locked (or read-only) and can be opened!
  // Until this line, there was no memory allocated on the heap. But in the rest
  // of the constructor, a lot of object will be created on the heap. If an
  // exception is thrown somewhere, we must ensure that all the allocated memory
  // gets freed. This is done by a try/catch block. In the catch-block, all
  // allocated memory will be freed. Then the exception is rethrown to leave the
  // constructor.

  try {
    // copy and/or load stroke fonts
    TransactionalDirectory fontobeneDir(*mDirectory, "resources/fontobene");
    if (create) {
      FilePath src = qApp->getResourcesFilePath("fontobene");
      foreach (const FilePath& fp,
               FileUtils::getFilesInDirectory(src, {"*.bene"})) {
        if (fp.getSuffix() == "bene") {
          fontobeneDir.write(fp.getFilename(), FileUtils::readFile(fp));
        }
      }
    }
    mStrokeFontPool.reset(new StrokeFontPool(fontobeneDir));

    // Create or load metadata
    if (create) {
      QString name = cleanElementName(getFilepath().getCompleteBasename());
      if (!ElementNameConstraint()(name)) {
        name = "New Project";  // fallback if the filename is not a valid name
      }
      mProjectMetadata.reset(new ProjectMetadata(
          Uuid::createRandom(), ElementName(name), tr("Unknown"), "v1",
          QDateTime::currentDateTime(), QDateTime::currentDateTime()));
    } else {
      QString fp = "project/metadata.lp";
      SExpression root =
          SExpression::parse(mDirectory->read(fp), mDirectory->getAbsPath(fp));
      mProjectMetadata.reset(new ProjectMetadata(root));
    }

    // Create all needed objects
    connect(mProjectMetadata.data(), &ProjectMetadata::attributesChanged, this,
            &Project::attributesChanged);
    mProjectSettings.reset(new ProjectSettings(*this, create));
    mProjectLibrary.reset(
        new ProjectLibrary(std::unique_ptr<TransactionalDirectory>(
            new TransactionalDirectory(*mDirectory, "library"))));
    mErcMsgList.reset(new ErcMsgList(*this));
    mCircuit.reset(new Circuit(*this, create));

    // Load all schematic layers
    mSchematicLayerProvider.reset(new SchematicLayerProvider(*this));

    // Load all schematics
    if (!create) {
      QString fp = "schematics/schematics.lp";
      SExpression schRoot =
          SExpression::parse(mDirectory->read(fp), mDirectory->getAbsPath(fp));
      foreach (const SExpression& node, schRoot.getChildren("schematic")) {
        FilePath fp =
            FilePath::fromRelative(getPath(), node.getChild("@0").getValue());
        std::unique_ptr<TransactionalDirectory> dir(new TransactionalDirectory(
            *mDirectory, fp.getParentDir().toRelative(getPath())));
        Schematic* schematic = new Schematic(*this, std::move(dir));
        addSchematic(*schematic);
      }
      qDebug() << mSchematics.count() << "schematics successfully loaded!";
    }

    // Load all boards
    if (!create) {
      QString fp = "boards/boards.lp";
      SExpression brdRoot =
          SExpression::parse(mDirectory->read(fp), mDirectory->getAbsPath(fp));
      foreach (const SExpression& node, brdRoot.getChildren("board")) {
        FilePath fp =
            FilePath::fromRelative(getPath(), node.getChild("@0").getValue());
        std::unique_ptr<TransactionalDirectory> dir(new TransactionalDirectory(
            *mDirectory, fp.getParentDir().toRelative(getPath())));
        Board* board = new Board(*this, std::move(dir));
        addBoard(*board);
      }
      qDebug() << mBoards.count() << "boards successfully loaded!";
    }

    // at this point, the whole circuit with all schematics and boards is
    // successfully loaded, so the ERC list now contains all the correct ERC
    // messages. So we can now restore the ignore state of each ERC message from
    // the file.
    mErcMsgList->restoreIgnoreState();  // can throw

    if (create) save();  // write all files to file system
  } catch (...) {
    // free the allocated memory in the reverse order of their allocation...
    foreach (Board* board, mBoards) {
      try {
        removeBoard(*board, true);
      } catch (...) {
      }
    }
    foreach (Schematic* schematic, mSchematics) {
      try {
        removeSchematic(*schematic, true);
      } catch (...) {
      }
    }
    throw;  // ...and rethrow the exception
  }

  // project successfully opened! :-)
  qDebug() << "project successfully loaded!";
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

  qDebug() << "closed project:" << getFilepath().toNative();
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

Schematic* Project::createSchematic(const ElementName& name) {
  QString dirname = FilePath::cleanFileName(
      *name, FilePath::ReplaceSpaces | FilePath::ToLowerCase);
  if (dirname.isEmpty()) {
    throw RuntimeError(__FILE__, __LINE__,
                       tr("Invalid schematic name: \"%1\"").arg(*name));
  }
  std::unique_ptr<TransactionalDirectory> dir(
      new TransactionalDirectory(*mDirectory, "schematics/" % dirname));
  if (dir->fileExists("schematic.lp")) {
    throw RuntimeError(__FILE__, __LINE__,
                       tr("The schematic exists already: \"%1\"")
                           .arg(dir->getAbsPath().toNative()));
  }
  return Schematic::create(*this, std::move(dir), name);
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

void Project::exportSchematicsAsPdf(const FilePath& filepath) {
  // Create output directory first because QPrinter silently fails if it doesn't
  // exist.
  FileUtils::makePath(filepath.getParentDir());  // can throw

  QPrinter printer(QPrinter::HighResolution);
  printer.setPaperSize(QPrinter::A4);
  printer.setOrientation(QPrinter::Landscape);
  printer.setOutputFormat(QPrinter::PdfFormat);
  printer.setCreator(QString("LibrePCB %1").arg(qApp->applicationVersion()));
  printer.setOutputFileName(filepath.toStr());

  QList<int> pages;
  for (int i = 0; i < mSchematics.count(); i++) pages.append(i);

  printSchematicPages(printer, pages);
}

void Project::printSchematicPages(QPrinter& printer, QList<int>& pages) {
  if (pages.isEmpty())
    throw RuntimeError(__FILE__, __LINE__, tr("No schematic pages selected."));

  QPainter painter(&printer);

  for (int i = 0; i < pages.count(); i++) {
    Schematic* schematic = getSchematicByIndex(pages[i]);
    if (!schematic) {
      throw RuntimeError(
          __FILE__, __LINE__,
          tr("No schematic page with the index %1 found.").arg(pages[i]));
    }
    schematic->clearSelection();
    schematic->renderToQPainter(painter);

    if (i != pages.count() - 1) {
      if (!printer.newPage()) {
        throw RuntimeError(__FILE__, __LINE__,
                           tr("Unknown error while printing."));
      }
    }
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

Board* Project::createBoard(const ElementName& name) {
  QString dirname = FilePath::cleanFileName(
      *name, FilePath::ReplaceSpaces | FilePath::ToLowerCase);
  if (dirname.isEmpty()) {
    throw RuntimeError(__FILE__, __LINE__,
                       tr("Invalid board name: \"%1\"").arg(*name));
  }
  std::unique_ptr<TransactionalDirectory> dir(
      new TransactionalDirectory(*mDirectory, "boards/" % dirname));
  if (dir->fileExists("board.lp")) {
    throw RuntimeError(__FILE__, __LINE__,
                       tr("The board exists already: \"%1\"")
                           .arg(dir->getAbsPath().toNative()));
  }
  return Board::create(*this, std::move(dir), name);
}

Board* Project::createBoard(const Board& other, const ElementName& name) {
  QString dirname = FilePath::cleanFileName(
      *name, FilePath::ReplaceSpaces | FilePath::ToLowerCase);
  if (dirname.isEmpty()) {
    throw RuntimeError(__FILE__, __LINE__,
                       tr("Invalid board name: \"%1\"").arg(*name));
  }
  std::unique_ptr<TransactionalDirectory> dir(
      new TransactionalDirectory(*mDirectory, "boards/" % dirname));
  if (dir->fileExists("board.lp")) {
    throw RuntimeError(__FILE__, __LINE__,
                       tr("The board exists already: \"%1\"")
                           .arg(dir->getAbsPath().toNative()));
  }
  return new Board(other, std::move(dir), name);
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

  if ((newIndex < 0) || (newIndex > mBoards.count())) {
    newIndex = mBoards.count();
  }

  board.addToProject();  // can throw
  mBoards.insert(newIndex, &board);

  if (mRemovedBoards.contains(&board)) {
    mRemovedBoards.removeOne(&board);
  }

  emit boardAdded(newIndex);
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

  // Save version file
  mDirectory->write(
      ".librepcb-project",
      VersionFile(qApp->getFileFormatVersion()).toByteArray());  // can throw

  // Save *.lpp project file
  mDirectory->write(mFilename, "LIBREPCB-PROJECT");  // can throw

  // Save project/metadata.lp
  mDirectory->write(
      "project/metadata.lp",
      mProjectMetadata->serializeToDomElement("librepcb_project_metadata")
          .toByteArray());  // can throw

  // Save settings
  mProjectSettings->save();  // can throw

  // Save library
  mProjectLibrary->save();  // can throw

  // Save circuit
  mCircuit->save();  // can throw

  // Save ERC messages list
  mErcMsgList->save();  // can throw

  // Save schematics/schematics.lp
  SExpression schRoot = SExpression::createList("librepcb_schematics");
  foreach (Schematic* schematic, mSchematics) {
    schRoot.appendChild("schematic",
                        schematic->getFilePath().toRelative(getPath()), true);
  }
  mDirectory->write("schematics/schematics.lp",
                    schRoot.toByteArray());  // can throw

  // Save boards/boards.lp
  SExpression brdRoot = SExpression::createList("librepcb_boards");
  foreach (Board* board, mBoards) {
    brdRoot.appendChild("board", board->getFilePath().toRelative(getPath()),
                        true);
  }
  mDirectory->write("boards/boards.lp", brdRoot.toByteArray());  // can throw

  // Save all removed schematics (*.lp files)
  foreach (Schematic* schematic, mRemovedSchematics) {
    schematic->save();  // can throw
  }
  // Save all added schematics (*.lp files)
  foreach (Schematic* schematic, mSchematics) {
    schematic->save();  // can throw
  }

  // Save all removed boards (*.lp files)
  foreach (Board* board, mRemovedBoards) {
    board->save();  // can throw
  }
  // Save all added boards (*.lp files)
  foreach (Board* board, mBoards) {
    board->save();  // can throw
  }

  // update the "last modified datetime" attribute of the project
  mProjectMetadata->updateLastModified();
}

/*******************************************************************************
 *  Inherited from AttributeProvider
 ******************************************************************************/

QString Project::getUserDefinedAttributeValue(const QString& key) const
    noexcept {
  if (const auto& attr = mProjectMetadata->getAttributes().find(key)) {
    return attr->getValueTr(true);
  } else {
    return QString();
  }
}

QString Project::getBuiltInAttributeValue(const QString& key) const noexcept {
  if (key == QLatin1String("PROJECT")) {
    return *mProjectMetadata->getName();
  } else if (key == QLatin1String("PROJECT_DIRPATH")) {
    return getPath().toNative();
  } else if (key == QLatin1String("PROJECT_BASENAME")) {
    return getFilepath().getBasename();
  } else if (key == QLatin1String("PROJECT_FILENAME")) {
    return getFilepath().getFilename();
  } else if (key == QLatin1String("PROJECT_FILEPATH")) {
    return getFilepath().toNative();
  } else if (key == QLatin1String("CREATED_DATE")) {
    return mProjectMetadata->getCreated().date().toString(Qt::ISODate);
  } else if (key == QLatin1String("CREATED_TIME")) {
    return mProjectMetadata->getCreated().time().toString(Qt::ISODate);
  } else if (key == QLatin1String("MODIFIED_DATE")) {
    return mProjectMetadata->getLastModified().date().toString(Qt::ISODate);
  } else if (key == QLatin1String("MODIFIED_TIME")) {
    return mProjectMetadata->getLastModified().time().toString(Qt::ISODate);
  } else if (key == QLatin1String("AUTHOR")) {
    return mProjectMetadata->getAuthor();
  } else if (key == QLatin1String("VERSION")) {
    return mProjectMetadata->getVersion();
  } else if (key == QLatin1String("PAGES")) {
    return QString::number(mSchematics.count());
  } else if (key == QLatin1String("PAGE_X_OF_Y")) {
    return "Page {{PAGE}} of {{PAGES}}";  // do not translate this, must be the
                                          // same for every user!
  } else {
    return QString();
  }
}

/*******************************************************************************
 *  Static Methods
 ******************************************************************************/

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
 *  End of File
 ******************************************************************************/

}  // namespace project
}  // namespace librepcb
