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

#ifndef LIBREPCB_CORE_RESOURCE_H
#define LIBREPCB_CORE_RESOURCE_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../serialization/serializableobjectlist.h"
#include "../types/elementname.h"

#include <QtCore>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class Resource
 ******************************************************************************/

/**
 * @brief The Resource class
 */
class Resource final {
  Q_DECLARE_TR_FUNCTIONS(Resource)

public:
  // Signals
  enum class Event {
    NameChanged,
    MediaTypeChanged,
    UrlChanged,
  };
  Signal<Resource, Event> onEdited;
  typedef Slot<Resource, Event> OnEditedSlot;

  // Constructors / Destructor
  Resource() = delete;
  Resource(const Resource& other) noexcept;
  explicit Resource(const SExpression& node);
  Resource(const ElementName& name, const QString& mimeType,
           const QUrl& url) noexcept;
  ~Resource() noexcept;

  // Getters
  const ElementName& getName() const noexcept { return mName; }
  const QString& getMediaType() const noexcept { return mMediaType; }
  const QUrl& getUrl() const noexcept { return mUrl; }

  // Setters
  void setName(const ElementName& name) noexcept;
  void setMediaType(const QString& type) noexcept;
  void setUrl(const QUrl& url) noexcept;

  // General Methods

  /**
   * @brief Serialize into ::librepcb::SExpression node
   *
   * @param root    Root node to serialize into.
   */
  void serialize(SExpression& root) const;

  // Operator Overloadings
  bool operator==(const Resource& rhs) const noexcept;
  bool operator!=(const Resource& rhs) const noexcept {
    return !(*this == rhs);
  }
  Resource& operator=(const Resource& rhs) noexcept;

private:  // Data
  ElementName mName;
  QString mMediaType;
  QUrl mUrl;
};

/*******************************************************************************
 *  Class ResourceList
 ******************************************************************************/

struct ResourceListNameProvider {
  static constexpr const char* tagname = "resource";
};
using ResourceList =
    SerializableObjectList<Resource, ResourceListNameProvider, Resource::Event>;

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
