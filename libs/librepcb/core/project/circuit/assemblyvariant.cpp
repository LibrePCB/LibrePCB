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
#include "assemblyvariant.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

AssemblyVariant::AssemblyVariant(const AssemblyVariant& other) noexcept
  : onEdited(*this),
    mUuid(other.mUuid),
    mName(other.mName),
    mDescription(other.mDescription) {
}

AssemblyVariant::AssemblyVariant(const SExpression& node)
  : onEdited(*this),
    mUuid(deserialize<Uuid>(node.getChild("@0"))),
    mName(deserialize<FileProofName>(node.getChild("name/@0"))),
    mDescription(node.getChild("description/@0").getValue()) {
}

AssemblyVariant::AssemblyVariant(const Uuid& uuid, const FileProofName& name,
                                 const QString& description)
  : onEdited(*this), mUuid(uuid), mName(name), mDescription(description) {
}

AssemblyVariant::~AssemblyVariant() noexcept {
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

QString AssemblyVariant::getDisplayText() const noexcept {
  QString s = *mName;
  if (!mDescription.isEmpty()) {
    s += " (" % mDescription % ")";
  }
  return s;
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void AssemblyVariant::setName(const FileProofName& name) noexcept {
  if (name != mName) {
    mName = name;
    onEdited.notify(Event::NameChanged);
  }
}

void AssemblyVariant::setDescription(const QString& description) noexcept {
  if (description != mDescription) {
    mDescription = description;
    onEdited.notify(Event::DescriptionChanged);
  }
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void AssemblyVariant::serialize(SExpression& root) const {
  root.appendChild(mUuid);
  root.appendChild("name", mName);
  root.ensureLineBreak();
  root.appendChild("description", mDescription);
  root.ensureLineBreak();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
