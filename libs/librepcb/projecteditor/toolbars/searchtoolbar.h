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

#ifndef LIBREPCB_PROJECTEDITOR_SEARCHTOOLBAR_H
#define LIBREPCB_PROJECTEDITOR_SEARCHTOOLBAR_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <QtCore>
#include <QtWidgets>

#include <functional>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace project {
namespace editor {

/*******************************************************************************
 *  Class SearchToolBar
 ******************************************************************************/

/**
 * @brief The SearchToolBar class
 */
class SearchToolBar final : public QToolBar {
  Q_OBJECT

public:
  typedef std::function<QStringList()> CompleterListFunction;

  // Constructors / Destructor
  SearchToolBar(const SearchToolBar& other) = delete;
  explicit SearchToolBar(QWidget* parent = nullptr) noexcept;
  ~SearchToolBar() noexcept;

  // Setters
  void setPlaceholderText(const QString& text) noexcept {
    mLineEdit->setPlaceholderText(text);
  }
  void setCompleterListFunction(CompleterListFunction fun) noexcept {
    mCompleterListFunction = fun;
  }

  // Operator Overloadings
  SearchToolBar& operator=(const SearchToolBar& rhs) = delete;

signals:
  void goToTriggered(const QString& name, unsigned int index = 0);

private:
  void updateCompleter() noexcept;
  void textEdited(const QString& text) noexcept;
  void enterPressed() noexcept;

private:
  CompleterListFunction mCompleterListFunction;
  QScopedPointer<QLineEdit> mLineEdit;
  unsigned int mIndex;  ///< Number of searches with the current search term
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace project
}  // namespace librepcb

#endif
