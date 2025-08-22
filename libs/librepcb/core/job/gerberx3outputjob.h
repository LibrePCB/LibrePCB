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

#ifndef LIBREPCB_CORE_GERBERX3OUTPUTJOB_H
#define LIBREPCB_CORE_GERBERX3OUTPUTJOB_H

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
 *  Class GerberX3OutputJob
 ******************************************************************************/

/**
 * @brief Gerber X3 pick&place output job
 */
class GerberX3OutputJob final : public OutputJob {
  Q_DECLARE_TR_FUNCTIONS(GerberX3OutputJob)

public:
  // Types
  using BoardSet = ObjectSet<Uuid>;
  using AssemblyVariantSet = ObjectSet<Uuid>;

  // Constructors / Destructor
  GerberX3OutputJob() noexcept;
  GerberX3OutputJob(const GerberX3OutputJob& other) noexcept;
  explicit GerberX3OutputJob(const SExpression& node);
  virtual ~GerberX3OutputJob() noexcept;

  // Getters
  virtual QString getTypeTr() const noexcept override;
  virtual QIcon getTypeIcon() const noexcept override;
  const BoardSet& getBoards() const noexcept { return mBoards; }
  const AssemblyVariantSet& getAssemblyVariants() const noexcept {
    return mAssemblyVariants;
  }
  bool getEnableComponentsTop() const noexcept { return mEnableComponentsTop; }
  bool getEnableComponentsBot() const noexcept { return mEnableComponentsBot; }
  const QString& getOutputPathComponentsTop() const noexcept {
    return mOutputPathComponentsTop;
  }
  const QString& getOutputPathComponentsBot() const noexcept {
    return mOutputPathComponentsBot;
  }
  bool getEnableGlueTop() const noexcept { return mEnableGlueTop; }
  bool getEnableGlueBot() const noexcept { return mEnableGlueBot; }
  const QString& getOutputPathGlueTop() const noexcept {
    return mOutputPathGlueTop;
  }
  const QString& getOutputPathGlueBot() const noexcept {
    return mOutputPathGlueBot;
  }

  // Setters
  void setBoards(const BoardSet& boards) noexcept;
  void setAssemblyVariants(const AssemblyVariantSet& avs) noexcept;
  void setEnableComponentsTop(bool create) noexcept;
  void setEnableComponentsBot(bool create) noexcept;
  void setOutputPathComponentsTop(const QString& path) noexcept;
  void setOutputPathComponentsBot(const QString& path) noexcept;
  void setEnableGlueTop(bool create) noexcept;
  void setEnableGlueBot(bool create) noexcept;
  void setOutputPathGlueTop(const QString& path) noexcept;
  void setOutputPathGlueBot(const QString& path) noexcept;

  // General Methods
  static QString getTypeName() noexcept { return "gerber_x3"; }
  static QString getTypeTrStatic() noexcept {
    return tr("Pick&Place / Glue Mask (Gerber X3)");
  }
  virtual std::shared_ptr<OutputJob> cloneShared() const noexcept override;

  // Operator Overloadings
  GerberX3OutputJob& operator=(const GerberX3OutputJob& rhs) = delete;

private:  // Methods
  virtual void serializeDerived(SExpression& root) const override;
  virtual bool equals(const OutputJob& rhs) const noexcept override;

private:  // Data
  BoardSet mBoards;
  AssemblyVariantSet mAssemblyVariants;
  bool mEnableComponentsTop;
  bool mEnableComponentsBot;
  QString mOutputPathComponentsTop;
  QString mOutputPathComponentsBot;
  bool mEnableGlueTop;
  bool mEnableGlueBot;
  QString mOutputPathGlueTop;
  QString mOutputPathGlueBot;
};

}  // namespace librepcb

/*******************************************************************************
 *  End of File
 ******************************************************************************/

#endif
