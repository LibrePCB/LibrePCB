/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
 * http://librepcb.org/
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
#include "gerberaperturelist.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

GerberApertureList::GerberApertureList() noexcept {
}

GerberApertureList::~GerberApertureList() noexcept {
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

QString GerberApertureList::generateString() const noexcept {
  QString str;
  str.append("G04 --- APERTURE LIST BEGIN --- *\n");
  foreach (const QString& macro, mApertureMacros) {
    str.append(QString("%AM%1*%\n").arg(macro));
  }
  foreach (int number, mApertures.keys()) {
    str.append(
        QString("%ADD%1%2*%\n").arg(number).arg(mApertures.value(number)));
  }
  str.append("G04 --- APERTURE LIST END --- *\n");
  return str;
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

int GerberApertureList::setCircle(const UnsignedLength& dia,
                                  const UnsignedLength& hole) {
  return setCurrentAperture(generateCircle(dia, hole));
}

int GerberApertureList::setRect(const UnsignedLength& w,
                                const UnsignedLength& h, const Angle& rot,
                                const UnsignedLength& hole) noexcept {
  if (rot % Angle::deg180() == 0) {
    return setCurrentAperture(generateRect(w, h, hole));
  } else if (rot % Angle::deg90() == 0) {
    return setCurrentAperture(generateRect(h, w, hole));
  } else {
    // Rotation is not a multiple of 90 degrees --> we need to use an aperture
    // macro
    if (hole > 0) {
      addMacro(generateRotatedRectMacroWithHole());
    } else {
      addMacro(generateRotatedRectMacro());
    }
    return setCurrentAperture(generateRotatedRect(w, h, rot, hole));
  }
}

int GerberApertureList::setObround(const UnsignedLength& w,
                                   const UnsignedLength& h, const Angle& rot,
                                   const UnsignedLength& hole) noexcept {
  if (rot % Angle::deg180() == 0) {
    return setCurrentAperture(generateObround(w, h, hole));
  } else if (rot % Angle::deg90() == 0) {
    return setCurrentAperture(generateObround(h, w, hole));
  } else {
    // Rotation is not a multiple of 90 degrees --> we need to use an aperture
    // macro
    if (hole > 0) {
      addMacro(generateRotatedObroundMacroWithHole());
    } else {
      addMacro(generateRotatedObroundMacro());
    }
    return setCurrentAperture(generateRotatedObround(w, h, rot, hole));
  }
}

int GerberApertureList::setRegularPolygon(const UnsignedLength& dia, int n,
                                          const Angle&          rot,
                                          const UnsignedLength& hole) noexcept {
  if (n < 3 || n > 12) {
    qWarning() << "Gerber Export: Specified number of vertices not supported "
                  "by gerber specs:"
               << n;
  }
  // Adjust rotation as its interpretation differs between LibrePCB and Gerber
  // specs
  Angle grbRot = rot + (Angle::deg180() / (n > 0 ? n : 1));
  return setCurrentAperture(generateRegularPolygon(dia, n, grbRot, hole));
}

void GerberApertureList::reset() noexcept {
  // mApertureMacros.clear();
  mApertures.clear();
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

int GerberApertureList::setCurrentAperture(const QString& aperture) noexcept {
  int number = mApertures.key(aperture, -1);
  if (number < 0) {
    number = mApertures.count() + 10;  // 10 is the number of the first aperture
    Q_ASSERT(!mApertures.contains(number));
    mApertures.insert(number, aperture);
  }
  return number;
}

void GerberApertureList::addMacro(const QString& macro) noexcept {
  if (!mApertureMacros.contains(macro)) {
    mApertureMacros.append(macro);
  }
}

/*******************************************************************************
 *  Aperture Generator Methods
 ******************************************************************************/

QString GerberApertureList::generateCircle(
    const UnsignedLength& dia, const UnsignedLength& hole) noexcept {
  if (hole > 0) {
    return QString("C,%1X%2").arg(dia->toMmString(), hole->toMmString());
  } else {
    return QString("C,%1").arg(dia->toMmString());
  }
}

QString GerberApertureList::generateRect(const UnsignedLength& w,
                                         const UnsignedLength& h,
                                         const UnsignedLength& hole) noexcept {
  if (hole > 0) {
    return QString("R,%1X%2X%3")
        .arg(w->toMmString(), h->toMmString(), hole->toMmString());
  } else {
    return QString("R,%1X%2").arg(w->toMmString(), h->toMmString());
  }
}

QString GerberApertureList::generateObround(
    const UnsignedLength& w, const UnsignedLength& h,
    const UnsignedLength& hole) noexcept {
  if (hole > 0) {
    return QString("O,%1X%2X%3")
        .arg(w->toMmString(), h->toMmString(), hole->toMmString());
  } else {
    return QString("O,%1X%2").arg(w->toMmString(), h->toMmString());
  }
}

QString GerberApertureList::generateRegularPolygon(
    const UnsignedLength& dia, int n, const Angle& rot,
    const UnsignedLength& hole) noexcept {
  QString str = QString("P,%1X%2").arg(dia->toMmString()).arg(n);
  if (rot != 0 || hole > 0) str += QString("X%1").arg(rot.toDegString());
  if (hole > 0) str += QString("X%1").arg(hole->toMmString());
  return str;
}

QString GerberApertureList::generateRotatedRectMacro() {
  // parameters: width, height, rotation
  return QString("ROTATEDRECT*21,1,$1,$2,0,0,$3");
}

QString GerberApertureList::generateRotatedRectMacroWithHole() {
  // parameters: width, height, rotation, hole
  return QString("ROTATEDRECTWITHHOLE*21,1,$1,$2,0,0,$3*1,0,$4,0,0,$3");
}

QString GerberApertureList::generateRotatedObroundMacro() {
  // parameters: x1, y1, x2, y2, width
  return QString(
      "ROTATEDOBROUND*1,1,$5,$1,$2,0*1,1,$5,$3,$4,0*20,1,$5,$1,$2,$3,$4,0");
}

QString GerberApertureList::generateRotatedObroundMacroWithHole() {
  // parameters: x1, y1, x2, y2, width, hole
  return QString(
      "ROTATEDOBROUNDWITHHOLE*1,1,$5,$1,$2,0*1,1,$5,$3,$4,0*20,1,$5,$1,$2,$3,$"
      "4,0*1,0,$6,0,0,0");
}

QString GerberApertureList::generateRotatedRect(
    const UnsignedLength& w, const UnsignedLength& h, const Angle& rot,
    const UnsignedLength& hole) noexcept {
  if (hole > 0) {
    return QString("ROTATEDRECTWITHHOLE,%1X%2X%3X%4")
        .arg(w->toMmString(), h->toMmString(), rot.toDegString(),
             hole->toMmString());
  } else {
    return QString("ROTATEDRECT,%1X%2X%3")
        .arg(w->toMmString(), h->toMmString(), rot.toDegString());
  }
}

QString GerberApertureList::generateRotatedObround(
    const UnsignedLength& w, const UnsignedLength& h, const Angle& rot,
    const UnsignedLength& hole) noexcept {
  UnsignedLength width = (w < h ? w : h);
  Point          start = Point(-w / 2 + width / 2, 0).rotated(rot);
  Point          end   = Point(w / 2 - width / 2, 0).rotated(rot);
  if (hole > 0) {
    return QString("ROTATEDOBROUNDWITHHOLE,%1X%2X%3X%4X%5X%6")
        .arg(start.getX().toMmString(), start.getY().toMmString(),
             end.getX().toMmString(), end.getY().toMmString(),
             width->toMmString(), hole->toMmString());
  } else {
    return QString("ROTATEDOBROUND,%1X%2X%3X%4X%5")
        .arg(start.getX().toMmString(), start.getY().toMmString(),
             end.getX().toMmString(), end.getY().toMmString(),
             width->toMmString());
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
