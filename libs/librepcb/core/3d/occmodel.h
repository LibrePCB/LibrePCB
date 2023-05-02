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

#ifndef LIBREPCB_CORE_OCCMODEL_H
#define LIBREPCB_CORE_OCCMODEL_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../types/angle.h"
#include "../types/length.h"
#include "../types/point.h"

#include <QtCore>
#include <QtGui>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class FilePath;
class Path;
class Transform;

/*******************************************************************************
 *  Class OccModel
 ******************************************************************************/

/**
 * @brief 3D model implemented with OpenCascade
 */
class OccModel final {
  Q_DECLARE_TR_FUNCTIONS(OccModel)

  struct Data;

public:
  // Types
  typedef std::tuple<qreal, qreal, qreal> Color;

  // Constructors / Destructor
  OccModel() noexcept = delete;
  OccModel(const OccModel& other) = delete;
  ~OccModel() noexcept;

  // General Methods
  void addToAssembly(const OccModel& model, const Point3D& pos,
                     const Angle3D& rot, const Transform& transform,
                     const QString& name);
  void saveAsStep(const QString& name, const FilePath& fp) const;
  QMap<Color, QVector<QVector3D>> tesselate() const;

  // Static Methods
  static bool isAvailable() noexcept;
  static std::unique_ptr<OccModel> createAssembly(const QString& name);
  static std::unique_ptr<OccModel> createBoard(const Path& outline,
                                               const QVector<Path>& holes,
                                               const PositiveLength& thickness,
                                               const QColor& color);
  static std::unique_ptr<OccModel> loadStep(const QByteArray content);
  static QByteArray minifyStep(const QByteArray& content);

  // Operator Overloadings
  OccModel& operator=(const OccModel& rhs) = delete;

private:  // Methods
  explicit OccModel(std::unique_ptr<Data> data);
  static void initOpenCascade();
  static QString cleanString(const QString& str);
  static void throwNotAvailable();

private:  // Data
  std::unique_ptr<Data> mImpl;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
