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
#include <librepcb/core/network/apiendpoint.h>

#include <QtCore>
#include <QtGui>

#include <optional>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

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
  struct PartInformation {
    QString source;  // API server URL; Empty for entries from LibrePCB < 2.0
    qint64 timestamp;  // Seconds since epoch
    ApiEndpoint::PartInformation data;  // Raw data from ApiEndpoint

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
  ~PartInformationProvider() noexcept override;

  // Getters
  bool isOperational() const noexcept;
  QString getProviderName() const noexcept;
  QUrl getProviderUrl() const noexcept;
  QUrl getInfoUrl() const noexcept;
  const QPixmap getProviderLogo() const noexcept { return mProviderLogo; }

  // Setters
  void setCacheDir(const FilePath& dir) noexcept;
  void setApiEndpoint(const std::shared_ptr<ApiEndpoint>& ep) noexcept;

  // General Methods
  bool startOperation(int timeoutMs = 0) noexcept;
  std::shared_ptr<PartInformation> getPartInfo(const Part& part) noexcept;
  std::shared_ptr<PartInformation> waitForPartInfo(const Part& part,
                                                   int timeoutMs) noexcept;
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
  void removeOutdatedInformation() noexcept;
  void loadCacheFromDisk() noexcept;
  void saveCacheToDisk() noexcept;

private:  // Data
  // Configuration
  FilePath mCacheFp;
  std::shared_ptr<ApiEndpoint> mEndpoint;
  QString mSource;  ///< URL of #mEndpoint

  // Error handling
  int mErrorCounter;
  bool mDisabledDueToErrors;

  // Status request state
  qint64 mStatusRequestedTimestamp;
  std::optional<ApiEndpoint::PartsStatusResult> mStatusResult;
  QPixmap mProviderLogo;  ///< Requested asynchronously.

  // Query request state
  QVector<Part> mScheduledParts;
  QSet<Part> mRequestedParts;

  // Cache (Sorted for file I/O!)
  QMap<Part, QMap<QString, std::shared_ptr<PartInformation>>> mCache;
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
