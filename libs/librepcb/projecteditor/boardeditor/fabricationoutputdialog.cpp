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
#include "fabricationoutputdialog.h"
#include "ui_fabricationoutputdialog.h"
#include <librepcb/project/project.h>
#include <librepcb/project/boards/board.h>
#include <librepcb/project/boards/boardgerberexport.h>

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace project {
namespace editor {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

FabricationOutputDialog::FabricationOutputDialog(Board& board, QWidget *parent) :
    QDialog(parent), mProject(board.getProject()), mBoard(board),
    mUi(new Ui::FabricationOutputDialog)
{
    mUi->setupUi(this);

    QString version = FilePath::cleanFileName(mProject.getVersion(),
                      FilePath::ReplaceSpaces | FilePath::KeepCase);
    QString outputDir = QString("output/%1/gerber").arg(version);
    FilePath gerberDir = mProject.getPath().getPathTo(outputDir);
    mUi->edtOutputDirPath->setText(gerberDir.toNative());
}

FabricationOutputDialog::~FabricationOutputDialog()
{
    delete mUi;     mUi = nullptr;
}

/*****************************************************************************************
 *  Private Slots
 ****************************************************************************************/

void FabricationOutputDialog::on_btnSelectDir_clicked()
{
    QString directory = QFileDialog::getExistingDirectory(this, tr("Select Output Directory"),
                                                          mUi->edtOutputDirPath->text());
    if (directory.isEmpty()) return;

    mUi->edtOutputDirPath->setText(FilePath(directory).toNative());
}

void FabricationOutputDialog::on_btnGenerate_clicked()
{
    try
    {
        FilePath filepath(mUi->edtOutputDirPath->text());
        BoardGerberExport grbExport(mBoard, filepath);
        grbExport.exportAllLayers();
    }
    catch (Exception& e)
    {
        QMessageBox::warning(this, tr("Error"), e.getMsg());
    }
}

void FabricationOutputDialog::on_btnBrowseOutputDir_clicked()
{
    FilePath filepath(mUi->edtOutputDirPath->text());
    if (filepath.isExistingDir()) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(filepath.toStr()));
    } else {
        QMessageBox::warning(this, tr("Warning"), tr("Directory does not exist."));
    }
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace editor
} // namespace project
} // namespace librepcb
