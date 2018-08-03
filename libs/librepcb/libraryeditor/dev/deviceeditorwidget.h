/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
 * http://librepcb.org/
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

#ifndef LIBREPCB_LIBRARY_EDITOR_DEVICEEDITORWIDGET_H
#define LIBREPCB_LIBRARY_EDITOR_DEVICEEDITORWIDGET_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include <QtWidgets>
#include <librepcb/common/exceptions.h>
#include <librepcb/common/fileio/filepath.h>
#include <librepcb/library/dev/devicepadsignalmap.h>
#include "../common/editorwidgetbase.h"
#include "../common/categorylisteditorwidget.h"

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {

class GraphicsScene;

namespace library {

class Symbol;
class Device;
class Component;
class Package;
class SymbolPreviewGraphicsItem;
class FootprintPreviewGraphicsItem;

namespace editor {

namespace Ui {
class DeviceEditorWidget;
}

/*****************************************************************************************
 *  Class DeviceEditorWidget
 ****************************************************************************************/

/**
 * @brief The DeviceEditorWidget class
 *
 * @author ubruhin
 * @date 2016-10-16
 */
class DeviceEditorWidget final : public EditorWidgetBase
{
        Q_OBJECT

    public:

        // Constructors / Destructor
        DeviceEditorWidget() = delete;
        DeviceEditorWidget(const DeviceEditorWidget& other) = delete;
        DeviceEditorWidget(const Context& context, const FilePath& fp,
                           QWidget* parent = nullptr);
        ~DeviceEditorWidget() noexcept;

        // Operator Overloadings
        DeviceEditorWidget& operator=(const DeviceEditorWidget& rhs) = delete;


    public slots:
        bool save() noexcept override;
        bool zoomIn() noexcept override;
        bool zoomOut() noexcept override;
        bool zoomAll() noexcept override;


    private: // Methods
        void btnChooseComponentClicked() noexcept;
        void btnChoosePackageClicked() noexcept;
        void updateDeviceComponentUuid(const Uuid& uuid) noexcept;
        void updateComponentPreview() noexcept;
        void updateDevicePackageUuid(const Uuid& uuid) noexcept;
        void updatePackagePreview() noexcept;
        void memorizeDeviceInterface() noexcept;
        bool isInterfaceBroken() const noexcept override;


    private: // Data
        QScopedPointer<Ui::DeviceEditorWidget> mUi;
        QScopedPointer<ComponentCategoryListEditorWidget> mCategoriesEditorWidget;
        QScopedPointer<Device> mDevice;

        // component
        QScopedPointer<Component> mComponent;
        QScopedPointer<GraphicsScene> mComponentGraphicsScene;
        QList<std::shared_ptr<Symbol>> mSymbols;
        QList<std::shared_ptr<SymbolPreviewGraphicsItem>> mSymbolGraphicsItems;

        // package
        QScopedPointer<Package> mPackage;
        QScopedPointer<GraphicsScene> mPackageGraphicsScene;
        QScopedPointer<FootprintPreviewGraphicsItem> mFootprintGraphicsItem;

        // broken interface detection
        tl::optional<Uuid> mOriginalComponentUuid;
        tl::optional<Uuid> mOriginalPackageUuid;
        DevicePadSignalMap mOriginalPadSignalMap;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace editor
} // namespace library
} // namespace librepcb

#endif // LIBREPCB_LIBRARY_EDITOR_DEVICEEDITORWIDGET_H
