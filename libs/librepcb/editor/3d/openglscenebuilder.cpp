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
#include "openglscenebuilder.h"

#include "opengltriangleobject.h"

#include <librepcb/core/3d/occmodel.h>
#include <librepcb/core/exceptions.h>
#include <librepcb/core/fileio/filesystem.h>
#include <librepcb/core/fileio/fileutils.h>
#include <librepcb/core/types/layer.h>
#include <librepcb/core/types/pcbcolor.h>
#include <librepcb/core/utils/clipperhelpers.h>
#include <librepcb/core/utils/scopeguard.h>
#include <librepcb/core/utils/toolbox.h>
#include <librepcb_build_env.h>

#include <QtConcurrent>
#include <QtCore>

#if USE_GLU
#ifdef __APPLE__
#include <OpenGL/glu.h>
#else
#include <GL/glu.h>
#endif
#endif

Q_DECLARE_METATYPE(std::shared_ptr<librepcb::editor::OpenGlObject>)

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

OpenGlSceneBuilder::OpenGlSceneBuilder(QObject* parent) noexcept
  : QObject(parent), mMaxArcTolerance(5000), mFuture(), mAbort(false) {
  qRegisterMetaType<std::shared_ptr<OpenGlObject>>();
}

OpenGlSceneBuilder::~OpenGlSceneBuilder() noexcept {
  cancel();
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void OpenGlSceneBuilder::start(std::shared_ptr<SceneData3D> data) noexcept {
  cancel();
#if (QT_VERSION_MAJOR >= 6)
  mFuture = QtConcurrent::run(&OpenGlSceneBuilder::run, this, data);
#else
  mFuture = QtConcurrent::run(this, &OpenGlSceneBuilder::run, data);
#endif
}

bool OpenGlSceneBuilder::isBusy() const noexcept {
  return (mFuture.isStarted() || mFuture.isRunning()) &&
      (!mFuture.isFinished()) && (!mFuture.isCanceled());
}

void OpenGlSceneBuilder::waitForFinished() noexcept {
  mFuture.waitForFinished();
}

void OpenGlSceneBuilder::cancel() noexcept {
  mAbort = true;
  mFuture.waitForFinished();
  mAbort = false;
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void OpenGlSceneBuilder::run(std::shared_ptr<SceneData3D> data) noexcept {
  // Note: This method is called from a different thread, thus be careful with
  //       calling other methods to only call thread-safe methods!

  QElapsedTimer timer;
  timer.start();
  qDebug() << "Start building board 3D scene in worker thread...";

  QString errorMsg;
  emit started();
  auto sg = scopeGuard([this, &errorMsg]() { emit finished(errorMsg); });

  try {
    // Preprocess the data.
    Length width;
    Length height;
    data->preprocess(true, false, &width, &height);
    const qreal scaleFactor =
        1.0 / std::max(std::max(width, height).toMm(), qreal(1));
    const qreal d = data->getThickness()->toMm() / 2;
    if (mAbort) return;

    // Show error if the board outline is invalid.
    if ((width <= 0) || (height <= 0)) {
      errorMsg = tr("The board outline is invalid. Please add exactly one "
                    "polygon on the '%1' layer and make sure it is closed. "
                    "For more information, check out the documentation.")
                     .arg(Layer::boardOutlines().getNameTr());
    }

    // Convert holes to areas.
    ClipperLib::Paths platedHoles =
        getPaths(data, {Layer::boardPlatedCutouts().getId()});
    ClipperLib::Paths nonPlatedHoles =
        getPaths(data, {Layer::boardCutouts().getId()});
    QHash<QString, ClipperLib::Paths> copperHoles;
    for (auto& hole : data->getHoles()) {
      const auto paths = ClipperHelpers::convert(
          hole.path->toOutlineStrokes(hole.diameter), mMaxArcTolerance);
      if (hole.copperLayer) {
        ClipperLib::Paths& holes = copperHoles[hole.copperLayer->getId()];
        holes.insert(holes.end(), paths.begin(), paths.end());
      } else if (hole.plated) {
        platedHoles.insert(platedHoles.end(), paths.begin(), paths.end());
      } else {
        nonPlatedHoles.insert(nonPlatedHoles.end(), paths.begin(), paths.end());
      }
    }
    ClipperLib::Paths allHoles = platedHoles;
    ClipperHelpers::unite(allHoles, nonPlatedHoles, ClipperLib::pftNonZero,
                          ClipperLib::pftNonZero);
    if (mAbort) return;

    // Board body.
    QStringList layers = {Layer::boardOutlines().getId()};
    const ClipperLib::Paths boardOutlines = getPaths(data, layers);
    std::unique_ptr<ClipperLib::PolyTree> tree = ClipperHelpers::subtractToTree(
        boardOutlines, allHoles, ClipperLib::pftNonZero,
        ClipperLib::pftNonZero);
    const ClipperLib::Paths boardArea = ClipperHelpers::flattenTree(*tree);
    tree = ClipperHelpers::subtractToTree(boardOutlines, allHoles,
                                          ClipperLib::pftNonZero,
                                          ClipperLib::pftNonZero, false);
    const ClipperLib::Paths boardEdges = ClipperHelpers::treeToPaths(*tree);
    publishTriangleData(
        Layer::boardOutlines().getId(), QColor(70, 80, 70),
        extrude(boardArea, -d, 2 * d, scaleFactor, true, false) +
            extrude(boardEdges, -d, 2 * d, scaleFactor, false, true, false));
    if (mAbort) return;

    // Plated holes.
    tree = ClipperHelpers::intersectToTree(platedHoles, boardOutlines,
                                           ClipperLib::pftNonZero,
                                           ClipperLib::pftNonZero, false);
    platedHoles = ClipperHelpers::treeToPaths(*tree);
    publishTriangleData(
        "pth", QColor(124, 104, 71),
        extrude(platedHoles, -d, 2 * d, scaleFactor, false, true, false));
    if (mAbort) return;

    // Non-plated holes.
    tree = ClipperHelpers::intersectToTree(nonPlatedHoles, boardOutlines,
                                           ClipperLib::pftNonZero,
                                           ClipperLib::pftNonZero, false);
    nonPlatedHoles = ClipperHelpers::treeToPaths(*tree);
    publishTriangleData(
        "npth", QColor(50, 50, 50),
        extrude(nonPlatedHoles, -d, 2 * d, scaleFactor, false, true, false));
    if (mAbort) return;

    for (bool top : {false, true}) {
      const Transform transform(Point(), Angle(), !top);
      const qreal side = top ? 1 : -1;

      // Copper.
      layers = QStringList{transform.map(Layer::topCopper()).getId()};
      ClipperLib::Paths copperArea = boardArea;
      if (copperHoles.contains(layers.first())) {
        ClipperHelpers::subtract(copperArea, copperHoles[layers.first()],
                                 ClipperLib::pftEvenOdd,
                                 ClipperLib::pftNonZero);
      }
      tree = ClipperHelpers::intersectToTree(copperArea, getPaths(data, layers),
                                             ClipperLib::pftEvenOdd,
                                             ClipperLib::pftNonZero);
      ClipperLib::Paths paths = ClipperHelpers::flattenTree(*tree);
      publishTriangleData(
          layers.first(), QColor(188, 156, 105),
          extrude(paths, (d - 0.001) * side, 0.035 * side, scaleFactor));
      if (mAbort) return;

      // Solder resist.
      layers = QStringList{transform.map(Layer::topStopMask()).getId(),
                           Layer::boardCutouts().getId(),
                           Layer::boardPlatedCutouts().getId()};
      ClipperLib::Paths solderResist;
      if (const PcbColor* color = data->getSolderResist()) {
        solderResist = boardOutlines;
        ClipperHelpers::subtract(solderResist, getPaths(data, layers),
                                 ClipperLib::pftEvenOdd,
                                 ClipperLib::pftNonZero);
        // Shrink the solder resist very slightly to give copper the higher
        // priority if copper edges and solder resist edges are exactly
        // overlapping (also avoids ugly rendering due to faces within the same
        // 3D plane).
        tree = ClipperHelpers::offsetToTree(solderResist, Length(-50),
                                            mMaxArcTolerance);
        solderResist = ClipperHelpers::flattenTree(*tree);
        publishTriangleData(layers.first(), color->toSolderResistColor(),
                            extrude(solderResist, (d + 0.001) * side,
                                    0.05 * side, scaleFactor));
      } else {
        publishTriangleData(layers.first(), Qt::transparent, {});
      }
      if (mAbort) return;

      // Solder paste.
      layers = QStringList{transform.map(Layer::topSolderPaste()).getId()};
      tree = ClipperHelpers::intersectToTree(boardArea, getPaths(data, layers),
                                             ClipperLib::pftEvenOdd,
                                             ClipperLib::pftNonZero);
      paths = ClipperHelpers::flattenTree(*tree);
      publishTriangleData(
          layers.first(), Qt::darkGray,
          extrude(paths, (d + 0.036) * side, 0.03 * side, scaleFactor));
      if (mAbort) return;

      // Silkscreen.
      layers = QStringList();
      foreach (const Layer* layer,
               top ? data->getSilkscreenLayersTop()
                   : data->getSilkscreenLayersBot()) {
        layers.append(layer->getId());
      }
      if (const PcbColor* color = data->getSilkscreen()) {
        tree = ClipperHelpers::intersectToTree(
            solderResist, getPaths(data, layers), ClipperLib::pftEvenOdd,
            ClipperLib::pftNonZero);
        paths = ClipperHelpers::flattenTree(*tree);
        publishTriangleData(
            transform.map(Layer::topLegend()).getId(),
            color->toSilkscreenColor(),
            extrude(paths, (d + 0.052) * side, 0.01 * side, scaleFactor));
      } else {
        publishTriangleData(transform.map(Layer::topLegend()).getId(),
                            Qt::transparent, {});
      }
      if (mAbort) return;
    }

    // Add/update devices.
    QSet<Uuid> deviceUuids;
    if (std::shared_ptr<FileSystem> fs = data->getFileSystem()) {
      for (const auto& obj : data->getDevices()) {
        const QByteArray content = fs->readIfExists(obj.stepFile);
        publishDevice(obj, content, d + 0.067, scaleFactor,
                      data->getStepAlphaValue());
        deviceUuids.insert(obj.uuid);
        if (mAbort) return;
      }
    }

    // Remove all no longer existing devices.
    foreach (const Uuid& uuid, Toolbox::toSet(mDevices.keys()) - deviceUuids) {
      foreach (auto obj, mDevices.take(uuid)) {
        emit objectRemoved(obj);
      }
    }

    qDebug() << "Successfully built 3D scene in" << timer.elapsed() << "ms.";
  } catch (const Exception& e) {
    qCritical().noquote() << "Failed to build 3D scene after" << timer.elapsed()
                          << "ms:" << e.getMsg();
    errorMsg = QStringList{errorMsg, e.getMsg()}.join("\n\n");
  }
}

ClipperLib::Paths OpenGlSceneBuilder::getPaths(
    const std::shared_ptr<SceneData3D>& data, const QStringList layers) const {
  ClipperLib::Paths paths;
  foreach (const auto& area, data->getAreas()) {
    if (layers.contains(area.layer->getId())) {
      paths.push_back(ClipperHelpers::convert(area.outline, mMaxArcTolerance));
    }
  }
  return paths;
}

QVector<QVector3D> OpenGlSceneBuilder::extrude(const ClipperLib::Paths& paths,
                                               qreal z, qreal height,
                                               qreal scaleFactor, bool faces,
                                               bool edges, bool closed) const {
  const qreal z0 = z * scaleFactor;
  const qreal z1 = (z + height) * scaleFactor;

  QVector<QVector3D> triangles;
  for (const ClipperLib::Path& path : paths) {
    if (faces) {
      const QVector<QVector3D> newTriangles = tesselate(path, z0, scaleFactor);
      triangles += newTriangles;
      foreach (const QVector3D& vertex, newTriangles) {
        triangles.append(QVector3D(vertex.x(), vertex.y(), z1));
      }
    }

    if (edges) {
      const std::size_t size = closed ? path.size() : (path.size() - 1);
      for (std::size_t i = 0; i < size; ++i) {
        const ClipperLib::IntPoint pos0 = path.at(i);
        const ClipperLib::IntPoint pos1 = path.at((i + 1) % path.size());
        const QVector3D p0(pos0.X * scaleFactor * 1e-6,
                           pos0.Y * scaleFactor * 1e-6, z0);
        const QVector3D p1(pos0.X * scaleFactor * 1e-6,
                           pos0.Y * scaleFactor * 1e-6, z1);
        const QVector3D p2(pos1.X * scaleFactor * 1e-6,
                           pos1.Y * scaleFactor * 1e-6, z1);
        const QVector3D p3(pos1.X * scaleFactor * 1e-6,
                           pos1.Y * scaleFactor * 1e-6, z0);
        triangles.append(p0);
        triangles.append(p1);
        triangles.append(p2);
        triangles.append(p2);
        triangles.append(p3);
        triangles.append(p0);
      }
    }
  }
  return triangles;
}

#if USE_GLU

#ifndef CALLBACK
#define CALLBACK
#endif

static void CALLBACK tessVertexCallback(const GLvoid* data, GLvoid* context) {
  const GLdouble* vertex = static_cast<const GLdouble*>(data);
  QVector<QVector3D>* triangles = static_cast<QVector<QVector3D>*>(context);
  triangles->append(QVector3D(vertex[0], vertex[1], vertex[2]));
}

static void CALLBACK tessEdgeFlagCallback(GLboolean) {
}

#endif

QVector<QVector3D> OpenGlSceneBuilder::tesselate(const ClipperLib::Path& path,
                                                 qreal z, qreal scaleFactor) {
  QVector<QVector3D> result;
#if USE_GLU
  QVector<GLdouble> input;
  for (const ClipperLib::IntPoint& point : path) {
    input.push_back(point.X * scaleFactor * 1e-6);
    input.push_back(point.Y * scaleFactor * 1e-6);
    input.push_back(z);
  }
  GLUtesselator* tess = gluNewTess();
  gluTessCallback(tess, GLU_TESS_VERTEX_DATA,
                  (void(CALLBACK*)())tessVertexCallback);
  gluTessCallback(tess, GLU_TESS_EDGE_FLAG,
                  (void(CALLBACK*)())tessEdgeFlagCallback);
  gluTessNormal(tess, 0.0, 0.0, 1.0);
  gluTessBeginPolygon(tess, &result);
  gluTessBeginContour(tess);
  for (std::size_t i = 0; i < path.size(); ++i) {
    gluTessVertex(tess, &input[i * 3], &input[i * 3]);
  }
  gluTessEndContour(tess);
  gluTessEndPolygon(tess);
  gluDeleteTess(tess);
#else
  Q_UNUSED(path);
  Q_UNUSED(z);
  Q_UNUSED(scaleFactor);
  qWarning() << "Could not tesselate 3D surface because LibrePCB was compiled "
                "without GLU library.";
#endif
  return result;
}

void OpenGlSceneBuilder::publishTriangleData(
    const QString& id, const QColor& color,
    const QVector<QVector3D>& triangles) {
  std::shared_ptr<OpenGlTriangleObject> obj = mBoardObjects.value(id);
  if (obj) {
    obj->setData(color, triangles);
    emit objectUpdated(obj);
  } else {
    obj = std::make_shared<OpenGlTriangleObject>();
    obj->setData(color, triangles);
    mBoardObjects[id] = obj;
    emit objectAdded(obj);
  }
}

void OpenGlSceneBuilder::publishDevice(const SceneData3D::DeviceData& obj,
                                       const QByteArray& stepContent, qreal z,
                                       qreal scaleFactor, qreal alpha) {
  StepModel model;
  if (mStepModels.contains(stepContent)) {
    model = mStepModels.value(stepContent);
  } else {
    if (stepContent.size()) {
      try {
        std::unique_ptr<OccModel> occModel = OccModel::loadStep(stepContent);
        model = occModel->tesselate();
      } catch (const Exception& e) {
        qCritical().nospace()
            << "Failed to draw 3D model of " << obj.name << ": " << e.getMsg();
      }
    }
    mStepModels.insert(stepContent, model);
  }

  QMatrix4x4 m;
  m.scale(scaleFactor);
  m.translate(obj.transform.getPosition().getX().toMm(),
              obj.transform.getPosition().getY().toMm(),
              obj.transform.getMirrored() ? -z : z);
  m.rotate(obj.transform.getRotation().toDeg(), 0, 0, 1);
  if (obj.transform.getMirrored()) {
    m.rotate(Angle::deg180().toDeg(), 0, 1, 0);
  }
  m.translate(std::get<0>(obj.stepPosition).toMm(),
              std::get<1>(obj.stepPosition).toMm(),
              std::get<2>(obj.stepPosition).toMm());
  m.rotate(std::get<2>(obj.stepRotation).toDeg(), 0, 0, 1);
  m.rotate(std::get<1>(obj.stepRotation).toDeg(), 0, 1, 0);
  m.rotate(std::get<0>(obj.stepRotation).toDeg(), 1, 0, 0);

  QMap<Color, std::shared_ptr<OpenGlTriangleObject>>& items =
      mDevices[obj.uuid];
  foreach (const Color& color, items.keys()) {
    if (!model.contains(color)) {
      emit objectRemoved(items.take(color));
    }
  }
  for (auto it = model.begin(); it != model.end(); it++) {
    QVector<QVector3D> vertices = it.value();
    for (auto& vertex : vertices) {
      vertex = m.map(vertex);
    }
    std::shared_ptr<OpenGlTriangleObject> obj = items.value(it.key());
    QColor color = QColor::fromRgbF(
        std::get<0>(it.key()), std::get<1>(it.key()), std::get<2>(it.key()));
    if (alpha != 1) {
      color.setAlphaF(alpha);
    }
    if (obj) {
      obj->setData(color, vertices);
      emit objectUpdated(obj);
    } else {
      obj = std::make_shared<OpenGlTriangleObject>();
      obj->setData(color, vertices);
      items[it.key()] = obj;
      emit objectAdded(obj);
    }
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
