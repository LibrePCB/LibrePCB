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
#include "mcpstdiotransport.h"

#include "mcpserver.h"

#include <QtCore>

#include <string>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace automation {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

McpStdioTransport::McpStdioTransport(McpServer& server, std::istream& input,
                                     std::ostream& output,
                                     QObject* parent) noexcept
  : QObject(parent), mServer(server), mInput(input), mOutput(output) {
}

McpStdioTransport::~McpStdioTransport() noexcept {
  if (mReaderThread) {
    mReaderThread->wait();
  }
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void McpStdioTransport::start() noexcept {
  Q_ASSERT(!mReaderThread);
  mReaderThread.reset(QThread::create([this]() {
    std::string line;
    while (std::getline(mInput, line)) {
      const QByteArray data = QByteArray::fromStdString(line);
      QMetaObject::invokeMethod(
          this, [this, data]() { enqueue(data); }, Qt::QueuedConnection);
    }
    QMetaObject::invokeMethod(
        this, [this]() { enqueue(std::nullopt); }, Qt::QueuedConnection);
  }));
  mReaderThread->start();
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void McpStdioTransport::enqueue(
    const std::optional<QByteArray>& line) noexcept {
  mQueue.enqueue(line);
  if (!mBusy) {
    processQueue();
  }
}

void McpStdioTransport::processQueue() noexcept {
  mBusy = true;
  while (!mQueue.isEmpty()) {
    const std::optional<QByteArray> line = mQueue.dequeue();
    if (!line) {
      emit finished();
      break;
    }
    const QByteArray response = mServer.handleLine(*line);
    if (!response.isEmpty()) {
      mOutput.write(response.constData(), response.size());
      mOutput.flush();
    }
  }
  mBusy = false;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace automation
}  // namespace librepcb
