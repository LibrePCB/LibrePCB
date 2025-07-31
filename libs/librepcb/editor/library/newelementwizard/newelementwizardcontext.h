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

#ifndef LIBREPCB_EDITOR_NEWELEMENTWIZARDCONTEXT_H
#define LIBREPCB_EDITOR_NEWELEMENTWIZARDCONTEXT_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/core/attribute/attribute.h>
#include <librepcb/core/fileio/filepath.h>
#include <librepcb/core/library/cmp/component.h>
#include <librepcb/core/library/dev/device.h>
#include <librepcb/core/library/dev/part.h>
#include <librepcb/core/library/pkg/package.h>
#include <librepcb/core/library/sym/symbol.h>
#include <librepcb/core/types/uuid.h>
#include <librepcb/core/types/version.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Library;
class Workspace;

namespace editor {

class GraphicsLayerList;

/*******************************************************************************
 *  Class NewElementWizardContext
 ******************************************************************************/

/**
 * @brief The NewElementWizardContext class
 */
class NewElementWizardContext final : public QObject {
  Q_OBJECT

public:
  // Types

  enum PageId {
    ID_None = -1,  ///< last page
    ID_ChooseType,
    ID_CopyFrom,
    ID_EnterMetadata,
    ID_DeviceProperties,
  };

  enum class ElementType {
    None,
    Device,
  };

  // Constructors / Destructor
  NewElementWizardContext() = delete;
  NewElementWizardContext(const NewElementWizardContext& other) = delete;
  NewElementWizardContext(const Workspace& ws, Library& lib,
                          const GraphicsLayerList& layers,
                          QObject* parent = nullptr) noexcept;
  ~NewElementWizardContext() noexcept;

  // Getters
  const FilePath& getOutputDirectory() const noexcept {
    return mOutputDirectory;
  }
  const Workspace& getWorkspace() const noexcept { return mWorkspace; }
  const GraphicsLayerList& getLayers() const noexcept { return mLayers; }
  const QStringList& getLibLocaleOrder() const noexcept;

  // General Methods
  void reset(ElementType newType = ElementType::None) noexcept;
  void copyElement(ElementType type, const FilePath& fp);
  void createLibraryElement();

  // Operator Overloadings
  NewElementWizardContext& operator=(const NewElementWizardContext& rhs) =
      delete;

private:  // Data
  const Workspace& mWorkspace;
  Library& mLibrary;
  const GraphicsLayerList& mLayers;
  FilePath mOutputDirectory;

public:  // Data
  // common
  QHash<QString, QByteArray> mFiles;
  ElementType mElementType;
  std::optional<ElementName> mElementName;
  QString mElementDescription;
  QString mElementKeywords;
  QString mElementAuthor;
  std::optional<Version> mElementVersion;
  QSet<Uuid> mElementCategoryUuids;

  // device
  std::optional<Uuid> mDeviceComponentUuid;
  std::optional<Uuid> mDevicePackageUuid;
  DevicePadSignalMap mDevicePadSignalMap;
  AttributeList mDeviceAttributes;
  PartList mDeviceParts;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
