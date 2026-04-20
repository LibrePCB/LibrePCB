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
#include "testhelpers.h"

#include <gtest/gtest.h>
#include <librepcb/core/fileio/fileutils.h>
#include <librepcb/core/utils/toolbox.h>

#include <QtCore>
#include <QtTest>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace tests {

/*******************************************************************************
 *  Static Methods
 ******************************************************************************/

std::function<Uuid()> TestHelpers::createDeterministicUuidFactory() noexcept {
  auto counter = std::make_shared<quint64>(0);
  return [counter]() -> Uuid {
    ++(*counter);
    return Uuid::fromString(QString("00000000-0000-4000-8000-%1")
                                .arg(*counter, 12, 16, QLatin1Char('0')));
  };
}

QDateTime TestHelpers::createDeterministicDateTime() noexcept {
  return QDateTime::fromString("2000-01-02T03:04:05Z", Qt::ISODate);
}

void TestHelpers::compareDirectories(const FilePath& actual,
                                     const FilePath& expected) {
  const QList<FilePath> actualFiles =
      FileUtils::getFilesInDirectory(actual, {}, true, false);
  const QList<FilePath> expectedFiles =
      FileUtils::getFilesInDirectory(expected, {}, true, false);

  // Build relative-path sets for structural comparison.
  QSet<QString> actualRels, expectedRels;
  for (const FilePath& fp : actualFiles) {
    actualRels.insert(fp.toRelative(actual));
  }
  for (const FilePath& fp : expectedFiles) {
    expectedRels.insert(fp.toRelative(expected));
  }

  // Check for files missing in actual output.
  for (const QString& rel : (expectedRels - actualRels)) {
    ADD_FAILURE() << "Missing file in output: " << rel.toStdString();
  }

  // Check for unexpected files in actual output.
  for (const QString& rel : (actualRels - expectedRels)) {
    ADD_FAILURE() << "Unexpected file in output: " << rel.toStdString();
  }

  // Compare contents of files present in both, one at a time.
  for (const QString& rel : (actualRels & expectedRels)) {
    EXPECT_EQ(FileUtils::readFile(expected.getPathTo(rel)).toStdString(),
              FileUtils::readFile(actual.getPathTo(rel)).toStdString())
        << "File content differs: " << rel.toStdString();
  }
}

void TestHelpers::testTabOrder(QWidget& widget) {
  // Skip test on systems other than Linux and Windows (details in function
  // documentation).
#if (!defined(Q_OS_LINUX)) && (!defined(Q_OS_WIN32)) && (!defined(Q_OS_WIN64))
  GTEST_SKIP();
#endif

  // Show and enable all child widgets to avoid skipping them in the tab order.
  foreach (QWidget* child, widget.findChildren<QWidget*>()) {
    child->setVisible(true);
    child->setEnabled(true);
  }

  // Show the whole widget, otherwise we don't get the positions of the child
  // widgets.
  widget.show();

  // Tab through all widgets and memorize their order.
  QVector<QWidget*> tabOrderWidgets;
  while (true) {
    QTest::keyClick(&widget, Qt::Key_Tab);
    QWidget* focusWidget = widget.focusWidget();
    ASSERT_NE(nullptr, focusWidget);
    if (tabOrderWidgets.contains(focusWidget)) {
      break;  // Back to the first widget, tab loop is closed.
    }
    tabOrderWidgets.append(focusWidget);
  }

  // Sanity check if the detection above works. We assume that the tab loop
  // contains at least 2 widgets.
  EXPECT_GE(tabOrderWidgets.count(), 2);

  // Helper function to get the relative position of a widget.
  auto absPos = [&widget](const QWidget* w) {
    return w->mapTo(&widget, QPoint(0, 0));
  };

  // Helper function to pretty print the tab order.
  auto tabOrderToString = [absPos](QVector<QWidget*>& widgets) {
    QStringList list;
    foreach (auto w, widgets) {
      QPoint p = absPos(w);
      QString name = w->objectName();
      if (name.isEmpty()) {
        name = w->metaObject()->className();
      }
      list.append(QString("%1[%2;%3]").arg(name).arg(p.x()).arg(p.y()));
    }
    // Sanity check that all items are unique - otherwise the test makes not
    // much sense since the final check only compares these strings.
    EXPECT_EQ(list.count(), Toolbox::toSet(list).count());
    return list.join(" -> ").toStdString();
  };

  // Determine sane tab order according widget positions.
  QVector<QWidget*> expectedTabOrderWidgets = tabOrderWidgets;
  std::sort(expectedTabOrderWidgets.begin(), expectedTabOrderWidgets.end(),
            [absPos](const QWidget* a, const QWidget* b) {
              QPoint pA = absPos(a);
              QPoint pB = absPos(b);
              return (pA.y() < pB.y()) ||
                  ((pA.y() == pB.y()) && (pA.x() < pB.x()));
            });

  // Check tab order.
  EXPECT_EQ(tabOrderToString(expectedTabOrderWidgets),
            tabOrderToString(tabOrderWidgets));
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace librepcb
