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
  for (auto it = mApertures.constBegin(); it != mApertures.constEnd(); ++it) {
    // Replace placeholders "{}" by the aperture number.
    str.append(QString(it.value()).replace("{}", QString::number(it.key())));
  }
  return str;
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

int GerberApertureList::addCircle(const UnsignedLength& dia) {
  return addAperture(QString("%ADD{}C,%1*%\n").arg(dia->toMmString()));
}

int GerberApertureList::addObround(const PositiveLength& w,
                                   const PositiveLength& h,
                                   const Angle& rot) noexcept {
  if (w == h) {
    // For maximum compatibility, use a circle if width==height.
    return addCircle(positiveToUnsigned(w));
  } else if (rot % Angle::deg180() == 0) {
    return addAperture(
        QString("%ADD{}O,%1X%2*%\n").arg(w->toMmString(), h->toMmString()));
  } else if (rot % Angle::deg90() == 0) {
    return addAperture(
        QString("%ADD{}O,%1X%2*%\n").arg(h->toMmString(), w->toMmString()));
  } else if (w < h) {
    // Same as condition below, but swap width and height and rotate by 90° to
    // simplify calculations and to merge all combinations of parameters
    // leading in the same image.
    return addObround(h, w, rot + Angle::deg90());
  } else {
    // Rotation is not a multiple of 90 degrees --> we need to use an aperture
    // macro.

    // Normalize the rotation to a range of 0..180° to avoid generating
    // multiple different apertures which represent exactly the same image.
    Angle uniqueRotatation = rot.mappedTo0_360deg() % Angle::deg180();
    QString s = "%AMROTATEDOBROUND{}";
    Point start = Point(-w / 2 + h / 2, 0).rotated(uniqueRotatation);
    Point end = Point(w / 2 - h / 2, 0).rotated(uniqueRotatation);
    // ATTENTION: Don't use the optional rotation parameter in the circles!
    // It causes critical issues with some crappy CAM software!
    s += QString("*1,1,%1,%2,%3")
             .arg(h->toMmString(), start.getX().toMmString(),
                  start.getY().toMmString());
    s += QString("*1,1,%1,%2,%3")
             .arg(h->toMmString(), end.getX().toMmString(),
                  end.getY().toMmString());
    s += QString("*20,1,%1,%2,%3,%4,%5,0*%\n")
             .arg(h->toMmString(), start.getX().toMmString(),
                  start.getY().toMmString(), end.getX().toMmString(),
                  end.getY().toMmString());
    s += "%ADD{}ROTATEDOBROUND{}*%\n";
    return addAperture(s);
  }
}

int GerberApertureList::addRect(const PositiveLength& w,
                                const PositiveLength& h,
                                const Angle& rot) noexcept {
  if (rot % Angle::deg180() == 0) {
    return addAperture(
        QString("%ADD{}R,%1X%2*%\n").arg(w->toMmString(), h->toMmString()));
  } else if (rot % Angle::deg90() == 0) {
    return addAperture(
        QString("%ADD{}R,%1X%2*%\n").arg(h->toMmString(), w->toMmString()));
  } else if (w < h) {
    // Same as condition below, but swap width and height and rotate by 90° to
    // simplify calculations and to merge all combinations of parameters
    // leading in the same image.
    return addRect(h, w, rot + Angle::deg90());
  } else {
    // Rotation is not a multiple of 90 degrees --> we need to use an aperture
    // macro. But don't use the "Center Line (Code 21)" since some Gerber
    // parsers interpret the rotation parameter in the wrong way! See Gerber
    // specs for details. Let's use the "Vector Line (Code 20)" macro instead.

    // Normalize the rotation to a range of 0..180° to avoid generating
    // multiple different apertures which represent exactly the same image.
    Angle uniqueRotatation = rot.mappedTo0_360deg() % Angle::deg180();
    QString s = "%AMROTATEDRECT{}";
    s += QString("*20,1,%1,%2,0.0,%3,0.0,%4*%\n")
             .arg(h->toMmString(), (-w / 2).toMmString(), (w / 2).toMmString(),
                  uniqueRotatation.toDegString());
    s += "%ADD{}ROTATEDRECT{}*%\n";
    return addAperture(s);
  }
}

int GerberApertureList::addOctagon(const PositiveLength& w,
                                   const PositiveLength& h,
                                   const Angle& rot) noexcept {
  // Note: If w==h, we could theoretically use the "Gegular Polygon (P)"
  // aperture. However, it seems some CAM software render such polygons the
  // wrong way. From the Gerber specs:
  //
  //     Some CAD systems incorrectly assume the parameter of a Regular Polygon
  //     specifies the inside diameter. This is wrong: it specifies the outside
  //     diameter.
  //
  // So let's always use an outline macro for octagons, probably this is more
  // compatible with CAM software.

  if (w < h) {
    // Same as condition below, but swap width and height and rotate by 90° to
    // simplify calculations and to merge all combinations of parameters
    // leading in the same image.
    return addOctagon(h, w, rot + Angle::deg90());
  } else {
    // Normalize the rotation to a range of 0..45° (w==h) resp. 0..180° (w!=h)
    // to avoid generating multiple different apertures which represent exactly
    // the same image.
    Angle rotationModulo = (w == h) ? Angle::deg45() : Angle::deg180();
    Angle uniqueRotatation = rot.mappedTo0_360deg() % rotationModulo;
    return addOutline("ROTATEDOCTAGON", Path::octagon(w, h), uniqueRotatation);
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

int GerberApertureList::addOutline(const QString& name, Path path,
                                   const Angle& rot) noexcept {
  path.close();
  Q_ASSERT(path.getVertices().count() >= 4);
  QString s =
      QString("%AM%1{}*4,1,%2,").arg(name).arg(path.getVertices().count() - 1);
  foreach (const Vertex& v, path.getVertices()) {
    Q_ASSERT(v.getAngle() == 0);
    s += QString("%1,%2,").arg(v.getPos().getX().toMmString(),
                               v.getPos().getY().toMmString());
  }
  s += QString("%1*%\n").arg(rot.toDegString());
  s += QString("%ADD{}%1{}*%\n").arg(name);
  return addAperture(s);
}

int GerberApertureList::addAperture(const QString& aperture) noexcept {
  int number = mApertures.key(aperture, -1);
  if (number < 0) {
    number = mApertures.count() + 10;  // 10 is the number of the first aperture
    Q_ASSERT(!mApertures.contains(number));
    mApertures.insert(number, aperture);
  }
  return number;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
