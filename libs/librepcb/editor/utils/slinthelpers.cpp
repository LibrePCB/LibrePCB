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
#include "slinthelpers.h"

#include <QtCore>
#include <QtGui>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Non-Member Functions
 ******************************************************************************/

slint::LogicalPosition q2s(const QPointF& p) noexcept {
  return slint::LogicalPosition(
      {static_cast<float>(p.x()), static_cast<float>(p.y())});
}

QPointF s2q(const slint::LogicalPosition& p) noexcept {
  return QPointF(p.x, p.y);
}

slint::PhysicalPosition q2s(const QPoint& p) noexcept {
  return slint::PhysicalPosition({p.x(), p.y()});
}

QPoint s2q(const slint::PhysicalPosition& p) noexcept {
  return QPoint(p.x, p.y);
}

slint::PhysicalSize q2s(const QSize& s) noexcept {
  return slint::PhysicalSize({static_cast<uint32_t>(std::max(s.width(), 0)),
                              static_cast<uint32_t>(std::max(s.height(), 0))});
}

QSize s2q(const slint::PhysicalSize& s) noexcept {
  return QSize(s.width, s.height);
}

slint::SharedString q2s(const QString& s) noexcept {
  return slint::SharedString(s.toUtf8().data());
}

QString s2q(const slint::SharedString& s) noexcept {
  std::string_view view(s);
  return QString::fromUtf8(view.data(), view.size());
}

std::shared_ptr<slint::VectorModel<slint::SharedString>> q2s(
    const QStringList& s) noexcept {
  auto m = std::make_shared<slint::VectorModel<slint::SharedString>>();
  for (const QString& item : s) {
    m->push_back(q2s(item));
  }
  return m;
}

QStringList s2q(const slint::Model<slint::SharedString>& s) noexcept {
  QStringList list;
  for (std::size_t i = 0; i < s.row_count(); ++i) {
    if (auto item = s.row_data(i)) {
      list.append(s2q(*item));
    }
  }
  return list;
}

bool operator==(const QString& s1, const slint::SharedString& s2) noexcept {
  return s1.toUtf8().data() == std::string_view(s2);
}

bool operator!=(const QString& s1, const slint::SharedString& s2) noexcept {
  return s1.toUtf8().data() != std::string_view(s2);
}

bool operator==(const slint::SharedString& s1, const QString& s2) noexcept {
  return std::string_view(s1) == s2.toUtf8().data();
}

bool operator!=(const slint::SharedString& s1, const QString& s2) noexcept {
  return std::string_view(s1) != s2.toUtf8().data();
}

slint::Image q2s(const QPixmap& p) noexcept {
  if (p.isNull()) {
    return slint::Image();
  }

  QImage img = p.toImage();
  img.convertTo(QImage::Format_RGBA8888);
  return slint::Image(slint::SharedPixelBuffer<slint::Rgba8Pixel>(
      img.width(), img.height(),
      reinterpret_cast<const slint::Rgba8Pixel*>(img.bits())));
}

slint::Color q2s(const QColor& c) noexcept {
  return slint::Color::from_argb_uint8(c.alpha(), c.red(), c.green(), c.blue());
}

slint::private_api::MouseCursor q2s(Qt::CursorShape s) noexcept {
  switch (s) {
    case Qt::ArrowCursor:
      return slint::private_api::MouseCursor::Default;
    case Qt::PointingHandCursor:
      return slint::private_api::MouseCursor::Pointer;
    case Qt::CrossCursor:
      return slint::private_api::MouseCursor::Crosshair;
    case Qt::ClosedHandCursor:
      return slint::private_api::MouseCursor::Grabbing;
    default: {
      qWarning() << "Unsupported cursor shape:" << s;
      return slint::private_api::MouseCursor::Default;
    }
  }
}

Qt::MouseButton s2q(const slint::private_api::PointerEventButton& b) noexcept {
  switch (b) {
    case slint::private_api::PointerEventButton::Left:
      return Qt::LeftButton;
    case slint::private_api::PointerEventButton::Right:
      return Qt::RightButton;
    case slint::private_api::PointerEventButton::Middle:
      return Qt::MiddleButton;
    case slint::private_api::PointerEventButton::Back:
      return Qt::BackButton;
    case slint::private_api::PointerEventButton::Forward:
      return Qt::ForwardButton;
    default:
      return Qt::NoButton;
  }
}

slint::private_api::KeyboardModifiers q2s(Qt::KeyboardModifiers m) noexcept {
  slint::private_api::KeyboardModifiers ret = {};
  ret.alt = m.testFlag(Qt::AltModifier);
  ret.control = m.testFlag(Qt::ControlModifier);
  ret.shift = m.testFlag(Qt::ShiftModifier);
  ret.meta = m.testFlag(Qt::MetaModifier);
  return ret;
}

Qt::KeyboardModifiers s2q(
    const slint::private_api::KeyboardModifiers& m) noexcept {
  Qt::KeyboardModifiers ret;
  ret.setFlag(Qt::ShiftModifier, m.shift);
  ret.setFlag(Qt::ControlModifier, m.control);
  ret.setFlag(Qt::AltModifier, m.alt);
  ret.setFlag(Qt::MetaModifier, m.meta);
  return ret;
}

slint::SharedString q2s(Qt::Key k) noexcept {
  switch (k) {
    case Qt::Key::Key_Backspace:
      return slint::platform::key_codes::Backspace;
    case Qt::Key::Key_Tab:
      return slint::platform::key_codes::Tab;
    case Qt::Key::Key_Enter:
    case Qt::Key::Key_Return:
      return slint::platform::key_codes::Return;
    case Qt::Key::Key_Escape:
      return slint::platform::key_codes::Escape;
    case Qt::Key::Key_Backtab:
      return slint::platform::key_codes::Backtab;
    case Qt::Key::Key_Delete:
      return slint::platform::key_codes::Delete;
    case Qt::Key::Key_Shift:
      return slint::platform::key_codes::Shift;
    case Qt::Key::Key_Control:
      return slint::platform::key_codes::Control;
    case Qt::Key::Key_Alt:
      return slint::platform::key_codes::Alt;
    case Qt::Key::Key_AltGr:
      return slint::platform::key_codes::AltGr;
    case Qt::Key::Key_CapsLock:
      return slint::platform::key_codes::CapsLock;
    case Qt::Key::Key_Meta:
      return slint::platform::key_codes::Meta;
    case Qt::Key::Key_Up:
      return slint::platform::key_codes::UpArrow;
    case Qt::Key::Key_Down:
      return slint::platform::key_codes::DownArrow;
    case Qt::Key::Key_Left:
      return slint::platform::key_codes::LeftArrow;
    case Qt::Key::Key_Right:
      return slint::platform::key_codes::RightArrow;
    case Qt::Key::Key_F1:
      return slint::platform::key_codes::F1;
    case Qt::Key::Key_F2:
      return slint::platform::key_codes::F2;
    case Qt::Key::Key_F3:
      return slint::platform::key_codes::F3;
    case Qt::Key::Key_F4:
      return slint::platform::key_codes::F4;
    case Qt::Key::Key_F5:
      return slint::platform::key_codes::F5;
    case Qt::Key::Key_F6:
      return slint::platform::key_codes::F6;
    case Qt::Key::Key_F7:
      return slint::platform::key_codes::F7;
    case Qt::Key::Key_F8:
      return slint::platform::key_codes::F8;
    case Qt::Key::Key_F9:
      return slint::platform::key_codes::F9;
    case Qt::Key::Key_F10:
      return slint::platform::key_codes::F10;
    case Qt::Key::Key_F11:
      return slint::platform::key_codes::F11;
    case Qt::Key::Key_F12:
      return slint::platform::key_codes::F12;
    case Qt::Key::Key_F13:
      return slint::platform::key_codes::F13;
    case Qt::Key::Key_F14:
      return slint::platform::key_codes::F14;
    case Qt::Key::Key_F15:
      return slint::platform::key_codes::F15;
    case Qt::Key::Key_F16:
      return slint::platform::key_codes::F16;
    case Qt::Key::Key_F17:
      return slint::platform::key_codes::F17;
    case Qt::Key::Key_F18:
      return slint::platform::key_codes::F18;
    case Qt::Key::Key_F19:
      return slint::platform::key_codes::F19;
    case Qt::Key::Key_F20:
      return slint::platform::key_codes::F20;
    case Qt::Key::Key_F21:
      return slint::platform::key_codes::F21;
    case Qt::Key::Key_F22:
      return slint::platform::key_codes::F22;
    case Qt::Key::Key_F23:
      return slint::platform::key_codes::F23;
    case Qt::Key::Key_F24:
      return slint::platform::key_codes::F24;
    case Qt::Key::Key_Insert:
      return slint::platform::key_codes::Insert;
    case Qt::Key::Key_Home:
      return slint::platform::key_codes::Home;
    case Qt::Key::Key_End:
      return slint::platform::key_codes::End;
    case Qt::Key::Key_PageUp:
      return slint::platform::key_codes::PageUp;
    case Qt::Key::Key_PageDown:
      return slint::platform::key_codes::PageDown;
    case Qt::Key::Key_ScrollLock:
      return slint::platform::key_codes::ScrollLock;
    case Qt::Key::Key_Pause:
      return slint::platform::key_codes::Pause;
    case Qt::Key::Key_SysReq:
      return slint::platform::key_codes::SysReq;
    case Qt::Key::Key_Stop:
      return slint::platform::key_codes::Stop;
    case Qt::Key::Key_Menu:
      return slint::platform::key_codes::Menu;
    default: {
      const QString s = QKeySequence(k).toString().toLower();
      if (s.isEmpty()) {
        qWarning() << "Unknown Qt key:" << k;
      }
      return q2s(s);
    }
  }
}

static slint::SharedString getInputError(const QString& input) {
  if (input.trimmed().isEmpty()) {
    return q2s(QCoreApplication::translate("SlintHelpers", "Required"));
  } else {
    return q2s(QCoreApplication::translate("SlintHelpers", "Invalid"));
  }
}

static slint::SharedString getDuplicateError() {
  return q2s(QCoreApplication::translate("SlintHelpers", "Duplicate"));
}

static slint::SharedString getRecommendedError() {
  return q2s(QCoreApplication::translate("SlintHelpers", "Recommended"));
}

std::optional<ElementName> validateElementName(
    const QString& input, slint::SharedString& error) noexcept {
  if (auto val = parseElementName(cleanElementName(input))) {
    error = slint::SharedString();
    return val;
  } else {
    error = getInputError(input);
    return std::nullopt;
  }
}

std::optional<Version> validateVersion(const QString& input,
                                       slint::SharedString& error) noexcept {
  if (auto val = Version::tryFromString(input.trimmed())) {
    error = slint::SharedString();
    return val;
  } else {
    error = getInputError(input);
    return std::nullopt;
  }
}

std::optional<FileProofName> validateFileProofName(
    const QString& input, slint::SharedString& error,
    const QString& requiredSuffix) noexcept {
  if (auto val = parseFileProofName(cleanFileProofName(input))) {
    if (requiredSuffix.isEmpty() || input.trimmed().endsWith(requiredSuffix)) {
      error = slint::SharedString();
      return val;
    } else {
      error = q2s(
          QCoreApplication::translate("FileProofName", "Suffix '%1' missing")
              .arg(requiredSuffix));
      return std::nullopt;
    }
  } else {
    error = getInputError(input);
    return std::nullopt;
  }
}

std::optional<AttributeKey> validateAttributeKey(const QString& input,
                                                 slint::SharedString& error,
                                                 bool isDuplicate) noexcept {
  auto val = parseAttributeKey(cleanAttributeKey(input));
  if (isDuplicate) {
    error = getDuplicateError();
  } else if (val) {
    error = slint::SharedString();
  } else {
    error = getInputError(input);
  }
  return val;
}

std::optional<CircuitIdentifier> validateCircuitIdentifier(
    const QString& input, slint::SharedString& error,
    bool isDuplicate) noexcept {
  auto val = parseCircuitIdentifier(cleanCircuitIdentifier(input));
  if (isDuplicate) {
    error = getDuplicateError();
  } else if (val) {
    error = slint::SharedString();
  } else {
    error = getInputError(input);
  }
  return val;
}

std::optional<QUrl> validateUrl(const QString& input,
                                slint::SharedString& error,
                                bool allowEmpty) noexcept {
  const QUrl url = QUrl::fromUserInput(input.trimmed());
  const std::optional<QUrl> val =
      url.isValid() ? std::make_optional(url) : std::nullopt;
  if (val || (allowEmpty && input.trimmed().isEmpty())) {
    error = slint::SharedString();
    return val;
  } else {
    error = getInputError(input);
    return std::nullopt;
  }
}

std::optional<ComponentPrefix> validateComponentPrefix(
    const QString& input, slint::SharedString& error) noexcept {
  auto val = parseComponentPrefix(cleanComponentPrefix(input));
  if (val && (!input.trimmed().isEmpty())) {
    error = slint::SharedString();
  } else if (!input.trimmed().isEmpty()) {
    error = getInputError(input);
  } else {
    error = getRecommendedError();
  }
  return val;
}

void validateComponentDefaultValue(const QString& input,
                                   slint::SharedString& error) noexcept {
  if (!input.trimmed().isEmpty()) {
    error = slint::SharedString();
  } else {
    error = getRecommendedError();
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
