/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2018 LibrePCB Developers, see AUTHORS.md for contributors.
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
#include "aboutdialog.h"
#include "ui_aboutdialog.h"
#include <librepcb/common/application.h>

/*****************************************************************************************
 *  Namespace
 ****************************************************************************************/
namespace librepcb {

AboutDialog::AboutDialog(QWidget* parent) noexcept :
    QDialog(parent),
    mUi(new Ui::AboutDialog)
{
    // Set up base dialog
    mUi->setupUi(this);

    // Event handlers
    connect(mUi->buttonBox, &QDialogButtonBox::clicked,
            this, &AboutDialog::close);

    // Layout
    mUi->tabWidget->setCurrentIndex(0);

    // Get some version information
    const Version& appVersion = qApp->getAppVersion();
    const QString& gitVersion = qApp->getGitVersion();
    const QString& buildDate = qApp->getBuildDate().toString("yyyy-MM-dd hh:mm:ss (t)");

    // Dynamic text
    mUi->textVersion->setText(QString("Version: %1 (%2)<br>Build date: %3").arg(appVersion.toPrettyStr(3), gitVersion, buildDate));
    mUi->textLinks->setText(tr("For more information, please check out <a href='%1'>librepcb.org</a> or our <a href='%2'>GitHub repository</a>.").arg("http://librepcb.org/", "https://github.com/LibrePCB/LibrePCB"));
    mUi->textContributeFinancially->setText(
        tr("Support sustainable development of LibrePCB by donating financially, either via <a href='%1'>Patreon</a> or via <a href='%2'>Bitcoin</a>!")
        .arg("https://www.patreon.com/librepcb", "bitcoin:1FiXZxoXe3px1nNuNygRb1NwcYr6U8AvG8")
    );
    mUi->textContributeCode->setText(
        tr("Check out our <a href='%1'>Contribution Guidelines</a> if you're interested in development of LibrePCB!")
        .arg("https://github.com/LibrePCB/LibrePCB/blob/master/CONTRIBUTING.md")
    );

    // Format content
    formatLabelHeading(mUi->headerLinks);
    formatLabelHeading(mUi->headerLicense);
    formatLabelHeading(mUi->headerContributeFinancially);
    formatLabelHeading(mUi->headerContributeCode);
    formatLabelHeading(mUi->headerContributeShare);
    formatLabelText(mUi->textIntro, false, true);
    formatLabelText(mUi->textVersion, true, false);
    formatLabelText(mUi->textLinks, false, true);
    formatLabelText(mUi->textLicense, false, true);
    formatLabelText(mUi->textContributeFinancially, false, true);
    formatLabelText(mUi->textContributeCode, false, true);
    formatLabelText(mUi->textContributeShare, false, false);
}

/**
 * @brief Format a heading label in the about dialog.
 * @param label Pointer to the QLabel instance
 */
void AboutDialog::formatLabelHeading(QLabel* label) noexcept
{
    int headerMarginTop = 12;
    int headerMarginBottom = 4;
    label->setContentsMargins(0, headerMarginTop, 0, headerMarginBottom);
}

/**
 * @brief Format a text label in the about dialog.
 * @param label Pointer to the QLabel instance
 * @param selectable Whether to make the text mouse-selectable
 * @param containsLinks Whether to open links in external application (e.g. web browser)
 */
void AboutDialog::formatLabelText(QLabel* label, bool selectable, bool containsLinks) noexcept
{
    label->setOpenExternalLinks(containsLinks);
    if (selectable) {
        label->setTextInteractionFlags(Qt::TextSelectableByMouse);
        if (containsLinks) {
            qWarning() << "If text is selectable, external links won't work anymore!";
        }
    }
}

AboutDialog::~AboutDialog() noexcept
{
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace librepcb
