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
#include "ui.h"

#include <QtCore>

#include <memory>
#include <optional>

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
  // Types
  enum class ObjectType {
    Component,
    Net,
  };
  struct Candidate {
    ObjectType type;
    QString name;
    bool operator==(const Candidate& rhs) const noexcept = default;
  };

  // Constructors / Destructor
  SearchContext(const SearchContext& other) = delete;
  explicit SearchContext(QObject* parent = nullptr) noexcept;
  ~SearchContext() noexcept override;

  // General Methods
  void init() noexcept;
  void deinit() noexcept;
  void setTerm(const QString& term) noexcept;
  const QString& getTerm() const noexcept { return mTerm; }
  void setCandidates(const QVector<Candidate>& list) noexcept;
  const std::shared_ptr<slint::VectorModel<ui::SimpleListItemData>>& getModel()
      const noexcept {
    return mModel;
  }
  void findNext() noexcept;
  void findPrevious() noexcept;

  // Operator Overloadings
  SearchContext& operator=(const SearchContext& rhs) = delete;

signals:
  void goToTriggered(const QVector<Candidate>& objects);

private:
  void applyFilter() noexcept;

private:
  QString mTerm;
  std::optional<QRegularExpression> mTermRe;  ///< If #mTerm contains wildcards
  bool mForward;  ///< Current search direction (forward or backward)
  int mIndex;  ///< Number of searches with the current search term
  QVector<Candidate> mCandidates;
  QVector<Candidate> mCandidatesFiltered;
  std::shared_ptr<slint::VectorModel<ui::SimpleListItemData>> mModel;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
