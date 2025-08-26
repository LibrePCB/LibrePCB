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

#ifndef LIBREPCB_CORE_TRACE_H
#define LIBREPCB_CORE_TRACE_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../serialization/serializableobjectlist.h"
#include "../types/length.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Layer;

/*******************************************************************************
 *  Class TraceAnchor
 ******************************************************************************/

/**
 * @brief The TraceAnchor class
 */
class TraceAnchor final {
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
  explicit TraceAnchor(const SExpression& node);
  ~TraceAnchor() noexcept;

  // Getters
  const std::optional<Uuid>& tryGetJunction() const noexcept {
    return mJunction;
  }
  const std::optional<Uuid>& tryGetVia() const noexcept { return mVia; }
  const std::optional<PadAnchor>& tryGetPad() const noexcept { return mPad; }

  // General Methods

  /**
   * @brief Serialize into ::librepcb::SExpression node
   *
   * @param root    Root node to serialize into.
   */
  void serialize(SExpression& root) const;

  // Operator Overloadings
  bool operator==(const TraceAnchor& rhs) const noexcept;
  bool operator!=(const TraceAnchor& rhs) const noexcept {
    return !(*this == rhs);
  }
  bool operator<(const TraceAnchor& rhs) const noexcept;
  TraceAnchor& operator=(const TraceAnchor& rhs) noexcept;

  // Static Methods
  static TraceAnchor junction(const Uuid& junction) noexcept;
  static TraceAnchor via(const Uuid& via) noexcept;
  static TraceAnchor pad(const Uuid& device, const Uuid& pad) noexcept;

private:  // Methods
  TraceAnchor(const std::optional<Uuid>& junction,
              const std::optional<Uuid>& via,
              const std::optional<PadAnchor>& pad) noexcept;

private:  // Data
  std::optional<Uuid> mJunction;
  std::optional<Uuid> mVia;
  std::optional<PadAnchor> mPad;
};

/*******************************************************************************
 *  Class Trace
 ******************************************************************************/

/**
 * @brief The Trace class represents a trace within a board
 *
 * The main purpose of this class is to serialize and deserialize traces.
 *
 * @note The order of anchors (P1 & P2) is deterministic (sorted) to ensure a
 *       canonical file format & behavior. The constructor and #setAnchors()
 *       will automatically swap the passed anchors if needed.
 */
class Trace final {
  Q_DECLARE_TR_FUNCTIONS(Trace)

public:
  // Signals
  enum class Event {
    UuidChanged,
    LayerChanged,
    WidthChanged,
    AnchorsChanged,
  };
  Signal<Trace, Event> onEdited;
  typedef Slot<Trace, Event> OnEditedSlot;

  // Constructors / Destructor
  Trace() = delete;
  Trace(const Trace& other) noexcept;
  Trace(const Uuid& uuid, const Trace& other) noexcept;
  Trace(const Uuid& uuid, const Layer& layer, const PositiveLength& width,
        const TraceAnchor& a, const TraceAnchor& b) noexcept;
  explicit Trace(const SExpression& node);
  ~Trace() noexcept;

  // Getters
  const Uuid& getUuid() const noexcept { return mUuid; }
  const Layer& getLayer() const noexcept { return *mLayer; }
  const PositiveLength& getWidth() const noexcept { return mWidth; }
  const TraceAnchor& getP1() const noexcept { return mP1; }
  const TraceAnchor& getP2() const noexcept { return mP2; }

  // Setters
  bool setUuid(const Uuid& uuid) noexcept;
  bool setLayer(const Layer& layer) noexcept;
  bool setWidth(const PositiveLength& width) noexcept;
  bool setAnchors(TraceAnchor a, TraceAnchor b) noexcept;

  // General Methods

  /**
   * @brief Serialize into ::librepcb::SExpression node
   *
   * @param root    Root node to serialize into.
   */
  void serialize(SExpression& root) const;

  // Operator Overloadings
  bool operator==(const Trace& rhs) const noexcept;
  bool operator!=(const Trace& rhs) const noexcept { return !(*this == rhs); }
  Trace& operator=(const Trace& rhs) noexcept;

private:  // Methods
  static void normalizeAnchors(TraceAnchor& start, TraceAnchor& end) noexcept;

private:  // Data
  Uuid mUuid;
  const Layer* mLayer;
  PositiveLength mWidth;
  TraceAnchor mP1;
  TraceAnchor mP2;
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

inline std::size_t qHash(const TraceAnchor& key,
                         std::size_t seed = 0) noexcept {
  QString s;
  if (std::optional<Uuid> anchor = key.tryGetJunction()) {
    s += anchor->toStr();
  }
  if (std::optional<Uuid> anchor = key.tryGetVia()) {
    s += anchor->toStr();
  }
  if (std::optional<TraceAnchor::PadAnchor> anchor = key.tryGetPad()) {
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
