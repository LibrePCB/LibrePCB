/*
 * EDA4U - Professional EDA for everyone!
 * Copyright (C) 2013 Urban Bruhin
 * http://eda4u.ubruhin.ch/
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
#include <QDomDocument>
#include <QDomElement>
#include "xmlfile.h"
#include "../common/exceptions.h"
#include "../common/filepath.h"

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

XmlFile::XmlFile(const FilePath& filepath, bool restore,
                 const QString& rootName) throw (Exception) :
    QObject(0), mFilepath(filepath), mDomDocument()
{
    // decide if we open the original file (*.xml) or the backup (*.xml~)
    FilePath xmlFilepath(mFilepath.toStr() % '~');
    if ((!restore) || (!xmlFilepath.isExistingFile()))
        xmlFilepath = mFilepath;

    // check if the file exists
    if (!xmlFilepath.isExistingFile())
    {
        throw RuntimeError(QString("The file \"%1\" does not exist!")
                           .arg(xmlFilepath.toNative()), __FILE__, __LINE__);
    }

    // try opening the file
    QFile file(xmlFilepath.toStr());
    if (!file.open(QIODevice::ReadOnly))
    {
        throw RuntimeError(QString("Cannot open file \"%1\": %2")
            .arg(xmlFilepath.toNative(), file.errorString()), __FILE__, __LINE__);
    }

    // load XML into mDomDocument
    QString errMsg;
    int errLine;
    int errColumn;
    if (!mDomDocument.setContent(&file, &errMsg, &errLine, &errColumn))
    {
        throw RuntimeError(QString("Cannot read XML file \"%1\": %2 [%3:%4]").arg(
            xmlFilepath.toNative(), errMsg).arg(errLine, errColumn), __FILE__, __LINE__);
    }

    // check if the root node exists
    QDomElement rootNode = getRoot();
    if (rootNode.isNull())
    {
        throw RuntimeError(QString("No XML root node found in \"%1\"!")
                           .arg(xmlFilepath.toNative()), __FILE__, __LINE__);
    }

    // check the name of the root node, if desired
    if ((!rootName.isEmpty()) && (rootNode.tagName() != rootName))
    {
        throw RuntimeError(QString("XML root node in \"%1\" is \"%2\", but should be \"%3\"!")
            .arg(xmlFilepath.toNative(), rootNode.tagName(), rootName), __FILE__, __LINE__);
    }
}

XmlFile::~XmlFile()
{
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void XmlFile::save(bool toOriginal) throw (Exception)
{
    QString filepath = toOriginal ? mFilepath.toStr() : mFilepath.toStr() % '~';
    saveDomDocument(mDomDocument, FilePath(filepath));
}

/*****************************************************************************************
 *  Static Methods
 ****************************************************************************************/

void XmlFile::create(const FilePath& filepath, const QString& rootName) throw (Exception)
{
    QString xmlTmpl("<?xml version='1.0' encoding='UTF-8' standalone='yes'?>\n<%1/>");

    QDomDocument dom;
    QString errMsg;
    if (!dom.setContent(xmlTmpl.arg(rootName), &errMsg))
        throw LogicError("Could not set DOM content: " % errMsg, __FILE__, __LINE__);

    saveDomDocument(dom, filepath);
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

void XmlFile::saveDomDocument(const QDomDocument& domDocument,
                              const FilePath& filepath) throw (Exception)
{
    if (!filepath.getParentDir().mkPath())
        qWarning() << "could not make path for file" << filepath.toNative();

    QFile file(filepath.toStr());
    if (!file.open(QIODevice::WriteOnly))
    {
        throw RuntimeError(QString("Could not open or create file \"%1\": %2").arg(
                           filepath.toNative(), file.errorString()), __FILE__, __LINE__);
    }

    QByteArray content = domDocument.toByteArray(4);
    if (content.isEmpty())
        throw LogicError("XML DOM Document is empty!", __FILE__, __LINE__);

    qint64 written = file.write(content);
    if (written != content.size())
    {
        qCritical() << "only" << written << "of" << content.size() << "bytes written!";
        throw RuntimeError(QString("Could not write to file \"%1\": %2").arg(
                           filepath.toNative(), file.errorString()), __FILE__, __LINE__);
    }
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/
