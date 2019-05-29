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
#include "projectmetadata.h"

#include <librepcb/common/fileio/sexpression.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace project {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

ProjectMetadata::ProjectMetadata(const Uuid& uuid, const ElementName& name,
                                 const QString& author, const QString& version,
                                 const QDateTime& created,
                                 const QDateTime& lastModified)
  : QObject(nullptr),
    mUuid(uuid),
    mName(name),
    mAuthor(author),
    mVersion(version),
    mCreated(created),
    mLastModified(lastModified) {
}

ProjectMetadata::ProjectMetadata(const SExpression& node)
  : QObject(nullptr), mUuid(Uuid::createRandom()), mName("Project") {
  qDebug() << "load project metadata...";

  mUuid    = node.getChildByIndex(0).getValue<Uuid>();
  mName    = node.getValueByPath<ElementName>("name");
  mAuthor  = node.getValueByPath<QString>("author");
  mVersion = node.getValueByPath<QString>("version");
  mCreated = node.getValueByPath<QDateTime>("created");
  mAttributes.loadFromSExpression(node);  // can throw

  mLastModified = QDateTime::currentDateTime();

  qDebug() << "metadata successfully loaded!";
}

ProjectMetadata::~ProjectMetadata() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void ProjectMetadata::setName(const ElementName& newName) noexcept {
  if (newName != mName) {
    mName = newName;
    emit attributesChanged();
  }
}

void ProjectMetadata::setAuthor(const QString& newAuthor) noexcept {
  if (newAuthor != mAuthor) {
    mAuthor = newAuthor;
    emit attributesChanged();
  }
}

void ProjectMetadata::setVersion(const QString& newVersion) noexcept {
  if (newVersion != mVersion) {
    mVersion = newVersion;
    emit attributesChanged();
  }
}

void ProjectMetadata::setAttributes(
    const AttributeList& newAttributes) noexcept {
  if (newAttributes != mAttributes) {
    mAttributes = newAttributes;
    emit attributesChanged();
  }
}

void ProjectMetadata::updateLastModified() noexcept {
  mLastModified = QDateTime::currentDateTime();
  emit attributesChanged();
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void ProjectMetadata::serialize(SExpression& root) const {
  root.appendChild(mUuid);
  root.appendChild("name", mName, true);
  root.appendChild("author", mAuthor, true);
  root.appendChild("version", mVersion, true);
  root.appendChild("created", mCreated, true);
  mAttributes.serialize(root);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace project
}  // namespace librepcb
