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

#ifndef LIBREPCB_PROJECT_BOARDUSERSETTINGS_H
#define LIBREPCB_PROJECT_BOARDUSERSETTINGS_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/common/fileio/serializableobject.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class GraphicsLayerStackAppearanceSettings;

namespace project {

class Board;

/*******************************************************************************
 *  Class BoardUserSettings
 ******************************************************************************/

/**
 * @brief The BoardUserSettings class
 */
class BoardUserSettings final : public QObject, public SerializableObject {
  Q_OBJECT

public:
  // Constructors / Destructor
  BoardUserSettings() = delete;
  BoardUserSettings(const BoardUserSettings& other) = delete;
  explicit BoardUserSettings(Board& board) noexcept;
  BoardUserSettings(Board& board, const BoardUserSettings& other) noexcept;
  BoardUserSettings(Board& board, const SExpression& node,
                    const Version& fileFormat);
  ~BoardUserSettings() noexcept;

  // General Methods

  /// @copydoc librepcb::SerializableObject::serialize()
  void serialize(SExpression& root) const override;

  // Operator Overloadings
  BoardUserSettings& operator=(const BoardUserSettings& rhs) = delete;

private:  // Methods
  // General
  Board& mBoard;
  QScopedPointer<GraphicsLayerStackAppearanceSettings> mLayerSettings;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace project
}  // namespace librepcb

#endif  // LIBREPCB_PROJECT_BOARDUSERSETTINGS_H
