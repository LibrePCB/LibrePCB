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

#ifndef LIBREPCB_EDITOR_BACKGROUNDIMAGEGRAPHICSITEM_H
#define LIBREPCB_EDITOR_BACKGROUNDIMAGEGRAPHICSITEM_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/core/types/angle.h>
#include <librepcb/core/types/point.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Class BackgroundImageSettings
 ******************************************************************************/

struct BackgroundImageSettings {
  bool enabled = true;  ///< Whether the background is enabled or not
  QImage image;  ///< The original loaded image
  QPointF referencePos;  ///< Reference in #image, from top left [pixels]
  std::pair<qreal, qreal> dpi = {720, 720};  ///< Scale X/Y [dpi]
  Point position;  ///< Destination scene position of #referencePos
  Angle rotation;  ///< Rotation in scene

  bool tryLoadFromDir(const FilePath& dir) noexcept;
  void saveToDir(const FilePath& dir) noexcept;

  bool operator==(const BackgroundImageSettings& rhs) const noexcept;
  bool operator!=(const BackgroundImageSettings& rhs) const noexcept;
};

/*******************************************************************************
 *  Class BackgroundImageGraphicsItem
 ******************************************************************************/

/**
 * @brief The BackgroundImageGraphicsItem class
 */
class BackgroundImageGraphicsItem final : public QGraphicsItem {
public:
  // Constructors / Destructor
  BackgroundImageGraphicsItem(const BackgroundImageGraphicsItem& other) =
      delete;
  BackgroundImageGraphicsItem() noexcept;
  ~BackgroundImageGraphicsItem() noexcept;

  // General Methods
  void setBackgroundColor(const QColor& c) noexcept { mBackgroundColor = c; }
  void setSettings(const BackgroundImageSettings& settings) noexcept;
  const BackgroundImageSettings& getSettings() const noexcept {
    return mSettings;
  }

  // Inherited from QGraphicsItem
  QRectF boundingRect() const noexcept override;
  QPainterPath shape() const noexcept override;
  void paint(QPainter* painter, const QStyleOptionGraphicsItem* option,
             QWidget* widget = 0) noexcept override;

  // Operator Overloadings
  BackgroundImageGraphicsItem& operator=(
      const BackgroundImageGraphicsItem& rhs) = delete;

private:
  QColor mBackgroundColor;
  BackgroundImageSettings mSettings;
  QImage mImage;
};

}  // namespace editor
}  // namespace librepcb

/*******************************************************************************
 *  End of File
 ******************************************************************************/

#endif
