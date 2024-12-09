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

#ifndef LIBREPCB_CORE_BOARD3DOUTPUTJOB_H
#define LIBREPCB_CORE_BOARD3DOUTPUTJOB_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "outputjob.h"

#include <QtCore>
#include <QtGui>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class Board3DOutputJob
 ******************************************************************************/

/**
 * @brief 3D board model output job
 */
class Board3DOutputJob final : public OutputJob {
  Q_DECLARE_TR_FUNCTIONS(Board3DOutputJob)

public:
  using BoardSet = ObjectSet<Uuid>;
  using AssemblyVariantSet = ObjectSet<std::optional<Uuid>>;

  // Constructors / Destructor
  Board3DOutputJob() noexcept;
  Board3DOutputJob(const Board3DOutputJob& other) noexcept;
  explicit Board3DOutputJob(const SExpression& node);
  virtual ~Board3DOutputJob() noexcept;

  // Getters
  virtual QString getTypeTr() const noexcept override;
  virtual QIcon getTypeIcon() const noexcept override;
  const BoardSet& getBoards() const noexcept { return mBoards; }
  const AssemblyVariantSet& getAssemblyVariants() const noexcept {
    return mAssemblyVariants;
  }
  const QString& getOutputPath() const noexcept { return mOutputPath; }

  // Setters
  void setBoards(const BoardSet& boards) noexcept;
  void setAssemblyVariants(const AssemblyVariantSet& avs) noexcept;
  void setOutputPath(const QString& path) noexcept;

  // General Methods
  static QString getTypeName() noexcept { return "3d_model"; }
  static QString getTypeTrStatic() noexcept {
    return tr("3D Model") % " (*.step)";
  }
  virtual std::shared_ptr<OutputJob> cloneShared() const noexcept override;

  // Operator Overloadings
  Board3DOutputJob& operator=(const Board3DOutputJob& rhs) = delete;

protected:  // Methods
  virtual void serializeDerived(SExpression& root) const override;
  virtual bool equals(const OutputJob& rhs) const noexcept override;

private:  // Data
  BoardSet mBoards;
  AssemblyVariantSet mAssemblyVariants;
  QString mOutputPath;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
