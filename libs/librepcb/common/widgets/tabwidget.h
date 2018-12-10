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

#ifndef LIBREPCB_TABWIDGET_H
#define LIBREPCB_TABWIDGET_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class TabWidget
 ******************************************************************************/

/**
 * @brief A QTabWidget subclass that allows closing closable tabs
 * with the middle mouse button.
 *
 * @author dbrgn
 * @date 2018-12-07
 */
class TabWidget final : public QTabWidget {
  Q_OBJECT

public:
  // Constructors / Destructor
  explicit TabWidget(QWidget* parent = nullptr) noexcept;
  TabWidget(const TabWidget& other) = delete;

  // Operator Overloadings
  TabWidget& operator=(const TabWidget& rhs) = delete;

private:
  // Event handlers
  virtual bool eventFilter(QObject* o, QEvent* e) override;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif  // LIBREPCB_TABWIDGET_H
