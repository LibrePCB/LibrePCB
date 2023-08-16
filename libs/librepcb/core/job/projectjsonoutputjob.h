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

#ifndef LIBREPCB_CORE_PROJECTJSONOUTPUTJOB_H
#define LIBREPCB_CORE_PROJECTJSONOUTPUTJOB_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "outputjob.h"

#include <QtCore>
#include <QtGui>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class ProjectJsonOutputJob
 ******************************************************************************/

/**
 * @brief JSON project data output job
 */
class ProjectJsonOutputJob final : public OutputJob {
  Q_DECLARE_TR_FUNCTIONS(ProjectJsonOutputJob)

public:
  // Constructors / Destructor
  ProjectJsonOutputJob() noexcept;
  ProjectJsonOutputJob(const ProjectJsonOutputJob& other) noexcept;
  explicit ProjectJsonOutputJob(const SExpression& node);
  virtual ~ProjectJsonOutputJob() noexcept;

  // Getters
  virtual QString getTypeTr() const noexcept override;
  virtual QIcon getTypeIcon() const noexcept override;
  const QString& getOutputPath() const noexcept { return mOutputPath; }

  // Setters
  void setOutputPath(const QString& path) noexcept;

  // General Methods
  static QString getTypeName() noexcept { return "project_json"; }
  static QString getTypeTrStatic() noexcept {
    return tr("Project Data") % " (*.json)";
  }
  virtual std::shared_ptr<OutputJob> cloneShared() const noexcept override;

  // Operator Overloadings
  ProjectJsonOutputJob& operator=(const ProjectJsonOutputJob& rhs) = delete;

private:  // Methods
  virtual void serializeDerived(SExpression& root) const override;
  virtual bool equals(const OutputJob& rhs) const noexcept override;

private:  // Data
  QString mOutputPath;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
