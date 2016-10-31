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
#include <QtWidgets>
#include "wsi_debugtools.h"
#include <librepcb/common/fileio/xmldomelement.h>

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace workspace {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

WSI_DebugTools::WSI_DebugTools(const QString& xmlTagName, XmlDomElement* xmlElement) throw (Exception) :
    WSI_Base(xmlTagName, xmlElement)
{
    if (xmlElement) {
        // load setting
    }

    // create a QWidget
    mWidget.reset(new QWidget());
    QGridLayout* layout = new QGridLayout(mWidget.data());
#ifndef QT_DEBUG
    layout->addWidget(new QLabel(tr("Warning: Some of these settings may only work in DEBUG mode!")), 0, 0);
#endif

    // stretch the last row
    layout->setRowStretch(layout->rowCount(), 1);
}

WSI_DebugTools::~WSI_DebugTools() noexcept
{
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void WSI_DebugTools::restoreDefault() noexcept
{
}

void WSI_DebugTools::apply() noexcept
{
}

void WSI_DebugTools::revert() noexcept
{
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

XmlDomElement* WSI_DebugTools::serializeToXmlDomElement() const throw (Exception)
{
    QScopedPointer<XmlDomElement> root(WSI_Base::serializeToXmlDomElement());
    return root.take();
}

bool WSI_DebugTools::checkAttributesValidity() const noexcept
{
    return true;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace workspace
} // namespace librepcb
