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

#ifndef LIBREPCB_CORE_ONLINEAUTOROUTER_H
#define LIBREPCB_CORE_ONLINEAUTOROUTER_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../network/apiendpoint.h"
#include "autorouter.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class OnlineAutorouter
 ******************************************************************************/

/**
 * @brief Online autorouter running through ::librepcb::ApiEndpoint
 */
class OnlineAutorouter : public Autorouter {
  Q_OBJECT

public:
  // Constructors / Destructor
  OnlineAutorouter() noexcept = delete;
  OnlineAutorouter(const std::shared_ptr<ApiEndpoint>& ep,
                   const QString& routerId,
                   const std::shared_ptr<MessageLogger>& logger) noexcept;
  OnlineAutorouter(const OnlineAutorouter& other) = delete;
  ~OnlineAutorouter() noexcept override;

  // General Methods
  void start(const QByteArray& dsn) noexcept override;
  void cancel() noexcept override;

  // Operator Overloadings
  OnlineAutorouter& operator=(const OnlineAutorouter& rhs) = delete;

private:
  void handleStatus(const ApiEndpoint::AutorouteJobResult& result) noexcept;
  void handleFailure(const ApiEndpoint::Exception& e) noexcept;

  const std::shared_ptr<ApiEndpoint> mEndpoint;
  const QString mRouterId;
  QFuture<ApiEndpoint::AutorouteJobResult> mPollRequest;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
