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

#ifndef LIBREPCB_CORE_ARCHIVEOUTPUTJOB_H
#define LIBREPCB_CORE_ARCHIVEOUTPUTJOB_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "outputjob.h"

#include <QtCore>
#include <QtGui>

#include <optional>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class ArchiveOutputJob
 ******************************************************************************/

/**
 * @brief File archiving (e.g. to ZIP) output job
 */
class ArchiveOutputJob final : public OutputJob {
  Q_DECLARE_TR_FUNCTIONS(ArchiveOutputJob)

public:
  // Constructors / Destructor
  ArchiveOutputJob() noexcept;
  ArchiveOutputJob(const ArchiveOutputJob& other) noexcept;
  explicit ArchiveOutputJob(const SExpression& node);
  virtual ~ArchiveOutputJob() noexcept;

  // Getters
  virtual QString getTypeTr() const noexcept override;
  virtual QIcon getTypeIcon() const noexcept override;
  virtual QSet<Uuid> getDependencies() const noexcept override;
  const QMap<Uuid, QString>& getInputJobs() const noexcept {
    return mInputJobs;
  }
  const QString& getOutputPath() const noexcept { return mOutputPath; }

  // Setters
  void setInputJobs(const QMap<Uuid, QString>& input) noexcept;
  void setOutputPath(const QString& path) noexcept;

  // General Methods
  static QString getTypeName() noexcept { return "archive"; }
  static QString getTypeTrStatic() noexcept {
    return tr("Archive") % " (*.zip)";
  }
  virtual void removeDependency(const Uuid& jobUuid) override;
  virtual std::shared_ptr<OutputJob> cloneShared() const noexcept override;

  // Operator Overloadings
  ArchiveOutputJob& operator=(const ArchiveOutputJob& rhs) = delete;

private:  // Methods
  virtual void serializeDerived(SExpression& root) const override;
  virtual bool equals(const OutputJob& rhs) const noexcept override;

private:  // Data
  QMap<Uuid, QString> mInputJobs;  ///< Job UUID, destination path
  QString mOutputPath;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
