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
#include "partinformationprovider.h"

#include <librepcb/core/exceptions.h>
#include <librepcb/core/fileio/fileutils.h>
#include <librepcb/core/network/apiendpoint.h>
#include <librepcb/core/network/networkrequest.h>
#include <librepcb/core/serialization/sexpression.h>
#include <librepcb/core/types/length.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Struct PartInformation
 ******************************************************************************/

QString PartInformationProvider::PartInformation::getStatusTr() const noexcept {
  const QHash<QString, QString> translations = {
      //: Part lifecycle status. Please keep it very very short!
      {"preview", tr("Preview")},
      //: Part lifecycle status. Please keep it very very short!
      {"active", tr("Active")},
      //: Part lifecycle status. Please keep it very very short! Don't use
      //: "not recommended for new designs"! If in doubt, just keep the
      //: English abbreviation.
      {"nrnd", tr("NRND")},
      //: Part lifecycle status. Please keep it very very short!
      {"obsolete", tr("Obsolete")},
  };
  return translations.value(status.toLower(), status);
}

QString PartInformationProvider::PartInformation::getStatusColorName()
    const noexcept {
  const QHash<QString, QString> map = {
      {"preview", "blue"},
      {"active", "lime"},
      {"nrnd", "gray"},
      {"obsolete", "red"},
  };
  return map.value(status.toLower());
}

QString PartInformationProvider::PartInformation::getAvailabilityTr()
    const noexcept {
  QString s;
  if (availability) {
    if (*availability > 5) {
      //: Part supplier availability. Please keep it relatively short!
      s = tr("Excellent Availability");
    } else if (*availability > 0) {
      //: Part supplier availability. Please keep it relatively short!
      s = tr("Good Availability");
    } else if (*availability > -5) {
      //: Part supplier availability. Please keep it relatively short!
      s = tr("Available");
    } else if (*availability > -10) {
      //: Part supplier availability. Please keep it relatively short!
      s = tr("Bad Availability");
    } else {
      //: Part supplier availability. Please keep it relatively short!
      s = tr("Not Available");
    }
  }
  return s;
}

QString PartInformationProvider::PartInformation::getAvailabilityColorName()
    const noexcept {
  QString s;
  if (availability) {
    if (*availability > 5) {
      s = "lime";
    } else if (*availability > 0) {
      s = "green";
    } else if (*availability > -5) {
      s = "gold";
    } else if (*availability > -10) {
      s = "darkorange";
    } else {
      s = "red";
    }
  }
  return s;
}

qreal PartInformationProvider::PartInformation::getPrice(
    int quantity) const noexcept {
  qreal price = 0;
  for (auto it = prices.begin(); it != prices.end(); it++) {
    if ((quantity >= it.key()) || (price == 0)) {
      price = it.value();
    }
  }
  return price;
}

QString PartInformationProvider::PartInformation::getPriceStr(
    int quantity, const char* prefix, const char* suffix) const noexcept {
  QString s;
  if (auto price = getPrice(quantity)) {
    s = QString("%1%2").arg(prefix).arg(price, 0, 'f', 3);
    if (s.endsWith('0')) s.chop(1);
    s += suffix;
  }
  return s;
}

void PartInformationProvider::PartInformation::serialize(
    SExpression& root) const {
  root.appendChild("mpn", mpn);
  root.appendChild("manufacturer", manufacturer);
  root.ensureLineBreak();
  root.appendChild("timestamp", timestamp);
  root.ensureLineBreak();
  root.appendChild("results", results);
  root.ensureLineBreak();
  if (!status.isEmpty()) {
    root.appendChild("status", status);
    root.ensureLineBreak();
  }
  if (availability) {
    root.appendChild("availability", *availability);
    root.ensureLineBreak();
  }
  if (!productUrl.isEmpty()) {
    root.appendChild("product_url", productUrl);
    root.ensureLineBreak();
  }
  if (!pictureUrl.isEmpty()) {
    root.appendChild("picture_url", pictureUrl);
    root.ensureLineBreak();
  }
  if (!pricingUrl.isEmpty()) {
    root.appendChild("pricing_url", pricingUrl);
    root.ensureLineBreak();
  }
  for (auto it = prices.begin(); it != prices.end(); it++) {
    SExpression& child = root.appendList("price");
    child.appendChild("quantity", it.key());
    child.appendChild("price", Length::fromMm(it.value()));  // No comment!
    root.ensureLineBreak();
  }
  foreach (const PartResource& resource, resources) {
    SExpression& child = root.appendList("resource");
    child.appendChild("name", resource.name);
    child.appendChild("media_type", resource.mediaType);
    child.ensureLineBreak();
    child.appendChild("url", resource.url);
    root.ensureLineBreak();
  }
}

void PartInformationProvider::PartInformation::load(const SExpression& node) {
  timestamp = deserialize<qint64>(node.getChild("timestamp/@0"));
  mpn = node.getChild("mpn/@0").getValue();
  manufacturer = node.getChild("manufacturer/@0").getValue();
  if (const SExpression* e = node.tryGetChild("results/@0")) {
    results = deserialize<int>(*e);
  }
  if (const SExpression* e = node.tryGetChild("product_url/@0")) {
    productUrl = deserialize<QUrl>(*e);
  }
  if (const SExpression* e = node.tryGetChild("picture_url/@0")) {
    pictureUrl = deserialize<QUrl>(*e);
  }
  if (const SExpression* e = node.tryGetChild("pricing_url/@0")) {
    pricingUrl = deserialize<QUrl>(*e);
  }
  if (const SExpression* e = node.tryGetChild("status/@0")) {
    status = e->getValue();
  }
  if (const SExpression* e = node.tryGetChild("availability/@0")) {
    availability = deserialize<int>(*e);
  }
  foreach (const SExpression* child, node.getChildren("price")) {
    prices.insert(deserialize<int>(child->getChild("quantity/@0")),
                  deserialize<Length>(child->getChild("price/@0")).toMm());
  }
  foreach (const SExpression* child, node.getChildren("resource")) {
    resources.append(
        PartResource{child->getChild("name/@0").getValue(),
                     child->getChild("media_type/@0").getValue(),
                     deserialize<QUrl>(child->getChild("url/@0"))});
  }
}

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

PartInformationProvider::PartInformationProvider(QObject* parent) noexcept
  : QObject(parent),
    mCacheFp(),
    mErrorCounter(0),
    mDisabledDueToErrors(false),
    mStatusRequestedTimestamp(0),
    mStatusReceived(false),
    mQueryMaxPartCount(0),
    mCacheModified(false) {
  // Try to recover from errors every hour.
  {
    QTimer* timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, [this]() {
      if (mDisabledDueToErrors) {
        qInfo() << "Reset parts information provider to recover from errors.";
        reset();
      }
    });
    timer->start(3600 * 1000);
  }

  // Clean up cache regularly and save it to disk.
  {
    QTimer* timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, [this]() {
      removeOutdatedInformation();
      saveCacheToDisk();
    });
    timer->start(15 * 60 * 1000);
  }

  // Save cache before exiting the application.
  connect(qApp, &QGuiApplication::aboutToQuit, this,
          &PartInformationProvider::saveCacheToDisk);
}

PartInformationProvider::~PartInformationProvider() noexcept {
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

bool PartInformationProvider::isOperational() const noexcept {
  return mEndpoint && mStatusReceived && mQueryUrl.isValid() &&
      (!mDisabledDueToErrors) && (mQueryMaxPartCount > 0);
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void PartInformationProvider::setCacheDir(const FilePath& dir) noexcept {
  mCacheFp = dir.getPathTo("parts.lp");
  loadCacheFromDisk();
}

void PartInformationProvider::setApiEndpoint(const QUrl& url) noexcept {
  if (mEndpoint && (mEndpoint->getUrl() == url)) {
    return;
  }

  mEndpoint.reset();
  if (url.isValid()) {
    mEndpoint.reset(new ApiEndpoint(url));
    connect(mEndpoint.data(),
            &ApiEndpoint::errorWhileFetchingPartsInformationStatus, this,
            &PartInformationProvider::errorWhileFetchingStatus);
    connect(mEndpoint.data(), &ApiEndpoint::partsInformationStatusReceived,
            this, &PartInformationProvider::statusReceived);
    connect(mEndpoint.data(), &ApiEndpoint::errorWhileFetchingPartsInformation,
            this, &PartInformationProvider::errorWhileFetchingPartsInformation);
    connect(mEndpoint.data(), &ApiEndpoint::partsInformationReceived, this,
            &PartInformationProvider::partsInformationReceived);
  }
  reset();
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void PartInformationProvider::startOperation() noexcept {
  requestStatus();
}

std::shared_ptr<PartInformationProvider::PartInformation>
    PartInformationProvider::getPartInfo(const Part& part) noexcept {
  return mCache.value(part);
}

bool PartInformationProvider::isOngoing(const Part& part) const noexcept {
  return mScheduledParts.contains(part) || mRequestedParts.contains(part);
}

void PartInformationProvider::scheduleRequest(const Part& part) noexcept {
  if (isOperational() && (!mScheduledParts.contains(part))) {
    mScheduledParts.append(part);
  }
}

void PartInformationProvider::requestScheduledParts() noexcept {
  if ((!isOperational()) || (!mRequestedParts.isEmpty()) ||
      (mScheduledParts.isEmpty())) {
    return;
  }

  QVector<ApiEndpoint::Part> batch;
  const int batchSize = std::min(mScheduledParts.count(), mQueryMaxPartCount);
  for (int i = 0; i < batchSize; ++i) {
    const auto& part = mScheduledParts.at(i);
    if (mCache.contains(part)) {
      qWarning() << "Requested part information of already cached part.";
    }
    batch.append(ApiEndpoint::Part{part.mpn, part.manufacturer});
    mRequestedParts.insert(part);
  }
  mScheduledParts.remove(0, batchSize);
  mEndpoint->requestPartsInformation(mQueryUrl, batch);
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void PartInformationProvider::reset() noexcept {
  mErrorCounter = 0;
  mDisabledDueToErrors = false;
  mStatusRequestedTimestamp = 0;
  mStatusReceived = mEndpoint.isNull();
  mProviderName.clear();
  mProviderUrl.clear();
  mProviderLogoUrl.clear();
  mProviderLogo = QPixmap();
  mInfoUrl.clear();
  mQueryUrl.clear();
  mQueryMaxPartCount = 0;
  mScheduledParts.clear();
  mRequestedParts.clear();
  emit providerInfoChanged();
}

void PartInformationProvider::requestStatus() noexcept {
  const qint64 ts = QDateTime::currentSecsSinceEpoch();
  if (mEndpoint && (!mStatusReceived) && (!mDisabledDueToErrors) &&
      (ts - mStatusRequestedTimestamp > 30)) {
    mStatusRequestedTimestamp = ts;
    mEndpoint->requestPartsInformationStatus();
  }
}

void PartInformationProvider::statusReceived(const QJsonObject& json) noexcept {
  mProviderName = json["provider_name"].toString();
  mProviderUrl = json["provider_url"].toString();
  mProviderLogoUrl = json["provider_logo_url"].toString();
  mInfoUrl = json["info_url"].toString();
  mQueryUrl = json["query_url"].toString();
  mQueryMaxPartCount = std::min(10, json["max_parts"].toInt());
  mErrorCounter = 0;
  mDisabledDueToErrors = false;
  mStatusRequestedTimestamp = 0;
  mStatusReceived = true;
  emit providerInfoChanged();

  // Request provider logo if an URL is given.
  if (mProviderLogoUrl.isValid()) {
    NetworkRequest* request = new NetworkRequest(mProviderLogoUrl);
    request->setMinimumCacheTime(7 * 24 * 3600);  // 7 days
    connect(
        request, &NetworkRequest::dataReceived, this,
        [this](const QByteArray& data) {
          mProviderLogo.loadFromData(data);
          if (!mProviderLogo.isNull()) {
            emit providerInfoChanged();
          }
        },
        Qt::QueuedConnection);
    request->start();
  }

  if (mQueryUrl.isValid()) {
    qInfo() << "Live parts information API is operational.";
    emit serviceOperational();
  } else {
    qInfo() << "Live parts information API is currently not available.";
  }
}

void PartInformationProvider::errorWhileFetchingStatus(
    const QString& errorMsg) noexcept {
  qCritical().noquote() << "Failed to request parts information API status:"
                        << errorMsg;
  if (mErrorCounter < 1) {
    ++mErrorCounter;
  } else if (!mDisabledDueToErrors) {
    qInfo() << "Live parts information disabled due to errors.";
    mDisabledDueToErrors = true;
  }
}

void PartInformationProvider::partsInformationReceived(
    const QJsonObject& json) noexcept {
  const qint64 timestamp = QDateTime::currentSecsSinceEpoch();
  const auto parts = json["parts"].toArray();
  foreach (const QJsonValue& partJson, parts) {
    const QJsonObject partObj = partJson.toObject();
    const Part part{
        partObj["mpn"].toString(),
        partObj["manufacturer"].toString(),
    };
    std::shared_ptr<PartInformation> info = std::make_shared<PartInformation>();
    info->timestamp = timestamp;
    info->mpn = part.mpn;
    info->manufacturer = part.manufacturer;
    info->results = partObj["results"].toInt();
    info->productUrl = partObj["product_url"].toString();
    info->pictureUrl = partObj["picture_url"].toString();
    info->pricingUrl = partObj["pricing_url"].toString();
    info->status = partObj["status"].toString();
    const int availability = partObj["availability"].toInt(INT_MIN);
    if (availability != INT_MIN) {
      info->availability = availability;
    }
    foreach (const QJsonValue& priceJson, partJson["prices"].toArray()) {
      info->prices.insert(priceJson["quantity"].toInt(),
                          priceJson["price"].toDouble());
    }
    foreach (const QJsonValue& resJson, partJson["resources"].toArray()) {
      info->resources.append(PartResource{
          resJson["name"].toString(),
          resJson["mediatype"].toString(),
          resJson["url"].toString(),
      });
    }
    mCache[part] = info;
    mCacheModified = true;
  }
  mRequestedParts.clear();
  mErrorCounter = 0;
  mDisabledDueToErrors = false;

  qDebug() << "Received live information about" << parts.count() << "parts.";
  emit newPartsInformationAvailable();
  requestScheduledParts();  // Request next batch.
}

void PartInformationProvider::errorWhileFetchingPartsInformation(
    const QString& errorMsg) noexcept {
  qCritical().noquote() << "Failed to request parts information:" << errorMsg;
  mRequestedParts.clear();
  if (mErrorCounter < 3) {
    ++mErrorCounter;
  } else if (!mDisabledDueToErrors) {
    qInfo() << "Live parts information disabled due to errors.";
    mDisabledDueToErrors = true;
  }
}

void PartInformationProvider::removeOutdatedInformation() noexcept {
  int count = 0;
  const qint64 timestamp = QDateTime::currentSecsSinceEpoch();
  foreach (const Part& part, mCache.keys()) {
    const qint64 lifetimeSecs = timestamp - mCache.value(part)->timestamp;
    if (lifetimeSecs > 6 * 3600) {  // 6 hours
      mCache.remove(part);
      mCacheModified = true;
      ++count;
    }
  }
  qDebug() << "Cleaned outdated live information about" << count << "parts.";
}

void PartInformationProvider::loadCacheFromDisk() noexcept {
  if (!mCacheFp.isExistingFile()) return;

  try {
    const SExpression root =
        SExpression::parse(FileUtils::readFile(mCacheFp), mCacheFp);
    foreach (const SExpression* node, root.getChildren("part")) {
      std::shared_ptr<PartInformation> info =
          std::make_shared<PartInformation>();
      info->load(*node);
      const Part part{info->mpn, info->manufacturer};
      auto it = mCache.find(part);
      if ((it == mCache.end()) || (info->timestamp > it.value()->timestamp)) {
        mCache.insert(part, info);
      }
    }
    qInfo().nospace() << "Loaded parts information cache from "
                      << mCacheFp.toNative() << ".";
  } catch (const Exception& e) {
    qCritical().nospace() << "Failed to load parts information cache from "
                          << mCacheFp.toNative() << ": " << e.getMsg();
  }

  removeOutdatedInformation();
}

void PartInformationProvider::saveCacheToDisk() noexcept {
  if (!mCacheModified) return;

  try {
    SExpression root = SExpression::createList("librepcb_parts_cache");
    root.ensureLineBreak();
    for (auto it = mCache.begin(); it != mCache.end(); it++) {
      it.value()->serialize(root.appendList("part"));
      root.ensureLineBreak();
    }
    FileUtils::writeFile(mCacheFp, root.toByteArray());
    qInfo().nospace() << "Saved parts information cache to "
                      << mCacheFp.toNative() << ".";
    mCacheModified = false;
  } catch (const Exception& e) {
    qCritical().nospace() << "Failed to save parts information cache to "
                          << mCacheFp.toNative() << ": " << e.getMsg();
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
