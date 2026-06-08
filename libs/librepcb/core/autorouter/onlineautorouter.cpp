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
#include "onlineautorouter.h"

#include "../utils/messagelogger.h"

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

OnlineAutorouter::OnlineAutorouter(
    const std::shared_ptr<ApiEndpoint>& ep, const QString& routerId,
    const std::shared_ptr<MessageLogger>& logger) noexcept
  : Autorouter(logger),
    mEndpoint(ep),
    mRouterId(routerId),
    mPollTimer(new QTimer(this)) {
  Q_ASSERT(mEndpoint);
  mPollTimer->setSingleShot(true);
  connect(mPollTimer.get(), &QTimer::timeout, this,
          [this]() { setFuture(mEndpoint->requestAutorouteStatus(mJobId)); });
}

OnlineAutorouter::~OnlineAutorouter() noexcept {
  mPollRequest.cancel();
  mPollRequest = {};
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void OnlineAutorouter::start(const QByteArray& dsn) noexcept {
  mPollRequest.cancel();
  mPollRequest = {};
  mJobId.clear();

  emit statusNotification(Status{
      State::Running,
      0,
      QByteArray(),
      QString(),
  });

  mLogger->info(
      tr("Sending DSN to '%1'...").arg(mEndpoint->getUrl().toString()));
  setFuture(mEndpoint->requestAutorouteStart(mRouterId, dsn));
}

void OnlineAutorouter::cancel() noexcept {
  mPollTimer->stop();
  mPollRequest.cancel();
  mPollRequest = {};
  mJobId.clear();

  mLogger->info(tr("Job cancelled."));
  emit statusNotification(Status{
      State::Canceled,
      100,
      QByteArray(),
      QString(),
  });
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void OnlineAutorouter::setFuture(
    const QFuture<ApiEndpoint::AutorouteJobResult>& f) noexcept {
  mPollRequest = f;
  mPollRequest
      .then(this,
            [this](const ApiEndpoint::AutorouteJobResult& result) {
              handleStatus(result);
            })
      .onFailed(this,
                [this](const ApiEndpoint::Exception& e) { handleFailure(e); });
}

void OnlineAutorouter::handleStatus(
    const ApiEndpoint::AutorouteJobResult& result) noexcept {
  if (mJobId.isEmpty()) {
    mJobId = result.jobId;
    mLogger->debug(tr("Job ID: %1").arg(mJobId));
  }

  if (result.status == "finished") {
    mLogger->info(tr("Job succeeded!"));
    emit statusNotification(Status{
        State::Succeeded,
        100,
        result.ses,
        QString(),
    });
  } else {
    mLogger->debug(tr("Progress: %1%%").arg(result.progress));
    mPollTimer->start(result.interval * 1000);
  }
}

void OnlineAutorouter::handleFailure(const ApiEndpoint::Exception& e) noexcept {
  mPollRequest.cancel();
  mPollRequest = {};
  mJobId.clear();

  const QString msg = tr("API call failed: %1").arg(e.getMsg());
  mLogger->critical(msg);
  emit statusNotification(Status{
      State::Failed,
      100,
      QByteArray(),
      msg,
  });
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
