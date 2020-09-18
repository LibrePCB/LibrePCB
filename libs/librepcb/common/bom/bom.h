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

#ifndef LIBREPCB_BOM_H
#define LIBREPCB_BOM_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class BomItem
 ******************************************************************************/

/**
 * @brief The BomItem class represents an item of a bill of materials list
 */
class BomItem final {
  Q_DECLARE_TR_FUNCTIONS(BomItem)

public:
  // Constructors / Destructor
  BomItem() = delete;
  BomItem(const QString& designator, const QStringList& attributes) noexcept
    : mDesignators({designator}), mAttributes(attributes) {}
  BomItem(const BomItem& other) noexcept
    : mDesignators(other.mDesignators), mAttributes(other.mAttributes) {}
  ~BomItem() noexcept {}

  // Getters
  const QStringList& getDesignators() const noexcept { return mDesignators; }
  const QStringList& getAttributes() const noexcept { return mAttributes; }

  // General Methods
  void addDesignator(const QString& designator) noexcept {
    mDesignators.append(designator);

    // sort designators to improve readability of the BOM
    QCollator collator;
    collator.setCaseSensitivity(Qt::CaseInsensitive);
    collator.setIgnorePunctuation(false);
    collator.setNumericMode(true);
    std::sort(mDesignators.begin(), mDesignators.end(),
              [&collator](const QString& lhs, const QString& rhs) {
                return collator(lhs, rhs);
              });
  }

  // Operator Overloadings
  BomItem& operator=(const BomItem& rhs) noexcept {
    mDesignators = rhs.mDesignators;
    mAttributes = rhs.mAttributes;
    return *this;
  }

private:
  QStringList mDesignators;
  QStringList mAttributes;
};

/*******************************************************************************
 *  Class Bom
 ******************************************************************************/

/**
 * @brief The Bom class represents a bill of materials list
 */
class Bom final {
  Q_DECLARE_TR_FUNCTIONS(Bom)

public:
  // Constructors / Destructor
  Bom() = delete;
  Bom(const Bom& other) noexcept = delete;
  explicit Bom(const QStringList& columns) noexcept;
  ~Bom() noexcept;

  // Getters
  const QStringList& getColumns() const noexcept { return mColumns; }
  const QList<BomItem>& getItems() const noexcept { return mItems; }

  // General Methods
  void addItem(const QString& designator,
               const QStringList& attributes) noexcept;

  // Operator Overloadings
  Bom& operator=(const Bom& rhs) noexcept = delete;

private:
  QStringList mColumns;
  QList<BomItem> mItems;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif  // LIBREPCB_BOM_H
