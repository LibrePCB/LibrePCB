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

#ifndef LIBREPCB_CORE_BOARDDESIGNRULECHECKMESSAGE_H
#define LIBREPCB_CORE_BOARDDESIGNRULECHECKMESSAGE_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../../geometry/path.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class BoardDesignRuleCheckMessage
 ******************************************************************************/

/**
 * @brief The BoardDesignRuleCheckMessage class represents a message produced
 *        by the design rule check (DRC)
 */
class BoardDesignRuleCheckMessage final {
public:
  // Constructors / Destructor
  BoardDesignRuleCheckMessage(const QString& message,
                              const Path& location) noexcept;
  BoardDesignRuleCheckMessage(const QString& message,
                              const QVector<Path>& locations) noexcept;
  ~BoardDesignRuleCheckMessage() noexcept;

  // Getters
  const QString& getMessage() const noexcept { return mMessage; }
  const QVector<Path>& getLocations() const noexcept { return mLocations; }

private:  // Data
  QString mMessage;
  QVector<Path> mLocations;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
