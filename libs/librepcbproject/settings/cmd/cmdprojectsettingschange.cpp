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
#include "cmdprojectsettingschange.h"
#include "../projectsettings.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

CmdProjectSettingsChange::CmdProjectSettingsChange(ProjectSettings& settings,
                                                   UndoCommand* parent) throw (Exception) :
    UndoCommand(tr("Change Project Settings"), parent), mSettings(settings),
    mRestoreDefaults(false),
    mLocaleOrderOld(settings.getLocaleOrder()), mLocaleOrderNew(mLocaleOrderOld),
    mNormOrderOld(settings.getNormOrder()), mNormOrderNew(mNormOrderOld)
{
}

CmdProjectSettingsChange::~CmdProjectSettingsChange() noexcept
{
}

/*****************************************************************************************
 *  Setters
 ****************************************************************************************/

void CmdProjectSettingsChange::restoreDefaults() noexcept
{
    Q_ASSERT((mRedoCount == 0) && (mUndoCount == 0));
    mRestoreDefaults = true;
}

void CmdProjectSettingsChange::setLocaleOrder(const QStringList& locales) noexcept
{
    Q_ASSERT((mRedoCount == 0) && (mUndoCount == 0));
    mLocaleOrderNew = locales;
}

void CmdProjectSettingsChange::setNormOrder(const QStringList& norms) noexcept
{
    Q_ASSERT((mRedoCount == 0) && (mUndoCount == 0));
    mNormOrderNew = norms;
}

/*****************************************************************************************
 *  Inherited from UndoCommand
 ****************************************************************************************/

void CmdProjectSettingsChange::redo() throw (Exception)
{
    try
    {
        applyNewSettings();
        UndoCommand::redo();
        mSettings.triggerSettingsChanged();
    }
    catch (Exception &e)
    {
        applyOldSettings();
        throw;
    }
}

void CmdProjectSettingsChange::undo() throw (Exception)
{
    try
    {
        applyOldSettings();
        UndoCommand::undo();
        mSettings.triggerSettingsChanged();
    }
    catch (Exception& e)
    {
        applyNewSettings();
        throw;
    }
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

void CmdProjectSettingsChange::applyNewSettings() throw (Exception)
{
    if (mRestoreDefaults)
    {
        mSettings.restoreDefaults();
    }
    else
    {
        mSettings.setLocaleOrder(mLocaleOrderNew);
        mSettings.setNormOrder(mNormOrderNew);
    }
}

void CmdProjectSettingsChange::applyOldSettings() throw (Exception)
{
    mSettings.setLocaleOrder(mLocaleOrderOld);
    mSettings.setNormOrder(mNormOrderOld);
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb
