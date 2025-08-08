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

#ifndef LIBREPCB_CORE_PACKAGE_H
#define LIBREPCB_CORE_PACKAGE_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../types/simplestring.h"
#include "../libraryelement.h"
#include "footprint.h"
#include "packagemodel.h"
#include "packagepad.h"

#include <QtCore>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class Package
 ******************************************************************************/

/**
 * @brief The Package class represents a package of a component (including
 * footprint and 3D model)
 *
 * Following information is considered as the "interface" of a package and must
 * therefore never be changed:
 *  - UUID
 *  - Package pads (neither adding nor removing pads is allowed)
 *    - UUID
 *  - Footprints (adding new footprints is allowed, but removing not)
 *    - UUID
 *    - Footprint pads (neither adding nor removing pads is allowed)
 *      - UUID
 */
class Package final : public LibraryElement {
  Q_OBJECT

public:
  // Types
  struct AlternativeName {
    ElementName name;
    SimpleString reference;

    AlternativeName(const ElementName& name, const SimpleString& reference)
      : name(name), reference(reference) {}
    AlternativeName(const SExpression& node)
      : name(deserialize<ElementName>(node.getChild("@0"))),
        reference(deserialize<SimpleString>(node.getChild("reference/@0"))) {}
    void serialize(SExpression& root) const {
      root.appendChild(name);
      root.appendChild("reference", reference);
    }
  };
  enum class AssemblyType {
    None,  ///< Nothing to mount (i.e. not a package, just a footprint)
    Tht,  ///< Pure THT package
    Smt,  ///< Pure SMT package
    Mixed,  ///< Mixed THT/SMT package
    Other,  ///< Anything special, e.g. mechanical parts
    Auto,  ///< Auto detection (deprecated, only for file format migration!)
    // ATTENTION: Update uihelpers.cpp & constants.slint when changing the enum!
  };

  // Constructors / Destructor
  Package() = delete;
  Package(const Package& other) = delete;
  Package(const Uuid& uuid, const Version& version, const QString& author,
          const ElementName& name_en_US, const QString& description_en_US,
          const QString& keywords_en_US, AssemblyType assemblyType);
  ~Package() noexcept;

  // Getters
  const QList<AlternativeName>& getAlternativeNames() const noexcept {
    return mAlternativeNames;
  }
  AssemblyType getAssemblyType(bool resolveAuto) const noexcept;
  AssemblyType guessAssemblyType() const noexcept;
  const PositiveLength& getGridInterval() const noexcept {
    return mGridInterval;
  }
  PackagePadList& getPads() noexcept { return mPads; }
  const PackagePadList& getPads() const noexcept { return mPads; }
  PackageModelList& getModels() noexcept { return mModels; }
  const PackageModelList& getModels() const noexcept { return mModels; }
  QVector<std::shared_ptr<const PackageModel>> getModelsForFootprint(
      const Uuid& fpt) const noexcept;
  FootprintList& getFootprints() noexcept { return mFootprints; }
  const FootprintList& getFootprints() const noexcept { return mFootprints; }

  // Setters
  void setAssemblyType(AssemblyType type) noexcept { mAssemblyType = type; }
  void setGridInterval(const PositiveLength& interval) noexcept {
    mGridInterval = interval;
  }

  // General Methods
  virtual RuleCheckMessageList runChecks() const override;

  // Operator Overloadings
  Package& operator=(const Package& rhs) = delete;

  // Static Methods
  static std::unique_ptr<Package> open(
      std::unique_ptr<TransactionalDirectory> directory,
      bool abortBeforeMigration = false);
  static QString getShortElementName() noexcept {
    return QStringLiteral("pkg");
  }
  static QString getLongElementName() noexcept {
    return QStringLiteral("package");
  }

protected:  // Methods
  virtual void serialize(SExpression& root) const override;

private:  // Methods
  Package(std::unique_ptr<TransactionalDirectory> directory,
          const SExpression& root);

private:  // Data
  QList<AlternativeName> mAlternativeNames;  ///< Optional
  AssemblyType mAssemblyType;  ///< Package assembly type (metadata)
  PositiveLength mGridInterval;
  PackagePadList mPads;  ///< empty list if the package has no pads
  PackageModelList mModels;  ///< 3D models (optional)
  FootprintList mFootprints;  ///< minimum one footprint
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

Q_DECLARE_METATYPE(librepcb::Package::AssemblyType)

#endif
