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

#ifndef LIBREPCB_TEXT_H
#define LIBREPCB_TEXT_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../alignment.h"
#include "../fileio/cmd/cmdlistelementinsert.h"
#include "../fileio/cmd/cmdlistelementremove.h"
#include "../fileio/cmd/cmdlistelementsswap.h"
#include "../fileio/serializableobjectlist.h"
#include "../graphics/graphicslayername.h"
#include "../units/all_length_units.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class Text
 ******************************************************************************/

/**
 * @brief The Text class
 */
class Text final : public SerializableObject {
  Q_DECLARE_TR_FUNCTIONS(Text)

public:
  // Signals
  enum class Event {
    UuidChanged,
    LayerNameChanged,
    TextChanged,
    PositionChanged,
    RotationChanged,
    HeightChanged,
    AlignChanged,
  };
  Signal<Text, Event>       onEdited;
  typedef Slot<Text, Event> OnEditedSlot;

  // Constructors / Destructor
  Text() = delete;
  Text(const Text& other) noexcept;
  Text(const Uuid& uuid, const Text& other) noexcept;
  Text(const Uuid& uuid, const GraphicsLayerName& layerName,
       const QString& text, const Point& pos, const Angle& rotation,
       const PositiveLength& height, const Alignment& align) noexcept;
  explicit Text(const SExpression& node);
  ~Text() noexcept;

  // Getters
  const Uuid&              getUuid() const noexcept { return mUuid; }
  const GraphicsLayerName& getLayerName() const noexcept { return mLayerName; }
  const Point&             getPosition() const noexcept { return mPosition; }
  const Angle&             getRotation() const noexcept { return mRotation; }
  const PositiveLength&    getHeight() const noexcept { return mHeight; }
  const Alignment&         getAlign() const noexcept { return mAlign; }
  const QString&           getText() const noexcept { return mText; }

  // Setters
  bool setLayerName(const GraphicsLayerName& name) noexcept;
  bool setText(const QString& text) noexcept;
  bool setPosition(const Point& pos) noexcept;
  bool setRotation(const Angle& rotation) noexcept;
  bool setHeight(const PositiveLength& height) noexcept;
  bool setAlign(const Alignment& align) noexcept;

  /// @copydoc librepcb::SerializableObject::serialize()
  void serialize(SExpression& root) const override;

  // Operator Overloadings
  bool  operator==(const Text& rhs) const noexcept;
  bool  operator!=(const Text& rhs) const noexcept { return !(*this == rhs); }
  Text& operator=(const Text& rhs) noexcept;

private:  // Data
  Uuid              mUuid;
  GraphicsLayerName mLayerName;
  QString           mText;
  Point             mPosition;
  Angle             mRotation;
  PositiveLength    mHeight;
  Alignment         mAlign;
};

/*******************************************************************************
 *  Class TextList
 ******************************************************************************/

struct TextListNameProvider {
  static constexpr const char* tagname = "text";
};
using TextList =
    SerializableObjectList<Text, TextListNameProvider, Text::Event>;
using CmdTextInsert =
    CmdListElementInsert<Text, TextListNameProvider, Text::Event>;
using CmdTextRemove =
    CmdListElementRemove<Text, TextListNameProvider, Text::Event>;
using CmdTextsSwap =
    CmdListElementsSwap<Text, TextListNameProvider, Text::Event>;

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif  // LIBREPCB_TEXT_H
