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
#include "repository.h"

#include "../application.h"
#include "network/networkrequest.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

Repository::Repository(const QUrl& url) noexcept : QObject(nullptr), mUrl(url) {
}

Repository::~Repository() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void Repository::requestLibraryList() const noexcept {
  QString path = "/api/v1/libraries/v" % qApp->getFileFormatVersion().toStr();
  requestLibraryList(QUrl(mUrl.toString() % path));
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void Repository::requestLibraryList(const QUrl& url) const noexcept {
  NetworkRequest* request = new NetworkRequest(url);
  request->setHeaderField("Accept", "application/json;charset=UTF-8");
  request->setHeaderField("Accept-Charset", "UTF-8");
  connect(request, &NetworkRequest::errored, this,
          &Repository::errorWhileFetchingLibraryList, Qt::QueuedConnection);
  connect(request, &NetworkRequest::dataReceived, this,
          &Repository::requestedDataReceived, Qt::QueuedConnection);
  request->start();
}

void Repository::requestedDataReceived(const QByteArray& data) noexcept {
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
      qDebug() << "Request more results from repository:" << url.toString();
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

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
