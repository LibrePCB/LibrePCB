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

#ifndef LIBREPCB_CORE_CORPORATE_H
#define LIBREPCB_CORE_CORPORATE_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../librarybaseelement.h"
#include "pcbmanufacturercapabilities.h"

#include <QtCore>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class Corporate
 ******************************************************************************/

/**
 * @brief The Corporate class represents a PCB manufacturer, assembly house etc.
 */
class Corporate final : public LibraryBaseElement {
  Q_OBJECT

public:
  // Constructors / Destructor
  Corporate() = delete;
  Corporate(const Corporate& other) = delete;
  Corporate(const Uuid& uuid, const Version& version, const QString& author,
            const ElementName& name_en_US, const QString& description_en_US,
            const QString& keywords_en_US);
  ~Corporate() noexcept;

  // Getters
  const QByteArray& getIcon() const noexcept { return mIcon; }
  QPixmap getIconAsPixmap() const noexcept;
  const QUrl& getUrl() const noexcept { return mUrl; }
  const QString& getCountry() const noexcept { return mCountry; }
  const QStringList& getFabs() const noexcept { return mFabs; }
  const QStringList& getShipping() const noexcept { return mShipping; }
  bool isSponsor() const noexcept { return mIsSponsor; }
  int getPriority() const noexcept { return mPriority; }
  const QVector<PcbManufacturerCapabilities>& getPcbCapabilities()
      const noexcept {
    return mPcbCapabilities;
  }
  const PcbManufacturerCapabilities* findPcbCapabilities(
      const Uuid& uuid) const noexcept;

  // Setters
  void setIcon(const QByteArray& png) noexcept { mIcon = png; }
  void setUrl(const QUrl& url) noexcept { mUrl = url; }

  // General Methods
  virtual RuleCheckMessageList runChecks() const override;
  virtual void save() override;

  // Operator Overloadings
  Corporate& operator=(const Corporate& rhs) = delete;

  // Static Methods
  static std::unique_ptr<Corporate> open(
      std::unique_ptr<TransactionalDirectory> directory,
      bool abortBeforeMigration = false);
  static QString getShortElementName() noexcept {
    return QStringLiteral("corp");
  }
  static QString getLongElementName() noexcept {
    return QStringLiteral("corporate");
  }

protected:  // Methods
  virtual void serialize(SExpression& root) const override;

private:  // Methods
  Corporate(std::unique_ptr<TransactionalDirectory> directory,
            const SExpression& root);

private:  // Data
  QByteArray mIcon;
  QUrl mUrl;
  QString mCountry;
  QStringList mFabs;
  QStringList mShipping;
  bool mIsSponsor;
  int mPriority;
  QVector<PcbManufacturerCapabilities> mPcbCapabilities;

  // Arbitrary options for forward compatibility in case we really need to
  // add new settings in a minor release.
  QMap<QString, QList<SExpression>> mOptions;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
