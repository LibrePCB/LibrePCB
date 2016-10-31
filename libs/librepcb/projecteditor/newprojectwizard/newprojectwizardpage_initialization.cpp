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
#include "newprojectwizardpage_initialization.h"
#include "ui_newprojectwizardpage_initialization.h"
#include <librepcb/common/fileio/filepath.h>

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {
namespace project {

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

NewProjectWizardPage_Initialization::NewProjectWizardPage_Initialization(QWidget *parent) noexcept :
    QWizardPage(parent), mUi(new Ui::NewProjectWizardPage_Initialization)
{
    mUi->setupUi(this);
    setPixmap(QWizard::LogoPixmap, QPixmap(":/img/actions/plus_2.png"));
    setPixmap(QWizard::WatermarkPixmap, QPixmap(":/img/wizards/watermark.jpg"));

    // signal/slot connections
    connect(mUi->cbxAddSchematic, &QGroupBox::toggled,
            this, &NewProjectWizardPage_Initialization::completeChanged);
    connect(mUi->cbxAddBoard, &QGroupBox::toggled,
            this, &NewProjectWizardPage_Initialization::completeChanged);
    connect(mUi->edtSchematicName, &QLineEdit::textChanged,
            this, &NewProjectWizardPage_Initialization::schematicNameChanged);
    connect(mUi->edtBoardName, &QLineEdit::textChanged,
            this, &NewProjectWizardPage_Initialization::boardNameChanged);

    // insert values
    mUi->edtSchematicName->setText("Main"); // do not translate this into other languages!
    mUi->edtBoardName->setText("default");  // do not translate this into other languages!
}

NewProjectWizardPage_Initialization::~NewProjectWizardPage_Initialization() noexcept
{
}

/*****************************************************************************************
 *  Getters
 ****************************************************************************************/

bool NewProjectWizardPage_Initialization::getCreateSchematic() const noexcept
{
    return mUi->cbxAddSchematic->isChecked();
}

QString NewProjectWizardPage_Initialization::getSchematicName() const noexcept
{
    return mUi->edtSchematicName->text();
}

QString NewProjectWizardPage_Initialization::getSchematicFileName() const noexcept
{
    return mUi->lblSchematicFileName->text();
}

bool NewProjectWizardPage_Initialization::getCreateBoard() const noexcept
{
    return mUi->cbxAddBoard->isChecked();
}

QString NewProjectWizardPage_Initialization::getBoardName() const noexcept
{
    return mUi->edtBoardName->text();
}

QString NewProjectWizardPage_Initialization::getBoardFileName() const noexcept
{
    return mUi->lblBoardFileName->text();
}

/*****************************************************************************************
 *  GUI Action Handlers
 ****************************************************************************************/

void NewProjectWizardPage_Initialization::schematicNameChanged(const QString& name) noexcept
{
    QString filename = FilePath::cleanFileName(name, FilePath::ReplaceSpaces | FilePath::ToLowerCase);
    if (!filename.isEmpty()) filename.append(".xml");
    mUi->lblSchematicFileName->setText(filename);
    emit completeChanged();
}

void NewProjectWizardPage_Initialization::boardNameChanged(const QString& name) noexcept
{
    QString filename = FilePath::cleanFileName(name, FilePath::ReplaceSpaces | FilePath::ToLowerCase);
    if (!filename.isEmpty()) filename.append(".xml");
    mUi->lblBoardFileName->setText(filename);
    emit completeChanged();
}

/*****************************************************************************************
 *  Private Methods
 ****************************************************************************************/

bool NewProjectWizardPage_Initialization::isComplete() const noexcept
{
    // check base class
    if (!QWizardPage::isComplete()) return false;

    // check schematic filename
    if (mUi->cbxAddSchematic->isChecked() && mUi->lblSchematicFileName->text().isEmpty()) {
        return false;
    }

    // check board filename
    if (mUi->cbxAddBoard->isChecked() && mUi->lblBoardFileName->text().isEmpty()) {
        return false;
    }

    return true;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace project
} // namespace librepcb
