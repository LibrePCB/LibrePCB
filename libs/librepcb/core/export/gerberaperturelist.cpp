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

#include "gerberattributewriter.h"

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
  GerberAttributeWriter attributeWriter;
  for (auto it = mApertures.constBegin(); it != mApertures.constEnd(); ++it) {
    // Set attributes.
    QList<GerberAttribute> attributes;
    if (Function function = it.value().first) {
      attributes.append(GerberAttribute::apertureFunction(*function));
    }
    str.append(attributeWriter.setAttributes(attributes));

    // Replace placeholders "{}" by the aperture number.
    QString definition = it.value().second;
    str.append(definition.replace("{}", QString::number(it.key())));
  }

  // Explicitly clear all attributes at the end of the aperture list to avoid
  // propagating attributes to the rest of the Gerber file!
  str.append(attributeWriter.setAttributes({}));

  return str;
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

int GerberApertureList::addCircle(const UnsignedLength& dia,
                                  Function function) {
  return addAperture(QString("%ADD{}C,%1*%\n").arg(dia->toMmString()),
                     function);
}

int GerberApertureList::addObround(const PositiveLength& w,
                                   const PositiveLength& h, const Angle& rot,
                                   Function function) noexcept {
  if (w == h) {
    // For maximum compatibility, use a circle if width==height.
    return addCircle(positiveToUnsigned(w), function);
  } else if (rot % Angle::deg180() == 0) {
    return addAperture(
        QString("%ADD{}O,%1X%2*%\n").arg(w->toMmString(), h->toMmString()),
        function);
  } else if (rot % Angle::deg90() == 0) {
    return addAperture(
        QString("%ADD{}O,%1X%2*%\n").arg(h->toMmString(), w->toMmString()),
        function);
  } else if (w < h) {
    // Same as condition below, but swap width and height and rotate by 90° to
    // simplify calculations and to merge all combinations of parameters
    // leading in the same image.
    return addObround(h, w, rot + Angle::deg90(), function);
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
    return addAperture(s, function);
  }
}

int GerberApertureList::addRect(const PositiveLength& w,
                                const PositiveLength& h,
                                const UnsignedLength& r, const Angle& rot,
                                Function function) noexcept {
  // Handle simple cases first.
  if ((r == 0) && (rot % Angle::deg180() == 0)) {
    return addAperture(
        QString("%ADD{}R,%1X%2*%\n").arg(w->toMmString(), h->toMmString()),
        function);
  } else if ((r == 0) && (rot % Angle::deg90() == 0)) {
    return addAperture(
        QString("%ADD{}R,%1X%2*%\n").arg(h->toMmString(), w->toMmString()),
        function);
  } else if (w < h) {
    // Swap width and height and rotate by 90° to simplify calculations and
    // to merge all combinations of parameters leading in the same image.
    return addRect(h, w, r, rot + Angle::deg90(), function);
  }

  // Normalize the rotation to a range of 0..90° (w==h) resp. 0..180° (w!=h)
  // to avoid generating multiple different apertures which represent exactly
  // the same image.
  const Angle rotationModulo = (w == h) ? Angle::deg90() : Angle::deg180();
  const Angle uniqueRotatation = rot.mappedTo0_360deg() % rotationModulo;

  // More complex cases --> we need to use an aperture macro. But don't use
  // the "Center Line (Code 21)" since some Gerber parsers interpret the
  // rotation parameter in the wrong way! See Gerber specs for details.
  // Let's use the "Vector Line (Code 20)" macro instead.
  if (r == 0) {
    // Corners are not rounded.
    QString s = "%AMROTATEDRECT{}";
    s += QString("*20,1,%1,%2,0.0,%3,0.0,%4*%\n")
             .arg(h->toMmString(), (-w / 2).toMmString(), (w / 2).toMmString(),
                  uniqueRotatation.toDegString());
    s += "%ADD{}ROTATEDRECT{}*%\n";
    return addAperture(s, function);
  } else if (r >= std::min(w, h) / 2) {
    // The radius is too large for the given size, it's actually an obround.
    return addObround(w, h, rot, function);
  } else {
    // Corners are rounded, build a macro with two rects and four circles.
    const QVector<Point> circlePositions = {
        Point(r - (w / 2), (h / 2) - r).rotated(uniqueRotatation),
        Point((w / 2) - r, (h / 2) - r).rotated(uniqueRotatation),
        Point((w / 2) - r, r - (h / 2)).rotated(uniqueRotatation),
        Point(r - (w / 2), r - (h / 2)).rotated(uniqueRotatation),
    };
    QString s = "%AMROUNDEDRECT{}*";
    s += QString("20,1,%1,%2,0.0,%3,0.0,%4*")
             .arg(h->toMmString(), (r - (w / 2)).toMmString(),
                  ((w / 2) - r).toMmString(), uniqueRotatation.toDegString());
    s += QString("20,1,%1,%2,0.0,%3,0.0,%4*")
             .arg((h - (r * 2)).toMmString(), (-w / 2).toMmString(),
                  (w / 2).toMmString(), uniqueRotatation.toDegString());
    foreach (const Point& p, circlePositions) {
      s += QString("1,1,%1,%2,%3*")
               .arg((r * 2).toMmString(), p.getX().toMmString(),
                    p.getY().toMmString());
    }
    s += "%\n";
    s += "%ADD{}ROUNDEDRECT{}*%\n";
    return addAperture(s, function);
  }
}

int GerberApertureList::addOctagon(const PositiveLength& w,
                                   const PositiveLength& h,
                                   const UnsignedLength& r, const Angle& rot,
                                   Function function) noexcept {
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

  // Normalize the rotation to a range of 0..45° (w==h) resp. 0..180° (w!=h)
  // to avoid generating multiple different apertures which represent exactly
  // the same image.
  const Angle rotationModulo = (w == h) ? Angle::deg45() : Angle::deg180();
  const Angle uniqueRotatation = rot.mappedTo0_360deg() % rotationModulo;
  const Length innerWidth = w - (r * 2);
  const Length innerHeight = h - (r * 2);

  if (w < h) {
    // Same as condition below, but swap width and height and rotate by 90° to
    // simplify calculations and to merge all combinations of parameters
    // leading in the same image.
    return addOctagon(h, w, r, rot + Angle::deg90(), function);
  } else if (r == 0) {
    return addOutline("ROTATEDOCTAGON", Path::octagon(w, h, r),
                      uniqueRotatation, function);
  } else if ((innerWidth <= 0) || (innerHeight <= 0)) {
    // The radius is too large for the given size, it's actually an obround.
    return addObround(w, h, rot, function);
  } else {
    // Corners are rounded, build a macro with four rects and eight circles.
    QString s = "%AMROUNDEDOCTAGON{}*";
    Path octagonWithoutArcs;
    foreach (const Vertex& v, Path::octagon(w, h, r).getVertices()) {
      octagonWithoutArcs.addVertex(v.getPos());
    }
    s += buildOutlineMacro(octagonWithoutArcs, uniqueRotatation);
    const Path innerOctagon =
        Path::octagon(PositiveLength(innerWidth), PositiveLength(innerHeight),
                      UnsignedLength(0))
            .rotated(uniqueRotatation);
    for (int i = 1; i < innerOctagon.getVertices().count(); ++i) {  // Skip [0]!
      const Point p = innerOctagon.getVertices().at(i).getPos();
      s += QString("1,1,%1,%2,%3*")
               .arg((r * 2).toMmString(), p.getX().toMmString(),
                    p.getY().toMmString());
    }
    s += "%\n";
    s += "%ADD{}ROUNDEDOCTAGON{}*%\n";
    return addAperture(s, function);
  }
}

int GerberApertureList::addOutline(const StraightAreaPath& path,
                                   const Angle& rot,
                                   Function function) noexcept {
  return addOutline("OUTLINE", *path, rot.mappedTo0_360deg(), function);
}

int GerberApertureList::addComponentMain() noexcept {
  // Note: The aperture shape, size and function is defined in the Gerber
  // specs, do not change them!
  return addCircle(UnsignedLength(300000),
                   GerberAttribute::ApertureFunction::ComponentMain);
}

int GerberApertureList::addComponentPin(bool isPin1) noexcept {
  // Note: The aperture shape, size and function is defined in the Gerber
  // specs, do not change them!
  if (isPin1) {
    return addAperture("%ADD{}P,0.36X4X0.0*%\n",
                       GerberAttribute::ApertureFunction::ComponentPin);
  } else {
    return addAperture("%ADD{}C,0*%\n",
                       GerberAttribute::ApertureFunction::ComponentPin);
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

int GerberApertureList::addOutline(const QString& name, const Path& path,
                                   const Angle& rot,
                                   Function function) noexcept {
  QString s = QString("%AM%1{}*").arg(name);
  s += buildOutlineMacro(path, rot);
  s += QString("%\n");
  s += QString("%ADD{}%1{}*%\n").arg(name);
  return addAperture(s, function);
}

QString GerberApertureList::buildOutlineMacro(Path path,
                                              const Angle& rot) const noexcept {
  path.close();
  Q_ASSERT(path.getVertices().count() >= 4);
  QString s = QString("4,1,%2,").arg(path.getVertices().count() - 1);
  foreach (const Vertex& v, path.getVertices()) {
    Q_ASSERT(v.getAngle() == 0);
    s += QString("%1,%2,").arg(v.getPos().getX().toMmString(),
                               v.getPos().getY().toMmString());
  }
  s += QString("%1*").arg(rot.toDegString());
  return s;
}

int GerberApertureList::addAperture(QString aperture,
                                    Function function) noexcept {
  auto value = std::make_pair(function, aperture);
  int number = mApertures.key(value, -1);
  if (number < 0) {
    number = mApertures.count() + 10;  // 10 is the number of the first aperture
    Q_ASSERT(!mApertures.contains(number));
    mApertures.insert(number, value);
  }
  return number;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
