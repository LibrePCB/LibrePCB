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

#ifndef LIBREPCB_EDITOR_PACKAGEEDITORWIDGET_H
#define LIBREPCB_EDITOR_PACKAGEEDITORWIDGET_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../widgets/if_graphicsvieweventhandler.h"
#include "../cat/categorylisteditorwidget.h"
#include "../editorwidgetbase.h"

#include <librepcb/core/library/pkg/footprint.h>

#include <QtCore>
#include <QtWidgets>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class FootprintGraphicsItem;
class GraphicsScene;
class GridProperties;
class Package;

namespace editor {

class PackageEditorFsm;

namespace Ui {
class PackageEditorWidget;
}

/*******************************************************************************
 *  Class PackageEditorWidget
 ******************************************************************************/

/**
 * @brief The PackageEditorWidget class
 */
class PackageEditorWidget final : public EditorWidgetBase,
                                  public IF_GraphicsViewEventHandler {
  Q_OBJECT

public:
  // Constructors / Destructor
  PackageEditorWidget() = delete;
  PackageEditorWidget(const PackageEditorWidget& other) = delete;
  PackageEditorWidget(const Context& context, const FilePath& fp,
                      QWidget* parent = nullptr);
  ~PackageEditorWidget() noexcept;

  // Getters
  QSet<Feature> getAvailableFeatures() const noexcept override;

  // Setters
  void connectEditor(UndoStackActionGroup& undoStackActionGroup,
                     ExclusiveActionGroup& toolsActionGroup,
                     QToolBar& commandToolBar,
                     StatusBar& statusBar) noexcept override;
  void disconnectEditor() noexcept override;

  // Operator Overloadings
  PackageEditorWidget& operator=(const PackageEditorWidget& rhs) = delete;

public slots:
  bool save() noexcept override;
  bool selectAll() noexcept override;
  bool cut() noexcept override;
  bool copy() noexcept override;
  bool paste() noexcept override;
  bool move(Qt::ArrowType direction) noexcept override;
  bool rotate(const librepcb::Angle& rotation) noexcept override;
  bool mirror(Qt::Orientation orientation) noexcept override;
  bool flip(Qt::Orientation orientation) noexcept override;
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
  bool toolChangeRequested(Tool newTool) noexcept override;
  void currentFootprintChanged(int index) noexcept;
  void memorizePackageInterface() noexcept;
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
  void setGridProperties(const GridProperties& grid) noexcept;

private:  // Data
  QScopedPointer<Ui::PackageEditorWidget> mUi;
  QScopedPointer<CategoryListEditorWidget> mCategoriesEditorWidget;
  QScopedPointer<GraphicsScene> mGraphicsScene;
  std::unique_ptr<Package> mPackage;

  // broken interface detection
  QSet<Uuid> mOriginalPadUuids;
  FootprintList mOriginalFootprints;

  /// Editor state machine
  QScopedPointer<PackageEditorFsm> mFsm;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
