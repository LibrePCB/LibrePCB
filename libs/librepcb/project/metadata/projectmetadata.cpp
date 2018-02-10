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
#include "projectmetadata.h"
#include <librepcb/common/systeminfo.h>
#include <librepcb/common/fileio/smartsexprfile.h>
#include <librepcb/common/fileio/sexpression.h>
#include "../project.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

ProjectMetadata::ProjectMetadata(Project& project, bool restore, bool readOnly, bool create) :
    QObject(nullptr), mProject(project),
    mFilepath(project.getPath().getPathTo("core/metadata.lp"))
{
    qDebug() << "load project metadata...";
    Q_ASSERT(!(create && (restore || readOnly)));

    if (create) {
        mFile.reset(SmartSExprFile::create(mFilepath));

        mUuid = Uuid::createRandom();
        mName = mProject.getFilepath().getCompleteBasename();
        mAuthor = SystemInfo::getFullUsername();
        mVersion = "v1";
        mCreated = QDateTime::currentDateTime();
    } else {
        mFile.reset(new SmartSExprFile(mFilepath, restore, readOnly));
        SExpression root = mFile->parseFileAndBuildDomTree();

        if (root.getChildByIndex(0).isString()) {
            mUuid = root.getChildByIndex(0).getValue<Uuid>(true);
        } else {
            // backward compatibility, remove this some time!
            mUuid = Uuid::createRandom();
        }
        mName = root.getValueByPath<QString>("name", false);
        mAuthor = root.getValueByPath<QString>("author", false);
        mVersion = root.getValueByPath<QString>("version", false);
        mCreated = root.getValueByPath<QDateTime>("created", true);
        mAttributes.loadFromDomElement(root); // can throw
    }

    mLastModified = QDateTime::currentDateTime();

    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);

    qDebug() << "metadata successfully loaded!";
}

ProjectMetadata::~ProjectMetadata() noexcept
{
}

/*****************************************************************************************
 *  Setters
 ****************************************************************************************/

void ProjectMetadata::setName(const QString& newName) noexcept
{
    if (newName != mName) {
        mName = newName;
        emit attributesChanged();
    }
}

void ProjectMetadata::setAuthor(const QString& newAuthor) noexcept
{
    if (newAuthor != mAuthor) {
        mAuthor = newAuthor;
        emit attributesChanged();
    }
}

void ProjectMetadata::setVersion(const QString& newVersion) noexcept
{
    if (newVersion != mVersion) {
        mVersion = newVersion;
        emit attributesChanged();
    }
}

void ProjectMetadata::setAttributes(const AttributeList& newAttributes) noexcept
{
    if (newAttributes != mAttributes) {
        mAttributes = newAttributes;
        emit attributesChanged();
    }
}

void ProjectMetadata::updateLastModified() noexcept
{
    mLastModified = QDateTime::currentDateTime();
    emit attributesChanged();
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

bool ProjectMetadata::save(bool toOriginal, QStringList& errors) noexcept
{
    bool success = true;

    try {
        SExpression doc(serializeToDomElement("librepcb_project_metadata"));
        mFile->save(doc, toOriginal);
    } catch (const Exception& e) {
        success = false;
        errors.append(e.getMsg());
    }

    return success;
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

void ProjectMetadata::serialize(SExpression& root) const
{
    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);

    root.appendToken(mUuid);
    root.appendStringChild("name", mName, true);
    root.appendStringChild("author", mAuthor, true);
    root.appendStringChild("version", mVersion, true);
    root.appendTokenChild("created", mCreated, true);
    mAttributes.serialize(root);
}

bool ProjectMetadata::checkAttributesValidity() const noexcept
{
    if (mName.isEmpty())    return false;
    return true;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb
