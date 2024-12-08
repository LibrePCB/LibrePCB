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

#ifndef LIBREPCB_CORE_GRAPHICSEXPORT_H
#define LIBREPCB_CORE_GRAPHICSEXPORT_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../fileio/filepath.h"
#include "../types/length.h"
#include "graphicsexportsettings.h"

#include <QtCore>
#include <QtGui>
#include <QtPrintSupport>

#include <memory>
#include <optional>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class GraphicsPagePainter
 ******************************************************************************/

/**
 * @brief Base class for printing a page for ::librepcb::GraphicsExport
 *
 * The class ::librepcb::GraphicsExport relies on this base class for
 * performing the actual printing of a page. Subclasses only need to implement
 * #paint() to draw arbitrary graphics which ::librepcb::GraphicsExport will
 * then either send to a printer, PDF file or other output formats.
 *
 * @see ::librepcb::GraphicsExport
 */
class GraphicsPagePainter {
public:
  /**
   * @brief Draw page content on a QPainter
   *
   * @attention This method must be thread-safe as it might be called from
   *            multiple threads at the same time!
   *
   * @note  Most settings are already handled by ::librepcb::GraphicsExport
   *        and should not be taken into account when implementing this method.
   *        In particular, any page layout or coordinate transformations do
   *        not need to be respected by this implementation. And for layer
   *        colors, just use ::librepcb::GraphicsExportSettings::getColor()
   *        and ::librepcb::GraphicsExportSettings::getFillColor().
   *
   * @param painter   Where to paint the content to.
   * @param settings  Helper class to fetch layer colors depending on the
   *                  current export settings.
   */
  virtual void paint(QPainter& painter,
                     const GraphicsExportSettings& settings) const noexcept = 0;
};

/*******************************************************************************
 *  Class GraphicsExport
 ******************************************************************************/

/**
 * @brief Asynchronously exports graphics to a QPainter
 *
 * Used for graphics printing, PDF export, SVG export etc. without blocking
 * the main thread.
 */
class GraphicsExport final : public QObject {
  Q_OBJECT

public:
  // Types
  typedef std::pair<std::shared_ptr<GraphicsPagePainter>,
                    std::shared_ptr<GraphicsExportSettings>>
      Page;
  typedef QList<Page> Pages;

  struct Result {
    QVector<FilePath> writtenFiles;
    QString errorMsg;
  };

  // Constructors / Destructor
  GraphicsExport(QObject* parent = nullptr) noexcept;
  GraphicsExport(const GraphicsExport& other) = delete;
  ~GraphicsExport() noexcept;

  // General Methods

  /**
   * @brief Set the document name used for printing, PDF and SVG export
   *
   * @param name  Document name.
   */
  void setDocumentName(const QString& name) noexcept { mDocumentName = name; }

  /**
   * @brief Start creating previews asynchronously
   *
   * The signal #previewReady() will be emitted from a worker thread for
   * each processed page.
   *
   * @param pages     The pages to create the preview of.
   */
  void startPreview(const Pages& pages) noexcept;

  /**
   * @brief Start exporting to a file or clipboard asynchronously
   *
   * The supported file type will be determined automatically by the file
   * extension. Supported file types are `pdf`, `svg` and all supported file
   * extensions of QImage. See also #getSupportedExtensions().
   *
   * The signals#savingFile() will be emitted from a worker thread for each
   * file created.
   *
   * @param pages     The pages to export.
   * @param filePath  Export file path. If invalid, pixmaps will be copied into
   *                  the clipboard. If multiple pages are exported, the page
   *                  number will automatically be appended to the filename.
   */
  void startExport(const Pages& pages, const FilePath& filePath) noexcept;

  /**
   * @brief Start printing to a printer asynchronously
   *
   * @param pages       The pages to export.
   * @param printerName Name of the printer to use.
   * @param duplex      The duplex mode to use.
   * @param copies      Number of copies to print.
   */
  void startPrint(const Pages& pages, const QString& printerName,
                  QPrinter::DuplexMode duplex, int copies) noexcept;

  /**
   * @brief Wait (block) until the preview/export/print is finished
   *
   * @return Result of the export.
   */
  Result waitForFinished() noexcept;

  /**
   * @brief Cancel the current job
   */
  void cancel() noexcept;

  // Operator Overloadings
  GraphicsExport& operator=(const GraphicsExport& rhs) = delete;

  // Static Methods.

  /**
   * @brief Get all supported file extensions for #startExport()
   *
   * @return File extensions (e.g. "pdf", "svg", "png").
   */
  static QStringList getSupportedExtensions() noexcept;

  /**
   * @brief Get all supported image file extensions for #startExport()
   *
   * @return File extensions (e.g. "svg", "bmp", "png").
   */
  static QStringList getSupportedImageExtensions() noexcept;

signals:
  void previewReady(int index, const QSize& pageSize, const QRectF margins,
                    std::shared_ptr<QPicture> picture);
  void savingFile(const librepcb::FilePath& filePath);
  void progress(int percent, int completed, int total);
  void succeeded();
  void failed(const QString& error);
  void imageCopiedToClipboard(const QImage& image, QClipboard::Mode mode);

private:  // Types
  struct RunArgs {
    bool preview;
    Pages pages;
    FilePath filePath;
    QString printerName;
    QPrinter::DuplexMode duplex;
    int copies;
  };

private:  // Methods
  Result run(RunArgs args) noexcept;
  static QTransform getSourceTransformation(
      const GraphicsExportSettings& settings) noexcept;
  static QRectF calcSourceRect(const GraphicsPagePainter& page,
                               const GraphicsExportSettings& settings) noexcept;
  static QPageLayout::Orientation getOrientation(const QSizeF& size) noexcept;

private:  // Data
  QString mCreator;
  QString mDocumentName;
  QFuture<Result> mFuture;
  bool mAbort;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
