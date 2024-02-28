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
#include "apiendpoint.h"

#include "../application.h"
#include "../types/version.h"
#include "network/networkrequest.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

ApiEndpoint::ApiEndpoint(const QUrl& url) noexcept
  : QObject(nullptr), mUrl(url) {
}

ApiEndpoint::~ApiEndpoint() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void ApiEndpoint::requestLibraryList() const noexcept {
  QString path =
      "/api/v1/libraries/v" % Application::getFileFormatVersion().toStr();
  requestLibraryList(QUrl(mUrl.toString() % path));
}

void ApiEndpoint::requestPartsInformationStatus() const noexcept {
  NetworkRequest* request =
      new NetworkRequest(QUrl(mUrl.toString() % "/api/v1/parts"));
  request->setHeaderField("Accept", "application/json;charset=UTF-8");
  request->setHeaderField("Accept-Charset", "UTF-8");
  connect(request, &NetworkRequest::errored, this,
          &ApiEndpoint::errorWhileFetchingPartsInformationStatus,
          Qt::QueuedConnection);
  connect(request, &NetworkRequest::dataReceived, this,
          &ApiEndpoint::partsInformationStatusResponseReceived,
          Qt::QueuedConnection);
  request->start();
}

void ApiEndpoint::requestPartsInformation(
    const QUrl& url, const QVector<Part>& parts) const noexcept {
  // Build JSON object to be uploaded.
  QJsonArray partsArray;
  foreach (const Part& part, parts) {
    partsArray.append(QJsonObject{
        {"mpn", part.mpn},
        {"manufacturer", part.manufacturer},
    });
  }
  const QJsonObject obj{
      {"parts", partsArray},
  };
  const QByteArray postData = QJsonDocument(obj).toJson();

  NetworkRequest* request = new NetworkRequest(url, postData);
  request->setHeaderField("Content-Type", "application/json");
  request->setHeaderField("Content-Length",
                          QString::number(postData.size()).toUtf8());
  request->setHeaderField("Accept", "application/json;charset=UTF-8");
  request->setHeaderField("Accept-Charset", "UTF-8");
  connect(request, &NetworkRequest::errored, this,
          &ApiEndpoint::errorWhileFetchingPartsInformation,
          Qt::QueuedConnection);
  connect(request, &NetworkRequest::dataReceived, this,
          &ApiEndpoint::partsInformationResponseReceived, Qt::QueuedConnection);
  request->start();
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void ApiEndpoint::requestLibraryList(const QUrl& url) const noexcept {
  NetworkRequest* request = new NetworkRequest(url);
  request->setHeaderField("Accept", "application/json;charset=UTF-8");
  request->setHeaderField("Accept-Charset", "UTF-8");
  connect(request, &NetworkRequest::errored, this,
          &ApiEndpoint::errorWhileFetchingLibraryList, Qt::QueuedConnection);
  connect(request, &NetworkRequest::dataReceived, this,
          &ApiEndpoint::libraryListResponseReceived, Qt::QueuedConnection);
  request->start();
}

void ApiEndpoint::libraryListResponseReceived(const QByteArray& data) noexcept {
  QJsonDocument doc = QJsonDocument::fromJson(data);
  if (doc.isNull() || doc.isEmpty() || (!doc.isObject())) {
    emit errorWhileFetchingLibraryList(
        tr("Received JSON object is not valid."));
    return;
  }
  QJsonValue nextResultsLink = doc.object().value("next");
  if (nextResultsLink.isString()) {
    QUrl url = QUrl(nextResultsLink.toString());
    if (url.isValid()) {
      qDebug().nospace() << "Request more results from API endpoint "
                         << url.toString() << "...";
      requestLibraryList(url);
    } else {
      qWarning() << "Invalid URL in received JSON object:"
                 << nextResultsLink.toString();
    }
  }
  QJsonValue reposVal = doc.object().value("results");
  if ((reposVal.isNull()) || (!reposVal.isArray())) {
    emit errorWhileFetchingLibraryList(
        tr("Received JSON object does not contain "
           "any results."));
    return;
  }
  emit libraryListReceived(reposVal.toArray());
}

void ApiEndpoint::partsInformationStatusResponseReceived(
    const QByteArray& data) noexcept {
  QJsonDocument doc = QJsonDocument::fromJson(data);
  if (doc.isNull() || doc.isEmpty() || (!doc.isObject())) {
    emit errorWhileFetchingPartsInformation(
        tr("Received JSON object is not valid."));
    return;
  }
  emit partsInformationStatusReceived(doc.object());
}

void ApiEndpoint::partsInformationResponseReceived(
    const QByteArray& data) noexcept {
  QJsonDocument doc = QJsonDocument::fromJson(data);
  if (doc.isNull() || doc.isEmpty() || (!doc.isObject())) {
    emit errorWhileFetchingPartsInformation(
        tr("Received JSON object is not valid."));
    return;
  }
  emit partsInformationReceived(doc.object());
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
