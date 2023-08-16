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
#include "outputjob.h"

#include "archiveoutputjob.h"
#include "board3doutputjob.h"
#include "bomoutputjob.h"
#include "copyoutputjob.h"
#include "gerberexcellonoutputjob.h"
#include "gerberx3outputjob.h"
#include "graphicsoutputjob.h"
#include "lppzoutputjob.h"
#include "netlistoutputjob.h"
#include "pickplaceoutputjob.h"
#include "projectjsonoutputjob.h"
#include "unknownoutputjob.h"

#include <QtCore>
#include <QtGui>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Serialization
 ******************************************************************************/

template <>
OutputJobList deserialize(const SExpression& node) {
  OutputJobList result;
  foreach (const SExpression* child, node.getChildren("job")) {
    const QString type = child->getChild("type/@0").getValue();  // can throw
    if (type == GraphicsOutputJob::getTypeName()) {
      result.append(std::make_shared<GraphicsOutputJob>(*child));
    } else if (type == GerberExcellonOutputJob::getTypeName()) {
      result.append(std::make_shared<GerberExcellonOutputJob>(*child));
    } else if (type == PickPlaceOutputJob::getTypeName()) {
      result.append(std::make_shared<PickPlaceOutputJob>(*child));
    } else if (type == GerberX3OutputJob::getTypeName()) {
      result.append(std::make_shared<GerberX3OutputJob>(*child));
    } else if (type == NetlistOutputJob::getTypeName()) {
      result.append(std::make_shared<NetlistOutputJob>(*child));
    } else if (type == BomOutputJob::getTypeName()) {
      result.append(std::make_shared<BomOutputJob>(*child));
    } else if (type == Board3DOutputJob::getTypeName()) {
      result.append(std::make_shared<Board3DOutputJob>(*child));
    } else if (type == ProjectJsonOutputJob::getTypeName()) {
      result.append(std::make_shared<ProjectJsonOutputJob>(*child));
    } else if (type == LppzOutputJob::getTypeName()) {
      result.append(std::make_shared<LppzOutputJob>(*child));
    } else if (type == CopyOutputJob::getTypeName()) {
      result.append(std::make_shared<CopyOutputJob>(*child));
    } else if (type == ArchiveOutputJob::getTypeName()) {
      result.append(std::make_shared<ArchiveOutputJob>(*child));
    } else {
      result.append(std::make_shared<UnknownOutputJob>(*child));
    }
  }
  return result;
}

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

OutputJob::OutputJob(const OutputJob& other) noexcept
  : onEdited(*this),
    mType(other.mType),
    mUuid(other.mUuid),
    mName(other.mName) {
}

OutputJob::OutputJob(const SExpression& node)
  : onEdited(*this),
    mType(node.getChild("type/@0").getValue()),
    mUuid(deserialize<Uuid>(node.getChild("@0"))),
    mName(deserialize<ElementName>(node.getChild("name/@0"))) {
  foreach (const SExpression* child, node.getChildren("option")) {
    mOptions[child->getChild("@0").getValue()].append(*child);
  }
}

OutputJob::OutputJob(const QString& type, const Uuid& uuid,
                     const ElementName& name) noexcept
  : onEdited(*this), mType(type), mUuid(uuid), mName(name) {
}

OutputJob::~OutputJob() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void OutputJob::setUuid(const Uuid& uuid) noexcept {
  if (uuid != mUuid) {
    mUuid = uuid;
    onEdited.notify(Event::UuidChanged);
  }
}

void OutputJob::setName(const ElementName& name) noexcept {
  if (name != mName) {
    mName = name;
    onEdited.notify(Event::NameChanged);
  }
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void OutputJob::serialize(SExpression& root) const {
  root.appendChild(mUuid);
  root.appendChild("name", *mName);
  root.ensureLineBreak();
  root.appendChild("type", SExpression::createToken(mType));
  serializeDerived(root);
  foreach (const auto& list, mOptions) {
    foreach (const auto& node, list) {
      root.ensureLineBreak();
      root.appendChild(node);
    }
  }
  root.ensureLineBreak();
}

/*******************************************************************************
 *  Operator Overloadings
 ******************************************************************************/

bool OutputJob::operator==(const OutputJob& rhs) const noexcept {
  if (mType != rhs.mType) return false;
  if (mUuid != rhs.mUuid) return false;
  if (mName != rhs.mName) return false;
  if (mOptions != rhs.mOptions) return false;
  if (typeid(*this) != typeid(rhs)) return false;
  return equals(rhs);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
