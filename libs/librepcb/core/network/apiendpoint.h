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

#ifndef LIBREPCB_CORE_APIENDPOINT_H
#define LIBREPCB_CORE_APIENDPOINT_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../types/uuid.h"
#include "../types/version.h"

#include <QtCore>

#include <optional>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class ApiEndpoint
 ******************************************************************************/

/**
 * @brief Access to a LibrePCB API endpoint
 *
 * @see @ref doc_server_api
 */
class ApiEndpoint final : public QObject {
  Q_OBJECT

public:
  class Exception : public QException {
    QString mMsg;

  public:
    Exception() = delete;
    Exception(const QString& msg) noexcept : mMsg(msg) {}
    Exception(const Exception& other) noexcept : mMsg(other.mMsg) {}
    Exception& operator=(const Exception& rhs) = delete;
    const QString& getMsg() const noexcept { return mMsg; }
    Exception* clone() const override { return new Exception(*this); }
    void raise() const override { throw *this; }
  };

  // Types
  struct OAuthDeviceCodeResult {
    QString deviceCode;
    QUrl verificationUriComplete;
    int expiresInSeconds = 0;
    int intervalSeconds = 0;
  };
  struct OAuthTokenResult {
    QString accessToken;  // If empty, keep polling.
    QString tokenType;
    int expiresInSeconds = 0;
    bool slowDown = false;
  };
  struct UserResult {
    QString email;  // Validated to be non-empty.
  };
  struct Library {
    Uuid uuid;
    QString name;
    QString description;
    QString author;
    Version version;
    bool recommended = false;
    QSet<Uuid> dependencies;
    QUrl iconUrl;
    QUrl downloadUrl;
    qint64 downloadSize = -1;
    QByteArray downloadSha256;
  };
  struct PartsStatusResult {
    QString providerName;  // Not validated.
    QUrl providerUrl;  // Optional.
    QUrl providerLogoUrl;  // Optional.
    QUrl infoUrl;  // Optional.
    QUrl queryUrl;  // If invalid, parts API is (temporarily) not operational.
    int queryMaxPartCount = 0;  // Not validated.
  };
  struct Part {
    QString mpn;
    QString manufacturer;
  };
  struct PartResource {
    QString name;
    QString mediaType;
    QUrl url;
  };
  struct PartInformation {
    QString mpn;
    QString manufacturer;
    int results = 0;
    QUrl productUrl;  // Empty if N/A
    QUrl pictureUrl;  // Empty if N/A
    QUrl pricingUrl;  // Empty if N/A
    QString status;  // Empty if N/A
    std::optional<int> availability;  // nullopt if N/A
    QMap<int, qreal> prices;  // Empty if N/A
    QVector<PartResource> resources;  // Empty if N/A
  };
  struct PartsInformationResult {
    QVector<PartInformation> parts;
  };

  // Constructors / Destructor
  ApiEndpoint() = delete;
  ApiEndpoint(const ApiEndpoint& other) = delete;
  explicit ApiEndpoint(const QUrl& url) noexcept;
  ~ApiEndpoint() noexcept override;

  // Getters
  const QUrl& getUrl() const noexcept { return mUrl; }
  bool hasCredentials() const noexcept;

  // General Methods
  bool deleteCredentials() noexcept;
  bool setAccessToken(const QString& token) noexcept;
  QFuture<OAuthDeviceCodeResult> requestOAuthDeviceCode(
      const QString& clientId, const QString& label) noexcept;
  QFuture<OAuthTokenResult> requestOAuthToken(
      const QString& grantType, const QString& deviceCode) noexcept;
  QFuture<UserResult> requestUser() noexcept;
  QFuture<Library> requestLibraries(bool forceNoCache = false) noexcept;
  QFuture<PartsStatusResult> requestPartsStatus() const noexcept;
  QFuture<PartsInformationResult> requestPartsInformation(
      const QUrl& url, const QVector<Part>& parts) const noexcept;

  // Static Methods
  static std::shared_ptr<ApiEndpoint> get(const QUrl& url) noexcept;

  // Operators
  ApiEndpoint& operator=(const ApiEndpoint& rhs) = delete;

private:  // Methods
  const QString& getToken() const noexcept;
  QFuture<Library> requestLibraries(
      const QUrl& url, bool forceNoCache,
      std::shared_ptr<QPromise<Library>> promise) noexcept;
  void libraryListResponseReceived(
      const QByteArray& data, bool forceNoCache,
      std::shared_ptr<QPromise<Library>> promise) noexcept;

private:  // Data
  QUrl mUrl;
  mutable std::optional<QString> mCachedToken;

  static QMutex sInstancesMutex;
  static QHash<QUrl, std::shared_ptr<ApiEndpoint>> sInstances;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
