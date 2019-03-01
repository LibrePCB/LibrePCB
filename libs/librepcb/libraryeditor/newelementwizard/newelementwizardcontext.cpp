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
#include "newelementwizardcontext.h"

#include <librepcb/common/fileio/transactionalfilesystem.h>
#include <librepcb/library/elements.h>
#include <librepcb/workspace/settings/workspacesettings.h>
#include <librepcb/workspace/workspace.h>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace library {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

NewElementWizardContext::NewElementWizardContext(
    const workspace::Workspace& ws, Library& lib,
    const IF_GraphicsLayerProvider& lp, QObject* parent) noexcept
  : QObject(parent),
    mWorkspace(ws),
    mLibrary(lib),
    mLayerProvider(lp),
    mElementType(ElementType::None),
    mComponentPrefixes(ComponentPrefix("")) {
  reset();
}

NewElementWizardContext::~NewElementWizardContext() noexcept {
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

const QStringList& NewElementWizardContext::getLibLocaleOrder() const noexcept {
  return mWorkspace.getSettings().getLibLocaleOrder().getLocaleOrder();
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void NewElementWizardContext::reset() noexcept {
  // common
  mElementType = ElementType::None;
  mElementName = tl::nullopt;
  mElementDescription.clear();
  mElementKeywords.clear();
  mElementAuthor       = mWorkspace.getSettings().getUser().getName();
  mElementVersion      = Version::fromString("0.1");
  mElementCategoryUuid = tl::nullopt;

  // symbol
  mSymbolPins.clear();
  mSymbolPolygons.clear();
  mSymbolCircles.clear();
  mSymbolTexts.clear();

  // package
  mPackagePads.clear();
  mPackageFootprints.clear();

  // component
  mComponentSchematicOnly = false;
  mComponentAttributes.clear();
  mComponentDefaultValue.clear();
  mComponentPrefixes = NormDependentPrefixMap(ComponentPrefix(""));
  mComponentSignals.clear();
  mComponentSymbolVariants.clear();

  // device
  mDeviceComponentUuid = tl::nullopt;
  mDevicePackageUuid   = tl::nullopt;
}

void NewElementWizardContext::createLibraryElement() {
  QSet<Uuid> categories;
  if (mElementCategoryUuid) {
    categories.insert(*mElementCategoryUuid);
  }

  if (!mElementName) throw LogicError(__FILE__, __LINE__);
  if (!mElementVersion) throw LogicError(__FILE__, __LINE__);

  switch (mElementType) {
    case NewElementWizardContext::ElementType::ComponentCategory: {
      ComponentCategory element(Uuid::createRandom(), *mElementVersion,
                                mElementAuthor, *mElementName,
                                mElementDescription, mElementKeywords);
      element.setParentUuid(mElementCategoryUuid);
      TransactionalDirectory dir(
          mLibrary.getDirectory(),
          mLibrary.getElementsDirectoryName<ComponentCategory>());
      element.moveIntoParentDirectory(dir);
      mOutputDirectory = element.getDirectory().getAbsPath();
      break;
    }
    case NewElementWizardContext::ElementType::PackageCategory: {
      PackageCategory element(Uuid::createRandom(), *mElementVersion,
                              mElementAuthor, *mElementName,
                              mElementDescription, mElementKeywords);
      element.setParentUuid(mElementCategoryUuid);
      TransactionalDirectory dir(
          mLibrary.getDirectory(),
          mLibrary.getElementsDirectoryName<PackageCategory>());
      element.moveIntoParentDirectory(dir);
      mOutputDirectory = element.getDirectory().getAbsPath();
      break;
    }
    case NewElementWizardContext::ElementType::Symbol: {
      Symbol element(Uuid::createRandom(), *mElementVersion, mElementAuthor,
                     *mElementName, mElementDescription, mElementKeywords);
      element.setCategories(categories);
      element.getPins()     = mSymbolPins;
      element.getPolygons() = mSymbolPolygons;
      element.getCircles()  = mSymbolCircles;
      element.getTexts()    = mSymbolTexts;
      TransactionalDirectory dir(mLibrary.getDirectory(),
                                 mLibrary.getElementsDirectoryName<Symbol>());
      element.moveIntoParentDirectory(dir);
      mOutputDirectory = element.getDirectory().getAbsPath();
      break;
    }
    case NewElementWizardContext::ElementType::Package: {
      Package element(Uuid::createRandom(), *mElementVersion, mElementAuthor,
                      *mElementName, mElementDescription, mElementKeywords);
      element.setCategories(categories);
      element.getPads()       = mPackagePads;
      element.getFootprints() = mPackageFootprints;
      if (element.getFootprints().isEmpty()) {
        element.getFootprints().append(std::make_shared<Footprint>(
            Uuid::createRandom(), ElementName("default"), ""));
      }
      TransactionalDirectory dir(mLibrary.getDirectory(),
                                 mLibrary.getElementsDirectoryName<Package>());
      element.moveIntoParentDirectory(dir);
      mOutputDirectory = element.getDirectory().getAbsPath();
      break;
    }
    case NewElementWizardContext::ElementType::Component: {
      Component element(Uuid::createRandom(), *mElementVersion, mElementAuthor,
                        *mElementName, mElementDescription, mElementKeywords);
      element.setCategories(categories);
      element.setIsSchematicOnly(mComponentSchematicOnly);
      element.setAttributes(mComponentAttributes);
      element.setDefaultValue(mComponentDefaultValue);
      element.setPrefixes(mComponentPrefixes);
      element.getSignals()        = mComponentSignals;
      element.getSymbolVariants() = mComponentSymbolVariants;
      TransactionalDirectory dir(
          mLibrary.getDirectory(),
          mLibrary.getElementsDirectoryName<Component>());
      element.moveIntoParentDirectory(dir);
      mOutputDirectory = element.getDirectory().getAbsPath();
      break;
    }
    case NewElementWizardContext::ElementType::Device: {
      if (!mDeviceComponentUuid) throw LogicError(__FILE__, __LINE__);
      if (!mDevicePackageUuid) throw LogicError(__FILE__, __LINE__);
      Device element(Uuid::createRandom(), *mElementVersion, mElementAuthor,
                     *mElementName, mElementDescription, mElementKeywords,
                     *mDeviceComponentUuid, *mDevicePackageUuid);
      element.setCategories(categories);
      element.getPadSignalMap() = mDevicePadSignalMap;
      TransactionalDirectory dir(mLibrary.getDirectory(),
                                 mLibrary.getElementsDirectoryName<Device>());
      element.moveIntoParentDirectory(dir);
      mOutputDirectory = element.getDirectory().getAbsPath();
      break;
    }
    default:
      throw LogicError(__FILE__, __LINE__);
  }

  // save to disk (a bit hacky, but should work...)
  mLibrary.getDirectory().getFileSystem()->save();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace library
}  // namespace librepcb
