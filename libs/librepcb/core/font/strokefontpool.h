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

#ifndef LIBREPCB_CORE_STROKEFONTPOOL_H
#define LIBREPCB_CORE_STROKEFONTPOOL_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "strokefont.h"

#include <QtCore>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class FileSystem;

/*******************************************************************************
 *  Class StrokeFontPool
 ******************************************************************************/

/**
 * @brief The StrokeFontPool class
 */
class StrokeFontPool final {
  Q_DECLARE_TR_FUNCTIONS(StrokeFontPool)

public:
  // Constructors / Destructor
  explicit StrokeFontPool(const FileSystem& directory) noexcept;
  StrokeFontPool(const StrokeFontPool& other) = delete;
  ~StrokeFontPool() noexcept;

  // Getters
  const StrokeFont& getFont(const QString& filename) const;

  // Operator Overloadings
  StrokeFontPool& operator=(const StrokeFontPool& rhs) noexcept;

private:  // Data
  QHash<QString, std::shared_ptr<StrokeFont>> mFonts;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
