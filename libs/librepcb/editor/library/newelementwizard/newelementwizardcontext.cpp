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

#include <librepcb/core/exceptions.h>
#include <librepcb/core/fileio/transactionalfilesystem.h>
#include <librepcb/core/library/cat/componentcategory.h>
#include <librepcb/core/library/cat/packagecategory.h>
#include <librepcb/core/library/cmp/component.h>
#include <librepcb/core/library/dev/device.h>
#include <librepcb/core/library/library.h>
#include <librepcb/core/library/pkg/package.h>
#include <librepcb/core/library/sym/symbol.h>
#include <librepcb/core/workspace/workspace.h>
#include <librepcb/core/workspace/workspacesettings.h>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

NewElementWizardContext::NewElementWizardContext(
    const Workspace& ws, Library& lib, const IF_GraphicsLayerProvider& lp,
    QObject* parent) noexcept
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
  return mWorkspace.getSettings().libraryLocaleOrder.get();
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void NewElementWizardContext::reset(ElementType newType) noexcept {
  // common
  mElementType = newType;
  mElementName = tl::nullopt;
  mElementDescription.clear();
  mElementKeywords.clear();
  mElementAuthor = mWorkspace.getSettings().userName.get();
  mElementVersion = Version::fromString("0.1");
  mElementCategoryUuids.clear();

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
  mDevicePackageUuid = tl::nullopt;
}

void NewElementWizardContext::copyElement(ElementType type,
                                          const FilePath& fp) {
  mElementType = type;

  std::unique_ptr<TransactionalDirectory> dir(
      new TransactionalDirectory(TransactionalFileSystem::openRO(fp)));

  QScopedPointer<LibraryBaseElement> element;

  switch (mElementType) {
    case NewElementWizardContext::ElementType::ComponentCategory: {
      element.reset(new ComponentCategory(std::move(dir)));
      break;
    }
    case NewElementWizardContext::ElementType::PackageCategory: {
      element.reset(new PackageCategory(std::move(dir)));
      break;
    }
    case NewElementWizardContext::ElementType::Symbol: {
      element.reset(new Symbol(std::move(dir)));
      break;
    }
    case NewElementWizardContext::ElementType::Component: {
      element.reset(new Component(std::move(dir)));
      break;
    }
    case NewElementWizardContext::ElementType::Device: {
      element.reset(new Device(std::move(dir)));
      break;
    }
    case NewElementWizardContext::ElementType::Package: {
      element.reset(new Package(std::move(dir)));
      break;
    }
    default: {
      qCritical() << "Unknown enum value:" << static_cast<int>(mElementType);
      break;
    }
  }

  mElementName = element->getNames().getDefaultValue();
  mElementDescription = element->getDescriptions().getDefaultValue();
  mElementKeywords = element->getKeywords().getDefaultValue();
  if (const LibraryCategory* category =
          dynamic_cast<const LibraryCategory*>(element.data())) {
    if (category->getParentUuid().has_value()) {
      mElementCategoryUuids.insert(*category->getParentUuid());
    }
  }
  if (const LibraryElement* libElement =
          dynamic_cast<const LibraryElement*>(element.data())) {
    mElementCategoryUuids = libElement->getCategories();
  }

  switch (mElementType) {
    case NewElementWizardContext::ElementType::Symbol: {
      const Symbol* symbol = dynamic_cast<Symbol*>(element.data());
      Q_ASSERT(symbol);
      // copy pins but generate new UUIDs
      mSymbolPins.clear();
      for (const SymbolPin& pin : symbol->getPins()) {
        mSymbolPins.append(std::make_shared<SymbolPin>(
            Uuid::createRandom(), pin.getName(), pin.getPosition(),
            pin.getLength(), pin.getRotation(), pin.getNamePosition(),
            pin.getNameRotation(), pin.getNameHeight(),
            pin.getNameAlignment()));
      }
      // copy polygons but generate new UUIDs
      mSymbolPolygons.clear();
      for (const Polygon& polygon : symbol->getPolygons()) {
        mSymbolPolygons.append(std::make_shared<Polygon>(
            Uuid::createRandom(), polygon.getLayerName(),
            polygon.getLineWidth(), polygon.isFilled(), polygon.isGrabArea(),
            polygon.getPath()));
      }
      // copy circles but generate new UUIDs
      mSymbolCircles.clear();
      for (const Circle& circle : symbol->getCircles()) {
        mSymbolCircles.append(std::make_shared<Circle>(
            Uuid::createRandom(), circle.getLayerName(), circle.getLineWidth(),
            circle.isFilled(), circle.isGrabArea(), circle.getCenter(),
            circle.getDiameter()));
      }
      // copy texts but generate new UUIDs
      mSymbolTexts.clear();
      for (const Text& text : symbol->getTexts()) {
        mSymbolTexts.append(std::make_shared<Text>(
            Uuid::createRandom(), text.getLayerName(), text.getText(),
            text.getPosition(), text.getRotation(), text.getHeight(),
            text.getAlign()));
      }
      break;
    }

    case ElementType::Package: {
      const Package* package = dynamic_cast<Package*>(element.data());
      Q_ASSERT(package);
      // copy pads but generate new UUIDs
      QHash<Uuid, tl::optional<Uuid>> padUuidMap;
      mPackagePads.clear();
      for (const PackagePad& pad : package->getPads()) {
        Uuid newUuid = Uuid::createRandom();
        padUuidMap.insert(pad.getUuid(), newUuid);
        mPackagePads.append(
            std::make_shared<PackagePad>(newUuid, pad.getName()));
      }
      // copy footprints but generate new UUIDs
      mPackageFootprints.clear();
      for (const Footprint& footprint : package->getFootprints()) {
        // don't copy translations as they would need to be adjusted anyway
        std::shared_ptr<Footprint> newFootprint(new Footprint(
            Uuid::createRandom(), footprint.getNames().getDefaultValue(),
            footprint.getDescriptions().getDefaultValue()));
        // copy pads but generate new UUIDs
        for (const FootprintPad& pad : footprint.getPads()) {
          tl::optional<Uuid> pkgPad = pad.getPackagePadUuid();
          if (pkgPad) {
            pkgPad = padUuidMap.value(*pkgPad);  // Translate to new UUID
          }
          newFootprint->getPads().append(std::make_shared<FootprintPad>(
              Uuid::createRandom(), pkgPad, pad.getPosition(),
              pad.getRotation(), pad.getShape(), pad.getWidth(),
              pad.getHeight(), pad.getDrillDiameter(), pad.getBoardSide()));
        }
        // copy polygons but generate new UUIDs
        for (const Polygon& polygon : footprint.getPolygons()) {
          newFootprint->getPolygons().append(std::make_shared<Polygon>(
              Uuid::createRandom(), polygon.getLayerName(),
              polygon.getLineWidth(), polygon.isFilled(), polygon.isGrabArea(),
              polygon.getPath()));
        }
        // copy circles but generate new UUIDs
        for (const Circle& circle : footprint.getCircles()) {
          newFootprint->getCircles().append(std::make_shared<Circle>(
              Uuid::createRandom(), circle.getLayerName(),
              circle.getLineWidth(), circle.isFilled(), circle.isGrabArea(),
              circle.getCenter(), circle.getDiameter()));
        }
        // copy stroke texts but generate new UUIDs
        for (const StrokeText& text : footprint.getStrokeTexts()) {
          newFootprint->getStrokeTexts().append(std::make_shared<StrokeText>(
              Uuid::createRandom(), text.getLayerName(), text.getText(),
              text.getPosition(), text.getRotation(), text.getHeight(),
              text.getStrokeWidth(), text.getLetterSpacing(),
              text.getLineSpacing(), text.getAlign(), text.getMirrored(),
              text.getAutoRotate()));
        }
        // copy holes but generate new UUIDs
        for (const Hole& hole : footprint.getHoles()) {
          newFootprint->getHoles().append(std::make_shared<Hole>(
              Uuid::createRandom(), hole.getPosition(), hole.getDiameter()));
        }
        mPackageFootprints.append(newFootprint);
      }
      break;
    }

    case ElementType::Component: {
      const Component* component = dynamic_cast<Component*>(element.data());
      Q_ASSERT(component);
      mComponentSchematicOnly = component->isSchematicOnly();
      mComponentAttributes = component->getAttributes();
      mComponentDefaultValue = component->getDefaultValue();
      mComponentPrefixes = component->getPrefixes();
      // copy signals but generate new UUIDs
      QHash<Uuid, Uuid> signalUuidMap;
      mComponentSignals.clear();
      for (const ComponentSignal& signal : component->getSignals()) {
        Uuid newUuid = Uuid::createRandom();
        signalUuidMap.insert(signal.getUuid(), newUuid);
        mComponentSignals.append(std::make_shared<ComponentSignal>(
            newUuid, signal.getName(), signal.getRole(),
            signal.getForcedNetName(), signal.isRequired(), signal.isNegated(),
            signal.isClock()));
      }
      // copy symbol variants but generate new UUIDs
      mComponentSymbolVariants.clear();
      for (const ComponentSymbolVariant& var : component->getSymbolVariants()) {
        // don't copy translations as they would need to be adjusted anyway
        std::shared_ptr<ComponentSymbolVariant> copy(new ComponentSymbolVariant(
            Uuid::createRandom(), var.getNorm(),
            var.getNames().getDefaultValue(),
            var.getDescriptions().getDefaultValue()));
        // copy items
        for (const ComponentSymbolVariantItem& item : var.getSymbolItems()) {
          std::shared_ptr<ComponentSymbolVariantItem> itemCopy(
              new ComponentSymbolVariantItem(
                  Uuid::createRandom(), item.getSymbolUuid(),
                  item.getSymbolPosition(), item.getSymbolRotation(),
                  item.isRequired(), item.getSuffix()));
          // copy pin-signal-map
          for (const ComponentPinSignalMapItem& map : item.getPinSignalMap()) {
            tl::optional<Uuid> signal = map.getSignalUuid();
            if (signal) {
              signal = *signalUuidMap.find(*map.getSignalUuid());
            }
            itemCopy->getPinSignalMap().append(
                std::make_shared<ComponentPinSignalMapItem>(
                    map.getPinUuid(), signal, map.getDisplayType()));
          }
          copy->getSymbolItems().append(itemCopy);
        }
        mComponentSymbolVariants.append(copy);
      }
      break;
    }

    case ElementType::Device: {
      const Device* device = dynamic_cast<Device*>(element.data());
      Q_ASSERT(device);
      mDeviceComponentUuid = device->getComponentUuid();
      mDevicePackageUuid = device->getPackageUuid();
      mDevicePadSignalMap = device->getPadSignalMap();
      break;
    }

    default: { break; }
  }
}

void NewElementWizardContext::createLibraryElement() {
  if (!mElementName) throw LogicError(__FILE__, __LINE__);
  if (!mElementVersion) throw LogicError(__FILE__, __LINE__);

  tl::optional<Uuid> rootCategoryUuid = tl::nullopt;
  if (mElementCategoryUuids.count()) {
    rootCategoryUuid = mElementCategoryUuids.values().first();
  }

  switch (mElementType) {
    case NewElementWizardContext::ElementType::ComponentCategory: {
      ComponentCategory element(Uuid::createRandom(), *mElementVersion,
                                mElementAuthor, *mElementName,
                                mElementDescription, mElementKeywords);
      element.setParentUuid(rootCategoryUuid);
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
      element.setParentUuid(rootCategoryUuid);
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
      element.setCategories(mElementCategoryUuids);
      element.getPins() = mSymbolPins;
      element.getPolygons() = mSymbolPolygons;
      element.getCircles() = mSymbolCircles;
      element.getTexts() = mSymbolTexts;
      TransactionalDirectory dir(mLibrary.getDirectory(),
                                 mLibrary.getElementsDirectoryName<Symbol>());
      element.moveIntoParentDirectory(dir);
      mOutputDirectory = element.getDirectory().getAbsPath();
      break;
    }
    case NewElementWizardContext::ElementType::Package: {
      Package element(Uuid::createRandom(), *mElementVersion, mElementAuthor,
                      *mElementName, mElementDescription, mElementKeywords);
      element.setCategories(mElementCategoryUuids);
      element.getPads() = mPackagePads;
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
      element.setCategories(mElementCategoryUuids);
      element.setIsSchematicOnly(mComponentSchematicOnly);
      element.getAttributes() = mComponentAttributes;
      element.setDefaultValue(mComponentDefaultValue);
      element.setPrefixes(mComponentPrefixes);
      element.getSignals() = mComponentSignals;
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
      element.setCategories(mElementCategoryUuids);
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
}  // namespace librepcb
