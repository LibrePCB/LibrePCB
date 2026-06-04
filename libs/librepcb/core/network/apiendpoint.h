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
  // Types
  struct Part {
    QString mpn;
    QString manufacturer;
  };
  struct Library {
    Uuid uuid;
    QString name;
    QString description;
    QString author;
    Version version;
    bool recommended;
    QSet<Uuid> dependencies;
    QUrl iconUrl;
    QUrl downloadUrl;
    qint64 downloadSize;
    QByteArray downloadSha256;
  };

  // Callbacks
  typedef std::function<void(const QString& errorMsg, const QString& deviceCode,
                             const QUrl& verificationUriComplete,
                             int expiresInSeconds, int intervalSeconds)>
      OAuthDeviceCodeCallback;
  typedef std::function<void(const QString& errorMsg,
                             const QString& accessToken,
                             const QString& tokenType, int expiresIn)>
      OAuthTokenCallback;
  typedef std::function<void(const QString& errorMsg,
                             const QVector<Library>& libraries)>
      LibrariesCallback;
  typedef std::function<void(const QString& errorMsg,
                             const QJsonObject& status)>
      PartsStatusCallback;

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
  void requestOAuthDeviceCode(const QString& clientId, const QString& label,
                              QObject* receiver,
                              const OAuthDeviceCodeCallback& callback) noexcept;
  void requestOAuthToken(const QString& grantType, const QString& deviceCode,
                         QObject* receiver,
                         const OAuthTokenCallback& callback) noexcept;
  void requestLibraries(bool forceNoCache, QObject* receiver,
                        const LibrariesCallback& callback) noexcept;
  void requestPartsStatus(QObject* receiver,
                          const PartsStatusCallback& callback) const noexcept;
  void requestPartsInformation(const QUrl& url,
                               const QVector<Part>& parts) const noexcept;

  // Static Methods
  static std::shared_ptr<ApiEndpoint> get(const QUrl& url) noexcept;

  // Operators
  ApiEndpoint& operator=(const ApiEndpoint& rhs) = delete;

signals:
  void partsInformationReceived(const QJsonObject& info);
  void errorWhileFetchingPartsInformation(const QString& errorMsg);

private:  // Methods
  const QString& getToken() const noexcept;
  void requestLibraries(const QUrl& url, bool forceNoCache) noexcept;
  void libraryListResponseReceived(const QByteArray& data,
                                   bool forceNoCache) noexcept;
  void partsInformationResponseReceived(const QByteArray& data) noexcept;

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
