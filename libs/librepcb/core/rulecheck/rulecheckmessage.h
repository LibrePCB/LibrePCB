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

#ifndef LIBREPCB_CORE_RULECHECKMESSAGE_H
#define LIBREPCB_CORE_RULECHECKMESSAGE_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../geometry/path.h"
#include "../serialization/sexpression.h"

#include <QtCore>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class RuleCheckMessage
 ******************************************************************************/

/**
 * @brief The RuleCheckMessage class
 */
class RuleCheckMessage {
  Q_DECLARE_TR_FUNCTIONS(RuleCheckMessage)

public:
  /// Message severity type (higher number = higher severity)
  enum class Severity {
    Hint = 0,
    Warning = 1,
    Error = 2,
  };

  // Constructors / Destructor
  RuleCheckMessage() = delete;

  // Getters
  Severity getSeverity() const noexcept { return mSeverity; }
  QString getSeverityTr() const noexcept;
  const QIcon& getSeverityIcon() const noexcept;
  const QString& getMessage() const noexcept { return mMessage; }
  const QString& getDescription() const noexcept { return mDescription; }
  const SExpression& getApproval() const noexcept { return mApproval; }
  const QVector<Path>& getLocations() const noexcept { return mLocations; }

  // General Methods
  template <typename T>
  T* as() noexcept {
    return dynamic_cast<T*>(this);
  }
  template <typename T>
  const T* as() const noexcept {
    return dynamic_cast<const T*>(this);
  }

  // Static Methods
  static QString getSeverityTr(Severity severity) noexcept;
  static const QIcon& getSeverityIcon(Severity severity) noexcept;
  static QSet<SExpression> getAllApprovals(
      const QVector<std::shared_ptr<const RuleCheckMessage>>&
          messages) noexcept;

  // Operator Overloads
  bool operator==(const RuleCheckMessage& rhs) const noexcept;
  bool operator!=(const RuleCheckMessage& rhs) const noexcept;

protected:  // Methods
  RuleCheckMessage(const RuleCheckMessage& other) noexcept;
  RuleCheckMessage(Severity severity, const QString& msg,
                   const QString& description, const QString& approvalName,
                   const QVector<Path>& locations = {}) noexcept;
  virtual ~RuleCheckMessage() noexcept;

protected:  // Data
  Severity mSeverity;
  QString mMessage;
  QString mDescription;
  SExpression mApproval;
  QVector<Path> mLocations;
};

typedef QVector<std::shared_ptr<const RuleCheckMessage>> RuleCheckMessageList;

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
