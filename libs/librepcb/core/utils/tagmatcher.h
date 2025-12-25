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

#ifndef LIBREPCB_CORE_TAGMATCHER_H
#define LIBREPCB_CORE_TAGMATCHER_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../types/tagexpression.h"

#include <QtCore>

#include <set>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class TagMatcher
 ******************************************************************************/

/**
 * @brief Helper class to find the best match for given tag expressions
 */
class TagMatcher {
  Q_DECLARE_TR_FUNCTIONS(TagMatcher)

  struct Option {
    QSet<Tag> builtIn;
    QSet<Tag> userDefined;
  };

public:
  // Constructors / Destructor
  TagMatcher() = delete;
  TagMatcher(const TagMatcher& other) = delete;
  explicit TagMatcher(const QVector<TagConditional>& conditionals) noexcept;
  ~TagMatcher() noexcept;

  // General Methods
  int addOption(const QSet<Tag>& builtIn,
                const QSet<Tag>& userDefined) noexcept;
  int findFirstMatch() noexcept;

  // Operator Overloadings
  TagMatcher& operator=(const TagMatcher& rhs) = delete;

private:
  bool applyFilter(std::set<int>& filtered,
                   const TagConditional& conditional) const noexcept;

  const QVector<TagConditional>& mConditionals;
  QVector<Option> mOptions;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
