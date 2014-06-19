/*
 * EDA4U - Professional EDA for everyone!
 * Copyright (C) 2013 Urban Bruhin
 * http://eda4u.ubruhin.ch
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

/*****************************************************************************************
 *  Includes / Namespaces
 ****************************************************************************************/

#include <QtCore>
#include "classtemplate.h"

#include "../exceptions/exceptions.h"
#include "../project/project.h"

using namespace project;

namespace classtemplate{

/*****************************************************************************************
 *  Constructors / Destructor
 ****************************************************************************************/

ClassTemplate::ClassTemplate(Project* project) :
    mProject(project), mMember(0)
{
    // Qt Debug Messages:
    // - qDebug() << ...
    // - qWarning() << ...
    // - qCritical() << ...
    // - qFatal() << ...


    // Qt Asserts:
    // - Q_ASSERT ( bool test )
    // - Q_ASSERT_X ( bool test, const char * where, const char * what )


    // Others:
    // - Q_CHECK_PTR ( void * pointer )
}

ClassTemplate::~ClassTemplate()
{
}

/*****************************************************************************************
 *  General
 ****************************************************************************************/

const Project& ClassTemplate::doSomething(int param1, int* param2, const QString& param3)
{
    Q_UNUSED(param1);
    Q_UNUSED(param2);
    Q_UNUSED(param3);

    return *mProject;
}

/*****************************************************************************************
 *  End of File
 ****************************************************************************************/

} // namespace classtemplate
