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
#include <librepcb/core/types/angle.h>
#include <librepcb/core/types/point.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class FilePath;

namespace editor {

namespace Ui {
class BackgroundImageSetupDialog;
}

/*******************************************************************************
 *  Class BackgroundImageSettings
 ******************************************************************************/

struct BackgroundImageSettings {
  bool enabled = true;  ///< Whether the background is enabled or not
  QImage image;  ///< The original loaded image
  QPointF referencePos;  ///< Reference in #image [pixels]
  std::pair<qreal, qreal> dpi = {0, 0};  ///< Scale X/Y [dpi]
  Point offset;  ///< Destination scene position of #referencePos
  Angle rotation;  ///< Rotation in scene

  void tryLoadFromDir(const FilePath& dir) noexcept;
  void saveToDir(const FilePath& dir) noexcept;

  bool operator==(const BackgroundImageSettings& rhs) const noexcept;
  bool operator!=(const BackgroundImageSettings& rhs) const noexcept;
};

/*******************************************************************************
 *  Class BackgroundImageSetupDialog
 ******************************************************************************/

/**
 * @brief Dialog (GUI) to configure the background image of a 2D view
 */
class BackgroundImageSetupDialog final : public QDialog {
  Q_OBJECT

  enum class State {
    Idle,
    SelectReference,
    MeasureStep1,
    MeasureStep2,
    MeasureStep3,
  };

public:
  // Constructors / Destructor
  BackgroundImageSetupDialog() = delete;
  BackgroundImageSetupDialog(const BackgroundImageSetupDialog& other) = delete;
  explicit BackgroundImageSetupDialog(const QString& settingsPrefix,
                                      QWidget* parent = nullptr) noexcept;
  ~BackgroundImageSetupDialog() noexcept;

  // General Methods
  void setSettings(const BackgroundImageSettings& s) noexcept;
  BackgroundImageSettings getSettings() const noexcept;

  // Operator Overloadings
  BackgroundImageSetupDialog& operator=(const BackgroundImageSetupDialog& rhs) =
      delete;

signals:
  void settingsModified();

private:
  void keyPressEvent(QKeyEvent* event) noexcept override;
  bool eventFilter(QObject* obj, QEvent* e) noexcept override;
  void takeScreenshot() noexcept;
  void pasteFromClipboard() noexcept;
  void loadFromFile() noexcept;
  void updateImageLabel() noexcept;
  void setState(State state) noexcept;

  QScopedPointer<Ui::BackgroundImageSetupDialog> mUi;
  const QString mSettingsPrefix;
  State mState;
  QImage mImage;
  qreal mViewScaleFactor;
  QPointF mMeasurePos1;  // In image pixel coordinates
  QPointF mMeasurePos2;  // In image pixel coordinates
  QPointF mCurrentCursorPos;  // In image pixel coordinates
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
