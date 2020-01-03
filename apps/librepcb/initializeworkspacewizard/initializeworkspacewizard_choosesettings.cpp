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
#include "initializeworkspacewizard_choosesettings.h"

#include "ui_initializeworkspacewizard_choosesettings.h"

#include <librepcb/common/application.h>
#include <librepcb/common/norms.h>
#include <librepcb/common/systeminfo.h>
#include <librepcb/common/toolbox.h>
#include <librepcb/common/units/lengthunit.h>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace application {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

InitializeWorkspaceWizard_ChooseSettings::
    InitializeWorkspaceWizard_ChooseSettings(
        InitializeWorkspaceWizardContext& context, QWidget* parent) noexcept
  : QWizardPage(parent),
    mContext(context),
    mUi(new Ui::InitializeWorkspaceWizard_ChooseSettings) {
  mUi->setupUi(this);

  // list all available locales
  {
    mUi->cbxAppLanguage->addItem(tr("System Language"), QString(""));
    QMap<QString, QString> map;  // map will be sorted by key
    foreach (const QString& locale, qApp->getAvailableTranslationLocales()) {
      map.insert(Toolbox::prettyPrintLocale(locale), locale);
    }
    QMap<QString, QString>::const_iterator i = map.constBegin();
    while (i != map.constEnd()) {
      mUi->cbxAppLanguage->addItem(i.key(), i.value());
      ++i;
    }
    mUi->cbxAppLanguage->setCurrentIndex(0);  // system language
  }

  // list all available norms
  {
    QSize iconSize = mUi->cbxLibraryNorm->iconSize();
    iconSize.setWidth(iconSize.height() * 4);
    mUi->cbxLibraryNorm->setIconSize(iconSize);
    mUi->cbxLibraryNorm->addItem(QIcon(), tr("None"), QVariant());
    foreach (const QString& norm, getAvailableNorms()) {
      mUi->cbxLibraryNorm->addItem(getNormIcon(norm), norm, norm);
    }
    mUi->cbxLibraryNorm->setCurrentIndex(0);
  }

  // list all measurement units
  foreach (const LengthUnit& unit, LengthUnit::getAllUnits()) {
    mUi->cbxLengthUnit->addItem(unit.toStringTr(), unit.getIndex());
  }
  mUi->cbxLengthUnit->setCurrentIndex(LengthUnit::millimeters().getIndex());

  // initialize user name
  mUi->edtUserName->setText(SystemInfo::getFullUsername());
}

InitializeWorkspaceWizard_ChooseSettings::
    ~InitializeWorkspaceWizard_ChooseSettings() noexcept {
}

/*******************************************************************************
 *  Public Methods
 ******************************************************************************/

bool InitializeWorkspaceWizard_ChooseSettings::validatePage() noexcept {
  try {
    mContext.setAppLocale(mUi->cbxAppLanguage->currentData().toString());
    mContext.setLengthUnit(
        LengthUnit::fromIndex(mUi->cbxLengthUnit->currentIndex()));
    QString selectedLibNorm = mUi->cbxLibraryNorm->currentData().toString();
    mContext.setLibraryNormOrder(selectedLibNorm.isEmpty()
                                     ? QStringList()
                                     : QStringList{selectedLibNorm});
    mContext.setUserName(mUi->edtUserName->text().trimmed());
    mContext.initializeEmptyWorkspace();
    return true;
  } catch (const Exception& e) {
    QMessageBox::critical(this, tr("Error"), e.getMsg());
    return false;
  }
}

int InitializeWorkspaceWizard_ChooseSettings::nextId() const noexcept {
  return InitializeWorkspaceWizardContext::PageId::ID_None;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace application
}  // namespace librepcb
