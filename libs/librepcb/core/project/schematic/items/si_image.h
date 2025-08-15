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

#ifndef LIBREPCB_CORE_SI_IMAGE_H
#define LIBREPCB_CORE_SI_IMAGE_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../../geometry/image.h"
#include "si_symbol.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Schematic;

/*******************************************************************************
 *  Class SI_Image
 ******************************************************************************/

/**
 * @brief The SI_Image class represents an image in a schematic
 */
class SI_Image final : public SI_Base {
  Q_OBJECT

public:
  // Constructors / Destructor
  SI_Image() = delete;
  SI_Image(const SI_Image& other) = delete;
  SI_Image(Schematic& schematic, const Image& image);
  ~SI_Image() noexcept;

  // Getters
  const Uuid& getUuid() const noexcept { return mImage->getUuid(); }
  const Point& getPosition() const noexcept { return mImage->getPosition(); }
  const Angle& getRotation() const noexcept { return mImage->getRotation(); }
  const std::shared_ptr<Image>& getImage() noexcept { return mImage; }
  std::shared_ptr<const Image> getImage() const noexcept { return mImage; }

  // General Methods
  SI_Symbol* getSymbol() const noexcept { return mSymbol; }
  void setSymbol(SI_Symbol* symbol) noexcept;
  void addToSchematic() override;
  void removeFromSchematic() override;

  // Operator Overloadings
  SI_Image& operator=(const SI_Image& rhs) = delete;

private:
  QPointer<SI_Symbol> mSymbol;
  std::shared_ptr<Image> mImage;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
