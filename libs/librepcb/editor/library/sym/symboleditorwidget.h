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

#ifndef LIBREPCB_EDITOR_SYMBOLEDITORWIDGET_H
#define LIBREPCB_EDITOR_SYMBOLEDITORWIDGET_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../widgets/if_graphicsvieweventhandler.h"
#include "../cat/categorylisteditorwidget.h"
#include "../editorwidgetbase.h"

#include <librepcb/core/types/lengthunit.h>
#include <librepcb/core/workspace/theme.h>

#include <QtCore>
#include <QtWidgets>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Symbol;

namespace editor {

class GraphicsScene;
class SymbolEditorFsm;
class SymbolGraphicsItem;

namespace Ui {
class SymbolEditorWidget;
}

/*******************************************************************************
 *  Class SymbolEditorWidget
 ******************************************************************************/

/**
 * @brief The SymbolEditorWidget class
 */
class SymbolEditorWidget final : public EditorWidgetBase,
                                 public IF_GraphicsViewEventHandler {
  Q_OBJECT

public:
  // Constructors / Destructor
  SymbolEditorWidget() = delete;
  SymbolEditorWidget(const SymbolEditorWidget& other) = delete;
  SymbolEditorWidget(const Context& context, const FilePath& fp,
                     QWidget* parent = nullptr);
  ~SymbolEditorWidget() noexcept;

  // Getters
  QSet<Feature> getAvailableFeatures() const noexcept override;

  // Setters
  void connectEditor(UndoStackActionGroup& undoStackActionGroup,
                     ExclusiveActionGroup& toolsActionGroup,
                     QToolBar& commandToolBar,
                     StatusBar& statusBar) noexcept override;
  void disconnectEditor() noexcept override;

  // Operator Overloadings
  SymbolEditorWidget& operator=(const SymbolEditorWidget& rhs) = delete;

public slots:
  bool save() noexcept override;
  bool selectAll() noexcept override;
  bool cut() noexcept override;
  bool copy() noexcept override;
  bool paste() noexcept override;
  bool move(Qt::ArrowType direction) noexcept override;
  bool rotate(const librepcb::Angle& rotation) noexcept override;
  bool mirror(Qt::Orientation orientation) noexcept override;
  bool snapToGrid() noexcept override;
  bool remove() noexcept override;
  bool editProperties() noexcept override;
  bool zoomIn() noexcept override;
  bool zoomOut() noexcept override;
  bool zoomAll() noexcept override;
  bool abortCommand() noexcept override;
  bool importDxf() noexcept override;
  bool editGridProperties() noexcept override;
  bool increaseGridInterval() noexcept override;
  bool decreaseGridInterval() noexcept override;

private:  // Methods
  void updateMetadata() noexcept;
  QString commitMetadata() noexcept;
  /// @see ::librepcb::editor::IF_GraphicsViewEventHandler
  bool graphicsViewEventHandler(QEvent* event) noexcept override;
  bool toolChangeRequested(Tool newTool,
                           const QVariant& mode) noexcept override;
  bool isInterfaceBroken() const noexcept override;
  bool runChecks(RuleCheckMessageList& msgs) const override;
  template <typename MessageType>
  void fixMsg(const MessageType& msg);
  template <typename MessageType>
  bool fixMsgHelper(std::shared_ptr<const RuleCheckMessage> msg, bool applyFix);
  bool processRuleCheckMessage(std::shared_ptr<const RuleCheckMessage> msg,
                               bool applyFix) override;
  void ruleCheckApproveRequested(std::shared_ptr<const RuleCheckMessage> msg,
                                 bool approve) noexcept override;
  bool execGraphicsExportDialog(GraphicsExportDialog::Output output,
                                const QString& settingsKey) noexcept override;
  void setGridProperties(const PositiveLength& interval, const LengthUnit& unit,
                         Theme::GridStyle style) noexcept;

private:  // Data
  QScopedPointer<Ui::SymbolEditorWidget> mUi;
  QScopedPointer<CategoryListEditorWidget> mCategoriesEditorWidget;
  QScopedPointer<GraphicsScene> mGraphicsScene;
  LengthUnit mLengthUnit;
  std::unique_ptr<Symbol> mSymbol;
  QScopedPointer<SymbolGraphicsItem> mGraphicsItem;

  /// Broken interface detection
  QSet<Uuid> mOriginalSymbolPinUuids;

  /// Editor state machine
  QScopedPointer<SymbolEditorFsm> mFsm;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
