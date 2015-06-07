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
#include "symbol.h"
#include <eda4ucommon/fileio/xmldomelement.h>

namespace library {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

Symbol::Symbol(const QUuid& uuid, const Version& version, const QString& author,
               const QString& name_en_US, const QString& description_en_US,
               const QString& keywords_en_US) throw (Exception) :
    LibraryElement("symbol", uuid, version, author, name_en_US, description_en_US, keywords_en_US)
{
}

Symbol::Symbol(const FilePath& xmlFilePath) throw (Exception) :
    LibraryElement(xmlFilePath, "symbol")
{
    try
    {
        readFromFile();
    }
    catch (Exception& e)
    {
        qDeleteAll(mEllipses);      mEllipses.clear();
        qDeleteAll(mTexts);         mTexts.clear();
        qDeleteAll(mPolygons);      mPolygons.clear();
        qDeleteAll(mPins);          mPins.clear();
        throw;
    }
}

Symbol::~Symbol() noexcept
{
    qDeleteAll(mEllipses);      mEllipses.clear();
    qDeleteAll(mTexts);         mTexts.clear();
    qDeleteAll(mPolygons);      mPolygons.clear();
    qDeleteAll(mPins);          mPins.clear();
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void Symbol::convertLineRectsToPolygonRects(bool fill, bool makeGrabArea) noexcept
{
    QList<const SymbolPolygon*> lines;
    while (findLineRectangle(lines))
    {
        QSet<LengthBase_t> xValues, yValues;
        foreach (const SymbolPolygon* line, lines)
        {
            xValues.insert(line->getStartPos().getX().toNm());
            xValues.insert(line->getSegments().first()->getEndPos().getX().toNm());
            yValues.insert(line->getStartPos().getY().toNm());
            yValues.insert(line->getSegments().first()->getEndPos().getY().toNm());
        }
        if (xValues.count() != 2 || yValues.count() != 2) break;
        //Q_ASSERT(xValues.count() == 2 && yValues.count() == 2);
        Point p1(xValues.values().first(), yValues.values().first());
        Point p2(xValues.values().first(), yValues.values().last());
        Point p3(xValues.values().last(), yValues.values().last());
        Point p4(xValues.values().last(), yValues.values().first());

        // create the new polygon
        SymbolPolygon* rect = new SymbolPolygon();
        rect->setLayerId(lines.first()->getLayerId());
        rect->setWidth(lines.first()->getWidth());
        rect->setIsFilled(fill);
        rect->setIsGrabArea(makeGrabArea);
        rect->setStartPos(p1);
        rect->appendSegment(new SymbolPolygonSegment(p2));
        rect->appendSegment(new SymbolPolygonSegment(p3));
        rect->appendSegment(new SymbolPolygonSegment(p4));
        rect->appendSegment(new SymbolPolygonSegment(p1));
        mPolygons.append(rect);

        // remove all lines
        foreach (const SymbolPolygon* line, lines)
        {
            mPolygons.removeOne(line);
            delete line;
        }
    }
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

void Symbol::parseDomTree(const XmlDomElement& root) throw (Exception)
{
    LibraryElement::parseDomTree(root);

    // Load all pins
    for (XmlDomElement* node = root.getFirstChild("pins/pin", true, false);
         node; node = node->getNextSibling("pin"))
    {
        SymbolPin* pin = new SymbolPin(*node);
        if (mPins.contains(pin->getUuid()))
        {
            throw RuntimeError(__FILE__, __LINE__, pin->getUuid().toString(),
                QString(tr("The pin \"%1\" exists multiple times in \"%2\"."))
                .arg(pin->getUuid().toString(), mXmlFilepath.toNative()));
        }
        mPins.insert(pin->getUuid(), pin);
    }

    // Load all geometry elements
    for (XmlDomElement* node = root.getFirstChild("geometry/*", true, false);
         node; node = node->getNextSibling())
    {
        if (node->getName() == "polygon")
        {
            mPolygons.append(new SymbolPolygon(*node));
        }
        else if (node->getName() == "text")
        {
            mTexts.append(new SymbolText(*node));
        }
        else if (node->getName() == "ellipse")
        {
            mEllipses.append(new SymbolEllipse(*node));
        }
        else
        {
            throw RuntimeError(__FILE__, __LINE__, node->getName(),
                QString(tr("Unknown geometry element \"%1\" in \"%2\"."))
                .arg(node->getName(), mXmlFilepath.toNative()));
        }
    }
}

XmlDomElement* Symbol::serializeToXmlDomElement() const throw (Exception)
{
    QScopedPointer<XmlDomElement> root(LibraryElement::serializeToXmlDomElement());
    XmlDomElement* geometry = root->appendChild("geometry");
    foreach (const SymbolPolygon* polygon, mPolygons)
        geometry->appendChild(polygon->serializeToXmlDomElement());
    foreach (const SymbolText* text, mTexts)
        geometry->appendChild(text->serializeToXmlDomElement());
    foreach (const SymbolEllipse* ellipse, mEllipses)
        geometry->appendChild(ellipse->serializeToXmlDomElement());
    XmlDomElement* pins = root->appendChild("pins");
    foreach (const SymbolPin* pin, mPins)
        pins->appendChild(pin->serializeToXmlDomElement());
    return root.take();
}

bool Symbol::checkAttributesValidity() const noexcept
{
    if (!LibraryElement::checkAttributesValidity())                 return false;
    if (mPins.isEmpty() && mTexts.isEmpty() &&
        mPolygons.isEmpty() && mEllipses.isEmpty())                 return false;
    return true;
}

bool Symbol::findLineRectangle(QList<const SymbolPolygon*>& lines) noexcept
{
    // find lines
    QList<const SymbolPolygon*> linePolygons;
    foreach (const SymbolPolygon* polygon, mPolygons)
    {
        if (polygon->getSegments().count() == 1)
            linePolygons.append(polygon);
    }

    // find rectangle
    const SymbolPolygon* line;
    Length width;
    for (int i=0; i<linePolygons.count(); i++)
    {
        lines.clear();
        Point p = linePolygons.at(i)->getStartPos();
        if (findHLine(linePolygons, p, nullptr, &line))
        {
            lines.append(line);
            width = line->getWidth();
            if (findVLine(linePolygons, p, &width, &line))
            {
                lines.append(line);
                if (findHLine(linePolygons, p, &width, &line))
                {
                    lines.append(line);
                    if (findVLine(linePolygons, p, &width, &line))
                    {
                        lines.append(line);
                        return true;
                    }
                }
            }
        }
    }

    lines.clear();
    return false;
}

bool Symbol::findHLine(const QList<const SymbolPolygon*>& lines, Point& p, Length* width,
                        const SymbolPolygon** line) noexcept
{
    foreach (const SymbolPolygon* polygon, lines)
    {
        if (width) {if (polygon->getWidth() != *width) continue;}
        Point p1 = polygon->getStartPos();
        Point p2 = polygon->getSegments().at(0)->getEndPos();
        if ((p1 == p) && (p2.getY() == p.getY()))
        {
            *line = polygon;
            p = p2;
            return true;
        }
        else if ((p2 == p) && (p1.getY() == p.getY()))
        {
            *line = polygon;
            p = p1;
            return true;
        }
    }
    return false;
}

bool Symbol::findVLine(const QList<const SymbolPolygon*>& lines, Point& p, Length* width,
                        const SymbolPolygon** line) noexcept
{
    foreach (const SymbolPolygon* polygon, lines)
    {
        if (width) {if (polygon->getWidth() != *width) continue;}
        Point p1 = polygon->getStartPos();
        Point p2 = polygon->getSegments().at(0)->getEndPos();
        if ((p1 == p) && (p2.getX() == p.getX()))
        {
            *line = polygon;
            p = p2;
            return true;
        }
        else if ((p2 == p) && (p1.getX() == p.getX()))
        {
            *line = polygon;
            p = p1;
            return true;
        }
    }
    return false;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace library
