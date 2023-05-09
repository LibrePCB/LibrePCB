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
#include "stepexport.h"

#include "../fileio/filesystem.h"
#include "../fileio/fileutils.h"
#include "../types/pcbcolor.h"
#include "../utils/clipperhelpers.h"
#include "../utils/scopeguard.h"
#include "occmodel.h"
#include "scenedata3d.h"

#include <QtConcurrent>
#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

StepExport::StepExport(QObject* parent) noexcept
  : QObject(parent), mFuture(), mAbort(false) {
}

StepExport::~StepExport() noexcept {
  cancel();
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void StepExport::start(std::shared_ptr<SceneData3D> data, const FilePath& fp,
                       int finishDelayMs) noexcept {
  cancel();
  mFuture = QtConcurrent::run(this, &StepExport::run, data, fp, finishDelayMs);
}

bool StepExport::isBusy() const noexcept {
  return (mFuture.isStarted() || mFuture.isRunning()) &&
      (!mFuture.isFinished()) && (!mFuture.isCanceled());
}

void StepExport::waitForFinished() noexcept {
  mFuture.waitForFinished();
}

void StepExport::cancel() noexcept {
  mAbort = true;
  mFuture.waitForFinished();
  mAbort = false;
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void StepExport::run(std::shared_ptr<SceneData3D> data, FilePath fp,
                     int finishDelayMs) noexcept {
  // Note: This method is called from a different thread, thus be careful with
  //       calling other methods to only call thread-safe methods!

  QElapsedTimer timer;
  timer.start();
  qDebug() << "Start exporting STEP file in worker thread...";

  emit started();
  emit progressStatus(QString());
  emit progressPercent(0);
  auto sg = scopeGuard([this]() { emit finished(); });

  try {
    // Preprocess the data.
    emit progressStatus(tr("Preparing..."));
    data->preprocess(false, true);
    emit progressPercent(10);
    if (mAbort) return;

    // Create assembly model.
    std::unique_ptr<OccModel> model =
        OccModel::createAssembly(data->getProjectName());

    // Add PCB body, if outlines are valid.
    emit progressStatus(tr("Exporting PCB..."));
    QVector<Path> outlines;
    for (const auto& obj : data->getAreas()) {
      if (obj.layer->getId() == Layer::boardOutlines().getId()) {
        outlines.append(obj.outline);
      }
    }
    std::unique_ptr<ClipperLib::PolyTree> tree = ClipperHelpers::uniteToTree(
        ClipperHelpers::convert(outlines, PositiveLength(5000)),
        ClipperLib::pftEvenOdd);
    outlines = ClipperHelpers::convert(ClipperHelpers::flattenTree(*tree));
    const Path outline = outlines.isEmpty() ? Path() : outlines.first();

    QVector<Path> holes;
    for (const auto& obj : data->getHoles()) {
      if (!obj.via) {
        holes.append(obj.path->toOutlineStrokes(obj.diameter));
      }
    }
    QColor color(70, 80, 70);
    if (const PcbColor* c = data->getSolderResist()) {
      color = c->toSolderResistColor().darker();
    }
    if (!outline.getVertices().isEmpty()) {
      std::unique_ptr<OccModel> pcb =
          OccModel::createBoard(outline, holes, data->getThickness(), color);
      model->addToAssembly(*pcb, Point3D(), Angle3D(), Transform(), "PCB");
    }
    emit progressPercent(20);

    // Add devices.
    int deviceErrors = 0;
    QString lastError;
    if (std::shared_ptr<FileSystem> fs = data->getFileSystem()) {
      int i = 1;
      for (const auto& obj : data->getDevices()) {
        try {
          emit progressStatus(tr("Exporting device %1/%2...")
                                  .arg(i)
                                  .arg(data->getDevices().count()));
          const QByteArray content = fs->readIfExists(obj.stepFile);
          if (!content.isEmpty()) {
            Point3D pos = obj.stepPosition;
            if (!obj.transform.getMirrored()) {
              std::get<2>(pos) += *data->getThickness();
            }
            std::unique_ptr<OccModel> devModel = OccModel::loadStep(content);
            model->addToAssembly(*devModel, pos, obj.stepRotation,
                                 obj.transform, obj.name);
          }
        } catch (const Exception& e) {
          qCritical().noquote() << "Failed to export STEP model of " << obj.name
                                << ": " << e.getMsg();
          ++deviceErrors;
          lastError = obj.name % ": " % e.getMsg();
        }
        emit progressPercent(20 + ((70 * i) / data->getDevices().count()));
        if (mAbort) return;
        ++i;
      }
    }

    // Save model to file.
    emit progressStatus(tr("Saving..."));
    model->saveAsStep(data->getProjectName(), fp);
    emit progressPercent(100);
    qDebug() << "Exported STEP file in" << timer.elapsed() << "ms.";

    if (deviceErrors > 0) {
      QString msg = tr("The export completed, but there were %1 errors!")
                        .arg(deviceErrors);
      msg += " " % tr("The last error was:");
      msg += "\n" % lastError;
      emit progressStatus(tr("Finished with errors!"));
      emit failed(msg);
    } else {
      emit progressStatus(tr("Success!"));
      emit succeeded();
      QThread::msleep(finishDelayMs);  // Keep displaying status message.
    }

    return;  // Do not handle mAbort anymore.
  } catch (const Exception& e) {
    emit progressStatus(tr("Failed!"));
    emit progressPercent(100);
    emit failed(e.getMsg());
    qCritical().noquote() << "Failed to export STEP file after"
                          << timer.elapsed() << "ms:" << e.getMsg();
    return;  // Do not handle mAbort anymore.
  }

  // Aborted.
  emit progressStatus(tr("Aborted!"));
  qDebug() << "STEP export aborted after" << timer.elapsed() << "ms.";
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
