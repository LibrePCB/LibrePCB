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

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "newelementwizardcontext.h"

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
    const workspace::Workspace& ws, const Library& lib,
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
      element.saveIntoParentDirectory(
          mLibrary.getElementsDirectory<ComponentCategory>());
      mOutputDirectory = element.getFilePath();
      break;
    }
    case NewElementWizardContext::ElementType::PackageCategory: {
      PackageCategory element(Uuid::createRandom(), *mElementVersion,
                              mElementAuthor, *mElementName,
                              mElementDescription, mElementKeywords);
      element.setParentUuid(mElementCategoryUuid);
      element.saveIntoParentDirectory(
          mLibrary.getElementsDirectory<PackageCategory>());
      mOutputDirectory = element.getFilePath();
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
      element.saveIntoParentDirectory(mLibrary.getElementsDirectory<Symbol>());
      mOutputDirectory = element.getFilePath();
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
      element.saveIntoParentDirectory(mLibrary.getElementsDirectory<Package>());
      mOutputDirectory = element.getFilePath();
      break;
    }
    case NewElementWizardContext::ElementType::Component: {
      Component element(Uuid::createRandom(), *mElementVersion, mElementAuthor,
                        *mElementName, mElementDescription, mElementKeywords);
      element.setCategories(categories);
      element.setIsSchematicOnly(mComponentSchematicOnly);
      element.getAttributes() = mComponentAttributes;
      element.setDefaultValue(mComponentDefaultValue);
      element.getPrefixes()       = mComponentPrefixes;
      element.getSignals()        = mComponentSignals;
      element.getSymbolVariants() = mComponentSymbolVariants;
      element.saveIntoParentDirectory(
          mLibrary.getElementsDirectory<Component>());
      mOutputDirectory = element.getFilePath();
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
      element.saveIntoParentDirectory(mLibrary.getElementsDirectory<Device>());
      mOutputDirectory = element.getFilePath();
      break;
    }
    default:
      throw LogicError(__FILE__, __LINE__);
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace library
}  // namespace librepcb
