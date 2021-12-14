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
#include "orderpcbdialog.h"

#include "ui_orderpcbdialog.h"

#include <librepcb/core/exceptions.h>
#include <librepcb/core/network/orderpcbapirequest.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

OrderPcbDialog::OrderPcbDialog(const QList<QUrl>& repositories,
                               std::function<QByteArray()> createLppzCallback,
                               const QString& boardRelativePath,
                               QWidget* parent) noexcept
  : QDialog(parent),
    mRequest(),
    mCreateLppzCallback(createLppzCallback),
    mBoardRelativePath(boardRelativePath),
    mUi(new Ui::OrderPcbDialog) {
  mUi->setupUi(this);
  mUi->lblMoreInformation->hide();
  mUi->cbxOpenBrowser->hide();
  mUi->btnUpload->hide();
  mUi->imgError->hide();
  connect(mUi->btnUpload, &QPushButton::clicked, this,
          &OrderPcbDialog::uploadButtonClicked);

  // Replace placeholder in the note label.
  QString forumLink = "<a href=\"https://librepcb.discourse.group/\">" %
      tr("discussion forum") % "</a>";
  mUi->lblNote->setText(mUi->lblNote->text().arg(forumLink));

  // Load the window geometry and settings.
  // Note: Do not use restoreGeometry(), only store the window size (but not
  // the position) since the dialog shall be centered within the parent window.
  QSettings clientSettings;
  QSize size = clientSettings.value("order_pcb_dialog/window_size").toSize();
  if (size.isValid()) {
    resize(size);
  }
  mUi->cbxOpenBrowser->setChecked(
      clientSettings.value("order_pcb_dialog/auto_open_browser", true)
          .toBool());

  // Request upload information.
  if (repositories.count() > 0) {
    mRequest.reset(new OrderPcbApiRequest(repositories.first()));
    connect(mRequest.data(), &OrderPcbApiRequest::infoRequestSucceeded, this,
            &OrderPcbDialog::infoRequestSucceeded);
    connect(mRequest.data(), &OrderPcbApiRequest::infoRequestFailed, this,
            &OrderPcbDialog::infoRequestFailed);
    mRequest->startInfoRequest();
  } else {
    infoRequestFailed(
        tr("This feature is not available because there is no API server "
           "configured in your workspace settings."));
  }
}

OrderPcbDialog::~OrderPcbDialog() noexcept {
  // Save the window geometry and settings.
  QSettings clientSettings;
  clientSettings.setValue("order_pcb_dialog/window_size", size());
  clientSettings.setValue("order_pcb_dialog/auto_open_browser",
                          mUi->cbxOpenBrowser->isChecked());
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void OrderPcbDialog::infoRequestSucceeded(QUrl infoUrl,
                                          int maxFileSize) noexcept {
  Q_UNUSED(maxFileSize);

  // Leave the UI busy state.
  mUi->progressBar->hide();
  setStatus(QString());

  // If we received an information URL, display it in the UI.
  if (infoUrl.isValid()) {
    QString link = QString("<a href=\"%1\">%2</a>")
                       .arg(infoUrl.toString(), infoUrl.toDisplayString());
    mUi->lblMoreInformation->setText(mUi->lblMoreInformation->text().arg(link));
    mUi->lblMoreInformation->show();
  }

  // Enable UI elements required to start the upload.
  mUi->cbxOpenBrowser->show();
  mUi->btnUpload->show();
}

void OrderPcbDialog::infoRequestFailed(QString errorMsg) noexcept {
  qWarning() << "Failed to request order information from server:" << errorMsg;

  mUi->progressBar->hide();
  setError(errorMsg);
}

void OrderPcbDialog::uploadButtonClicked() noexcept {
  // Lock UI during work.
  mUi->btnUpload->hide();
  mUi->progressBar->setMaximum(100);
  mUi->progressBar->setValue(0);
  mUi->progressBar->show();
  setStatus(tr("Exporting project..."));

  // To get the UI updated immediately, delay the upload slightly.
  QTimer::singleShot(5, this, &OrderPcbDialog::startUpload);
}

void OrderPcbDialog::startUpload() noexcept {
  try {
    // Sanity check.
    if ((!mRequest) || (!mCreateLppzCallback)) {
      throw LogicError(__FILE__, __LINE__);
    }

    // Generate *.lppz.
    qDebug() << "Export project to *.lppz for ordering PCBs...";
    QByteArray lppz = mCreateLppzCallback();  // can throw
    mUi->progressBar->setValue(10);

    // Start uploading project.
    qDebug() << "Upload *.lppz to API server...";
    setStatus(tr("Uploading project..."));
    connect(mRequest.data(), &OrderPcbApiRequest::uploadProgressState, this,
            &OrderPcbDialog::setStatus);
    connect(mRequest.data(), &OrderPcbApiRequest::uploadProgressPercent, this,
            &OrderPcbDialog::uploadProgressPercent);
    connect(mRequest.data(), &OrderPcbApiRequest::uploadSucceeded, this,
            &OrderPcbDialog::uploadSucceeded);
    connect(mRequest.data(), &OrderPcbApiRequest::uploadFailed, this,
            &OrderPcbDialog::uploadFailed);
    mRequest->startUpload(lppz, mBoardRelativePath);
  } catch (const Exception& e) {
    uploadFailed(e.getMsg());
  }
}

void OrderPcbDialog::uploadProgressPercent(int percent) noexcept {
  mUi->progressBar->setValue(10 + ((percent * 8) / 10));
}

void OrderPcbDialog::uploadSucceeded(const QUrl& redirectUrl) noexcept {
  qDebug() << "Successfully uploaded *.lppz to API server:"
           << redirectUrl.toDisplayString();

  mUi->progressBar->setValue(100);
  QString hyperlink =
      QString("<a href=\"%1\">%2</a>")
          .arg(redirectUrl.toString(), redirectUrl.toDisplayString());
  setStatus(tr("Success! Open %1 to continue.",
               "Placeholder is an URL with hyperlink.")
                .arg(hyperlink));
  if (mUi->cbxOpenBrowser->isChecked()) {
    if (QDesktopServices::openUrl(redirectUrl)) {
      // The web browser might need a few seconds to open. Let's keep the dialog
      // open during this time - if the dialog closes immediately but no browser
      // is visible yet, it looks like the feature does not work.
      QTimer::singleShot(5000, this, &OrderPcbDialog::accept);
      setStatus(tr("Success! Opening %1...").arg(hyperlink));
    } else {
      qWarning() << "Failed to open the webbrowser with QDesktopServices.";
    }
  }
}

void OrderPcbDialog::uploadFailed(const QString& errorMsg) noexcept {
  qWarning() << "Failed to upload *.lppz to API server:" << errorMsg;

  mUi->progressBar->hide();
  mUi->btnUpload->show();
  setError(errorMsg);
}

void OrderPcbDialog::setStatus(const QString msg) noexcept {
  mUi->imgError->hide();
  mUi->lblStatus->setText(msg);
  mUi->lblStatus->setStyleSheet("");
}

void OrderPcbDialog::setError(const QString& msg) noexcept {
  mUi->imgError->show();
  mUi->lblStatus->setText(msg);
  mUi->lblStatus->setStyleSheet("QLabel { color: red; }");
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
