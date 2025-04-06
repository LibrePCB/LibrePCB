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

#ifndef LIBREPCB_EDITOR_SLINTKEYEVENTTEXTBUILDER_H
#define LIBREPCB_EDITOR_SLINTKEYEVENTTEXTBUILDER_H

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
 *  Class SlintKeyEventTextBuilder
 ******************************************************************************/

/**
 * @brief The SlintKeyEventTextBuilder class
 */
class SlintKeyEventTextBuilder final : public QObject {
  Q_OBJECT

public:
  // Constructors / Destructor
  SlintKeyEventTextBuilder(const SlintKeyEventTextBuilder& other) = delete;
  explicit SlintKeyEventTextBuilder(QObject* parent = nullptr) noexcept;
  ~SlintKeyEventTextBuilder() noexcept;

  // General Methods
  const QString& getText() const noexcept { return mText; }
  slint::private_api::EventResult process(
      const slint::private_api::KeyEvent& e) noexcept;

  // Operator Overloadings
  SlintKeyEventTextBuilder& operator=(const SlintKeyEventTextBuilder& rhs) =
      delete;

signals:
  void textChanged(QString text);

private:
  QString mText;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
