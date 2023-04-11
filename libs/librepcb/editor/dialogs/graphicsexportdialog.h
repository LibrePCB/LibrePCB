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

#ifndef LIBREPCB_EDITOR_GRAPHICSEXPORTDIALOG_H
#define LIBREPCB_EDITOR_GRAPHICSEXPORTDIALOG_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/core/fileio/filepath.h>
#include <librepcb/core/types/length.h>
#include <optional/tl/optional.hpp>

#include <QtCore>
#include <QtPrintSupport>
#include <QtWidgets>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class GraphicsExport;
class GraphicsExportSettings;
class GraphicsPagePainter;
class LengthUnit;
class Theme;

namespace editor {

namespace Ui {
class GraphicsExportDialog;
}

/*******************************************************************************
 *  Class GraphicsExportDialog
 ******************************************************************************/

/**
 * @brief This class provides a Dialog (GUI) to choose DXF import settings
 */
class GraphicsExportDialog final : public QDialog {
  Q_OBJECT

  // Types
  enum class ClientSettingsAction {
    Load,
    Store,
  };

  struct ContentItem {
    QString name;
    bool enabled;
    bool mirror;
    QSet<QString> colors;
  };

public:
  // Types
  enum class Mode {
    Schematic,  ///< Initialize settings suitable for symbols/schematics.
    Board,  ///< Initialize settings suitable for footprints/boards.
  };
  enum class Output {
    Image,
    Pdf,
    Print,
  };
  typedef std::function<QString(QWidget*, const QString&, const QString&,
                                const QString&, QString*, QFileDialog::Options)>
      SaveAsCallback;
  typedef std::pair<std::shared_ptr<GraphicsPagePainter>,
                    std::shared_ptr<GraphicsExportSettings>>
      Page;

  // Constructors / Destructor
  GraphicsExportDialog() = delete;
  GraphicsExportDialog(const GraphicsExportDialog& other) = delete;
  explicit GraphicsExportDialog(
      Mode mode, Output output,
      const QList<std::shared_ptr<GraphicsPagePainter>>& pages, int currentPage,
      const QString& documentName, int innerLayerCount,
      const FilePath& defaultFilePath, const LengthUnit& lengthUnit,
      const Theme& theme, const QString& settingsPrefix,
      QWidget* parent = nullptr) noexcept;
  ~GraphicsExportDialog() noexcept;

  // General Methods
  void setSaveAsCallback(SaveAsCallback callback) noexcept;
  const QList<Page>& getPages() const noexcept { return mPages; }

  // Operator Overloadings
  GraphicsExportDialog& operator=(const GraphicsExportDialog& rhs) = delete;

signals:  // Signals
  void requestOpenFile(const librepcb::FilePath& filePath);

private:  // Methods
  void loadDefaultSettings() noexcept;
  void syncClientSettings(ClientSettingsAction action) noexcept;
  void buttonBoxClicked(QDialogButtonBox::StandardButton btn) noexcept;
  void printersAvailable() noexcept;
  void printerChanged(int index) noexcept;
  void setAvailablePageSizes(QList<tl::optional<QPageSize>> sizes) noexcept;
  void layerListItemDoubleClicked(QListWidgetItem* item) noexcept;
  void applySettings() noexcept;
  void startExport(bool toClipboard) noexcept;
  void openProgressDialog() noexcept;
  bool eventFilter(QObject* object, QEvent* event) noexcept override;

  // GUI Access Methods
  void setPageSize(const tl::optional<QPageSize::PageSizeId>& size) noexcept;
  tl::optional<QPageSize> getPageSize() const noexcept;
  void setOrientation(
      const tl::optional<QPageLayout::Orientation>& orientation) noexcept;
  tl::optional<QPageLayout::Orientation> getOrientation() const noexcept;
  void setMarginLeft(const UnsignedLength& margin) noexcept;
  UnsignedLength getMarginLeft() const noexcept;
  void setMarginTop(const UnsignedLength& margin) noexcept;
  UnsignedLength getMarginTop() const noexcept;
  void setMarginRight(const UnsignedLength& margin) noexcept;
  UnsignedLength getMarginRight() const noexcept;
  void setMarginBottom(const UnsignedLength& margin) noexcept;
  UnsignedLength getMarginBottom() const noexcept;
  void setShowPinNumbers(bool show) noexcept;
  bool getShowPinNumbers() const noexcept;
  void setRotate(bool rotate) noexcept;
  bool getRotate() const noexcept;
  void setMirror(bool mirror) noexcept;
  bool getMirror() const noexcept;
  void setFitToPage(bool fit) noexcept;
  bool getFitToPage() const noexcept;
  void setScaleFactor(qreal factor) noexcept;
  qreal getScaleFactor() const noexcept;
  void setDpi(int dpi) noexcept;
  int getDpi() const noexcept;
  void setBlackWhite(bool blackWhite) noexcept;
  bool getBlackWhite() const noexcept;
  void setBackgroundColor(Qt::GlobalColor color) noexcept;
  Qt::GlobalColor getBackgroundColor() const noexcept;
  void setMinLineWidth(const UnsignedLength& width) noexcept;
  UnsignedLength getMinLineWidth() const noexcept;
  void setPrinterName(const QString& name) noexcept;
  QString getPrinterName() const noexcept;
  void setDuplex(QPrinter::DuplexMode duplex) noexcept;
  QPrinter::DuplexMode getDuplex() const noexcept;
  void setPageContent(const QList<ContentItem>& items) noexcept;
  const QList<ContentItem>& getPageContent() const noexcept;
  void setOpenExportedFiles(bool open) noexcept;
  bool getOpenExportedFiles() const noexcept;
  void updateColorsListWidget() noexcept;

private:  // Data
  const Mode mMode;
  const Output mOutput;
  const QList<std::shared_ptr<GraphicsPagePainter>> mInputPages;
  const int mCurrentPage;  // Note: Might be out of range!
  const FilePath mDefaultFilePath;
  const Theme& mTheme;
  const QString mSettingsPrefix;
  SaveAsCallback mSaveAsCallback;  // Guaranteed to be not null.

  QScopedPointer<GraphicsExportSettings> mDefaultSettings;
  QList<std::pair<QString, QColor>> mColors;

  QString mSettingsPrinterName;
  tl::optional<QPageSize::PageSizeId> mSettingsPageSize;
  QPrinter::DuplexMode mSettingsDuplexMode;

  bool mDisableApplySettings;
  QScopedPointer<Ui::GraphicsExportDialog> mUi;
  QScopedPointer<QProgressDialog> mProgressDialog;

  QScopedPointer<QFutureWatcher<QList<QPrinterInfo>>> mPrinterWatcher;
  QList<QPrinterInfo> mAvailablePrinters;
  QList<tl::optional<QPageSize>> mAvailablePageSizes;
  QList<ContentItem> mPageContentItems;
  QList<Page> mPages;

  QScopedPointer<GraphicsExport> mPreview;
  QScopedPointer<GraphicsExport> mExport;
  FilePath mPathToOpenAfterExport;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
