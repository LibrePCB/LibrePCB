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
#include "padshapeselector.h"

#include <librepcb/common/widgets/positivelengthedit.h>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace library {
namespace editor {

PadShapeSelector::PadShapeSelector(const LengthUnit defaultUnit,
                                   QWidget*         parent) noexcept
  : QToolBar(parent) {
  // add shape buttons
  QButtonGroup* buttonGroup(new QButtonGroup(this));
  buttonGroup->setExclusive(true);

  for (int i = 0; i < (int)FootprintPad::Shape::COUNT; ++i) {
    FootprintPad::Shape shape  = (FootprintPad::Shape)i;
    QToolButton*        button = new QToolButton(this);

    button->setCheckable(true);
    button->setAutoRaise(true);
    button->setIconSize(QSize(24, 24));
    addWidget(button);
    buttonGroup->addButton(button, i);

    mButtons[shape] = button;
  }
  Q_ASSERT(mButtons.size() == (int)FootprintPad::Shape::COUNT);

  mButtons[FootprintPad::Shape::ROUND]->setIcon(
      QIcon(":/img/command_toolbars/shape_round.png"));
  mButtons[FootprintPad::Shape::ROUND]->setToolTip(tr("Round"));
  mButtons[FootprintPad::Shape::RECT]->setIcon(
      QIcon(":/img/command_toolbars/shape_rect.png"));
  mButtons[FootprintPad::Shape::RECT]->setToolTip(tr("Rectangular"));
  mButtons[FootprintPad::Shape::OCTAGON]->setIcon(
      QIcon(":/img/command_toolbars/shape_octagon.png"));
  mButtons[FootprintPad::Shape::OCTAGON]->setToolTip(tr("Octagon"));

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
    Q_ASSERT(mButtons.size() == (int)FootprintPad::Shape::COUNT);
    emit shapeChanged((FootprintPad::Shape)shape);
  });

  // add size edit
  QLabel* widthLabel = new QLabel(tr("Width:"), this);
  widthLabel->setIndent(10);
  addWidget(widthLabel);

  mWidthEdit = new PositiveLengthEdit(this);
  mWidthEdit->configure(defaultUnit, LengthEditBase::Steps::generic(),
                        "package_editor/add_pads/width");
  addWidget(mWidthEdit);
  connect(mWidthEdit, &PositiveLengthEdit::valueChanged,
          [this](const PositiveLength& width) { emit widthChanged(width); });

  QLabel* heightLabel = new QLabel(tr("Height:"), this);
  heightLabel->setIndent(10);
  addWidget(heightLabel);

  mHeightEdit = new PositiveLengthEdit(this);
  mHeightEdit->configure(defaultUnit, LengthEditBase::Steps::generic(),
                         "package_editor/add_pads/height");
  connect(mHeightEdit, &PositiveLengthEdit::valueChanged,
          [this](const PositiveLength& height) { emit heightChanged(height); });
  addWidget(mHeightEdit);
}

void PadShapeSelector::setShape(const FootprintPad::Shape shape) noexcept {
  Q_ASSERT(mButtons.size() == (int)FootprintPad::Shape::COUNT);
  if (mButtons.contains(shape)) {
    mButtons[shape]->click();
  }
}

void PadShapeSelector::setWidth(const PositiveLength& width) noexcept {
  mWidthEdit->setValue(width);
}

void PadShapeSelector::setHeight(const PositiveLength& height) noexcept {
  mHeightEdit->setValue(height);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace library
}  // namespace librepcb
