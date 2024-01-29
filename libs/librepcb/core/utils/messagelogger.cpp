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

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "messagelogger.h"

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class MessageLogger::Message
 ******************************************************************************/

QString MessageLogger::Message::toRichText(bool colored,
                                           bool bulletPoint) const noexcept {
  static const QHash<QtMsgType, QString> colorMap = {
      {QtDebugMsg, "blue"},
      {QtInfoMsg, "darkblue"},
      {QtWarningMsg, "orangered"},
      {QtCriticalMsg, "red"},
  };

  QString s;
  if (colored && colorMap.contains(type)) {
    s += QString("<font color=\"%1\">").arg(colorMap[type]);
  }
  if (bulletPoint) {
    s += "&#x2022; ";
  }
  s += QString(message).replace("\n", "<br>");
  if (colored && colorMap.contains(type)) {
    s += "</font>";
  }
  return s;
}

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

MessageLogger::MessageLogger(bool record, QObject* parent) noexcept
  : QObject(parent), mMutex(QMutex::Recursive), mParent(), mRecord(record) {
}

MessageLogger::MessageLogger(MessageLogger* parentLogger, const QString& group,
                             bool record, QObject* parent) noexcept
  : QObject(parent),
    mMutex(QMutex::Recursive),
    mParent(parentLogger),
    mRecord(record) {
  if (!group.isEmpty()) {
    mPrefix = "[" % group % "] ";
  }
}

MessageLogger::~MessageLogger() noexcept {
}

/*******************************************************************************
 *  Getters
 ******************************************************************************/

bool MessageLogger::hasMessages() const noexcept {
  QMutexLocker lock(&mMutex);
  return !mMessages.isEmpty();
}

QList<MessageLogger::Message> MessageLogger::getMessages() const noexcept {
  QMutexLocker lock(&mMutex);
  if (!mRecord) {
    qWarning() << "Attempted to retrieve messages from a logger which does not "
                  "record!";
  }
  return QList<MessageLogger::Message>(mMessages);
}

QStringList MessageLogger::getMessagesPlain() const noexcept {
  QStringList l;
  foreach (const Message& msg, getMessages()) {
    l.append(msg.message);
  }
  return l;
}

QString MessageLogger::getMessagesRichText() const noexcept {
  QStringList l;
  foreach (const Message& msg, getMessages()) {
    l.append(msg.toRichText());
  }
  return l.join("<br>");
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void MessageLogger::clear() noexcept {
  QMutexLocker lock(&mMutex);
  mMessages.clear();
}

void MessageLogger::log(QtMsgType type, const QString& msg) noexcept {
  QMutexLocker lock(&mMutex);
  if (mParent) {
    mParent->log(type, mPrefix % msg);
  } else {
    qt_message_output(type, QMessageLogContext(), msg);
  }
  const Message obj{type, msg};
  if (mRecord) {
    mMessages.append(obj);
  }
  emit msgEmitted(obj);
}

void MessageLogger::debug(const QString& msg) noexcept {
  log(QtDebugMsg, msg);
}

void MessageLogger::info(const QString& msg) noexcept {
  log(QtInfoMsg, msg);
}

void MessageLogger::warning(const QString& msg) noexcept {
  log(QtWarningMsg, msg);
}

void MessageLogger::critical(const QString& msg) noexcept {
  log(QtCriticalMsg, msg);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
