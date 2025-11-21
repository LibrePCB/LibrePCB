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
#include "organizationpcbdesignrules.h"

#include "../../serialization/sexpression.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

OrganizationPcbDesignRules::OrganizationPcbDesignRules(
    const Uuid& uuid, const ElementName& name, const QString& description,
    const QUrl& url, const BoardDesignRuleCheckSettings& settings) noexcept
  : mUuid(uuid),
    mNames(name),
    mDescriptions(description),
    mUrl(url),
    mDrcSettings(settings) {
}

OrganizationPcbDesignRules::OrganizationPcbDesignRules(
    const OrganizationPcbDesignRules& other) noexcept
  : mUuid(other.mUuid),
    mNames(other.mNames),
    mDescriptions(other.mDescriptions),
    mUrl(other.mUrl),
    mDrcSettings(other.mDrcSettings) {
}

OrganizationPcbDesignRules::OrganizationPcbDesignRules(const SExpression& node)
  : mUuid(deserialize<Uuid>(node.getChild("@0"))),
    mNames(node),
    mDescriptions(node),
    // Note: Don't use SExpression::getValueByPath<QUrl>() because it would
    // throw an exception if the URL is empty, which is actually legal in this
    // case.
    mUrl(node.getChild("url/@0").getValue(), QUrl::StrictMode),
    mDrcSettings(node) {
  mDrcSettings.setSources({});  // Not supported in this context.
}

OrganizationPcbDesignRules::~OrganizationPcbDesignRules() noexcept {
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

BoardDesignRuleCheckSettings OrganizationPcbDesignRules::getDrcSettings(
    bool cleanOptions) const noexcept {
  if (!cleanOptions) {
    return mDrcSettings;
  }

  // Discard options with the prefix "org_" since they are intended only
  // for this class, not for BoardDesignRuleCheckSettings.
  QMap<QString, QList<SExpression>> options;
  for (auto it = mDrcSettings.getOptions().begin();
       it != mDrcSettings.getOptions().end(); it++) {
    if (!it.key().startsWith("org_")) {
      options.insert(it.key(), it.value());
    }
  }

  BoardDesignRuleCheckSettings s = mDrcSettings;
  s.setOptions(options);
  return s;
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void OrganizationPcbDesignRules::serialize(SExpression& root) const {
  root.appendChild(mUuid);
  root.ensureLineBreak();
  mNames.serialize(root);
  root.ensureLineBreak();
  mDescriptions.serialize(root);
  root.ensureLineBreak();
  root.appendChild("url", mUrl);
  root.ensureLineBreak();
  mDrcSettings.serialize(root);
  root.ensureLineBreak();
}

/*******************************************************************************
 *  Operator Overloadings
 ******************************************************************************/

OrganizationPcbDesignRules& OrganizationPcbDesignRules::operator=(
    const OrganizationPcbDesignRules& rhs) noexcept {
  mUuid = rhs.mUuid;
  mNames = rhs.mNames;
  mDescriptions = rhs.mDescriptions;
  mUrl = rhs.mUrl;
  mDrcSettings = rhs.mDrcSettings;
  return *this;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
