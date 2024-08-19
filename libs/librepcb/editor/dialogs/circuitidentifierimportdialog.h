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

#ifndef LIBREPCB_EDITOR_CIRCUITIDENTIFIERIMPORTDIALOG_H
#define LIBREPCB_EDITOR_CIRCUITIDENTIFIERIMPORTDIALOG_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/core/types/circuitidentifier.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

namespace Ui {
class CircuitIdentifierImportDialog;
}

/*******************************************************************************
 *  Class CircuitIdentifierImportDialog
 ******************************************************************************/

/**
 * @brief The CircuitIdentifierImportDialog class
 */
class CircuitIdentifierImportDialog final : public QDialog {
  Q_OBJECT

public:
  // Constructors / Destructor
  CircuitIdentifierImportDialog() = delete;
  CircuitIdentifierImportDialog(const CircuitIdentifierImportDialog& other) =
      delete;
  CircuitIdentifierImportDialog(const QString& settingsPrefix,
                                QWidget* parent = nullptr) noexcept;
  ~CircuitIdentifierImportDialog() noexcept;

  // Getters
  const QList<CircuitIdentifier>& getValues() const noexcept { return mValues; }

  // Operator Overloadings
  CircuitIdentifierImportDialog& operator=(
      const CircuitIdentifierImportDialog& rhs) = delete;

private:  // Methods
  void updatePlaceholder() noexcept;
  void checkClipboard() noexcept;
  void parseInput() noexcept;
  void autoDetectFilterColumn(const QStringList& lines) noexcept;
  bool columnContainsOnlyNumbers(const QList<QStringList>& data,
                                 int col) const noexcept;

private:  // Data
  QScopedPointer<Ui::CircuitIdentifierImportDialog> mUi;
  const QString mSettingsPrefix;
  const QRegularExpression mSpaceRegex;
  QList<CircuitIdentifier> mValues;
  QString mLastClipboardValue;
  int mFilterColumnIndex;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
