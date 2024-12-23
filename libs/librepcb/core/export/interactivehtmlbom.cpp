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
#include "interactivehtmlbom.h"

#include "../exceptions.h"

#include <librepcb/rust-core/ffi.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class InteractiveHtmlBom
 ******************************************************************************/

static RustHandle<rs::InteractiveHtmlBom> construct(const QString& title,
                                                    const QString& revision,
                                                    const QString& company,
                                                    const QString& date) {
  if (auto obj = rs::ffi_ibom_new(&title, &revision, &company, &date)) {
    return RustHandle<rs::InteractiveHtmlBom>(*obj, &rs::ffi_ibom_delete);
  } else {
    throw RuntimeError(__FILE__, __LINE__,
                       QString("Failed to create InteractiveHtmlBom"));
  }
}

InteractiveHtmlBom::InteractiveHtmlBom(const QString& title,
                                       const QString& revision,
                                       const QString& company,
                                       const QString& date)
  : mHandle(construct(title, revision, company, date)) {
}

QString InteractiveHtmlBom::generate() const {
  QString out;
  rs::ffi_ibom_generate(*mHandle, &out);
  return out;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
