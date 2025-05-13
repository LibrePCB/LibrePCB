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
#include "searchcontext.h"

#include "slinthelpers.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

SearchContext::SearchContext(QObject* parent) noexcept
  : QObject(parent), mTerm(), mForward(true), mIndex(0) {
}

SearchContext::~SearchContext() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void SearchContext::init() noexcept {
  mTerm.clear();
  mForward = true;
  mIndex = 0;
  mSuggestions.reset(new slint::VectorModel<slint::SharedString>());
  QPointer<SearchContext> ctx = this;
  mSuggestionsFiltered.reset(new slint::FilterModel<slint::SharedString>(
      mSuggestions, [ctx](const slint::SharedString& data) {
        return ctx && s2q(data).toLower().startsWith(ctx->mTerm.toLower());
      }));
}

void SearchContext::deinit() noexcept {
  mSuggestionsFiltered.reset();
  mSuggestions.reset();
}

void SearchContext::setTerm(const QString& term) noexcept {
  if (term != mTerm) {
    mTerm = term.trimmed();
    mIndex = 0;
    mForward = true;
    if (mSuggestionsFiltered) {
      mSuggestionsFiltered->reset();
    }
  }
}

void SearchContext::setSuggestions(const QStringList& list) noexcept {
  if (mSuggestions) {
    std::vector<slint::SharedString> vector;
    vector.reserve(list.size());
    for (const auto& s : list) {
      vector.push_back(q2s(s));
    }
    mSuggestions->set_vector(vector);
  }
}

void SearchContext::findNext() noexcept {
  if (!mForward) {
    mForward = true;
    mIndex += 2;
  }
  emit goToTriggered(mTerm, mIndex);
  ++mIndex;
}

void SearchContext::findPrevious() noexcept {
  if (mForward) {
    mForward = false;
    mIndex -= 2;
  }
  emit goToTriggered(mTerm, mIndex);
  --mIndex;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
