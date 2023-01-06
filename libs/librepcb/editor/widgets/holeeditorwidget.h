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

#ifndef LIBREPCB_EDITOR_HOLEEDITORWIDGET_H
#define LIBREPCB_EDITOR_HOLEEDITORWIDGET_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/core/geometry/hole.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class LengthUnit;
class Path;

namespace editor {

namespace Ui {
class HoleEditorWidget;
}

/*******************************************************************************
 *  Class HoleEditorWidget
 ******************************************************************************/

/**
 * @brief The HoleEditorWidget class
 */
class HoleEditorWidget : public QWidget {
  Q_OBJECT

public:
  // Constructors / Destructor
  HoleEditorWidget() = delete;
  explicit HoleEditorWidget(QWidget* parent = nullptr) noexcept;
  HoleEditorWidget(const HoleEditorWidget& other) = delete;
  virtual ~HoleEditorWidget() noexcept;

  // Getters
  const Hole& getHole() const noexcept { return mHole; }

  // Setters
  void setReadOnly(bool readOnly) noexcept;
  void setHole(const Hole& hole) noexcept;

  // General Methods
  void setFocusToDiameterEdit() noexcept;
  void configureClientSettings(const LengthUnit& lengthUnit,
                               const QString& settingsPrefix) noexcept;

  // Operator Overloadings
  HoleEditorWidget& operator=(const HoleEditorWidget& rhs) = delete;

signals:
  void holeChanged(const Hole& hole);

private:  // Methods
  void updatePathFromCircularTab() noexcept;
  void updatePathFromLinearTab() noexcept;
  void updateCircularTabFromPath(const Path& path) noexcept;
  void updateLinearTabFromPath(const Path& path) noexcept;
  void updateLinearOuterSize(const Path& path) noexcept;

private:  // Data
  QScopedPointer<Ui::HoleEditorWidget> mUi;
  Hole mHole;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
