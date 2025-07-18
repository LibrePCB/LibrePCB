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

#ifndef LIBREPCB_EDITOR_UIHELPERS_H
#define LIBREPCB_EDITOR_UIHELPERS_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "appwindow.h"

#include <librepcb/core/library/pkg/package.h>
#include <librepcb/core/rulecheck/rulecheckmessage.h>
#include <librepcb/core/types/alignment.h>
#include <librepcb/core/types/length.h>
#include <librepcb/core/types/lengthunit.h>
#include <librepcb/core/types/ratio.h>
#include <librepcb/core/workspace/theme.h>

#include <optional>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

class EditorCommand;

// l2s(): LibrePCB C++ to LibrePCB Slint UI type conversion
// s2l(): LibrePCB Slint UI to LibrePCB C++ type conversion

/*******************************************************************************
 *  Non-Member Functions
 ******************************************************************************/

static_assert(sizeof(ui::Int64) == 8);
static_assert(sizeof(LengthBase_t) == 8);
static_assert(sizeof(Angle) == sizeof(int));
static_assert(sizeof(Ratio) == sizeof(int));

qint64 s2l(const ui::Int64& v) noexcept;

ui::Int64 l2s(const Length& v) noexcept;
Length s2length(const ui::Int64& v) noexcept;
std::optional<UnsignedLength> s2ulength(const ui::Int64& v) noexcept;
std::optional<PositiveLength> s2plength(const ui::Int64& v) noexcept;

int l2s(const Angle& v) noexcept;
Angle s2angle(int v) noexcept;

int l2s(const Ratio& v) noexcept;
Ratio s2ratio(int v) noexcept;

ui::GridStyle l2s(Theme::GridStyle v) noexcept;
Theme::GridStyle s2l(ui::GridStyle v) noexcept;

ui::LengthUnit l2s(const LengthUnit& v) noexcept;
LengthUnit s2l(ui::LengthUnit v) noexcept;

slint::cbindgen_private::TextHorizontalAlignment l2s(const HAlign& v) noexcept;
HAlign s2l(slint::cbindgen_private::TextHorizontalAlignment v) noexcept;

slint::cbindgen_private::TextVerticalAlignment l2s(const VAlign& v) noexcept;
VAlign s2l(slint::cbindgen_private::TextVerticalAlignment v) noexcept;

ui::NotificationType l2s(RuleCheckMessage::Severity v) noexcept;

int l2s(Package::AssemblyType v) noexcept;
std::optional<Package::AssemblyType> s2assemblyType(int v) noexcept;

ui::EditorCommand l2s(const EditorCommand& cmd, ui::EditorCommand in) noexcept;

ui::FeatureState toFs(bool enabled) noexcept;

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
