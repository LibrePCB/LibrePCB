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

#ifndef LIBREPCB_COMMON_NETWORKREQUEST_H
#define LIBREPCB_COMMON_NETWORKREQUEST_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "networkrequestbase.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class NetworkRequest
 ******************************************************************************/

/**
 * @brief This class is used to process general purpose network requests (up to
 * 100MB)
 *
 * @see librepcb::NetworkRequestBase, librepcb::NetworkAccessManager
 */
class NetworkRequest final : public NetworkRequestBase {
  Q_OBJECT

public:
  // Constructors / Destructor
  NetworkRequest() = delete;
  NetworkRequest(const NetworkRequest& other) = delete;
  NetworkRequest(const QUrl& url,
                 const QByteArray& postData = QByteArray()) noexcept;
  ~NetworkRequest() noexcept;

  // Operator Overloadings
  NetworkRequest& operator=(const NetworkRequest& rhs) = delete;

signals:

  /**
   * @brief Data successfully received signal (emitted right before #finished())
   */
  void dataReceived(QByteArray data);

private:  // Methods
  void prepareRequest() override;
  void finalizeRequest() override;
  void emitSuccessfullyFinishedSignals() noexcept override;
  void fetchNewData() noexcept override;

private:  // Data
  QByteArray mReceivedData;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
