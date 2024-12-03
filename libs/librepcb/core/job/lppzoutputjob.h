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

#ifndef LIBREPCB_CORE_LPPZOUTPUTJOB_H
#define LIBREPCB_CORE_LPPZOUTPUTJOB_H

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
 *  Class LppzOutputJob
 ******************************************************************************/

/**
 * @brief Zipped project (*.lppz) output job
 */
class LppzOutputJob final : public OutputJob {
  Q_DECLARE_TR_FUNCTIONS(LppzOutputJob)

public:
  // Constructors / Destructor
  LppzOutputJob() noexcept;
  LppzOutputJob(const LppzOutputJob& other) noexcept;
  explicit LppzOutputJob(const SExpression& node);
  virtual ~LppzOutputJob() noexcept;

  // Getters
  virtual QString getTypeTr() const noexcept override;
  virtual QIcon getTypeIcon() const noexcept override;
  const QString& getOutputPath() const noexcept { return mOutputPath; }

  // Setters
  void setOutputPath(const QString& path) noexcept;

  // General Methods
  static QString getTypeName() noexcept { return "lppz"; }
  static QString getTypeTrStatic() noexcept {
    return tr("Project Archive") % " (*.lppz)";
  }
  virtual std::shared_ptr<OutputJob> cloneShared() const noexcept override;

  // Operator Overloadings
  LppzOutputJob& operator=(const LppzOutputJob& rhs) = delete;

private:  // Methods
  virtual void serializeDerived(SExpression& root) const override;
  virtual bool equals(const OutputJob& rhs) const noexcept override;

private:  // Data
  QString mOutputPath;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
