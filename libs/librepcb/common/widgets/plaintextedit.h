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

#ifndef LIBREPCB_PLAINTEXTEDIT_H
#define LIBREPCB_PLAINTEXTEDIT_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class PlainTextEdit
 ******************************************************************************/

/**
 * @brief The PlainTextEdit class is a customized QPlainTextEdit
 *
 * Differences compared to QPlainTextEdit:
 *   - New signal editingFinished() (equivalent of QLineEdit::editingFinished())
 */
class PlainTextEdit final : public QPlainTextEdit {
  Q_OBJECT

public:
  // Constructors / Destructor
  explicit PlainTextEdit(QWidget* parent = nullptr) noexcept;
  PlainTextEdit(const PlainTextEdit& other) = delete;
  ~PlainTextEdit() noexcept;

  // Operator Overloadings
  PlainTextEdit& operator=(const PlainTextEdit& rhs) = delete;

signals:
  void editingFinished();

private:  // Methods
  void focusInEvent(QFocusEvent* e) noexcept override;
  void focusOutEvent(QFocusEvent* e) noexcept override;

private:  // Data
  QString mPlainTextBeforeGettingFocus;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif  // LIBREPCB_PLAINTEXTEDIT_H
