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

#ifndef LIBREPCB_LIBRARY_EDITOR_FOOTPRINTCLIPBOARDDATA_H
#define LIBREPCB_LIBRARY_EDITOR_FOOTPRINTCLIPBOARDDATA_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/

#include <librepcb/common/fileio/serializableobject.h>
#include <librepcb/common/geometry/circle.h>
#include <librepcb/common/geometry/hole.h>
#include <librepcb/common/geometry/polygon.h>
#include <librepcb/common/geometry/stroketext.h>
#include <librepcb/library/pkg/footprintpad.h>
#include <librepcb/library/pkg/packagepad.h>

#include <QtCore>
#include <QtWidgets>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class IF_GraphicsLayerProvider;

namespace library {
namespace editor {

/*******************************************************************************
 *  Class FootprintClipboardData
 ******************************************************************************/

/**
 * @brief The FootprintClipboardData class
 */
class FootprintClipboardData final : public SerializableObject {
public:
  // Constructors / Destructor
  FootprintClipboardData() = delete;
  FootprintClipboardData(const FootprintClipboardData& other) = delete;
  FootprintClipboardData(const Uuid& footprintUuid,
                         const PackagePadList& packagePads,
                         const Point& cursorPos) noexcept;
  explicit FootprintClipboardData(const SExpression& node);
  ~FootprintClipboardData() noexcept;

  // Getters
  bool getItemCount() const noexcept {
    return mFootprintPads.count() + mPolygons.count() + mCircles.count() +
        mStrokeTexts.count() + mHoles.count();
  }
  const Uuid& getFootprintUuid() const noexcept { return mFootprintUuid; }
  const Point& getCursorPos() const noexcept { return mCursorPos; }
  PackagePadList& getPackagePads() noexcept { return mPackagePads; }
  const PackagePadList& getPackagePads() const noexcept { return mPackagePads; }
  FootprintPadList& getFootprintPads() noexcept { return mFootprintPads; }
  const FootprintPadList& getFootprintPads() const noexcept {
    return mFootprintPads;
  }
  PolygonList& getPolygons() noexcept { return mPolygons; }
  const PolygonList& getPolygons() const noexcept { return mPolygons; }
  CircleList& getCircles() noexcept { return mCircles; }
  const CircleList& getCircles() const noexcept { return mCircles; }
  StrokeTextList& getStrokeTexts() noexcept { return mStrokeTexts; }
  const StrokeTextList& getStrokeTexts() const noexcept { return mStrokeTexts; }
  HoleList& getHoles() noexcept { return mHoles; }
  const HoleList& getHoles() const noexcept { return mHoles; }

  // General Methods
  std::unique_ptr<QMimeData> toMimeData(const IF_GraphicsLayerProvider& lp);
  static std::unique_ptr<FootprintClipboardData> fromMimeData(
      const QMimeData* mime);

  // Operator Overloadings
  FootprintClipboardData& operator=(const FootprintClipboardData& rhs) = delete;

private:  // Methods
  /// @copydoc librepcb::SerializableObject::serialize()
  void serialize(SExpression& root) const override;

  QPixmap generatePixmap(const IF_GraphicsLayerProvider& lp) noexcept;
  static QString getMimeType() noexcept;

private:  // Data
  Uuid mFootprintUuid;
  PackagePadList mPackagePads;
  Point mCursorPos;
  FootprintPadList mFootprintPads;
  PolygonList mPolygons;
  CircleList mCircles;
  StrokeTextList mStrokeTexts;
  HoleList mHoles;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace library
}  // namespace librepcb

#endif  // LIBREPCB_LIBRARY_EDITOR_FOOTPRINTCLIPBOARDDATA_H
