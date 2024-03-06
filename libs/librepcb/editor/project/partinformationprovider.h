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

#ifndef LIBREPCB_EDITOR_PARTINFORMATIONPROVIDER_H
#define LIBREPCB_EDITOR_PARTINFORMATIONPROVIDER_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/core/fileio/filepath.h>
#include <optional/tl/optional.hpp>

#include <QtCore>
#include <QtGui>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class ApiEndpoint;
class SExpression;

namespace editor {

/*******************************************************************************
 *  Class PartInformationProvider
 ******************************************************************************/

/**
 * @brief Parts information provider & cache
 *
 * To avoid duplicate API requests, received information is cached in the
 * global instance #instance().
 */
class PartInformationProvider final : public QObject {
  Q_OBJECT

public:
  // Types
  struct Part {
    QString mpn;
    QString manufacturer;
    inline bool operator==(const Part& rhs) const noexcept {
      return (mpn == rhs.mpn) && (manufacturer == rhs.manufacturer);
    }
    inline bool operator<(const Part& rhs) const noexcept {
      if (mpn != rhs.mpn) {
        return mpn < rhs.mpn;
      } else {
        return manufacturer < rhs.manufacturer;
      }
    }
  };
  struct PartResource {
    QString name;
    QString mediaType;
    QUrl url;
  };
  struct PartInformation {
    qint64 timestamp;  // Seconds since epoch
    QString mpn;
    QString manufacturer;
    int results;
    QUrl productUrl;  // Empty if N/A
    QUrl pictureUrl;  // Empty if N/A
    QUrl pricingUrl;  // Empty if N/A
    QString status;  // Empty if N/A
    tl::optional<int> availability;  // nullopt if N/A
    QMap<int, qreal> prices;  // Empty if N/A
    QVector<PartResource> resources;  // Empty if N/A

    QString getStatusTr() const noexcept;
    QString getStatusColorName() const noexcept;
    QString getAvailabilityTr() const noexcept;
    QString getAvailabilityColorName() const noexcept;
    qreal getPrice(int quantity = 1) const noexcept;
    QString getPriceStr(int quantity = 1, const char* prefix = "$ ",
                        const char* suffix = "") const noexcept;
    static QString formatQuantity(const QLocale& locale, int qty) noexcept;

    /**
     * @brief Serialize into ::librepcb::SExpression node
     *
     * @param root    Root node to serialize into.
     */
    void serialize(SExpression& root) const;

    void load(const SExpression& node);
  };

  // Constructors / Destructor
  PartInformationProvider(const PartInformationProvider& other) noexcept =
      delete;
  explicit PartInformationProvider(QObject* parent = nullptr) noexcept;
  ~PartInformationProvider() noexcept;

  // Getters
  bool isOperational() const noexcept;
  const QString& getProviderName() const noexcept { return mProviderName; }
  const QUrl& getProviderUrl() const noexcept { return mProviderUrl; }
  const QUrl& getProviderLogoUrl() const noexcept { return mProviderLogoUrl; }
  const QPixmap getProviderLogo() const noexcept { return mProviderLogo; }
  const QUrl& getInfoUrl() const noexcept { return mInfoUrl; }

  // Setters
  void setCacheDir(const FilePath& dir) noexcept;
  void setApiEndpoint(const QUrl& url) noexcept;

  // General Methods
  void startOperation() noexcept;
  std::shared_ptr<PartInformation> getPartInfo(const Part& part) noexcept;
  bool isOngoing(const Part& part) const noexcept;
  void scheduleRequest(const Part& part) noexcept;
  void requestScheduledParts() noexcept;

  // Static Methods
  static PartInformationProvider& instance() noexcept {
    static PartInformationProvider obj;
    return obj;
  }

  // Operator Overloadings
  PartInformationProvider& operator=(
      const PartInformationProvider& rhs) noexcept = delete;

signals:
  void serviceOperational();
  void providerInfoChanged();
  void newPartsInformationAvailable();

private:  // Methods
  void reset() noexcept;
  void requestStatus() noexcept;
  void statusReceived(const QJsonObject& json) noexcept;
  void errorWhileFetchingStatus(const QString& errorMsg) noexcept;
  void partsInformationReceived(const QJsonObject& json) noexcept;
  void errorWhileFetchingPartsInformation(const QString& errorMsg) noexcept;
  void removeOutdatedInformation() noexcept;
  void loadCacheFromDisk() noexcept;
  void saveCacheToDisk() noexcept;

private:  // Data
  // Configuration
  FilePath mCacheFp;
  QScopedPointer<ApiEndpoint> mEndpoint;

  // Error handling
  int mErrorCounter;
  bool mDisabledDueToErrors;

  // Status request state
  qint64 mStatusRequestedTimestamp;
  bool mStatusReceived;
  QString mProviderName;  ///< Valid only if #mStatusReceived is `true`
  QUrl mProviderUrl;  ///< Valid only if #mStatusReceived is `true`
  QUrl mProviderLogoUrl;  ///< Valid only if #mStatusReceived is `true`
  QPixmap mProviderLogo;  ///< Requested asynchronously.
  QUrl mInfoUrl;  ///< Valid only if #mStatusReceived is `true`
  QUrl mQueryUrl;  ///< Valid only if #mStatusReceived is `true`
  int mQueryMaxPartCount;  ///< Valid only if #mStatusReceived is `true`

  // Query request state
  QVector<Part> mScheduledParts;
  QSet<Part> mRequestedParts;

  // Cache
  QMap<Part, std::shared_ptr<PartInformation>> mCache;  // Sorted for file I/O!
  bool mCacheModified;
};

/*******************************************************************************
 *  Non-Member Functions
 ******************************************************************************/

inline uint qHash(const PartInformationProvider::Part& key,
                  uint seed = 0) noexcept {
  return ::qHash(qMakePair(key.mpn, key.manufacturer), seed);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

Q_DECLARE_METATYPE(librepcb::editor::PartInformationProvider::PartInformation)

#endif
