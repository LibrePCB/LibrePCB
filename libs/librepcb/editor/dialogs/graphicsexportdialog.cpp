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
#include "graphicsexportdialog.h"

#include "../editorcommandset.h"
#include "../utils/editortoolbox.h"
#include "filedialog.h"
#include "ui_graphicsexportdialog.h"

#include <librepcb/core/export/graphicsexport.h>
#include <librepcb/core/export/graphicsexportsettings.h>
#include <librepcb/core/graphics/graphicslayer.h>
#include <librepcb/core/utils/toolbox.h>

#include <QtConcurrent>
#include <QtCore>
#include <QtPrintSupport>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

GraphicsExportDialog::GraphicsExportDialog(
    Mode mode, Output output,
    const QList<std::shared_ptr<GraphicsPagePainter>>& pages, int currentPage,
    const QString& documentName, int innerLayerCount,
    const FilePath& defaultFilePath, const LengthUnit& lengthUnit,
    const QString& settingsPrefix, QWidget* parent) noexcept
  : QDialog(parent),
    mMode(mode),
    mOutput(output),
    mInputPages(pages),
    mCurrentPage(currentPage),
    mDefaultFilePath(defaultFilePath),
    mSettingsPrefix(settingsPrefix),
    mSaveAsCallback(&FileDialog::getSaveFileName),
    mLayers(),
    mSettingsPrinterName(),
    mSettingsPageSize(tl::nullopt),
    mSettingsDuplexMode(QPrinter::DuplexNone),
    mDisableApplySettings(true),
    mUi(new Ui::GraphicsExportDialog),
    mProgressDialog(new QProgressDialog(tr("Operation in progress..."),
                                        tr("Cancel"), 0, 100, this)),
    mPrinterWatcher(new QFutureWatcher<QList<QPrinterInfo>>(this)),
    mAvailablePrinters(),
    mAvailablePageSizes(),
    mPageContentItems(),
    mPages(),
    mPreview(new GraphicsExport()),
    mExport(new GraphicsExport()),
    mPathToOpenAfterExport() {
  mUi->setupUi(this);
  connect(mUi->buttonBoxLeft, &QDialogButtonBox::clicked, this,
          [this](QAbstractButton* btn) {
            buttonBoxClicked(mUi->buttonBoxLeft->standardButton(btn));
          });
  connect(mUi->buttonBox, &QDialogButtonBox::clicked, this,
          [this](QAbstractButton* btn) {
            buttonBoxClicked(mUi->buttonBox->standardButton(btn));
          });

  // Set window title.
  switch (output) {
    case Output::Image:
      setWindowTitle(tr("Export Image"));
      break;
    case Output::Pdf:
      setWindowTitle(tr("Export PDF"));
      break;
    default:
      setWindowTitle(tr("Print"));
      break;
  }

  // Add all available layers.
  if (mMode == Mode::Schematic) {
    mLayers.append(GraphicsLayer(GraphicsLayer::sSchematicSheetFrames));
    mLayers.append(GraphicsLayer(GraphicsLayer::sSymbolOutlines));
    mLayers.append(GraphicsLayer(GraphicsLayer::sSymbolGrabAreas));
    mLayers.append(GraphicsLayer(GraphicsLayer::sSymbolPinLines));
    mLayers.append(GraphicsLayer(GraphicsLayer::sSymbolPinNames));
    mLayers.append(GraphicsLayer(GraphicsLayer::sSymbolPinNumbers));
    mLayers.append(GraphicsLayer(GraphicsLayer::sSymbolNames));
    mLayers.append(GraphicsLayer(GraphicsLayer::sSymbolValues));
    mLayers.append(GraphicsLayer(GraphicsLayer::sSchematicNetLines));
    mLayers.append(GraphicsLayer(GraphicsLayer::sSchematicNetLabels));
    mLayers.append(GraphicsLayer(GraphicsLayer::sSchematicDocumentation));
    mLayers.append(GraphicsLayer(GraphicsLayer::sSchematicComments));
    mLayers.append(GraphicsLayer(GraphicsLayer::sSchematicGuide));
  } else if (mMode == Mode::Board) {
    mLayers.append(GraphicsLayer(GraphicsLayer::sBoardGuide));
    mLayers.append(GraphicsLayer(GraphicsLayer::sBoardComments));
    mLayers.append(GraphicsLayer(GraphicsLayer::sBoardDocumentation));
    mLayers.append(GraphicsLayer(GraphicsLayer::sBoardAlignment));
    mLayers.append(GraphicsLayer(GraphicsLayer::sBoardMeasures));
    mLayers.append(GraphicsLayer(GraphicsLayer::sBoardSheetFrames));
    mLayers.append(GraphicsLayer(GraphicsLayer::sBoardOutlines));
    mLayers.append(GraphicsLayer(GraphicsLayer::sBoardDrillsNpth));
    mLayers.append(GraphicsLayer(GraphicsLayer::sBoardMillingPth));
    mLayers.append(GraphicsLayer(GraphicsLayer::sBoardPadsTht));
    mLayers.append(GraphicsLayer(GraphicsLayer::sBoardViasTht));
    mLayers.append(GraphicsLayer(GraphicsLayer::sTopDocumentation));
    mLayers.append(GraphicsLayer(GraphicsLayer::sTopNames));
    mLayers.append(GraphicsLayer(GraphicsLayer::sTopValues));
    mLayers.append(GraphicsLayer(GraphicsLayer::sTopCourtyard));
    mLayers.append(GraphicsLayer(GraphicsLayer::sTopGrabAreas));
    mLayers.append(GraphicsLayer(GraphicsLayer::sTopPlacement));
    mLayers.append(GraphicsLayer(GraphicsLayer::sTopGlue));
    mLayers.append(GraphicsLayer(GraphicsLayer::sTopSolderPaste));
    mLayers.append(GraphicsLayer(GraphicsLayer::sTopStopMask));
    mLayers.append(GraphicsLayer(GraphicsLayer::sTopCopper));
    for (int i = 1; i <= innerLayerCount; ++i) {
      mLayers.append(GraphicsLayer(GraphicsLayer::getInnerLayerName(i)));
    }
    mLayers.append(GraphicsLayer(GraphicsLayer::sBotCopper));
    mLayers.append(GraphicsLayer(GraphicsLayer::sBotStopMask));
    mLayers.append(GraphicsLayer(GraphicsLayer::sBotSolderPaste));
    mLayers.append(GraphicsLayer(GraphicsLayer::sBotGlue));
    mLayers.append(GraphicsLayer(GraphicsLayer::sBotPlacement));
    mLayers.append(GraphicsLayer(GraphicsLayer::sBotGrabAreas));
    mLayers.append(GraphicsLayer(GraphicsLayer::sBotCourtyard));
    mLayers.append(GraphicsLayer(GraphicsLayer::sBotValues));
    mLayers.append(GraphicsLayer(GraphicsLayer::sBotNames));
    mLayers.append(GraphicsLayer(GraphicsLayer::sBotDocumentation));
  }

  // Open exported files checkbox.
  if (output == Output::Print) {
    mUi->cbxOpenExportedFiles->hide();
  }

  // Copy to clipboard button.
  if (output == Output::Image) {
    QPushButton* btn = new QPushButton(tr("Copy to clipboard"));
    btn->setObjectName("btnCopyToClipboard");
    btn->setToolTip(
        tr("Copy the image to the clipboard instead of saving it as a file."));
    // Note: Must have AcceptRole to get a reasonable position on all systems.
    mUi->buttonBox->addButton(btn, QDialogButtonBox::AcceptRole);
  }

  // Printer.
  if (output == Output::Print) {
    // Start fetching printers asynchronously since it can take some time.
    mUi->cbxPrinter->addItem(QIcon(":/img/actions/search.png"),
                             tr("Looking for printers..."));
    connect(mPrinterWatcher.data(),
            &QFutureWatcher<QList<QPrinterInfo>>::finished, this,
            &GraphicsExportDialog::printersAvailable);
    mPrinterWatcher->setFuture(
        QtConcurrent::run(&QPrinterInfo::availablePrinters));
  } else {
    EditorToolbox::removeFormLayoutRow(*mUi->lblPrinter);
  }

  // Duplex.
  if (output != Output::Print) {
    EditorToolbox::removeFormLayoutRow(*mUi->lblDuplex);
  }

  // Copies.
  if (output != Output::Print) {
    EditorToolbox::removeFormLayoutRow(*mUi->lblCopies);
  }

  // Page size.
  if ((output == Output::Pdf) || (output == Output::Print)) {
    if (output == Output::Pdf) {
      QList<tl::optional<QPageSize>> sizes = {
          tl::nullopt,  // Auto size.
          QPageSize(QPageSize::A0),
          QPageSize(QPageSize::A1),
          QPageSize(QPageSize::A2),
          QPageSize(QPageSize::A3),
          QPageSize(QPageSize::A4),
          QPageSize(QPageSize::A5),
          QPageSize(QPageSize::A6),
          QPageSize(QPageSize::A7),
          QPageSize(QPageSize::A8),
          QPageSize(QPageSize::A9),
          QPageSize(QPageSize::A10),
          QPageSize(QPageSize::B0),
          QPageSize(QPageSize::B1),
          QPageSize(QPageSize::B2),
          QPageSize(QPageSize::B3),
          QPageSize(QPageSize::B4),
          QPageSize(QPageSize::B5),
          QPageSize(QPageSize::B6),
          QPageSize(QPageSize::B7),
          QPageSize(QPageSize::B8),
          QPageSize(QPageSize::B9),
          QPageSize(QPageSize::B10),
          QPageSize(QPageSize::JisB0),
          QPageSize(QPageSize::JisB1),
          QPageSize(QPageSize::JisB2),
          QPageSize(QPageSize::JisB3),
          QPageSize(QPageSize::JisB4),
          QPageSize(QPageSize::JisB5),
          QPageSize(QPageSize::JisB6),
          QPageSize(QPageSize::JisB7),
          QPageSize(QPageSize::JisB8),
          QPageSize(QPageSize::JisB9),
          QPageSize(QPageSize::JisB10),
          QPageSize(QPageSize::Letter),
          QPageSize(QPageSize::Legal),
          QPageSize(QPageSize::ExecutiveStandard),
          QPageSize(QPageSize::Ledger),
          QPageSize(QPageSize::Tabloid),
          QPageSize(QPageSize::AnsiC),
          QPageSize(QPageSize::AnsiD),
          QPageSize(QPageSize::AnsiE),
      };
      setAvailablePageSizes(sizes);
      setPageSize(QPageSize::A4);
    }
    connect(
        mUi->cbxPageSize,
        static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
        this, &GraphicsExportDialog::applySettings);
  } else {
    EditorToolbox::removeFormLayoutRow(*mUi->lblPageSize);
  }

  // Orientation.
  if ((output == Output::Pdf) || (output == Output::Print)) {
    connect(mUi->rbtnOrientationAuto, &QRadioButton::toggled, this,
            &GraphicsExportDialog::applySettings);
    connect(mUi->rbtnOrientationLandscape, &QRadioButton::toggled, this,
            &GraphicsExportDialog::applySettings);
    connect(mUi->rbtnOrientationPortrait, &QRadioButton::toggled, this,
            &GraphicsExportDialog::applySettings);
  } else {
    EditorToolbox::removeFormLayoutRow(*mUi->lblOrientation);
  }

  // Resolution.
  if (output == Output::Image) {
    connect(mUi->spbxResolutionDpi,
            static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this,
            &GraphicsExportDialog::applySettings);
  } else {
    EditorToolbox::removeFormLayoutRow(*mUi->lblResolution);
  }

  // Margins.
  mUi->edtMarginLeft->configure(lengthUnit, LengthEditBase::Steps::generic(),
                                mSettingsPrefix % "/margin_left");
  mUi->edtMarginTop->configure(lengthUnit, LengthEditBase::Steps::generic(),
                               mSettingsPrefix % "/margin_top");
  mUi->edtMarginRight->configure(lengthUnit, LengthEditBase::Steps::generic(),
                                 mSettingsPrefix % "/margin_right");
  mUi->edtMarginBottom->configure(lengthUnit, LengthEditBase::Steps::generic(),
                                  mSettingsPrefix % "/margin_bottom");
  connect(mUi->edtMarginLeft, &UnsignedLengthEdit::valueChanged, this,
          &GraphicsExportDialog::applySettings);
  connect(mUi->edtMarginTop, &UnsignedLengthEdit::valueChanged, this,
          &GraphicsExportDialog::applySettings);
  connect(mUi->edtMarginRight, &UnsignedLengthEdit::valueChanged, this,
          &GraphicsExportDialog::applySettings);
  connect(mUi->edtMarginBottom, &UnsignedLengthEdit::valueChanged, this,
          &GraphicsExportDialog::applySettings);

  // Rotation.
  connect(mUi->cbxRotate, &QCheckBox::toggled, this,
          &GraphicsExportDialog::applySettings);

  // Mirror,
  connect(mUi->cbxMirror, &QCheckBox::toggled, this,
          &GraphicsExportDialog::applySettings);

  // Scale.
  if ((output == Output::Pdf) || (output == Output::Print)) {
    mUi->spbxScaleFactor->setEnabled(!mUi->cbxScaleAuto->isChecked());
    connect(mUi->cbxScaleAuto, &QCheckBox::toggled, mUi->spbxScaleFactor,
            &DoubleSpinBox::setDisabled);
    connect(mUi->cbxScaleAuto, &QCheckBox::toggled, this,
            &GraphicsExportDialog::applySettings);
    connect(mUi->spbxScaleFactor,
            static_cast<void (DoubleSpinBox::*)(double)>(
                &DoubleSpinBox::valueChanged),
            this, &GraphicsExportDialog::applySettings);
  } else {
    EditorToolbox::removeFormLayoutRow(*mUi->lblScale);
  }

  // Min. line width.
  mUi->edtMinLineWidth->configure(lengthUnit, LengthEditBase::Steps::generic(),
                                  mSettingsPrefix % "/min_line_width");
  connect(mUi->edtMinLineWidth, &UnsignedLengthEdit::valueChanged, this,
          &GraphicsExportDialog::applySettings);

  // Black/white.
#if (QT_VERSION >= QT_VERSION_CHECK(5, 15, 0))
  auto setTabColorsHidden = [this](bool hidden) {
    mUi->tabWidget->setTabVisible(mUi->tabWidget->indexOf(mUi->tabColors),
                                  !hidden);
    // Also disable/enable the tab to work around a bug:
    // https://bugreports.qt.io/browse/QTBUG-101219
    mUi->tabWidget->setTabEnabled(mUi->tabWidget->indexOf(mUi->tabColors),
                                  !hidden);
  };
  setTabColorsHidden(mUi->cbxBlackWhite->isChecked());
  connect(mUi->cbxBlackWhite, &QCheckBox::toggled, this, setTabColorsHidden);
#endif
  connect(mUi->cbxBlackWhite, &QCheckBox::toggled, this,
          &GraphicsExportDialog::applySettings);

  // Background color.
  if (output != Output::Print) {
    connect(mUi->rbtnBackgroundNone, &QRadioButton::toggled, this,
            &GraphicsExportDialog::applySettings);
    connect(mUi->rbtnBackgroundWhite, &QRadioButton::toggled, this,
            &GraphicsExportDialog::applySettings);
    connect(mUi->rbtnBackgroundBlack, &QRadioButton::toggled, this,
            &GraphicsExportDialog::applySettings);
  } else {
    EditorToolbox::removeFormLayoutRow(*mUi->lblBackground);
  }

  // Layer colors.
  connect(mUi->lstLayerColors, &QListWidget::itemDoubleClicked, this,
          &GraphicsExportDialog::layerListItemDoubleClicked);
  connect(mUi->lstLayerColors, &QListWidget::itemChanged, this,
          &GraphicsExportDialog::applySettings);

  // Content.
  if (mMode == Mode::Board) {
    EditorCommandSet& cmd = EditorCommandSet::instance();

    QAction* aNew = cmd.itemNew.createAction(
        this, this,
        [this]() {
          QList<ContentItem> items = getPageContent();
          items.append(ContentItem{"", false, false, {}});
          setPageContent(items);
          if (auto item = mUi->treeContent->topLevelItem(items.count() - 1)) {
            mUi->treeContent->editItem(item, 0);
          }
        },
        EditorCommand::ActionFlag::WidgetShortcut);
    mUi->treeContent->addAction(aNew);

    QAction* aRemove = cmd.remove.createAction(
        this, this,
        [this]() {
          QModelIndexList indices =
              mUi->treeContent->selectionModel()->selectedIndexes();
          if ((!indices.isEmpty()) && indices.first().isValid() &&
              (!indices.first().parent().isValid()) &&
              (indices.first().row() >= 0) &&
              (indices.first().row() < mPageContentItems.count())) {
            QList<ContentItem> items = getPageContent();
            items.removeAt(indices.first().row());
            setPageContent(items);
          }
        },
        EditorCommand::ActionFlag::WidgetShortcut);
    mUi->treeContent->addAction(aRemove);

    QAction* aRename = cmd.rename.createAction(
        this, this,
        [this]() {
          foreach (auto item, mUi->treeContent->selectedItems()) {
            mUi->treeContent->editItem(item, 0);
          }
        },
        EditorCommand::ActionFlag::WidgetShortcut);
    mUi->treeContent->addAction(aRename);

    mUi->treeContent->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    mUi->treeContent->header()->setSectionResizeMode(
        1, QHeaderView::ResizeToContents);
    mUi->treeContent->viewport()->installEventFilter(this);
    connect(mUi->treeContent, &QTreeWidget::itemChanged, this,
            &GraphicsExportDialog::applySettings);
  } else {
    mUi->tabWidget->removeTab(mUi->tabWidget->indexOf(mUi->tabContent));
  }

  // Page range.
  if (mode == Mode::Schematic) {
    mUi->edtPageRange->setPlaceholderText(
        QString("%1-%2").arg(1).arg(mInputPages.count()));
    mUi->edtPageRange->setEnabled(mUi->rbtnRangeCustom->isChecked());
    connect(mUi->rbtnRangeAll, &QRadioButton::toggled, this,
            &GraphicsExportDialog::applySettings);
    connect(mUi->rbtnRangeCurrent, &QRadioButton::toggled, this,
            &GraphicsExportDialog::applySettings);
    connect(mUi->rbtnRangeCustom, &QRadioButton::toggled, this,
            &GraphicsExportDialog::applySettings);
    connect(mUi->rbtnRangeCustom, &QRadioButton::toggled, mUi->edtPageRange,
            &QLineEdit::setEnabled);
    connect(mUi->edtPageRange, &QLineEdit::textChanged, this,
            &GraphicsExportDialog::applySettings);
  } else {
    mUi->tabWidget->removeTab(mUi->tabWidget->indexOf(mUi->tabPages));
  }

  // Select first tab.
  mUi->tabWidget->setCurrentIndex(0);

  // Setup preview.
  mUi->previewWidget->setShowPageNumbers(pages.count() > 1);
  mUi->previewWidget->setShowResolution(output == Output::Image);
  connect(mPreview.data(), &GraphicsExport::previewReady, mUi->previewWidget,
          &GraphicsExportWidget::setPageContent);

  // Setup export.
  mExport->setDocumentName(documentName);
  connect(mExport.data(), &GraphicsExport::succeeded, this,
          [this]() {
            const bool canceled = mProgressDialog->wasCanceled();
            mProgressDialog->reset();
            if (!canceled) {
              if (mPathToOpenAfterExport.isValid()) {
                emit requestOpenFile(mPathToOpenAfterExport);
              }
              close();
            }
          },
          Qt::QueuedConnection);
  connect(mExport.data(), &GraphicsExport::failed, this,
          [this](const QString& msg) {
            mProgressDialog->reset();
            QMessageBox::critical(this, tr("Error"), msg);
          },
          Qt::QueuedConnection);

  // Setup progress dialog.
  mProgressDialog->reset();
  mProgressDialog->setWindowModality(Qt::WindowModal);
  mProgressDialog->setMinimumDuration(0);
  mProgressDialog->setAutoReset(false);
  connect(mProgressDialog.data(), &QProgressDialog::canceled, mExport.data(),
          &GraphicsExport::cancel);
  connect(mExport.data(), &GraphicsExport::progress, mProgressDialog.data(),
          [this](int percent, int page, int total) {
            if (mProgressDialog->isVisible()) {
              mProgressDialog->setLabelText(
                  tr("Processing page %1 of %2...").arg(page).arg(total));
              mProgressDialog->setValue(percent);
            }
          },
          Qt::QueuedConnection);

  // Load settings.
  loadDefaultSettings();
  syncClientSettings(ClientSettingsAction::Load);

  // Apply settings & update preview.
  mDisableApplySettings = false;
  applySettings();
}

GraphicsExportDialog::~GraphicsExportDialog() noexcept {
  syncClientSettings(ClientSettingsAction::Store);
  mDisableApplySettings = true;
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void GraphicsExportDialog::setSaveAsCallback(SaveAsCallback callback) noexcept {
  Q_ASSERT(callback);
  mSaveAsCallback = callback;
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void GraphicsExportDialog::loadDefaultSettings() noexcept {
  GraphicsExportSettings defaultSettings;

  setPageSize(QPageSize::A4);
  setOrientation(defaultSettings.getOrientation());
  setMarginLeft(defaultSettings.getMarginLeft());
  setMarginTop(defaultSettings.getMarginTop());
  setMarginRight(defaultSettings.getMarginRight());
  setMarginBottom(defaultSettings.getMarginBottom());
  setRotate(defaultSettings.getRotate());
  setMirror(defaultSettings.getMirror());
  setFitToPage(!defaultSettings.getScale().has_value());
  setScaleFactor(defaultSettings.getScale() ? *defaultSettings.getScale() : 1);
  setDpi(defaultSettings.getPixmapDpi());
  setBlackWhite(defaultSettings.getBlackWhite());
  setBackgroundColor(defaultSettings.getBackgroundColor());
  setMinLineWidth(defaultSettings.getMinLineWidth());
  setDuplex(QPrinter::DuplexNone);
  setOpenExportedFiles(true);
  mUi->rbtnRangeAll->setChecked(true);

  // Layer colors.
  QHash<QString, QColor> defaultColors;
  for (const std::pair<QString, QColor>& pair : defaultSettings.getLayers()) {
    defaultColors.insert(pair.first, pair.second);
  }
  for (GraphicsLayer& layer : mLayers) {
    QColor fallback = GraphicsLayer(layer.getName()).getColor();
    layer.setColor(defaultColors.value(layer.getName(), fallback));
  }
  updateLayerColorsListWidget();

  // Page content.
  QSet<QString> allLayers;
  foreach (const GraphicsLayer& layer, mLayers) {
    allLayers.insert(layer.getName());
  }
  QSet<QString> commonLayers = {
      GraphicsLayer::sBoardMeasures,
      GraphicsLayer::sBoardSheetFrames,
      GraphicsLayer::sBoardOutlines,
  };
  QSet<QString> cutOutLayers = {
      GraphicsLayer::sBoardDrillsNpth,
      GraphicsLayer::sBoardMillingPth,
  };
  QSet<QString> assemblyLayers = {
      GraphicsLayer::sBoardGuide,
      GraphicsLayer::sBoardComments,
      GraphicsLayer::sBoardDocumentation,
  };
  QList<ContentItem> items = {
      {
          tr("All Layers"),
          (mOutput == Output::Image),  // Imege export -> single page.
          false,
          allLayers,
      },
      {
          tr("Assembly Top"),
          (mOutput != Output::Image),  // Multi-page export.
          false,
          commonLayers + cutOutLayers + assemblyLayers +
              QSet<QString>{
                  GraphicsLayer::sTopDocumentation,
                  GraphicsLayer::sTopNames,
                  GraphicsLayer::sTopValues,
                  GraphicsLayer::sTopGrabAreas,
                  GraphicsLayer::sTopPlacement,
                  GraphicsLayer::sTopSolderPaste,
                  GraphicsLayer::sTopStopMask,
              },
      },
      {
          tr("Assembly Bottom"),
          (mOutput != Output::Image),  // Multi-page export.
          true,
          commonLayers + cutOutLayers + assemblyLayers +
              QSet<QString>{
                  GraphicsLayer::sBotDocumentation,
                  GraphicsLayer::sBotNames,
                  GraphicsLayer::sBotValues,
                  GraphicsLayer::sBotGrabAreas,
                  GraphicsLayer::sBotPlacement,
                  GraphicsLayer::sBotSolderPaste,
                  GraphicsLayer::sBotStopMask,
              },
      },
      {
          tr("Drills"),
          false,
          false,
          commonLayers + cutOutLayers +
              QSet<QString>{
                  GraphicsLayer::sBoardPadsTht,
                  GraphicsLayer::sBoardViasTht,
              },
      },
      {
          tr("Copper Top"),
          false,
          false,
          commonLayers +
              QSet<QString>{
                  GraphicsLayer::sBoardPadsTht,
                  GraphicsLayer::sBoardViasTht,
                  GraphicsLayer::sTopCopper,
              },
      },
      {
          tr("Copper Bottom"),
          false,
          false,
          commonLayers +
              QSet<QString>{
                  GraphicsLayer::sBoardPadsTht,
                  GraphicsLayer::sBoardViasTht,
                  GraphicsLayer::sBotCopper,
              },
      },
  };
  setPageContent(items);
}

void GraphicsExportDialog::syncClientSettings(
    ClientSettingsAction action) noexcept {
  try {
    QSettings s;

    // Window size.
    if (action == ClientSettingsAction::Store) {
      s.setValue(mSettingsPrefix % "/window_size", size());
    } else {
      QSize value = s.value(mSettingsPrefix % "/window_size").toSize();
      if (!value.isEmpty()) {
        resize(value);
      }
    }

    // Printer name.
    if (action == ClientSettingsAction::Store) {
      QString value = getPrinterName();
      if (!value.isEmpty()) {
        s.setValue(mSettingsPrefix % "/printer_name", value);
      }
    } else {
      QString value = s.value(mSettingsPrefix % "/printer_name").toString();
      if (!value.isEmpty()) {
        // WIll be applied later when printers are available.
        mSettingsPrinterName = value;
      }
    }

    // Duplex.
    QHash<QString, QPrinter::DuplexMode> duplexModeMap = {
        {"none", QPrinter::DuplexNone},
        {"long_edge", QPrinter::DuplexLongSide},
        {"short_edge", QPrinter::DuplexShortSide},
    };
    if (action == ClientSettingsAction::Store) {
      if (mUi->cbxDuplex->count()) {
        s.setValue(mSettingsPrefix % "/duplex", duplexModeMap.key(getDuplex()));
      }
    } else {
      QString value = s.value(mSettingsPrefix % "/duplex").toString();
      if (duplexModeMap.contains(value)) {
        // WIll be applied later when printers are available.
        mSettingsDuplexMode = duplexModeMap.value(value);
      }
    }

    // Page size.
    if (action == ClientSettingsAction::Store) {
      if (mUi->cbxPageSize->count()) {
        auto value = getPageSize();
        s.setValue(mSettingsPrefix % "/page_size",
                   value ? value->key() : "auto");
      }
    } else {
      QString value = s.value(mSettingsPrefix % "/page_size").toString();
      if (value == "auto") {
        mSettingsPageSize = tl::nullopt;
        setPageSize(tl::nullopt);
      } else if (!value.isEmpty()) {
        for (int i = 0; i < QPageSize::LastPageSize; ++i) {
          QPageSize::PageSizeId id = static_cast<QPageSize::PageSizeId>(i);
          if (QPageSize::key(id) == value) {
            mSettingsPageSize = id;
            setPageSize(id);
            break;
          }
        }
      }
    }

    // Orientation.
    QHash<QString, tl::optional<QPageLayout::Orientation>> orientationMap = {
        {"auto", tl::nullopt},
        {"landscape", QPageLayout::Landscape},
        {"portrait", QPageLayout::Portrait},
    };
    if (action == ClientSettingsAction::Store) {
      s.setValue(mSettingsPrefix % "/orientation",
                 orientationMap.key(getOrientation()));
    } else {
      QString value = s.value(mSettingsPrefix % "/orientation").toString();
      if (orientationMap.contains(value)) {
        setOrientation(orientationMap.value(value));
      }
    }

    // Margins.
    if (action == ClientSettingsAction::Store) {
      s.setValue(mSettingsPrefix % "/margin_left",
                 getMarginLeft()->toMmString());
      s.setValue(mSettingsPrefix % "/margin_top", getMarginTop()->toMmString());
      s.setValue(mSettingsPrefix % "/margin_right",
                 getMarginRight()->toMmString());
      s.setValue(mSettingsPrefix % "/margin_bottom",
                 getMarginBottom()->toMmString());
    } else {
      QString value = s.value(mSettingsPrefix % "/margin_left").toString();
      if (!value.isEmpty()) {
        setMarginLeft(UnsignedLength(Length::fromMm(value)));
      }
      value = s.value(mSettingsPrefix % "/margin_top").toString();
      if (!value.isEmpty()) {
        setMarginTop(UnsignedLength(Length::fromMm(value)));
      }
      value = s.value(mSettingsPrefix % "/margin_right").toString();
      if (!value.isEmpty()) {
        setMarginRight(UnsignedLength(Length::fromMm(value)));
      }
      value = s.value(mSettingsPrefix % "/margin_bottom").toString();
      if (!value.isEmpty()) {
        setMarginBottom(UnsignedLength(Length::fromMm(value)));
      }
    }

    // Rotate.
    if (action == ClientSettingsAction::Store) {
      s.setValue(mSettingsPrefix % "/rotate", getRotate());
    } else {
      QVariant value = s.value(mSettingsPrefix % "/rotate");
      if (!value.isNull()) {
        setRotate(value.toBool());
      }
    }

    // Mirror.
    if (action == ClientSettingsAction::Store) {
      s.setValue(mSettingsPrefix % "/mirror", getMirror());
    } else {
      QVariant value = s.value(mSettingsPrefix % "/mirror");
      if (!value.isNull()) {
        setMirror(value.toBool());
      }
    }

    // Fit to page.
    if (action == ClientSettingsAction::Store) {
      s.setValue(mSettingsPrefix % "/fit_to_page", getFitToPage());
    } else {
      QVariant value = s.value(mSettingsPrefix % "/fit_to_page");
      if (!value.isNull()) {
        setFitToPage(value.toBool());
      }
    }

    // Scale factor.
    if (action == ClientSettingsAction::Store) {
      s.setValue(mSettingsPrefix % "/scale_factor", getScaleFactor());
    } else {
      QVariant value = s.value(mSettingsPrefix % "/scale_factor");
      if ((!value.isNull()) && (value.value<qreal>() > 0)) {
        setScaleFactor(value.value<qreal>());
      }
    }

    // DPI.
    if (action == ClientSettingsAction::Store) {
      s.setValue(mSettingsPrefix % "/dpi", getDpi());
    } else {
      QVariant value = s.value(mSettingsPrefix % "/dpi");
      if ((!value.isNull()) && (value.toInt() > 0)) {
        setDpi(value.toInt());
      }
    }

    // Black/white.
    if (action == ClientSettingsAction::Store) {
      s.setValue(mSettingsPrefix % "/black_white", getBlackWhite());
    } else {
      QVariant value = s.value(mSettingsPrefix % "/black_white");
      if (!value.isNull()) {
        setBlackWhite(value.toBool());
      }
    }

    // Background color.
    if (action == ClientSettingsAction::Store) {
      s.setValue(
          mSettingsPrefix % "/background_color",
          QMetaEnum::fromType<Qt::GlobalColor>().key(getBackgroundColor()));
    } else {
      QVariant value = s.value(mSettingsPrefix % "/background_color");
      if (!value.isNull()) {
        setBackgroundColor(value.value<Qt::GlobalColor>());
      }
    }

    // Min. line width.
    if (action == ClientSettingsAction::Store) {
      s.setValue(mSettingsPrefix % "/min_line_width",
                 getMinLineWidth()->toMmString());
    } else {
      QString value = s.value(mSettingsPrefix % "/min_line_width").toString();
      if (!value.isEmpty()) {
        setMinLineWidth(UnsignedLength(Length::fromMm(value)));
      }
    }

    // Open exported files.
    if (action == ClientSettingsAction::Store) {
      s.setValue(mSettingsPrefix % "/open_exported_files",
                 getOpenExportedFiles());
    } else {
      QVariant value = s.value(mSettingsPrefix % "/open_exported_files");
      if (!value.isNull()) {
        setOpenExportedFiles(value.toBool());
      }
    }

    // Layer colors.
    if (action == ClientSettingsAction::Store) {
      foreach (const GraphicsLayer& layer, mLayers) {
        s.setValue(mSettingsPrefix % "/color/" % layer.getName(),
                   layer.getColor().name(QColor::HexArgb));
      }
    } else {
      for (GraphicsLayer& layer : mLayers) {
        QColor value(
            s.value(mSettingsPrefix % "/color/" % layer.getName()).toString());
        if (value.isValid()) {
          layer.setColor(value);
        }
      }
      updateLayerColorsListWidget();
    }

    // Page content items.
    if (action == ClientSettingsAction::Store) {
      s.beginWriteArray(mSettingsPrefix % "/page_content");
      for (int i = 0; i < mPageContentItems.count(); ++i) {
        s.setArrayIndex(i);
        s.setValue("name", mPageContentItems.at(i).name);
        s.setValue("enabled", mPageContentItems.at(i).enabled);
        s.setValue("mirror", mPageContentItems.at(i).mirror);
        s.setValue("layers",
                   QVariant::fromValue(QStringList(
                       Toolbox::sortedQSet(mPageContentItems.at(i).layers))));
      }
      s.endArray();
    } else {
      QList<ContentItem> items;
      int count = s.beginReadArray(mSettingsPrefix % "/page_content");
      for (int i = 0; i < count; ++i) {
        s.setArrayIndex(i);
        items.append(ContentItem{
            s.value("name", QString::number(i + 1)).toString(),
            s.value("enabled").toBool(),
            s.value("mirror").toBool(),
            Toolbox::toSet(s.value("layers").toStringList()),
        });
      }
      s.endArray();
      if (!items.isEmpty()) {
        setPageContent(items);
      }
    }
  } catch (const Exception& e) {
    qCritical() << "Failed to sync graphics export dialog client settings:"
                << e.getMsg();
  }
}

void GraphicsExportDialog::buttonBoxClicked(
    QDialogButtonBox::StandardButton btn) noexcept {
  switch (btn) {
    case QDialogButtonBox::Cancel: {
      reject();
      break;
    }

    case QDialogButtonBox::RestoreDefaults: {
      mDisableApplySettings = true;
      loadDefaultSettings();
      mDisableApplySettings = false;
      applySettings();
      break;
    }

    case QDialogButtonBox::Ok: {
      startExport(false);  // Print or export to file.
      break;
    }

    default: {
      startExport(true);  // Copy to clipboard.
      break;
    }
  }
}

void GraphicsExportDialog::printersAvailable() noexcept {
  mUi->cbxPrinter->clear();
  mAvailablePrinters = mPrinterWatcher->result();
  if (mAvailablePrinters.isEmpty()) {
    mUi->cbxPrinter->addItem(QIcon(":/img/status/dialog_warning.png"),
                             tr("No printer found"));
    return;
  }

  int selectedIndex = -1;
  foreach (const QPrinterInfo& info, mAvailablePrinters) {
    if ((info.printerName() == mSettingsPrinterName) ||
        (info.isDefault() && (selectedIndex < 0))) {
      selectedIndex = mUi->cbxPrinter->count();
    }
    QString name = info.printerName();
    if (!info.location().isEmpty()) {
      name += " (" % info.location() % ")";
    }
    mUi->cbxPrinter->addItem(QIcon(":/img/actions/print.png"),
                             info.printerName());
    mUi->cbxPrinter->setItemData(
        mUi->cbxPrinter->count() - 1,
        (info.description() + "\n" + info.makeAndModel()).trimmed(),
        Qt::ToolTipRole);
  }
  mUi->cbxPrinter->setCurrentIndex(qMax(selectedIndex, 0));
  printerChanged(mUi->cbxPrinter->currentIndex());
  connect(
      mUi->cbxPrinter,
      static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
      this, &GraphicsExportDialog::printerChanged);
  mUi->spbxCopies->setEnabled(true);
}

void GraphicsExportDialog::printerChanged(int index) noexcept {
  QPrinterInfo printer = mAvailablePrinters.value(index);

  bool wasEmpty = mAvailablePageSizes.isEmpty();
  QList<tl::optional<QPageSize>> sizes;
  foreach (const auto& size, printer.supportedPageSizes()) {
    sizes.append(size);
  }
  setAvailablePageSizes(sizes);
  if (wasEmpty) {
    if (mSettingsPageSize) {
      setPageSize(*mSettingsPageSize);
    } else {
      setPageSize(printer.defaultPageSize().id());
    }
  }

  QPrinter::DuplexMode selectedDuplexMode =
      (mUi->cbxDuplex->count()) ? getDuplex() : mSettingsDuplexMode;
  QList<QPrinter::DuplexMode> duplexModes = printer.supportedDuplexModes();
  mUi->cbxDuplex->clear();
  mUi->cbxDuplex->addItem(tr("One Sided"),
                          static_cast<int>(QPrinter::DuplexNone));
  if (duplexModes.contains(QPrinter::DuplexLongSide)) {
    mUi->cbxDuplex->addItem(tr("Long Edge"),
                            static_cast<int>(QPrinter::DuplexLongSide));
  }
  if (duplexModes.contains(QPrinter::DuplexShortSide)) {
    mUi->cbxDuplex->addItem(tr("Short Edge"),
                            static_cast<int>(QPrinter::DuplexShortSide));
  }
  setDuplex(selectedDuplexMode);
  mUi->cbxDuplex->setEnabled(mUi->cbxDuplex->count() > 1);

  applySettings();
}

void GraphicsExportDialog::setAvailablePageSizes(
    QList<tl::optional<QPageSize>> sizes) noexcept {
  QCollator collator;
  collator.setCaseSensitivity(Qt::CaseInsensitive);
  collator.setIgnorePunctuation(false);
  collator.setNumericMode(true);
  std::sort(sizes.begin(), sizes.end(),
            [&collator](const tl::optional<QPageSize>& lhs,
                        const tl::optional<QPageSize>& rhs) {
              if (lhs && rhs) {
                return collator(lhs->name(), rhs->name());
              } else {
                return lhs.has_value() < rhs.has_value();
              }
            });

  tl::optional<QPageSize> selectedSize = getPageSize();
  mAvailablePageSizes = sizes;
  mUi->cbxPageSize->clear();
  foreach (const tl::optional<QPageSize>& size, sizes) {
    mUi->cbxPageSize->addItem(size ? size->name()
                                   : tr("Custom (adjust to content)"));
  }
  setPageSize(selectedSize ? tl::make_optional(selectedSize->id())
                           : tl::nullopt);
}

void GraphicsExportDialog::layerListItemDoubleClicked(
    QListWidgetItem* item) noexcept {
  Q_ASSERT(item);
  QColor color = item->data(Qt::DecorationRole).value<QColor>();
  color = QColorDialog::getColor(color, this, QString(),
                                 QColorDialog::ShowAlphaChannel);
  if (color.isValid()) {
    item->setData(Qt::DecorationRole, color);
  }
}

void GraphicsExportDialog::applySettings() noexcept {
  if (mDisableApplySettings) {
    return;
  }

  bool isValid = true;

  // Check printer name.
  if (getPrinterName().isEmpty() && (mOutput == Output::Print)) {
    isValid = false;
  }

  // Build settings.
  std::shared_ptr<GraphicsExportSettings> settings =
      std::make_shared<GraphicsExportSettings>();
  settings->setPageSize(getPageSize());
  settings->setPixmapDpi(getDpi());
  settings->setOrientation(getOrientation());
  settings->setMarginLeft(getMarginLeft());
  settings->setMarginTop(getMarginTop());
  settings->setMarginRight(getMarginRight());
  settings->setMarginBottom(getMarginBottom());
  settings->setRotate(getRotate());
  settings->setMirror(getMirror());
  settings->setScale(getFitToPage() ? tl::nullopt
                                    : tl::make_optional(getScaleFactor()));
  settings->setMinLineWidth(getMinLineWidth());
  settings->setBlackWhite(getBlackWhite());
  settings->setBackgroundColor(getBackgroundColor());

  // Update layer colors from list widget.
  for (int i = 0; i < qMin(mUi->lstLayerColors->count(), mLayers.count());
       ++i) {
    QColor color =
        mUi->lstLayerColors->item(i)->data(Qt::DecorationRole).value<QColor>();
    mLayers[i].setColor(color);
  }

  // Update page content from tree view.
  for (int i = 0; i < std::min(mUi->treeContent->topLevelItemCount(),
                               mPageContentItems.count());
       ++i) {
    const QTreeWidgetItem* node = mUi->treeContent->topLevelItem(i);
    ContentItem& item = mPageContentItems[i];
    item.name = node->text(0);
    item.enabled = (node->checkState(0) == Qt::Checked);
    item.mirror = (node->checkState(1) == Qt::Checked);
    for (int k = 0; k < std::min(node->childCount(), mLayers.count()); ++k) {
      const QTreeWidgetItem* child = node->child(k);
      const QString layer = mLayers.at(k).getName();
      if (child->checkState(0) == Qt::Checked) {
        item.layers.insert(layer);
      } else {
        item.layers.remove(layer);
      }
    }
  }

  // Build pages.
  mPages.clear();
  if (mMode == Mode::Schematic) {
    QList<int> pageIndices;
    if (mUi->rbtnRangeCustom->isChecked()) {
      QStringList ranges =
          mUi->edtPageRange->text().split(',', QString::SkipEmptyParts);
      foreach (const QString& range, ranges) {
        int start = qBound(1, range.split('-').first().trimmed().toInt(),
                           mInputPages.count());
        int end = qBound(1, range.split('-').last().trimmed().toInt(),
                         mInputPages.count());
        for (int i = start; i <= end; ++i) {
          pageIndices.append(i - 1);
        }
      }
    } else if (mUi->rbtnRangeCurrent->isChecked() && (mCurrentPage >= 0) &&
               (mCurrentPage < mInputPages.count())) {
      pageIndices.append(mCurrentPage);
    }
    if (pageIndices.isEmpty()) {
      for (int i = 0; i < mInputPages.count(); ++i) {
        pageIndices.append(i);
      }
    }
    QList<std::pair<QString, QColor>> layers;
    foreach (const GraphicsLayer& layer, mLayers) {
      layers.append(std::make_pair(layer.getName(), layer.getColor()));
    }
    settings->setLayers(layers);
    foreach (int i, pageIndices) {
      mPages.append(std::make_pair(mInputPages.at(i), settings));
    }
  } else if ((mMode == Mode::Board) && (mInputPages.count() == 1)) {
    foreach (const ContentItem& item, getPageContent()) {
      if (item.enabled) {
        QList<std::pair<QString, QColor>> layers;
        foreach (const GraphicsLayer& layer, mLayers) {
          if (item.layers.contains(layer.getName())) {
            layers.append(std::make_pair(layer.getName(), layer.getColor()));
          }
        }
        std::shared_ptr<GraphicsExportSettings> pageSettings =
            std::make_shared<GraphicsExportSettings>(*settings);
        pageSettings->setMirror(settings->getMirror() ^ item.mirror);
        pageSettings->setLayers(layers);
        mPages.append(std::make_pair(mInputPages.at(0), pageSettings));
      }
    }
  }
  if (mPages.isEmpty()) {
    isValid = false;
  }

  // Update UI.
  mUi->previewWidget->setNumberOfPages(mPages.count());
  mUi->lblNoteMultiplePagesSuffix->setVisible((mPages.count() > 1) &&
                                              (mOutput == Output::Image));
  if (QAbstractButton* btn = mUi->buttonBox->button(QDialogButtonBox::Ok)) {
    btn->setEnabled(isValid);
  }

  // Update preview.
  mPreview->startPreview(mPages);
}

void GraphicsExportDialog::startExport(bool toClipboard) noexcept {
  mPathToOpenAfterExport = FilePath();

  if (mOutput == Output::Print) {
    int copies = mUi->spbxCopies->value();
    openProgressDialog();
    mExport->startPrint(mPages, getPrinterName(), getDuplex(), copies);
  } else if (toClipboard) {
    // Copy to clipboard only makes sense for a single page. For that, we use
    // the "current page" index as passed to the constructor.
    auto pages = mPages;
    if ((pages.count() > 1) && (mCurrentPage >= 0) &&
        (mCurrentPage < pages.count())) {
      pages = {pages.at(mCurrentPage)};
    }
    openProgressDialog();
    mExport->startExport(pages, FilePath());
  } else {
    const bool isPdf = (mOutput == Output::Pdf);
    const QString defaultExtension = isPdf ? "pdf" : "png";
    const QStringList extensions = isPdf
        ? QStringList{"pdf"}
        : GraphicsExport::getSupportedImageExtensions();
    QString extensionsStr;
    foreach (const QString& ext, extensions) {
      extensionsStr += "*." % ext % " ";
    }
    static QHash<QString, QString> sUsedFilePaths;
    const QString key = mDefaultFilePath.toStr() % "." % defaultExtension;
    QString defaultPath = sUsedFilePaths.value(key, key);
    FilePath fp = FilePath(mSaveAsCallback(this, tr("Save as..."), defaultPath,
                                           extensionsStr, nullptr,
                                           QFileDialog::Options()));
    if (!fp.isValid()) {
      return;
    }
    if (!extensions.contains(fp.getSuffix().toLower().toUtf8())) {
      fp.setPath(fp.toStr() % "." % defaultExtension);
    }
    if (fp.toStr() == key) {
      sUsedFilePaths.remove(key);
    } else {
      sUsedFilePaths[key] = fp.toStr();
    }
    if (getOpenExportedFiles()) {
      if (isPdf) {
        mPathToOpenAfterExport = fp;
      } else {
        mPathToOpenAfterExport = fp.getParentDir();
      }
    }
    openProgressDialog();
    mExport->startExport(mPages, fp);
  }
}

void GraphicsExportDialog::openProgressDialog() noexcept {
  mProgressDialog->setValue(0);  // Auto-opens the dialog.
}

bool GraphicsExportDialog::eventFilter(QObject* object,
                                       QEvent* event) noexcept {
  if (object == mUi->treeContent->viewport()) {
    switch (event->type()) {
      case QEvent::Drop: {
        // The view will change the order of items, so we have to schedule
        // an update of our settings to apply this reordering.
        applySettings();
        break;
      }
      default:
        break;
    }
  }
  return QDialog::eventFilter(object, event);
}

/*******************************************************************************
 *  GUI Access Methods
 ******************************************************************************/

void GraphicsExportDialog::setPageSize(
    const tl::optional<QPageSize::PageSizeId>& size) noexcept {
  for (int i = 0; i < mAvailablePageSizes.count(); ++i) {
    tl::optional<QPageSize> value = mAvailablePageSizes.at(i);
    if (((!size) && (!value)) || (size && value && size == value->id())) {
      mUi->cbxPageSize->setCurrentIndex(i);
      return;
    }
  }
}

tl::optional<QPageSize> GraphicsExportDialog::getPageSize() const noexcept {
  return mAvailablePageSizes.value(mUi->cbxPageSize->currentIndex());
}

void GraphicsExportDialog::setOrientation(
    const tl::optional<QPageLayout::Orientation>& orientation) noexcept {
  if (orientation == QPageLayout::Landscape) {
    mUi->rbtnOrientationLandscape->setChecked(true);
  } else if (orientation == QPageLayout::Portrait) {
    mUi->rbtnOrientationPortrait->setChecked(true);
  } else {
    mUi->rbtnOrientationAuto->setChecked(true);
  }
}

tl::optional<QPageLayout::Orientation> GraphicsExportDialog::getOrientation()
    const noexcept {
  if (mUi->rbtnOrientationLandscape->isChecked()) {
    return QPageLayout::Landscape;
  } else if (mUi->rbtnOrientationPortrait->isChecked()) {
    return QPageLayout::Portrait;
  } else {
    return tl::nullopt;
  }
}

void GraphicsExportDialog::setMarginLeft(
    const UnsignedLength& margin) noexcept {
  mUi->edtMarginLeft->setValue(margin);
}

UnsignedLength GraphicsExportDialog::getMarginLeft() const noexcept {
  return mUi->edtMarginLeft->getValue();
}

void GraphicsExportDialog::setMarginTop(const UnsignedLength& margin) noexcept {
  mUi->edtMarginTop->setValue(margin);
}

UnsignedLength GraphicsExportDialog::getMarginTop() const noexcept {
  return mUi->edtMarginTop->getValue();
}

void GraphicsExportDialog::setMarginRight(
    const UnsignedLength& margin) noexcept {
  mUi->edtMarginRight->setValue(margin);
}

UnsignedLength GraphicsExportDialog::getMarginRight() const noexcept {
  return mUi->edtMarginRight->getValue();
}

void GraphicsExportDialog::setMarginBottom(
    const UnsignedLength& margin) noexcept {
  mUi->edtMarginBottom->setValue(margin);
}

UnsignedLength GraphicsExportDialog::getMarginBottom() const noexcept {
  return mUi->edtMarginBottom->getValue();
}

void GraphicsExportDialog::setRotate(bool rotate) noexcept {
  mUi->cbxRotate->setChecked(rotate);
}

bool GraphicsExportDialog::getRotate() const noexcept {
  return mUi->cbxRotate->isChecked();
}

void GraphicsExportDialog::setMirror(bool mirror) noexcept {
  mUi->cbxMirror->setChecked(mirror);
}

bool GraphicsExportDialog::getMirror() const noexcept {
  return mUi->cbxMirror->isChecked();
}

void GraphicsExportDialog::setFitToPage(bool fit) noexcept {
  mUi->cbxScaleAuto->setChecked(fit);
}

bool GraphicsExportDialog::getFitToPage() const noexcept {
  return mUi->cbxScaleAuto->isChecked();
}

void GraphicsExportDialog::setScaleFactor(qreal factor) noexcept {
  mUi->spbxScaleFactor->setValue(factor);
}

qreal GraphicsExportDialog::getScaleFactor() const noexcept {
  return mUi->spbxScaleFactor->value();
}

void GraphicsExportDialog::setDpi(int dpi) noexcept {
  mUi->spbxResolutionDpi->setValue(dpi);
}

int GraphicsExportDialog::getDpi() const noexcept {
  return mUi->spbxResolutionDpi->value();
}

void GraphicsExportDialog::setBlackWhite(bool blackWhite) noexcept {
  mUi->cbxBlackWhite->setChecked(blackWhite);
}

bool GraphicsExportDialog::getBlackWhite() const noexcept {
  return mUi->cbxBlackWhite->isChecked();
}

void GraphicsExportDialog::setBackgroundColor(Qt::GlobalColor color) noexcept {
  if (color == Qt::white) {
    mUi->rbtnBackgroundWhite->setChecked(true);
  } else if (color == Qt::black) {
    mUi->rbtnBackgroundBlack->setChecked(true);
  } else {
    mUi->rbtnBackgroundNone->setChecked(true);
  }
}

Qt::GlobalColor GraphicsExportDialog::getBackgroundColor() const noexcept {
  if (mUi->rbtnBackgroundWhite->isChecked()) {
    return Qt::white;
  } else if (mUi->rbtnBackgroundBlack->isChecked()) {
    return Qt::black;
  } else {
    return Qt::transparent;
  }
}

void GraphicsExportDialog::setMinLineWidth(
    const UnsignedLength& width) noexcept {
  mUi->edtMinLineWidth->setValue(width);
}

UnsignedLength GraphicsExportDialog::getMinLineWidth() const noexcept {
  return mUi->edtMinLineWidth->getValue();
}

void GraphicsExportDialog::setPrinterName(const QString& name) noexcept {
  for (int i = 0; i < mAvailablePrinters.count(); ++i) {
    if (mAvailablePrinters.at(i).printerName() == name) {
      mUi->cbxPrinter->setCurrentIndex(i);
      return;
    }
  }
}

QString GraphicsExportDialog::getPrinterName() const noexcept {
  return mAvailablePrinters.value(mUi->cbxPrinter->currentIndex())
      .printerName();
}

void GraphicsExportDialog::setDuplex(QPrinter::DuplexMode duplex) noexcept {
  for (int i = 0; i < mUi->cbxDuplex->count(); ++i) {
    if (mUi->cbxDuplex->itemData(i).toInt() == duplex) {
      mUi->cbxDuplex->setCurrentIndex(i);
      return;
    }
  }
}

QPrinter::DuplexMode GraphicsExportDialog::getDuplex() const noexcept {
  const int intValue = mUi->cbxDuplex->currentData().toInt();
  const QSet<int> validValues = {
      QPrinter::DuplexNone,
      QPrinter::DuplexLongSide,
      QPrinter::DuplexShortSide,
  };
  if (validValues.contains(intValue)) {
    return static_cast<QPrinter::DuplexMode>(intValue);
  } else {
    return QPrinter::DuplexNone;
  }
}

void GraphicsExportDialog::setPageContent(
    const QList<ContentItem>& items) noexcept {
  QSignalBlocker signalBlocker(mUi->treeContent);  // Avoid recursion.
  mUi->treeContent->clear();
  mPageContentItems = items;
  foreach (const ContentItem& item, items) {
    QTreeWidgetItem* node = new QTreeWidgetItem(mUi->treeContent);
    node->setText(0, item.name);
    node->setCheckState(0, item.enabled ? Qt::Checked : Qt::Unchecked);
    node->setCheckState(1, item.mirror ? Qt::Checked : Qt::Unchecked);
    node->setFlags(Qt::ItemIsSelectable | Qt::ItemIsUserCheckable |
                   Qt::ItemIsEnabled | Qt::ItemIsEditable |
                   Qt::ItemIsDragEnabled);
    foreach (const GraphicsLayer& layer, mLayers) {
      QTreeWidgetItem* child = new QTreeWidgetItem(node);
      child->setText(0, layer.getNameTr());
      child->setCheckState(
          0,
          item.layers.contains(layer.getName()) ? Qt::Checked : Qt::Unchecked);
    }
  }
  mUi->treeContent->viewport()->update();  // Fix UI flicker.
  applySettings();  // Because we disabled updates triggered by the UI.
}

const QList<GraphicsExportDialog::ContentItem>&
    GraphicsExportDialog::getPageContent() const noexcept {
  return mPageContentItems;
}

void GraphicsExportDialog::setOpenExportedFiles(bool open) noexcept {
  mUi->cbxOpenExportedFiles->setChecked(open);
}

bool GraphicsExportDialog::getOpenExportedFiles() const noexcept {
  return mUi->cbxOpenExportedFiles->isChecked();
}

void GraphicsExportDialog::updateLayerColorsListWidget() noexcept {
  mUi->lstLayerColors->clear();
  foreach (const GraphicsLayer& layer, mLayers) {
    QListWidgetItem* item = new QListWidgetItem(layer.getNameTr());
    item->setData(Qt::DecorationRole, layer.getColor());
    mUi->lstLayerColors->addItem(item);
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
