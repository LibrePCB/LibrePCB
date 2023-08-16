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

#ifndef LIBREPCB_CORE_NETLISTOUTPUTJOB_H
#define LIBREPCB_CORE_NETLISTOUTPUTJOB_H

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
 *  Class NetlistOutputJob
 ******************************************************************************/

/**
 * @brief Netlist output job
 */
class NetlistOutputJob final : public OutputJob {
  Q_DECLARE_TR_FUNCTIONS(NetlistOutputJob)

public:
  using BoardSet = ObjectSet<Uuid>;

  // Constructors / Destructor
  NetlistOutputJob() noexcept;
  NetlistOutputJob(const NetlistOutputJob& other) noexcept;
  explicit NetlistOutputJob(const SExpression& node);
  virtual ~NetlistOutputJob() noexcept;

  // Getters
  virtual QString getTypeTr() const noexcept override;
  virtual QIcon getTypeIcon() const noexcept override;
  const BoardSet& getBoards() const noexcept { return mBoards; }
  const QString& getOutputPath() const noexcept { return mOutputPath; }

  // Setters
  void setBoards(const BoardSet& boards) noexcept;
  void setOutputPath(const QString& path) noexcept;

  // General Methods
  static QString getTypeName() noexcept { return "netlist"; }
  static QString getTypeTrStatic() noexcept {
    return tr("Netlist") % " (*.d356)";
  }
  virtual std::shared_ptr<OutputJob> cloneShared() const noexcept override;

  // Operator Overloadings
  NetlistOutputJob& operator=(const NetlistOutputJob& rhs) = delete;

protected:  // Methods
  virtual void serializeDerived(SExpression& root) const override;
  virtual bool equals(const OutputJob& rhs) const noexcept override;

private:  // Data
  BoardSet mBoards;
  QString mOutputPath;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
