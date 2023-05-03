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

#ifndef LIBREPCB_EDITOR_WAITINGSPINNERWIDGET_H
#define LIBREPCB_EDITOR_WAITINGSPINNERWIDGET_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Class WaitingSpinnerWidget
 ******************************************************************************/

/**
 * @brief A widget drawing a rotating spinner to indicate an ongoing operation
 *
 * Usage:
 *   - Pass the widget where the spinner shall be shown as the parent widget
 *     to the constructor. Important: Do NOT change the parent afterwards!
 *   - Call either `show()`, `hide()` or `setVisible()` to make the spinner
 *     visible or not.
 *   - Do not set a widget size or position manually, this is controlled by the
 *     widget itself.
 *
 * Memory management is done by the Qt parent-child mechanism.
 */
class WaitingSpinnerWidget final : public QWidget {
  Q_OBJECT

public:
  // Constructors / Destructor
  explicit WaitingSpinnerWidget(QWidget* parent = nullptr) noexcept;
  WaitingSpinnerWidget(const WaitingSpinnerWidget& other) = delete;
  ~WaitingSpinnerWidget() noexcept;

  // Setters
  void setColor(const QColor& color) noexcept { mColor = color; }

  // Operator Overloadings
  WaitingSpinnerWidget& operator=(const WaitingSpinnerWidget& rhs) = delete;

protected:  // Methods
  void showEvent(QShowEvent* e) noexcept override;
  void hideEvent(QHideEvent* e) noexcept override;
  void paintEvent(QPaintEvent* e) noexcept override;
  bool eventFilter(QObject* watched, QEvent* event) noexcept override;

private:  // Methods
  int calculateSize() const noexcept;
  void updateSize() noexcept;
  void updatePosition() noexcept;

private:  // Data
  QColor mColor;
  int mTotalRotations;
  int mCurrentRotation;
  int mCircleDiameter;
  int mDotDiameter;
  int mMargin;
  QScopedPointer<QTimer> mTimer;
  QPointer<QWidget> mEventFilterObject;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
