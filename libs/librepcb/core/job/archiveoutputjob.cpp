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
#include "archiveoutputjob.h"

#include "../utils/toolbox.h"

#include <QtCore>
#include <QtGui>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

ArchiveOutputJob::ArchiveOutputJob() noexcept
  : OutputJob(
        getTypeName(), Uuid::createRandom(),
        elementNameFromTr("ArchiveOutputJob", QT_TR_NOOP("Output Archive"))),
    mInputJobs(),
    mOutputPath("{{PROJECT}}_{{VERSION}}.zip") {
}

ArchiveOutputJob::ArchiveOutputJob(const ArchiveOutputJob& other) noexcept
  : OutputJob(other),
    mInputJobs(other.mInputJobs),
    mOutputPath(other.mOutputPath) {
}

ArchiveOutputJob::ArchiveOutputJob(const SExpression& node)
  : OutputJob(node),
    mInputJobs(),
    mOutputPath(node.getChild("output/@0").getValue()) {
  foreach (const SExpression* child, node.getChildren("input")) {
    mInputJobs.insert(deserialize<Uuid>(child->getChild("@0")),
                      child->getChild("destination/@0").getValue());
  }
}

ArchiveOutputJob::~ArchiveOutputJob() noexcept {
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

QString ArchiveOutputJob::getTypeTr() const noexcept {
  return getTypeTrStatic();
}

QIcon ArchiveOutputJob::getTypeIcon() const noexcept {
  return QIcon(":/img/actions/export_zip.png");
}

QSet<Uuid> ArchiveOutputJob::getDependencies() const noexcept {
  return Toolbox::toSet(mInputJobs.keys());
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void ArchiveOutputJob::setInputJobs(const QMap<Uuid, QString>& input) noexcept {
  if (input != mInputJobs) {
    mInputJobs = input;
    onEdited.notify(Event::PropertyChanged);
  }
}

void ArchiveOutputJob::setOutputPath(const QString& path) noexcept {
  if (path != mOutputPath) {
    mOutputPath = path;
    onEdited.notify(Event::PropertyChanged);
  }
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void ArchiveOutputJob::removeDependency(const Uuid& jobUuid) {
  auto input = mInputJobs;
  input.remove(jobUuid);
  setInputJobs(input);
}

std::shared_ptr<OutputJob> ArchiveOutputJob::cloneShared() const noexcept {
  return std::make_shared<ArchiveOutputJob>(*this);
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void ArchiveOutputJob::serializeDerived(SExpression& root) const {
  root.ensureLineBreak();
  for (auto it = mInputJobs.begin(); it != mInputJobs.end(); ++it) {
    SExpression& child = root.appendList("input");
    child.appendChild(it.key());
    child.appendChild("destination", it.value());
    root.ensureLineBreak();
  }
  root.appendChild("output", mOutputPath);
}

bool ArchiveOutputJob::equals(const OutputJob& rhs) const noexcept {
  const ArchiveOutputJob& other = static_cast<const ArchiveOutputJob&>(rhs);
  if (mInputJobs != other.mInputJobs) return false;
  if (mOutputPath != other.mOutputPath) return false;
  return true;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
