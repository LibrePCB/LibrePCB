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

#ifndef LIBREPCB_CORE_STEPEXPORT_H
#define LIBREPCB_CORE_STEPEXPORT_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <QtCore>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class FilePath;
class SceneData3D;

/*******************************************************************************
 *  Class StepExport
 ******************************************************************************/

/**
 * @brief Asynchronously generates an assembly STEP file of a PCB
 */
class StepExport final : public QObject {
  Q_OBJECT

public:
  // Types
  typedef std::tuple<qreal, qreal, qreal> Color;
  typedef QMap<Color, QVector<QVector3D>> StepModel;

  // Constructors / Destructor
  StepExport(QObject* parent = nullptr) noexcept;
  StepExport(const StepExport& other) = delete;
  ~StepExport() noexcept;

  // General Methods

  /**
   * @brief Start building scene asynchronously
   */
  void start(std::shared_ptr<SceneData3D> data, const FilePath& fp,
             int finishDelayMs = 0) noexcept;

  /**
   * @brief Check if there is currently a build in progress
   *
   * @retval true Build in progress.
   * @retval false Idle.
   */
  bool isBusy() const noexcept;

  /**
   * @brief Wait (block) until the build is finished
   *
   * @return Error message (null on success).
   */
  QString waitForFinished() noexcept;

  /**
   * @brief Cancel the build
   */
  void cancel() noexcept;

  // Operator Overloadings
  StepExport& operator=(const StepExport& rhs) = delete;

signals:
  void started();
  void progressStatus(QString status);
  void progressPercent(int percent);
  void succeeded();
  void failed(QString errorMsg);
  void finished();

private:  // Methods
  QString run(std::shared_ptr<SceneData3D> data, FilePath fp,
              int finishDelayMs) noexcept;

private:  // Data
  QFuture<QString> mFuture;
  bool mAbort;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
