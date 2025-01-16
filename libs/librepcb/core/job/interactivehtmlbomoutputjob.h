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

#ifndef LIBREPCB_CORE_INTERACTIVEHTMLBOMOUTPUTJOB_H
#define LIBREPCB_CORE_INTERACTIVEHTMLBOMOUTPUTJOB_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../export/interactivehtmlbom.h"
#include "outputjob.h"

#include <QtCore>
#include <QtGui>

#include <optional>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class InteractiveHtmlBomOutputJob
 ******************************************************************************/

/**
 * @brief Interactive HTML BOM output job
 */
class InteractiveHtmlBomOutputJob final : public OutputJob {
  Q_DECLARE_TR_FUNCTIONS(InteractiveHtmlBomOutputJob)

public:
  using BoardSet = ObjectSet<Uuid>;
  using AssemblyVariantSet = ObjectSet<Uuid>;

  // Constructors / Destructor
  InteractiveHtmlBomOutputJob() noexcept;
  InteractiveHtmlBomOutputJob(
      const InteractiveHtmlBomOutputJob& other) noexcept;
  explicit InteractiveHtmlBomOutputJob(const SExpression& node);
  virtual ~InteractiveHtmlBomOutputJob() noexcept;

  // Getters
  virtual QString getTypeTr() const noexcept override;
  virtual QIcon getTypeIcon() const noexcept override;
  InteractiveHtmlBom::ViewMode getViewMode() const noexcept {
    return mViewMode;
  }
  InteractiveHtmlBom::HighlightPin1Mode getHighlightPin1() const noexcept {
    return mHighlightPin1;
  }
  bool getDarkMode() const noexcept { return mDarkMode; }
  const Angle& getBoardRotation() const noexcept { return mBoardRotation; }
  bool getOffsetBackRotation() const noexcept { return mOffsetBackRotation; }
  bool getShowSilkscreen() const noexcept { return mShowSilkscreen; }
  bool getShowFabrication() const noexcept { return mShowFabrication; }
  bool getShowPads() const noexcept { return mShowPads; }
  bool getShowTracks() const noexcept { return mShowTracks; }
  bool getShowZones() const noexcept { return mShowZones; }
  const QStringList& getCheckBoxes() const noexcept { return mCheckBoxes; }
  const QStringList& getComponentOrder() const noexcept {
    return mComponentOrder;
  }
  const QStringList& getCustomAttributes() const noexcept {
    return mCustomAttributes;
  }
  const BoardSet& getBoards() const noexcept { return mBoards; }
  const AssemblyVariantSet& getAssemblyVariants() const noexcept {
    return mAssemblyVariants;
  }
  const QString& getOutputPath() const noexcept { return mOutputPath; }

  // Setters
  void setViewMode(InteractiveHtmlBom::ViewMode mode) noexcept;
  void setHighlightPin1(InteractiveHtmlBom::HighlightPin1Mode mode) noexcept;
  void setDarkMode(bool dark) noexcept;
  void setBoardRotation(const Angle& rot) noexcept;
  void setOffsetBackRotation(bool offset) noexcept;
  void setShowSilkscreen(bool show) noexcept;
  void setShowFabrication(bool show) noexcept;
  void setShowPads(bool show) noexcept;
  void setShowTracks(bool show) noexcept;
  void setShowZones(bool show) noexcept;
  void setCheckBoxes(const QStringList& cbx) noexcept;
  void setComponentOrder(const QStringList& order) noexcept;
  void setCustomAttributes(const QStringList& attrs) noexcept;
  void setBoards(const BoardSet& boards) noexcept;
  void setAssemblyVariants(const AssemblyVariantSet& avs) noexcept;
  void setOutputPath(const QString& path) noexcept;

  // General Methods
  static QString getTypeName() noexcept { return "interactive_bom"; }
  static QString getTypeTrStatic() noexcept {
    return tr("Interactive Bill Of Materials") % " (*.html)";
  }
  virtual std::shared_ptr<OutputJob> cloneShared() const noexcept override;

  // Operator Overloadings
  InteractiveHtmlBomOutputJob& operator=(
      const InteractiveHtmlBomOutputJob& rhs) = delete;

private:  // Methods
  virtual void serializeDerived(SExpression& root) const override;
  virtual bool equals(const OutputJob& rhs) const noexcept override;

private:  // Data
  InteractiveHtmlBom::ViewMode mViewMode;
  InteractiveHtmlBom::HighlightPin1Mode mHighlightPin1;
  bool mDarkMode;
  Angle mBoardRotation;
  bool mOffsetBackRotation;
  bool mShowSilkscreen;
  bool mShowFabrication;
  bool mShowPads;
  bool mShowTracks;
  bool mShowZones;
  QStringList mCheckBoxes;
  QStringList mComponentOrder;
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
