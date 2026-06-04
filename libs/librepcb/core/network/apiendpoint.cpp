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

#include <librepcb/rust-core/ffi.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

QMutex ApiEndpoint::sInstancesMutex;
QHash<QUrl, std::shared_ptr<ApiEndpoint>> ApiEndpoint::sInstances;

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

bool ApiEndpoint::hasCredentials() const noexcept {
  return !getToken().isEmpty();
}

bool ApiEndpoint::deleteCredentials() noexcept {
  const QString service = "librepcb";
  const QString user = mUrl.toString();
  const bool success = rs::ffi_keyring_delete_credentials(&service, &user);
  if (!success) {
    qWarning().nospace() << "Failed to delete credentials for "
                         << mUrl.toString() << ".";
  }
  mCachedToken = std::nullopt;
  return success;
}

bool ApiEndpoint::setAccessToken(const QString& token) noexcept {
  const QString service = "librepcb";
  const QString user = mUrl.toString();
  const bool success = rs::ffi_keyring_set_password(&service, &user, &token);
  if (!success) {
    qWarning().nospace() << "Failed to set access token for " << mUrl.toString()
                         << ".";
  }
  mCachedToken = std::nullopt;
  return success;
}

QFuture<ApiEndpoint::OAuthDeviceCodeResult> ApiEndpoint::requestOAuthDeviceCode(
    const QString& clientId, const QString& label) noexcept {
  auto promise = std::make_shared<QPromise<OAuthDeviceCodeResult>>();
  promise->start();

  auto errorCallback = [promise](const QString& errorMsg) {
    promise->setException(Exception(errorMsg));
    promise->finish();
  };

  auto successCallback = [promise](const QByteArray& data) {
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull() || doc.isEmpty() || (!doc.isObject())) {
      promise->setException(Exception("Received JSON object is not valid."));
      promise->finish();
      return;
    }
    const OAuthDeviceCodeResult result{
        doc.object().value("device_code").toString(),
        QUrl(doc.object().value("verification_uri_complete").toString(),
             QUrl::StrictMode),
        doc.object().value("expires_in").toInt(),
        doc.object().value("interval").toInt(),
    };
    if (result.deviceCode.isEmpty() ||
        (!result.verificationUriComplete.isValid()) ||
        result.verificationUriComplete.isLocalFile() ||
        (result.expiresInSeconds <= 0) || (result.intervalSeconds <= 0)) {
      promise->setException(
          Exception("Received invalid device code response from server."));
      promise->finish();
      return;
    }
    promise->addResult(result);
    promise->finish();
  };

  const QJsonObject obj{
      {"client_id", clientId},
      {"label", label},
  };
  const QByteArray postData = QJsonDocument(obj).toJson();

  NetworkRequest* request = new NetworkRequest(
      QUrl(mUrl.toString() % "/api/v1/oauth/device/code"), postData);
  request->setHeaderField("Content-Type", "application/json");
  request->setHeaderField("Content-Length",
                          QString::number(postData.size()).toUtf8());
  request->setHeaderField("Accept", "application/json;charset=UTF-8");
  request->setHeaderField("Accept-Charset", "UTF-8");
  connect(request, &NetworkRequest::errored, this, errorCallback,
          Qt::QueuedConnection);
  connect(request, &NetworkRequest::dataReceived, this, successCallback,
          Qt::QueuedConnection);
  request->start();

  return promise->future();
}

void ApiEndpoint::requestOAuthToken(
    const QString& grantType, const QString& deviceCode, QObject* receiver,
    const OAuthTokenCallback& callback) noexcept {
  auto errorCallback = [callback](const QString& errorMsg) {
    callback(errorMsg, QString(), QString(), 0);
  };

  auto successCallback = [callback](const QByteArray& data) {
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull() || doc.isEmpty() || (!doc.isObject())) {
      callback("Received JSON object is not valid.", QString(), QString(), 0);
      return;
    }
    const QString error = doc.object().value("error").toString();
    const QString accessToken = doc.object().value("access_token").toString();
    const QString tokenType = doc.object().value("token_type").toString();
    const int expiresIn = doc.object().value("expires_in").toInt();
    if ((!accessToken.isEmpty()) && (!tokenType.isEmpty()) && (expiresIn > 0)) {
      callback(QString(), accessToken, tokenType, expiresIn);
    } else if ((error == "authorization_pending") || (error == "slow_down")) {
      callback(QString(), QString(), QString(), 0);
    } else if (!error.isEmpty()) {
      callback("Received error from server: " % error, QString(), QString(), 0);
    } else {
      callback("Received invalid data from server: " % QString(data), QString(),
               QString(), 0);
    }
  };

  const QJsonObject obj{
      {"grant_type", grantType},
      {"device_code", deviceCode},
  };
  const QByteArray postData = QJsonDocument(obj).toJson();

  NetworkRequest* request = new NetworkRequest(
      QUrl(mUrl.toString() % "/api/v1/oauth/token"), postData);
  request->setHeaderField("Content-Type", "application/json");
  request->setHeaderField("Content-Length",
                          QString::number(postData.size()).toUtf8());
  request->setHeaderField("Accept", "application/json;charset=UTF-8");
  request->setHeaderField("Accept-Charset", "UTF-8");
  connect(request, &NetworkRequest::errored, receiver, errorCallback,
          Qt::QueuedConnection);
  connect(request, &NetworkRequest::dataReceived, receiver, successCallback,
          Qt::QueuedConnection);
  request->start();
}

QFuture<ApiEndpoint::Library> ApiEndpoint::requestLibraries(
    bool forceNoCache) noexcept {
  auto promise = std::make_shared<QPromise<Library>>();
  promise->start();
  const QString path =
      "/api/v1/libraries/v" % Application::getFileFormatVersion().toStr();
  return requestLibraries(QUrl(mUrl.toString() % path), forceNoCache, promise);
}

void ApiEndpoint::requestPartsStatus(
    QObject* receiver, const PartsStatusCallback& callback) const noexcept {
  auto errorCallback = [callback](const QString& errorMsg) {
    callback(errorMsg, QJsonObject());
  };

  auto successCallback = [callback](const QByteArray& data) {
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull() || doc.isEmpty() || (!doc.isObject())) {
      callback("Received JSON object is not valid.", QJsonObject());
    } else {
      callback(QString(), doc.object());
    }
  };

  NetworkRequest* request =
      new NetworkRequest(QUrl(mUrl.toString() % "/api/v1/parts"));
  request->setHeaderField("Accept", "application/json;charset=UTF-8");
  request->setHeaderField("Accept-Charset", "UTF-8");
  connect(request, &NetworkRequest::errored, receiver, errorCallback,
          Qt::QueuedConnection);
  connect(request, &NetworkRequest::dataReceived, receiver, successCallback,
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
 *  Static Methods
 ******************************************************************************/

std::shared_ptr<ApiEndpoint> ApiEndpoint::get(const QUrl& url) noexcept {
  QMutexLocker lock(&sInstancesMutex);
  auto it = sInstances.find(url);
  if (it == sInstances.end()) {
    it = sInstances.insert(url, std::make_shared<ApiEndpoint>(url));
  }
  return *it;
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

const QString& ApiEndpoint::getToken() const noexcept {
  if (!mCachedToken) {
    QString token;
    const QString service = "librepcb";
    const QString user = mUrl.toString();
    if (!rs::ffi_keyring_get_password(&service, &user, &token)) {
      qWarning() << "No authentication token found for" << mUrl;
    }
    mCachedToken = token;
  }
  return *mCachedToken;
}

QFuture<ApiEndpoint::Library> ApiEndpoint::requestLibraries(
    const QUrl& url, bool forceNoCache,
    std::shared_ptr<QPromise<Library>> promise) noexcept {
  NetworkRequest* request = new NetworkRequest(url);
  request->setHeaderField("Accept", "application/json;charset=UTF-8");
  request->setHeaderField("Accept-Charset", "UTF-8");
  if (forceNoCache) {
    request->setCacheLoadControl(QNetworkRequest::AlwaysNetwork);
  }
  connect(
      request, &NetworkRequest::errored, this,
      [promise](const QString& errorMsg) {
        promise->setException(Exception(errorMsg));
        promise->finish();
      },
      Qt::QueuedConnection);
  connect(
      request, &NetworkRequest::dataReceived, this,
      [this, promise, forceNoCache](const QByteArray& data) {
        libraryListResponseReceived(data, forceNoCache, promise);
      },
      Qt::QueuedConnection);
  request->start();

  return promise->future();
}

void ApiEndpoint::libraryListResponseReceived(
    const QByteArray& data, bool forceNoCache,
    std::shared_ptr<QPromise<Library>> promise) noexcept {
  QJsonDocument doc = QJsonDocument::fromJson(data);
  if (doc.isNull() || doc.isEmpty() || (!doc.isObject())) {
    promise->setException(Exception("Received JSON object is not valid."));
    promise->finish();
    return;
  }
  QJsonValue nextResultsLink = doc.object().value("next");
  if (nextResultsLink.isString()) {
    QUrl url = QUrl(nextResultsLink.toString());
    if (url.isValid()) {
      qDebug().nospace() << "Request more results from API endpoint "
                         << url.toString() << "...";
      requestLibraries(url, forceNoCache, promise);
    } else {
      qWarning() << "Invalid URL in received JSON object:"
                 << nextResultsLink.toString();
    }
  }
  const QJsonValue results = doc.object().value("results");
  if ((results.isNull()) || (!results.isArray())) {
    promise->setException(
        Exception("Received JSON object does not contain any results."));
    promise->finish();
    return;
  }

  // Parse JSON.
  const QJsonArray resultsArray = results.toArray();
  for (const QJsonValue& item : resultsArray) {
    const QJsonObject obj = item.toObject();
    auto uuid = Uuid::tryFromString(obj.value("uuid").toString());
    auto version = Version::tryFromString(obj.value("version").toString());
    if (!uuid) {
      qCritical() << "Invalid UUID received:" << obj.value("uuid").toString();
      continue;
    }
    if (!version) {
      qCritical() << "Invalid version received:"
                  << obj.value("version").toString();
      continue;
    }
    Library lib{
        *uuid,
        obj.value("name").toObject().value("default").toString(),
        obj.value("description").toObject().value("default").toString(),
        obj.value("author").toString(),
        *version,
        obj.value("recommended").toBool(),
        {},
        QUrl(obj.value("icon_url").toString()),
        QUrl(obj.value("download_url").toString()),
        obj.value("download_size").toInt(-1),
        obj.value("download_sha256").toString().toUtf8(),
    };
    const QJsonArray dependenciesArray = obj.value("dependencies").toArray();
    for (const QJsonValue& value : dependenciesArray) {
      if (auto uuid = Uuid::tryFromString(value.toString())) {
        lib.dependencies.insert(*uuid);
      } else {
        qWarning() << "Invalid library dependency UUID:" << value.toString();
      }
    }
    promise->addResult(lib);
  }
  promise->finish();
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
