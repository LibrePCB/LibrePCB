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

#ifndef LIBREPCB_EDITOR_CENTEREDCHECKBOX_H
#define LIBREPCB_EDITOR_CENTEREDCHECKBOX_H

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
 *  Class CenteredCheckBox
 ******************************************************************************/

/**
 * @brief The CenteredCheckBox class is a centered variant of the QCheckBox
 */
class CenteredCheckBox final : public QWidget {
  Q_OBJECT

public:
  // Constructors / Destructor
  explicit CenteredCheckBox(QWidget* parent = nullptr) noexcept;
  explicit CenteredCheckBox(const QString& text,
                            QWidget* parent = nullptr) noexcept;
  CenteredCheckBox(const CenteredCheckBox& other) = delete;
  ~CenteredCheckBox() noexcept;

  // Wrapper Methods
  void setText(const QString& text) noexcept { mCheckBox->setText(text); }
  bool isChecked() const noexcept { return mCheckBox->isChecked(); }
  void setChecked(bool checked) noexcept { mCheckBox->setChecked(checked); }

  // Operator Overloadings
  CenteredCheckBox& operator=(const CenteredCheckBox& rhs) = delete;

signals:
  void toggled(bool checked);
  void clicked(bool checked);
  void stateChanged(int state);

private:  // Data
  QCheckBox* mCheckBox;  // ownership by Qt's parent-child-mechanism
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
