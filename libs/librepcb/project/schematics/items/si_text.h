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

#ifndef LIBREPCB_PROJECT_SI_TEXT_H
#define LIBREPCB_PROJECT_SI_TEXT_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "si_base.h"

#include <librepcb/common/geometry/text.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class TextGraphicsItem;

namespace project {

class Schematic;

/*******************************************************************************
 *  Class SI_Text
 ******************************************************************************/

/**
 * @brief The SI_Text class represents a text label in a schematic
 */
class SI_Text final : public SI_Base, public SerializableObject {
  Q_OBJECT

public:
  // Constructors / Destructor
  SI_Text() = delete;
  SI_Text(const SI_Text& other) = delete;
  SI_Text(Schematic& schematic, const SExpression& node,
          const Version& fileFormat);
  SI_Text(Schematic& schematic, const Text& text);
  ~SI_Text() noexcept;

  // Getters
  const Uuid& getUuid() const noexcept { return mText.getUuid(); }
  const Angle& getRotation() const noexcept { return mText.getRotation(); }
  Text& getText() noexcept { return mText; }
  const Text& getText() const noexcept { return mText; }

  // General Methods
  void addToSchematic() override;
  void removeFromSchematic() override;

  /// @copydoc librepcb::SerializableObject::serialize()
  void serialize(SExpression& root) const override;

  // Inherited from SI_Base
  Type_t getType() const noexcept override { return SI_Base::Type_t::Text; }
  const Point& getPosition() const noexcept override {
    return mText.getPosition();
  }
  QPainterPath getGrabAreaScenePx() const noexcept override;
  void setSelected(bool selected) noexcept override;

  // Operator Overloadings
  SI_Text& operator=(const SI_Text& rhs) = delete;

private:  // Methods
  void init();
  void schematicAttributesChanged() noexcept;

private:  // Attributes
  Text mText;
  QScopedPointer<TextGraphicsItem> mGraphicsItem;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace project
}  // namespace librepcb

#endif
