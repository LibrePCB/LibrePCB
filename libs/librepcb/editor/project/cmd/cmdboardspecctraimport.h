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

#ifndef LIBREPCB_EDITOR_CMDBOARDSPECCTRAIMPORT_H
#define LIBREPCB_EDITOR_CMDBOARDSPECCTRAIMPORT_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../undocommandgroup.h"

#include <librepcb/core/geometry/path.h>
#include <librepcb/core/types/angle.h>
#include <librepcb/core/types/maskconfig.h>
#include <librepcb/core/types/point.h>

#include <QtCore>

#include <memory>
#include <optional>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Board;
class Circuit;
class Layer;
class MessageLogger;
class NetSignal;
class Project;
class SExpression;

namespace editor {

/*******************************************************************************
 *  Class CmdBoardSpecctraImport
 ******************************************************************************/

/**
 * @brief Undo command to import a Specctra session (SES)
 */
class CmdBoardSpecctraImport : public UndoCommandGroup {
public:
  // Types
  enum class Side { Front, Back };

  struct ComponentOut {
    QString name;
    Point pos;
    Side side;
    Angle rot;
  };

  struct PadStackOut {
    const Layer* startLayer;
    const Layer* endLayer;
    Length diameter;
  };

  struct ViaOut {
    QString padStackId;
    Point pos;
  };

  struct WireOut {
    const Layer* layer;
    Length width;
    Path path;
  };

  struct NetOut {
    QString netName;
    QList<ViaOut> vias;
    QList<WireOut> wires;
  };

  struct NetOutProcessed {
    NetSignal* netSignal;
    QList<ViaOut> vias;
    QList<WireOut> wires;
  };

  // Constructors / Destructor
  explicit CmdBoardSpecctraImport(Board& board, const SExpression& root,
                                  std::shared_ptr<MessageLogger> logger);
  ~CmdBoardSpecctraImport() noexcept;

private:  // Methods
  /// @copydoc ::librepcb::editor::UndoCommand::performExecute()
  bool performExecute() override;

  static std::optional<PositiveLength> extractViaDrillDiameter(
      const QString& padStackId) noexcept;
  static std::optional<MaskConfig> extractViaExposureConfig(
      const QString& padStackId) noexcept;

  Project& mProject;
  Circuit& mCircuit;
  Board& mBoard;
  std::shared_ptr<MessageLogger> mLogger;
  std::optional<QList<ComponentOut>> mComponents;
  QHash<QString, PadStackOut> mPadStacks;
  QList<NetOut> mNets;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
