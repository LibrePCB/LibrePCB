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
#include "viashapeselector.h"

#include <librepcb/common/widgets/positivelengthedit.h>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace project {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

ViaShapeSelector::ViaShapeSelector(QWidget* parent) noexcept
  : QToolBar(parent) {
  // shape buttons
  QButtonGroup* buttonGroup(new QButtonGroup(this));
  buttonGroup->setExclusive(true);

  for (int i = 0; i < (int)BI_Via::Shape::Count; ++i) {
    BI_Via::Shape shape  = (BI_Via::Shape)i;
    QToolButton*  button = new QToolButton(this);

    button->setCheckable(true);
    button->setAutoRaise(true);
    button->setIconSize(QSize(24, 24));
    addWidget(button);
    buttonGroup->addButton(button, i);

    mButtons[shape] = button;
  }

  mButtons[BI_Via::Shape::Round]->setIcon(
      QIcon(":/img/command_toolbars/via_round.png"));
  mButtons[BI_Via::Shape::Round]->setToolTip(tr("Round"));
  mButtons[BI_Via::Shape::Square]->setIcon(
      QIcon(":/img/command_toolbars/via_square.png"));
  mButtons[BI_Via::Shape::Square]->setToolTip(tr("Square"));
  mButtons[BI_Via::Shape::Octagon]->setIcon(
      QIcon(":/img/command_toolbars/via_octagon.png"));
  mButtons[BI_Via::Shape::Octagon]->setToolTip(tr("Octagon"));

  // Note(5n8ke): idClicked is only available with Qt 5.15
  // Note(5n8ke): QOverload is only available with Qt 5.7
#if (QT_VERSION >= QT_VERSION_CHECK(5, 15, 0))
  connect(buttonGroup, &QButtonGroup::idClicked, [this](int shape) {
#elif (QT_VERSION >= QT_VERSION_CHECK(5, 7, 0))
  connect(buttonGroup, QOverload<int>::of(&QButtonGroup::buttonClicked),
          [this](int shape) {
#else
  connect(
      buttonGroup,
      static_cast<void (QButtonGroup::*)(int)>(&QButtonGroup::buttonClicked),
      [this](int shape) {
#endif
    Q_ASSERT(mButtons.size() == (int)BI_Via::Shape::Count);
    emit shapeChanged((BI_Via::Shape)shape);
  });

  // size selection
  QLabel* sizeLabel = new QLabel(tr("Size:"), this);
  sizeLabel->setIndent(10);
  addWidget(sizeLabel);
  mSizeEdit = new PositiveLengthEdit(this);
  addWidget(mSizeEdit);
  connect(mSizeEdit, &PositiveLengthEdit::valueChanged,
          [this](const PositiveLength& value) { emit sizeChanged(value); });

  // drill selection
  QLabel* drillLabel = new QLabel(tr("Drill:"), this);
  drillLabel->setIndent(10);
  addWidget(drillLabel);
  mDrillEdit = new PositiveLengthEdit(this);
  addWidget(mDrillEdit);
  connect(mDrillEdit, &PositiveLengthEdit::valueChanged,
          [this](const PositiveLength& value) { emit drillChanged(value); });
}

void ViaShapeSelector::setShape(const BI_Via::Shape shape) noexcept {
  Q_ASSERT(mButtons.size() == (int)BI_Via::Shape::Count);
  if (mButtons.contains(shape)) {
    mButtons[shape]->click();
  }
}

void ViaShapeSelector::setSize(const PositiveLength& size) noexcept {
  mSizeEdit->setValue(size);
}

void ViaShapeSelector::stepSize(const int steps) noexcept {
  mSizeEdit->stepBy(steps);
}

void ViaShapeSelector::setDrill(const PositiveLength& drill) noexcept {
  mDrillEdit->setValue(drill);
}

void ViaShapeSelector::stepDrill(const int steps) noexcept {
  mDrillEdit->stepBy(steps);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace project
}  // namespace librepcb
