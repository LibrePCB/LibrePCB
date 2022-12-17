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
#include "holeeditorwidget.h"

#include "ui_holeeditorwidget.h"

#include <librepcb/core/geometry/hole.h>
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

HoleEditorWidget::HoleEditorWidget(QWidget* parent) noexcept
  : QWidget(parent),
    mUi(new Ui::HoleEditorWidget),
    mHole(Uuid::createRandom(), PositiveLength(1000000),
          makeNonEmptyPath(Point(0, 0))) {
  mUi->setupUi(this);
  mUi->pathEditorWidget->setFrameShape(QFrame::NoFrame);
  mUi->pathEditorWidget->setMinimumVertexCount(1);
  connect(mUi->edtDiameter, &PositiveLengthEdit::valueChanged, this,
          [this](const PositiveLength& value) {
            mHole.setDiameter(value);
            updateLinearOuterSize(*mHole.getPath());
            emit holeChanged(mHole);
          });
  connect(mUi->edtPosX, &LengthEdit::valueChanged, this,
          &HoleEditorWidget::updatePathFromCircularTab);
  connect(mUi->edtPosY, &LengthEdit::valueChanged, this,
          &HoleEditorWidget::updatePathFromCircularTab);
  connect(mUi->edtCenterX, &LengthEdit::valueChanged, this,
          &HoleEditorWidget::updatePathFromLinearTab);
  connect(mUi->edtCenterY, &LengthEdit::valueChanged, this,
          &HoleEditorWidget::updatePathFromLinearTab);
  connect(mUi->edtLength, &UnsignedLengthEdit::valueChanged, this,
          &HoleEditorWidget::updatePathFromLinearTab);
  connect(mUi->edtRotation, &AngleEdit::valueChanged, this,
          &HoleEditorWidget::updatePathFromLinearTab);
  connect(mUi->pathEditorWidget, &PathEditorWidget::pathChanged, this,
          [this](const Path& path) {
            if (!path.getVertices().isEmpty()) {
              mHole.setPath(NonEmptyPath(path));
              updateCircularTabFromPath(path);
              updateLinearTabFromPath(path);
              updateLinearOuterSize(path);
              emit holeChanged(mHole);
            }
          });
}

HoleEditorWidget::~HoleEditorWidget() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void HoleEditorWidget::setReadOnly(bool readOnly) noexcept {
  mUi->edtDiameter->setReadOnly(readOnly);
  mUi->edtPosX->setReadOnly(readOnly);
  mUi->edtPosY->setReadOnly(readOnly);
  mUi->edtCenterX->setReadOnly(readOnly);
  mUi->edtCenterY->setReadOnly(readOnly);
  mUi->edtLength->setReadOnly(readOnly);
  mUi->edtRotation->setReadOnly(readOnly);
  mUi->pathEditorWidget->setReadOnly(readOnly);
}

void HoleEditorWidget::setHole(const Hole& hole) noexcept {
  // Load attributes.
  mHole = hole;
  mUi->edtDiameter->setValue(mHole.getDiameter());
  mUi->pathEditorWidget->setPath(*mHole.getPath());

  // Open the most reasonable tab.
  if (mUi->tabCircular->isEnabled()) {
    mUi->tabWidget->setCurrentWidget(mUi->tabCircular);
  } else if (mUi->tabLinear->isEnabled()) {
    mUi->tabWidget->setCurrentWidget(mUi->tabLinear);
  } else {
    mUi->tabWidget->setCurrentWidget(mUi->tabArbitrary);
  }
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void HoleEditorWidget::setFocusToDiameterEdit() noexcept {
  mUi->edtDiameter->setFocus();
}

void HoleEditorWidget::configureClientSettings(
    const LengthUnit& lengthUnit, const QString& settingsPrefix) noexcept {
  mUi->edtDiameter->configure(lengthUnit,
                              LengthEditBase::Steps::drillDiameter(),
                              settingsPrefix % "/diameter");
  mUi->edtPosX->configure(lengthUnit, LengthEditBase::Steps::generic(),
                          settingsPrefix % "/pos_x");
  mUi->edtPosY->configure(lengthUnit, LengthEditBase::Steps::generic(),
                          settingsPrefix % "/pos_y");
  mUi->edtCenterX->configure(lengthUnit, LengthEditBase::Steps::generic(),
                             settingsPrefix % "/center_x");
  mUi->edtCenterY->configure(lengthUnit, LengthEditBase::Steps::generic(),
                             settingsPrefix % "/center_y");
  mUi->edtLength->configure(lengthUnit, LengthEditBase::Steps::generic(),
                            settingsPrefix % "/length");
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void HoleEditorWidget::updatePathFromCircularTab() noexcept {
  QSignalBlocker blocker(mUi->pathEditorWidget);

  const NonEmptyPath path(Path(
      {Vertex(Point(mUi->edtPosX->getValue(), mUi->edtPosY->getValue()))}));
  if (path != mHole.getPath()) {
    mHole.setPath(path);
    mUi->pathEditorWidget->setPath(*path);
    updateLinearTabFromPath(*path);
    updateLinearOuterSize(*path);
    emit holeChanged(mHole);
  }
}

void HoleEditorWidget::updatePathFromLinearTab() noexcept {
  QSignalBlocker blocker(mUi->pathEditorWidget);

  const Point center(mUi->edtCenterX->getValue(), mUi->edtCenterY->getValue());
  const UnsignedLength length = mUi->edtLength->getValue();
  const Angle rotation = mUi->edtRotation->getValue();

  const Point p1 = center + Point(length / 2, 0).rotated(rotation);
  const Point p2 = center + Point(length / -2, 0).rotated(rotation);
  Path path(Path({Vertex(p1)}));
  if (p2 != p1) {
    path.addVertex(p2);
  }

  if (path != *mHole.getPath()) {
    mHole.setPath(NonEmptyPath(path));
    mUi->pathEditorWidget->setPath(path);
    updateCircularTabFromPath(path);
    updateLinearOuterSize(path);
    emit holeChanged(mHole);
  }
}

void HoleEditorWidget::updateCircularTabFromPath(const Path& path) noexcept {
  // Avoid possible endless signal loop.
  QSignalBlocker blockPosX(mUi->edtPosX);
  QSignalBlocker blockPosY(mUi->edtPosY);

  const bool isCircular = (path.getVertices().count() == 1);
  mUi->tabWidget->setTabEnabled(mUi->tabWidget->indexOf(mUi->tabCircular),
                                isCircular);
  if (isCircular) {
    mUi->edtPosX->setValue(path.getVertices().first().getPos().getX());
    mUi->edtPosY->setValue(path.getVertices().first().getPos().getY());
  }
}

void HoleEditorWidget::updateLinearTabFromPath(const Path& path) noexcept {
  // Avoid possible endless signal loop.
  QSignalBlocker blockCenterX(mUi->edtCenterX);
  QSignalBlocker blockCenterY(mUi->edtCenterY);
  QSignalBlocker blockLength(mUi->edtLength);
  QSignalBlocker blockRotation(mUi->edtRotation);

  const bool isCircular = (path.getVertices().count() == 1);
  const bool isLinear = (path.getVertices().count() == 2) &&
      (path.getVertices().first().getAngle() == Angle::deg0());
  mUi->tabWidget->setTabEnabled(mUi->tabWidget->indexOf(mUi->tabLinear),
                                isCircular || isLinear);
  if (isCircular || isLinear) {
    const Point p1 = path.getVertices().first().getPos();
    const Point p2 = path.getVertices().last().getPos();
    const Point diff = p2 - p1;
    const Point center = (p1 + p2) / 2;
    const UnsignedLength length = (p2 - p1).getLength();
    const Angle rotation = isCircular
        ? Angle::deg0()
        : Angle::fromRad(
              std::atan2(diff.toMmQPointF().y(), diff.toMmQPointF().x()))
              .rounded(Angle(1000));
    mUi->edtCenterX->setValue(center.getX());
    mUi->edtCenterY->setValue(center.getY());
    mUi->edtLength->setValue(length);
    if ((rotation.mappedTo0_360deg() % Angle::deg180()) !=
        (mUi->edtRotation->getValue().mappedTo0_360deg() % Angle::deg180())) {
      mUi->edtRotation->setValue(rotation);
    }
  }
}

void HoleEditorWidget::updateLinearOuterSize(const Path& path) noexcept {
  QLocale locale;
  const PositiveLength diameter = mUi->edtDiameter->getValue();
  const PositiveLength length = path.getTotalStraightLength() + diameter;
  const LengthUnit& unit = mUi->edtLength->getDisplayedUnit();
  const int decimals = unit.getReasonableNumberOfDecimals();
  const qreal width = unit.convertToUnit(*length);
  const qreal height = unit.convertToUnit(*diameter);
  mUi->lblOuterSize->setText(
      tr("Outer Size:") %
      QString(" %1x%2%3")
          .arg(Toolbox::floatToString(width, decimals, locale))
          .arg(Toolbox::floatToString(height, decimals, locale))
          .arg(unit.toShortStringTr()));
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
