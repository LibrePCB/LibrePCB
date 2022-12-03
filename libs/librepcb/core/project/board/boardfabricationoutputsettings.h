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

#ifndef LIBREPCB_CORE_BOARDFABRICATIONOUTPUTSETTINGS_H
#define LIBREPCB_CORE_BOARDFABRICATIONOUTPUTSETTINGS_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class SExpression;

/*******************************************************************************
 *  Class BoardFabricationOutputSettings
 ******************************************************************************/

/**
 * @brief The BoardFabricationOutputSettings class
 */
class BoardFabricationOutputSettings final {
public:
  // Constructors / Destructor
  BoardFabricationOutputSettings() noexcept;
  BoardFabricationOutputSettings(
      const BoardFabricationOutputSettings& other) noexcept;
  explicit BoardFabricationOutputSettings(const SExpression& node);
  ~BoardFabricationOutputSettings() noexcept;

  // Getters
  const QString& getOutputBasePath() const noexcept { return mOutputBasePath; }
  const QString& getSuffixDrills() const noexcept { return mSuffixDrills; }
  const QString& getSuffixDrillsNpth() const noexcept {
    return mSuffixDrillsNpth;
  }
  const QString& getSuffixDrillsPth() const noexcept {
    return mSuffixDrillsPth;
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
  const QStringList& getSilkscreenLayersTop() const noexcept {
    return mSilkscreenLayersTop;
  }
  const QStringList& getSilkscreenLayersBot() const noexcept {
    return mSilkscreenLayersBot;
  }
  bool getMergeDrillFiles() const noexcept { return mMergeDrillFiles; }
  bool getEnableSolderPasteTop() const noexcept {
    return mEnableSolderPasteTop;
  }
  bool getEnableSolderPasteBot() const noexcept {
    return mEnableSolderPasteBot;
  }

  // Setters
  void setOutputBasePath(const QString& p) noexcept { mOutputBasePath = p; }
  void setSuffixDrills(const QString& s) noexcept { mSuffixDrills = s; }
  void setSuffixDrillsNpth(const QString& s) noexcept { mSuffixDrillsNpth = s; }
  void setSuffixDrillsPth(const QString& s) noexcept { mSuffixDrillsPth = s; }
  void setSuffixOutlines(const QString& s) noexcept { mSuffixOutlines = s; }
  void setSuffixCopperTop(const QString& s) noexcept { mSuffixCopperTop = s; }
  void setSuffixCopperInner(const QString& s) noexcept {
    mSuffixCopperInner = s;
  }
  void setSuffixCopperBot(const QString& s) noexcept { mSuffixCopperBot = s; }
  void setSuffixSolderMaskTop(const QString& s) noexcept {
    mSuffixSolderMaskTop = s;
  }
  void setSuffixSolderMaskBot(const QString& s) noexcept {
    mSuffixSolderMaskBot = s;
  }
  void setSuffixSilkscreenTop(const QString& s) noexcept {
    mSuffixSilkscreenTop = s;
  }
  void setSuffixSilkscreenBot(const QString& s) noexcept {
    mSuffixSilkscreenBot = s;
  }
  void setSuffixSolderPasteTop(const QString& s) noexcept {
    mSuffixSolderPasteTop = s;
  }
  void setSuffixSolderPasteBot(const QString& s) noexcept {
    mSuffixSolderPasteBot = s;
  }
  void setSilkscreenLayersTop(const QStringList& l) noexcept {
    mSilkscreenLayersTop = l;
  }
  void setSilkscreenLayersBot(const QStringList& l) noexcept {
    mSilkscreenLayersBot = l;
  }
  void setMergeDrillFiles(bool m) noexcept { mMergeDrillFiles = m; }
  void setEnableSolderPasteTop(bool e) noexcept { mEnableSolderPasteTop = e; }
  void setEnableSolderPasteBot(bool e) noexcept { mEnableSolderPasteBot = e; }

  // General Methods

  /**
   * @brief Serialize into ::librepcb::SExpression node
   *
   * @param root    Root node to serialize into.
   */
  void serialize(SExpression& root) const;

  // Operator Overloadings
  BoardFabricationOutputSettings& operator=(
      const BoardFabricationOutputSettings& rhs) noexcept;
  bool operator==(const BoardFabricationOutputSettings& rhs) const noexcept;
  bool operator!=(const BoardFabricationOutputSettings& rhs) const noexcept {
    return !(*this == rhs);
  }

private:  // Data
  QString mOutputBasePath;
  QString mSuffixDrills;  // NPTH and PTH combined
  QString mSuffixDrillsNpth;
  QString mSuffixDrillsPth;
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
  QStringList mSilkscreenLayersTop;
  QStringList mSilkscreenLayersBot;
  bool mMergeDrillFiles;
  bool mEnableSolderPasteTop;
  bool mEnableSolderPasteBot;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
