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

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class GraphicsScene;
class GridProperties;
class Symbol;
class SymbolGraphicsItem;

namespace editor {

class SymbolEditorFsm;

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
  virtual bool hasGraphicalEditor() const noexcept override { return true; }

  // Setters
  void setToolsActionGroup(ExclusiveActionGroup* group) noexcept override;
  void setStatusBar(StatusBar* statusbar) noexcept override;

  // Operator Overloadings
  SymbolEditorWidget& operator=(const SymbolEditorWidget& rhs) = delete;

public slots:
  bool save() noexcept override;
  bool selectAll() noexcept override;
  bool cut() noexcept override;
  bool copy() noexcept override;
  bool paste() noexcept override;
  bool rotateCw() noexcept override;
  bool rotateCcw() noexcept override;
  bool mirror() noexcept override;
  bool remove() noexcept override;
  bool zoomIn() noexcept override;
  bool zoomOut() noexcept override;
  bool zoomAll() noexcept override;
  bool abortCommand() noexcept override;
  bool importDxf() noexcept override;
  bool editGridProperties() noexcept override;

private:  // Methods
  void updateMetadata() noexcept;
  QString commitMetadata() noexcept;
  /// @see ::librepcb::editor::IF_GraphicsViewEventHandler
  bool graphicsViewEventHandler(QEvent* event) noexcept override;
  bool toolChangeRequested(Tool newTool) noexcept override;
  bool isInterfaceBroken() const noexcept override;
  bool runChecks(LibraryElementCheckMessageList& msgs) const override;
  template <typename MessageType>
  void fixMsg(const MessageType& msg);
  template <typename MessageType>
  bool fixMsgHelper(std::shared_ptr<const LibraryElementCheckMessage> msg,
                    bool applyFix);
  bool processCheckMessage(
      std::shared_ptr<const LibraryElementCheckMessage> msg,
      bool applyFix) override;
  bool execGraphicsExportDialog(GraphicsExportDialog::Output output,
                                const QString& settingsKey) noexcept override;

private:  // Data
  QScopedPointer<Ui::SymbolEditorWidget> mUi;
  QScopedPointer<CategoryListEditorWidget> mCategoriesEditorWidget;
  QScopedPointer<GraphicsScene> mGraphicsScene;
  QScopedPointer<Symbol> mSymbol;
  QScopedPointer<SymbolGraphicsItem> mGraphicsItem;
  QScopedPointer<SymbolEditorFsm> mFsm;

  // broken interface detection
  QSet<Uuid> mOriginalSymbolPinUuids;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
