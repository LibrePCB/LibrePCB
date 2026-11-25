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
#include "graphicsexportrpc.h"

#include "../../guiapplication.h"
#include "../../project/board/boardeditor.h"
#include "../../project/projecteditor.h"
#include "../../project/schematic/schematiceditor.h"
#include "../rpcnotification.h"
#include "../rpcobjectregistry.h"
#include "../rpcrequest.h"
#include "rpcerrors.h"
#include "rpchelpers.h"
#include "rpctypes.h"

#include <librepcb/core/application.h>
#include <librepcb/core/export/graphicsexport.h>
#include <librepcb/core/fileio/fileutils.h>
#include <librepcb/core/project/board/board.h>
#include <librepcb/core/project/board/boardpainter.h>
#include <librepcb/core/project/board/realisticboardpainter.h>
#include <librepcb/core/project/project.h>
#include <librepcb/core/project/schematic/schematic.h>
#include <librepcb/core/project/schematic/schematicpainter.h>
#include <librepcb/core/workspace/basecolorscheme.h>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {
namespace rpc {

/*******************************************************************************
 *  Helper Functions
 ******************************************************************************/

/*******************************************************************************
 *  Command Handlers
 ******************************************************************************/

void GraphicsExportRpc::run(GuiApplication& app, RpcObjectRegistry& registry,
                            RpcRequest& request) {
  Q_UNUSED(app);

  auto settings = std::make_shared<GraphicsExportSettings>();
  settings->setPixmapDpi(getOptionalArg<qint64>(request, "dpi").value_or(300));
  settings->setMirror(getOptionalArg<bool>(request, "flip").value_or(false));
  settings->setBackgroundColor(
      getOptionalArg<QColor>(request, "background").value_or(Qt::transparent));

  QStringList painterErrors;
  std::shared_ptr<GraphicsPagePainter> painter;
  if (auto editor = registry.tryGetAs<SchematicEditor>(request.getArg("obj"))) {
    painter = std::make_shared<SchematicPainter>(editor->getSchematic(),
                                                 &painterErrors);
    settings->loadColorsFromScheme(&BaseColorScheme::schematicLibrePcbLight(),
                                   &BaseColorScheme::boardLibrePcbDark(), 0);
  } else if (auto editor =
                 registry.tryGetAs<BoardEditor>(request.getArg("obj"))) {
    Board& board = editor->getBoard();
    if (getOptionalArg<bool>(request, "realistic").value_or(false)) {
      painter = std::make_shared<RealisticBoardPainter>(
          board.buildScene3D(std::nullopt));
      settings->loadBoardRenderingColors(board.getInnerLayerCount());
    } else {
      painter = std::make_shared<BoardPainter>(board);
      settings->loadColorsFromScheme(&BaseColorScheme::schematicLibrePcbLight(),
                                     &BaseColorScheme::boardLibrePcbDark(),
                                     board.getInnerLayerCount());
    }
  }
  if (!painter) throw RpcInvalidHandle();

  const QString format =
      getOptionalArg<QString>(request, "format").value_or("png");
  const FilePath fp(Application::getRandomTempPath().toStr() % "." % format);
  GraphicsExport* exporter = new GraphicsExport();

  auto sendResponse = [&request, exporter,
                       fp](const QString& error = QString()) {
    if (request.isCancelled()) {
      request.send(RpcCancelled());
    } else if (!error.isEmpty()) {
      request.send(RpcCommandFailed(error));
    } else {
      try {
        request.send({
            {"files", toVariant(QList<QByteArray>{FileUtils::readFile(fp)})},
        });
      } catch (const Exception& e) {
        request.send(RpcCommandFailed(e.getMsg()));
      }
    }
    QFile(fp.toStr()).remove();
    exporter->deleteLater();
  };

  request.makeAsync(*exporter);
  request.send(
      RpcNotification().result({{"errors", toVariant(painterErrors)}}));
  QObject::connect(&request, &RpcRequest::cancelled, exporter,
                   &GraphicsExport::cancel);
  QObject::connect(exporter, &GraphicsExport::succeeded, &request,
                   sendResponse);
  QObject::connect(exporter, &GraphicsExport::failed, &request, sendResponse);
  exporter->startExport({{painter, settings}}, fp);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace rpc
}  // namespace editor
}  // namespace librepcb
