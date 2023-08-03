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

#ifndef LIBREPCB_CORE_UNKNOWNOUTPUTJOB_H
#define LIBREPCB_CORE_UNKNOWNOUTPUTJOB_H

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
 *  Class UnknownOutputJob
 ******************************************************************************/

/**
 * @brief Fallback output job for unknown types
 *
 * Intended to provide kind of forward compatibility with output jobs
 * implemented in a later LibrePCB release.
 */
class UnknownOutputJob final : public OutputJob {
  Q_DECLARE_TR_FUNCTIONS(UnknownOutputJob)

public:
  // Constructors / Destructor
  UnknownOutputJob() = delete;
  UnknownOutputJob(const UnknownOutputJob& other) noexcept;
  explicit UnknownOutputJob(const SExpression& node);
  virtual ~UnknownOutputJob() noexcept;

  // Getters
  virtual QString getTypeTr() const noexcept override;
  virtual QIcon getTypeIcon() const noexcept override;

  // General Methods
  virtual std::shared_ptr<OutputJob> cloneShared() const noexcept override;
  virtual void serialize(SExpression& root) const override;

  // Operator Overloadings
  UnknownOutputJob& operator=(const UnknownOutputJob& rhs) = delete;

private:  // Methods
  virtual void serializeDerived(SExpression& root) const override;
  virtual bool equals(const OutputJob& rhs) const noexcept override;

private:  // Data
  SExpression mNode;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
