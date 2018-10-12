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

#ifndef LIBREPCB_WSI_APPLOCALE_H
#define LIBREPCB_WSI_APPLOCALE_H

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
 *  Class WSI_AppLocale
 ******************************************************************************/

/**
 * @brief The WSI_AppLocale class represents the application's locale settings
 *        (for translation and localization)
 *
 * @author ubruhin
 * @date 2014-10-04
 */
class WSI_AppLocale final : public WSI_Base {
  Q_OBJECT

public:
  // Constructors / Destructor
  WSI_AppLocale()                           = delete;
  WSI_AppLocale(const WSI_AppLocale& other) = delete;
  explicit WSI_AppLocale(const SExpression& node);
  ~WSI_AppLocale() noexcept;

  // Getters
  const QString& getAppLocaleName() const noexcept { return mAppLocale; }

  // Getters: Widgets
  QString  getLabelText() const noexcept { return tr("Application Language:"); }
  QWidget* getWidget() const noexcept { return mWidget.data(); }

  // General Methods
  void restoreDefault() noexcept override;
  void apply() noexcept override;
  void revert() noexcept override;

  /// @copydoc librepcb::SerializableObject::serialize()
  void serialize(SExpression& root) const override;

  // Operator Overloadings
  WSI_AppLocale& operator=(const WSI_AppLocale& rhs) = delete;

private:  // Methods
  void comboBoxIndexChanged(int index) noexcept;
  void updateComboBoxIndex() noexcept;

private:  // Data
  /**
   * @brief The locale name
   *
   * Examples:
   *  - QString("de_CH") for German/Switzerland
   *  - QString("") or QString() means "use system locale"
   *
   * Default: QString()
   */
  QString mAppLocale;
  QString mAppLocaleTmp;

  QList<QTranslator*>
      mInstalledTranslators;  ///< see constructor/destructor code

  // Widgets
  QScopedPointer<QWidget>   mWidget;
  QScopedPointer<QComboBox> mComboBox;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace workspace
}  // namespace librepcb

#endif  // LIBREPCB_WSI_APPLOCALE_H
