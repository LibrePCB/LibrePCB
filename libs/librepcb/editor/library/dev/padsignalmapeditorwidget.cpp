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

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "padsignalmapeditorwidget.h"

#include "../../dialogs/filedialog.h"
#include "../../library/dev/devicepadsignalmapmodel.h"
#include "../../modelview/comboboxdelegate.h"
#include "../../modelview/sortfilterproxymodel.h"
#include "../../undocommandgroup.h"
#include "../../undostack.h"
#include "../cmd/cmddevicepadsignalmapitemedit.h"

#include <librepcb/core/fileio/fileutils.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

PadSignalMapEditorWidget::PadSignalMapEditorWidget(QWidget* parent) noexcept
  : QWidget(parent),
    mReadOnly(false),
    mInteractiveModePadIndex(-1),
    mModel(new DevicePadSignalMapModel(this)),
    mProxy(new SortFilterProxyModel(this)),
    mView(new QTableView(this)),
    mInteractiveFrame(new QFrame(this)),
    mInteractiveLabel1(new QLabel(this)),
    mInteractiveLabel2(new QLabel(this)),
    mInteractiveEdit(new QLineEdit(this)),
    mInteractiveAbortButton(new QToolButton(this)),
    mInteractiveList(new QListWidget(this)),
    mToolButton(new QToolButton(this)),
    mAutoConnectButton(new QPushButton(this)),
    mButtonsVLine(new QFrame(this)),
    mInteractiveConnectButton(new QPushButton(this)),
    mPadSignalMap(nullptr),
    mUndoStack(nullptr),
    mSignals(),
    mPads() {
  mProxy->setSourceModel(mModel.data());
  mView->setModel(mProxy.data());
  mView->setAlternatingRowColors(true);  // increase readability
  mView->setCornerButtonEnabled(false);  // not needed
  mView->setSelectionBehavior(QAbstractItemView::SelectRows);
  mView->setSelectionMode(QAbstractItemView::SingleSelection);
  mView->setEditTriggers(QAbstractItemView::AllEditTriggers);
  mView->setSortingEnabled(true);
  mView->setWordWrap(false);  // avoid too high cells due to word wrap
  mView->verticalHeader()->setVisible(false);  // no content
  mView->verticalHeader()->setMinimumSectionSize(10);  // more compact rows
  mView->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
  mView->horizontalHeader()->setSectionResizeMode(
      DevicePadSignalMapModel::COLUMN_PAD, QHeaderView::ResizeToContents);
  mView->horizontalHeader()->setSectionResizeMode(
      DevicePadSignalMapModel::COLUMN_SIGNAL, QHeaderView::Stretch);
  mView->setItemDelegateForColumn(DevicePadSignalMapModel::COLUMN_SIGNAL,
                                  new ComboBoxDelegate(false, this));
  mView->sortByColumn(DevicePadSignalMapModel::COLUMN_PAD, Qt::AscendingOrder);
  mView->verticalScrollBar()->installEventFilter(this);

  mInteractiveFrame->setObjectName("interactiveFrame");
  mInteractiveFrame->setFrameStyle(QFrame::Box | QFrame::Plain);
  mInteractiveFrame->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
  mInteractiveFrame->setStyleSheet(
      "#interactiveFrame {border: 1px solid gray; border-radius: 2px;}");
  mInteractiveFrame->setFixedHeight(25);

  mInteractiveLabel1->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
  mInteractiveLabel1->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
  mInteractiveLabel1->setFixedHeight(
      mInteractiveFrame->contentsRect().height());

  mInteractiveLabel2->setSizePolicy(QSizePolicy::Expanding,
                                    QSizePolicy::Expanding);
  mInteractiveLabel2->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
  mInteractiveLabel2->setFixedHeight(
      mInteractiveFrame->contentsRect().height());

  mInteractiveEdit->setFixedHeight(mInteractiveFrame->height());
  mInteractiveEdit->installEventFilter(this);
  connect(mInteractiveEdit.data(), &QLineEdit::textChanged, this,
          &PadSignalMapEditorWidget::updateInteractiveList);

  mInteractiveAbortButton->setIcon(QIcon(":/img/actions/stop.png"));
  mInteractiveAbortButton->setToolTip(
      tr("Exit interactive mode") %
      QString(" (%1)").arg(
          QKeySequence(Qt::Key_Escape).toString(QKeySequence::NativeText)));
  mInteractiveAbortButton->setFixedSize(mInteractiveFrame->height(),
                                        mInteractiveFrame->height());
  mInteractiveAbortButton->setFocusPolicy(Qt::NoFocus);
  connect(mInteractiveAbortButton.data(), &QToolButton::clicked, this,
          [this]() { setInteractiveMode(false); });

  connect(mInteractiveList.data(), &QListWidget::currentTextChanged,
          mInteractiveEdit.data(), &QLineEdit::setPlaceholderText);
  connect(mInteractiveList.data(), &QListWidget::itemDoubleClicked, this,
          &PadSignalMapEditorWidget::commitInteractiveMode);
  setInteractiveMode(false);

  //: Please try to keep it short!
  mAutoConnectButton->setText(tr("Auto-Connect"));
  mAutoConnectButton->setToolTip(
      tr("Try to automatically connect pads to signals by their name"));
  mAutoConnectButton->setAutoFillBackground(true);
  mAutoConnectButton->setFlat(true);
  mAutoConnectButton->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
  connect(mAutoConnectButton.data(), &QPushButton::clicked, this,
          &PadSignalMapEditorWidget::autoConnect);

  mButtonsVLine->setFrameStyle(QFrame::VLine);
  mButtonsVLine->setStyleSheet("color: lightgray;");

  //: Please try to keep it short!
  mInteractiveConnectButton->setText(tr("Connect Interactively"));
  mInteractiveConnectButton->setToolTip(
      tr("Connect the remaining pads one by one in an interactive mode"));
  mInteractiveConnectButton->setAutoFillBackground(true);
  mInteractiveConnectButton->setFlat(true);
  mInteractiveConnectButton->setSizePolicy(QSizePolicy::Ignored,
                                           QSizePolicy::Fixed);
  connect(mInteractiveConnectButton.data(), &QPushButton::clicked, this,
          [this]() { setInteractiveMode(true); });

  connect(mModel.data(), &DevicePadSignalMapModel::dataChanged, this,
          &PadSignalMapEditorWidget::updateButtonsVisibility,
          Qt::QueuedConnection);
  updateButtonsVisibility();

  QHBoxLayout* hLayoutFrame = new QHBoxLayout(mInteractiveFrame.data());
  hLayoutFrame->setContentsMargins(3, 0, 3, 0);
  hLayoutFrame->addWidget(mInteractiveLabel1.data());
  hLayoutFrame->addWidget(mInteractiveLabel2.data());

  QHBoxLayout* hLayoutTop = new QHBoxLayout();
  hLayoutTop->setContentsMargins(0, 0, 0, 0);
  hLayoutTop->setSpacing(3);
  hLayoutTop->addWidget(mInteractiveFrame.data());
  hLayoutTop->setStretchFactor(mInteractiveFrame.data(), 3);
  hLayoutTop->addWidget(mInteractiveEdit.data());
  hLayoutTop->setStretchFactor(mInteractiveEdit.data(), 2);
  hLayoutTop->addWidget(mInteractiveAbortButton.data());
  hLayoutTop->setStretchFactor(mInteractiveAbortButton.data(), 0);

  QHBoxLayout* hLayoutBottom = new QHBoxLayout();
  hLayoutBottom->setContentsMargins(0, 0, 0, 0);
  hLayoutBottom->setSpacing(0);
  hLayoutBottom->addWidget(mAutoConnectButton.data());
  hLayoutBottom->setStretchFactor(mAutoConnectButton.data(),
                                  mAutoConnectButton->sizeHint().width());
  hLayoutBottom->addWidget(mButtonsVLine.data());
  hLayoutBottom->addWidget(mInteractiveConnectButton.data());
  hLayoutBottom->setStretchFactor(
      mInteractiveConnectButton.data(),
      mInteractiveConnectButton->sizeHint().width());

  QVBoxLayout* vLayout = new QVBoxLayout(this);
  vLayout->setContentsMargins(0, 0, 0, 0);
  vLayout->setSpacing(0);
  vLayout->addItem(hLayoutTop);
  vLayout->addWidget(mInteractiveList.data());
  vLayout->addWidget(mView.data());
  vLayout->addItem(hLayoutBottom);

  mToolButton->setArrowType(Qt::DownArrow);
  mToolButton->setFixedSize(mView->horizontalHeader()->height() - 5,
                            mView->horizontalHeader()->height() - 5);
  mToolButton->setFocusPolicy(Qt::NoFocus);
  connect(mToolButton.data(), &QToolButton::clicked, this,
          &PadSignalMapEditorWidget::toolButtonClicked);
  scheduleToolButtonPositionUpdate();
}

PadSignalMapEditorWidget::~PadSignalMapEditorWidget() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void PadSignalMapEditorWidget::setFrameStyle(int style) noexcept {
  mView->setFrameStyle(style);
}

void PadSignalMapEditorWidget::setReadOnly(bool readOnly) noexcept {
  mReadOnly = readOnly;
  mView->setEditTriggers(readOnly
                             ? QAbstractItemView::EditTrigger::NoEditTriggers
                             : QAbstractItemView::AllEditTriggers);
  updateButtonsVisibility();
}

void PadSignalMapEditorWidget::setReferences(UndoStack* undoStack,
                                             DevicePadSignalMap* map) noexcept {
  setInteractiveMode(false);
  mPadSignalMap = map;
  mUndoStack = undoStack;
  mModel->setPadSignalMap(map);
  mModel->setUndoStack(undoStack);
  updateButtonsVisibility();
}

void PadSignalMapEditorWidget::setPadList(const PackagePadList& list) noexcept {
  setInteractiveMode(false);
  QCollator collator;
  collator.setNumericMode(true);
  collator.setCaseSensitivity(Qt::CaseInsensitive);
  collator.setIgnorePunctuation(false);
  mPads =
      list.sorted([&collator](const PackagePad& lhs, const PackagePad& rhs) {
        return collator(*lhs.getName(), *rhs.getName());
      });

  mModel->setPadList(list);
  updateButtonsVisibility();
}

void PadSignalMapEditorWidget::setSignalList(
    const ComponentSignalList& list) noexcept {
  setInteractiveMode(false);
  mSignals = list;
  mModel->setSignalList(list);
  updateButtonsVisibility();
}

/*******************************************************************************
 *  Inherited Methods
 ******************************************************************************/

bool PadSignalMapEditorWidget::eventFilter(QObject* watched,
                                           QEvent* event) noexcept {
  if ((watched == mView->verticalScrollBar()) &&
      ((event->type() == QEvent::Show) || (event->type() == QEvent::Hide))) {
    scheduleToolButtonPositionUpdate();
  } else if ((watched == mInteractiveEdit.data()) &&
             (mInteractiveModePadIndex != -1) &&
             (event->type() == QEvent::KeyPress)) {
    const QKeyEvent* ke = static_cast<QKeyEvent*>(event);
    const int row = mInteractiveList->currentRow();
    const int count = mInteractiveList->count();
    if (ke->key() == Qt::Key_Down) {
      mInteractiveList->setCurrentRow((row + 1) % count);
      return true;
    } else if (ke->key() == Qt::Key_Up) {
      mInteractiveList->setCurrentRow((std::max(row, 0) - 1 + count) % count);
      return true;
    } else if ((!mInteractiveEdit->text().isEmpty()) &&
               (ke->key() == Qt::Key_Escape)) {
      mInteractiveEdit->clear();
      return true;
    } else if ((mInteractiveEdit->text().isEmpty()) &&
               (ke->key() == Qt::Key_Backspace)) {
      for (int i = 0; i < mInteractiveList->count(); ++i) {
        if (const QListWidgetItem* item = mInteractiveList->item(i)) {
          if (item->data(Qt::UserRole).isNull()) {
            mInteractiveList->setCurrentRow(i);
            return true;
          }
        }
      }
    }
  }
  return QWidget::eventFilter(watched, event);
}

/*******************************************************************************
 *  Protected Methods
 ******************************************************************************/

void PadSignalMapEditorWidget::resizeEvent(QResizeEvent* e) noexcept {
  Q_UNUSED(e);
  scheduleToolButtonPositionUpdate();
}

void PadSignalMapEditorWidget::keyPressEvent(QKeyEvent* e) noexcept {
  if (mInteractiveModePadIndex != -1) {
    switch (e->key()) {
      case Qt::Key_Return: {
        commitInteractiveMode(mInteractiveList->currentItem());
        e->accept();
        return;
      }
      case Qt::Key_Escape: {
        if (!mInteractiveEdit->text().isEmpty()) {
          mInteractiveEdit->clear();
        } else {
          setInteractiveMode(false);
        }
        e->accept();
        return;
      }
      default:
        break;
    }
  }
  return QWidget::keyPressEvent(e);
}

/*******************************************************************************
 *  Protected Methods
 ******************************************************************************/

void PadSignalMapEditorWidget::scheduleToolButtonPositionUpdate() noexcept {
  QMetaObject::invokeMethod(this,
                            &PadSignalMapEditorWidget::updateToolButtonPosition,
                            Qt::QueuedConnection);
}

void PadSignalMapEditorWidget::updateToolButtonPosition() noexcept {
  int x = width() - mToolButton->width() - 2;
  if (mView->verticalScrollBar()->isVisible()) {
    x -= mView->verticalScrollBar()->width();
  }
  mToolButton->move(x, 2);
}

void PadSignalMapEditorWidget::updateButtonsVisibility() noexcept {
  mToolButton->setVisible((!mReadOnly) && (mInteractiveModePadIndex == -1));
  mAutoConnectButton->setVisible((!mReadOnly) &&
                                 (mInteractiveModePadIndex == -1) &&
                                 hasAutoConnectablePads());
  mInteractiveConnectButton->setVisible((!mReadOnly) &&
                                        (mInteractiveModePadIndex == -1) &&
                                        hasUnconnectedPadsAndUnusedSignals());
  mButtonsVLine->setVisible(mAutoConnectButton->isVisible() &&
                            mInteractiveConnectButton->isVisible());
}

void PadSignalMapEditorWidget::toolButtonClicked() noexcept {
  QMenu menu(this);

  QAction* aReset =
      menu.addAction(QIcon(":/img/actions/undo.png"), tr("Reset All"));
  aReset->setStatusTip(tr("Reset all pads to 'unconnected' state"));
  connect(aReset, &QAction::triggered, this,
          &PadSignalMapEditorWidget::resetAll, Qt::QueuedConnection);

  QAction* aLoad =
      menu.addAction(QIcon(":/img/actions/import.png"), tr("Load From File"));
  aLoad->setStatusTip(
      tr("Import the pinout from a CSV file with these columns:") %
      " Pad,Signal");
  connect(aLoad, &QAction::triggered, this,
          &PadSignalMapEditorWidget::loadFromFile, Qt::QueuedConnection);

  foreach (QAction* a, menu.actions()) {
    connect(a, &QAction::hovered, this,
            [this, a]() { emit statusTipChanged(a->statusTip()); });
  }
  menu.exec(mapToGlobal(mToolButton->geometry().bottomLeft()));
  emit statusTipChanged(QString());
}

void PadSignalMapEditorWidget::resetAll() noexcept {
  if (!mPadSignalMap) return;

  try {
    setMap(tr("Reset Pinout"), QMap<Uuid, Uuid>());
  } catch (const Exception& e) {
    QMessageBox::critical(this, tr("Error"), e.getMsg());
  }
}

void PadSignalMapEditorWidget::autoConnect() noexcept {
  if (!mPadSignalMap) return;

  try {
    // Get initial pinout.
    QMap<Uuid, Uuid> map = getMap();
    if ((!map.isEmpty()) && askForResetFirst()) {
      map.clear();
    }

    // Connect.
    for (const auto& pad : mPads) {
      if (map.contains(pad.getUuid())) continue;  // Already connected.
      if (auto s = mSignals.find(*pad.getName(), Qt::CaseSensitive)) {
        map.insert(pad.getUuid(), s->getUuid());
      } else if (auto s = mSignals.find(*pad.getName(), Qt::CaseInsensitive)) {
        map.insert(pad.getUuid(), s->getUuid());
      }
    }

    // Save pinout.
    setMap(tr("Auto-Connect Pads To Signals"), map);
  } catch (const Exception& e) {
    QMessageBox::critical(this, tr("Error"), e.getMsg());
  }
}

void PadSignalMapEditorWidget::loadFromFile() noexcept {
  if (!mPadSignalMap) return;

  try {
    // Select file.
    QSettings cs;
    const QString csKey = "library_editor/device_editor/load_pinout_file";
    const QString defaultFp = cs.value(csKey, QDir::homePath()).toString();
    const FilePath fp(FileDialog::getOpenFileName(
        nullptr, tr("Choose Pinout File"), defaultFp,
        "Comma-Separated Values (*.csv)"));
    if (!fp.isValid()) return;
    cs.setValue(csKey, fp.toStr());

    // Parse file.
    QMap<QString, QString> pinout;  // pad name -> signal name
    const QStringList lines =
        QString(FileUtils::readFile(fp)).remove("\r").split("\n");
    const QStringList cols = lines.value(0).toLower().split(',');
    const int padCol = cols.contains("pad") ? cols.indexOf("pad") : 0;
    const int signalCol = cols.contains("signal") ? cols.indexOf("signal") : 1;
    foreach (const QString& line, lines) {
      const QStringList values = line.split(',');
      if (values.count() > std::max(padCol, signalCol)) {
        pinout.insert(values.at(padCol).trimmed(),
                      values.at(signalCol).trimmed());
      }
    }

    // Get initial pinout.
    QMap<Uuid, Uuid> map = getMap();
    if ((!map.isEmpty()) && askForResetFirst()) {
      map.clear();
    }

    // Map pad and signal names.
    QMap<QString, Uuid> padMap;
    QMap<QString, Uuid> signalMap;
    for (auto it = pinout.begin(); it != pinout.end(); it++) {
      if (auto pad = mPads.find(it.key())) {
        padMap.insert(it.key(), pad->getUuid());
      } else if (auto pad = mPads.find(it.key(), Qt::CaseInsensitive)) {
        padMap.insert(it.key(), pad->getUuid());
      }
      if (auto sig = mSignals.find(it.value())) {
        signalMap.insert(it.value(), sig->getUuid());
      } else if (auto sig = mSignals.find(it.value(), Qt::CaseInsensitive)) {
        signalMap.insert(it.value(), sig->getUuid());
      }
    }

    // Connect.
    for (auto it = pinout.begin(); it != pinout.end(); it++) {
      auto padIt = padMap.find(it.key());
      auto signalIt = signalMap.find(it.value());
      if ((padIt != padMap.end()) && (signalIt != signalMap.end()) &&
          (!map.contains(*padIt)) && (mPadSignalMap->contains(*padIt))) {
        map.insert(*padIt, *signalIt);
      }
    }

    // Save pinout.
    setMap(tr("Load Pinout From File"), map);
  } catch (const Exception& e) {
    QMessageBox::critical(this, tr("Error"), e.getMsg());
  }
}

void PadSignalMapEditorWidget::setInteractiveMode(bool enabled) noexcept {
  if (enabled) {
    mView->hide();
    mInteractiveFrame->show();
    mInteractiveEdit->show();
    mInteractiveAbortButton->show();
    mInteractiveList->show();
    mInteractiveModePadIndex = -1;
    commitInteractiveMode(nullptr);
    emit statusTipChanged(
        tr("Type to filter signals, press %1 or double-click to assign")
            .arg(QKeySequence(Qt::Key_Enter)
                     .toString(QKeySequence::NativeText)));
  } else {
    mInteractiveFrame->hide();
    mInteractiveEdit->hide();
    mInteractiveAbortButton->hide();
    mInteractiveList->hide();
    mView->show();
    mInteractiveModePadIndex = -1;
    emit statusTipChanged(QString());
  }
  updateButtonsVisibility();
}

void PadSignalMapEditorWidget::updateInteractiveList(QString filter) noexcept {
  if (!mPadSignalMap) return;
  filter = filter.toLower();

  QVector<QListWidgetItem*> items;
  QSet<QListWidgetItem*> usedSignals;
  if (filter.isEmpty()) {
    QListWidgetItem* item = new QListWidgetItem(tr("(unconnected)"));
    item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    item->setCheckState(Qt::PartiallyChecked);
    items.append(item);
  }
  for (const auto& signal : mSignals) {
    if (filter.isEmpty() ||
        signal.getName()->toLower().contains(filter.toLower())) {
      QListWidgetItem* item = new QListWidgetItem();
      item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
      item->setText(*signal.getName());
      item->setData(Qt::UserRole, signal.getUuid().toStr());
      for (const auto& mapItem : *mPadSignalMap) {
        if (mapItem.getSignalUuid() == signal.getUuid()) {
          usedSignals.insert(item);
          break;
        }
      }
      item->setCheckState(usedSignals.contains(item) ? Qt::Checked
                                                     : Qt::Unchecked);
      items.append(item);
    }
  }

  const auto pad = mPads.value(mInteractiveModePadIndex);
  const QString padName = pad ? pad->getName()->toLower() : QString();
  Toolbox::sortNumeric(
      items,
      [&usedSignals, &filter, &padName](
          const QCollator& collator, QListWidgetItem* a, QListWidgetItem* b) {
        if (!padName.isEmpty()) {
          const bool aMatch = a->text().toLower() == padName;
          const bool bMatch = b->text().toLower() == padName;
          if (aMatch != bMatch) {
            return aMatch;
          }
        }
        if (!filter.isEmpty()) {
          const bool aMatch = a->text().toLower() == filter;
          const bool bMatch = b->text().toLower() == filter;
          if (aMatch != bMatch) {
            return aMatch;
          }
        }
        const bool aUsed = usedSignals.contains(a);
        const bool bUsed = usedSignals.contains(b);
        if (aUsed != bUsed) {
          return bUsed;
        }
        if (!filter.isEmpty()) {
          const bool aMatch = a->text().toLower().startsWith(filter);
          const bool bMatch = b->text().toLower().startsWith(filter);
          if (aMatch != bMatch) {
            return aMatch;
          }
        }
        const bool aUnconnected = a->data(Qt::UserRole).isNull();
        const bool bUnconnected = b->data(Qt::UserRole).isNull();
        if (aUnconnected != bUnconnected) {
          return aUnconnected;
        }
        return collator(a->text(), b->text());
      },
      Qt::CaseInsensitive, false);

  mInteractiveList->clear();
  foreach (auto item, items) {
    mInteractiveList->addItem(item);
  }

  if (!items.isEmpty()) {
    mInteractiveList->setCurrentRow(0);
  }
}

void PadSignalMapEditorWidget::commitInteractiveMode(
    const QListWidgetItem* listItem) noexcept {
  if (!mPadSignalMap) {
    setInteractiveMode(false);
    return;
  }

  // Commit current pad, if any.
  std::shared_ptr<PackagePad> pad = mPads.value(mInteractiveModePadIndex);
  if (mPadSignalMap && pad && listItem) {
    std::shared_ptr<DevicePadSignalMapItem> item =
        mPadSignalMap->find(pad->getUuid());
    const tl::optional<Uuid> sigUuid =
        Uuid::tryFromString(listItem->data(Qt::UserRole).toString());
    if (item && sigUuid && mUndoStack) {
      try {
        QScopedPointer<CmdDevicePadSignalMapItemEdit> cmd(
            new CmdDevicePadSignalMapItemEdit(*item));
        cmd->setSignalUuid(sigUuid);
        mUndoStack->execCmd(cmd.take());
      } catch (const Exception& e) {
        QMessageBox::critical(this, tr("Error"), e.getMsg());
      }
    }
  }

  // Load next pad.
  while (mInteractiveModePadIndex < (mPads.count() - 1)) {
    ++mInteractiveModePadIndex;
    auto pad = mPads.at(mInteractiveModePadIndex);
    auto item = mPadSignalMap->find(pad->getUuid());
    if (item && (!item->getSignalUuid())) {
      QSignalBlocker blocker(mInteractiveEdit.data());
      mInteractiveEdit->clear();
      updateInteractiveList(QString());
      mInteractiveLabel1->setText(
          "<small>" %
          tr("Pad %1/%2").arg(mInteractiveModePadIndex + 1).arg(mPads.count()) %
          "</small>");
      mInteractiveLabel2->setText(
          "<big><b>" %
          (pad ? *pad->getName() : item->getPadUuid().toStr()).toHtmlEscaped() %
          ":</b></big>");
      mInteractiveEdit->setFocus();
      return;
    }
  }

  // If none loaded, exit interactive mode.
  setInteractiveMode(false);
}

QMap<Uuid, Uuid> PadSignalMapEditorWidget::getMap() const noexcept {
  QMap<Uuid, Uuid> map;
  if (mPadSignalMap) {
    for (auto& item : *mPadSignalMap) {
      if (auto signalUuid = item.getSignalUuid()) {
        map.insert(item.getPadUuid(), *signalUuid);
      }
    }
  }
  return map;
}

void PadSignalMapEditorWidget::setMap(const QString& cmdText,
                                      const QMap<Uuid, Uuid>& map) {
  QScopedPointer<UndoCommandGroup> cmdGrp(new UndoCommandGroup(cmdText));
  for (auto& item : *mPadSignalMap) {
    auto it = map.find(item.getPadUuid());
    const tl::optional<Uuid> sig =
        (it != map.end()) ? tl::make_optional(*it) : tl::nullopt;
    if (item.getSignalUuid() != sig) {
      QScopedPointer<CmdDevicePadSignalMapItemEdit> cmd(
          new CmdDevicePadSignalMapItemEdit(item));
      cmd->setSignalUuid(sig);
      cmdGrp->appendChild(cmd.take());
    }
  }
  if (mUndoStack) {
    mUndoStack->execCmd(cmdGrp.take());
  }
}

bool PadSignalMapEditorWidget::hasAutoConnectablePads() const noexcept {
  if (mPadSignalMap) {
    for (auto& item : *mPadSignalMap) {
      if (!item.getSignalUuid()) {
        if (const auto pad = mPads.find(item.getPadUuid())) {
          if (mSignals.find(*pad->getName(), Qt::CaseInsensitive)) {
            return true;
          }
        }
      }
    }
  }
  return false;
}

bool PadSignalMapEditorWidget::hasUnconnectedPadsAndUnusedSignals()
    const noexcept {
  bool unconnectedPads = false;
  QSet<Uuid> unusedSignals = mSignals.getUuidSet();
  if (mPadSignalMap) {
    for (auto& item : *mPadSignalMap) {
      if (auto signal = item.getSignalUuid()) {
        unusedSignals.remove(*signal);
      } else {
        unconnectedPads = true;
      }
    }
  }
  return (unconnectedPads) && (!unusedSignals.isEmpty());
}

bool PadSignalMapEditorWidget::askForResetFirst() noexcept {
  const QMessageBox::StandardButton btn = QMessageBox::question(
      this, tr("Reset Pinout?"),
      tr("There are already some signals connected. Should they be "
         "disconnected before attempting to make new connections?"),
      QMessageBox::Yes | QMessageBox::No);
  return (btn == QMessageBox::Yes);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
