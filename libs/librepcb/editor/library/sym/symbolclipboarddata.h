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

#ifndef LIBREPCB_EDITOR_SYMBOLCLIPBOARDDATA_H
#define LIBREPCB_EDITOR_SYMBOLCLIPBOARDDATA_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/

#include <librepcb/core/geometry/circle.h>
#include <librepcb/core/geometry/polygon.h>
#include <librepcb/core/geometry/text.h>
#include <librepcb/core/library/sym/symbolpin.h>

#include <QtCore>
#include <QtWidgets>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

class GraphicsLayerList;

/*******************************************************************************
 *  Class SymbolClipboardData
 ******************************************************************************/

/**
 * @brief The SymbolClipboardData class
 */
class SymbolClipboardData final {
public:
  // Constructors / Destructor
  SymbolClipboardData() = delete;
  SymbolClipboardData(const SymbolClipboardData& other) = delete;
  SymbolClipboardData(const Uuid& symbolUuid, const Point& cursorPos) noexcept;
  explicit SymbolClipboardData(const SExpression& node);
  ~SymbolClipboardData() noexcept;

  // Getters
  bool getItemCount() const noexcept {
    return mPins.count() + mPolygons.count() + mCircles.count() +
        mTexts.count();
  }
  const Uuid& getSymbolUuid() const noexcept { return mSymbolUuid; }
  const Point& getCursorPos() const noexcept { return mCursorPos; }
  SymbolPinList& getPins() noexcept { return mPins; }
  const SymbolPinList& getPins() const noexcept { return mPins; }
  PolygonList& getPolygons() noexcept { return mPolygons; }
  const PolygonList& getPolygons() const noexcept { return mPolygons; }
  CircleList& getCircles() noexcept { return mCircles; }
  const CircleList& getCircles() const noexcept { return mCircles; }
  TextList& getTexts() noexcept { return mTexts; }
  const TextList& getTexts() const noexcept { return mTexts; }

  // General Methods
  std::unique_ptr<QMimeData> toMimeData(const GraphicsLayerList& layers);
  static std::unique_ptr<SymbolClipboardData> fromMimeData(
      const QMimeData* mime);

  // Operator Overloadings
  SymbolClipboardData& operator=(const SymbolClipboardData& rhs) = delete;

private:  // Methods
  QPixmap generatePixmap(const GraphicsLayerList& layers) noexcept;
  static QString getMimeType() noexcept;

private:  // Data
  Uuid mSymbolUuid;
  Point mCursorPos;
  SymbolPinList mPins;
  PolygonList mPolygons;
  CircleList mCircles;
  TextList mTexts;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
