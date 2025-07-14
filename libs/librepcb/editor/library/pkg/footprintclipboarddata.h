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

#ifndef LIBREPCB_EDITOR_FOOTPRINTCLIPBOARDDATA_H
#define LIBREPCB_EDITOR_FOOTPRINTCLIPBOARDDATA_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/core/geometry/circle.h>
#include <librepcb/core/geometry/hole.h>
#include <librepcb/core/geometry/polygon.h>
#include <librepcb/core/geometry/stroketext.h>
#include <librepcb/core/geometry/zone.h>
#include <librepcb/core/library/pkg/footprintpad.h>
#include <librepcb/core/library/pkg/packagepad.h>

#include <QtCore>
#include <QtWidgets>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Class FootprintClipboardData
 ******************************************************************************/

/**
 * @brief The FootprintClipboardData class
 */
class FootprintClipboardData final {
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
        mStrokeTexts.count() + mZones.count() + mHoles.count();
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
  ZoneList& getZones() noexcept { return mZones; }
  const ZoneList& getZones() const noexcept { return mZones; }
  HoleList& getHoles() noexcept { return mHoles; }
  const HoleList& getHoles() const noexcept { return mHoles; }

  // General Methods
  std::unique_ptr<QMimeData> toMimeData();
  static std::unique_ptr<FootprintClipboardData> fromMimeData(
      const QMimeData* mime);
  static bool isValid(const QMimeData* mime) noexcept;

  // Operator Overloadings
  FootprintClipboardData& operator=(const FootprintClipboardData& rhs) = delete;

private:  // Methods
  QPixmap generatePixmap() noexcept;
  static QString getMimeType() noexcept;

private:  // Data
  Uuid mFootprintUuid;
  PackagePadList mPackagePads;
  Point mCursorPos;
  FootprintPadList mFootprintPads;
  PolygonList mPolygons;
  CircleList mCircles;
  StrokeTextList mStrokeTexts;
  ZoneList mZones;
  HoleList mHoles;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
