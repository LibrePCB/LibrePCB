/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 Urban Bruhin
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

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

WSI_ProjectAutosaveInterval::WSI_ProjectAutosaveInterval(WorkspaceSettings& settings) :
    WSI_Base(settings), mWidget(0), mSpinBox(0)
{
    bool ok;
    mInterval = loadValue("project_autosave_interval", 600).toUInt(&ok);
    if (!ok) mInterval = 600;
    if (mInterval % 60 != 0)
        mInterval += 60 - (mInterval % 60); // round up to the next full minute
    mIntervalTmp = mInterval;

    mSpinBox = new QSpinBox();
    mSpinBox->setMinimum(0);
    mSpinBox->setMaximum(60);
    mSpinBox->setValue(mInterval / 60);
    mSpinBox->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    connect(mSpinBox, SIGNAL(valueChanged(int)), this, SLOT(spinBoxValueChanged(int)));

    // create a QWidget
    mWidget = new QWidget();
    QHBoxLayout* layout = new QHBoxLayout(mWidget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(mSpinBox);
    layout->addWidget(new QLabel(tr("Minutes (0 = disable autosave)")));
}

WSI_ProjectAutosaveInterval::~WSI_ProjectAutosaveInterval()
{
    delete mSpinBox;            mSpinBox = 0;
    delete mWidget;             mWidget = 0;
}

/*****************************************************************************************
 *  General Methods
 ****************************************************************************************/

void WSI_ProjectAutosaveInterval::restoreDefault()
{
    mIntervalTmp = 600;
    mSpinBox->setValue(mIntervalTmp / 60);
}

void WSI_ProjectAutosaveInterval::apply()
{
    if (mInterval == mIntervalTmp)
        return;

    mInterval = mIntervalTmp;
    saveValue("project_autosave_interval", mInterval);
}

void WSI_ProjectAutosaveInterval::revert()
{
    mIntervalTmp = mInterval;
    mSpinBox->setValue(mIntervalTmp / 60);
}

/*****************************************************************************************
 *  Public Slots
 ****************************************************************************************/

void WSI_ProjectAutosaveInterval::spinBoxValueChanged(int value)
{
    mIntervalTmp = value * 60;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace librepcb
