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
#include "slintkeyeventtextbuilder.h"

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

SlintKeyEventTextBuilder::SlintKeyEventTextBuilder(QObject* parent) noexcept
  : QObject(parent), mText() {
}

SlintKeyEventTextBuilder::~SlintKeyEventTextBuilder() noexcept {
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

slint::private_api::EventResult SlintKeyEventTextBuilder::process(
    const slint::private_api::KeyEvent& e) noexcept {
  if (e.event_type != slint::private_api::KeyEventType::KeyPressed) {
    return slint::private_api::EventResult::Reject;
  }

  const QString text(s2q(e.text));
  if (text.size() != 1) {
    return slint::private_api::EventResult::Reject;
  }
  const QChar c = text.front();

  if ((c == '\x1b') && (mText.size() > 0)) {
    mText.clear();
    emit textChanged(mText);
    return slint::private_api::EventResult::Accept;
  } else if ((c == '\b') && (mText.size() > 0)) {
    mText.chop(1);
    emit textChanged(mText);
    return slint::private_api::EventResult::Accept;
  } else if (c.isPrint()) {
    mText += c;
    emit textChanged(mText);
    return slint::private_api::EventResult::Accept;
  } else {
    return slint::private_api::EventResult::Reject;
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
