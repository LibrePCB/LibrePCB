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

#ifndef LIBREPCB_CORE_LIBRARYELEMENTCHECKMESSAGE_H
#define LIBREPCB_CORE_LIBRARYELEMENTCHECKMESSAGE_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../../serialization/sexpression.h"

#include <QtCore>
#include <QtWidgets>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class LibraryElementCheckMessage
 ******************************************************************************/

/**
 * @brief The LibraryElementCheckMessage class
 */
class LibraryElementCheckMessage {
  Q_DECLARE_TR_FUNCTIONS(LibraryElementCheckMessage)

public:
  /// Message severity type (higher number = higher severity)
  enum class Severity {
    Hint = 0,
    Warning = 1,
    Error = 2,
  };

  // Constructors / Destructor
  LibraryElementCheckMessage() = delete;

  // Getters
  Severity getSeverity() const noexcept { return mSeverity; }
  const QIcon& getSeverityIcon() const noexcept { return mSeverityIcon; }
  const QString& getMessage() const noexcept { return mMessage; }
  const QString& getDescription() const noexcept { return mDescription; }
  const SExpression& getApproval() const noexcept { return mApproval; }

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
  static QIcon getSeverityIcon(Severity severity) noexcept;

  // Operator Overloads
  bool operator==(const LibraryElementCheckMessage& rhs) const noexcept;
  bool operator!=(const LibraryElementCheckMessage& rhs) const noexcept;

protected:  // Methods
  LibraryElementCheckMessage(const LibraryElementCheckMessage& other) noexcept;
  LibraryElementCheckMessage(Severity severity, const QString& msg,
                             const QString& description,
                             const QString& approvalName) noexcept;
  virtual ~LibraryElementCheckMessage() noexcept;

protected:  // Data
  Severity mSeverity;
  QIcon mSeverityIcon;
  QString mMessage;
  QString mDescription;
  SExpression mApproval;
};

typedef QVector<std::shared_ptr<const LibraryElementCheckMessage>>
    LibraryElementCheckMessageList;

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
