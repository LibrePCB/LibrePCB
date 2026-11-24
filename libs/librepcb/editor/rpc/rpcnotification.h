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

#ifndef LIBREPCB_EDITOR_RPCNOTIFICATION_H
#define LIBREPCB_EDITOR_RPCNOTIFICATION_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <QtCore>

#include <algorithm>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Class RpcNotification
 ******************************************************************************/

/**
 * @brief The payload of a notification sent with RpcRequest::send()
 *
 * A builder for the notification payload (see the protocol documentation):
 * The members @c progress, @c step, @c steps and @c message carry state
 * (the latest value replaces the previous one on the client side), and the
 * @c result member carries a fragment of the method's result, which is
 * merged into the client's rolling result. Every instance is a fresh
 * payload — create a new one per notification, since result fragments
 * must not be sent twice.
 */
class RpcNotification final {
public:
  // Constructors / Destructor
  RpcNotification() = default;
  RpcNotification(const RpcNotification& other) = default;

  // Setters

  /**
   * @brief Set the progress in percent (0..100), transferred as an integer
   */
  RpcNotification& progress(int percent) noexcept {
    mPayload.insert("progress", QVariant(std::clamp(percent, 0, 100)));
    return *this;
  }

  /**
   * @brief Set the progress in percent (0..100), transferred as a float
   *
   * For fractional resolution. Clients handle both variants the same way.
   */
  RpcNotification& progress(double percent) noexcept {
    mPayload.insert("progress", QVariant(std::clamp(percent, 0.0, 100.0)));
    return *this;
  }

  /**
   * @brief Set the current step (1-based) and the total number of steps
   */
  RpcNotification& step(int current, int total) noexcept {
    mPayload.insert("step", QVariant(current));
    mPayload.insert("steps", QVariant(total));
    return *this;
  }

  /**
   * @brief Set a human-readable message line
   */
  RpcNotification& message(const QString& message) noexcept {
    mPayload.insert("message", QVariant(message));
    return *this;
  }

  /**
   * @brief Add a fragment of the method's result
   *
   * The layout of the fragment shall match the documented result of the
   * method (any subset of members), and it shall only contain data which
   * was not delivered yet — the client merges it into its rolling result.
   */
  RpcNotification& result(const QVariantMap& fragment) noexcept {
    mPayload.insert("result", QVariant(fragment));
    return *this;
  }

  // General Methods

  /**
   * @brief Get the payload, as sent by RpcRequest::send()
   */
  QVariant toVariant() const noexcept { return QVariant(mPayload); }

private:  // Data
  QVariantMap mPayload;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
