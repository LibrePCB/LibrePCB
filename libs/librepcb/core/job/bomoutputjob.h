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

#ifndef LIBREPCB_CORE_BOMOUTPUTJOB_H
#define LIBREPCB_CORE_BOMOUTPUTJOB_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "outputjob.h"

#include <optional/tl/optional.hpp>

#include <QtCore>
#include <QtGui>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class BomOutputJob
 ******************************************************************************/

/**
 * @brief BOM output job
 */
class BomOutputJob final : public OutputJob {
  Q_DECLARE_TR_FUNCTIONS(BomOutputJob)

public:
  using BoardSet = ObjectSet<tl::optional<Uuid>>;
  using AssemblyVariantSet = ObjectSet<Uuid>;

  // Constructors / Destructor
  BomOutputJob() noexcept;
  BomOutputJob(const BomOutputJob& other) noexcept;
  explicit BomOutputJob(const SExpression& node);
  virtual ~BomOutputJob() noexcept;

  // Getters
  virtual QString getTypeTr() const noexcept override;
  virtual QIcon getTypeIcon() const noexcept override;
  const QStringList& getCustomAttributes() const noexcept {
    return mCustomAttributes;
  }
  const BoardSet& getBoards() const noexcept { return mBoards; }
  const AssemblyVariantSet& getAssemblyVariants() const noexcept {
    return mAssemblyVariants;
  }
  const QString& getOutputPath() const noexcept { return mOutputPath; }

  // Setters
  void setCustomAttributes(const QStringList& attrs) noexcept;
  void setBoards(const BoardSet& boards) noexcept;
  void setAssemblyVariants(const AssemblyVariantSet& avs) noexcept;
  void setOutputPath(const QString& path) noexcept;

  // General Methods
  static QString getTypeName() noexcept { return "bom"; }
  static QString getTypeTrStatic() noexcept {
    return tr("Bill Of Materials") % " (*.csv)";
  }
  virtual std::shared_ptr<OutputJob> cloneShared() const noexcept override;

  // Operator Overloadings
  BomOutputJob& operator=(const BomOutputJob& rhs) = delete;

private:  // Methods
  virtual void serializeDerived(SExpression& root) const override;
  virtual bool equals(const OutputJob& rhs) const noexcept override;

private:  // Data
  QStringList mCustomAttributes;
  BoardSet mBoards;
  AssemblyVariantSet mAssemblyVariants;
  QString mOutputPath;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
