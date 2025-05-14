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

#ifndef LIBREPCB_EDITOR_OPENGLSCENEBUILDER_H
#define LIBREPCB_EDITOR_OPENGLSCENEBUILDER_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "openglobject.h"

#include <librepcb/core/3d/scenedata3d.h>
#include <polyclipping/clipper.hpp>

#include <QtCore>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

class OpenGlTriangleObject;

/*******************************************************************************
 *  Class OpenGlSceneBuilder
 ******************************************************************************/

/**
 * @brief Asynchronously generates a 3D board scene for OpenGL rendering
 */
class OpenGlSceneBuilder final : public QObject {
  Q_OBJECT

public:
  // Types
  typedef std::tuple<qreal, qreal, qreal> Color;
  typedef QMap<Color, QVector<QVector3D>> StepModel;

  // Constructors / Destructor
  OpenGlSceneBuilder(QObject* parent = nullptr) noexcept;
  OpenGlSceneBuilder(const OpenGlSceneBuilder& other) = delete;
  ~OpenGlSceneBuilder() noexcept;

  // General Methods

  /**
   * @brief Start building scene asynchronously
   */
  void start(std::shared_ptr<SceneData3D> data) noexcept;

  /**
   * @brief Check if there is currently a build in progress
   *
   * @retval true Build in progress.
   * @retval false Idle.
   */
  bool isBusy() const noexcept;

  /**
   * @brief Wait (block) until the build is finished
   */
  void waitForFinished() noexcept;

  /**
   * @brief Cancel the build
   */
  void cancel() noexcept;

  // Operator Overloadings
  OpenGlSceneBuilder& operator=(const OpenGlSceneBuilder& rhs) = delete;

signals:
  void started();
  void finished(QStringList errors);
  void objectAdded(std::shared_ptr<librepcb::editor::OpenGlObject> obj);
  void objectRemoved(std::shared_ptr<librepcb::editor::OpenGlObject> obj);
  void objectUpdated(std::shared_ptr<librepcb::editor::OpenGlObject> obj);

private:  // Methods
  void run(std::shared_ptr<SceneData3D> data) noexcept;
  ClipperLib::Paths getPaths(const std::shared_ptr<SceneData3D>& data,
                             const QStringList layers) const;
  QVector<QVector3D> extrude(const ClipperLib::Paths& paths, qreal z,
                             qreal height, qreal scaleFactor, bool faces = true,
                             bool edges = true, bool closed = true) const;
  static QVector<QVector3D> tesselate(const ClipperLib::Path& path, qreal z,
                                      qreal scaleFactor);
  void publishTriangleData(const QString& id, OpenGlObject::Type type,
                           const QColor& color,
                           const QVector<QVector3D>& triangles);
  void publishDevice(const SceneData3D::DeviceData& obj,
                     const QByteArray& stepContent, qreal z, qreal scaleFactor,
                     qreal alpha);

private:  // Data
  const PositiveLength mMaxArcTolerance;
  QFuture<void> mFuture;
  bool mAbort;

  // Thread data.
  QHash<QString, std::shared_ptr<OpenGlTriangleObject>> mBoardObjects;
  QHash<Uuid, QMap<Color, std::shared_ptr<OpenGlTriangleObject>>> mDevices;
  QHash<QByteArray, StepModel> mStepModels;  ///< Cache
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
