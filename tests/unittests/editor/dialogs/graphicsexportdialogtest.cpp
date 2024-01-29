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

#include "../../core/export/graphicsexporttest.h"
#include "../../testhelpers.h"

#include <gtest/gtest.h>
#include <librepcb/core/export/graphicsexport.h>
#include <librepcb/core/export/graphicsexportsettings.h>
#include <librepcb/core/fileio/filepath.h>
#include <librepcb/core/fileio/fileutils.h>
#include <librepcb/core/types/lengthunit.h>
#include <librepcb/core/workspace/theme.h>
#include <librepcb/editor/dialogs/graphicsexportdialog.h>
#include <librepcb/editor/widgets/unsignedlengthedit.h>
#include <librepcb/editor/widgets/unsignedratioedit.h>

#include <QtTest>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {
namespace tests {

using librepcb::tests::GraphicsPagePainterMock;
using librepcb::tests::TestHelpers;

/*******************************************************************************
 *  Test Class
 ******************************************************************************/

class GraphicsExportDialogTest : public ::testing::Test {
protected:
  FilePath mOutputDir;
  QVector<FilePath> mRequestedFilesToOpen;  // From signal requestOpenFile().

  GraphicsExportDialogTest() : mOutputDir(FilePath::getRandomTempPath()) {
    QSettings().clear();
  }

  ~GraphicsExportDialogTest() { QDir(mOutputDir.toStr()).removeRecursively(); }

  FilePath getFilePath(const QString& fileName) const {
    return mOutputDir.getPathTo(fileName);
  }

  QList<std::shared_ptr<GraphicsPagePainter>> getPages(int count) const {
    QList<std::shared_ptr<GraphicsPagePainter>> pages;
    for (int i = 0; i < count; ++i) {
      pages.append(std::make_shared<GraphicsPagePainterMock>());
    }
    return pages;
  }

  void prepareDialog(GraphicsExportDialog& dlg, const FilePath& fp) {
    dlg.setSaveAsCallback([fp](QWidget*, const QString&, const QString&,
                               const QString&, QString*,
                               QFileDialog::Options) { return fp.toNative(); });
    QObject::connect(
        &dlg, &GraphicsExportDialog::requestOpenFile,
        [this](const FilePath& fp) { mRequestedFilesToOpen.append(fp); });
    dlg.show();
  }

  void enablePinNumbers(GraphicsExportDialog& dlg) {
    TestHelpers::getChild<QCheckBox>(
        dlg,
        "tabWidget/qt_tabwidget_stackedwidget/tabAdvanced/cbxShowPinNumbers")
        .setChecked(true);
  }

  QList<std::shared_ptr<GraphicsExportSettings>> getSettings(
      const GraphicsExportDialog& dlg, int expectedCount = -1) {
    QList<std::shared_ptr<GraphicsExportSettings>> settings;
    foreach (const auto& pair, dlg.getPages()) {
      settings.append(pair.second);
    }
    if ((expectedCount >= 0) && (settings.count() != expectedCount)) {
      throw Exception(__FILE__, __LINE__,
                      QString("Expected %1 pages, but got %2 pages.")
                          .arg(expectedCount)
                          .arg(settings.count()));
    }
    return settings;
  }

  void performExport(GraphicsExportDialog& dlg, int timeoutMs = 10000) {
    // Clear any results.
    mRequestedFilesToOpen.clear();

    // Start the export.
    QTest::keyClick(&dlg, Qt::Key_Enter);

    // Wait until the dialog is hidden, which means the export has finished.
    EXPECT_TRUE(
        TestHelpers::waitFor([&]() { return !dlg.isVisible(); }, timeoutMs));

    // Make dialog ready again for further tests.
    dlg.show();
  }

  void performCopyToClipboard(GraphicsExportDialog& dlg,
                              int timeoutMs = 10000) {
    // Clear any results.
    mRequestedFilesToOpen.clear();
    qApp->clipboard()->clear();

    // Start the export.
    QAbstractButton& btn = TestHelpers::getChild<QAbstractButton>(
        dlg, "buttonBox/btnCopyToClipboard");
    btn.click();

    // Wait until the dialog is hidden, which means the export has finished.
    EXPECT_TRUE(
        TestHelpers::waitFor([&]() { return !dlg.isVisible(); }, timeoutMs));

    // Make dialog ready again for further tests.
    dlg.show();
  }

  void restoreDefaults(GraphicsExportDialog& dlg) {
    QDialogButtonBox& btnBox =
        TestHelpers::getChild<QDialogButtonBox>(dlg, "buttonBoxLeft");
    QPushButton* btn = btnBox.button(QDialogButtonBox::RestoreDefaults);
    Q_ASSERT(btn);
    btn->click();
  }

  std::string str(const QList<std::pair<QString, QColor>>& colors) {
    QString s;
    for (int i = 0; i < colors.count(); ++i) {
      s += QString("[%1] %2: %3\n")
               .arg(i)
               .arg(colors.at(i).first,
                    colors.at(i).second.name(QColor::HexArgb));
    }
    return s.toStdString();
  }
};

/*******************************************************************************
 *  Test Methods
 ******************************************************************************/

TEST_F(GraphicsExportDialogTest, testDefaultTab) {
  Theme theme;
  GraphicsExportDialog dlg(GraphicsExportDialog::Mode::Schematic,
                           GraphicsExportDialog::Output::Pdf, getPages(0), 0,
                           "test", 0, FilePath(), LengthUnit::millimeters(),
                           theme, "unittest");
  prepareDialog(dlg, FilePath());
  QTabWidget& tabWidget = TestHelpers::getChild<QTabWidget>(dlg, "tabWidget");
  EXPECT_EQ(0, tabWidget.currentIndex());
}

TEST_F(GraphicsExportDialogTest, testExportSchematicEmptyPages) {
  FilePath fp = getFilePath("out.pdf");
  Theme theme;
  GraphicsExportDialog dlg(GraphicsExportDialog::Mode::Schematic,
                           GraphicsExportDialog::Output::Pdf, getPages(0), 0,
                           "test", 0, FilePath(), LengthUnit::millimeters(),
                           theme, "unittest");
  prepareDialog(dlg, fp);
}

TEST_F(GraphicsExportDialogTest, testExportSchematicPdf) {
  FilePath outFile = getFilePath("out.pdf");
  Theme theme;
  GraphicsExportDialog dlg(GraphicsExportDialog::Mode::Schematic,
                           GraphicsExportDialog::Output::Pdf, getPages(3), 0,
                           "test", 0, FilePath(), LengthUnit::millimeters(),
                           theme, "unittest");
  prepareDialog(dlg, outFile);
  performExport(dlg);
  EXPECT_EQ(QVector<FilePath>{outFile}, mRequestedFilesToOpen);
  EXPECT_TRUE(outFile.isExistingFile());
}

TEST_F(GraphicsExportDialogTest, testExportSchematicImage) {
  Theme theme;
  GraphicsExportDialog dlg(GraphicsExportDialog::Mode::Schematic,
                           GraphicsExportDialog::Output::Image, getPages(3), 0,
                           "test", 0, FilePath(), LengthUnit::millimeters(),
                           theme, "unittest");
  prepareDialog(dlg, getFilePath("out.svg"));
  performExport(dlg);
  EXPECT_EQ(QVector<FilePath>{mOutputDir}, mRequestedFilesToOpen);
  EXPECT_TRUE(getFilePath("out1.svg").isExistingFile());
  EXPECT_TRUE(getFilePath("out2.svg").isExistingFile());
  EXPECT_TRUE(getFilePath("out3.svg").isExistingFile());
}

TEST_F(GraphicsExportDialogTest, testExportBoardEmptyPages) {
  FilePath fp = getFilePath("out.pdf");
  Theme theme;
  GraphicsExportDialog dlg(GraphicsExportDialog::Mode::Board,
                           GraphicsExportDialog::Output::Pdf, getPages(0), 0,
                           "test", 0, FilePath(), LengthUnit::millimeters(),
                           theme, "unittest");
  prepareDialog(dlg, fp);
}

TEST_F(GraphicsExportDialogTest, testExportBoardPdf) {
  FilePath outFile = getFilePath("out.pdf");
  Theme theme;
  GraphicsExportDialog dlg(GraphicsExportDialog::Mode::Board,
                           GraphicsExportDialog::Output::Pdf, getPages(1), 0,
                           "test", 0, FilePath(), LengthUnit::millimeters(),
                           theme, "unittest");
  prepareDialog(dlg, outFile);
  performExport(dlg);
  EXPECT_EQ(QVector<FilePath>{outFile}, mRequestedFilesToOpen);
  EXPECT_TRUE(outFile.isExistingFile());
}

TEST_F(GraphicsExportDialogTest, testExportBoardImage) {
  FilePath outFile = getFilePath("out.svg");
  Theme theme;
  GraphicsExportDialog dlg(GraphicsExportDialog::Mode::Board,
                           GraphicsExportDialog::Output::Image, getPages(1), 0,
                           "test", 0, FilePath(), LengthUnit::millimeters(),
                           theme, "unittest");
  prepareDialog(dlg, outFile);
  performExport(dlg);
  EXPECT_EQ(QVector<FilePath>{mOutputDir}, mRequestedFilesToOpen);
  EXPECT_TRUE(outFile.isExistingFile());  // All layers
}

TEST_F(GraphicsExportDialogTest, testExportManyPages) {
  FilePath outFile = getFilePath("out.pdf");
  Theme theme;
  GraphicsExportDialog dlg(GraphicsExportDialog::Mode::Schematic,
                           GraphicsExportDialog::Output::Pdf, getPages(1000), 0,
                           "test", 0, FilePath(), LengthUnit::millimeters(),
                           theme, "unittest");
  prepareDialog(dlg, outFile);
  performExport(dlg);
  EXPECT_EQ(QVector<FilePath>{outFile}, mRequestedFilesToOpen);
  EXPECT_TRUE(outFile.isExistingFile());
}

TEST_F(GraphicsExportDialogTest, testCopyToClipboard) {
  Theme theme;
  GraphicsExportDialog dlg(GraphicsExportDialog::Mode::Schematic,
                           GraphicsExportDialog::Output::Image, getPages(1), 0,
                           "test", 0, FilePath(), LengthUnit::millimeters(),
                           theme, "unittest");
  prepareDialog(dlg, FilePath());
  performCopyToClipboard(dlg);
  EXPECT_EQ(QVector<FilePath>{}, mRequestedFilesToOpen);
  EXPECT_FALSE(qApp->clipboard()->image().isNull());
}

// Find potential multithreading issues by exporting many times.
TEST_F(GraphicsExportDialogTest, testExportPdfManyTimes) {
  for (int i = 0; i < 50; ++i) {
    FilePath outFile = mOutputDir.getPathTo(QString::number(i) % ".pdf");
    Theme theme;
    GraphicsExportDialog dlg(GraphicsExportDialog::Mode::Schematic,
                             GraphicsExportDialog::Output::Pdf, getPages(5), 0,
                             "test", 0, FilePath(), LengthUnit::millimeters(),
                             theme, "unittest");
    prepareDialog(dlg, outFile);
    performExport(dlg);
    EXPECT_EQ(QVector<FilePath>{outFile}, mRequestedFilesToOpen);
    EXPECT_TRUE(outFile.isExistingFile());
  }
}

TEST_F(GraphicsExportDialogTest, testPageSize) {
  const QPageSize defaultValue(QPageSize::A4);
  const QPageSize newValue(QPageSize::Letter);
  QString widget =
      "tabWidget/qt_tabwidget_stackedwidget/tabGeneral/cbxPageSize";
  FilePath outFile = getFilePath("out.pdf");

  {
    Theme theme;
    GraphicsExportDialog dlg(GraphicsExportDialog::Mode::Schematic,
                             GraphicsExportDialog::Output::Pdf, getPages(1), 0,
                             "test", 0, FilePath(), LengthUnit::millimeters(),
                             theme, "unittest");
    prepareDialog(dlg, outFile);
    QComboBox& cbx = TestHelpers::getChild<QComboBox>(dlg, widget);

    // Check the default value.
    EXPECT_EQ(defaultValue.name().toStdString(),
              cbx.currentText().toStdString());
    EXPECT_EQ(defaultValue, getSettings(dlg, 1).at(0)->getPageSize());

    // Check if the value can be changed and are applied properly.
    cbx.setCurrentText(newValue.name());
    EXPECT_EQ(newValue, getSettings(dlg, 1).at(0)->getPageSize());
  }

  // Check if the setting is saved and restored automatically, and can be
  // reset to its default value.
  {
    Theme theme;
    GraphicsExportDialog dlg(GraphicsExportDialog::Mode::Schematic,
                             GraphicsExportDialog::Output::Pdf, getPages(1), 0,
                             "test", 0, FilePath(), LengthUnit::millimeters(),
                             theme, "unittest");
    prepareDialog(dlg, outFile);
    QComboBox& cbx = TestHelpers::getChild<QComboBox>(dlg, widget);

    // Check new value.
    EXPECT_EQ(newValue.name().toStdString(), cbx.currentText().toStdString());
    EXPECT_EQ(newValue, getSettings(dlg, 1).at(0)->getPageSize());

    // Restore default value.
    restoreDefaults(dlg);
    EXPECT_EQ(defaultValue.name().toStdString(),
              cbx.currentText().toStdString());
    EXPECT_EQ(defaultValue, getSettings(dlg, 1).at(0)->getPageSize());

    // Sanity check that the export is actually successful.
    performExport(dlg);
    EXPECT_TRUE(outFile.isExistingFile());
  }
}

TEST_F(GraphicsExportDialogTest, testOrientation) {
  const GraphicsExportSettings::Orientation defaultValue =
      GraphicsExportSettings::Orientation::Auto;
  const GraphicsExportSettings::Orientation newValue =
      GraphicsExportSettings::Orientation::Portrait;
  QString defaultWidget =
      "tabWidget/qt_tabwidget_stackedwidget/tabGeneral/rbtnOrientationAuto";
  QString newWidget =
      "tabWidget/qt_tabwidget_stackedwidget/tabGeneral/rbtnOrientationPortrait";
  FilePath outFile = getFilePath("out.pdf");

  {
    Theme theme;
    GraphicsExportDialog dlg(GraphicsExportDialog::Mode::Schematic,
                             GraphicsExportDialog::Output::Pdf, getPages(1), 0,
                             "test", 0, FilePath(), LengthUnit::millimeters(),
                             theme, "unittest");
    prepareDialog(dlg, outFile);
    QRadioButton& rbtnDefault =
        TestHelpers::getChild<QRadioButton>(dlg, defaultWidget);
    QRadioButton& rbtnNew = TestHelpers::getChild<QRadioButton>(dlg, newWidget);

    // Check the default value.
    EXPECT_TRUE(rbtnDefault.isChecked());
    EXPECT_FALSE(rbtnNew.isChecked());
    EXPECT_EQ(defaultValue, getSettings(dlg, 1).at(0)->getOrientation());

    // Check if the value can be changed and are applied properly.
    rbtnNew.click();
    EXPECT_EQ(newValue, getSettings(dlg, 1).at(0)->getOrientation());
  }

  // Check if the setting is saved and restored automatically, and can be
  // reset to its default value.
  {
    Theme theme;
    GraphicsExportDialog dlg(GraphicsExportDialog::Mode::Schematic,
                             GraphicsExportDialog::Output::Pdf, getPages(1), 0,
                             "test", 0, FilePath(), LengthUnit::millimeters(),
                             theme, "unittest");
    prepareDialog(dlg, outFile);
    QRadioButton& rbtnDefault =
        TestHelpers::getChild<QRadioButton>(dlg, defaultWidget);
    QRadioButton& rbtnNew = TestHelpers::getChild<QRadioButton>(dlg, newWidget);

    // Check new value.
    EXPECT_FALSE(rbtnDefault.isChecked());
    EXPECT_TRUE(rbtnNew.isChecked());
    EXPECT_EQ(newValue, getSettings(dlg, 1).at(0)->getOrientation());

    // Restore default value.
    restoreDefaults(dlg);
    EXPECT_TRUE(rbtnDefault.isChecked());
    EXPECT_FALSE(rbtnNew.isChecked());
    EXPECT_EQ(defaultValue, getSettings(dlg, 1).at(0)->getOrientation());

    // Sanity check that the export is actually successful.
    performExport(dlg);
    EXPECT_TRUE(outFile.isExistingFile());
  }
}

TEST_F(GraphicsExportDialogTest, testMargins) {
  const UnsignedLength defaultValue(10000000);
  QString widgetLeft =
      "tabWidget/qt_tabwidget_stackedwidget/tabGeneral/edtMarginLeft";
  QString widgetRight =
      "tabWidget/qt_tabwidget_stackedwidget/tabGeneral/edtMarginRight";
  QString widgetTop =
      "tabWidget/qt_tabwidget_stackedwidget/tabGeneral/edtMarginTop";
  QString widgetBottom =
      "tabWidget/qt_tabwidget_stackedwidget/tabGeneral/edtMarginBottom";
  FilePath outFile = getFilePath("out.pdf");

  {
    Theme theme;
    GraphicsExportDialog dlg(GraphicsExportDialog::Mode::Schematic,
                             GraphicsExportDialog::Output::Pdf, getPages(1), 0,
                             "test", 0, FilePath(), LengthUnit::millimeters(),
                             theme, "unittest");
    prepareDialog(dlg, outFile);
    UnsignedLengthEdit& edtLeft =
        TestHelpers::getChild<UnsignedLengthEdit>(dlg, widgetLeft);
    UnsignedLengthEdit& edtRight =
        TestHelpers::getChild<UnsignedLengthEdit>(dlg, widgetRight);
    UnsignedLengthEdit& edtTop =
        TestHelpers::getChild<UnsignedLengthEdit>(dlg, widgetTop);
    UnsignedLengthEdit& edtBottom =
        TestHelpers::getChild<UnsignedLengthEdit>(dlg, widgetBottom);

    // Check the default value.
    EXPECT_EQ(defaultValue, edtLeft.getValue());
    EXPECT_EQ(defaultValue, edtRight.getValue());
    EXPECT_EQ(defaultValue, edtTop.getValue());
    EXPECT_EQ(defaultValue, edtBottom.getValue());
    EXPECT_EQ(defaultValue, getSettings(dlg, 1).at(0)->getMarginLeft());
    EXPECT_EQ(defaultValue, getSettings(dlg, 1).at(0)->getMarginRight());
    EXPECT_EQ(defaultValue, getSettings(dlg, 1).at(0)->getMarginTop());
    EXPECT_EQ(defaultValue, getSettings(dlg, 1).at(0)->getMarginBottom());

    // Check if the value can be changed and are applied properly.
    edtLeft.setValue(UnsignedLength(1));
    EXPECT_EQ(UnsignedLength(1), getSettings(dlg, 1).at(0)->getMarginLeft());
    edtRight.setValue(UnsignedLength(2));
    EXPECT_EQ(UnsignedLength(2), getSettings(dlg, 1).at(0)->getMarginRight());
    edtTop.setValue(UnsignedLength(3));
    EXPECT_EQ(UnsignedLength(3), getSettings(dlg, 1).at(0)->getMarginTop());
    edtBottom.setValue(UnsignedLength(4));
    EXPECT_EQ(UnsignedLength(4), getSettings(dlg, 1).at(0)->getMarginBottom());
  }

  // Check if the setting is saved and restored automatically, and can be
  // reset to its default value.
  {
    Theme theme;
    GraphicsExportDialog dlg(GraphicsExportDialog::Mode::Schematic,
                             GraphicsExportDialog::Output::Pdf, getPages(1), 0,
                             "test", 0, FilePath(), LengthUnit::millimeters(),
                             theme, "unittest");
    prepareDialog(dlg, outFile);
    UnsignedLengthEdit& edtLeft =
        TestHelpers::getChild<UnsignedLengthEdit>(dlg, widgetLeft);
    UnsignedLengthEdit& edtRight =
        TestHelpers::getChild<UnsignedLengthEdit>(dlg, widgetRight);
    UnsignedLengthEdit& edtTop =
        TestHelpers::getChild<UnsignedLengthEdit>(dlg, widgetTop);
    UnsignedLengthEdit& edtBottom =
        TestHelpers::getChild<UnsignedLengthEdit>(dlg, widgetBottom);

    // Check new value.
    EXPECT_EQ(UnsignedLength(1), edtLeft.getValue());
    EXPECT_EQ(UnsignedLength(2), edtRight.getValue());
    EXPECT_EQ(UnsignedLength(3), edtTop.getValue());
    EXPECT_EQ(UnsignedLength(4), edtBottom.getValue());
    EXPECT_EQ(UnsignedLength(1), getSettings(dlg, 1).at(0)->getMarginLeft());
    EXPECT_EQ(UnsignedLength(2), getSettings(dlg, 1).at(0)->getMarginRight());
    EXPECT_EQ(UnsignedLength(3), getSettings(dlg, 1).at(0)->getMarginTop());
    EXPECT_EQ(UnsignedLength(4), getSettings(dlg, 1).at(0)->getMarginBottom());

    // Restore default value.
    restoreDefaults(dlg);
    EXPECT_EQ(defaultValue, edtLeft.getValue());
    EXPECT_EQ(defaultValue, edtRight.getValue());
    EXPECT_EQ(defaultValue, edtTop.getValue());
    EXPECT_EQ(defaultValue, edtBottom.getValue());
    EXPECT_EQ(defaultValue, getSettings(dlg, 1).at(0)->getMarginLeft());
    EXPECT_EQ(defaultValue, getSettings(dlg, 1).at(0)->getMarginRight());
    EXPECT_EQ(defaultValue, getSettings(dlg, 1).at(0)->getMarginTop());
    EXPECT_EQ(defaultValue, getSettings(dlg, 1).at(0)->getMarginBottom());

    // Sanity check that the export is actually successful.
    performExport(dlg);
    EXPECT_TRUE(outFile.isExistingFile());
  }
}

TEST_F(GraphicsExportDialogTest, testShowPinNumbers) {
  const bool defaultValue = false;
  const bool newValue = true;
  const QString widget =
      "tabWidget/qt_tabwidget_stackedwidget/tabAdvanced/cbxShowPinNumbers";
  const FilePath outFile = getFilePath("out.pdf");

  {
    Theme theme;
    GraphicsExportDialog dlg(GraphicsExportDialog::Mode::Schematic,
                             GraphicsExportDialog::Output::Pdf, getPages(1), 0,
                             "test", 0, FilePath(), LengthUnit::millimeters(),
                             theme, "unittest");
    prepareDialog(dlg, outFile);
    QCheckBox& cbx = TestHelpers::getChild<QCheckBox>(dlg, widget);

    // Check the default value.
    EXPECT_EQ(defaultValue, cbx.isChecked());
    EXPECT_EQ(defaultValue,
              getSettings(dlg, 1)
                  .at(0)
                  ->getColor(Theme::Color::sSchematicPinNumbers)
                  .isValid());

    // Check if the value can be changed and are applied properly.
    cbx.setChecked(newValue);
    EXPECT_EQ(newValue,
              getSettings(dlg, 1)
                  .at(0)
                  ->getColor(Theme::Color::sSchematicPinNumbers)
                  .isValid());
  }

  // Check if the setting is saved and restored automatically, and can be
  // reset to its default value.
  {
    Theme theme;
    GraphicsExportDialog dlg(GraphicsExportDialog::Mode::Schematic,
                             GraphicsExportDialog::Output::Pdf, getPages(1), 0,
                             "test", 0, FilePath(), LengthUnit::millimeters(),
                             theme, "unittest");
    prepareDialog(dlg, outFile);
    QCheckBox& cbx = TestHelpers::getChild<QCheckBox>(dlg, widget);

    // Check new value.
    EXPECT_EQ(newValue, cbx.isChecked());
    EXPECT_EQ(newValue,
              getSettings(dlg, 1)
                  .at(0)
                  ->getColor(Theme::Color::sSchematicPinNumbers)
                  .isValid());

    // Restore default value.
    restoreDefaults(dlg);
    EXPECT_EQ(defaultValue, cbx.isChecked());
    EXPECT_EQ(defaultValue,
              getSettings(dlg, 1)
                  .at(0)
                  ->getColor(Theme::Color::sSchematicPinNumbers)
                  .isValid());

    // Sanity check that the export is actually successful.
    performExport(dlg);
    EXPECT_TRUE(outFile.isExistingFile());
  }
}

TEST_F(GraphicsExportDialogTest, testRotate) {
  const bool defaultValue = false;
  const bool newValue = true;
  QString widget = "tabWidget/qt_tabwidget_stackedwidget/tabAdvanced/cbxRotate";
  FilePath outFile = getFilePath("out.pdf");

  {
    Theme theme;
    GraphicsExportDialog dlg(GraphicsExportDialog::Mode::Schematic,
                             GraphicsExportDialog::Output::Pdf, getPages(1), 0,
                             "test", 0, FilePath(), LengthUnit::millimeters(),
                             theme, "unittest");
    prepareDialog(dlg, outFile);
    QCheckBox& cbx = TestHelpers::getChild<QCheckBox>(dlg, widget);

    // Check the default value.
    EXPECT_EQ(defaultValue, cbx.isChecked());
    EXPECT_EQ(defaultValue, getSettings(dlg, 1).at(0)->getRotate());

    // Check if the value can be changed and are applied properly.
    cbx.setChecked(newValue);
    EXPECT_EQ(newValue, getSettings(dlg, 1).at(0)->getRotate());
  }

  // Check if the setting is saved and restored automatically, and can be
  // reset to its default value.
  {
    Theme theme;
    GraphicsExportDialog dlg(GraphicsExportDialog::Mode::Schematic,
                             GraphicsExportDialog::Output::Pdf, getPages(1), 0,
                             "test", 0, FilePath(), LengthUnit::millimeters(),
                             theme, "unittest");
    prepareDialog(dlg, outFile);
    QCheckBox& cbx = TestHelpers::getChild<QCheckBox>(dlg, widget);

    // Check new value.
    EXPECT_EQ(newValue, cbx.isChecked());
    EXPECT_EQ(newValue, getSettings(dlg, 1).at(0)->getRotate());

    // Restore default value.
    restoreDefaults(dlg);
    EXPECT_EQ(defaultValue, cbx.isChecked());
    EXPECT_EQ(defaultValue, getSettings(dlg, 1).at(0)->getRotate());

    // Sanity check that the export is actually successful.
    performExport(dlg);
    EXPECT_TRUE(outFile.isExistingFile());
  }
}

TEST_F(GraphicsExportDialogTest, testMirror) {
  const bool defaultValue = false;
  const bool newValue = true;
  QString widget = "tabWidget/qt_tabwidget_stackedwidget/tabAdvanced/cbxMirror";
  FilePath outFile = getFilePath("out.pdf");

  {
    Theme theme;
    GraphicsExportDialog dlg(GraphicsExportDialog::Mode::Schematic,
                             GraphicsExportDialog::Output::Pdf, getPages(1), 0,
                             "test", 0, FilePath(), LengthUnit::millimeters(),
                             theme, "unittest");
    prepareDialog(dlg, outFile);
    QCheckBox& cbx = TestHelpers::getChild<QCheckBox>(dlg, widget);

    // Check the default value.
    EXPECT_EQ(defaultValue, cbx.isChecked());
    EXPECT_EQ(defaultValue, getSettings(dlg, 1).at(0)->getMirror());

    // Check if the value can be changed and are applied properly.
    cbx.setChecked(newValue);
    EXPECT_EQ(newValue, getSettings(dlg, 1).at(0)->getMirror());
  }

  // Check if the setting is saved and restored automatically, and can be
  // reset to its default value.
  {
    Theme theme;
    GraphicsExportDialog dlg(GraphicsExportDialog::Mode::Schematic,
                             GraphicsExportDialog::Output::Pdf, getPages(1), 0,
                             "test", 0, FilePath(), LengthUnit::millimeters(),
                             theme, "unittest");
    prepareDialog(dlg, outFile);
    QCheckBox& cbx = TestHelpers::getChild<QCheckBox>(dlg, widget);

    // Check new value.
    EXPECT_EQ(newValue, cbx.isChecked());
    EXPECT_EQ(newValue, getSettings(dlg, 1).at(0)->getMirror());

    // Restore default value.
    restoreDefaults(dlg);
    EXPECT_EQ(defaultValue, cbx.isChecked());
    EXPECT_EQ(defaultValue, getSettings(dlg, 1).at(0)->getMirror());

    // Sanity check that the export is actually successful.
    performExport(dlg);
    EXPECT_TRUE(outFile.isExistingFile());
  }
}

TEST_F(GraphicsExportDialogTest, testScale) {
  const tl::optional<UnsignedRatio> defaultValue = tl::nullopt;
  const tl::optional<UnsignedRatio> newValue =
      UnsignedRatio(Ratio::fromNormalized(2));
  QString widgetCbx =
      "tabWidget/qt_tabwidget_stackedwidget/tabGeneral/cbxScaleAuto";
  QString widgetSpbx =
      "tabWidget/qt_tabwidget_stackedwidget/tabGeneral/spbxScaleFactor";
  FilePath outFile = getFilePath("out.pdf");

  {
    Theme theme;
    GraphicsExportDialog dlg(GraphicsExportDialog::Mode::Schematic,
                             GraphicsExportDialog::Output::Pdf, getPages(1), 0,
                             "test", 0, FilePath(), LengthUnit::millimeters(),
                             theme, "unittest");
    prepareDialog(dlg, outFile);
    QCheckBox& cbx = TestHelpers::getChild<QCheckBox>(dlg, widgetCbx);
    UnsignedRatioEdit& spbx =
        TestHelpers::getChild<UnsignedRatioEdit>(dlg, widgetSpbx);

    // Check the default value.
    EXPECT_EQ(!defaultValue, cbx.isChecked());
    EXPECT_EQ(defaultValue.has_value(), spbx.isEnabled());
    EXPECT_EQ(defaultValue ? **defaultValue : Ratio::fromPercent(100),
              *spbx.getValue());
    EXPECT_EQ(defaultValue, getSettings(dlg, 1).at(0)->getScale());

    // Check if the value can be changed and are applied properly.
    cbx.setChecked(!newValue);
    EXPECT_EQ(newValue.has_value(), spbx.isEnabled());
    EXPECT_EQ(UnsignedRatio(Ratio::fromPercent(100)),
              getSettings(dlg, 1).at(0)->getScale());
    spbx.setValue(*newValue);
    EXPECT_EQ(newValue, getSettings(dlg, 1).at(0)->getScale());
  }

  // Check if the setting is saved and restored automatically, and can be
  // reset to its default value.
  {
    Theme theme;
    GraphicsExportDialog dlg(GraphicsExportDialog::Mode::Schematic,
                             GraphicsExportDialog::Output::Pdf, getPages(1), 0,
                             "test", 0, FilePath(), LengthUnit::millimeters(),
                             theme, "unittest");
    prepareDialog(dlg, outFile);
    QCheckBox& cbx = TestHelpers::getChild<QCheckBox>(dlg, widgetCbx);
    UnsignedRatioEdit& spbx =
        TestHelpers::getChild<UnsignedRatioEdit>(dlg, widgetSpbx);

    // Check new value.
    EXPECT_EQ(!newValue, cbx.isChecked());
    EXPECT_EQ(newValue.has_value(), spbx.isEnabled());
    EXPECT_EQ(newValue ? **newValue : Ratio::fromPercent(100),
              *spbx.getValue());
    EXPECT_EQ(newValue, getSettings(dlg, 1).at(0)->getScale());

    // Restore default value.
    restoreDefaults(dlg);
    EXPECT_EQ(!defaultValue, cbx.isChecked());
    EXPECT_EQ(defaultValue.has_value(), spbx.isEnabled());
    EXPECT_EQ(defaultValue ? **defaultValue : Ratio::fromPercent(100),
              *spbx.getValue());
    EXPECT_EQ(defaultValue, getSettings(dlg, 1).at(0)->getScale());

    // Sanity check that the export is actually successful.
    performExport(dlg);
    EXPECT_TRUE(outFile.isExistingFile());
  }
}

TEST_F(GraphicsExportDialogTest, testPixmapDpi) {
  const int defaultValue = 600;
  const int newValue = 1200;
  QString widget =
      "tabWidget/qt_tabwidget_stackedwidget/tabGeneral/spbxResolutionDpi";
  FilePath outFile = getFilePath("out.svg");

  {
    Theme theme;
    GraphicsExportDialog dlg(GraphicsExportDialog::Mode::Schematic,
                             GraphicsExportDialog::Output::Image, getPages(1),
                             0, "test", 0, FilePath(),
                             LengthUnit::millimeters(), theme, "unittest");
    prepareDialog(dlg, outFile);
    QSpinBox& spbx = TestHelpers::getChild<QSpinBox>(dlg, widget);

    // Check the default value.
    EXPECT_EQ(defaultValue, spbx.value());
    EXPECT_EQ(defaultValue, getSettings(dlg, 1).at(0)->getPixmapDpi());

    // Check if the value can be changed and are applied properly.
    spbx.setValue(newValue);
    EXPECT_EQ(newValue, getSettings(dlg, 1).at(0)->getPixmapDpi());
  }

  // Check if the setting is saved and restored automatically, and can be
  // reset to its default value.
  {
    Theme theme;
    GraphicsExportDialog dlg(GraphicsExportDialog::Mode::Schematic,
                             GraphicsExportDialog::Output::Image, getPages(1),
                             0, "test", 0, FilePath(),
                             LengthUnit::millimeters(), theme, "unittest");
    prepareDialog(dlg, outFile);
    QSpinBox& spbx = TestHelpers::getChild<QSpinBox>(dlg, widget);

    // Check new value.
    EXPECT_EQ(newValue, spbx.value());
    EXPECT_EQ(newValue, getSettings(dlg, 1).at(0)->getPixmapDpi());

    // Restore default value.
    restoreDefaults(dlg);
    EXPECT_EQ(defaultValue, spbx.value());
    EXPECT_EQ(defaultValue, getSettings(dlg, 1).at(0)->getPixmapDpi());

    // Sanity check that the export is actually successful.
    performExport(dlg);
    EXPECT_TRUE(outFile.isExistingFile());
  }
}

TEST_F(GraphicsExportDialogTest, testBlackWhite) {
  const bool defaultValue = false;
  const bool newValue = true;
  QString widget =
      "tabWidget/qt_tabwidget_stackedwidget/tabAdvanced/cbxBlackWhite";
  FilePath outFile = getFilePath("out.pdf");

  {
    Theme theme;
    GraphicsExportDialog dlg(GraphicsExportDialog::Mode::Schematic,
                             GraphicsExportDialog::Output::Pdf, getPages(1), 0,
                             "test", 0, FilePath(), LengthUnit::millimeters(),
                             theme, "unittest");
    prepareDialog(dlg, outFile);
    QCheckBox& cbx = TestHelpers::getChild<QCheckBox>(dlg, widget);

    // Check the default value.
    EXPECT_EQ(defaultValue, cbx.isChecked());
    EXPECT_EQ(defaultValue, getSettings(dlg, 1).at(0)->getBlackWhite());

    // Check if the value can be changed and are applied properly.
    cbx.setChecked(newValue);
    EXPECT_EQ(newValue, getSettings(dlg, 1).at(0)->getBlackWhite());
  }

  // Check if the setting is saved and restored automatically, and can be
  // reset to its default value.
  {
    Theme theme;
    GraphicsExportDialog dlg(GraphicsExportDialog::Mode::Schematic,
                             GraphicsExportDialog::Output::Pdf, getPages(1), 0,
                             "test", 0, FilePath(), LengthUnit::millimeters(),
                             theme, "unittest");
    prepareDialog(dlg, outFile);
    QCheckBox& cbx = TestHelpers::getChild<QCheckBox>(dlg, widget);

    // Check new value.
    EXPECT_EQ(newValue, cbx.isChecked());
    EXPECT_EQ(newValue, getSettings(dlg, 1).at(0)->getBlackWhite());

    // Restore default value.
    restoreDefaults(dlg);
    EXPECT_EQ(defaultValue, cbx.isChecked());
    EXPECT_EQ(defaultValue, getSettings(dlg, 1).at(0)->getBlackWhite());

    // Sanity check that the export is actually successful.
    performExport(dlg);
    EXPECT_TRUE(outFile.isExistingFile());
  }
}

TEST_F(GraphicsExportDialogTest, testBackgroundColor) {
  const QColor defaultValue = Qt::transparent;
  const QColor newValue = Qt::black;
  QString widgetDefault =
      "tabWidget/qt_tabwidget_stackedwidget/tabGeneral/rbtnBackgroundNone";
  QString widgetNew =
      "tabWidget/qt_tabwidget_stackedwidget/tabGeneral/rbtnBackgroundBlack";
  FilePath outFile = getFilePath("out.pdf");

  {
    Theme theme;
    GraphicsExportDialog dlg(GraphicsExportDialog::Mode::Schematic,
                             GraphicsExportDialog::Output::Pdf, getPages(1), 0,
                             "test", 0, FilePath(), LengthUnit::millimeters(),
                             theme, "unittest");
    prepareDialog(dlg, outFile);
    QRadioButton& rbtnDefault =
        TestHelpers::getChild<QRadioButton>(dlg, widgetDefault);
    QRadioButton& rbtnNew = TestHelpers::getChild<QRadioButton>(dlg, widgetNew);

    // Check the default value.
    EXPECT_TRUE(rbtnDefault.isChecked());
    EXPECT_FALSE(rbtnNew.isChecked());
    EXPECT_EQ(defaultValue, getSettings(dlg, 1).at(0)->getBackgroundColor());

    // Check if the value can be changed and are applied properly.
    rbtnNew.setChecked(true);
    EXPECT_EQ(newValue, getSettings(dlg, 1).at(0)->getBackgroundColor());
  }

  // Check if the setting is saved and restored automatically, and can be
  // reset to its default value.
  {
    Theme theme;
    GraphicsExportDialog dlg(GraphicsExportDialog::Mode::Schematic,
                             GraphicsExportDialog::Output::Pdf, getPages(1), 0,
                             "test", 0, FilePath(), LengthUnit::millimeters(),
                             theme, "unittest");
    prepareDialog(dlg, outFile);
    QRadioButton& rbtnDefault =
        TestHelpers::getChild<QRadioButton>(dlg, widgetDefault);
    QRadioButton& rbtnNew = TestHelpers::getChild<QRadioButton>(dlg, widgetNew);

    // Check new value.
    EXPECT_FALSE(rbtnDefault.isChecked());
    EXPECT_TRUE(rbtnNew.isChecked());
    EXPECT_EQ(newValue, getSettings(dlg, 1).at(0)->getBackgroundColor());

    // Restore default value.
    restoreDefaults(dlg);
    EXPECT_TRUE(rbtnDefault.isChecked());
    EXPECT_FALSE(rbtnNew.isChecked());
    EXPECT_EQ(defaultValue, getSettings(dlg, 1).at(0)->getBackgroundColor());

    // Sanity check that the export is actually successful.
    performExport(dlg);
    EXPECT_TRUE(outFile.isExistingFile());
  }
}

TEST_F(GraphicsExportDialogTest, testMinLineWidth) {
  const UnsignedLength defaultValue(100000);
  const UnsignedLength newValue(500000);
  QString widget =
      "tabWidget/qt_tabwidget_stackedwidget/tabAdvanced/edtMinLineWidth";
  FilePath outFile = getFilePath("out.pdf");

  {
    Theme theme;
    GraphicsExportDialog dlg(GraphicsExportDialog::Mode::Schematic,
                             GraphicsExportDialog::Output::Pdf, getPages(1), 0,
                             "test", 0, FilePath(), LengthUnit::millimeters(),
                             theme, "unittest");
    prepareDialog(dlg, outFile);
    UnsignedLengthEdit& edt =
        TestHelpers::getChild<UnsignedLengthEdit>(dlg, widget);

    // Check the default value.
    EXPECT_EQ(defaultValue, edt.getValue());
    EXPECT_EQ(defaultValue, getSettings(dlg, 1).at(0)->getMinLineWidth());

    // Check if the value can be changed and are applied properly.
    edt.setValue(newValue);
    EXPECT_EQ(newValue, getSettings(dlg, 1).at(0)->getMinLineWidth());
  }

  // Check if the setting is saved and restored automatically, and can be
  // reset to its default value.
  {
    Theme theme;
    GraphicsExportDialog dlg(GraphicsExportDialog::Mode::Schematic,
                             GraphicsExportDialog::Output::Pdf, getPages(1), 0,
                             "test", 0, FilePath(), LengthUnit::millimeters(),
                             theme, "unittest");
    prepareDialog(dlg, outFile);
    UnsignedLengthEdit& edt =
        TestHelpers::getChild<UnsignedLengthEdit>(dlg, widget);

    // Check new value.
    EXPECT_EQ(newValue, edt.getValue());
    EXPECT_EQ(newValue, getSettings(dlg, 1).at(0)->getMinLineWidth());

    // Restore default value.
    restoreDefaults(dlg);
    EXPECT_EQ(defaultValue, edt.getValue());
    EXPECT_EQ(defaultValue, getSettings(dlg, 1).at(0)->getMinLineWidth());

    // Sanity check that the export is actually successful.
    performExport(dlg);
    EXPECT_TRUE(outFile.isExistingFile());
  }
}

TEST_F(GraphicsExportDialogTest, testLayerColors) {
  const QStringList layers = {
      Theme::Color::sSchematicFrames,        Theme::Color::sSchematicOutlines,
      Theme::Color::sSchematicGrabAreas,     Theme::Color::sSchematicPinLines,
      Theme::Color::sSchematicPinNames,      Theme::Color::sSchematicPinNumbers,
      Theme::Color::sSchematicNames,         Theme::Color::sSchematicValues,
      Theme::Color::sSchematicWires,         Theme::Color::sSchematicNetLabels,
      Theme::Color::sSchematicDocumentation, Theme::Color::sSchematicComments,
      Theme::Color::sSchematicGuide,
  };
  QList<std::pair<QString, QColor>> defaultValue;
  QList<std::pair<QString, QColor>> newValue;
  const Theme theme;
  for (int i = 0; i < layers.count(); ++i) {
    const ThemeColor& color = theme.getColor(layers.at(i));
    defaultValue.append(std::make_pair(layers.at(i), color.getPrimaryColor()));
    newValue.append(
        std::make_pair(layers.at(i), QColor::colorNames().value(i)));
  }
  QString widget =
      "tabWidget/qt_tabwidget_stackedwidget/tabColors/lstLayerColors";
  FilePath outFile = getFilePath("out.pdf");

  {
    Theme theme;
    GraphicsExportDialog dlg(GraphicsExportDialog::Mode::Schematic,
                             GraphicsExportDialog::Output::Pdf, getPages(1), 0,
                             "test", 0, FilePath(), LengthUnit::millimeters(),
                             theme, "unittest");
    prepareDialog(dlg, outFile);
    enablePinNumbers(dlg);
    QListWidget& lst = TestHelpers::getChild<QListWidget>(dlg, widget);

    // Check the default value.
    EXPECT_EQ(defaultValue.count(), lst.count());
    for (int i = 0; i < defaultValue.count(); ++i) {
      EXPECT_EQ(defaultValue.at(i).second,
                lst.item(i)->data(Qt::DecorationRole));
    }
    EXPECT_EQ(str(defaultValue), str(getSettings(dlg, 1).at(0)->getColors()));

    // Check if the value can be changed and are applied properly.
    for (int i = 0; i < defaultValue.count(); ++i) {
      lst.item(i)->setData(Qt::DecorationRole, newValue.at(i).second);
    }
    EXPECT_EQ(str(newValue), str(getSettings(dlg, 1).at(0)->getColors()));
  }

  // Check if the setting is saved and restored automatically, and can be
  // reset to its default value.
  {
    Theme theme;
    GraphicsExportDialog dlg(GraphicsExportDialog::Mode::Schematic,
                             GraphicsExportDialog::Output::Pdf, getPages(1), 0,
                             "test", 0, FilePath(), LengthUnit::millimeters(),
                             theme, "unittest");
    prepareDialog(dlg, outFile);
    enablePinNumbers(dlg);
    QListWidget& lst = TestHelpers::getChild<QListWidget>(dlg, widget);

    // Check new value.
    EXPECT_EQ(newValue.count(), lst.count());
    for (int i = 0; i < newValue.count(); ++i) {
      EXPECT_EQ(newValue.at(i).second, lst.item(i)->data(Qt::DecorationRole));
    }
    EXPECT_EQ(str(newValue), str(getSettings(dlg, 1).at(0)->getColors()));

    // Restore default value.
    restoreDefaults(dlg);
    enablePinNumbers(dlg);
    EXPECT_EQ(defaultValue.count(), lst.count());
    for (int i = 0; i < defaultValue.count(); ++i) {
      EXPECT_EQ(defaultValue.at(i).second,
                lst.item(i)->data(Qt::DecorationRole));
    }
    EXPECT_EQ(str(defaultValue), str(getSettings(dlg, 1).at(0)->getColors()));

    // Sanity check that the export is actually successful.
    performExport(dlg);
    EXPECT_TRUE(outFile.isExistingFile());
  }
}

TEST_F(GraphicsExportDialogTest, testOpenExportedFiles) {
  const bool defaultValue = true;
  const bool newValue = false;
  QString widget = "cbxOpenExportedFiles";
  FilePath outFile = getFilePath("out.pdf");

  {
    Theme theme;
    GraphicsExportDialog dlg(GraphicsExportDialog::Mode::Schematic,
                             GraphicsExportDialog::Output::Pdf, getPages(1), 0,
                             "test", 0, FilePath(), LengthUnit::millimeters(),
                             theme, "unittest");
    prepareDialog(dlg, outFile);
    QCheckBox& cbx = TestHelpers::getChild<QCheckBox>(dlg, widget);

    // Check the default value.
    EXPECT_EQ(defaultValue, cbx.isChecked());
    performExport(dlg);
    EXPECT_EQ(QVector<FilePath>{outFile}, mRequestedFilesToOpen);

    // Check if the value can be changed and are applied properly.
    cbx.setChecked(newValue);
    performExport(dlg);
    EXPECT_EQ(QVector<FilePath>{}, mRequestedFilesToOpen);
  }

  // Check if the setting is saved and restored automatically, and can be
  // reset to its default value.
  {
    Theme theme;
    GraphicsExportDialog dlg(GraphicsExportDialog::Mode::Schematic,
                             GraphicsExportDialog::Output::Pdf, getPages(1), 0,
                             "test", 0, FilePath(), LengthUnit::millimeters(),
                             theme, "unittest");
    prepareDialog(dlg, outFile);
    QCheckBox& cbx = TestHelpers::getChild<QCheckBox>(dlg, widget);

    // Check new value.
    EXPECT_EQ(newValue, cbx.isChecked());
    performExport(dlg);
    EXPECT_EQ(QVector<FilePath>{}, mRequestedFilesToOpen);

    // Restore default value.
    restoreDefaults(dlg);
    EXPECT_EQ(defaultValue, cbx.isChecked());
    performExport(dlg);
    EXPECT_EQ(QVector<FilePath>{outFile}, mRequestedFilesToOpen);
  }
}

TEST_F(GraphicsExportDialogTest, testPageRange) {
  QString widgetAll =
      "tabWidget/qt_tabwidget_stackedwidget/tabPages/rbtnRangeAll";
  QString widgetCurrent =
      "tabWidget/qt_tabwidget_stackedwidget/tabPages/rbtnRangeCurrent";
  QString widgetCustom =
      "tabWidget/qt_tabwidget_stackedwidget/tabPages/rbtnRangeCustom";
  QString widgetCustomRange =
      "tabWidget/qt_tabwidget_stackedwidget/tabPages/edtPageRange";
  FilePath outFile = getFilePath("out.pdf");

  {
    Theme theme;
    GraphicsExportDialog dlg(GraphicsExportDialog::Mode::Schematic,
                             GraphicsExportDialog::Output::Pdf, getPages(3), 1,
                             "test", 0, FilePath(), LengthUnit::millimeters(),
                             theme, "unittest");
    prepareDialog(dlg, outFile);
    QRadioButton& rbtnAll = TestHelpers::getChild<QRadioButton>(dlg, widgetAll);
    QRadioButton& rbtnCurrent =
        TestHelpers::getChild<QRadioButton>(dlg, widgetCurrent);
    QRadioButton& rbtnCustom =
        TestHelpers::getChild<QRadioButton>(dlg, widgetCustom);
    QLineEdit& edtRange =
        TestHelpers::getChild<QLineEdit>(dlg, widgetCustomRange);

    // Check the default value.
    EXPECT_TRUE(rbtnAll.isChecked());
    EXPECT_FALSE(rbtnCurrent.isChecked());
    EXPECT_FALSE(rbtnCustom.isChecked());
    EXPECT_FALSE(edtRange.isEnabled());
    EXPECT_EQ("", edtRange.text().toStdString());
    EXPECT_EQ("1-3", edtRange.placeholderText().toStdString());
    EXPECT_EQ(3, getSettings(dlg).count());  // Number of exported pages.

    // Test custom range 1-3.
    rbtnCustom.click();
    EXPECT_FALSE(rbtnAll.isChecked());
    EXPECT_FALSE(rbtnCurrent.isChecked());
    EXPECT_TRUE(rbtnCustom.isChecked());
    EXPECT_TRUE(edtRange.isEnabled());
    EXPECT_EQ(3, getSettings(dlg).count());  // Number of exported pages.

    // Test custom range 1,3.
    edtRange.setText("1,3");
    EXPECT_EQ(2, getSettings(dlg).count());  // Number of exported pages.

    // Test current page.
    rbtnCurrent.click();
    EXPECT_FALSE(rbtnAll.isChecked());
    EXPECT_TRUE(rbtnCurrent.isChecked());
    EXPECT_FALSE(rbtnCustom.isChecked());
    EXPECT_FALSE(edtRange.isEnabled());
    EXPECT_EQ(1, getSettings(dlg).count());  // Number of exported pages.

    // Sanity check that the export is actually successful.
    performExport(dlg);
    EXPECT_TRUE(outFile.isExistingFile());
  }

  // Check if the setting is NOT saved and restored, to avoid accidentally
  // printing the wrong pages.
  {
    Theme theme;
    GraphicsExportDialog dlg(GraphicsExportDialog::Mode::Schematic,
                             GraphicsExportDialog::Output::Pdf, getPages(3), 1,
                             "test", 0, FilePath(), LengthUnit::millimeters(),
                             theme, "unittest");
    prepareDialog(dlg, outFile);
    QRadioButton& rbtnAll = TestHelpers::getChild<QRadioButton>(dlg, widgetAll);
    QRadioButton& rbtnCurrent =
        TestHelpers::getChild<QRadioButton>(dlg, widgetCurrent);
    QRadioButton& rbtnCustom =
        TestHelpers::getChild<QRadioButton>(dlg, widgetCustom);
    QLineEdit& edtRange =
        TestHelpers::getChild<QLineEdit>(dlg, widgetCustomRange);

    // Check the default value.
    EXPECT_TRUE(rbtnAll.isChecked());
    EXPECT_FALSE(rbtnCurrent.isChecked());
    EXPECT_FALSE(rbtnCustom.isChecked());
    EXPECT_FALSE(edtRange.isEnabled());
    EXPECT_EQ("", edtRange.text().toStdString());
    EXPECT_EQ("1-3", edtRange.placeholderText().toStdString());
    EXPECT_EQ(3, getSettings(dlg).count());  // Number of exported pages.
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace editor
}  // namespace librepcb
