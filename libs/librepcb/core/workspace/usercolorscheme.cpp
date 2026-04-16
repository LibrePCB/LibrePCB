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
#include "usercolorscheme.h"

#include "../exceptions.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

UserColorScheme::UserColorScheme(const Uuid& uuid, const QString& name,
                                 const BaseColorScheme& base) noexcept
  : QObject(),
    mLoadedNodes(),
    mUuid(uuid),
    mName(name),
    mBaseUuid(base.getUuid()),
    mBase(&base) {
  auto node = SExpression::createList("base");
  node->appendChild(mBaseUuid);
  node->appendChild(base.getName());
  mLoadedNodes.append(*node);
}

UserColorScheme::UserColorScheme(const Uuid& uuid, const QString& name,
                                 const UserColorScheme& copyFrom) noexcept
  : UserColorScheme(copyFrom) {
  mUuid = uuid;
  mName = name;
}

UserColorScheme::UserColorScheme(const SExpression& root,
                                 const QVector<const BaseColorScheme*>& bases)
  : QObject(),
    mLoadedNodes(),
    mUuid(deserialize<Uuid>(root.getChild("@0"))),
    mName(root.getChild("@1").getValue()),
    mBaseUuid(deserialize<Uuid>(root.getChild("base/@0"))),
    mBase(bases.first()) {
  Q_ASSERT(!bases.isEmpty());
  for (const BaseColorScheme* base : bases) {
    if (base->getUuid() == mBaseUuid) {
      mBase = base;
      break;
    }
  }
  foreach (const SExpression* node, root.getChildren(SExpression::Type::List)) {
    mLoadedNodes.append(*node);
  }
  for (const SExpression* child : root.getChildren("color")) {
    const QString role = child->getChild("@0").getValue();
    if (const SExpression* colorChild = child->tryGetChild("primary/@0")) {
      mPrimaryColorOverrides.insert(role, deserialize<QColor>(*colorChild));
    }
    if (const SExpression* colorChild = child->tryGetChild("secondary/@0")) {
      mSecondaryColorOverrides.insert(role, deserialize<QColor>(*colorChild));
    }
  }
}

UserColorScheme::UserColorScheme(const UserColorScheme& other) noexcept
  : QObject(),
    mLoadedNodes(other.mLoadedNodes),
    mModifiedColors(other.mModifiedColors),
    mUuid(other.mUuid),
    mName(other.mName),
    mBaseUuid(other.mBaseUuid),
    mBase(other.mBase),
    mPrimaryColorOverrides(other.mPrimaryColorOverrides),
    mSecondaryColorOverrides(other.mSecondaryColorOverrides) {
}

UserColorScheme::~UserColorScheme() noexcept {
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

static void applyOverride(QColor& out, const QHash<QString, QColor>& overrides,
                          const QString& role) noexcept {
  const QColor color = overrides.value(role);
  if (color.isValid()) {
    out = color;
  }
}

std::optional<ColorScheme::Colors> UserColorScheme::tryGetColors(
    const QString& role) const noexcept {
  auto colors = mBase->tryGetColors(role);
  if (colors) {
    applyOverride(colors->primary, mPrimaryColorOverrides, role);
    applyOverride(colors->secondary, mSecondaryColorOverrides, role);
  }
  return colors;
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void UserColorScheme::setName(const QString& name) noexcept {
  if (name != mName) {
    mName = name;
    emit modified();
  }
}

void UserColorScheme::setPrimary(const QString& role,
                                 const QColor& color) noexcept {
  if (color != mPrimaryColorOverrides.value(role)) {
    mPrimaryColorOverrides[role] = color;
    mModifiedColors.insert(role);
    emit colorsModified(role);
    emit modified();
  }
}

void UserColorScheme::setSecondary(const QString& role,
                                   const QColor& color) noexcept {
  if (color != mSecondaryColorOverrides.value(role)) {
    mSecondaryColorOverrides[role] = color;
    mModifiedColors.insert(role);
    emit colorsModified(role);
    emit modified();
  }
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void UserColorScheme::serialize(SExpression& root) const {
  QVector<SExpression> nodes = mLoadedNodes;

  if (!mModifiedColors.isEmpty()) {
    for (int i = nodes.count() - 1; i >= 0; --i) {
      const SExpression& node = nodes.at(i);
      if ((node.getName() == "color") && (node.getChildCount() >= 1) &&
          (node.getChild(0).getType() == SExpression::Type::Token) &&
          mModifiedColors.contains(node.getChild(0).getValue())) {
        nodes.removeAt(i);
      }
    }
    for (const QString& role : mModifiedColors) {
      auto node = SExpression::createList("color");
      node->appendChild(SExpression::createToken(role));
      const QColor primary = mPrimaryColorOverrides.value(role);
      const QColor secondary = mSecondaryColorOverrides.value(role);
      if (primary.isValid()) {
        node->appendChild("primary", primary);
      }
      if (secondary.isValid()) {
        node->appendChild("secondary", secondary);
      }
      if (primary.isValid() || secondary.isValid()) {
        nodes.append(*node);
      }
    }
    std::sort(nodes.begin(), nodes.end());
  }

  root.appendChild(mUuid);
  root.appendChild(mName);
  for (const SExpression& node : nodes) {
    root.ensureLineBreak();
    root.appendChild(node);
  }
  root.ensureLineBreak();
}

/*******************************************************************************
 *  Operator Overloadings
 ******************************************************************************/

bool UserColorScheme::operator==(const UserColorScheme& rhs) const noexcept {
  return (mLoadedNodes == rhs.mLoadedNodes)  //
      && (mModifiedColors == rhs.mModifiedColors)  //
      && (mUuid == rhs.mUuid)  //
      && (mName == rhs.mName)  //
      && (mBase == rhs.mBase)  //
      && (mPrimaryColorOverrides == rhs.mPrimaryColorOverrides)  //
      && (mSecondaryColorOverrides == rhs.mSecondaryColorOverrides)  //
      ;
}

UserColorScheme& UserColorScheme::operator=(
    const UserColorScheme& rhs) noexcept {
  mLoadedNodes = rhs.mLoadedNodes;
  mModifiedColors = rhs.mModifiedColors;
  mUuid = rhs.mUuid;
  mName = rhs.mName;
  mBase = rhs.mBase;
  mPrimaryColorOverrides = rhs.mPrimaryColorOverrides;
  mSecondaryColorOverrides = rhs.mSecondaryColorOverrides;
  return *this;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
