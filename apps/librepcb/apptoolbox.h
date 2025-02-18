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

#ifndef LIBREPCB_APPTOOLBOX_H
#define LIBREPCB_APPTOOLBOX_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/core/types/elementname.h>
#include <librepcb/core/types/fileproofname.h>
#include <librepcb/core/types/version.h>
#include <librepcb/core/workspace/theme.h>

#include <QtCore>
#include <QtGui>

#include <slint.h>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {
namespace app {

/*******************************************************************************
 *  Non-Member Functions
 ******************************************************************************/

int q2s(int i) noexcept;
slint::PhysicalPosition q2s(const QPoint& p) noexcept;
slint::PhysicalSize q2s(const QSize& s) noexcept;
slint::SharedString q2s(const QString& s) noexcept;
slint::Image q2s(const QPixmap& p) noexcept;
slint::Color q2s(const QColor& c) noexcept;
slint::cbindgen_private::MouseCursor q2s(Qt::CursorShape s) noexcept;

QPoint s2q(const slint::PhysicalPosition& p) noexcept;
QSize s2q(const slint::PhysicalSize& s) noexcept;
QString s2q(const slint::SharedString& s) noexcept;
bool operator==(const QString& s1, const slint::SharedString& s2) noexcept;
bool operator!=(const QString& s1, const slint::SharedString& s2) noexcept;
bool operator==(const slint::SharedString& s1, const QString& s2) noexcept;
bool operator!=(const slint::SharedString& s1, const QString& s2) noexcept;

template <typename TTarget, typename TSlint, typename TClass, typename TQt>
static void bind(
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

std::optional<QUrl> validateUrl(const QString& input,
                                slint::SharedString& error,
                                bool allowEmpty = false) noexcept;

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace app
}  // namespace editor
}  // namespace librepcb

#endif
