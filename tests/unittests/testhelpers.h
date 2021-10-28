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

#ifndef TESTHELPERS_H
#define TESTHELPERS_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/common/exceptions.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace tests {

/*******************************************************************************
 *  TestHelpers Class
 ******************************************************************************/

/**
 * @brief The TestHelpers class provides some helper methods useful in tests
 */
class TestHelpers {
public:
  // Constructors / Destructor
  TestHelpers() = delete;
  TestHelpers(const TestHelpers& other) = delete;
  ~TestHelpers() = delete;

  // Operator Overloadings
  TestHelpers& operator=(const TestHelpers& rhs) = delete;

  // Static Methods

  /**
   * @brief Get a child object of a given parent object by path specification
   *
   * @tparam T      Type of child object to get.
   * @param parent  Parent object to search for the child.
   * @param path    Path of object names, e.g. "cbxLayer/QComboBox".
   * @return Reference to the child.
   * @throw Exception if the child was not found or could not be casted to
   *        the given type.
   */
  template <typename T>
  static T& getChild(QObject& parent, const QString& path) {
    QObject* qObject = &parent;
    foreach (const QString& name, path.split("/")) {
      QObject* child =
          qObject->findChild<QObject*>(name, Qt::FindDirectChildrenOnly);
      if (!child) {
        QStringList childrenNames;
        foreach (QObject* c, qObject->children()) {
          childrenNames.append(c->objectName());
        }
        throw LogicError(
            __FILE__, __LINE__,
            QString("Child object '%1' not found! Available children: %2")
                .arg(name)
                .arg(childrenNames.join(", ")));
      }
      qObject = child;
    }
    T* tObject = qobject_cast<T*>(qObject);
    if (!tObject) {
      throw LogicError(
          __FILE__, __LINE__,
          QString("Object '%1' does not have the expected type!").arg(path));
    }
    return *tObject;
  }

  /**
   * @brief Check if the tab order within a given widget is reasonable
   *
   * This method loops simulates tabulator key presses in the given widget and
   * records a complete tab order loop. Then a check validates if the tab order
   * is reasonable. A tab order is considered as reasonable if it starts at
   * the top left and goes to the right and then to the bottom. Any other
   * order will make this test fail.
   *
   * Note that this is a helper method which directly calls GTest macros to
   * verify the results. So this method must only be called from within unit
   * tests.
   *
   * @note  The tab order feature depends on the operating system resp. desktop
   *        environment on which the application is run. So far it turned out
   *        that on Windows and Linux this test works, while on macOS it does
   *        something different. To avoid test failures on specific
   *        environments, this function calls GTEST_SKIP() to skip the unit
   *        test on operating systems other than Windows and Linux. This is
   *        totally fine, since the test basically only verifies the configured
   *        tab order, which is independent of the runtime environment.
   *
   * @param widget    The parent widget to check the tab order of.
   */
  static void testTabOrder(QWidget& widget);
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace librepcb

#endif
