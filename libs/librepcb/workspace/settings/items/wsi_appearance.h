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

#ifndef LIBREPCB_WSI_APPEARANCE_H
#define LIBREPCB_WSI_APPEARANCE_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "wsi_base.h"

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace workspace {

/*******************************************************************************
 *  Class WSI_Appearance
 ******************************************************************************/

/**
 * @brief The WSI_Appearance class
 */
class WSI_Appearance final : public WSI_Base {
  Q_OBJECT

public:
  // Constructors / Destructor
  WSI_Appearance()                            = delete;
  WSI_Appearance(const WSI_Appearance& other) = delete;
  explicit WSI_Appearance(const SExpression& node);
  ~WSI_Appearance() noexcept;

  // Getters
  bool getUseOpenGl() const noexcept { return mUseOpenGlCheckBox->isChecked(); }

  // Getters: Widgets
  QString getUseOpenGlLabelText() const noexcept {
    return tr("Rendering Method:");
  }
  QWidget* getUseOpenGlWidget() const noexcept {
    return mUseOpenGlWidget.data();
  }

  // General Methods
  void restoreDefault() noexcept override;
  void apply() noexcept override;
  void revert() noexcept override;

  /// @copydoc librepcb::SerializableObject::serialize()
  void serialize(SExpression& root) const override;

  // Operator Overloadings
  WSI_Appearance& operator=(const WSI_Appearance& rhs) = delete;

private:  // Data
  bool mUseOpenGl;

  // Widgets
  QScopedPointer<QWidget>   mUseOpenGlWidget;
  QScopedPointer<QCheckBox> mUseOpenGlCheckBox;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace workspace
}  // namespace librepcb

#endif  // LIBREPCB_WSI_APPEARANCE_H
