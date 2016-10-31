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
#include "wsi_appearance.h"
#include <librepcb/common/fileio/xmldomelement.h>

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace workspace {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

WSI_Appearance::WSI_Appearance(const QString& xmlTagName, XmlDomElement* xmlElement) throw (Exception) :
    WSI_Base(xmlTagName, xmlElement), mUseOpenGl(false)
{
    if (xmlElement) {
        // load setting
        mUseOpenGl = xmlElement->getFirstChild("use_opengl", true)->getText<bool>(true);
    }

    // create widgets
    mUseOpenGlWidget.reset(new QWidget());
    QGridLayout* openGlLayout = new QGridLayout(mUseOpenGlWidget.data());
    openGlLayout->setContentsMargins(0, 0, 0, 0);
    mUseOpenGlCheckBox.reset(new QCheckBox(tr("Use OpenGL Hardware Acceleration")));
    mUseOpenGlCheckBox->setChecked(mUseOpenGl);
    openGlLayout->addWidget(mUseOpenGlCheckBox.data(), openGlLayout->rowCount(), 0);
    openGlLayout->addWidget(new QLabel(tr("This setting will be applied only to newly "
                            "opened windows.")), openGlLayout->rowCount(), 0);
}

WSI_Appearance::~WSI_Appearance() noexcept
{
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void WSI_Appearance::restoreDefault() noexcept
{
    mUseOpenGlCheckBox->setChecked(false);
}

void WSI_Appearance::apply() noexcept
{
    mUseOpenGl = mUseOpenGlCheckBox->isChecked();
}

void WSI_Appearance::revert() noexcept
{
    mUseOpenGlCheckBox->setChecked(mUseOpenGl);
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

XmlDomElement* WSI_Appearance::serializeToXmlDomElement() const throw (Exception)
{
    QScopedPointer<XmlDomElement> root(WSI_Base::serializeToXmlDomElement());
    root->appendTextChild("use_opengl", mUseOpenGlCheckBox->isChecked());
    return root.take();
}

bool WSI_Appearance::checkAttributesValidity() const noexcept
{
    return true;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace workspace
} // namespace librepcb
