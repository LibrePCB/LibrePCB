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

#ifndef LIBREPCB_CORE_AUTOROUTER_H
#define LIBREPCB_CORE_AUTOROUTER_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <QtCore>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class MessageLogger;

/*******************************************************************************
 *  Class Autorouter
 ******************************************************************************/

/**
 * @brief Base class for different kinds of autorouters
 */
class Autorouter : public QObject {
  Q_OBJECT

public:
  // Types
  enum class State {
    Running,
    Succeeded,
    Failed,
    Canceled,
  };
  struct Status {
    State state = State::Running;
    int progressPercent = 0;
    QByteArray ses;
    QString error;
  };

  // Constructors / Destructor
  Autorouter() noexcept = delete;
  explicit Autorouter(const std::shared_ptr<MessageLogger>& logger) noexcept;
  Autorouter(const Autorouter& other) = delete;
  ~Autorouter() noexcept override;

  // General Methods
  virtual void start(const QByteArray& dsn) noexcept = 0;
  virtual void cancel() noexcept = 0;

  // Operator Overloadings
  Autorouter& operator=(const Autorouter& rhs) = delete;

signals:
  void statusNotification(const Status& status);

protected:
  std::shared_ptr<MessageLogger> mLogger;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
