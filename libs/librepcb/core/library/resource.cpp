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
#include "resource.h"

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

Resource::Resource(const Resource& other) noexcept
  : onEdited(*this),
    mName(other.mName),
    mMediaType(other.mMediaType),
    mUrl(other.mUrl) {
}

Resource::Resource(const SExpression& node)
  : onEdited(*this),
    mName(deserialize<ElementName>(node.getChild("@0"))),
    mMediaType(node.getChild("mediatype/@0").getValue()),
    // Don't use deserialize<QUrl>() to avoid any exception thrown.
    mUrl(node.getChild("url/@0").getValue(), QUrl::StrictMode) {
}

Resource::Resource(const ElementName& name, const QString& mimeType,
                   const QUrl& url) noexcept
  : onEdited(*this), mName(name), mMediaType(mimeType), mUrl(url) {
}

Resource::~Resource() noexcept {
}

/*******************************************************************************
 *  Setters
 ******************************************************************************/

void Resource::setName(const ElementName& name) noexcept {
  if (name != mName) {
    mName = name;
    onEdited.notify(Event::NameChanged);
  }
}

void Resource::setMediaType(const QString& type) noexcept {
  if (type != mMediaType) {
    mMediaType = type;
    onEdited.notify(Event::MediaTypeChanged);
  }
}

void Resource::setUrl(const QUrl& url) noexcept {
  if (url != mUrl) {
    mUrl = url;
    onEdited.notify(Event::UrlChanged);
  }
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void Resource::serialize(SExpression& root) const {
  root.appendChild(mName);
  root.appendChild("mediatype", mMediaType);
  root.ensureLineBreak();
  root.appendChild("url", mUrl.toString(QUrl::PrettyDecoded));
  root.ensureLineBreak();
}

/*******************************************************************************
 *  Operator Overloadings
 ******************************************************************************/

bool Resource::operator==(const Resource& rhs) const noexcept {
  if (mName != rhs.mName) return false;
  if (mMediaType != rhs.mMediaType) return false;
  if (mUrl != rhs.mUrl) return false;
  return true;
}

Resource& Resource::operator=(const Resource& rhs) noexcept {
  setName(rhs.mName);
  setMediaType(rhs.mMediaType);
  setUrl(rhs.mUrl);
  return *this;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
