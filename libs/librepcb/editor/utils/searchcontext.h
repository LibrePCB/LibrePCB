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

#ifndef LIBREPCB_EDITOR_SEARCHCONTEXT_H
#define LIBREPCB_EDITOR_SEARCHCONTEXT_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <QtCore>

#include <slint.h>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Class SearchContext
 ******************************************************************************/

/**
 * @brief The SearchContext class
 */
class SearchContext final : public QObject {
  Q_OBJECT

public:
  // Constructors / Destructor
  SearchContext(const SearchContext& other) = delete;
  explicit SearchContext(QObject* parent = nullptr) noexcept;
  ~SearchContext() noexcept;

  // General Methods
  void init() noexcept;
  void deinit() noexcept;
  void setTerm(const QString& term) noexcept;
  const QString& getTerm() const noexcept { return mTerm; }
  void setSuggestions(const QStringList& list) noexcept;
  const std::shared_ptr<slint::FilterModel<slint::SharedString>>&
      getSuggestions() const noexcept {
    return mSuggestionsFiltered;
  }
  void findNext() noexcept;
  void findPrevious() noexcept;

  // Operator Overloadings
  SearchContext& operator=(const SearchContext& rhs) = delete;

signals:
  void goToTriggered(const QString& name, int index = 0);

private:
  QString mTerm;
  bool mForward;  ///< Current search direction (forward or backward)
  int mIndex;  ///< Number of searches with the current search term

  std::shared_ptr<slint::VectorModel<slint::SharedString>> mSuggestions;
  std::shared_ptr<slint::FilterModel<slint::SharedString>> mSuggestionsFiltered;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
