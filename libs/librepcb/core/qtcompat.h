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

#ifndef LIBREPCB_CORE_QTCOMPAT_H
#define LIBREPCB_CORE_QTCOMPAT_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class QtCompat
 ******************************************************************************/

/**
 * @brief Qt compatibility helper class
 *
 * This class provides some helpers to simplify keeping compatibility with
 * various Qt versions without getting deprecation warnings.
 */
class QtCompat final {
public:
  // Disable Instantiation
  QtCompat() = delete;
  QtCompat(const QtCompat& other) = delete;
  ~QtCompat() = delete;
  QtCompat& operator=(const QtCompat& rhs) = delete;

  //@{
  /**
   * @brief Return type of Qt's qHash() function
   */
#if QT_VERSION_MAJOR >= 6
  using Hash = std::size_t;
#else
  using Hash = uint;
#endif
  //@}

  //@{
  /**
   * @brief String split behavior
   *
   * Used for QString::split() which changed its function signature in Qt 5.15.
   */
#if (QT_VERSION >= QT_VERSION_CHECK(5, 15, 0))
  static inline constexpr Qt::SplitBehavior keepEmptyParts() noexcept {
    return Qt::KeepEmptyParts;
  }
  static inline constexpr Qt::SplitBehavior skipEmptyParts() noexcept {
    return Qt::SkipEmptyParts;
  }
#else
  static inline constexpr QString::SplitBehavior keepEmptyParts() noexcept {
    return QString::KeepEmptyParts;
  }
  static inline constexpr QString::SplitBehavior skipEmptyParts() noexcept {
    return QString::SkipEmptyParts;
  }
#endif
  //@}

  //@{
/**
 * @brief Wrapper for QString::midRef()
 */
#if QT_VERSION_MAJOR >= 6
  static inline QStringView midRef(const QString& s, int pos,
                                   int n = -1) noexcept {
    // Note: QStringView exists since Qt 5.10, but the behavior of mid() was
    // different!
    return QStringView(s).mid(pos, n);
  }
#else
  static inline QStringRef midRef(const QString& s, int pos,
                                  int n = -1) noexcept {
    return s.midRef(pos, n);
  }
#endif
  //@}
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
