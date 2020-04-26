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

#ifndef LIBREPCB_LIBRARY_EDITOR_PACKAGEEDITORWIDGET_H
#define LIBREPCB_LIBRARY_EDITOR_PACKAGEEDITORWIDGET_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../common/categorylisteditorwidget.h"
#include "../common/editorwidgetbase.h"

#include <librepcb/common/graphics/if_graphicsvieweventhandler.h>
#include <librepcb/library/pkg/footprint.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class GridProperties;
class GraphicsScene;

namespace library {

class Package;
class FootprintGraphicsItem;

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
  PackageEditorWidget()                                 = delete;
  PackageEditorWidget(const PackageEditorWidget& other) = delete;
  PackageEditorWidget(const Context& context, const FilePath& fp,
                      QWidget* parent = nullptr);
  ~PackageEditorWidget() noexcept;

  // Getters
  virtual bool hasGraphicalEditor() const noexcept override { return true; }
  virtual bool supportsFlip() const noexcept override { return true; }

  // Setters
  void setToolsActionGroup(ExclusiveActionGroup* group) noexcept override;
  void setStatusBar(StatusBar* statusbar) noexcept override;

  // Operator Overloadings
  PackageEditorWidget& operator=(const PackageEditorWidget& rhs) = delete;

public slots:
  bool save() noexcept override;
  bool cut() noexcept override;
  bool copy() noexcept override;
  bool paste() noexcept override;
  bool rotateCw() noexcept override;
  bool rotateCcw() noexcept override;
  bool mirror() noexcept override;
  bool flip() noexcept override;
  bool remove() noexcept override;
  bool zoomIn() noexcept override;
  bool zoomOut() noexcept override;
  bool zoomAll() noexcept override;
  bool abortCommand() noexcept override;
  bool editGridProperties() noexcept override;

private:  // Methods
  void    updateMetadata() noexcept;
  QString commitMetadata() noexcept;
  /// @copydoc librepcb::IF_GraphicsViewEventHandler::graphicsViewEventHandler()
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
                    bool                                              applyFix);
  bool processCheckMessage(
      std::shared_ptr<const LibraryElementCheckMessage> msg,
      bool                                              applyFix) override;

private:  // Data
  QScopedPointer<Ui::PackageEditorWidget>         mUi;
  QScopedPointer<PackageCategoryListEditorWidget> mCategoriesEditorWidget;
  QScopedPointer<GraphicsScene>                   mGraphicsScene;
  QScopedPointer<Package>                         mPackage;
  QScopedPointer<PackageEditorFsm>                mFsm;

  // broken interface detection
  QSet<Uuid>    mOriginalPadUuids;
  FootprintList mOriginalFootprints;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace library
}  // namespace librepcb

#endif  // LIBREPCB_LIBRARY_EDITOR_PACKAGEEDITORWIDGET_H
