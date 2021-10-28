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
#include "networkrequest.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

NetworkRequest::NetworkRequest(const QUrl& url,
                               const QByteArray& postData) noexcept
  : NetworkRequestBase(url, postData), mReceivedData() {
}

NetworkRequest::~NetworkRequest() noexcept {
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void NetworkRequest::prepareRequest() {
  mReceivedData.clear();
}

void NetworkRequest::finalizeRequest() {
  if (mReceivedData.size() > 100 * 1000 * 1000) {
    throw RuntimeError(
        __FILE__, __LINE__,
        tr("The received content exceeds the 100MB size limit."));
  }
}

void NetworkRequest::emitSuccessfullyFinishedSignals() noexcept {
  emit dataReceived(mReceivedData);
}

void NetworkRequest::fetchNewData() noexcept {
  QByteArray data = mReply->readAll();
  if (mReceivedData.size() <= 100 * 1000 * 1000) {
    mReceivedData.append(data);
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
