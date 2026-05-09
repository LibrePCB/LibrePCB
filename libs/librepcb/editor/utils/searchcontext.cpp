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

#include "editortoolbox.h"
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
  mTermRe = std::nullopt;
  mForward = true;
  mIndex = 0;
  mCandidates.clear();
  mCandidatesFiltered.clear();
  mModel = std::make_shared<slint::VectorModel<ui::SimpleListItemData>>();
}

void SearchContext::deinit() noexcept {
  mModel.reset();
  mCandidatesFiltered.clear();
  mCandidates.clear();
}

void SearchContext::setTerm(const QString& term) noexcept {
  if (term != mTerm) {
    mTerm = term.trimmed();
    if (mTerm.contains("*")) {
      mTermRe = QRegularExpression::fromWildcard(mTerm, Qt::CaseInsensitive);
    } else {
      mTermRe = std::nullopt;
    }
    mIndex = 0;
    mForward = true;
    applyFilter();
  }
}

void SearchContext::setCandidates(const QVector<Candidate>& list) noexcept {
  mCandidates = list;
  Toolbox::sortNumeric(
      mCandidates,
      [](const QCollator& cmp, const Candidate& a, const Candidate& b) {
        if (a.type != b.type) {
          return a.type < b.type;
        } else {
          return cmp(a.name, b.name);
        }
      });
  applyFilter();
}

void SearchContext::findNext() noexcept {
  if (!mForward) {
    mForward = true;
    mIndex += 2;
  }
  if (mCandidatesFiltered.isEmpty() || mTermRe) {
    emit goToTriggered(mCandidatesFiltered);
  } else if (!mTerm.isEmpty()) {
    mIndex %= mCandidatesFiltered.count();
    emit goToTriggered(mCandidatesFiltered.mid(mIndex, 1));
    ++mIndex;
  } else {
    emit goToTriggered({});
  }
}

void SearchContext::findPrevious() noexcept {
  if (mForward) {
    mForward = false;
    mIndex -= 2;
  }
  if (mCandidatesFiltered.isEmpty() || mTermRe) {
    emit goToTriggered(mCandidatesFiltered);
  } else if (!mTerm.isEmpty()) {
    mIndex %= mCandidatesFiltered.count();
    emit goToTriggered(mCandidatesFiltered.mid(mIndex, 1));
    --mIndex;
  } else {
    emit goToTriggered({});
  }
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void SearchContext::applyFilter() noexcept {
  if (!mModel) return;

  const QHash<ObjectType, slint::Image> images = {
      {ObjectType::Component,
       q2s(EditorToolbox::svgIcon(":/bi/cpu.svg").pixmap(48, 48))},
      {ObjectType::Net,
       q2s(EditorToolbox::svgIcon(":/fa/solid/circle-nodes.svg")
               .pixmap(48, 48))},
  };

  QVector<Candidate> filtered;
  if (mTerm.isEmpty()) {
    filtered = mCandidates;
  } else {
    filtered.reserve(mCandidates.count());
    for (const auto& candidate : std::as_const(mCandidates)) {
      if ((mTermRe && mTermRe->match(candidate.name).hasMatch()) ||
          ((!mTermRe) && candidate.name.contains(mTerm, Qt::CaseInsensitive))) {
        filtered.append(candidate);
      }
    }

    // Move best matches to top, to ensure the first result is always the best.
    // For example, the net "RXD" must appear before "MCU_RXD" if the search
    // term "rxd" was entered. Or "RST" must appear before "!RST" when searching
    // for "rst".
    std::stable_sort(
        filtered.begin(), filtered.end(),
        [this](const Candidate& a, const Candidate& b) {
          bool aMatch = a.name.compare(mTerm, Qt::CaseInsensitive) == 0;
          bool bMatch = b.name.compare(mTerm, Qt::CaseInsensitive) == 0;
          if (aMatch != bMatch) {
            return aMatch;
          }
          aMatch = a.name.startsWith(mTerm, Qt::CaseInsensitive);
          bMatch = b.name.startsWith(mTerm, Qt::CaseInsensitive);
          if (aMatch != bMatch) {
            return aMatch;
          }
          return false;
        });
  }

  if (filtered != mCandidatesFiltered) {
    mCandidatesFiltered = filtered;
    std::vector<ui::SimpleListItemData> vector;
    vector.reserve(mCandidatesFiltered.size());
    for (const auto& candidate : std::as_const(mCandidatesFiltered)) {
      vector.push_back(ui::SimpleListItemData{images.value(candidate.type),
                                              q2s(candidate.name)});
    }
    mModel->set_vector(vector);
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
