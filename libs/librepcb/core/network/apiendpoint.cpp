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
    const QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull() || doc.isEmpty() || (!doc.isObject())) {
      promise->setException(Exception("Received JSON object is not valid."));
      promise->finish();
      return;
    }
    const QJsonObject obj = doc.object();
    const OAuthDeviceCodeResult result{
        obj["device_code"].toString(),
        QUrl(obj["verification_uri_complete"].toString(), QUrl::StrictMode),
        obj["expires_in"].toInt(),
        obj["interval"].toInt(),
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

QFuture<ApiEndpoint::OAuthTokenResult> ApiEndpoint::requestOAuthToken(
    const QString& grantType, const QString& deviceCode) noexcept {
  auto promise = std::make_shared<QPromise<OAuthTokenResult>>();
  promise->start();

  auto errorCallback = [promise](const QString& errorMsg) {
    promise->setException(Exception(errorMsg));
    promise->finish();
  };

  auto successCallback = [promise](const QByteArray& data) {
    const QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull() || doc.isEmpty() || (!doc.isObject())) {
      promise->setException(Exception("Received JSON object is not valid."));
      promise->finish();
      return;
    }
    const QJsonObject obj = doc.object();
    const QString error = obj["error"].toString();
    const OAuthTokenResult result{
        obj["access_token"].toString(),
        obj["token_type"].toString(),
        obj["expires_in"].toInt(),
        error == "slow_down",
    };
    if ((error == "authorization_pending") || (error == "slow_down") ||
        ((!result.accessToken.isEmpty()) && (!result.tokenType.isEmpty()) &&
         (result.expiresInSeconds > 0))) {
      promise->addResult(result);
    } else if (!error.isEmpty()) {
      promise->setException(Exception("Received error from server: " % error));
    } else {
      promise->setException(
          Exception("Received invalid token response from server."));
    }
    promise->finish();
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
  connect(request, &NetworkRequest::errored, this, errorCallback,
          Qt::QueuedConnection);
  connect(request, &NetworkRequest::dataReceived, this, successCallback,
          Qt::QueuedConnection);
  request->start();

  return promise->future();
}

QFuture<ApiEndpoint::UserResult> ApiEndpoint::requestUser() noexcept {
  auto promise = std::make_shared<QPromise<UserResult>>();
  promise->start();

  auto errorCallback = [promise](const QString& errorMsg) {
    promise->setException(Exception(errorMsg));
    promise->finish();
  };

  auto successCallback = [promise](const QByteArray& data) {
    const QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull() || doc.isEmpty() || (!doc.isObject())) {
      promise->setException(Exception("Received JSON object is not valid."));
      promise->finish();
      return;
    }
    const QJsonObject obj = doc.object();
    const UserResult result{
        obj["email"].toString(),
    };
    if (!result.email.isEmpty()) {
      promise->addResult(result);
    } else {
      promise->setException(
          Exception("Received invalid user response from server."));
    }
    promise->finish();
  };

  NetworkRequest* request =
      new NetworkRequest(QUrl(mUrl.toString() % "/api/v1/user"));
  request->setHeaderField("Accept", "application/json;charset=UTF-8");
  request->setHeaderField("Accept-Charset", "UTF-8");
  connect(request, &NetworkRequest::errored, this, errorCallback,
          Qt::QueuedConnection);
  connect(request, &NetworkRequest::dataReceived, this, successCallback,
          Qt::QueuedConnection);
  request->start();

  return promise->future();
}

QFuture<ApiEndpoint::Library> ApiEndpoint::requestLibraries(
    bool forceNoCache) noexcept {
  auto promise = std::make_shared<QPromise<Library>>();
  promise->start();
  const QString path =
      "/api/v1/libraries/v" % Application::getFileFormatVersion().toStr();
  return requestLibraries(QUrl(mUrl.toString() % path), forceNoCache, promise);
}

QFuture<ApiEndpoint::PartsStatusResult> ApiEndpoint::requestPartsStatus()
    const noexcept {
  auto promise = std::make_shared<QPromise<PartsStatusResult>>();
  promise->start();

  auto errorCallback = [promise](const QString& errorMsg) {
    promise->setException(Exception(errorMsg));
    promise->finish();
  };

  auto successCallback = [promise](const QByteArray& data) {
    const QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull() || doc.isEmpty() || (!doc.isObject())) {
      promise->setException(Exception("Received JSON object is not valid."));
      promise->finish();
      return;
    }
    const QJsonObject obj = doc.object();
    const PartsStatusResult result{
        obj["provider_name"].toString(),
        QUrl(obj["provider_url"].toString(), QUrl::StrictMode),
        QUrl(obj["provider_logo_url"].toString(), QUrl::StrictMode),
        QUrl(obj["info_url"].toString(), QUrl::StrictMode),
        QUrl(obj["query_url"].toString(), QUrl::StrictMode),
        obj["max_parts"].toInt(),
    };
    promise->addResult(result);
    promise->finish();
  };

  NetworkRequest* request =
      new NetworkRequest(QUrl(mUrl.toString() % "/api/v1/parts"));
  request->setHeaderField("Accept", "application/json;charset=UTF-8");
  request->setHeaderField("Accept-Charset", "UTF-8");
  connect(request, &NetworkRequest::errored, this, errorCallback,
          Qt::QueuedConnection);
  connect(request, &NetworkRequest::dataReceived, this, successCallback,
          Qt::QueuedConnection);
  request->start();

  return promise->future();
}

QFuture<ApiEndpoint::PartsInformationResult>
    ApiEndpoint::requestPartsInformation(
        const QUrl& url, const QVector<Part>& parts) const noexcept {
  auto promise = std::make_shared<QPromise<PartsInformationResult>>();
  promise->start();

  auto errorCallback = [promise](const QString& errorMsg) {
    promise->setException(Exception(errorMsg));
    promise->finish();
  };

  auto successCallback = [promise](const QByteArray& data) {
    const QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull() || doc.isEmpty() || (!doc.isObject())) {
      promise->setException(Exception("Received JSON object is not valid."));
      promise->finish();
      return;
    }
    const QJsonObject obj = doc.object();
    PartsInformationResult result{};
    const QJsonArray partsArray = obj["parts"].toArray();
    for (const QJsonValue& partJson : partsArray) {
      const QJsonObject partObj = partJson.toObject();
      PartInformation info{
          partObj["mpn"].toString(),
          partObj["manufacturer"].toString(),
          partObj["results"].toInt(),
          QUrl(partObj["product_url"].toString(), QUrl::StrictMode),
          QUrl(partObj["picture_url"].toString(), QUrl::StrictMode),
          QUrl(partObj["pricing_url"].toString(), QUrl::StrictMode),
          partObj["status"].toString(),
          std::nullopt,  // Availability
          {},  // Prices
          {},  // Resources
      };
      const int availability = partObj["availability"].toInt(INT_MIN);
      if (availability != INT_MIN) {
        info.availability = availability;
      }
      const QJsonArray pricesArray = partObj["prices"].toArray();
      for (const QJsonValue& priceJson : pricesArray) {
        info.prices.insert(priceJson["quantity"].toInt(),
                           priceJson["price"].toDouble());
      }
      const QJsonArray resourcesArray = partObj["resources"].toArray();
      for (const QJsonValue& resJson : resourcesArray) {
        info.resources.append(PartResource{
            resJson["name"].toString(),
            resJson["mediatype"].toString(),
            resJson["url"].toString(),
        });
      }
      result.parts.append(info);
    }
    promise->addResult(result);
    promise->finish();
  };

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
  connect(request, &NetworkRequest::errored, this, errorCallback,
          Qt::QueuedConnection);
  connect(request, &NetworkRequest::dataReceived, this, successCallback,
          Qt::QueuedConnection);
  request->start();

  return promise->future();
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
    auto uuid = Uuid::tryFromString(obj["uuid"].toString());
    auto version = Version::tryFromString(obj["version"].toString());
    if (!uuid) {
      qCritical() << "Invalid UUID received:" << obj["uuid"].toString();
      continue;
    }
    if (!version) {
      qCritical() << "Invalid version received:" << obj["version"].toString();
      continue;
    }
    Library lib{
        *uuid,
        obj["name"].toObject().value("default").toString(),
        obj["description"].toObject().value("default").toString(),
        obj["author"].toString(),
        *version,
        obj["recommended"].toBool(),
        {},
        QUrl(obj["icon_url"].toString()),
        QUrl(obj["download_url"].toString()),
        obj["download_size"].toInt(-1),
        obj["download_sha256"].toString().toUtf8(),
    };
    const QJsonArray dependenciesArray = obj["dependencies"].toArray();
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

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
