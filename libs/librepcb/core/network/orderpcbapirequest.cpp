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
#include "orderpcbapirequest.h"

#include "network/networkrequest.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

OrderPcbApiRequest::OrderPcbApiRequest(const QUrl& apiServerUrl,
                                       QObject* parent) noexcept
  : QObject(parent),
    mApiServerUrl(apiServerUrl),
    mInfoUrl(),
    mUploadUrl(),
    mMaxFileSize(-1) {
}

OrderPcbApiRequest::~OrderPcbApiRequest() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void OrderPcbApiRequest::startInfoRequest() noexcept {
  QUrl url = QUrl(mApiServerUrl.toString() % "/api/v1/order");
  NetworkRequest* request = new NetworkRequest(url);
  request->setHeaderField("Accept", "application/json;charset=UTF-8");
  request->setHeaderField("Accept-Charset", "UTF-8");
  connect(request, &NetworkRequest::errored, this,
          &OrderPcbApiRequest::infoRequestFailed, Qt::QueuedConnection);
  connect(request, &NetworkRequest::dataReceived, this,
          &OrderPcbApiRequest::infoRequestResponseReceived,
          Qt::QueuedConnection);
  connect(this, &OrderPcbApiRequest::destroyed, request, &NetworkRequest::abort,
          Qt::QueuedConnection);
  request->start();
}

void OrderPcbApiRequest::startUpload(const QByteArray& lppz,
                                     const QString& boardPath) const noexcept {
  // Check if the info request succeeded.
  if (!mUploadUrl.isValid()) {
    emit uploadFailed("Upload URL not known yet.");  // No tr().
    return;
  }

  // Check file size.
  if ((mMaxFileSize > 0) && (lppz.size() > mMaxFileSize)) {
    const QString size = QLocale().formattedDataSize(lppz.size(), 0);
    emit uploadFailed(
        tr("The project is too large (%1). If you manually added files to "
           "the project directory, you might need to move them out of the "
           "project directory.",
           "Placeholder is the file size.")
            .arg(size));
    return;
  }

  // Build JSON object to be uploaded.
  QJsonObject obj;
  obj.insert("project", QString(lppz.toBase64()));
  obj.insert("board", boardPath.isEmpty() ? nullptr : boardPath);
  QByteArray postData = QJsonDocument(obj).toJson();

  // Upload data to API server.
  NetworkRequest* request = new NetworkRequest(mUploadUrl, postData);
  request->setHeaderField("Content-Type", "application/json");
  request->setHeaderField("Content-Length",
                          QString::number(postData.size()).toUtf8());
  request->setHeaderField("Accept", "application/json;charset=UTF-8");
  request->setHeaderField("Accept-Charset", "UTF-8");
  connect(request, &NetworkRequest::progressState, this,
          &OrderPcbApiRequest::uploadProgressState);
  connect(request, &NetworkRequest::progressPercent, this,
          &OrderPcbApiRequest::uploadProgressPercent, Qt::QueuedConnection);
  connect(request, &NetworkRequest::errored, this,
          &OrderPcbApiRequest::uploadFailed, Qt::QueuedConnection);
  connect(request, &NetworkRequest::dataReceived, this,
          &OrderPcbApiRequest::uploadResponseReceived, Qt::QueuedConnection);
  connect(this, &OrderPcbApiRequest::destroyed, request, &NetworkRequest::abort,
          Qt::QueuedConnection);
  request->start();
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void OrderPcbApiRequest::infoRequestResponseReceived(
    const QByteArray& data) noexcept {
  qDebug().noquote() << "Received JSON:" << data.left(500).replace("\n", " ");
  QJsonDocument doc = QJsonDocument::fromJson(data);
  if (doc.isNull() || doc.isEmpty() || (!doc.isObject())) {
    emit infoRequestFailed("Received JSON object is not valid.");  // No tr().
    return;
  }
  mInfoUrl.setUrl(doc.object().value("info_url").toString());
  mUploadUrl.setUrl(doc.object().value("upload_url").toString());
  mMaxFileSize = doc.object().value("max_size").toInt(-1);
  if (!mUploadUrl.isValid()) {
    // No or invalid upload_url -> consider it as "service not available" so
    // the server can just remove upload_url when out of service.
    emit infoRequestFailed(
        tr("This service is currently not available. Please try again later or "
           "order the PCB manually either with the Gerber export or the *.lppz "
           "export."));
    return;
  }
  emit infoRequestSucceeded(mInfoUrl, mMaxFileSize);
}

void OrderPcbApiRequest::uploadResponseReceived(
    const QByteArray& data) const noexcept {
  qDebug().noquote() << "Received JSON:" << data.left(500).replace("\n", " ");
  QJsonDocument doc = QJsonDocument::fromJson(data);
  if (doc.isNull() || doc.isEmpty() || (!doc.isObject())) {
    emit uploadFailed("Received JSON object is not valid.");  // No tr().
    return;
  }
  QUrl redirectUrl(doc.object().value("redirect_url").toString());
  if (!redirectUrl.isValid()) {
    emit uploadFailed("Received an invalid redirection URL.");  // No tr().
    return;
  }
  emit uploadSucceeded(redirectUrl);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
