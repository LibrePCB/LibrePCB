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
#include "centeredcheckbox.h"

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

CenteredCheckBox::CenteredCheckBox(QWidget* parent) noexcept
  : CenteredCheckBox(QString(), parent) {
}

CenteredCheckBox::CenteredCheckBox(const QString& text,
                                   QWidget* parent) noexcept
  : QWidget(parent), mCheckBox(nullptr) {
  QHBoxLayout* layout = new QHBoxLayout(this);
  layout->setAlignment(Qt::AlignCenter);
  layout->setContentsMargins(0, 0, 0, 0);
  mCheckBox = new QCheckBox(text, this);
  layout->addWidget(mCheckBox);
  connect(mCheckBox, &QCheckBox::toggled, this, &CenteredCheckBox::toggled);
  connect(mCheckBox, &QCheckBox::clicked, this, &CenteredCheckBox::clicked);
  connect(mCheckBox, &QCheckBox::stateChanged, this,
          &CenteredCheckBox::stateChanged);
}

CenteredCheckBox::~CenteredCheckBox() noexcept {
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
