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

#ifndef LIBREPCB_EDITOR_RPCOBJECTREGISTRY_H
#define LIBREPCB_EDITOR_RPCOBJECTREGISTRY_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Class RpcObjectRegistry
 ******************************************************************************/

/**
 * @brief A registry of object handles for the RPC API
 *
 * RPC object handles are opaque numbers referring to objects of the
 * application (e.g. an open project). The registry assigns a new handle when
 * an object is embedded into a response for the first time, and returns the
 * same handle for the same object on every subsequent response. When the
 * object is destroyed, its handle becomes invalid automatically (the
 * `destroyed()` signal removes the registration).
 *
 * The registry has the same lifetime as the #RpcServer, i.e. all handles
 * become invalid when the server is stopped. The handle counter is static,
 * so handle values are never reused, not even across server restarts. Handle
 * value 0 is reserved (null handle).
 *
 * @note This class is not thread-safe. It is only used on the GUI thread.
 */
class RpcObjectRegistry final : public QObject {
  Q_OBJECT

public:
  // Constructors / Destructor
  RpcObjectRegistry(const RpcObjectRegistry& other) = delete;
  RpcObjectRegistry& operator=(const RpcObjectRegistry& rhs) = delete;
  explicit RpcObjectRegistry(QObject* parent = nullptr) noexcept;
  ~RpcObjectRegistry() noexcept override;

  /**
   * @brief Get the handle of an object, registering it if needed
   *
   * @param obj  The object to embed into a response as handle.
   *
   * @return The handle of the object (0 is never returned).
   */
  uint32_t getHandle(QObject& obj) noexcept;

  template <typename T>
  T* tryGetAs(const QVariant& handle) const {
    return qobject_cast<T*>(mObjects.value(handle.toUInt(), nullptr));
  }

  /**
   * @brief Look up the object referenced by a request's handle
   *
   * @param handle  The value of the request's reserved "obj" key.
   *
   * @return The referenced object, already cast to the expected type.
   *
   * @throws RpcInvalidHandle  If the handle is unknown or references an
   *                           object of another type.
   */
  template <typename T>
  T& getAs(const QVariant& handle) const {
    if (T* obj = tryGetAs<T>(handle)) {
      return *obj;
    }
    throwInvalidHandle();
  }

private:  // Methods
  void removeObject(QObject* obj) noexcept;

  /// Throw an InvalidHandle error for an unknown or expired handle.
  /// Defined in rpcobjectregistry.cpp (the only place allowed to include
  /// the generated error classes).
  [[noreturn]] static void throwInvalidHandle();

private:  // Data
  /// All registered objects, by handle.
  QHash<uint32_t, QObject*> mObjects;

  /// All handles, by object.
  QHash<QObject*, uint32_t> mHandles;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
