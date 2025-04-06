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

int q2s(int i) noexcept;

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

slint::Image q2s(const QPixmap& p) noexcept;

slint::Color q2s(const QColor& c) noexcept;

slint::private_api::MouseCursor q2s(Qt::CursorShape s) noexcept;

Qt::MouseButton s2q(const slint::private_api::PointerEventButton& b) noexcept;

slint::private_api::KeyboardModifiers q2s(Qt::KeyboardModifiers m) noexcept;
Qt::KeyboardModifiers s2q(
    const slint::private_api::KeyboardModifiers& m) noexcept;

slint::SharedString q2s(Qt::Key k) noexcept;

std::optional<ElementName> validateElementName(
    const QString& input, slint::SharedString& error) noexcept;

std::optional<Version> validateVersion(const QString& input,
                                       slint::SharedString& error) noexcept;

std::optional<FileProofName> validateFileProofName(
    const QString& input, slint::SharedString& error,
    const QString& requiredSuffix = QString()) noexcept;

std::optional<QUrl> validateUrl(const QString& input,
                                slint::SharedString& error,
                                bool allowEmpty = false) noexcept;

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
