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
#include "alignment.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class HAlign
 ******************************************************************************/

HAlign& HAlign::mirror() noexcept {
  switch (mAlign) {
    case Qt::AlignLeft:
      mAlign = Qt::AlignRight;
      break;
    case Qt::AlignRight:
      mAlign = Qt::AlignLeft;
      break;
    case Qt::AlignHCenter:
      break;
    default:
      Q_ASSERT(false);
      break;
  }
  return *this;
}

template <>
SExpression serialize(const VAlign& obj) {
  switch (obj.toQtAlignFlag()) {
    case Qt::AlignTop:
      return SExpression::createToken("top");
    case Qt::AlignVCenter:
      return SExpression::createToken("center");
    case Qt::AlignBottom:
      return SExpression::createToken("bottom");
    default:
      throw LogicError(__FILE__, __LINE__);
  }
}

template <>
HAlign deserialize(const SExpression& node) {
  const QString str = node.getValue();
  if (str == "left") {
    return HAlign::left();
  } else if (str == "center") {
    return HAlign::center();
  } else if (str == "right") {
    return HAlign::right();
  } else {
    throw RuntimeError(__FILE__, __LINE__,
                       QString("Invalid horizontal alignment: '%1'").arg(str));
  }
}

/*******************************************************************************
 *  Class VAlign
 ******************************************************************************/

VAlign& VAlign::mirror() noexcept {
  switch (mAlign) {
    case Qt::AlignTop:
      mAlign = Qt::AlignBottom;
      break;
    case Qt::AlignBottom:
      mAlign = Qt::AlignTop;
      break;
    case Qt::AlignVCenter:
      break;
    default:
      Q_ASSERT(false);
      break;
  }
  return *this;
}

template <>
SExpression serialize(const HAlign& obj) {
  switch (obj.toQtAlignFlag()) {
    case Qt::AlignLeft:
      return SExpression::createToken("left");
    case Qt::AlignHCenter:
      return SExpression::createToken("center");
    case Qt::AlignRight:
      return SExpression::createToken("right");
    default:
      throw LogicError(__FILE__, __LINE__);
  }
}

template <>
VAlign deserialize(const SExpression& node) {
  const QString str = node.getValue();
  if (str == "top") {
    return VAlign::top();
  } else if (str == "center") {
    return VAlign::center();
  } else if (str == "bottom") {
    return VAlign::bottom();
  } else {
    throw RuntimeError(__FILE__, __LINE__,
                       QString("Invalid vertical alignment: '%1'").arg(str));
  }
}

/*******************************************************************************
 *  Class Alignment
 ******************************************************************************/

Alignment::Alignment(const SExpression& node)
  : mH(deserialize<HAlign>(node.getChild("@0"))),
    mV(deserialize<VAlign>(node.getChild("@1"))) {
}

Alignment& Alignment::mirror() noexcept {
  mH.mirror();
  mV.mirror();
  return *this;
}

Alignment& Alignment::mirrorH() noexcept {
  mH.mirror();
  return *this;
}

Alignment& Alignment::mirrorV() noexcept {
  mV.mirror();
  return *this;
}

void Alignment::serialize(SExpression& root) const {
  root.appendChild(mH);
  root.appendChild(mV);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
