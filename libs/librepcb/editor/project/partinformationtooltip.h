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

#ifndef LIBREPCB_EDITOR_PARTINFORMATIONTOOLTIP_H
#define LIBREPCB_EDITOR_PARTINFORMATIONTOOLTIP_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "partinformationprovider.h"

#include <QtCore>
#include <QtWidgets>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class NetworkRequest;
class WorkspaceSettings;

namespace editor {

class WaitingSpinnerWidget;

namespace Ui {
class PartInformationToolTip;
}

/*******************************************************************************
 *  Class PartInformationToolTip
 ******************************************************************************/

/**
 * @brief The PartInformationToolTip class
 */
class PartInformationToolTip final : public QFrame {
  Q_OBJECT

public:
  // Constructors / Destructor
  PartInformationToolTip() = delete;
  PartInformationToolTip(const PartInformationToolTip& other) = delete;
  explicit PartInformationToolTip(const WorkspaceSettings& settings,
                                  QWidget* parent = nullptr) noexcept;
  ~PartInformationToolTip() noexcept;

  // Setters
  void setProviderInfo(const QString& name, const QUrl& url,
                       const QPixmap& logo, const QUrl& infoUrl) noexcept;
  void showPart(const std::shared_ptr<
                    const PartInformationProvider::PartInformation>& info,
                const QPoint& pos) noexcept;

  // General Methods
  void hideAndReset(bool reset = true) noexcept;
  virtual bool eventFilter(QObject* watched, QEvent* event) noexcept override;

  // Operator Overloads
  PartInformationToolTip& operator=(const PartInformationToolTip& rhs) = delete;

protected:
  virtual void showEvent(QShowEvent* e) noexcept override;
  virtual void hideEvent(QHideEvent* e) noexcept override;

private:  // Methods
  void scheduleLoadPicture() noexcept;
  void startLoadPicture(bool onlyCache) noexcept;
  void setLabelPixmap(QLabel* label, const QPixmap& pixmap,
                      const QSize& space) noexcept;
  void updateShape() noexcept;
  void setSourceDetailsExpanded(bool expanded, bool animated) noexcept;
  void openUrl(const QUrl& url) noexcept;

private:  // Data
  const WorkspaceSettings& mSettings;
  QScopedPointer<Ui::PartInformationToolTip> mUi;
  QScopedPointer<WaitingSpinnerWidget> mWaitingSpinner;
  QScopedPointer<QVariantAnimation> mExpandAnimation;

  QScopedPointer<QTimer> mPopUpDelayTimer;

  int mArrowPositionY;
  std::shared_ptr<const PartInformationProvider::PartInformation> mPartInfo;
  QScopedPointer<QTimer> mPictureDelayTimer;

  // Constants
  static const constexpr int sPopupDelayMs = 300;
  static const constexpr int sWindowArrowSize = 8;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
