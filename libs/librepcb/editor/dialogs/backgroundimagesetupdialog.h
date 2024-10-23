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

#ifndef LIBREPCB_EDITOR_BACKGROUNDIMAGESETUPDIALOG_H
#define LIBREPCB_EDITOR_BACKGROUNDIMAGESETUPDIALOG_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../widgets/if_graphicsvieweventhandler.h"

#include <librepcb/core/types/angle.h>
#include <librepcb/core/types/point.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

namespace Ui {
class BackgroundImageSetupDialog;
}

/*******************************************************************************
 *  Class BackgroundImageSetupDialog
 ******************************************************************************/

/**
 * @brief Dialog (GUI) to configure the background image of a 2D view
 */
class BackgroundImageSetupDialog final : public QDialog,
                                         private IF_GraphicsViewEventHandler {
  Q_OBJECT

public:
  // Types
  enum class State {
    Idle,
    Crop,
    Rotate,
    SelectRef1,
    SelectRef2,
  };

  // Constructors / Destructor
  BackgroundImageSetupDialog() = delete;
  BackgroundImageSetupDialog(const BackgroundImageSetupDialog& other) = delete;
  explicit BackgroundImageSetupDialog(const QString& settingsPrefix,
                                      QWidget* parent = nullptr) noexcept;
  ~BackgroundImageSetupDialog() noexcept;

  // General Methods
  const QImage& getImage() const noexcept { return mImage; }
  const Angle& getRotation() const noexcept { return mRotation; }
  const QList<std::pair<QPointF, Point>>& getReferences() const noexcept {
    return mReferences;
  }
  void setData(const QImage& image, const Angle& rotation,
               const QList<std::pair<QPointF, Point>>& references) noexcept;

  // Operator Overloadings
  BackgroundImageSetupDialog& operator=(const BackgroundImageSetupDialog& rhs) =
      delete;

private:
  void keyPressEvent(QKeyEvent* event) noexcept override;
  bool graphicsViewEventHandler(QEvent* event) noexcept override;
  void startScreenshot() noexcept;
  void screenshotCountdownTick() noexcept;
  void takeScreenshot() noexcept;
  void pasteFromClipboard() noexcept;
  void loadFromFile() noexcept;
  void updateUi(QString msg = QString()) noexcept;
  void fitImageInView() noexcept;
  void updateAnchors() noexcept;
  void updateStatusMsg() noexcept;
  static QImage cropImage(const QImage& img, const QPainterPath& p) noexcept;

  QScopedPointer<Ui::BackgroundImageSetupDialog> mUi;
  const QString mSettingsPrefix;

  // State
  State mState;
  QImage mImage;
  Angle mRotation;
  QList<std::pair<QPointF, Point>> mLoadedReferences;
  QList<std::pair<QPointF, Point>> mReferences;
  QPointer<QScreen> mScreen;
  int mCountdownSecs;

  // Widgets & graphics items
  QScopedPointer<QWidget> mRotateWidget;
  QScopedPointer<QGraphicsPixmapItem> mImageGraphicsItem;
  std::shared_ptr<QGraphicsPathItem> mCursorGraphicsItem;
  QScopedPointer<QGraphicsPathItem> mCropGraphicsItem;

  // Always in sync with #mReferences
  QList<std::shared_ptr<QGraphicsPathItem>> mReferenceGraphicsItems;
  QList<std::shared_ptr<QGraphicsLineItem>> mReferenceLineGraphicsItems;
  QList<std::shared_ptr<QWidget>> mReferenceWidgets;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
