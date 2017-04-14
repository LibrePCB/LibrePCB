/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
 * http://librepcb.org/
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

#ifndef LIBREPCB_WSI_BASE_H
#define LIBREPCB_WSI_BASE_H

/*****************************************************************************************
 *  Includes
 ****************************************************************************************/
#include <QtCore>
#include <QtWidgets>
#include <librepcb/common/fileio/serializableobject.h>

/*****************************************************************************************
 *  Namespace / Forward Declarations
 ****************************************************************************************/
namespace librepcb {

class XmlDomElement;

namespace workspace {

/*****************************************************************************************
 *  Class WSI_Base
 ****************************************************************************************/

/**
 * @brief The WSI_Base class is the base class of all workspace settings items
 *
 * Every workspace setting is represented by a seperate object. All of these objects have
 * this class as base class. The name of all Workspace Settings Items begin with the
 * prefix "WSI_" to easily recognize them.
 *
 * @author ubruhin
 * @date 2014-10-04
 */
class WSI_Base : public QObject, public SerializableObject
{
        Q_OBJECT

    public:

        // Constructors / Destructor
        WSI_Base() = delete;
        WSI_Base(const WSI_Base& other) = delete;
        explicit WSI_Base(const QString& xmlTagName, XmlDomElement* xmlElement) throw (Exception);
        virtual ~WSI_Base() noexcept;

        // Getters
        const QString& getXmlElementTagName() const noexcept {return mXmlElementTagName;}

        // General Methods
        virtual void restoreDefault() noexcept = 0;
        virtual void apply() noexcept = 0;
        virtual void revert() noexcept = 0;


        // Operator Overloadings
        WSI_Base& operator=(const WSI_Base& rhs) = delete;


    private:

        // General Attributes
        QString mXmlElementTagName;
};

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace workspace
} // namespace librepcb

#endif // LIBREPCB_WSI_BASE_H
