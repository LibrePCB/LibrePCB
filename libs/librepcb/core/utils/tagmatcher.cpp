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

TagMatcher::TagMatcher() noexcept {
}

TagMatcher::~TagMatcher() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

int TagMatcher::addOption(const QSet<Tag>& tags) noexcept {
  const int index = mOptions.count();
  mOptions.append(tags);
  return index;
}

int TagMatcher::findFirstMatch(const QVector<Tag>& preferredTags) noexcept {
  std::set<int> filtered;
  for (int i = 0; i < mOptions.count(); ++i) {
    filtered.insert(i);
  }
  for (const Tag& tag : preferredTags) {
    if (applyFilter(filtered, tag)) {
      return *filtered.begin();  // Exactly one result remaining.
    }
  }
  return filtered.empty() ? -1 : *filtered.begin();
}

bool TagMatcher::applyFilter(std::set<int>& filtered,
                             const Tag& tag) const noexcept {
  std::set<int> remaining;
  for (int i : filtered) {
    if (mOptions.at(i).contains(tag)) {
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
