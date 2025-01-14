/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2018 LibrePCB Developers, see AUTHORS.md for contributors.
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
#include "aboutdialog.h"

#include "../workspace/desktopservices.h"
#include "ui_aboutdialog.h"

#include <librepcb/core/3d/occmodel.h>
#include <librepcb/core/application.h>
#include <librepcb/core/systeminfo.h>

#include <QtCore>
#include <QtNetwork>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

AboutDialog::AboutDialog(const WorkspaceSettings& settings,
                         QWidget* parent) noexcept
  : QDialog(parent), mSettings(settings), mUi(new Ui::AboutDialog) {
  // Set up base dialog
  mUi->setupUi(this);
  mUi->txtDetails->setFont(Application::getDefaultMonospaceFont());

  // Event handlers
  connect(mUi->lblIntro, &QLabel::linkActivated, this,
          &AboutDialog::openExternalLink);
  connect(mUi->lblContributing, &QLabel::linkActivated, this,
          &AboutDialog::openExternalLink);
  connect(mUi->lblCredits, &QLabel::linkActivated, this,
          &AboutDialog::openExternalLink);
  connect(mUi->lblNgi0Text, &QLabel::linkActivated, this,
          &AboutDialog::openExternalLink);
  connect(mUi->lblCopyDetailsToClipboard, &QLabel::linkActivated, [this]() {
    QApplication::clipboard()->setText(mUi->txtDetails->toPlainText());
    QToolTip::showText(QCursor::pos(), tr("Copied!"),
                       mUi->lblCopyDetailsToClipboard,
                       mUi->lblCopyDetailsToClipboard->rect(), 1000);
  });
  connect(mUi->buttonBox, &QDialogButtonBox::clicked, this,
          &AboutDialog::close);

  // Layout
  mUi->tabWidget->setCurrentIndex(0);

  // Get some version information
  const QString appVersion = Application::getVersion();
  const QString gitRevision =
      Application::getGitRevision().left(10);  // 10 digits should be enough
  const QString buildDate =
      Application::getBuildDate().toString("yyyy-MM-dd hh:mm:ss (t)");

  // Set title text.
  mUi->lblTitle->setText(QString("LibrePCB %1").arg(appVersion));

  // Set revision text.
  mUi->lblRevision->setText(QString("Git revision: %2<br>Build date: %3")
                                .arg(gitRevision, buildDate));

  // Set intro text (not using Qt designer to avoid layouting issues).
  {
    QString text;
    text += "<p>" %
        tr("LibrePCB is a free &amp; open source schematic/layout-editor. "
           "It is mainly developed by Urban Bruhin, with the support of "
           "<a href='%1'>many other contributors</a>.")
            .arg("https://github.com/LibrePCB/LibrePCB/graphs/contributors") %
        "<p>";
    text += "<h4>" % tr("Links") % "</h4>";
    text += "<p>" %
        tr("For more information, check out "
           "<a href='%1'>librepcb.org</a> or our "
           "<a href='%2'>GitHub repository</a>.")
            .arg("https://librepcb.org/",
                 "https://github.com/LibrePCB/LibrePCB") %
        "</p>";
    text += "<h4>" % tr("Help") % "</h4>";
    text += "<p>" %
        tr("If you need help, please check out the "
           "<a href='%1'>documentation</a> or <a href='%2'>contact us</a>.")
            .arg("https://librepcb.org/docs/", "https://librepcb.org/help/") %
        "</p>";
    text += "<h4>" % tr("License") % "</h4>";
    text += "<p>" %
        tr("LibrePCB is free software, released under the GNU General Public "
           "License (GPL) version 3 or later. You can find the full license "
           "text <a href='%1'>in our source code</a>.")
            .arg(
                "https://github.com/LibrePCB/LibrePCB/blob/master/"
                "LICENSE.txt") %
        "</p>";
    mUi->lblIntro->setText(text);
  }

  // Set contributing text (not using Qt designer to avoid layouting issues).
  {
    QString text;
    text += "<p>" %
        tr("LibrePCB is a community project, and therefore it relies on "
           "contributions! There are different ways you can contribute:") %
        "<p>";
    text += "<h4>" % tr("Donate") % "</h4>";
    text += "<p>" %
        tr("Support sustainable development of LibrePCB by donating "
           "financially via Patreon, PayPal, Bitcoin or other ways. Check out "
           "<a href='%1'>%2</a> for details.")
            .arg("https://librepcb.org/donate/", "librepcb.org/donate") %
        "</p>";
    text += "<h4>" % tr("Improve LibrePCB") % "</h4>";
    text += "<p>" %
        tr("If you're interested in helping us to develop LibrePCB, check out "
           "<a href='%1'>%2</a> to see how you can contribute!")
            .arg("https://librepcb.org/contribute/",
                 "librepcb.org/contribute") %
        "</p>";
    text += "<h4>" % tr("Spread The Word") % "</h4>";
    text += "<p>" %
        tr("Speak about LibrePCB with your friends and colleagues, or write "
           "about it in the internet! Write a blogpost, or create a video "
           "tutorial. We're happy if more people can get to know LibrePCB.") %
        "</p>";
    mUi->lblContributing->setText(text);
  }

  // Set credits text (not using Qt designer to avoid layouting issues).
  {
    QString text;
    text += "<p>" %
        tr("This project relies on <a href='%1'>many contributors</a>, "
           "sponsors and other open-source components like software libraries "
           "or icons. Many thanks to all the people and projects supporting "
           "LibrePCB!")
            .arg("https://github.com/LibrePCB/LibrePCB/graphs/contributors") %
        " ♥<p>";
    text += "<h4>" % tr("Sponsors") % "</h4>";
    text += "<p>" %
        tr("For the list of current sponsors, see <a href='%1'>%2</a>.")
            .arg("https://librepcb.org/sponsors/")
            .arg("librepcb.org/sponsors") %
        "<p>";
    text += "<h4>" % tr("Icons") % "</h4>";
    text += "<p>" %
        tr("Some of the icons used in LibrePCB are provided by "
           "<a href='%1'>%2</a>, thank you!")
            .arg("https://icons8.com")
            .arg("icons8.com") %
        "</p>";
    mUi->lblCredits->setText(text);
  }

  // Information text (always English, not translatable)
  QStringList details;
  QString qt = QString(qVersion()) + " (built against " + QT_VERSION_STR + ")";
  details << "LibrePCB Version: " + Application::getVersion();
  details << "Git Revision:     " + Application::getGitRevision();
  details << "Build Date:       " +
          Application::getBuildDate().toString(Qt::ISODate);
  if (!Application::getBuildAuthor().isEmpty()) {
    details << "Build Author:     " + Application::getBuildAuthor();
  }
  details << "Qt Version:       " + qt;
  details << "CPU Architecture: " + QSysInfo::currentCpuArchitecture();
  details << "Operating System: " + QSysInfo::prettyProductName();
  details << "Platform Plugin:  " + qApp->platformName();
  details << "TLS Library:      " + QSslSocket::sslLibraryVersionString();
  details << "OCC Library:      " + OccModel::getOccVersionString();
  if (!SystemInfo::detectRuntime().isEmpty()) {
    details << "Runtime:          " + SystemInfo::detectRuntime();
  }
  mUi->txtDetails->setPlainText(details.join("\n"));
}

AboutDialog::~AboutDialog() noexcept {
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void AboutDialog::openExternalLink(const QString& url) noexcept {
  DesktopServices ds(mSettings);
  ds.openWebUrl(QUrl(url));
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
