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

#ifndef LIBREPCB_CORE_LAYER_H
#define LIBREPCB_CORE_LAYER_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class Layer
 ******************************************************************************/

/**
 * @brief The Layer class provides all supported geometry layers
 */
class Layer final {
  Q_DECLARE_TR_FUNCTIONS(Layer)

public:
  // Types
  enum class Flag : quint32 {
    NumberMask = 0xFFU,  ///< Copper layer number (0 = top, 64 = bottom)..
    Schematic = (1 << 8),
    Board = (1 << 16),
    Top = (1 << 17),
    Inner = (1 << 18),
    Bottom = (1 << 19),
    Copper = (1 << 20),
    StopMask = (1 << 21),
    SolderPaste = (1 << 22),
  };
  Q_DECLARE_FLAGS(Flags, Flag)

  // Constructors / Destructor
  Layer() = delete;
  Layer(const Layer& other) noexcept;
  ~Layer() noexcept;

  // Getters

  /**
   * @brief Get the identifier used for serialization
   *
   * @return Identifier string (lower_snake_case)
   */
  const QString& getId() const noexcept { return mId; }

  /**
   * @brief Get the name of the Layer (human readable and translated)
   *
   * @return The name of the Layer
   */
  const QString& getNameTr() const noexcept { return mNameTr; }

  /**
   * @brief Get the name of the corresponding theme color
   *
   * @return The name of the corresponding theme color
   *
   * @see ::librepcb::Theme::Color
   */
  const QString& getThemeColor() const noexcept { return mThemeColor; }

  /**
   * @brief Check if this is a schematic layer
   *
   * @return Whether this is a schematic layer or not
   */
  bool isSchematic() const noexcept { return mFlags.testFlag(Flag::Schematic); }

  /**
   * @brief Check if this is a board layer
   *
   * @return Whether this is a board layer or not
   */
  bool isBoard() const noexcept { return mFlags.testFlag(Flag::Board); }

  /**
   * @brief Check if this is a top board layer
   *
   * @return Whether this is a top board layer or not
   */
  bool isTop() const noexcept { return mFlags.testFlag(Flag::Top); }

  /**
   * @brief Check if this is an inner board layer
   *
   * @return Whether this is an inner board layer or not
   */
  bool isInner() const noexcept { return mFlags.testFlag(Flag::Inner); }

  /**
   * @brief Check if this is a bottom board layer
   *
   * @return Whether this is a bottom board layer or not
   */
  bool isBottom() const noexcept { return mFlags.testFlag(Flag::Bottom); }

  /**
   * @brief Check if this is a copper layer
   *
   * @return Whether this is a copper layer or not
   */
  bool isCopper() const noexcept { return mFlags.testFlag(Flag::Copper); }

  /**
   * @brief Check if this is a stop mask layer
   *
   * @return Whether this is a stop mask layer or not
   */
  bool isStopMask() const noexcept { return mFlags.testFlag(Flag::StopMask); }

  /**
   * @brief Check if this is a solder paste layer
   *
   * @return Whether this is a solder paste layer or not
   */
  bool isSolderPaste() const noexcept {
    return mFlags.testFlag(Flag::SolderPaste);
  }

  /**
   * @brief Get the copper layer number
   *
   * @return Copper layer number (0 = top, 1 = first inner, 63 = bottom)
   */
  int getCopperNumber() const noexcept { return mFlags & Flag::NumberMask; }

  /**
   * @brief Mirror this layer to the other board side
   *
   * @param innerLayers   If specified, inner copper layers will be mirrored
   *                      within this layer count as well. Otherwise, inner
   *                      layers are not mirrored.
   *
   * @return Mirrored layer, or this layer if not mirrorable.
   */
  const Layer& mirrored(int innerLayers = -1) const noexcept;

  // Operator Overloadings
  Layer& operator=(const Layer& rhs) noexcept = delete;
  bool operator==(const Layer& rhs) const noexcept { return this == &rhs; }
  bool operator!=(const Layer& rhs) const noexcept { return this != &rhs; }

  // Static Methods
  static const Layer& schematicSheetFrames() noexcept;
  static const Layer& schematicDocumentation() noexcept;
  static const Layer& schematicComments() noexcept;
  static const Layer& schematicGuide() noexcept;
  static const Layer& symbolOutlines() noexcept;
  static const Layer& symbolHiddenGrabAreas() noexcept;
  static const Layer& symbolNames() noexcept;
  static const Layer& symbolValues() noexcept;
  static const Layer& symbolPinNames() noexcept;  // Used by Eagle import
  static const Layer& boardSheetFrames() noexcept;
  static const Layer& boardOutlines() noexcept;
  static const Layer& boardMillingPth() noexcept;
  static const Layer& boardMeasures() noexcept;
  static const Layer& boardAlignment() noexcept;
  static const Layer& boardDocumentation() noexcept;
  static const Layer& boardComments() noexcept;
  static const Layer& boardGuide() noexcept;
  static const Layer& topPlacement() noexcept;
  static const Layer& botPlacement() noexcept;
  static const Layer& topDocumentation() noexcept;
  static const Layer& botDocumentation() noexcept;
  static const Layer& topHiddenGrabAreas() noexcept;
  static const Layer& botHiddenGrabAreas() noexcept;
  static const Layer& topNames() noexcept;
  static const Layer& botNames() noexcept;
  static const Layer& topValues() noexcept;
  static const Layer& botValues() noexcept;
  static const Layer& topCourtyard() noexcept;
  static const Layer& botCourtyard() noexcept;
  static const Layer& topStopMask() noexcept;
  static const Layer& botStopMask() noexcept;
  static const Layer& topSolderPaste() noexcept;
  static const Layer& botSolderPaste() noexcept;
  static const Layer& topFinish() noexcept;
  static const Layer& botFinish() noexcept;
  static const Layer& topGlue() noexcept;
  static const Layer& botGlue() noexcept;
  static const Layer& topCopper() noexcept;
  static const Layer& botCopper() noexcept;
  static const QVector<const Layer*>& innerCopper() noexcept;
  static const Layer* innerCopper(int number) noexcept;
  static int innerCopperCount() noexcept;
  static const Layer* copper(int number) noexcept;

  /**
   * @brief Get a list of all available layers
   *
   * @return A hash table of all layers
   */
  static const QVector<const Layer*>& all() noexcept;

  /**
   * @brief Get a layer by its identifier
   *
   * @param id  Layer identifier.
   *
   * @return Layer object.
   *
   * @throw Exception if the layer was not found.
   */
  static const Layer& get(const QString& id);

  /**
   * @brief Comparison for sorting layers by function
   *
   * @param a   First layer to compare.
   * @param b   Second layer to compare.
   *
   * @return Whether `a` is considered as less than `b`.
   */
  static bool lessThan(const Layer* a, const Layer* b) noexcept;

private:  // Methods
  Layer(const QString& id, const QString& nameTr, const QString& themeColor,
        Flags flags) noexcept;

private:  // Data
  const QString mId;
  const QString mNameTr;
  const QString mThemeColor;
  const Flags mFlags;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

Q_DECLARE_OPERATORS_FOR_FLAGS(librepcb::Layer::Flags)

#endif
