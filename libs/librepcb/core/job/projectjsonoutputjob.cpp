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
#include "projectjsonoutputjob.h"

#include <QtCore>
#include <QtGui>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

ProjectJsonOutputJob::ProjectJsonOutputJob() noexcept
  : OutputJob(
        getTypeName(), Uuid::createRandom(),
        elementNameFromTr("ProjectJsonOutputJob", QT_TR_NOOP("Project Data"))),
    mOutputPath("{{PROJECT}}_{{VERSION}}.json") {
}

ProjectJsonOutputJob::ProjectJsonOutputJob(
    const ProjectJsonOutputJob& other) noexcept
  : OutputJob(other), mOutputPath(other.mOutputPath) {
}

ProjectJsonOutputJob::ProjectJsonOutputJob(const SExpression& node)
  : OutputJob(node), mOutputPath(node.getChild("output/@0").getValue()) {
}

ProjectJsonOutputJob::~ProjectJsonOutputJob() noexcept {
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

QString ProjectJsonOutputJob::getTypeTr() const noexcept {
  return getTypeTrStatic();
}

QIcon ProjectJsonOutputJob::getTypeIcon() const noexcept {
  return QIcon(":/img/logo/48x48.png");
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void ProjectJsonOutputJob::setOutputPath(const QString& path) noexcept {
  if (path != mOutputPath) {
    mOutputPath = path;
    onEdited.notify(Event::PropertyChanged);
  }
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

std::shared_ptr<OutputJob> ProjectJsonOutputJob::cloneShared() const noexcept {
  return std::make_shared<ProjectJsonOutputJob>(*this);
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void ProjectJsonOutputJob::serializeDerived(SExpression& root) const {
  root.ensureLineBreak();
  root.appendChild("output", mOutputPath);
}

bool ProjectJsonOutputJob::equals(const OutputJob& rhs) const noexcept {
  const ProjectJsonOutputJob& other =
      static_cast<const ProjectJsonOutputJob&>(rhs);
  if (mOutputPath != other.mOutputPath) return false;
  return true;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
