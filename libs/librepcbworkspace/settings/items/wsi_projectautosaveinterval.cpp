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
#include "wsi_projectautosaveinterval.h"
#include <librepcbcommon/fileio/xmldomelement.h>

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace workspace {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

WSI_ProjectAutosaveInterval::WSI_ProjectAutosaveInterval(const QString& xmlTagName,
                                                         XmlDomElement* xmlElement) throw (Exception) :
    WSI_Base(xmlTagName, xmlElement),
    mInterval(600), mIntervalTmp(mInterval)
{
    if (xmlElement) {
        // load setting
        mInterval = xmlElement->getText<uint>(true);
    }
    if (mInterval % 60 != 0) {
        mInterval += 60 - (mInterval % 60); // round up to the next full minute
    }
    mIntervalTmp = mInterval;

    // create a spinbox
    mSpinBox.reset(new QSpinBox());
    mSpinBox->setMinimum(0);
    mSpinBox->setMaximum(60);
    mSpinBox->setValue(mInterval / 60);
    mSpinBox->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    connect(mSpinBox.data(), static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged),
            this, &WSI_ProjectAutosaveInterval::spinBoxValueChanged);

    // create a QWidget
    mWidget.reset(new QWidget());
    QHBoxLayout* layout = new QHBoxLayout(mWidget.data());
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(mSpinBox.data());
    layout->addWidget(new QLabel(tr("Minutes (0 = disable autosave)")));
}

WSI_ProjectAutosaveInterval::~WSI_ProjectAutosaveInterval() noexcept
{
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void WSI_ProjectAutosaveInterval::restoreDefault() noexcept
{
    mIntervalTmp = 600;
    mSpinBox->setValue(mIntervalTmp / 60);
}

void WSI_ProjectAutosaveInterval::apply() noexcept
{
    mInterval = mIntervalTmp;
}

void WSI_ProjectAutosaveInterval::revert() noexcept
{
    mIntervalTmp = mInterval;
    mSpinBox->setValue(mIntervalTmp / 60);
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

void WSI_ProjectAutosaveInterval::spinBoxValueChanged(int value) noexcept
{
    mIntervalTmp = value * 60;
}

XmlDomElement* WSI_ProjectAutosaveInterval::serializeToXmlDomElement() const throw (Exception)
{
    QScopedPointer<XmlDomElement> root(WSI_Base::serializeToXmlDomElement());
    root->setText(mInterval);
    return root.take();
}

bool WSI_ProjectAutosaveInterval::checkAttributesValidity() const noexcept
{
    return true;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace workspace
} // namespace librepcb
