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

#ifndef LIBREPCB_ALIGNMENT_H
#define LIBREPCB_ALIGNMENT_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "fileio/serializableobject.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class HAlign
 ******************************************************************************/

/**
 * @brief The HAlign class
 */
class HAlign final {
  Q_DECLARE_TR_FUNCTIONS(HAlign)

public:
  HAlign() noexcept : mAlign(Qt::AlignLeft) {}
  HAlign(const HAlign& other) noexcept : mAlign(other.mAlign) {}
  Qt::AlignmentFlag toQtAlignFlag() const noexcept { return mAlign; }
  HAlign& mirror() noexcept;
  HAlign mirrored() const noexcept { return HAlign(*this).mirror(); }
  static HAlign left() noexcept { return HAlign(Qt::AlignLeft); }
  static HAlign center() noexcept { return HAlign(Qt::AlignHCenter); }
  static HAlign right() noexcept { return HAlign(Qt::AlignRight); }
  HAlign& operator=(const HAlign& rhs) noexcept {
    mAlign = rhs.mAlign;
    return *this;
  }
  bool operator==(const HAlign& rhs) const noexcept {
    return mAlign == rhs.mAlign;
  }
  bool operator!=(const HAlign& rhs) const noexcept {
    return mAlign != rhs.mAlign;
  }

private:
  explicit HAlign(Qt::AlignmentFlag align) noexcept : mAlign(align) {}

  Qt::AlignmentFlag mAlign;
};

/*******************************************************************************
 *  HAlign Non-Member Functions
 ******************************************************************************/

template <>
inline SExpression serialize(const HAlign& obj) {
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
inline HAlign deserialize(const SExpression& sexpr) {
  QString str = sexpr.getValue();
  if (str == "left")
    return HAlign::left();
  else if (str == "center")
    return HAlign::center();
  else if (str == "right")
    return HAlign::right();
  else {
    throw RuntimeError(
        __FILE__, __LINE__,
        HAlign::tr("Invalid horizontal alignment: \"%1\"").arg(str));
  }
}

/*******************************************************************************
 *  Class VAlign
 ******************************************************************************/

/**
 * @brief The VAlign class
 */
class VAlign final {
  Q_DECLARE_TR_FUNCTIONS(VAlign)

public:
  VAlign() noexcept : mAlign(Qt::AlignTop) {}
  VAlign(const VAlign& other) noexcept : mAlign(other.mAlign) {}
  Qt::AlignmentFlag toQtAlignFlag() const noexcept { return mAlign; }
  VAlign& mirror() noexcept;
  VAlign mirrored() const noexcept { return VAlign(*this).mirror(); }
  static VAlign top() noexcept { return VAlign(Qt::AlignTop); }
  static VAlign center() noexcept { return VAlign(Qt::AlignVCenter); }
  static VAlign bottom() noexcept { return VAlign(Qt::AlignBottom); }
  VAlign& operator=(const VAlign& rhs) noexcept {
    mAlign = rhs.mAlign;
    return *this;
  }
  bool operator==(const VAlign& rhs) const noexcept {
    return mAlign == rhs.mAlign;
  }
  bool operator!=(const VAlign& rhs) const noexcept {
    return mAlign != rhs.mAlign;
  }

private:
  explicit VAlign(Qt::AlignmentFlag align) noexcept : mAlign(align) {}

  Qt::AlignmentFlag mAlign;
};

/*******************************************************************************
 *  VAlign Non-Member Functions
 ******************************************************************************/

template <>
inline SExpression serialize(const VAlign& obj) {
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
inline VAlign deserialize(const SExpression& sexpr) {
  QString str = sexpr.getValue();
  if (str == "top")
    return VAlign::top();
  else if (str == "center")
    return VAlign::center();
  else if (str == "bottom")
    return VAlign::bottom();
  else {
    throw RuntimeError(
        __FILE__, __LINE__,
        VAlign::tr("Invalid vertical alignment: \"%1\"").arg(str));
  }
}

/*******************************************************************************
 *  Class Alignment
 ******************************************************************************/

/**
 * @brief The Alignment class
 */
class Alignment final : public SerializableObject {
  Q_DECLARE_TR_FUNCTIONS(Alignment)

public:
  Alignment() noexcept : mH(HAlign::left()), mV(VAlign::bottom()) {}
  Alignment(const Alignment& other) noexcept : mH(other.mH), mV(other.mV) {}
  explicit Alignment(const HAlign& h, const VAlign& v) noexcept
    : mH(h), mV(v) {}
  explicit Alignment(const SExpression& node);
  const HAlign getH() const noexcept { return mH; }
  const VAlign getV() const noexcept { return mV; }
  void setH(const HAlign& h) noexcept { mH = h; }
  void setV(const VAlign& v) noexcept { mV = v; }
  Qt::Alignment toQtAlign() const noexcept {
    return mH.toQtAlignFlag() | mV.toQtAlignFlag();
  }
  Alignment& mirror() noexcept;
  Alignment& mirrorH() noexcept;
  Alignment& mirrorV() noexcept;
  Alignment mirrored() const noexcept { return Alignment(*this).mirror(); }
  Alignment mirroredH() const noexcept { return Alignment(*this).mirrorH(); }
  Alignment mirroredV() const noexcept { return Alignment(*this).mirrorV(); }

  /// @copydoc librepcb::SerializableObject::serialize()
  void serialize(SExpression& root) const override;

  Alignment& operator=(const Alignment& rhs) noexcept {
    mH = rhs.mH;
    mV = rhs.mV;
    return *this;
  }
  bool operator==(const Alignment& rhs) const noexcept {
    return mH == rhs.mH && mV == rhs.mV;
  }
  bool operator!=(const Alignment& rhs) const noexcept {
    return mH != rhs.mH || mV != rhs.mV;
  }

private:
  HAlign mH;
  VAlign mV;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

Q_DECLARE_METATYPE(librepcb::HAlign)
Q_DECLARE_METATYPE(librepcb::VAlign)

#endif  // LIBREPCB_ALIGNMENT_H
