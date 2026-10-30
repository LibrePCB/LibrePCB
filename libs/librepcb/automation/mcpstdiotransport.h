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

#ifndef LIBREPCB_AUTOMATION_MCPSTDIOTRANSPORT_H
#define LIBREPCB_AUTOMATION_MCPSTDIOTRANSPORT_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <QtCore>

#include <iostream>
#include <memory>
#include <optional>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace automation {

class McpServer;

/*******************************************************************************
 *  Class McpStdioTransport
 ******************************************************************************/

/**
 * @brief Connects an ::librepcb::automation::McpServer to stdin/stdout
 *
 * Other streams can be passed instead, e.g. for tests.
 *
 * Messages are newline-delimited JSON. Stdin is read in a worker thread, but
 * all messages are handled in the thread of this object, so tools may use the
 * event loop. Messages are handled strictly one after another, even if a tool
 * runs a nested event loop. #finished() is emitted when stdin is closed and
 * all messages are handled.
 */
class McpStdioTransport final : public QObject {
  Q_OBJECT

public:
  // Constructors / Destructor
  McpStdioTransport() = delete;
  McpStdioTransport(const McpStdioTransport& other) = delete;
  explicit McpStdioTransport(McpServer& server, std::istream& input = std::cin,
                             std::ostream& output = std::cout,
                             QObject* parent = nullptr) noexcept;
  ~McpStdioTransport() noexcept override;

  // General Methods
  void start() noexcept;

  // Operator Overloadings
  McpStdioTransport& operator=(const McpStdioTransport& rhs) = delete;

signals:
  void finished();

private:  // Methods
  void enqueue(const std::optional<QByteArray>& line) noexcept;
  void processQueue() noexcept;

private:  // Data
  McpServer& mServer;
  std::istream& mInput;
  std::ostream& mOutput;
  std::unique_ptr<QThread> mReaderThread;
  QQueue<std::optional<QByteArray>> mQueue;  ///< nullopt = end of input
  bool mBusy = false;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace automation
}  // namespace librepcb

#endif
