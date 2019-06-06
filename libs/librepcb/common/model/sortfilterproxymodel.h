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

#ifndef LIBREPCB_LIBRARY_SORTFILTERPROXYMODEL_H
#define LIBREPCB_LIBRARY_SORTFILTERPROXYMODEL_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class SortFilterProxyModel
 ******************************************************************************/

/**
 * @brief A QSortFilterProxyModel subclass to implement custom sort behavior
 *
 * Features:
 *   - Use natural sort algorithm (e.g. to sort "X9" in front of "X10").
 *   - Optionally don't sort the last row (needed if the last row is used to
 *     add more items instead of displaying existing items).
 */
class SortFilterProxyModel : public QSortFilterProxyModel {
  Q_OBJECT

public:
  // Constructors / Destructor
  SortFilterProxyModel() = delete;
  SortFilterProxyModel(const SortFilterProxyModel& other) noexcept;
  explicit SortFilterProxyModel(QObject* parent = nullptr) noexcept;
  virtual ~SortFilterProxyModel() noexcept;

  // Setters
  void setKeepLastRowAtBottom(bool keep) noexcept {
    mKeepLastRowAtBottom = keep;
  }

  // Operator Overloadings
  SortFilterProxyModel& operator=(const SortFilterProxyModel& rhs) noexcept;

protected:
  virtual bool lessThan(const QModelIndex& source_left,
                        const QModelIndex& source_right) const override;

private:
  QCollator mCollator;
  bool      mKeepLastRowAtBottom;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif  // LIBREPCB_LIBRARY_SORTFILTERPROXYMODEL_H
