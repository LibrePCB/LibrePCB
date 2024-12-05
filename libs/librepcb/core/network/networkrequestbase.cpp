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
#include "networkrequestbase.h"

#include "../application.h"
#include "../exceptions.h"
#include "../types/version.h"
#include "networkaccessmanager.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

NetworkRequestBase::NetworkRequestBase(const QUrl& url,
                                       const QByteArray& postData) noexcept
  : mUrl(url),
    mPostData(postData),
    mExpectedContentSize(-1),
    mMinimumCacheTime(0),
    mStarted(false),
    mAborted(false),
    mErrored(false),
    mFinished(false) {
  Q_ASSERT(QThread::currentThread() != NetworkAccessManager::instance());

  // set initial HTTP header fields
  mRequest.setHeader(QNetworkRequest::UserAgentHeader, getUserAgent());
  mRequest.setRawHeader("Accept-Language", QLocale().name().toUtf8());
  mRequest.setRawHeader("X-LibrePCB-AppVersion",
                        Application::getVersion().toUtf8());
  mRequest.setRawHeader("X-LibrePCB-GitRevision",
                        Application::getGitRevision().toUtf8());
  mRequest.setRawHeader("X-LibrePCB-FileFormatVersion",
                        Application::getFileFormatVersion().toStr().toUtf8());

  // In Qt6, redirect implementation has changed.
  mRequest.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                        QNetworkRequest::ManualRedirectPolicy);

  // create queued connection to let executeRequest() execute in download thread
  connect(this, &NetworkRequestBase::startRequested, this,
          &NetworkRequestBase::executeRequest, Qt::QueuedConnection);
}

NetworkRequestBase::~NetworkRequestBase() noexcept {
  if (mStarted) {
    Q_ASSERT(QThread::currentThread() == NetworkAccessManager::instance());
  } else {
    Q_ASSERT(QThread::currentThread() != NetworkAccessManager::instance());
  }
}

/*******************************************************************************
 *  Public Methods
 ******************************************************************************/

void NetworkRequestBase::setHeaderField(const QByteArray& name,
                                        const QByteArray& value) noexcept {
  Q_ASSERT(QThread::currentThread() != NetworkAccessManager::instance());
  Q_ASSERT(!mStarted);
  mRequest.setRawHeader(name, value);
}

void NetworkRequestBase::setCacheLoadControl(
    QNetworkRequest::CacheLoadControl value) noexcept {
  Q_ASSERT(QThread::currentThread() != NetworkAccessManager::instance());
  Q_ASSERT(!mStarted);
  mRequest.setAttribute(QNetworkRequest::CacheLoadControlAttribute, value);
}

void NetworkRequestBase::setExpectedReplyContentSize(qint64 bytes) noexcept {
  Q_ASSERT(QThread::currentThread() != NetworkAccessManager::instance());
  Q_ASSERT(!mStarted);
  mExpectedContentSize = bytes;
}

void NetworkRequestBase::setMinimumCacheTime(int seconds) noexcept {
  Q_ASSERT(QThread::currentThread() != NetworkAccessManager::instance());
  Q_ASSERT(!mStarted);
  mMinimumCacheTime = seconds;
}

void NetworkRequestBase::useBrowserUserAgent() noexcept {
  Q_ASSERT(QThread::currentThread() != NetworkAccessManager::instance());
  Q_ASSERT(!mStarted);
  mRequest.setHeader(QNetworkRequest::UserAgentHeader,
                     "Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:131.0) "
                     "Gecko/20100101 Firefox/131.0");
}

void NetworkRequestBase::start() noexcept {
  Q_ASSERT(QThread::currentThread() != NetworkAccessManager::instance());

  NetworkAccessManager* nam = NetworkAccessManager::instance();
  if (nam) {
    mStarted = true;
    moveToThread(
        nam);  // move event processing of this object to the download thread
    emit progressState(tr("Start request..."));
    emit startRequested();  // execute executeRequest() in download thread
  } else {
    finalize(tr("Fatal error: Download manager is not running."));
  }
}

void NetworkRequestBase::abort() noexcept {
  Q_ASSERT(QThread::currentThread() == NetworkAccessManager::instance());
  if (mReply) {
    emit progressState(tr("Abort request..."));
    mAborted = true;
    mReply->abort();
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void NetworkRequestBase::executeRequest() noexcept {
  Q_ASSERT(QThread::currentThread() == NetworkAccessManager::instance());

  emit progressState(tr("Request started..."));

  // get network access manager object
  NetworkAccessManager* nam = NetworkAccessManager::instance();
  if (!nam) {
    finalize(tr("Network access manager is not running."));
    return;
  }

  // prepare request
  mRequest.setUrl(mUrl);
  try {
    prepareRequest();  // can throw
  } catch (const Exception& e) {
    finalize(e.getMsg());
    return;
  }

  // With Qt6 the AlwaysCache mode doesn't seem to work (receiving empty
  // bytearray in its response), no idea why (bug?). As a workaround, we
  // handle this case manually instead of making a normal network request.
  if (mRequest.attribute(QNetworkRequest::CacheLoadControlAttribute).toInt() ==
      QNetworkRequest::AlwaysCache) {
    QString contentType;
    if (std::unique_ptr<QIODevice> dev =
            nam->readFromCache(mUrl, contentType)) {
      try {
        fetchNewData(*dev);
        finalizeRequest();  // can throw
        finalize(QString(), contentType);
      } catch (const Exception& e) {
        finalize(e.getMsg());
      }
    } else {
      finalize("Not in cache.");  // No tr().
    }
    return;
  }

  // start request
  if (!mPostData.isNull()) {
    mReply.reset(nam->post(mRequest, mPostData));
  } else {
    mReply.reset(nam->get(mRequest));
  }
  if (mReply.isNull()) {
    finalize("Network request failed with unknown error!");  // No tr() needed.
    return;
  }

  // connect to signals of reply
  if (!mPostData.isNull()) {
    connect(mReply.data(), &QNetworkReply::uploadProgress, this,
            &NetworkRequestBase::uploadProgressSlot);
  } else {
    connect(mReply.data(), &QNetworkReply::downloadProgress, this,
            &NetworkRequestBase::replyDownloadProgressSlot);
  }
  connect(mReply.data(), &QNetworkReply::readyRead, this,
          &NetworkRequestBase::replyReadyReadSlot);
  connect(mReply.data(), &QNetworkReply::errorOccurred, this,
          &NetworkRequestBase::replyErrorSlot);
  connect(mReply.data(), &QNetworkReply::sslErrors, this,
          &NetworkRequestBase::replySslErrorsSlot);
  connect(mReply.data(), &QNetworkReply::finished, this,
          &NetworkRequestBase::replyFinishedSlot);
}

void NetworkRequestBase::uploadProgressSlot(qint64 bytesSent,
                                            qint64 bytesTotal) noexcept {
  Q_ASSERT(QThread::currentThread() == NetworkAccessManager::instance());
  if (mAborted || mErrored || mFinished) return;
  if (mReply->attribute(QNetworkRequest::RedirectionTargetAttribute).isValid())
    return;

  if (bytesTotal < bytesSent) {
    bytesTotal = bytesSent + 10e6;
  }
  int estimatedPercent = (100 * bytesSent) / qMax(bytesTotal, qint64(1));
  if (bytesSent == bytesTotal) {
    estimatedPercent = 100;
  }
  emit progressState(tr("Send data: %1").arg(formatFileSize(bytesSent)));
  emit progressPercent(estimatedPercent);
  emit progress(bytesSent, bytesTotal, estimatedPercent);
}

void NetworkRequestBase::replyDownloadProgressSlot(qint64 bytesReceived,
                                                   qint64 bytesTotal) noexcept {
  Q_ASSERT(QThread::currentThread() == NetworkAccessManager::instance());
  if (mAborted || mErrored || mFinished) return;
  if (mReply->attribute(QNetworkRequest::RedirectionTargetAttribute).isValid())
    return;

  qint64 estimatedTotal = (bytesTotal > 0) ? bytesTotal : mExpectedContentSize;
  if (estimatedTotal < bytesReceived) {
    estimatedTotal = bytesReceived + 10e6;
  }
  int estimatedPercent =
      (100 * bytesReceived) / qMax(estimatedTotal, qint64(1));
  emit progressState(tr("Receive data: %1").arg(formatFileSize(bytesReceived)));
  emit progressPercent(estimatedPercent);
  emit progress(bytesReceived, bytesTotal, estimatedPercent);
}

void NetworkRequestBase::replyReadyReadSlot() noexcept {
  Q_ASSERT(QThread::currentThread() == NetworkAccessManager::instance());
  Q_ASSERT(mReply);
  fetchNewData(*mReply);
}

void NetworkRequestBase::replyErrorSlot(
    QNetworkReply::NetworkError code) noexcept {
  Q_ASSERT(QThread::currentThread() == NetworkAccessManager::instance());
  mErrored = true;
  finalize(tr("%1 (%2)").arg(mReply->errorString()).arg(code));
}

void NetworkRequestBase::replySslErrorsSlot(
    const QList<QSslError>& errors) noexcept {
  Q_ASSERT(QThread::currentThread() == NetworkAccessManager::instance());
  mErrored = true;
  QStringList errorsList;
  foreach (const QSslError& e, errors) {
    errorsList.append(e.errorString());
  }
  finalize(tr("SSL errors occurred:\n\n%1").arg(errorsList.join("\n")));
}

void NetworkRequestBase::replyFinishedSlot() noexcept {
  Q_ASSERT(QThread::currentThread() == NetworkAccessManager::instance());

  // check if an error was already handled
  if (mErrored) {
    return;
  }

  // check if the request was aborted
  if (mAborted) {
    finalize(tr("Network request aborted."));
    return;
  }

  // check if we received a redirection
  QUrl redirectUrl =
      mReply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
  if (!redirectUrl.isEmpty()) {
    redirectUrl = mUrl.resolved(redirectUrl);  // handle relative URLs
    if (mRedirectedUrls.contains(redirectUrl)) {
      finalize(tr("Redirection loop detected."));
      return;
    } else if (mRedirectedUrls.count() > 10) {
      finalize(tr("Too many redirects."));
      return;
    } else {
      // follow redirection
      qDebug().nospace() << "Redirect from " << mUrl.toString() << " to "
                         << redirectUrl.toString() << ".";
      emit progressState(tr("Redirect to %1...").arg(redirectUrl.toString()));
      mReply.take()->deleteLater();
      mRedirectedUrls.append(mUrl);
      mUrl = redirectUrl;
      executeRequest();  // restart download with new url
      return;
    }
  }

  // check for download error
  if (mReply->error() != QNetworkReply::NoError) {
    finalize(tr("%1 (%2)").arg(mReply->errorString()).arg(mReply->error()));
    return;
  }

  // finalize download
  try {
    finalizeRequest();  // can throw
  } catch (const Exception& e) {
    finalize(e.getMsg());
    return;
  }

  // download successfully finished!
  const QString contentType =
      mReply->header(QNetworkRequest::ContentTypeHeader).toString();
  finalize(QString(), contentType);
}

void NetworkRequestBase::finalize(const QString& errorMsg,
                                  const QString& contentType) noexcept {
  Q_ASSERT(QThread::currentThread() == NetworkAccessManager::instance());

  const bool onlyFromCache =
      (mRequest.attribute(QNetworkRequest::CacheLoadControlAttribute).toInt() ==
       QNetworkRequest::AlwaysCache);

  if (errorMsg.isNull()) {
    // If a minimum cache time was specified, apply it to the cache now.
    NetworkAccessManager* nam = NetworkAccessManager::instance();
    const bool fromCache = onlyFromCache ||
        (mReply &&
         mReply->attribute(QNetworkRequest::SourceIsFromCacheAttribute)
             .toBool());
    const bool cacheExtended = (!fromCache) && (mMinimumCacheTime > 0) && nam &&
        nam->setMinimumCacheExpirationDate(
            mUrl, QDateTime::currentDateTimeUtc().addSecs(mMinimumCacheTime));
    qDebug().nospace().noquote()
        << "Request succeeded: " << mUrl.toString()
        << (cacheExtended ? QString(" (set max_age=%1)").arg(mMinimumCacheTime)
                          : (fromCache ? " (from cache)" : ""));
    emit progressState(tr("Request successfully finished."));
    emitSuccessfullyFinishedSignals(contentType);
    emit succeeded();
    emit finished(true);
  } else if (mAborted) {
    qDebug() << "Request aborted:" << mUrl.toString();
    emit progressState(tr("Request aborted."));
    emit aborted();
    emit finished(false);
  } else if (onlyFromCache) {
    qDebug() << "Not in cache:" << mUrl.toString();
    emit progressState(QString("Not in cache."));  // No tr().
    emit errored(errorMsg);
    emit finished(false);
  } else {
    qCritical() << "Request failed:" << mUrl.toString();
    qCritical() << "Network error:" << errorMsg;
    emit progressState(tr("Request failed: %1").arg(errorMsg));
    emit errored(errorMsg);
    emit finished(false);
  }
  mFinished = true;
  deleteLater();
}

/*******************************************************************************
 *  Static Methods
 ******************************************************************************/

QString NetworkRequestBase::formatFileSize(qint64 bytes) noexcept {
  qreal num = bytes;
  QStringList list({"KB", "MB", "GB", "TB"});
  QStringListIterator i(list);
  QString unit("Bytes");
  while (num >= 1024.0 && i.hasNext()) {
    unit = i.next();
    num /= 1024;
  }
  return QString::number(num, 'f', 2) % " " % unit;
}

QString NetworkRequestBase::getUserAgent() noexcept {
  static QString userAgent;
  if (userAgent.isEmpty()) {
    QStringList details;
    details << QSysInfo::prettyProductName();
    details << QSysInfo::currentCpuArchitecture();
    details << QLocale::system().name();
    userAgent =
        QString("LibrePCB/%1 (%2) Qt/%3")
            .arg(Application::getVersion(),
                 details.join("; ").remove("(").remove(")"), qVersion());
  }
  return userAgent;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
