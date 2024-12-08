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
  mFuture = QtConcurrent::run(&StepExport::run, this, data, fp, finishDelayMs);
}

bool StepExport::isBusy() const noexcept {
  return (mFuture.isStarted() || mFuture.isRunning()) &&
      (!mFuture.isFinished()) && (!mFuture.isCanceled());
}

QString StepExport::waitForFinished() noexcept {
  mFuture.waitForFinished();
  return mFuture.result();
}

void StepExport::cancel() noexcept {
  mAbort = true;
  mFuture.waitForFinished();
  mAbort = false;
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

QString StepExport::run(std::shared_ptr<SceneData3D> data, FilePath fp,
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
    if (mAbort) return QString();

    // Create assembly model.
    std::unique_ptr<OccModel> model =
        OccModel::createAssembly(data->getProjectName());

    // Add PCB bodies (export all for consistency with built-in 3D viewer,
    // see https://github.com/LibrePCB/LibrePCB/issues/1364).
    emit progressStatus(tr("Exporting PCB..."));
    QList<Path> outlines;
    for (const auto& obj : data->getAreas()) {
      if ((obj.layer->getId() == Layer::boardOutlines().getId()) &&
          (obj.outline.isClosed())) {
        outlines.append(obj.outline);
      }
    }
    QVector<Path> holes;
    for (const auto& obj : data->getAreas()) {
      if (((obj.layer->getId() == Layer::boardCutouts().getId()) ||
           (obj.layer->getId() == Layer::boardPlatedCutouts().getId())) &&
          (obj.outline.isClosed())) {
        holes.append(obj.outline);
      }
    }
    for (const auto& obj : data->getHoles()) {
      if (!obj.via) {
        holes.append(obj.path->toOutlineStrokes(obj.diameter));
      }
    }
    QColor color(70, 80, 70);
    if (const PcbColor* c = data->getSolderResist()) {
      color = c->toSolderResistColor().darker();
    }
    for (int i = 0; i < outlines.size(); ++i) {
      std::unique_ptr<OccModel> pcb = OccModel::createBoard(
          outlines.at(i), holes, data->getThickness(), color);
      const QString suffix =
          (outlines.size() > 1) ? QString::number(i + 1) : QString();
      model->addToAssembly(*pcb, Point3D(), Angle3D(), Transform(),
                           "PCB" % suffix);
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
        if (mAbort) return QString();
        ++i;
      }
    }

    // Save model to file.
    emit progressStatus(tr("Saving..."));
    model->saveAsStep(data->getProjectName(), fp);
    emit progressPercent(100);
    qDebug() << "Exported STEP file in" << timer.elapsed() << "ms.";

    QString errMsg;
    if (deviceErrors > 0) {
      errMsg = tr("The export completed, but there were %1 errors!")
                   .arg(deviceErrors);
      errMsg += " " % tr("The last error was:");
      errMsg += "\n" % lastError;
      emit progressStatus(tr("Finished with errors!"));
      emit failed(errMsg);
    } else {
      emit progressStatus(tr("Success!"));
      emit succeeded();
      QThread::msleep(finishDelayMs);  // Keep displaying status message.
    }

    return errMsg;  // Do not handle mAbort anymore.
  } catch (const Exception& e) {
    emit progressStatus(tr("Failed!"));
    emit progressPercent(100);
    emit failed(e.getMsg());
    qCritical().noquote() << "Failed to export STEP file after"
                          << timer.elapsed() << "ms:" << e.getMsg();
    return e.getMsg();  // Do not handle mAbort anymore.
  }

  // Aborted.
  emit progressStatus(tr("Aborted!"));
  qDebug() << "STEP export aborted after" << timer.elapsed() << "ms.";
  return QString();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
