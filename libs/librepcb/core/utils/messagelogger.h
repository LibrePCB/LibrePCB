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

#ifndef LIBREPCB_CORE_MESSAGELOGGER_H
#define LIBREPCB_CORE_MESSAGELOGGER_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class MessageLogger
 ******************************************************************************/

/**
 * @brief Generic logger class to pass messages between objects
 *
 * @note  This class is thread-safe, i.e. several threads can log or retrieve
 *        logging messages simultanously.
 */
class MessageLogger final : public QObject {
  Q_OBJECT

public:
  struct Message {
    QtMsgType type;
    QString message;

    QString toRichText(bool colored = true,
                       bool bulletPoint = false) const noexcept;
  };

  // Constructors / Destructor

  /**
   * @brief (Default) construdtor creating a top-level logger
   */
  MessageLogger(bool record = true, QObject* parent = nullptr) noexcept;

  /**
   * @brief Constructor for a (conditionally) child logger
   */
  MessageLogger(MessageLogger* parentLogger, const QString& group = QString(),
                bool record = false, QObject* parent = nullptr) noexcept;

  /**
   * @brief Copy constructor
   *
   * @param other Object to copy.
   */
  MessageLogger(const MessageLogger& other) = delete;

  /**
   * @brief Destructor
   */
  virtual ~MessageLogger() noexcept;

  // Getters
  bool hasMessages() const noexcept;
  QList<Message> getMessages() const noexcept;
  QStringList getMessagesPlain() const noexcept;
  QString getMessagesRichText() const noexcept;

  // General Methods
  void clear() noexcept;
  void log(QtMsgType type, const QString& msg) noexcept;
  void debug(const QString& msg) noexcept;
  void info(const QString& msg) noexcept;
  void warning(const QString& msg) noexcept;
  void critical(const QString& msg) noexcept;

  // Operator Overloadings
  MessageLogger& operator=(const MessageLogger& rhs) = delete;

signals:
  void msgEmitted(const Message& msg);

private:  // Data
#if (QT_VERSION >= QT_VERSION_CHECK(5, 15, 0))
  mutable QRecursiveMutex mMutex;
#else
  mutable QMutex mMutex;
#endif
  QPointer<MessageLogger> mParent;
  QString mPrefix;
  bool mRecord;
  QList<Message> mMessages;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
