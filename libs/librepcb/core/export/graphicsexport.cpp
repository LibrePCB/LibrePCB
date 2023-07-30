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
#include "graphicsexport.h"

#include "../application.h"
#include "../fileio/fileutils.h"
#include "graphicsexportsettings.h"
#include "utils/qtmetatyperegistration.h"

#include <QtConcurrent>
#include <QtCore>
#include <QtGui>
#include <QtPrintSupport>
#include <QtSvg>

Q_DECLARE_METATYPE(QImage)
Q_DECLARE_METATYPE(std::shared_ptr<QPicture>)

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

// Register Qt meta types.
static QtMetaTypeRegistration<QImage> sImageMetaType;
static QtMetaTypeRegistration<std::shared_ptr<QPicture>> sSharedPictureMetaType;

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

GraphicsExport::GraphicsExport(QObject* parent) noexcept
  : QObject(parent),
    mCreator(QString("LibrePCB %1").arg(Application::getVersion())),
    mDocumentName(),
    mFuture(),
    mAbort(false) {
  connect(this, &GraphicsExport::imageCopiedToClipboard, qApp->clipboard(),
          &QClipboard::setImage, Qt::BlockingQueuedConnection);
}

GraphicsExport::~GraphicsExport() noexcept {
  cancel();
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void GraphicsExport::startPreview(const Pages& pages) noexcept {
  cancel();
  RunArgs args{
      true, pages, FilePath(), QString(), QPrinter::DuplexNone, 1,
  };
  mFuture = QtConcurrent::run(this, &GraphicsExport::run, args);
}

void GraphicsExport::startExport(const Pages& pages,
                                 const FilePath& filePath) noexcept {
  cancel();
  RunArgs args{
      false, pages, filePath, QString(), QPrinter::DuplexNone, 1,
  };
  mFuture = QtConcurrent::run(this, &GraphicsExport::run, args);
}

void GraphicsExport::startPrint(const Pages& pages, const QString& printerName,
                                QPrinter::DuplexMode duplex,
                                int copies) noexcept {
  cancel();
  RunArgs args{
      false, pages, FilePath(), printerName, duplex, copies,
  };
  mFuture = QtConcurrent::run(this, &GraphicsExport::run, args);
}

QString GraphicsExport::waitForFinished() noexcept {
  mFuture.waitForFinished();
  return mFuture.result();
}

void GraphicsExport::cancel() noexcept {
  mAbort = true;
  mFuture.waitForFinished();
  mAbort = false;
}

/*******************************************************************************
 *  Static Methods
 ******************************************************************************/

QStringList GraphicsExport::getSupportedExtensions() noexcept {
  return QStringList{"pdf"} + getSupportedImageExtensions();
}

QStringList GraphicsExport::getSupportedImageExtensions() noexcept {
  QStringList l = {"svg"};
  foreach (const QByteArray& ext, QImageWriter::supportedImageFormats()) {
    l.append(ext);
  }
  return l;
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

QString GraphicsExport::run(RunArgs args) noexcept {
  // Note: This method is called from a different thread, thus be careful with
  //       calling other methods to only call thread-safe methods!

  QElapsedTimer timer;
  timer.start();
  qDebug() << "Start graphics export in worker thread...";
  emit progress(10, 0, args.pages.count());

  try {
    QPagedPaintDevice* pagedPaintDevice = nullptr;

    // Determine file basename and extension.
    const QString fileExt = args.filePath.getSuffix().toLower();
    QString outputFilePathTmpl;
    if (args.filePath.isValid() && (args.pages.count() > 1)) {
      outputFilePathTmpl = args.filePath.toStr();
      outputFilePathTmpl.chop(fileExt.length() + 1);
      outputFilePathTmpl += "%1." % args.filePath.getSuffix();
    }

    // Create output directory first because QPrinter (and maybe QImage?)
    // silently fails if it doesn't exist.
    if (args.filePath.isValid()) {
      FileUtils::makePath(args.filePath.getParentDir());  // can throw
    }

    // Setup printer only for printing.
    QScopedPointer<QPrinter> printer;
    if (!args.printerName.isEmpty()) {
      printer.reset(new QPrinter(QPrinter::HighResolution));
      printer->setPrinterName(args.printerName);
      printer->setCreator(mCreator);
      printer->setDocName(mDocumentName);
      printer->setFontEmbeddingEnabled(true);  // Not sure if needed...
      printer->setFullPage(true);  // Avoid scaling error caused by margins.
      printer->setDuplex(args.duplex);
      printer->setCopyCount(args.copies);
      pagedPaintDevice = printer.data();
    }

    // Setup PDF writer only for PDF export.
    QScopedPointer<QPdfWriter> pdfWriter;
    if (fileExt == "pdf") {
      pdfWriter.reset(new QPdfWriter(args.filePath.toStr()));
      pdfWriter->setCreator(mCreator);
      pdfWriter->setTitle(mDocumentName);
      pdfWriter->setPageMargins(QMarginsF(0, 0, 0, 0));  // Manually set below.
      pagedPaintDevice = pdfWriter.data();
      emit savingFile(args.filePath);
    }

    // QPagedPaintDevice fails if there are no pages, so let's throw a clear
    // error in that case.
    if ((pagedPaintDevice) && args.pages.isEmpty()) {
      throw RuntimeError(__FILE__, __LINE__, tr("No pages to export/print."));
    }

    // Export all pages.
    QPainter painter;
    for (int index = 0; index < args.pages.count(); ++index) {
      const qreal percentPerPage = qreal(80) / args.pages.count();
      emit progress(20 + std::ceil(percentPerPage * index), index + 1,
                    args.pages.count());
      const Page& page = args.pages.at(index);
      if (mAbort) {
        break;
      }

      // Determine source bounding rect.
      QRectF sourceRectPx = calcSourceRect(*page.first, *page.second);
      QTransform sourceTransform = getSourceTransformation(*page.second);
      QRectF sourceRectTransformedPx = sourceTransform.mapRect(sourceRectPx);

      // Determine output page size.
      QPageSize pageSize;
      if (page.second->getPageSize() && page.second->getPageSize()->isValid()) {
        // Fixed page size is specified.
        pageSize = *page.second->getPageSize();
      } else {
        // Derive page size from source size.
        Length width = Length::fromPx(sourceRectTransformedPx.width()) +
            *page.second->getMarginLeft() + *page.second->getMarginRight();
        Length height = Length::fromPx(sourceRectTransformedPx.height()) +
            *page.second->getMarginTop() + *page.second->getMarginBottom();
        pageSize =
            QPageSize(QSizeF(width.toMm(), height.toMm()),
                      QPageSize::Millimeter, "Custom", QPageSize::ExactMatch);
      }
      if (pagedPaintDevice && (!pagedPaintDevice->setPageSize(pageSize))) {
        qCritical().nospace()
            << "Failed to set page size for graphics export to "
            << pageSize.name() << ".";
      }

      // Determine output page orientation.
      const QPageLayout::Orientation pageOrientation =
          page.second->getOrientation()
          ? (*page.second->getOrientation())
          : getOrientation(sourceRectTransformedPx.size());
      if (pagedPaintDevice) {
        QPageLayout::Orientation orientation = pageOrientation;
        if (getOrientation(pageSize.sizePoints()) == QPageLayout::Landscape) {
          // QPagedPaintDevice orientation seems to be swapped if page size is
          // landscape (e.g. the Ledger/Tabloid page size).
          if (orientation == QPageLayout::Landscape) {
            orientation = QPageLayout::Portrait;
          } else {
            orientation = QPageLayout::Landscape;
          }
        }
        if (!pagedPaintDevice->setPageOrientation(orientation)) {
          qCritical() << "Failed to set page orientation for graphics export!";
        }
      }

      // Determine DPI.
      int dpi;
      if (printer) {
        dpi = printer->resolution();
      } else if (pdfWriter) {
        dpi = pdfWriter->resolution();
      } else {
        dpi = page.second->getPixmapDpi();
      }
      const qreal pxScale = static_cast<qreal>(dpi) / Length(25400000).toPx();

      // Calculate page margins in output device pixels.
      const QMarginsF pageMarginsPx(
          page.second->getMarginLeft()->toInch() * dpi,
          page.second->getMarginTop()->toInch() * dpi,
          page.second->getMarginRight()->toInch() * dpi,
          page.second->getMarginBottom()->toInch() * dpi);

      // Determine output page rect.
      QRect pageRectPx = pageSize.rectPixels(dpi);
      if (getOrientation(pageRectPx.size()) != pageOrientation) {
        pageRectPx.setSize(pageRectPx.size().transposed());
      }
      const QRectF pageContentRectPx = pageRectPx - pageMarginsPx;

      // Calculate final scale factor.
      const qreal scale = page.second->getScale()
          ? pxScale
          : qMin(pageContentRectPx.width() / sourceRectTransformedPx.width(),
                 pageContentRectPx.height() / sourceRectTransformedPx.height());

      // Determine output file path.
      const FilePath outputFilePath = (!outputFilePathTmpl.isEmpty())
          ? FilePath(outputFilePathTmpl.arg(index + 1))
          : args.filePath;

      // Last chance to abort before exporting.
      emit progress(20 + std::ceil(percentPerPage * (index + qreal(0.5))),
                    index + 1, args.pages.count());
      if (mAbort) {
        break;
      }

      // Prepare painter.
      bool beginSuccess = false;
      QScopedPointer<QSvgGenerator> svgGenerator;
      QScopedPointer<QImage> image;
      std::shared_ptr<QPicture> picture;
      if (pagedPaintDevice) {
        qDebug().nospace() << "Export page " << (index + 1) << " to "
                           << args.printerName % args.filePath.toStr() << "...";
        if (index == 0) {
          beginSuccess = painter.begin(pagedPaintDevice);
        } else {
          beginSuccess = pagedPaintDevice->newPage();
        }
      } else if (fileExt == "svg") {
        qDebug().nospace() << "Export page " << (index + 1) << " as SVG to "
                           << outputFilePath.toStr() << "...";
        svgGenerator.reset(new QSvgGenerator());
        svgGenerator->setTitle(mDocumentName);
        svgGenerator->setFileName(outputFilePath.toStr());
        svgGenerator->setSize(pageRectPx.size());
        svgGenerator->setViewBox(pageRectPx);
        svgGenerator->setResolution(dpi);
        beginSuccess = painter.begin(svgGenerator.data());
        emit savingFile(outputFilePath);
      } else if (!args.preview) {
        QString target =
            outputFilePath.isValid() ? outputFilePath.toStr() : "clipboard";
        qDebug().nospace() << "Export page " << (index + 1) << " as pixmap to "
                           << target << "...";
        image.reset(
            new QImage(pageRectPx.size(), QImage::Format_ARGB32_Premultiplied));
        image->fill(Qt::transparent);
        beginSuccess = painter.begin(image.data());
        painter.setRenderHints(QPainter::Antialiasing |
                               QPainter::SmoothPixmapTransform);
      } else {
        qDebug().nospace() << "Generate preview of page " << index + 1 << "...";
        picture = std::make_shared<QPicture>();
        beginSuccess = painter.begin(picture.get());
        painter.setRenderHints(QPainter::Antialiasing |
                               QPainter::SmoothPixmapTransform);
      }
      if (!beginSuccess) {
        throw RuntimeError(
            __FILE__, __LINE__,
            "Failed to start printing - invalid printer or output file?");
      }

      // Perform the export.
      painter.save();
      if (page.second->getBackgroundColor() != Qt::transparent) {
        painter.fillRect(pageRectPx, page.second->getBackgroundColor());
      }
      painter.translate(pageContentRectPx.center().x(),
                        pageContentRectPx.center().y());
      painter.setTransform(sourceTransform, true);
      painter.scale(scale, scale);
      painter.translate(-sourceRectPx.center().x(), -sourceRectPx.center().y());
      page.first->paint(painter, *page.second);
      painter.restore();

      // Finish painting of current page.
      if ((!pagedPaintDevice) && (!painter.end())) {
        throw RuntimeError(__FILE__, __LINE__, "Failed to finish painting.");
      }
      if (image && outputFilePath.isValid()) {
        emit savingFile(outputFilePath);
        if (!image->save(outputFilePath.toStr())) {
          throw RuntimeError(
              __FILE__, __LINE__,
              tr("Failed to export image \"%1\". Check file permissions and "
                 "make sure to use a supported image file extension.")
                  .arg(outputFilePath.toNative()));
        }
      } else if (image) {
        // Copy to clipboard must be performed in the main thread since
        // QClipboard is not thread-safe. This is done by a queued signal-slot
        // connection.
        emit imageCopiedToClipboard(*image, QClipboard::Clipboard);
      }
      if (picture) {
        emit previewReady(index, pageRectPx.size(), pageContentRectPx, picture);
      }
      emit progress(20 + std::ceil(percentPerPage * (index + 1)), index + 1,
                    args.pages.count());
    }

    // Finish export.
    if ((pagedPaintDevice) && (!painter.end())) {
      if (pdfWriter) {
        throw RuntimeError(__FILE__, __LINE__,
                           tr("Failed to finish PDF export. Check "
                              "permissions of output file."));
      } else {
        throw RuntimeError(__FILE__, __LINE__,
                           tr("Failed to finish printing with unknown error."));
      }
    }

    // If a PDF export was aborted, let's delete the PDF file since it might
    // be incomplete and usually it's not expected that an incomplete file
    // is created. However, use QFile::remove() instead of
    // FileUtils::removeFile() since here we don't need an exception if the
    // removal fails (no critical error).
    if (mAbort && pdfWriter) {
      if (!QFile::remove(args.filePath.toStr())) {
        qWarning() << "Failed to remove partially exported PDF file.";
      }
    }

    qDebug() << "Successfully exported graphics in" << timer.elapsed() << "ms.";
    emit progress(100, args.pages.count(), args.pages.count());
    emit succeeded();
    return QString();
  } catch (const Exception& e) {
    QString msg = e.getMsg().isEmpty() ? "Unknown error" : e.getMsg();
    qCritical().noquote() << "Graphics export failed after" << timer.elapsed()
                          << "ms:" << msg;
    emit failed(msg);
    return msg;
  }
}

QTransform GraphicsExport::getSourceTransformation(
    const GraphicsExportSettings& settings) noexcept {
  QTransform t;
  if (settings.getRotate()) {
    t.rotate(-90);
  }
  if (settings.getMirror()) {
    t.scale(-1, 1);
  }
  if (settings.getScale()) {
    t.scale(*settings.getScale(), *settings.getScale());
  }
  return t;
}

QRectF GraphicsExport::calcSourceRect(
    const GraphicsPagePainter& page,
    const GraphicsExportSettings& settings) noexcept {
  QPicture picture;
  QPainter painter;
  painter.begin(&picture);
  page.paint(painter, settings);
  painter.end();
  return picture.boundingRect();
}

QPageLayout::Orientation GraphicsExport::getOrientation(
    const QSizeF& size) noexcept {
  return size.height() > size.width() ? QPageLayout::Orientation::Portrait
                                      : QPageLayout::Orientation::Landscape;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
