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
#include "dxfreader.h"

#include "../fileio/filepath.h"

#include <dl_creationadapter.h>
#include <dl_dxf.h>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class DxfReaderImpl
 ******************************************************************************/

/**
 * @brief Private helper class to break dependency to dxflib
 *
 * Having this class in the *.cpp file avoid including dxflib headers in our
 * own header file. This way, dxflib only needs to be in the include path for
 * the librepcb common library, but not for any other library or application.
 *
 * Or in other words: This way, the dependency to dxflib is an implementation
 * detail which other parts of the code base do not have to care about.
 */
class DxfReaderImpl : public DL_CreationAdapter {
public:
  DxfReaderImpl(DxfReader& reader)
    : mReader(reader),
      mScaleToMm(1),
      mPolylineClosed(false),
      mPolylineVertices(0),
      mPolylinePath() {}

  virtual ~DxfReaderImpl() {}

  virtual void addPoint(const DL_PointData& data) override {
    mReader.mPoints.append(point(data.x, data.y));
  }

  virtual void addLine(const DL_LineData& data) override {
    mReader.mPolygons.append(
        Path::line(point(data.x1, data.y1), point(data.x2, data.y2)));
  }

  virtual void addArc(const DL_ArcData& data) override {
    Point center = point(data.cx, data.cy);
    Length radius = length(data.radius);
    Angle angle1 = angle(data.angle1);
    Angle angle2 = angle(data.angle2);

    Point p1 = center + Point(radius, 0).rotated(angle1);
    Point p2 = center + Point(radius, 0).rotated(angle2);
    Angle angle = angle2 - angle1;
    if (angle < 0) {
      angle.invert();
    }
    mReader.mPolygons.append(Path::line(p1, p2, angle));
  }

  virtual void addCircle(const DL_CircleData& data) override {
    Length diameter = length(data.radius * 2);
    if (diameter > 0) {
      mReader.mCircles.append(
          DxfReader::Circle{point(data.cx, data.cy), PositiveLength(diameter)});
    } else {
      qWarning() << "Circle in DXF file ignored due to invalid radius:"
                 << data.radius;
    }
  }

  virtual void addEllipse(const DL_EllipseData& data) override {
    Q_UNUSED(data);
    qWarning() << "Ellipse in DXF file ignored since it is not supported yet.";
  }

  virtual void addPolyline(const DL_PolylineData& data) override {
    mPolylineClosed = (data.flags & DL_CLOSED_PLINE) != 0;
    mPolylineVertices = data.number;
    mPolylinePath = Path();
  }

  virtual void addVertex(const DL_VertexData& data) override {
    mPolylinePath.addVertex(point(data.x, data.y), bulgeToAngle(data.bulge));
    if (mPolylinePath.getVertices().count() == mPolylineVertices) {
      endSequence();
    }
  }

  virtual void endSequence() override {
    if (mPolylinePath.getVertices().count() >= 2) {
      if (mPolylineClosed && (mPolylinePath.getVertices().count() >= 3)) {
        mPolylinePath.close();
      }
      mReader.mPolygons.append(mPolylinePath);
    }
    mPolylinePath = Path();
  }

  virtual void setVariableInt(const std::string& key, int value,
                              int code) override {
    if ((key == "$INSUNITS") && (code == 70)) {
      // Unit specified in DXF, use corresponding conversion scaling factors.
      QHash<int, qreal> map = {
          {0, 1},  // unspecified -> consider as millimeters, the only real unit
          {1, 25.4},  // inches
          {2, 304.8},  // feet
          {4, 1},  // millimeters
          {5, 10},  // centimeters
          {6, 1000},  // meters
          {8, 2.54e-5},  // microinches
          {9, 0.0254},  // mils
          {10, 914.4},  // yards
          {11, 1.0e-7},  // angstroms
          {12, 1.0e-6},  // nanometers
          {13, 1.0e-3},  // micrometers
          {14, 100},  // decimeters
      };
      mScaleToMm = map.value(value, 1);  // use millimeters if unit not found
    }
  }

private:  // Methods
  Angle angle(double angle) const { return Angle::fromDeg(angle); }
  Angle bulgeToAngle(double bulge) const {
    // Round to 0.001° to avoid odd numbers like 179.999999°.
    return Angle::fromRad(std::atan(bulge) * 4).rounded(Angle(1000));
  }
  Point point(double x, double y) const {
    return Point(length(x), length(y));  // can (theoretically) throw
  }
  Length length(double value) const {
    return Length::fromMm(value * mScaleToMm *
                          mReader.mScaleFactor);  // can (theoretically) throw
  }

private:  // Data
  DxfReader& mReader;
  qreal mScaleToMm;

  // Current polygon state
  bool mPolylineClosed;
  int mPolylineVertices;
  Path mPolylinePath;
};

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

DxfReader::DxfReader() noexcept : mScaleFactor(1) {
}

DxfReader::~DxfReader() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void DxfReader::parse(const FilePath& dxfFile) {
  try {
    DL_Dxf dxf;
    DxfReaderImpl helper(*this);
    if (!dxf.in(dxfFile.toNative().toStdString(), &helper)) {
      throw RuntimeError(__FILE__, __LINE__,
                         tr("File does not exist or is not readable."));
    }
  } catch (const std::exception& e) {
    // Since a third party library was used, catch std::exception and convert
    // it to our own exception type.
    throw RuntimeError(__FILE__, __LINE__,
                       tr("Failed to read DXF file \"%1\": %2")
                           .arg(dxfFile.toNative(), e.what()));
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
