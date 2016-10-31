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
#include "libraryinfowidget.h"
#include "ui_libraryinfowidget.h"
#include <librepcblibrary/library.h>
#include "../../workspace.h"
#include "../../settings/workspacesettings.h"

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace workspace {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

LibraryInfoWidget::LibraryInfoWidget(Workspace& ws, QSharedPointer<library::Library> lib) noexcept :
    QWidget(nullptr), mUi(new Ui::LibraryInfoWidget),
    mWorkspace(ws), mLib(lib)
{
    mUi->setupUi(this);
    connect(mUi->btnRemove, &QPushButton::clicked,
            this, &LibraryInfoWidget::btnRemoveLibraryClicked);

    const QStringList& localeOrder = ws.getSettings().getLibLocaleOrder().getLocaleOrder();

    // image
    if (!lib->getIcon().isNull()) {
        mUi->lblIcon->setPixmap(lib->getIcon().scaled(mUi->lblIcon->size(),
            Qt::KeepAspectRatio, Qt::SmoothTransformation));
    } else {
        mUi->lblIcon->setVisible(false);
        mUi->line->setVisible(false);
    }

    // general attributes
    mUi->lblName->setText(lib->getName(localeOrder));
    mUi->lblDescription->setText(lib->getDescription(localeOrder));
    mUi->lblVersion->setText(lib->getVersion().toStr());
    mUi->lblAuthor->setText(lib->getAuthor());
    mUi->lblUrl->setText(QString("<a href='%1'>%2</a>").arg(
        lib->getUrl().toEncoded(), lib->getUrl().toDisplayString()));
    mUi->lblCreated->setText(lib->getCreated().toString(Qt::TextDate));
    mUi->lblLastModified->setText(lib->getLastModified().toString(Qt::TextDate));
    mUi->lblDeprecated->setText(lib->isDeprecated() ?
        tr("Yes - Consider switching to another library.") : tr("No"));

    // extended attributes
    mUi->lblUuid->setText(lib->getUuid().toStr());
    mUi->lblLibType->setText(isRemoteLibrary() ? tr("Remote") : tr("Local"));
    QString dependencies;
    foreach (const Uuid& uuid, lib->getDependencies()) {
        Version installedVersion = mWorkspace.getVersionOfLibrary(uuid, true, true);
        QString line = dependencies.isEmpty() ? "" : "<br>";
        if (installedVersion.isValid()) {
            line += QString(" <font color=\"green\">%1 ✔</font>").arg(uuid.toStr());
        } else {
            line += QString(" <font color=\"red\">%1 ✖</font>").arg(uuid.toStr());
        }
        dependencies.append(line);
    }
    mUi->lblDependencies->setText(dependencies);
    mUi->lblDirectory->setText(QString("<a href='%1'>%2</a>").arg(
        lib->getFilePath().toQUrl().toLocalFile(),
        lib->getFilePath().toRelative(ws.getLibrariesPath())));
    mUi->lblDirectory->setToolTip(lib->getFilePath().toNative());
}

LibraryInfoWidget::~LibraryInfoWidget() noexcept
{
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

void LibraryInfoWidget::btnRemoveLibraryClicked() noexcept
{
    QString title = tr("Remove Library");
    QString text = QString(tr("Attention! This will remove the whole library directory:"
                              "\n\n%1\n\nAre you really sure to remove \"%2\"?"))
                   .arg(mLib->getFilePath().toNative(), mUi->lblName->text());

    int res = QMessageBox::question(this, title, text, QMessageBox::Yes | QMessageBox::No);

    if (res == QMessageBox::Yes) {
        try {
            if (isRemoteLibrary()) {
                mWorkspace.removeRemoteLibrary(mLib->getFilePath().getFilename()); // can throw
            } else {
                mWorkspace.removeLocalLibrary(mLib->getFilePath().getFilename()); // can throw
            }
            emit libraryRemoved(mLib->getFilePath());
        } catch (const Exception& e) {
            QMessageBox::critical(this, tr("Error"), e.getUserMsg());
        }
    }
}

bool LibraryInfoWidget::isRemoteLibrary() const noexcept
{
    return mLib->isOpenedReadOnly();
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace workspace
} // namespace librepcb
