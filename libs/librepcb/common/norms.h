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

#ifndef LIBREPCB_NORMS_H
#define LIBREPCB_NORMS_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  List of norms
 ******************************************************************************/

/**
 * @brief Get a list of available "built-in" norms
 *
 * These norms are used e.g. in the library editor and workspace/project
 * settings dialogs.
 *
 * @return List of norms
 */
inline QStringList getAvailableNorms() noexcept {
  return QStringList{"IEC 60617", "IEEE 315"};
}

inline QIcon getNormIcon(const QString& norm) noexcept {
  return QIcon(
      QString(":/img/norm/%1.png").arg(norm.toLower().replace(" ", "_")));
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif  // LIBREPCB_NORMS_H
