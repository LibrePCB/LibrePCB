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

#ifndef LIBREPCB_CORE_COPYOUTPUTJOB_H
#define LIBREPCB_CORE_COPYOUTPUTJOB_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "outputjob.h"

#include <QtCore>
#include <QtGui>

#include <optional>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class CopyOutputJob
 ******************************************************************************/

/**
 * @brief File copy output job
 */
class CopyOutputJob final : public OutputJob {
  Q_DECLARE_TR_FUNCTIONS(CopyOutputJob)

public:
  using BoardSet = ObjectSet<std::optional<Uuid>>;
  using AssemblyVariantSet = ObjectSet<std::optional<Uuid>>;

  // Constructors / Destructor
  CopyOutputJob() noexcept;
  CopyOutputJob(const CopyOutputJob& other) noexcept;
  explicit CopyOutputJob(const SExpression& node);
  virtual ~CopyOutputJob() noexcept;

  // Getters
  virtual QString getTypeTr() const noexcept override;
  virtual QIcon getTypeIcon() const noexcept override;
  bool getSubstituteVariables() const noexcept { return mSubstituteVariables; }
  const BoardSet& getBoards() const noexcept { return mBoards; }
  const AssemblyVariantSet& getAssemblyVariants() const noexcept {
    return mAssemblyVariants;
  }
  const QString& getInputPath() const noexcept { return mInputPath; }
  const QString& getOutputPath() const noexcept { return mOutputPath; }

  // Setters
  void setSubstituteVariables(bool subst) noexcept;
  void setBoards(const BoardSet& boards) noexcept;
  void setAssemblyVariants(const AssemblyVariantSet& avs) noexcept;
  void setInputPath(const QString& path) noexcept;
  void setOutputPath(const QString& path) noexcept;

  // General Methods
  static QString getTypeName() noexcept { return "copy"; }
  static QString getTypeTrStatic() noexcept { return tr("File Copy"); }
  virtual std::shared_ptr<OutputJob> cloneShared() const noexcept override;

  // Operator Overloadings
  CopyOutputJob& operator=(const CopyOutputJob& rhs) = delete;

private:  // Methods
  virtual void serializeDerived(SExpression& root) const override;
  virtual bool equals(const OutputJob& rhs) const noexcept override;

private:  // Data
  bool mSubstituteVariables;
  BoardSet mBoards;
  AssemblyVariantSet mAssemblyVariants;
  QString mInputPath;
  QString mOutputPath;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
