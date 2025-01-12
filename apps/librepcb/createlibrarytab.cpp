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
#include "createlibrarytab.h"

#include "apptoolbox.h"

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {
namespace app {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

CreateLibraryTab::CreateLibraryTab(GuiApplication& app, int id,
                                   QObject* parent) noexcept
  : WindowTab(app, id, ui::TabType::CreateLibrary, nullptr, -1, tr("New Library"),
              parent),
    mUiData() {
  mUiData.used = true;
  mUiData.name = q2s(tr("My Library"));
  mUiData.description = q2s(QString("foo! %1").arg(id, 8, 16, QChar('0')));

  auto t = new QTimer();
  t->setInterval(1000);
  connect(t, &QTimer::timeout, this, [this](){
    mUiData.version_default = q2s(QDateTime::currentDateTime().toString());
    mUiData.valid = !mUiData.valid;
    emit uiDataChanged();
  });
  t->start();
}

CreateLibraryTab::~CreateLibraryTab() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void CreateLibraryTab::activate() noexcept {
}

void CreateLibraryTab::deactivate() noexcept {
}

void CreateLibraryTab::setUiData(
    const ui::CreateLibraryTabData& data) noexcept {
  mUiData = data;
  qDebug() << data.description.begin();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace app
}  // namespace editor
}  // namespace librepcb
