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

  // Constructors / Destructor
  ApiEndpoint() = delete;
  ApiEndpoint(const ApiEndpoint& other) = delete;
  explicit ApiEndpoint(const QUrl& url) noexcept;
  ~ApiEndpoint() noexcept;

  // Getters
  const QUrl& getUrl() const noexcept { return mUrl; }

  // General Methods
  void requestLibraryList() const noexcept;
  void requestPartsInformationStatus() const noexcept;
  void requestPartsInformation(const QUrl& url,
                               const QVector<Part>& parts) const noexcept;

  // Operators
  ApiEndpoint& operator=(const ApiEndpoint& rhs) = delete;

signals:
  void libraryListReceived(QList<Library> libs);
  void errorWhileFetchingLibraryList(const QString& errorMsg);
  void errorWhileFetchingPartsInformationStatus(const QString& errorMsg);
  void partsInformationStatusReceived(const QJsonObject& status);
  void partsInformationReceived(const QJsonObject& info);
  void errorWhileFetchingPartsInformation(const QString& errorMsg);

private:  // Methods
  void requestInfo() noexcept;
  void infoResponseReceived(const QByteArray& data) noexcept;
  void requestLibraryList(const QUrl& url) const noexcept;
  void libraryListResponseReceived(const QByteArray& data) noexcept;
  void partsInformationStatusResponseReceived(const QByteArray& data) noexcept;
  void partsInformationResponseReceived(const QByteArray& data) noexcept;

private:  // Data
  QUrl mUrl;
  bool mInfoAvailable;
  QString mInfoError;

  bool mProvidesLibraries;
  bool mProvidesOrder;
  bool mProvidesParts;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
