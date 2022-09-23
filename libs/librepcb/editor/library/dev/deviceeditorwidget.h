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

#ifndef LIBREPCB_EDITOR_DEVICEEDITORWIDGET_H
#define LIBREPCB_EDITOR_DEVICEEDITORWIDGET_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../cat/categorylisteditorwidget.h"
#include "../editorwidgetbase.h"

#include <librepcb/core/library/dev/devicepadsignalmap.h>

#include <QtCore>
#include <QtWidgets>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Component;
class DefaultGraphicsLayerProvider;
class Device;
class GraphicsScene;
class Package;
class Symbol;

namespace editor {

class FootprintGraphicsItem;
class SymbolGraphicsItem;

namespace Ui {
class DeviceEditorWidget;
}

/*******************************************************************************
 *  Class DeviceEditorWidget
 ******************************************************************************/

/**
 * @brief The DeviceEditorWidget class
 */
class DeviceEditorWidget final : public EditorWidgetBase {
  Q_OBJECT

public:
  // Constructors / Destructor
  DeviceEditorWidget() = delete;
  DeviceEditorWidget(const DeviceEditorWidget& other) = delete;
  DeviceEditorWidget(const Context& context, const FilePath& fp,
                     QWidget* parent = nullptr);
  ~DeviceEditorWidget() noexcept;

  // Getters
  QSet<Feature> getAvailableFeatures() const noexcept override;

  // Operator Overloadings
  DeviceEditorWidget& operator=(const DeviceEditorWidget& rhs) = delete;

public slots:
  bool save() noexcept override;
  bool zoomIn() noexcept override;
  bool zoomOut() noexcept override;
  bool zoomAll() noexcept override;

private:  // Methods
  void updateMetadata() noexcept;
  QString commitMetadata() noexcept;
  void btnChooseComponentClicked() noexcept;
  void btnChoosePackageClicked() noexcept;
  void updateDeviceComponentUuid(const Uuid& uuid) noexcept;
  void updateComponentPreview() noexcept;
  void updateDevicePackageUuid(const Uuid& uuid) noexcept;
  void updatePackagePreview() noexcept;
  void memorizeDeviceInterface() noexcept;
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

private:  // Data
  QScopedPointer<Ui::DeviceEditorWidget> mUi;
  QScopedPointer<CategoryListEditorWidget> mCategoriesEditorWidget;
  QScopedPointer<Device> mDevice;
  QScopedPointer<DefaultGraphicsLayerProvider> mGraphicsLayerProvider;

  // component
  std::shared_ptr<Component> mComponent;
  QScopedPointer<GraphicsScene> mComponentGraphicsScene;
  QList<std::shared_ptr<Symbol>> mSymbols;
  QList<std::shared_ptr<SymbolGraphicsItem>> mSymbolGraphicsItems;

  // package
  QScopedPointer<Package> mPackage;
  QScopedPointer<GraphicsScene> mPackageGraphicsScene;
  QScopedPointer<FootprintGraphicsItem> mFootprintGraphicsItem;

  // broken interface detection
  tl::optional<Uuid> mOriginalComponentUuid;
  tl::optional<Uuid> mOriginalPackageUuid;
  DevicePadSignalMap mOriginalPadSignalMap;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
