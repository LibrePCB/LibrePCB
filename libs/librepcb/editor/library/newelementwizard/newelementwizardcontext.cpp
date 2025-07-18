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
#include <librepcb/core/library/cmp/component.h>
#include <librepcb/core/library/dev/device.h>
#include <librepcb/core/library/library.h>
#include <librepcb/core/library/pkg/package.h>
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
    const Workspace& ws, Library& lib, const GraphicsLayerList& layers,
    QObject* parent) noexcept
  : QObject(parent),
    mWorkspace(ws),
    mLibrary(lib),
    mLayers(layers),
    mElementType(ElementType::None),
    mPackageAssemblyType(Package::AssemblyType::Auto),
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
  mFiles.clear();
  mElementType = newType;
  mElementName = std::nullopt;
  mElementDescription.clear();
  mElementKeywords.clear();
  mElementAuthor = mWorkspace.getSettings().userName.get();
  mElementVersion = Version::fromString("0.1");
  mElementCategoryUuids.clear();

  // package
  mPackageAssemblyType = Package::AssemblyType::Auto;
  mPackagePads.clear();
  mPackageModels.clear();
  mPackageFootprints.clear();

  // component
  mComponentSchematicOnly = false;
  mComponentAttributes.clear();
  mComponentDefaultValue.clear();
  mComponentPrefixes = NormDependentPrefixMap(ComponentPrefix(""));
  mComponentSignals.clear();
  mComponentSymbolVariants.clear();

  // device
  mDeviceComponentUuid = std::nullopt;
  mDevicePackageUuid = std::nullopt;
  mDevicePadSignalMap.clear();
  mDeviceAttributes.clear();
  mDeviceParts.clear();
}

void NewElementWizardContext::copyElement(ElementType type,
                                          const FilePath& fp) {
  mElementType = type;

  std::unique_ptr<TransactionalDirectory> dir(
      new TransactionalDirectory(TransactionalFileSystem::openRO(fp)));

  std::unique_ptr<LibraryBaseElement> element;

  switch (mElementType) {
    case NewElementWizardContext::ElementType::Component: {
      element = Component::open(std::move(dir));
      break;
    }
    case NewElementWizardContext::ElementType::Device: {
      element = Device::open(std::move(dir));
      break;
    }
    case NewElementWizardContext::ElementType::Package: {
      element = Package::open(std::move(dir));
      break;
    }
    default: {
      qCritical()
          << "Unhandled switch-case in NewElementWizardContext::copyElement():"
          << static_cast<int>(mElementType);
      break;
    }
  }

  mElementName = element->getNames().getDefaultValue();
  mElementDescription = element->getDescriptions().getDefaultValue();
  mElementKeywords = element->getKeywords().getDefaultValue();
  if (const LibraryElement* libElement =
          dynamic_cast<const LibraryElement*>(element.get())) {
    mElementCategoryUuids = libElement->getCategories();
  }

  switch (mElementType) {
    case ElementType::Package: {
      const Package* package = dynamic_cast<Package*>(element.get());
      Q_ASSERT(package);
      mPackageAssemblyType = package->getAssemblyType(false);
      // copy pads but generate new UUIDs
      QHash<Uuid, std::optional<Uuid>> padUuidMap;
      mPackagePads.clear();
      for (const PackagePad& pad : package->getPads()) {
        Uuid newUuid = Uuid::createRandom();
        padUuidMap.insert(pad.getUuid(), newUuid);
        mPackagePads.append(
            std::make_shared<PackagePad>(newUuid, pad.getName()));
      }
      // Copy 3D models but generate new UUIDs.
      QHash<Uuid, std::optional<Uuid>> modelsUuidMap;
      mPackageModels.clear();
      for (const PackageModel& model : package->getModels()) {
        auto newModel = std::make_shared<PackageModel>(Uuid::createRandom(),
                                                       model.getName());
        modelsUuidMap.insert(model.getUuid(), newModel->getUuid());
        mPackageModels.append(newModel);
        const QByteArray fileContent =
            package->getDirectory().readIfExists(model.getFileName());
        if (!fileContent.isNull()) {
          mFiles.insert(newModel->getFileName(), fileContent);
        }
      }
      // copy footprints but generate new UUIDs
      mPackageFootprints.clear();
      for (const Footprint& footprint : package->getFootprints()) {
        // don't copy translations as they would need to be adjusted anyway
        std::shared_ptr<Footprint> newFootprint(new Footprint(
            Uuid::createRandom(), footprint.getNames().getDefaultValue(),
            footprint.getDescriptions().getDefaultValue()));
        newFootprint->setModelPosition(footprint.getModelPosition());
        newFootprint->setModelRotation(footprint.getModelRotation());
        // Copy models but with the new UUIDs.
        QSet<Uuid> models;
        foreach (const Uuid& uuid, footprint.getModels()) {
          if (auto newUuid = modelsUuidMap.value(uuid)) {
            models.insert(*newUuid);
          }
        }
        newFootprint->setModels(models);
        // copy pads but generate new UUIDs
        for (const FootprintPad& pad : footprint.getPads()) {
          std::optional<Uuid> pkgPad = pad.getPackagePadUuid();
          if (pkgPad) {
            pkgPad = padUuidMap.value(*pkgPad);  // Translate to new UUID
          }
          newFootprint->getPads().append(std::make_shared<FootprintPad>(
              Uuid::createRandom(), pkgPad, pad.getPosition(),
              pad.getRotation(), pad.getShape(), pad.getWidth(),
              pad.getHeight(), pad.getRadius(), pad.getCustomShapeOutline(),
              pad.getStopMaskConfig(), pad.getSolderPasteConfig(),
              pad.getCopperClearance(), pad.getComponentSide(),
              pad.getFunction(), pad.getHoles()));
        }
        // copy polygons but generate new UUIDs
        for (const Polygon& polygon : footprint.getPolygons()) {
          newFootprint->getPolygons().append(std::make_shared<Polygon>(
              Uuid::createRandom(), polygon.getLayer(), polygon.getLineWidth(),
              polygon.isFilled(), polygon.isGrabArea(), polygon.getPath()));
        }
        // copy circles but generate new UUIDs
        for (const Circle& circle : footprint.getCircles()) {
          newFootprint->getCircles().append(std::make_shared<Circle>(
              Uuid::createRandom(), circle.getLayer(), circle.getLineWidth(),
              circle.isFilled(), circle.isGrabArea(), circle.getCenter(),
              circle.getDiameter()));
        }
        // copy stroke texts but generate new UUIDs
        for (const StrokeText& text : footprint.getStrokeTexts()) {
          newFootprint->getStrokeTexts().append(std::make_shared<StrokeText>(
              Uuid::createRandom(), text.getLayer(), text.getText(),
              text.getPosition(), text.getRotation(), text.getHeight(),
              text.getStrokeWidth(), text.getLetterSpacing(),
              text.getLineSpacing(), text.getAlign(), text.getMirrored(),
              text.getAutoRotate()));
        }
        // copy zones but generate new UUIDs
        for (const Zone& zone : footprint.getZones()) {
          newFootprint->getZones().append(
              std::make_shared<Zone>(Uuid::createRandom(), zone));
        }
        // copy holes but generate new UUIDs
        for (const Hole& hole : footprint.getHoles()) {
          newFootprint->getHoles().append(
              std::make_shared<Hole>(Uuid::createRandom(), hole.getDiameter(),
                                     hole.getPath(), hole.getStopMaskConfig()));
        }
        mPackageFootprints.append(newFootprint);
      }
      break;
    }

    case ElementType::Component: {
      const Component* component = dynamic_cast<Component*>(element.get());
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
            std::optional<Uuid> signal = map.getSignalUuid();
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
      const Device* device = dynamic_cast<Device*>(element.get());
      Q_ASSERT(device);
      mDeviceComponentUuid = device->getComponentUuid();
      mDevicePackageUuid = device->getPackageUuid();
      mDevicePadSignalMap = device->getPadSignalMap();
      mDeviceAttributes = device->getAttributes();
      mDeviceParts = device->getParts();
      break;
    }

    default: {
      break;
    }
  }
}

void NewElementWizardContext::createLibraryElement() {
  if (!mElementName) throw LogicError(__FILE__, __LINE__);
  if (!mElementVersion) throw LogicError(__FILE__, __LINE__);

  std::optional<Uuid> rootCategoryUuid = std::nullopt;
  if (mElementCategoryUuids.count()) {
    rootCategoryUuid = mElementCategoryUuids.values().first();
  }

  auto copyFiles = [this](TransactionalDirectory& dst) {
    for (auto it = mFiles.begin(); it != mFiles.end(); ++it) {
      dst.write(it.key(), it.value());
    }
  };

  switch (mElementType) {
    case NewElementWizardContext::ElementType::Package: {
      Package element(Uuid::createRandom(), *mElementVersion, mElementAuthor,
                      *mElementName, mElementDescription, mElementKeywords,
                      mPackageAssemblyType);
      element.setCategories(mElementCategoryUuids);
      element.getPads() = mPackagePads;
      element.getModels() = mPackageModels;
      element.getFootprints() = mPackageFootprints;
      if (element.getFootprints().isEmpty()) {
        element.getFootprints().append(std::make_shared<Footprint>(
            Uuid::createRandom(), ElementName("default"), ""));
      }
      TransactionalDirectory dir(mLibrary.getDirectory(),
                                 mLibrary.getElementsDirectoryName<Package>());
      element.moveIntoParentDirectory(dir);
      copyFiles(element.getDirectory());
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
      copyFiles(element.getDirectory());
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
      element.getAttributes() = mDeviceAttributes;
      element.getParts() = mDeviceParts;
      TransactionalDirectory dir(mLibrary.getDirectory(),
                                 mLibrary.getElementsDirectoryName<Device>());
      element.moveIntoParentDirectory(dir);
      copyFiles(element.getDirectory());
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
