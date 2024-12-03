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

#ifndef LIBREPCB_CORE_PICKPLACEOUTPUTJOB_H
#define LIBREPCB_CORE_PICKPLACEOUTPUTJOB_H

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
 *  Class PickPlaceOutputJob
 ******************************************************************************/

/**
 * @brief Pick&Place output job
 */
class PickPlaceOutputJob final : public OutputJob {
  Q_DECLARE_TR_FUNCTIONS(PickPlaceOutputJob)

public:
  // Types
  using BoardSet = ObjectSet<Uuid>;
  using AssemblyVariantSet = ObjectSet<Uuid>;

  enum class Technology : int {
    Tht = (1 << 0),
    Smt = (1 << 1),
    Mixed = (1 << 2),
    Fiducial = (1 << 3),
    Other = (1 << 4),
    All = Tht | Smt | Mixed | Fiducial | Other,
  };
  Q_DECLARE_FLAGS(Technologies, Technology)

  // Constructors / Destructor
  PickPlaceOutputJob() noexcept;
  PickPlaceOutputJob(const PickPlaceOutputJob& other) noexcept;
  explicit PickPlaceOutputJob(const SExpression& node);
  virtual ~PickPlaceOutputJob() noexcept;

  // Getters
  virtual QString getTypeTr() const noexcept override;
  virtual QIcon getTypeIcon() const noexcept override;
  Technologies getTechnologies() const noexcept { return mTechnologies; }
  bool getIncludeComment() const noexcept { return mIncludeComment; }
  const BoardSet& getBoards() const noexcept { return mBoards; }
  const AssemblyVariantSet& getAssemblyVariants() const noexcept {
    return mAssemblyVariants;
  }
  bool getCreateTop() const noexcept { return mCreateTop; }
  bool getCreateBottom() const noexcept { return mCreateBottom; }
  bool getCreateBoth() const noexcept { return mCreateBoth; }
  const QString& getOutputPathTop() const noexcept { return mOutputPathTop; }
  const QString& getOutputPathBottom() const noexcept {
    return mOutputPathBottom;
  }
  const QString& getOutputPathBoth() const noexcept { return mOutputPathBoth; }

  // Setters
  void setTechnologies(Technologies technologies) noexcept;
  void setIncludeComment(bool include) noexcept;
  void setBoards(const BoardSet& boards) noexcept;
  void setAssemblyVariants(const AssemblyVariantSet& avs) noexcept;
  void setCreateTop(bool create) noexcept;
  void setCreateBottom(bool create) noexcept;
  void setCreateBoth(bool create) noexcept;
  void setOutputPathTop(const QString& path) noexcept;
  void setOutputPathBottom(const QString& path) noexcept;
  void setOutputPathBoth(const QString& path) noexcept;

  // General Methods
  static QString getTypeName() noexcept { return "pnp"; }
  static QString getTypeTrStatic() noexcept {
    return tr("Pick&Place") % " (*.csv)";
  }
  virtual std::shared_ptr<OutputJob> cloneShared() const noexcept override;

  // Operator Overloadings
  PickPlaceOutputJob& operator=(const PickPlaceOutputJob& rhs) = delete;

private:  // Methods
  virtual void serializeDerived(SExpression& root) const override;
  virtual bool equals(const OutputJob& rhs) const noexcept override;

private:  // Data
  Technologies mTechnologies;
  bool mIncludeComment;
  BoardSet mBoards;
  AssemblyVariantSet mAssemblyVariants;
  bool mCreateTop;
  bool mCreateBottom;
  bool mCreateBoth;
  QString mOutputPathTop;
  QString mOutputPathBottom;
  QString mOutputPathBoth;
};

}  // namespace librepcb

Q_DECLARE_OPERATORS_FOR_FLAGS(librepcb::PickPlaceOutputJob::Technologies)

/*******************************************************************************
 *  End of File
 ******************************************************************************/

#endif
