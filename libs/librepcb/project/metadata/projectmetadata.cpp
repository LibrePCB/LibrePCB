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
#include <librepcb/common/fileio/smartxmlfile.h>
#include <librepcb/common/fileio/domdocument.h>
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
    mXmlFilepath(project.getPath().getPathTo("core/metadata.xml"))
{
    qDebug() << "load project metadata...";
    Q_ASSERT(!(create && (restore || readOnly)));

    if (create) {
        mXmlFile.reset(SmartXmlFile::create(mXmlFilepath));

        mName = mProject.getFilepath().getCompleteBasename();
        mAuthor = SystemInfo::getFullUsername();
        mVersion = "v1";
        mCreated = QDateTime::currentDateTime();
    } else {
        mXmlFile.reset(new SmartXmlFile(mXmlFilepath, restore, readOnly));
        std::unique_ptr<DomDocument> doc = mXmlFile->parseFileAndBuildDomTree();
        DomElement& root = doc->getRoot();

        mName = root.getFirstChild("name", true)->getText<QString>(false);
        mAuthor = root.getFirstChild("author", true)->getText<QString>(false);
        mVersion = root.getFirstChild("version", true)->getText<QString>(false);
        mCreated = root.getFirstChild("created", true)->getText<QDateTime>(true);
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
        DomDocument doc(*serializeToDomElement("metadata"));
        mXmlFile->save(doc, toOriginal);
    } catch (const Exception& e) {
        success = false;
        errors.append(e.getMsg());
    }

    return success;
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

void ProjectMetadata::serialize(DomElement& root) const
{
    if (!checkAttributesValidity()) throw LogicError(__FILE__, __LINE__);

    root.appendTextChild("name", mName);
    root.appendTextChild("author", mAuthor);
    root.appendTextChild("version", mVersion);
    root.appendTextChild("created", mCreated);
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
