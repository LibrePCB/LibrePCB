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

#ifndef LIBREPCB_EDITOR_PADSIGNALMAPEDITORWIDGET_H
#define LIBREPCB_EDITOR_PADSIGNALMAPEDITORWIDGET_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/core/library/cmp/componentsignal.h>
#include <librepcb/core/library/dev/devicepadsignalmap.h>
#include <librepcb/core/library/pkg/packagepad.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Workspace;

namespace editor {

class DevicePadSignalMapModel;
class SortFilterProxyModel;
class UndoStack;

/*******************************************************************************
 *  Class PadSignalMapEditorWidget
 ******************************************************************************/

/**
 * @brief The PadSignalMapEditorWidget class
 */
class PadSignalMapEditorWidget final : public QWidget {
  Q_OBJECT

public:
  // Constructors / Destructor
  explicit PadSignalMapEditorWidget(QWidget* parent = nullptr) noexcept;
  PadSignalMapEditorWidget(const PadSignalMapEditorWidget& other) = delete;
  ~PadSignalMapEditorWidget() noexcept;

  // General Methods
  void setFrameStyle(int style) noexcept;
  void setReadOnly(bool readOnly) noexcept;
  void setReferences(UndoStack* undoStack, DevicePadSignalMap* map) noexcept;
  void setPadList(const PackagePadList& list) noexcept;
  void setSignalList(const ComponentSignalList& list) noexcept;

  // Inherited Methods
  bool eventFilter(QObject* watched, QEvent* event) noexcept override;

  // Operator Overloadings
  PadSignalMapEditorWidget& operator=(const PadSignalMapEditorWidget& rhs) =
      delete;

signals:
  void statusTipChanged(const QString& statusTip);

protected:
  void resizeEvent(QResizeEvent* e) noexcept override;
  void keyPressEvent(QKeyEvent* e) noexcept override;

private:
  void scheduleToolButtonPositionUpdate() noexcept;
  void updateToolButtonPosition() noexcept;
  void updateButtonsVisibility() noexcept;
  void toolButtonClicked() noexcept;
  void resetAll() noexcept;
  void autoConnect() noexcept;
  void loadFromFile() noexcept;
  void setInteractiveMode(bool enabled) noexcept;
  void updateInteractiveList(QString filter) noexcept;
  void commitInteractiveMode(const QListWidgetItem* listItem) noexcept;
  QMap<Uuid, Uuid> getMap() const noexcept;
  void setMap(const QString& cmdText, const QMap<Uuid, Uuid>& map);
  bool hasAutoConnectablePads() const noexcept;
  bool hasUnconnectedPadsAndUnusedSignals() const noexcept;
  bool askForResetFirst() noexcept;

private:
  bool mReadOnly;
  int mInteractiveModePadIndex;
  QScopedPointer<DevicePadSignalMapModel> mModel;
  QScopedPointer<SortFilterProxyModel> mProxy;
  QScopedPointer<QTableView> mView;
  QScopedPointer<QFrame> mInteractiveFrame;
  QScopedPointer<QLabel> mInteractiveLabel1;
  QScopedPointer<QLabel> mInteractiveLabel2;
  QScopedPointer<QLineEdit> mInteractiveEdit;
  QScopedPointer<QToolButton> mInteractiveAbortButton;
  QScopedPointer<QListWidget> mInteractiveList;
  QScopedPointer<QToolButton> mToolButton;
  QScopedPointer<QPushButton> mAutoConnectButton;
  QScopedPointer<QFrame> mButtonsVLine;
  QScopedPointer<QPushButton> mInteractiveConnectButton;

  DevicePadSignalMap* mPadSignalMap;
  UndoStack* mUndoStack;
  ComponentSignalList mSignals;
  PackagePadList mPads;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
