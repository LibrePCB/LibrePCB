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

#ifndef LIBREPCB_ALIGNMENTSELECTOR_H
#define LIBREPCB_ALIGNMENTSELECTOR_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../alignment.h"

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

namespace Ui {
class AlignmentSelector;
}

/*******************************************************************************
 *  Class AlignmentSelector
 ******************************************************************************/

/**
 * @brief The AlignmentSelector class
 *
 * @author ubruhin
 * @date 2017-01-29
 */
class AlignmentSelector final : public QWidget {
  Q_OBJECT

public:
  // Constructors / Destructor
  explicit AlignmentSelector(QWidget* parent = nullptr) noexcept;
  AlignmentSelector(const AlignmentSelector& other) = delete;
  ~AlignmentSelector() noexcept;

  // General Methods
  Alignment getAlignment() const noexcept;
  void      setAlignment(const Alignment& align) noexcept;

  // Operator Overloadings
  AlignmentSelector& operator=(const AlignmentSelector& rhs) = delete;

private:  // Data
  QScopedPointer<Ui::AlignmentSelector> mUi;
  QMap<QRadioButton*, Alignment>        mLookupTable;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif  // LIBREPCB_ALIGNMENTSELECTOR_H
