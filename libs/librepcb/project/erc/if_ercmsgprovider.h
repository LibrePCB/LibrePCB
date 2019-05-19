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

#ifndef LIBREPCB_PROJECT_IF_ERCMSGPROVIDER_H
#define LIBREPCB_PROJECT_IF_ERCMSGPROVIDER_H

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace project {

class ErcMsg;  // all classes which implement IF_ErcMsgProvider will need this
               // declaration

/*******************************************************************************
 *  Macros
 ******************************************************************************/

/**
 * @note    The specified class name should be unique only in the namespace
 * #project, so we won't use the namespace as a prefix. Simple use the class
 * name.
 *
 * @warning Do not change the name of an existing class if you don't know what
 * you're doing!
 */
#define DECLARE_ERC_MSG_CLASS_NAME(msgOwnerClassName)                     \
public:                                                                   \
  virtual const char* getErcMsgOwnerClassName() const noexcept override { \
    return #msgOwnerClassName;                                            \
  }                                                                       \
                                                                          \
private:

/*******************************************************************************
 *  Class IF_ErcMsgProvider
 ******************************************************************************/

/**
 * @brief The IF_ErcMsgProvider class
 */
class IF_ErcMsgProvider {
public:
  // Constructors / Destructor
  IF_ErcMsgProvider() {}
  virtual ~IF_ErcMsgProvider() {}

  // Getters
  virtual const char* getErcMsgOwnerClassName() const noexcept = 0;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace project
}  // namespace librepcb

#endif  // LIBREPCB_PROJECT_IF_ERCMSGPROVIDER_H
