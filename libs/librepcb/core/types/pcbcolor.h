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

#ifndef LIBREPCB_CORE_PCBCOLOR_H
#define LIBREPCB_CORE_PCBCOLOR_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <QtCore>
#include <QtGui>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class PcbColor
 ******************************************************************************/

/**
 * @brief Predefined colors relevant for PCB fabrication
 */
class PcbColor final {
  Q_DECLARE_TR_FUNCTIONS(PcbColor)

public:
  // Types
  enum class Flag : quint32 {
    SolderResist = (1 << 0),  ///< Color available for solder resist
    Silkscreen = (1 << 1),  ///< Color available for silkscreen
  };
  Q_DECLARE_FLAGS(Flags, Flag)

  // Constructors / Destructor
  PcbColor() = delete;
  PcbColor(const PcbColor& other) noexcept;
  ~PcbColor() noexcept;

  // Getters

  /**
   * @brief Get the identifier used for serialization
   *
   * @return Identifier string (lower_snake_case)
   */
  const QString& getId() const noexcept { return mId; }

  /**
   * @brief Get the name of the color (human readable and translated)
   *
   * @return The name of the color
   */
  const QString& getNameTr() const noexcept { return mNameTr; }

  /**
   * @brief Get the actual color for solder resist rendering
   *
   * @return A QColor object representing this color, or a default color
   *         if this color does not represent a valid solder resist color
   */
  const QColor& toSolderResistColor() const noexcept;

  /**
   * @brief Get the actual color for silkscreen rendering
   *
   * @return A QColor object representing this color, or a default color
   *         if this color does not represent a valid silkscreen color
   */
  const QColor& toSilkscreenColor() const noexcept;

  /**
   * @brief Check if this color is available for solder resist
   *
   * @return Whether this color is available or not
   */
  bool isAvailableForSolderResist() const noexcept {
    return mFlags.testFlag(Flag::SolderResist);
  }

  /**
   * @brief Check if this color is available for silkscreen
   *
   * @return Whether this color is available or not
   */
  bool isAvailableForSilkscreen() const noexcept {
    return mFlags.testFlag(Flag::Silkscreen);
  }

  // Operator Overloadings
  PcbColor& operator=(const PcbColor& rhs) noexcept = delete;
  bool operator==(const PcbColor& rhs) const noexcept { return this == &rhs; }
  bool operator!=(const PcbColor& rhs) const noexcept { return this != &rhs; }

  // Static Methods
  static const PcbColor& black() noexcept;
  static const PcbColor& blackMatte() noexcept;
  static const PcbColor& blue() noexcept;
  static const PcbColor& clear() noexcept;
  static const PcbColor& green() noexcept;
  static const PcbColor& greenMatte() noexcept;
  static const PcbColor& purple() noexcept;
  static const PcbColor& red() noexcept;
  static const PcbColor& white() noexcept;
  static const PcbColor& yellow() noexcept;
  static const PcbColor& other() noexcept;

  /**
   * @brief Get a list of all available colors
   *
   * @return A list of all colors
   */
  static const QVector<const PcbColor*>& all() noexcept;

  /**
   * @brief Get a color by its identifier
   *
   * @param id  Color identifier.
   *
   * @return PcbColor object.
   *
   * @throw Exception if the color was not found.
   */
  static const PcbColor& get(const QString& id);

private:  // Methods
  PcbColor(const QString& id, const QString& nameTr, Flags flags,
           const QColor& solderResistColor,
           const QColor& silkscreenColor) noexcept;

private:  // Data
  const QString mId;
  const QString mNameTr;
  const Flags mFlags;
  const QColor mSolderResistColor;
  const QColor mSilkscreenColor;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

Q_DECLARE_OPERATORS_FOR_FLAGS(librepcb::PcbColor::Flags)
Q_DECLARE_METATYPE(const librepcb::PcbColor*)

#endif
