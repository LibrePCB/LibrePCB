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

#ifndef LIBREPCB_COMMON_TRACE_H
#define LIBREPCB_COMMON_TRACE_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../fileio/serializableobjectlist.h"
#include "../graphics/graphicslayername.h"
#include "../units/length.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class TraceAnchor
 ******************************************************************************/

/**
 * @brief The TraceAnchor class
 */
class TraceAnchor final : public SerializableObject {
  Q_DECLARE_TR_FUNCTIONS(TraceAnchor)

public:
  // Types
  struct PadAnchor {
    Uuid device;
    Uuid pad;

    bool operator==(const PadAnchor& rhs) const noexcept {
      return (device == rhs.device) && (pad == rhs.pad);
    }
  };

  // Constructors / Destructor
  TraceAnchor() = delete;
  TraceAnchor(const TraceAnchor& other) noexcept;
  TraceAnchor(const SExpression& node, const Version& fileFormat);
  ~TraceAnchor() noexcept;

  // Getters
  const tl::optional<Uuid>& tryGetJunction() const noexcept {
    return mJunction;
  }
  const tl::optional<Uuid>& tryGetVia() const noexcept { return mVia; }
  const tl::optional<PadAnchor>& tryGetPad() const noexcept { return mPad; }

  /// @copydoc librepcb::SerializableObject::serialize()
  void serialize(SExpression& root) const override;

  // Operator Overloadings
  bool operator==(const TraceAnchor& rhs) const noexcept;
  bool operator!=(const TraceAnchor& rhs) const noexcept {
    return !(*this == rhs);
  }
  TraceAnchor& operator=(const TraceAnchor& rhs) noexcept;

  // Static Methods
  static TraceAnchor junction(const Uuid& junction) noexcept;
  static TraceAnchor via(const Uuid& via) noexcept;
  static TraceAnchor pad(const Uuid& device, const Uuid& pad) noexcept;

private:  // Methods
  TraceAnchor(const tl::optional<Uuid>& junction, const tl::optional<Uuid>& via,
              const tl::optional<PadAnchor>& pad) noexcept;

private:  // Data
  tl::optional<Uuid> mJunction;
  tl::optional<Uuid> mVia;
  tl::optional<PadAnchor> mPad;
};

/*******************************************************************************
 *  Class Trace
 ******************************************************************************/

/**
 * @brief The Trace class represents a trace within a board
 *
 * The main purpose of this class is to serialize and deserialize traces.
 */
class Trace final : public SerializableObject {
  Q_DECLARE_TR_FUNCTIONS(Trace)

public:
  // Signals
  enum class Event {
    UuidChanged,
    LayerChanged,
    WidthChanged,
    StartPointChanged,
    EndPointChanged,
  };
  Signal<Trace, Event> onEdited;
  typedef Slot<Trace, Event> OnEditedSlot;

  // Constructors / Destructor
  Trace() = delete;
  Trace(const Trace& other) noexcept;
  Trace(const Uuid& uuid, const Trace& other) noexcept;
  Trace(const Uuid& uuid, const GraphicsLayerName& layer,
        const PositiveLength& width, const TraceAnchor& start,
        const TraceAnchor& end) noexcept;
  Trace(const SExpression& node, const Version& fileFormat);
  ~Trace() noexcept;

  // Getters
  const Uuid& getUuid() const noexcept { return mUuid; }
  const GraphicsLayerName& getLayer() const noexcept { return mLayer; }
  const PositiveLength& getWidth() const noexcept { return mWidth; }
  const TraceAnchor& getStartPoint() const noexcept { return mStart; }
  const TraceAnchor& getEndPoint() const noexcept { return mEnd; }

  // Setters
  bool setUuid(const Uuid& uuid) noexcept;
  bool setLayer(const GraphicsLayerName& layer) noexcept;
  bool setWidth(const PositiveLength& width) noexcept;
  bool setStartPoint(const TraceAnchor& start) noexcept;
  bool setEndPoint(const TraceAnchor& end) noexcept;

  /// @copydoc librepcb::SerializableObject::serialize()
  void serialize(SExpression& root) const override;

  // Operator Overloadings
  bool operator==(const Trace& rhs) const noexcept;
  bool operator!=(const Trace& rhs) const noexcept { return !(*this == rhs); }
  Trace& operator=(const Trace& rhs) noexcept;

private:  // Data
  Uuid mUuid;
  GraphicsLayerName mLayer;
  PositiveLength mWidth;
  TraceAnchor mStart;
  TraceAnchor mEnd;
};

/*******************************************************************************
 *  Class TraceList
 ******************************************************************************/

struct TraceListNameProvider {
  static constexpr const char* tagname = "trace";
};
using TraceList =
    SerializableObjectList<Trace, TraceListNameProvider, Trace::Event>;

/*******************************************************************************
 *  Non-Member Functions
 ******************************************************************************/

inline uint qHash(const TraceAnchor& key, uint seed) noexcept {
  QString s;
  if (tl::optional<Uuid> anchor = key.tryGetJunction()) {
    s += anchor->toStr();
  }
  if (tl::optional<Uuid> anchor = key.tryGetVia()) {
    s += anchor->toStr();
  }
  if (tl::optional<TraceAnchor::PadAnchor> anchor = key.tryGetPad()) {
    s += anchor->device.toStr();
    s += anchor->pad.toStr();
  }
  Q_ASSERT(!s.isEmpty());

  return ::qHash(s, seed);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
