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

#ifndef LIBREPCB_LIBRARY_MSGWRONGFOOTPRINTTEXTLAYER_H
#define LIBREPCB_LIBRARY_MSGWRONGFOOTPRINTTEXTLAYER_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../msg/libraryelementcheckmessage.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class StrokeText;

namespace library {

class Footprint;

/*******************************************************************************
 *  Class MsgWrongFootprintTextLayer
 ******************************************************************************/

/**
 * @brief The MsgWrongFootprintTextLayer class
 */
class MsgWrongFootprintTextLayer final : public LibraryElementCheckMessage {
  Q_DECLARE_TR_FUNCTIONS(MsgWrongFootprintTextLayer)

public:
  // Constructors / Destructor
  MsgWrongFootprintTextLayer() = delete;
  MsgWrongFootprintTextLayer(std::shared_ptr<const Footprint>  footprint,
                             std::shared_ptr<const StrokeText> text,
                             const QString& expectedLayerName) noexcept;
  MsgWrongFootprintTextLayer(const MsgWrongFootprintTextLayer& other) noexcept
    : LibraryElementCheckMessage(other),
      mFootprint(other.mFootprint),
      mText(other.mText),
      mExpectedLayerName(other.mExpectedLayerName) {}
  virtual ~MsgWrongFootprintTextLayer() noexcept;

  // Getters
  std::shared_ptr<const Footprint> getFootprint() const noexcept {
    return mFootprint;
  }
  std::shared_ptr<const StrokeText> getText() const noexcept { return mText; }
  QString getExpectedLayerName() const noexcept { return mExpectedLayerName; }

private:
  std::shared_ptr<const Footprint>  mFootprint;
  std::shared_ptr<const StrokeText> mText;
  QString                           mExpectedLayerName;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace library
}  // namespace librepcb

#endif  // LIBREPCB_LIBRARY_MSGWRONGFOOTPRINTTEXTLAYER_H
