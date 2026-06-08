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
  : Autorouter(logger), mEndpoint(ep), mRouterId(routerId) {
  Q_ASSERT(mEndpoint);
}

OnlineAutorouter::~OnlineAutorouter() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void OnlineAutorouter::start(const QByteArray& dsn) noexcept {
  mPollRequest.cancel();
  mPollRequest = {};

  emit statusNotification(Status{
      State::Running,
      0,
      QByteArray(),
      QString(),
  });

  mLogger->info(
      tr("Sending DSN to '%1'...").arg(mEndpoint->getUrl().toString()));
  mPollRequest = mEndpoint->requestAutorouteStart(mRouterId, dsn);
  mPollRequest
      .then(this,
            std::bind(&OnlineAutorouter::handleStatus, this,
                      std::placeholders::_1))
      .onFailed(this,
                std::bind(&OnlineAutorouter::handleFailure, this,
                          std::placeholders::_1));
}

void OnlineAutorouter::cancel() noexcept {
  mPollRequest.cancel();
  mPollRequest = {};
  emit statusNotification(Status{
      State::Canceled,
      0,
      QByteArray(),
      QString(),
  });
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void OnlineAutorouter::handleStatus(
    const ApiEndpoint::AutorouteJobResult& result) noexcept {
  if (result.status == "finished") {
    qInfo() << "Finished:" << result.ses;
  } else {
    qInfo() << "Status:" << result.jobId << result.status << result.progress;
    QTimer::singleShot(
        result.interval * 1000, this, [this, jobId = result.jobId]() {
          mPollRequest = mEndpoint->requestAutorouteStatus(jobId);
          mPollRequest
              .then(this,
                    std::bind(&OnlineAutorouter::handleStatus, this,
                              std::placeholders::_1))
              .onFailed(this,
                        std::bind(&OnlineAutorouter::handleFailure, this,
                                  std::placeholders::_1));
        });
  }
}

void OnlineAutorouter::handleFailure(const ApiEndpoint::Exception& e) noexcept {
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
