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
#include "keysequenceseditorwidget.h"

#include "../utils/editortoolbox.h"

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

KeySequencesEditorWidget::KeySequencesEditorWidget(
    const QList<QKeySequence>& defaultSequences, QWidget* parent) noexcept
  : QWidget(parent),
    mLayout(new QVBoxLayout()),
    mDefault(defaultSequences),
    mOverrides(),
    mRowHeight(25) {
  mLayout->setContentsMargins(0, 0, 0, 0);
  mLayout->setSpacing(0);
  setLayout(mLayout);
}

KeySequencesEditorWidget::~KeySequencesEditorWidget() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void KeySequencesEditorWidget::setOverrides(
    const std::optional<QList<QKeySequence> >& overrides) noexcept {
  mOverrides = overrides;
  updateWidgets();
}

void KeySequencesEditorWidget::setRowHeight(int height) noexcept {
  foreach (QKeySequenceEdit* edt, findChildren<QKeySequenceEdit*>()) {
    edt->setFixedHeight(height);
  }
  foreach (QToolButton* btn, findChildren<QToolButton*>()) {
    btn->setFixedHeight(height);
    if (btn->sizePolicy().horizontalPolicy() == QSizePolicy::Fixed) {
      btn->setFixedWidth(height);
    }
    btn->setIconSize(QSize(height, height) * 0.8);
  }
  mRowHeight = height;
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void KeySequencesEditorWidget::updateWidgets() noexcept {
  setFocusProxy(nullptr);
  while (mLayout->count() > 0) {
    QLayoutItem* item = mLayout->takeAt(0);
    Q_ASSERT(item);
    EditorToolbox::deleteLayoutItemRecursively(item);
  }

  QList<QKeySequence> sequences = mOverrides ? (*mOverrides) : mDefault;

  for (int i = 0; i <= sequences.count(); ++i) {
    QHBoxLayout* hLayout = new QHBoxLayout();
    hLayout->setContentsMargins(0, 0, 0, 0);
    hLayout->setSpacing(0);

    QKeySequenceEdit* edit = new QKeySequenceEdit(this);
    edit->setKeySequence(sequences.value(i));
    if (i == 0) {
      // When opening this widget, move focus into first edit field.
      setFocusProxy(edit);
    }
    connect(edit, &QKeySequenceEdit::editingFinished, edit, [this, edit, i]() {
      if (!mOverrides) {
        mOverrides = mDefault;
      }
      const bool exists = (i < mOverrides->count());
      const bool valid = !edit->keySequence().isEmpty();
      if (valid && exists) {
        mOverrides->replace(i, edit->keySequence());
      } else if (exists) {
        mOverrides->removeAt(i);
      } else if (valid) {
        mOverrides->append(edit->keySequence());
      }
      updateWidgets();
    });
    hLayout->addWidget(edit);

    if (i < sequences.count()) {
      QToolButton* btnClear = new QToolButton(this);
      btnClear->setToolTip(tr("Remove this shortcut"));
      btnClear->setIcon(QIcon(":/img/actions/delete.png"));
      connect(btnClear, &QToolButton::clicked, this, [this, i]() {
        if (!mOverrides) {
          mOverrides = mDefault;
        }
        mOverrides->removeAt(i);
        updateWidgets();
      });
      hLayout->addWidget(btnClear);
    } else {
      QToolButton* btnReset = new QToolButton(this);
      btnReset->setToolTip(tr("Restore default shortcut(s)"));
      btnReset->setIcon(QIcon(":/img/actions/undo.png"));
      btnReset->setEnabled(mOverrides.has_value());
      connect(btnReset, &QToolButton::clicked, this, [this]() {
        mOverrides = std::nullopt;
        updateWidgets();
      });
      hLayout->addWidget(btnReset);
    }

    mLayout->addLayout(hLayout);
  }

  QHBoxLayout* hLayout = new QHBoxLayout();
  hLayout->setContentsMargins(0, 0, 0, 0);
  hLayout->setSpacing(0);

  QToolButton* btnApply = new QToolButton(this);
  btnApply->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  btnApply->setToolTip(tr("Apply"));
  btnApply->setIcon(QIcon(":/img/actions/apply.png"));
  connect(btnApply, &QToolButton::clicked, this,
          &KeySequencesEditorWidget::applyTriggered);
  hLayout->addWidget(btnApply);

  QToolButton* btnCancel = new QToolButton(this);
  btnCancel->setToolTip(tr("Cancel"));
  btnCancel->setIcon(QIcon(":/img/actions/cancel.png"));
  connect(btnCancel, &QToolButton::clicked, this,
          &KeySequencesEditorWidget::cancelTriggered);
  hLayout->addWidget(btnCancel);

  mLayout->addLayout(hLayout);

  setFixedHeight(mRowHeight * (sequences.count() + 2));
  setRowHeight(mRowHeight);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
