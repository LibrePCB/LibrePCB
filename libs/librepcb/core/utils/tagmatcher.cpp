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
#include "tagmatcher.h"

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

TagMatcher::TagMatcher(const QVector<TagConditional>& conditionals) noexcept
  : mConditionals(conditionals) {
}

TagMatcher::~TagMatcher() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

int TagMatcher::addOption(const QSet<Tag>& builtIn,
                          const QSet<Tag>& userDefined) noexcept {
  const int index = mOptions.count();
  mOptions.append(Option{builtIn, userDefined});
  return index;
}

int TagMatcher::findFirstMatch() noexcept {
  std::set<int> filtered;
  for (int i = 0; i < mOptions.count(); ++i) {
    filtered.insert(i);
  }
  for (const TagConditional& conditional : mConditionals) {
    if (applyFilter(filtered, conditional)) {
      return *filtered.begin();  // Exactly one result remaining.
    }
  }
  return filtered.empty() ? -1 : *filtered.begin();
}

bool TagMatcher::applyFilter(std::set<int>& filtered,
                             const TagConditional& conditional) const noexcept {
  std::set<int> remaining;
  for (int i : filtered) {
    if (!conditional.matches(mOptions.at(i).builtIn,
                             mOptions.at(i).userDefined)) {
      remaining.insert(i);
    }
  }
  if ((!remaining.empty()) && (remaining.size() < filtered.size())) {
    filtered = remaining;
  }
  return (filtered.size() == 1);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
