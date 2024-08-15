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
#include "circuitidentifierimportdialog.h"

#include "ui_circuitidentifierimportdialog.h"

#include <librepcb/core/exceptions.h>
#include <librepcb/core/utils/toolbox.h>

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

CircuitIdentifierImportDialog::CircuitIdentifierImportDialog(
    const QString& settingsPrefix, QWidget* parent) noexcept
  : QDialog(parent),
    mUi(new Ui::CircuitIdentifierImportDialog),
    mSettingsPrefix(settingsPrefix),
    mSpaceRegex("[\\s]"),
    mFilterColumnIndex(0) {
  mUi->setupUi(this);

  connect(mUi->buttonBox, &QDialogButtonBox::accepted, this,
          &CircuitIdentifierImportDialog::accept);
  connect(mUi->buttonBox, &QDialogButtonBox::rejected, this,
          &CircuitIdentifierImportDialog::reject);
  connect(mUi->btnRecordClipboard, &QToolButton::toggled, this,
          &CircuitIdentifierImportDialog::updatePlaceholder);
  connect(mUi->btnRecordClipboard, &QToolButton::toggled, this,
          [](bool checked) {
            if (checked && (!qApp->clipboard()->text().isEmpty())) {
              qApp->clipboard()->setText("");  // clear() does not work!
            }
          });
  connect(mUi->txtInput, &QTextEdit::textChanged, this,
          &CircuitIdentifierImportDialog::parseInput);
  connect(mUi->cbxFilterColumn, &QCheckBox::toggled, this,
          &CircuitIdentifierImportDialog::parseInput);
  connect(mUi->cbxFilterColumn, &QCheckBox::toggled, mUi->spbxColumn,
          &QSpinBox::setEnabled);
  connect(mUi->spbxColumn,
          static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this,
          &CircuitIdentifierImportDialog::parseInput);
  connect(mUi->cbxSort, &QCheckBox::toggled, this,
          &CircuitIdentifierImportDialog::parseInput);
  connect(mUi->txtInput->verticalScrollBar(), &QScrollBar::valueChanged,
          mUi->txtResult->verticalScrollBar(), &QScrollBar::setValue);
  connect(mUi->txtResult->verticalScrollBar(), &QScrollBar::valueChanged,
          mUi->txtInput->verticalScrollBar(), &QScrollBar::setValue);

  QTimer* timer = new QTimer(this);
  timer->setInterval(200);
  connect(timer, &QTimer::timeout, this,
          &CircuitIdentifierImportDialog::checkClipboard);
  timer->start();

  checkClipboard();
  updatePlaceholder();
  parseInput();

  // Load client settings.
  QSettings cs;
  QSize windowSize = cs.value(mSettingsPrefix % "/window_size").toSize();
  if (!windowSize.isEmpty()) {
    resize(windowSize);
  }
  mUi->btnRecordClipboard->setChecked(
      cs.value(mSettingsPrefix % "/record_clipboard", false).toBool());
  mUi->cbxFilterColumn->setChecked(
      cs.value(mSettingsPrefix % "/filter", true).toBool());
  mUi->spbxColumn->setValue(cs.value(mSettingsPrefix % "/column").toInt());
  mUi->cbxSort->setChecked(cs.value(mSettingsPrefix % "/sort", false).toBool());
}

CircuitIdentifierImportDialog::~CircuitIdentifierImportDialog() noexcept {
  QSettings cs;
  cs.setValue(mSettingsPrefix % "/window_size", size());
  cs.setValue(mSettingsPrefix % "/record_clipboard",
              mUi->btnRecordClipboard->isChecked());
  cs.setValue(mSettingsPrefix % "/filter", mUi->cbxFilterColumn->isChecked());
  cs.setValue(mSettingsPrefix % "/column", mUi->spbxColumn->value());
  cs.setValue(mSettingsPrefix % "/sort", mUi->cbxSort->isChecked());
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void CircuitIdentifierImportDialog::updatePlaceholder() noexcept {
  const bool autoMode = mUi->btnRecordClipboard->isChecked();

  QString s;
  s +=
      tr("Specify the items for mass import in this text field, each item on "
         "a separate line.");
  s += " " %
      tr("To copy values e.g. from a datasheet PDF, two modes are "
         "available:");
  s += QString("\n\n%1 ").arg(autoMode ? "○" : "●");
  s +=
      tr("Copy a whole table from the PDF and paste it into this field. "
         "Attention: If the table contains line breaks, manually remove "
         "unrelated lines afterwards! Also note that this does not work with "
         "every PDF reader.");
  s += QString("\n\n%1 ").arg(autoMode ? "●" : "○");
  s += tr("Check the button '%1' below and copy item-by-item into the "
          "clipboard. LibrePCB monitors the clipboard and automatically "
          "pastes each item here.")
           .arg(mUi->btnRecordClipboard->text());
  if (autoMode) {
    s += "\n\n" %
        tr("Clipboard monitoring is active! Now copy the items one-by-one into "
           "the clipboard. LibrePCB does not need to stay in foreground for "
           "this.");
  }
  mUi->txtInput->setPlaceholderText(s);
}

void CircuitIdentifierImportDialog::checkClipboard() noexcept {
  const QString value = qApp->clipboard()->text().trimmed();
  if ((value != mLastClipboardValue) &&
      (mUi->btnRecordClipboard->isChecked()) && (!value.isEmpty())) {
    mUi->txtInput->append(value);
    mUi->txtInput->verticalScrollBar()->setValue(
        mUi->txtInput->verticalScrollBar()->maximum());
    qApp->beep();
  }
  mLastClipboardValue = value;
}

void CircuitIdentifierImportDialog::parseInput() noexcept {
  // Get input lines.
  const QString inputText = mUi->txtInput->toPlainText();
  const QStringList inputLines = inputText.split("\n");

  // If enabled, auto-detect and apply column to filter.
  mFilterColumnIndex = mUi->spbxColumn->value() - 1;
  if (mFilterColumnIndex < 0) {
    autoDetectFilterColumn(inputLines);
  }

  // Determine input values (keep empty lines).
  struct InputItem {
    QString input;  // whole line as-is, just trimmed
    QString filtered;  // column extracted
    QString cleaned;  // converted to CircuitIdentifier
  };
  int nonEmptyLines = 0;
  QList<InputItem> items;
  for (const QString& line : inputLines) {
    InputItem item;
    item.input = line.trimmed();
    item.filtered = item.input.replace(", ", ",");
    if (mUi->cbxFilterColumn->isChecked()) {
      item.filtered =
          item.filtered.split(mSpaceRegex).value(mFilterColumnIndex);
    }
    item.cleaned = cleanCircuitIdentifier(item.filtered);
    items.append(item);
    if (!item.input.isEmpty()) {
      ++nonEmptyLines;
    }
  }

  // If checked, sort lines (and move empty lines to end).
  if (mUi->cbxSort->isChecked()) {
    Toolbox::sortNumeric(
        items,
        [](const QCollator& cmp, const InputItem& a, const InputItem& b) {
          if (a.filtered.isEmpty() || b.filtered.isEmpty()) {
            return b.filtered.isEmpty();
          } else if (a.cleaned.isEmpty() || b.cleaned.isEmpty()) {
            return b.cleaned.isEmpty();
          } else {
            return cmp(a.cleaned, b.cleaned);
          }
        });
  }

  // Update result.
  QSet<QString> filteredValues;
  QSet<QString> cleanedValues;
  QSignalBlocker signalBlocker(mUi->txtResult->verticalScrollBar());
  mUi->txtResult->clear();
  mValues.clear();
  for (const InputItem& item : items) {
    if (item.cleaned.isEmpty() && (!item.filtered.isEmpty())) {
      mUi->txtResult->setTextColor(Qt::red);
      mUi->txtResult->append("(" % tr("INVALID INPUT") % ")");
    } else if (filteredValues.contains(item.filtered)) {
      mUi->txtResult->setTextColor(QColor(255, 165, 0));  // orange
      mUi->txtResult->append(item.cleaned % " (" % tr("DUPLICATE") % ")");
    } else if (cleanedValues.contains(item.cleaned)) {
      mUi->txtResult->setTextColor(Qt::red);
      mUi->txtResult->append(item.cleaned % " (" % tr("NAME CONFLICT") % ")");
    } else {
      QTextCharFormat f;
      f.setForeground(QBrush());
      mUi->txtResult->mergeCurrentCharFormat(f);
      mUi->txtResult->append(item.cleaned + " ");
      if (CircuitIdentifierConstraint()(item.cleaned)) {
        mValues.append(CircuitIdentifier(item.cleaned));
      }
    }
    if (!item.filtered.isEmpty()) {
      filteredValues.insert(item.filtered);
    }
    if (!item.cleaned.isEmpty()) {
      cleanedValues.insert(item.cleaned);
    }
  }
  mUi->txtResult->verticalScrollBar()->setValue(
      mUi->txtInput->verticalScrollBar()->value());

  // Update statistics.
  mUi->gbxInput->setTitle(tr("Input") % QString(" (%1)").arg(nonEmptyLines));
  mUi->gbxResult->setTitle(tr("Result") %
                           QString(" (%1)").arg(mValues.count()));
}

void CircuitIdentifierImportDialog::autoDetectFilterColumn(
    const QStringList& lines) noexcept {
  QList<QStringList> data;
  int columnCount = 0;
  for (QString line : lines) {
    line = line.trimmed();
    if (!line.isEmpty()) {
      const QStringList cols = line.replace(", ", ",").split(mSpaceRegex);
      data.append(cols);
      if ((columnCount == 0) || (cols.count() < columnCount)) {
        columnCount = cols.count();
      }
    }
  }
  if (data.isEmpty()) {
    return;
  }

  // Select first column which contains not only numbers.
  mFilterColumnIndex = 0;
  while ((mFilterColumnIndex < (columnCount - 1)) &&
         columnContainsOnlyNumbers(data, mFilterColumnIndex)) {
    ++mFilterColumnIndex;
  }
}

bool CircuitIdentifierImportDialog::columnContainsOnlyNumbers(
    const QList<QStringList>& data, int col) const noexcept {
  const QRegularExpression re("^[\\d, ]+$");
  for (const QStringList& row : data) {
    if (!re.match(row.value(col)).hasMatch()) {
      return false;
    }
  }
  return true;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
