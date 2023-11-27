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

#include "graphicsexporttest.h"

#include <gtest/gtest.h>
#include <librepcb/core/export/graphicsexport.h>

#include <QtCore>
#include <QtSvg>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace tests {

/*******************************************************************************
 *  Test Class
 ******************************************************************************/

class GraphicsExportTest : public ::testing::Test {
protected:
  FilePath mOutputDir;
  QVector<FilePath> mSavedFiles;  // From signal savingFile().

  GraphicsExportTest() : mOutputDir(FilePath::getRandomTempPath()) {
    QSettings().clear();
  }

  ~GraphicsExportTest() { QDir(mOutputDir.toStr()).removeRecursively(); }

  FilePath getFilePath(const QString& fileName) const {
    return mOutputDir.getPathTo(fileName);
  }

  void prepare(GraphicsExport& e) {
    QObject::connect(&e, &GraphicsExport::savingFile,
                     [this](const FilePath& fp) { mSavedFiles.append(fp); });
  }

  QSize getImageSize(const FilePath& fp) const {
    QImage image;
    if (!image.load(fp.toStr())) {
      throw Exception(__FILE__, __LINE__, "Image could not be loaded.");
    }
    return image.size();
  }

  QSize getSvgSize(const FilePath& fp) const {
    QSvgRenderer svg;
    if (!svg.load(fp.toStr())) {
      throw Exception(__FILE__, __LINE__, "SVG could not be loaded.");
    }
    return svg.viewBox().size();
  }

  std::string str(const QVector<FilePath>& paths) const {
    QStringList l;
    foreach (const FilePath& fp, paths) {
      l.append(fp.toStr());
    }
    return l.join(", ").toStdString();
  }

  std::string str(const QSize& size) const {
    return QString("%1x%2").arg(size.width()).arg(size.height()).toStdString();
  }
};

/*******************************************************************************
 *  Test Methods
 ******************************************************************************/

TEST_F(GraphicsExportTest, testExportImageWithAutoScaling) {
  std::shared_ptr<GraphicsPagePainter> page =
      std::make_shared<GraphicsPagePainterMock>(
          Length(10000000), Length(20000000), Length(508000000),
          Length(254000000));
  std::shared_ptr<GraphicsExportSettings> settings =
      std::make_shared<GraphicsExportSettings>();
  settings->setPixmapDpi(100);
  settings->setScale(tl::nullopt);
  settings->setMarginLeft(UnsignedLength(25400000));  // 5% of width.
  settings->setMarginTop(UnsignedLength(12700000));  // 5% of height.
  settings->setMarginRight(UnsignedLength(50800000));  // 10% of width.
  settings->setMarginBottom(UnsignedLength(25400000));  // 10% of height.
  GraphicsExport::Pages pages = {std::make_pair(page, settings)};

  GraphicsExport e;
  prepare(e);

  FilePath outFile = getFilePath("out.png");
  e.startExport(pages, outFile);
  const GraphicsExport::Result result = e.waitForFinished();
  EXPECT_EQ("", result.errorMsg.toStdString());
  EXPECT_EQ(str({outFile}), str(result.writtenFiles));
  EXPECT_EQ(str({outFile}), str(mSavedFiles));
  EXPECT_TRUE(outFile.isExistingFile());
  EXPECT_EQ("2300x1150", str(getImageSize(outFile)));  // 2000x1000 + margins.
}

TEST_F(GraphicsExportTest, testExportImageWithManualScaling) {
  std::shared_ptr<GraphicsPagePainter> page =
      std::make_shared<GraphicsPagePainterMock>(
          Length(10000000), Length(20000000), Length(508000000),
          Length(254000000));
  std::shared_ptr<GraphicsExportSettings> settings =
      std::make_shared<GraphicsExportSettings>();
  settings->setPixmapDpi(100);
  settings->setScale(UnsignedRatio(Ratio::fromNormalized(4)));
  settings->setMarginLeft(UnsignedLength(25400000));  // 5% of width.
  settings->setMarginTop(UnsignedLength(12700000));  // 5% of height.
  settings->setMarginRight(UnsignedLength(50800000));  // 10% of width.
  settings->setMarginBottom(UnsignedLength(25400000));  // 10% of height.
  GraphicsExport::Pages pages = {std::make_pair(page, settings)};

  GraphicsExport e;
  prepare(e);

  FilePath outFile = getFilePath("out.png");
  e.startExport(pages, outFile);
  const GraphicsExport::Result result = e.waitForFinished();
  EXPECT_EQ("", result.errorMsg.toStdString());
  EXPECT_EQ(str({outFile}), str(result.writtenFiles));
  EXPECT_EQ(str({outFile}), str(mSavedFiles));
  EXPECT_TRUE(outFile.isExistingFile());
  EXPECT_EQ("8300x4150", str(getImageSize(outFile)));  // 8000x4000 + margins.
}

TEST_F(GraphicsExportTest, testExportMultipleImages) {
  std::shared_ptr<GraphicsPagePainter> page =
      std::make_shared<GraphicsPagePainterMock>(
          Length(10000000), Length(20000000), Length(508000000),
          Length(254000000));
  std::shared_ptr<GraphicsExportSettings> settings1 =
      std::make_shared<GraphicsExportSettings>();
  settings1->setPixmapDpi(10);
  settings1->setScale(tl::nullopt);
  settings1->setMarginLeft(UnsignedLength(0));
  settings1->setMarginTop(UnsignedLength(0));
  settings1->setMarginRight(UnsignedLength(0));
  settings1->setMarginBottom(UnsignedLength(0));
  std::shared_ptr<GraphicsExportSettings> settings2 =
      std::make_shared<GraphicsExportSettings>(*settings1);
  settings2->setPixmapDpi(20);
  std::shared_ptr<GraphicsExportSettings> settings3 =
      std::make_shared<GraphicsExportSettings>(*settings1);
  settings3->setPixmapDpi(30);
  GraphicsExport::Pages pages = {
      std::make_pair(page, settings1),
      std::make_pair(page, settings2),
      std::make_pair(page, settings3),
  };

  GraphicsExport e;
  prepare(e);

  e.startExport(pages, getFilePath("out.png"));
  const GraphicsExport::Result result = e.waitForFinished();
  EXPECT_EQ("", result.errorMsg.toStdString());
  EXPECT_EQ(str({
                getFilePath("out1.png"),
                getFilePath("out2.png"),
                getFilePath("out3.png"),
            }),
            str(result.writtenFiles));
  EXPECT_EQ(str({
                getFilePath("out1.png"),
                getFilePath("out2.png"),
                getFilePath("out3.png"),
            }),
            str(mSavedFiles));
  EXPECT_TRUE(getFilePath("out1.png").isExistingFile());
  EXPECT_TRUE(getFilePath("out2.png").isExistingFile());
  EXPECT_TRUE(getFilePath("out3.png").isExistingFile());
  EXPECT_EQ("200x100", str(getImageSize(getFilePath("out1.png"))));
  EXPECT_EQ("400x200", str(getImageSize(getFilePath("out2.png"))));
  EXPECT_EQ("600x300", str(getImageSize(getFilePath("out3.png"))));
}

TEST_F(GraphicsExportTest, testExportSvgWithAutoScaling) {
  std::shared_ptr<GraphicsPagePainter> page =
      std::make_shared<GraphicsPagePainterMock>(
          Length(10000000), Length(20000000), Length(508000000),
          Length(254000000));
  std::shared_ptr<GraphicsExportSettings> settings =
      std::make_shared<GraphicsExportSettings>();
  settings->setPixmapDpi(100);
  settings->setScale(tl::nullopt);
  settings->setMarginLeft(UnsignedLength(25400000));  // 5% of width.
  settings->setMarginTop(UnsignedLength(12700000));  // 5% of height.
  settings->setMarginRight(UnsignedLength(50800000));  // 10% of width.
  settings->setMarginBottom(UnsignedLength(25400000));  // 10% of height.
  GraphicsExport::Pages pages = {std::make_pair(page, settings)};

  GraphicsExport e;
  prepare(e);

  FilePath outFile = getFilePath("out.svg");
  e.startExport(pages, outFile);
  const GraphicsExport::Result result = e.waitForFinished();
  EXPECT_EQ("", result.errorMsg.toStdString());
  EXPECT_EQ(str({outFile}), str(result.writtenFiles));
  EXPECT_EQ(str({outFile}), str(mSavedFiles));
  EXPECT_TRUE(outFile.isExistingFile());
  EXPECT_EQ("2300x1150", str(getSvgSize(outFile)));  // 2000x1000 + margins.
}

TEST_F(GraphicsExportTest, testExportSvgWithManualScaling) {
  std::shared_ptr<GraphicsPagePainter> page =
      std::make_shared<GraphicsPagePainterMock>(
          Length(10000000), Length(20000000), Length(508000000),
          Length(254000000));
  std::shared_ptr<GraphicsExportSettings> settings =
      std::make_shared<GraphicsExportSettings>();
  settings->setPixmapDpi(100);
  settings->setScale(UnsignedRatio(Ratio::fromNormalized(4)));
  settings->setMarginLeft(UnsignedLength(25400000));  // 5% of width.
  settings->setMarginTop(UnsignedLength(12700000));  // 5% of height.
  settings->setMarginRight(UnsignedLength(50800000));  // 10% of width.
  settings->setMarginBottom(UnsignedLength(25400000));  // 10% of height.
  GraphicsExport::Pages pages = {std::make_pair(page, settings)};

  GraphicsExport e;
  prepare(e);

  FilePath outFile = getFilePath("out.svg");
  e.startExport(pages, outFile);
  const GraphicsExport::Result result = e.waitForFinished();
  EXPECT_EQ("", result.errorMsg.toStdString());
  EXPECT_EQ(str({outFile}), str(result.writtenFiles));
  EXPECT_EQ(str({outFile}), str(mSavedFiles));
  EXPECT_TRUE(outFile.isExistingFile());
  EXPECT_EQ("8300x4150", str(getSvgSize(outFile)));  // 8000x4000 + margins.
}

TEST_F(GraphicsExportTest, testExportPdfWithAutoScaling) {
  std::shared_ptr<GraphicsPagePainter> page1 =
      std::make_shared<GraphicsPagePainterMock>(
          Length(10000000), Length(20000000), Length(200000000),
          Length(100000000));
  std::shared_ptr<GraphicsPagePainter> page2 =
      std::make_shared<GraphicsPagePainterMock>(
          Length(10000000), Length(20000000), Length(300000000),
          Length(100000000));
  std::shared_ptr<GraphicsPagePainter> page3 =
      std::make_shared<GraphicsPagePainterMock>(
          Length(10000000), Length(20000000), Length(400000000),
          Length(100000000));
  std::shared_ptr<GraphicsExportSettings> settings =
      std::make_shared<GraphicsExportSettings>();
  settings->setPageSize(tl::nullopt);
  settings->setOrientation(GraphicsExportSettings::Orientation::Auto);
  settings->setScale(tl::nullopt);
  settings->setMarginLeft(UnsignedLength(10000000));  // 10mm.
  settings->setMarginTop(UnsignedLength(20000000));  // 20mm.
  settings->setMarginRight(UnsignedLength(30000000));  // 30mm.
  settings->setMarginBottom(UnsignedLength(40000000));  // 40mm.
  GraphicsExport::Pages pages = {
      std::make_pair(page1, settings),
      std::make_pair(page2, settings),
      std::make_pair(page3, settings),
  };

  GraphicsExport e;
  prepare(e);

  FilePath outFile = getFilePath("out.pdf");
  e.startExport(pages, outFile);
  const GraphicsExport::Result result = e.waitForFinished();
  EXPECT_EQ("", result.errorMsg.toStdString());
  EXPECT_EQ(str({outFile}), str(result.writtenFiles));
  EXPECT_EQ(str({outFile}), str(mSavedFiles));
  EXPECT_TRUE(outFile.isExistingFile());
}

TEST_F(GraphicsExportTest, testGetSupportedExtensions) {
  // Note that the result is platform dependent, thus only checking the
  // most important extensions.
  auto extensions = GraphicsExport::getSupportedExtensions();
  EXPECT_TRUE(extensions.contains("pdf"));
  EXPECT_TRUE(extensions.contains("svg"));
  EXPECT_TRUE(extensions.contains("png"));
}

TEST_F(GraphicsExportTest, testGetSupportedImageExtensions) {
  // Note that the result is platform dependent, thus only checking the
  // most important extensions.
  auto extensions = GraphicsExport::getSupportedImageExtensions();
  EXPECT_FALSE(extensions.contains("pdf"));
  EXPECT_TRUE(extensions.contains("svg"));
  EXPECT_TRUE(extensions.contains("png"));
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace librepcb
