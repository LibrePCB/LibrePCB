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
#include "pcbcolor.h"

#include "../exceptions.h"
#include "../serialization/sexpression.h"

#include <QtCore>
#include <QtGui>

#include <algorithm>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

PcbColor::PcbColor(const QString& id, const QString& nameTr, Flags flags,
                   const QColor& solderResistColor,
                   const QColor& silkscreenColor) noexcept
  : mId(id),
    mNameTr(nameTr),
    mFlags(flags),
    mSolderResistColor(solderResistColor),
    mSilkscreenColor(silkscreenColor) {
}

PcbColor::PcbColor(const PcbColor& other) noexcept
  : mId(other.mId),
    mNameTr(other.mNameTr),
    mFlags(other.mFlags),
    mSolderResistColor(other.mSolderResistColor),
    mSilkscreenColor(other.mSilkscreenColor) {
}

PcbColor::~PcbColor() noexcept {
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

const QColor& PcbColor::toSolderResistColor() const noexcept {
  return mSolderResistColor.isValid() ? mSolderResistColor
                                      : green().toSolderResistColor();
}

const QColor& PcbColor::toSilkscreenColor() const noexcept {
  return mSilkscreenColor.isValid() ? mSilkscreenColor
                                    : white().toSilkscreenColor();
}

/*******************************************************************************
 *  Static Methods
 ******************************************************************************/

const PcbColor& PcbColor::black() noexcept {
  static PcbColor obj("black", tr("Black"),
                      Flag::SolderResist | Flag::Silkscreen,
                      QColor(0, 0, 0, 210), QColor(40, 40, 40));
  return obj;
}

const PcbColor& PcbColor::blackMatte() noexcept {
  static PcbColor obj("black_matte", tr("Black Matte"), Flag::SolderResist,
                      QColor(0, 0, 0, 210), Qt::black);
  return obj;
}

const PcbColor& PcbColor::blue() noexcept {
  static PcbColor obj("blue", tr("Blue"), Flag::SolderResist | Flag::Silkscreen,
                      QColor(0, 20, 100, 220), QColor(0, 0, 110));
  return obj;
}

const PcbColor& PcbColor::clear() noexcept {
  static PcbColor obj("clear", tr("Clear"), Flags(/* not available (yet) */),
                      QColor(50, 50, 50, 50), Qt::white);
  return obj;
}

const PcbColor& PcbColor::green() noexcept {
  static PcbColor obj("green", tr("Green"), Flag::SolderResist,
                      QColor(0, 50, 0, 180), Qt::green);
  return obj;
}

const PcbColor& PcbColor::greenMatte() noexcept {
  static PcbColor obj("green_matte", tr("Green Matte"), Flag::SolderResist,
                      QColor(0, 50, 0, 180), Qt::green);
  return obj;
}

const PcbColor& PcbColor::purple() noexcept {
  static PcbColor obj("purple", tr("Purple"), Flag::SolderResist,
                      QColor(80, 0, 130, 180), QColor(100, 0, 160));
  return obj;
}

const PcbColor& PcbColor::red() noexcept {
  static PcbColor obj("red", tr("Red"), Flag::SolderResist | Flag::Silkscreen,
                      QColor(160, 0, 0, 180), QColor(140, 0, 0));
  return obj;
}

const PcbColor& PcbColor::white() noexcept {
  static PcbColor obj("white", tr("White"),
                      Flag::SolderResist | Flag::Silkscreen,
                      QColor(220, 220, 220, 210), Qt::white);
  return obj;
}

const PcbColor& PcbColor::yellow() noexcept {
  static PcbColor obj("yellow", tr("Yellow"),
                      Flag::SolderResist | Flag::Silkscreen,
                      QColor(220, 220, 0, 160), QColor(210, 210, 0));
  return obj;
}

const PcbColor& PcbColor::other() noexcept {
  static PcbColor obj("other", tr("Other"),
                      Flag::SolderResist | Flag::Silkscreen, QColor(),
                      QColor());
  return obj;
}

const QVector<const PcbColor*>& PcbColor::all() noexcept {
  auto init = []() {
    // Sort the list by function and name.
    QVector<const PcbColor*> l{
        &black(),  //
        &blackMatte(),  //
        &blue(),  //
        &clear(),  //
        &green(),  //
        &greenMatte(),  //
        &purple(),  //
        &red(),  //
        &white(),  //
        &yellow(),  //
    };
    std::sort(l.begin(), l.end(), [](const PcbColor* a, const PcbColor* b) {
      return a->mNameTr < b->mNameTr;
    });
    l.append(&other());
    return l;
  };
  static QVector<const PcbColor*> list = init();  // Thread-safe initialization.
  return list;
}

const PcbColor& PcbColor::get(const QString& id) {
  foreach (const PcbColor* PcbColor, all()) {
    if (PcbColor->getId() == id) {
      return *PcbColor;
    }
  }
  throw RuntimeError(__FILE__, __LINE__,
                     QString("Unknown color: '%1'").arg(id));
}

/*******************************************************************************
 *  Non-Member Functions
 ******************************************************************************/

template <>
std::unique_ptr<SExpression> serialize(const PcbColor& obj) {
  return SExpression::createToken(obj.getId());
}

template <>
std::unique_ptr<SExpression> serialize(const PcbColor* const& obj) {
  if (obj) {
    return SExpression::createToken(obj->getId());
  } else {
    return SExpression::createToken("none");
  }
}

template <>
const PcbColor& deserialize(const SExpression& node) {
  return PcbColor::get(node.getValue());
}

template <>
const PcbColor* deserialize(const SExpression& node) {
  const QString value = node.getValue();
  if (value == "none") {
    return nullptr;
  } else {
    return &PcbColor::get(node.getValue());
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
