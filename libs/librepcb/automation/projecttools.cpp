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
#include "projecttools.h"

#include "designsession.h"
#include "mcpserver.h"
#include "toolhelpers.h"

#include <librepcb/core/exceptions.h>
#include <librepcb/core/export/graphicsexport.h>
#include <librepcb/core/export/graphicsexportsettings.h>
#include <librepcb/core/fileio/fileutils.h>
#include <librepcb/core/job/graphicsoutputjob.h>
#include <librepcb/core/library/cmp/component.h>
#include <librepcb/core/library/dev/device.h>
#include <librepcb/core/project/board/board.h>
#include <librepcb/core/project/board/boardplanefragmentsbuilder.h>
#include <librepcb/core/project/board/drc/boarddesignrulecheck.h>
#include <librepcb/core/project/board/items/bi_device.h>
#include <librepcb/core/project/circuit/circuit.h>
#include <librepcb/core/project/circuit/componentassemblyoption.h>
#include <librepcb/core/project/circuit/componentinstance.h>
#include <librepcb/core/project/circuit/componentsignalinstance.h>
#include <librepcb/core/project/circuit/netsignal.h>
#include <librepcb/core/project/erc/electricalrulecheck.h>
#include <librepcb/core/project/outputjobrunner.h>
#include <librepcb/core/project/project.h>
#include <librepcb/core/project/schematic/items/si_symbol.h>
#include <librepcb/core/project/schematic/schematic.h>
#include <librepcb/core/rulecheck/rulecheckmessage.h>
#include <librepcb/core/utils/toolbox.h>
#include <librepcb/core/workspace/colorrole.h>
#include <librepcb/editor/project/cmd/cmdprojectedit.h>
#include <librepcb/editor/undostack.h>

#include <QtCore>
#include <QtGui>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace automation {

/*******************************************************************************
 *  Helpers
 ******************************************************************************/

/**
 * @brief Render a page as PNG, together with the pixel to millimeter mapping
 *
 * The image extent depends on the page content (e.g. texts outside the board
 * outline), thus the mapping is returned with each image.
 */
static McpToolResult renderPng(const GraphicsExport::Page& page,
                               const QJsonValue& region) {
  QTemporaryDir dir;
  if (!dir.isValid()) {
    throw RuntimeError(__FILE__, __LINE__,
                       "Failed to create temporary directory.");
  }
  const FilePath fp(dir.filePath("render.png"));
  GraphicsExport graphicsExport;
  graphicsExport.startExport({page}, fp);
  const GraphicsExport::Result result = graphicsExport.waitForFinished();
  if (!result.errorMsg.isEmpty()) {
    throw RuntimeError(__FILE__, __LINE__, result.errorMsg);
  }
  const QByteArray png =
      FileUtils::readFile(result.writtenFiles.value(0, fp));  // can throw

  // Same source rect as determined by GraphicsExport.
  const GraphicsExportSettings& settings = *page.second;
  QPicture picture;
  QPainter painter(&picture);
  page.first->paint(painter, settings);
  painter.end();
  const QRectF sourceRect = picture.boundingRect();
  const qreal centerX = Length::fromPx(sourceRect.center().x()).toMm();
  const qreal centerY = -Length::fromPx(sourceRect.center().y()).toMm();
  const QSize imageSize = QImage::fromData(png, "PNG").size();
  const qreal pxPerMm = settings.getPixmapDpi() / qreal(25.4);
  const qreal contentCenterX =
      (settings.getMarginLeft()->toMm() * pxPerMm + imageSize.width() -
       settings.getMarginRight()->toMm() * pxPerMm) /
      2;
  const qreal contentCenterY =
      (settings.getMarginTop()->toMm() * pxPerMm + imageSize.height() -
       settings.getMarginBottom()->toMm() * pxPerMm) /
      2;
  const qreal xDirection = settings.getMirror() ? -1 : 1;
  QJsonArray topLeft{centerX - xDirection * contentCenterX / pxPerMm,
                     centerY + contentCenterY / pxPerMm};

  QByteArray output = png;
  QSize size = imageSize;
  if (region.isObject()) {
    const Point a = pointMm(region.toObject().value("from"), "region.from");
    const Point b = pointMm(region.toObject().value("to"), "region.to");
    auto px = [&](const Point& p) {
      return QPointF(
          xDirection * (p.getX().toMm() - topLeft.at(0).toDouble()) * pxPerMm,
          (topLeft.at(1).toDouble() - p.getY().toMm()) * pxPerMm);
    };
    const QRect rect = QRectF(px(a), px(b))
                           .normalized()
                           .toAlignedRect()
                           .intersected(QRect(QPoint(0, 0), imageSize));
    if (rect.isEmpty()) {
      throw RuntimeError(__FILE__, __LINE__,
                         "The region is outside of the rendered area.");
    }
    QBuffer buffer(&output);
    output.clear();
    buffer.open(QIODevice::WriteOnly);
    QImage::fromData(png, "PNG").copy(rect).save(&buffer, "PNG");
    topLeft = QJsonArray{
        topLeft.at(0).toDouble() + xDirection * rect.left() / pxPerMm,
        topLeft.at(1).toDouble() - rect.top() / pxPerMm};
    size = rect.size();
  }

  McpToolResult mcpResult = McpToolResult::image(output);
  mcpResult.structuredContent = QJsonObject{
      {"width", size.width()},
      {"height", size.height()},
      {"pixelsPerMm", pxPerMm},
      {"topLeft", topLeft},
      {"mirrored", settings.getMirror()},
  };
  mcpResult.content.append(QJsonObject{
      {"type", "text"},
      {"text",
       QString("Image %1x%2 px, %3 px/mm. Pixel (0, 0) is at (%4, %5) mm, "
               "x grows to the %6, y (mm) decreases downwards.")
           .arg(size.width())
           .arg(size.height())
           .arg(pxPerMm, 0, 'f', 3)
           .arg(topLeft.at(0).toDouble(), 0, 'f', 3)
           .arg(topLeft.at(1).toDouble(), 0, 'f', 3)
           .arg(settings.getMirror() ? "left" : "right")},
  });
  return mcpResult;
}

static int dpiArgument(const QJsonObject& args, int defaultDpi) {
  const int dpi = args.value("dpi").toInt(defaultDpi);
  if ((dpi < 10) || (dpi > 1200)) {
    throw RuntimeError(__FILE__, __LINE__,
                       "The DPI must be between 10 and 1200.");
  }
  return dpi;
}

static const QString sMessagesHelp =
    "Each message has an ID (to approve it), the marked areas ('locations', "
    "center and size in mm, one message may cover several places) and an "
    "explanation, which is given only at the first message with the same "
    "explanation.";

static const QJsonObject sNameOrIndex{
    {"type", QJsonArray{"string", "integer"}},
};

static const QJsonObject sRegion = objectSchema(
    {{"from", pointProp("One corner")}, {"to", pointProp("Opposite corner")}},
    {"from", "to"});

/*******************************************************************************
 *  Functions
 ******************************************************************************/

void addProjectTools(McpServer& server, DesignSession& session) noexcept {
  auto summary = [&session]() {
    Project& project = session.getProject();
    QJsonArray schematics;
    for (const Schematic* schematic : project.getSchematics()) {
      schematics.append(*schematic->getName());
    }
    QJsonArray boards;
    for (const Board* board : project.getBoards()) {
      boards.append(*board->getName());
    }
    return QJsonObject{
        {"name", *project.getName()},
        {"version", *project.getVersion()},
        {"author", project.getAuthor()},
        {"path", project.getFilepath().toNative()},
        {"schematics", schematics},
        {"boards", boards},
        {"partCount", project.getCircuit().getComponentInstances().count()},
        {"netCount", project.getCircuit().getNetSignals().count()},
        {"unsavedChanges", !session.getUndoStack().isClean()},
    };
  };

  server.addTool(McpTool{
      "project_open",
      "Open a LibrePCB project (*.lpp). Closes the previously opened project "
      "without saving. Unless readOnly is set, the project directory stays "
      "locked until it is closed, so it cannot be open in the LibrePCB "
      "application at the same time.",
      QJsonObject{
          {"type", "object"},
          {"properties",
           QJsonObject{
               {"path",
                QJsonObject{
                    {"type", "string"},
                    {"description", "Absolute path to the *.lpp file"}}},
               {"readOnly", QJsonObject{{"type", "boolean"}}},
           }},
          {"required", QJsonArray{"path"}},
      },
      false,
      [&session, summary](const QJsonObject& args) {
        session.openProject(absoluteFilePath(requiredString(args, "path")),
                            args.value("readOnly").toBool(false));
        return McpToolResult::json(summary());
      },
  });

  server.addTool(McpTool{
      "project_close",
      "Close the project without saving and release its lock.",
      QJsonObject(),
      false,
      [&session](const QJsonObject&) {
        session.closeProject();
        return McpToolResult::text("Project closed.");
      },
  });

  server.addTool(McpTool{
      "project_save",
      "Save the project to disk.",
      QJsonObject(),
      false,
      [&session](const QJsonObject&) {
        session.saveProject();
        return McpToolResult::text("Project saved.");
      },
  });

  server.addTool(McpTool{
      "project_summary",
      "Get name, version, schematic pages, boards and counts of the open "
      "project.",
      QJsonObject(),
      true,
      [summary](const QJsonObject&) { return McpToolResult::json(summary()); },
  });

  server.addTool(McpTool{
      "project_set_metadata",
      "Change the project name, version or author. They appear in title "
      "blocks, output file names (version) and via placeholders like "
      "{{PROJECT}}, {{VERSION}} and {{AUTHOR}}.",
      objectSchema({{"name", stringProp("Project name")},
                    {"version", stringProp("Version, e.g. 'v1.0' or '2'")},
                    {"author", stringProp("Author")}}),
      false,
      [&session, summary](const QJsonObject& args) {
        auto cmd =
            std::make_unique<editor::CmdProjectEdit>(session.getProject());
        if (args.contains("name")) {
          cmd->setName(
              ElementName(cleanElementName(requiredString(args, "name"))));
        }
        if (args.contains("version")) {
          cmd->setVersion(FileProofName(requiredString(args, "version")));
        }
        if (args.contains("author")) {
          cmd->setAuthor(args.value("author").toString());
        }
        session.execCmd(cmd.release());  // can throw
        return McpToolResult::json(summary());
      },
  });

  server.addTool(McpTool{
      "parts_list",
      "List all parts with designator, value, library component and their "
      "placement in schematics and boards (positions in mm).",
      QJsonObject(),
      true,
      [&session](const QJsonObject&) {
        Project& project = session.getProject();
        QList<ComponentInstance*> components =
            project.getCircuit().getComponentInstances().values();
        Toolbox::sortNumeric(
            components,
            [](const QCollator& collator, const ComponentInstance* a,
               const ComponentInstance* b) {
              return collator(*a->getName(), *b->getName());
            });
        QJsonArray parts;
        for (const ComponentInstance* cmp : components) {
          QJsonArray symbols;
          for (const SI_Symbol* symbol : cmp->getSymbols()) {
            symbols.append(QJsonObject{
                {"schematic", *symbol->getSchematic().getName()},
                {"position", toJson(symbol->getPosition())},
                {"rotation", symbol->getRotation().toDeg()},
                {"mirrored", symbol->getMirrored()},
            });
          }
          QJsonArray devices;
          for (const Board* board : project.getBoards()) {
            if (const BI_Device* dev =
                    board->getDeviceInstanceByComponentUuid(cmp->getUuid())) {
              devices.append(QJsonObject{
                  {"board", *board->getName()},
                  {"device", *dev->getLibDevice().getNames().getDefaultValue()},
                  {"position", toJson(dev->getPosition())},
                  {"rotation", dev->getRotation().toDeg()},
                  {"mirrored", dev->getMirrored()},
              });
            }
          }
          std::shared_ptr<const Part> part;
          if (const auto option = cmp->getAssemblyOptions().value(0)) {
            part = option->getParts().value(0);
          }
          parts.append(QJsonObject{
              {"designator", *cmp->getName()},
              {"value", partValue(*cmp)},
              {"mpn", part ? QJsonValue(*part->getMpn()) : QJsonValue()},
              {"component",
               *cmp->getLibComponent().getNames().getDefaultValue()},
              {"symbols", symbols},
              {"devices", devices},
          });
        }
        return McpToolResult::json(QJsonObject{{"parts", parts}});
      },
  });

  server.addTool(McpTool{
      "netlist_get",
      "List all nets with their connected pins ('<designator>.<signal>').",
      QJsonObject(),
      true,
      [&session](const QJsonObject&) {
        Project& project = session.getProject();
        QList<NetSignal*> nets = project.getCircuit().getNetSignals().values();
        Toolbox::sortNumeric(nets,
                             [](const QCollator& collator, const NetSignal* a,
                                const NetSignal* b) {
                               return collator(*a->getName(), *b->getName());
                             });
        QJsonArray array;
        for (const NetSignal* net : nets) {
          QStringList pins;
          for (const ComponentSignalInstance* sig :
               net->getComponentSignals()) {
            pins.append(*sig->getComponentInstance().getName() % "." %
                        *sig->getCompSignal().getName());
          }
          Toolbox::sortNumeric(pins);
          array.append(QJsonObject{
              {"name", *net->getName()},
              {"pins", QJsonArray::fromStringList(pins)},
          });
        }
        return McpToolResult::json(QJsonObject{{"nets", array}});
      },
  });

  auto runErc = [&session]() {
    Project& project = session.getProject();
    ElectricalRuleCheck erc(project);
    return erc.runChecks();
  };
  auto runDrc = [](Board& board) {
    rebuildPlanes(board);  // can throw
    BoardDesignRuleCheck drc;
    drc.start(board, board.getDrcSettings(), false);
    const BoardDesignRuleCheck::Result result = drc.waitForFinished();
    if (!result.errors.isEmpty()) {
      throw RuntimeError(__FILE__, __LINE__, result.errors.join("\n"));
    }
    // Same as in the board editor, to remove approvals of messages which
    // disappeared.
    board.updateDrcMessageApprovals(
        RuleCheckMessage::getAllApprovals(result.messages), false);
    return result.messages;
  };
  const QJsonObject approveSchema = objectSchema(
      {{"ids",
        arrayProp(stringProp("Message ID"),
                  "IDs of the messages, as returned by the check")},
       {"approved", boolProp("Approve (default) or revoke the approval")}},
      {"ids"});

  server.addTool(McpTool{
      "erc_run",
      "Run the electrical rule check of the schematics. " % sMessagesHelp,
      QJsonObject(),
      true,
      [&session, runErc](const QJsonObject&) {
        return McpToolResult::json(
            toJson(runErc(), session.getProject().getErcMessageApprovals()));
      },
  });

  server.addTool(McpTool{
      "erc_approve",
      "Approve ERC messages which are fine by intention, so they don't count "
      "as open issues anymore. Save the project to keep the approvals.",
      approveSchema,
      false,
      [&session, runErc](const QJsonObject& args) {
        Project& project = session.getProject();
        const int changed = approveMessages(
            runErc(), project.getErcMessageApprovals(), args.value("ids"),
            args.value("approved").toBool(true),
            [&project](const SExpression& approval, bool approved) {
              project.setErcMessageApproved(approval, approved);
            });
        return McpToolResult::json(QJsonObject{{"changed", changed}});
      },
  });

  server.addTool(McpTool{
      "drc_run",
      "Run the design rule check of a board (planes are rebuilt first). " %
          sMessagesHelp,
      QJsonObject{
          {"type", "object"},
          {"properties", QJsonObject{{"board", sNameOrIndex}}},
      },
      true,
      [&session, runDrc](const QJsonObject& args) {
        Board& board = session.getBoard(args.value("board"));
        return McpToolResult::json(
            toJson(runDrc(board), board.getDrcMessageApprovals()));
      },
  });

  QJsonObject drcApproveSchema = approveSchema;
  QJsonObject drcApproveProperties =
      drcApproveSchema.value("properties").toObject();
  drcApproveProperties.insert("board", sNameOrIndex);
  drcApproveSchema.insert("properties", drcApproveProperties);
  server.addTool(McpTool{
      "drc_approve",
      "Approve DRC messages which you reviewed and accept by intention, so "
      "they don't count as open issues anymore. Approve only messages you "
      "can explain: clearance errors are often real shorts. Save the project "
      "to keep the approvals.",
      drcApproveSchema,
      false,
      [&session, runDrc](const QJsonObject& args) {
        Board& board = session.getBoard(args.value("board"));
        const int changed = approveMessages(
            runDrc(board), board.getDrcMessageApprovals(), args.value("ids"),
            args.value("approved").toBool(true),
            [&board](const SExpression& approval, bool approved) {
              board.setDrcMessageApproved(approval, approved);
            });
        return McpToolResult::json(QJsonObject{{"changed", changed}});
      },
  });

  server.addTool(McpTool{
      "render_schematic",
      "Render a schematic page as PNG image, optionally only a region "
      "(e.g. at a higher DPI to read details).",
      QJsonObject{
          {"type", "object"},
          {"properties",
           QJsonObject{
               {"schematic", sNameOrIndex},
               {"region", sRegion},
               {"dpi",
                QJsonObject{{"type", "integer"},
                            {"minimum", 10},
                            {"maximum", 1200},
                            {"description", "Resolution, default 100"}}},
           }},
      },
      true,
      [&session](const QJsonObject& args) {
        Project& project = session.getProject();
        Schematic& schematic = session.getSchematic(args.value("schematic"));
        GraphicsOutputJob::Content content(
            GraphicsOutputJob::Content::Preset::Schematic);
        content.pixmapDpi = dpiArgument(args, 100);
        auto job = GraphicsOutputJob::schematicPdf();
        job->setContent({content});
        OutputJobRunner runner(project);
        const GraphicsExport::Pages pages = runner.buildPages(*job, false);
        const int index = project.getSchematics().indexOf(&schematic);
        if ((index < 0) || (index >= pages.count())) {
          throw LogicError(__FILE__, __LINE__);
        }
        return renderPng(pages.at(index), args.value("region"));
      },
  });

  server.addTool(McpTool{
      "render_board",
      "Render a board as PNG image, either with colored layers (good to check "
      "placement and routing) or as a realistic rendering. Optionally only "
      "a region (e.g. at a higher DPI to inspect details) or only some "
      "layers. Returns the mapping from pixels to board coordinates.",
      QJsonObject{
          {"type", "object"},
          {"properties",
           QJsonObject{
               {"board", sNameOrIndex},
               {"side",
                QJsonObject{{"type", "string"},
                            {"enum", QJsonArray{"top", "bottom"}}}},
               {"style",
                QJsonObject{{"type", "string"},
                            {"enum", QJsonArray{"layers", "realistic"}}}},
               {"region", sRegion},
               {"layers",
                arrayProp(stringProp("Layer name"),
                          "Only these layers of the 'layers' style, e.g. "
                          "['board_copper_top', 'board_outlines'], or also "
                          "'board_courtyard_top' or "
                          "'board_package_outlines_top' (in magenta). An "
                          "unknown name lists all names.")},
               {"dpi",
                QJsonObject{{"type", "integer"},
                            {"minimum", 10},
                            {"maximum", 1200},
                            {"description", "Resolution, default 600"}}},
           }},
      },
      true,
      [&session](const QJsonObject& args) {
        using Preset = GraphicsOutputJob::Content::Preset;
        Project& project = session.getProject();
        Board& board = session.getBoard(args.value("board"));
        const bool bottom = (args.value("side").toString() == "bottom");
        const bool realistic = (args.value("style").toString() == "realistic");
        Preset preset = Preset::BoardImage;
        if (realistic) {
          preset =
              bottom ? Preset::BoardRenderingBottom : Preset::BoardRenderingTop;
        }
        GraphicsOutputJob::Content content(preset);
        content.mirror = bottom;
        content.pixmapDpi = dpiArgument(args, 600);
        content.boards = GraphicsOutputJob::BoardSet::set(
            {std::optional<Uuid>(board.getUuid())});
        if (args.contains("layers")) {
          if (realistic) {
            throw RuntimeError(__FILE__, __LINE__,
                               "Layers can only be chosen for the 'layers' "
                               "style.");
          }
          const QStringList extras = {
              ColorRole::boardCourtyardTop().getId(),
              ColorRole::boardCourtyardBot().getId(),
              ColorRole::boardPackageOutlinesTop().getId(),
              ColorRole::boardPackageOutlinesBot().getId(),
          };
          QMap<QString, QColor> layers;
          for (const QJsonValue& value : args.value("layers").toArray()) {
            const QString id = value.toString();
            if (content.layers.contains(id)) {
              layers.insert(id, content.layers.value(id));
            } else if (extras.contains(id)) {
              layers.insert(id, QColor("#ff00ff"));
            } else {
              throw RuntimeError(
                  __FILE__, __LINE__,
                  QString("Unknown layer '%1'. Available: %2")
                      .arg(id, (content.layers.keys() + extras).join(", ")));
            }
          }
          content.layers = layers;
        }
        auto job = GraphicsOutputJob::schematicPdf();
        job->setContent({content});
        OutputJobRunner runner(project);
        const GraphicsExport::Pages pages = runner.buildPages(*job, true);
        if (pages.count() != 1) {
          throw LogicError(__FILE__, __LINE__);
        }
        return renderPng(pages.first(), args.value("region"));
      },
  });
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace automation
}  // namespace librepcb
