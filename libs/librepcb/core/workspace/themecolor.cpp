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
#include "themecolor.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

ThemeColor::ThemeColor(const QString& identifier, const QString& categoryTr,
                       const QString& nameTr, const QColor& primary,
                       const QColor& secondary) noexcept
  : mIdentifier(identifier),
    mCategoryTr(categoryTr),
    mNameTr(nameTr),
    mPrimary(primary),
    mSecondary(secondary),
    mEdited(false) {
}

ThemeColor::ThemeColor(const ThemeColor& other) noexcept
  : mIdentifier(other.mIdentifier),
    mCategoryTr(other.mCategoryTr),
    mNameTr(other.mNameTr),
    mPrimary(other.mPrimary),
    mSecondary(other.mSecondary),
    mEdited(other.mEdited) {
}

ThemeColor::~ThemeColor() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void ThemeColor::setPrimaryColor(const QColor& color) noexcept {
  if (color.isValid() == mPrimary.isValid()) {
    mPrimary = color;
    mEdited = true;
  }
}

void ThemeColor::setSecondaryColor(const QColor& color) noexcept {
  if (color.isValid() == mSecondary.isValid()) {
    mSecondary = color;
    mEdited = true;
  }
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void ThemeColor::load(const SExpression& root) {
  auto tryLoadColor = [&root](QColor& dst, const QString& path) {
    if (const SExpression* node = root.tryGetChild(path)) {
      const QColor color = deserialize<QColor>(*node);
      if (color.isValid() == dst.isValid()) {
        dst = color;
      }
    }
  };

  tryLoadColor(mPrimary, "primary/@0");
  tryLoadColor(mSecondary, "secondary/@0");
}

SExpression ThemeColor::serialize() const {
  SExpression root = SExpression::createList(mIdentifier);
  root.appendChild("primary", mPrimary);
  if (mSecondary.isValid()) {
    root.appendChild("secondary", mSecondary);
  }
  return root;
}

/*******************************************************************************
 *  Operator Overloadings
 ******************************************************************************/

bool ThemeColor::operator==(const ThemeColor& rhs) const noexcept {
  return (mIdentifier == rhs.mIdentifier)  //
      && (mPrimary == rhs.mPrimary)  //
      && (mSecondary == rhs.mSecondary)  //
      && (mEdited == rhs.mEdited)  //
      ;
}

ThemeColor& ThemeColor::operator=(const ThemeColor& rhs) noexcept {
  mIdentifier = rhs.mIdentifier;
  mCategoryTr = rhs.mCategoryTr;
  mNameTr = rhs.mNameTr;
  mPrimary = rhs.mPrimary;
  mSecondary = rhs.mSecondary;
  mEdited = rhs.mEdited;
  return *this;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
