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

#ifndef LIBREPCB_EDITOR_SLINTHELPERS_H
#define LIBREPCB_EDITOR_SLINTHELPERS_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/core/attribute/attributekey.h>
#include <librepcb/core/library/cmp/componentprefix.h>
#include <librepcb/core/types/circuitidentifier.h>
#include <librepcb/core/types/elementname.h>
#include <librepcb/core/types/fileproofname.h>
#include <librepcb/core/types/version.h>

#include <QtCore>
#include <QtGui>

#include <optional>
#include <slint.h>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

// q2s(): Qt to Slint conversion (only built-in types, not LibrePCB specific)
// s2q(): Slint to Qt conversion (only built-in types, not LibrePCB specific)

/*******************************************************************************
 *  Non-Member Functions
 ******************************************************************************/

slint::LogicalPosition q2s(const QPointF& p) noexcept;
QPointF s2q(const slint::LogicalPosition& p) noexcept;

slint::PhysicalPosition q2s(const QPoint& p) noexcept;
QPoint s2q(const slint::PhysicalPosition& p) noexcept;

slint::PhysicalSize q2s(const QSize& s) noexcept;
QSize s2q(const slint::PhysicalSize& s) noexcept;

slint::SharedString q2s(const QString& s) noexcept;
QString s2q(const slint::SharedString& s) noexcept;
bool operator==(const QString& s1, const slint::SharedString& s2) noexcept;
bool operator!=(const QString& s1, const slint::SharedString& s2) noexcept;
bool operator==(const slint::SharedString& s1, const QString& s2) noexcept;
bool operator!=(const slint::SharedString& s1, const QString& s2) noexcept;

std::shared_ptr<slint::VectorModel<slint::SharedString>> q2s(
    const QStringList& s) noexcept;
QStringList s2q(const slint::Model<slint::SharedString>& s) noexcept;

slint::Image q2s(const QPixmap& p) noexcept;

slint::Color q2s(const QColor& c) noexcept;

slint::private_api::MouseCursor q2s(Qt::CursorShape s) noexcept;

Qt::MouseButton s2q(const slint::private_api::PointerEventButton& b) noexcept;

slint::private_api::KeyboardModifiers q2s(Qt::KeyboardModifiers m) noexcept;
Qt::KeyboardModifiers s2q(
    const slint::private_api::KeyboardModifiers& m) noexcept;

slint::SharedString q2s(Qt::Key k) noexcept;

// Bind property without type conversion
template <typename TTarget, typename TClass, typename T>
inline void bind(QObject* context, const TTarget& target,
                 void (TTarget::*setter)(const T&) const, TClass* source,
                 void (TClass::*signal)(T), const T& defaultValue) noexcept {
  QObject::connect(source, signal, context, [&target, setter](const T& value) {
    (target.*setter)(value);
  });
  (target.*setter)(defaultValue);
}

// Bind property with type conversion
template <typename TTarget, typename TSlint, typename TClass, typename TQt>
inline void bind(
    QObject* context, const TTarget& target,
    void (TTarget::*setter)(const TSlint&) const, TClass* source,
    void (TClass::*signal)(TQt), const TQt& defaultValue,
    std::function<TSlint(const TQt&)> convert = [](const TQt& value) {
      return q2s(value);
    }) noexcept {
  QObject::connect(source, signal, context,
                   [&target, setter, convert](const TQt& value) {
                     (target.*setter)(convert(value));
                   });
  (target.*setter)(convert(defaultValue));
}

std::optional<ElementName> validateElementName(
    const QString& input, slint::SharedString& error) noexcept;

std::optional<Version> validateVersion(const QString& input,
                                       slint::SharedString& error) noexcept;

std::optional<FileProofName> validateFileProofName(
    const QString& input, slint::SharedString& error,
    const QString& requiredSuffix = QString()) noexcept;

std::optional<AttributeKey> validateAttributeKey(const QString& input,
                                                 slint::SharedString& error,
                                                 bool isDuplicate) noexcept;

std::optional<CircuitIdentifier> validateCircuitIdentifier(
    const QString& input, slint::SharedString& error,
    bool isDuplicate) noexcept;

std::optional<QUrl> validateUrl(const QString& input,
                                slint::SharedString& error,
                                bool allowEmpty = false) noexcept;

std::optional<ComponentPrefix> validateComponentPrefix(
    const QString& input, slint::SharedString& error) noexcept;

void validateComponentDefaultValue(const QString& input,
                                   slint::SharedString& error) noexcept;

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
