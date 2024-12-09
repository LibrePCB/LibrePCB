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

#ifndef LIBREPCB_CORE_GERBEREXCELLONOUTPUTJOB_H
#define LIBREPCB_CORE_GERBEREXCELLONOUTPUTJOB_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "outputjob.h"

#include <QtCore>
#include <QtGui>

#include <memory>
#include <optional>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class GerberExcellonOutputJob
 ******************************************************************************/

/**
 * @brief Gerber/Excellon output job
 */
class GerberExcellonOutputJob final : public OutputJob {
  Q_DECLARE_TR_FUNCTIONS(GerberExcellonOutputJob)

public:
  using BoardSet = ObjectSet<Uuid>;

  // Constructors / Destructor
  GerberExcellonOutputJob(const GerberExcellonOutputJob& other) noexcept;
  explicit GerberExcellonOutputJob(const SExpression& node);
  virtual ~GerberExcellonOutputJob() noexcept;

  // Getters
  virtual QString getTypeTr() const noexcept override;
  virtual QIcon getTypeIcon() const noexcept override;
  const QString& getSuffixDrills() const noexcept { return mSuffixDrills; }
  const QString& getSuffixDrillsNpth() const noexcept {
    return mSuffixDrillsNpth;
  }
  const QString& getSuffixDrillsPth() const noexcept {
    return mSuffixDrillsPth;
  }
  const QString& getSuffixDrillsBlindBuried() const noexcept {
    return mSuffixDrillsBlindBuried;
  }
  const QString& getSuffixOutlines() const noexcept { return mSuffixOutlines; }
  const QString& getSuffixCopperTop() const noexcept {
    return mSuffixCopperTop;
  }
  const QString& getSuffixCopperInner() const noexcept {
    return mSuffixCopperInner;
  }
  const QString& getSuffixCopperBot() const noexcept {
    return mSuffixCopperBot;
  }
  const QString& getSuffixSolderMaskTop() const noexcept {
    return mSuffixSolderMaskTop;
  }
  const QString& getSuffixSolderMaskBot() const noexcept {
    return mSuffixSolderMaskBot;
  }
  const QString& getSuffixSilkscreenTop() const noexcept {
    return mSuffixSilkscreenTop;
  }
  const QString& getSuffixSilkscreenBot() const noexcept {
    return mSuffixSilkscreenBot;
  }
  const QString& getSuffixSolderPasteTop() const noexcept {
    return mSuffixSolderPasteTop;
  }
  const QString& getSuffixSolderPasteBot() const noexcept {
    return mSuffixSolderPasteBot;
  }
  bool getMergeDrillFiles() const noexcept { return mMergeDrillFiles; }
  bool getUseG85SlotCommand() const noexcept { return mUseG85SlotCommand; }
  bool getEnableSolderPasteTop() const noexcept {
    return mEnableSolderPasteTop;
  }
  bool getEnableSolderPasteBot() const noexcept {
    return mEnableSolderPasteBot;
  }
  const BoardSet& getBoards() const noexcept { return mBoards; }
  const QString& getOutputPath() const noexcept { return mOutputPath; }

  // Setters
  void setSuffixDrills(const QString& s) noexcept;
  void setSuffixDrillsNpth(const QString& s) noexcept;
  void setSuffixDrillsPth(const QString& s) noexcept;
  void setSuffixDrillsBlindBuried(const QString& s) noexcept;
  void setSuffixOutlines(const QString& s) noexcept;
  void setSuffixCopperTop(const QString& s) noexcept;
  void setSuffixCopperInner(const QString& s) noexcept;
  void setSuffixCopperBot(const QString& s) noexcept;
  void setSuffixSolderMaskTop(const QString& s) noexcept;
  void setSuffixSolderMaskBot(const QString& s) noexcept;
  void setSuffixSilkscreenTop(const QString& s) noexcept;
  void setSuffixSilkscreenBot(const QString& s) noexcept;
  void setSuffixSolderPasteTop(const QString& s) noexcept;
  void setSuffixSolderPasteBot(const QString& s) noexcept;
  void setMergeDrillFiles(bool m) noexcept;
  void setUseG85SlotCommand(bool u) noexcept;
  void setEnableSolderPasteTop(bool e) noexcept;
  void setEnableSolderPasteBot(bool e) noexcept;
  void setBoards(const BoardSet& boards) noexcept;
  void setOutputPath(const QString& path) noexcept;

  // General Methods
  static QString getTypeName() noexcept { return "gerber_excellon"; }
  static QString getTypeTrStatic() noexcept { return tr("Gerber/Excellon"); }
  virtual std::shared_ptr<OutputJob> cloneShared() const noexcept override;

  // Operator Overloadings
  GerberExcellonOutputJob& operator=(const GerberExcellonOutputJob& rhs) =
      delete;

  // Static Methods
  static std::shared_ptr<GerberExcellonOutputJob> defaultStyle() noexcept;
  static std::shared_ptr<GerberExcellonOutputJob> protelStyle() noexcept;

private:  // Methods
  GerberExcellonOutputJob() noexcept;
  virtual void serializeDerived(SExpression& root) const override;
  virtual bool equals(const OutputJob& rhs) const noexcept override;

private:  // Data
  QString mSuffixDrills;  // NPTH and PTH combined
  QString mSuffixDrillsNpth;
  QString mSuffixDrillsPth;
  QString mSuffixDrillsBlindBuried;  // Vias (plated)
  QString mSuffixOutlines;
  QString mSuffixCopperTop;
  QString mSuffixCopperInner;
  QString mSuffixCopperBot;
  QString mSuffixSolderMaskTop;
  QString mSuffixSolderMaskBot;
  QString mSuffixSilkscreenTop;
  QString mSuffixSilkscreenBot;
  QString mSuffixSolderPasteTop;
  QString mSuffixSolderPasteBot;
  bool mMergeDrillFiles;
  bool mUseG85SlotCommand;
  bool mEnableSolderPasteTop;
  bool mEnableSolderPasteBot;
  BoardSet mBoards;
  QString mOutputPath;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
