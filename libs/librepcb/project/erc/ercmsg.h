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

#ifndef LIBREPCB_PROJECT_ERCMSG_H
#define LIBREPCB_PROJECT_ERCMSG_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace project {

class IF_ErcMsgProvider;
class ErcMsgList;
class Project;

/*******************************************************************************
 *  Class ErcMsg
 ******************************************************************************/

/**
 * @brief The ErcMsg class represents a message in the ERC (Electrical Rule
 * Check) list
 */
class ErcMsg {
public:
  /// ERC message types
  enum class ErcMsgType_t {
    CircuitError = 0,  ///< example: two output pins in the same net
    CircuitWarning,    ///< example: nets with only one pin
    SchematicError,    ///< example: unplaced required symbols
    SchematicWarning,  ///< example: unplaced optional symbols
    BoardError,        ///< example: unplaced footprints
    BoardWarning,      ///< example: ???
    _Count             ///< count of message types
  };

  // Constructors / Destructor
  explicit ErcMsg(Project& project, const IF_ErcMsgProvider& owner,
                  const QString& ownerKey, const QString& msgKey,
                  ErcMsg::ErcMsgType_t msgType, const QString& msg = QString());
  virtual ~ErcMsg() noexcept;

  // Getters
  const IF_ErcMsgProvider& getOwner() const noexcept { return mOwner; }
  const QString&           getOwnerKey() const noexcept { return mOwnerKey; }
  const QString&           getMsgKey() const noexcept { return mMsgKey; }
  ErcMsgType_t             getMsgType() const noexcept { return mMsgType; }
  const QString&           getMsg() const noexcept { return mMsg; }
  bool                     isVisible() const noexcept { return mIsVisible; }
  bool                     isIgnored() const noexcept { return mIsIgnored; }

  // Setters
  void setMsg(const QString& msg) noexcept;
  void setVisible(bool visible) noexcept;
  void setIgnored(bool ignored) noexcept;

private:
  // make some methods inaccessible...
  ErcMsg();
  ErcMsg(const ErcMsg& other);
  ErcMsg& operator=(const ErcMsg& rhs);

  // General
  Project&    mProject;
  ErcMsgList& mErcMsgList;

  // Attributes
  const IF_ErcMsgProvider& mOwner;
  QString                  mOwnerKey;
  QString                  mMsgKey;
  ErcMsgType_t             mMsgType;
  QString                  mMsg;

  // Misc
  bool mIsVisible;
  bool mIsIgnored;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace project
}  // namespace librepcb

#endif  // LIBREPCB_PROJECT_ERCMSG_H
