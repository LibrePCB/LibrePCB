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
#include "graphicsoutputjobwidget.h"

#include "ui_graphicsoutputjobwidget.h"

#include <librepcb/core/project/board/board.h>
#include <librepcb/core/project/outputjobrunner.h>
#include <librepcb/core/project/project.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

GraphicsOutputJobWidget::GraphicsOutputJobWidget(
    Project& project, std::shared_ptr<GraphicsOutputJob> job,
    const LengthUnit& lengthUnit, const QString& settingsPrefix,
    QWidget* parent) noexcept
  : QWidget(parent),
    mProject(project),
    mJob(job),
    mPreviewRunner(new OutputJobRunner(mProject)),
    mPreviewGraphicsExport(new GraphicsExport(this)),
    mDisableModification(true),
    mUi(new Ui::GraphicsOutputJobWidget) {
  mUi->setupUi(this);
  connect(mPreviewGraphicsExport.data(), &GraphicsExport::previewReady,
          mUi->previewWidget, &GraphicsExportWidget::setPageContent);

  // Name.
  mUi->edtName->setText(*job->getName());
  connect(mUi->edtName, &QLineEdit::textEdited, this, [this](QString text) {
    text = cleanElementName(text);
    if (!text.isEmpty()) {
      mJob->setName(ElementName(text));
    }
  });

  // Document title.
  mUi->edtDocumentTitle->setText(*job->getDocumentTitle());
  connect(mUi->edtDocumentTitle, &QLineEdit::textEdited, this,
          [this](QString text) {
            mJob->setDocumentTitle(cleanSimpleString(text));
          });

  // Output path.
  mUi->edtOutput->setText(job->getOutputPath());
  connect(mUi->edtOutput, &QLineEdit::textEdited, this, [this](QString text) {
    mJob->setOutputPath(text.replace("\\", "/").trimmed());
  });

  // Contents list.
  updateContentList();
  currentContentChanged(0);  // Force disabling if there is no content.
  mUi->lstContent->setCurrentRow(0);
  connect(mUi->lstContent, &QListWidget::currentRowChanged, this,
          &GraphicsOutputJobWidget::currentContentChanged);
  connect(mUi->lstContent, &QListWidget::itemChanged, this,
          [this](QListWidgetItem* item) {
            if (item) {
              const int index = mUi->lstContent->row(item);
              auto content = mJob->getContent();
              if ((index >= 0) && (index < content.count())) {
                content[index].title = item->text().trimmed();
                mJob->setContent(content);
              }
            }
          });
  connect(mUi->btnAdd, &QToolButton::clicked, this,
          &GraphicsOutputJobWidget::addClicked);
  connect(mUi->btnCopy, &QToolButton::clicked, this,
          &GraphicsOutputJobWidget::copyClicked);
  connect(mUi->btnRemove, &QToolButton::clicked, this,
          &GraphicsOutputJobWidget::removeClicked);

  // Page size.
  mPageSizes = {
      std::nullopt,  // Auto size.
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
  foreach (const std::optional<QPageSize>& size, mPageSizes) {
    mUi->cbxPageSize->addItem(size ? size->name()
                                   : tr("Custom (adjust to content)"));
  }
  connect(
      mUi->cbxPageSize,
      static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
      this, [this](int index) {
        modify([this, index](GraphicsOutputJob::Content& c) {
          if (auto size = mPageSizes.value(index)) {
            c.pageSizeKey = size->key();
          } else {
            c.pageSizeKey = std::nullopt;
          }
        });
      });

  // Orientation.
  connect(mUi->rbtnOrientationAuto, &QRadioButton::toggled, this,
          [this](bool checked) {
            if (checked) {
              modify([](GraphicsOutputJob::Content& c) {
                c.orientation = GraphicsExportSettings::Orientation::Auto;
              });
            }
          });
  connect(mUi->rbtnOrientationLandscape, &QRadioButton::toggled, this,
          [this](bool checked) {
            if (checked) {
              modify([](GraphicsOutputJob::Content& c) {
                c.orientation = GraphicsExportSettings::Orientation::Landscape;
              });
            }
          });
  connect(mUi->rbtnOrientationPortrait, &QRadioButton::toggled, this,
          [this](bool checked) {
            if (checked) {
              modify([](GraphicsOutputJob::Content& c) {
                c.orientation = GraphicsExportSettings::Orientation::Portrait;
              });
            }
          });

  // Margins.
  mUi->edtMarginLeft->configure(lengthUnit, LengthEditBase::Steps::generic(),
                                settingsPrefix % "/margin_left");
  mUi->edtMarginTop->configure(lengthUnit, LengthEditBase::Steps::generic(),
                               settingsPrefix % "/margin_top");
  mUi->edtMarginRight->configure(lengthUnit, LengthEditBase::Steps::generic(),
                                 settingsPrefix % "/margin_right");
  mUi->edtMarginBottom->configure(lengthUnit, LengthEditBase::Steps::generic(),
                                  settingsPrefix % "/margin_bottom");
  connect(mUi->edtMarginLeft, &UnsignedLengthEdit::valueChanged, this,
          [this](UnsignedLength value) {
            modify([value](GraphicsOutputJob::Content& c) {
              c.marginLeft = value;
            });
          });
  connect(
      mUi->edtMarginTop, &UnsignedLengthEdit::valueChanged, this,
      [this](UnsignedLength value) {
        modify([value](GraphicsOutputJob::Content& c) { c.marginTop = value; });
      });
  connect(mUi->edtMarginRight, &UnsignedLengthEdit::valueChanged, this,
          [this](UnsignedLength value) {
            modify([value](GraphicsOutputJob::Content& c) {
              c.marginRight = value;
            });
          });
  connect(mUi->edtMarginBottom, &UnsignedLengthEdit::valueChanged, this,
          [this](UnsignedLength value) {
            modify([value](GraphicsOutputJob::Content& c) {
              c.marginBottom = value;
            });
          });

  // Rotate.
  connect(mUi->cbxRotate, &QCheckBox::toggled, this, [this](bool checked) {
    modify([checked](GraphicsOutputJob::Content& c) { c.rotate = checked; });
  });

  // Mirror.
  connect(mUi->cbxMirror, &QCheckBox::toggled, this, [this](bool checked) {
    modify([checked](GraphicsOutputJob::Content& c) { c.mirror = checked; });
  });

  // Scale.
  connect(mUi->cbxScaleAuto, &QCheckBox::toggled, mUi->spbxScaleFactor,
          &UnsignedRatioEdit::setDisabled);
  connect(mUi->cbxScaleAuto, &QCheckBox::toggled, this, [this](bool checked) {
    modify([this, checked](GraphicsOutputJob::Content& c) {
      c.scale = checked ? std::nullopt
                        : std::make_optional(mUi->spbxScaleFactor->getValue());
    });
  });
  connect(
      mUi->spbxScaleFactor, &UnsignedRatioEdit::valueChanged, this,
      [this](UnsignedRatio ratio) {
        if (!mUi->cbxScaleAuto->isChecked()) {
          modify([ratio](GraphicsOutputJob::Content& c) { c.scale = ratio; });
        }
      });

  // Pixmap DPI.
  connect(mUi->spbxResolutionDpi,
          static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), this,
          [this](int dpi) {
            modify([dpi](GraphicsOutputJob::Content& c) { c.pixmapDpi = dpi; });
          });

  // Monochrome.
  connect(mUi->cbxMonochrome, &QCheckBox::toggled, this, [this](bool checked) {
    modify(
        [checked](GraphicsOutputJob::Content& c) { c.monochrome = checked; });
  });

  // Background color.
  connect(mUi->rbtnBackgroundNone, &QRadioButton::toggled, this,
          [this](bool checked) {
            if (checked) {
              modify([](GraphicsOutputJob::Content& c) {
                c.backgroundColor = Qt::transparent;
              });
            }
          });
  connect(mUi->rbtnBackgroundWhite, &QRadioButton::toggled, this,
          [this](bool checked) {
            if (checked) {
              modify([](GraphicsOutputJob::Content& c) {
                c.backgroundColor = Qt::white;
              });
            }
          });
  connect(mUi->rbtnBackgroundBlack, &QRadioButton::toggled, this,
          [this](bool checked) {
            if (checked) {
              modify([](GraphicsOutputJob::Content& c) {
                c.backgroundColor = Qt::black;
              });
            }
          });

  // Min. line width.
  mUi->edtMinLineWidth->configure(lengthUnit, LengthEditBase::Steps::generic(),
                                  settingsPrefix % "/min_line_width");
  connect(mUi->edtMinLineWidth, &UnsignedLengthEdit::valueChanged, this,
          [this](UnsignedLength value) {
            modify([value](GraphicsOutputJob::Content& c) {
              c.minLineWidth = value;
            });
          });

  // Layers.
  connect(mUi->lstLayerColors, &QListWidget::itemDoubleClicked, this,
          &GraphicsOutputJobWidget::layerListItemDoubleClicked);
  connect(mUi->lstLayerColors, &QListWidget::itemChanged, this,
          [this](QListWidgetItem* item) {
            modify([item](GraphicsOutputJob::Content& c) {
              Q_ASSERT(item);
              const QString layerName = item->data(Qt::UserRole).toString();
              const bool checked = (item->checkState() == Qt::Checked);
              const QColor color =
                  item->data(Qt::DecorationRole).value<QColor>();
              if (checked && (!layerName.isEmpty()) && color.isValid()) {
                c.layers[layerName] = color;
              } else if (!checked) {
                c.layers.remove(layerName);
              }
            });
          });

  // Select first tab.
  mUi->tabWidget->setCurrentIndex(0);

  // Setup preview.
  mUi->previewWidget->setShowPageNumbers(false);
  mUi->previewWidget->setShowResolution(false);
  updatePreview();
}

GraphicsOutputJobWidget::~GraphicsOutputJobWidget() noexcept {
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

void GraphicsOutputJobWidget::addClicked() noexcept {
  const int index =
      qBound(0, mUi->lstContent->currentRow() + 1, mUi->lstContent->count());
  auto content = mJob->getContent();
  QMenu menu;
  menu.addAction(QIcon(":/img/actions/schematic.png"), tr("Schematic"), [&]() {
    content.insert(index,
                   GraphicsOutputJob::Content(
                       GraphicsOutputJob::Content::Preset::Schematic));
  });
  menu.addAction(
      QIcon(":/img/actions/board_editor.png"), tr("Board Image"), [&]() {
        content.insert(index,
                       GraphicsOutputJob::Content(
                           GraphicsOutputJob::Content::Preset::BoardImage));
      });
  menu.addAction(
      QIcon(":/img/actions/board_editor.png"), tr("Assembly Top/Bottom"),
      [&]() {
        content.insert(
            index,
            GraphicsOutputJob::Content(
                GraphicsOutputJob::Content::Preset::BoardAssemblyTop));
        content.insert(
            index + 1,
            GraphicsOutputJob::Content(
                GraphicsOutputJob::Content::Preset::BoardAssemblyBottom));
      });
  if (menu.exec(QCursor::pos())) {
    mJob->setContent(content);
    updateContentList();
    mUi->lstContent->setCurrentRow(index);
    updatePreview();
  }
}

void GraphicsOutputJobWidget::copyClicked() noexcept {
  const int index = mUi->lstContent->currentRow();
  auto content = mJob->getContent();
  if ((index >= 0) && (index < content.count())) {
    content.insert(index + 1, content.at(index));
    mJob->setContent(content);
    updateContentList();
    mUi->lstContent->setCurrentRow(index + 1);
    updatePreview();
  }
}

void GraphicsOutputJobWidget::removeClicked() noexcept {
  const int index = mUi->lstContent->currentRow();
  auto content = mJob->getContent();
  if ((index >= 0) && (index < content.count())) {
    content.removeAt(index);
    mJob->setContent(content);
    updateContentList();
    mUi->lstContent->setCurrentRow(std::min(index, int(content.count()) - 1));
    updatePreview();
  }
}

void GraphicsOutputJobWidget::currentContentChanged(int index) noexcept {
  const bool valid = ((index >= 0) && (index < mJob->getContent().count()));
  mUi->tabWidget->setEnabled(valid);
  mUi->previewWidget->setEnabled(valid);

  if (valid) {
    const GraphicsOutputJob::Content c = mJob->getContent().at(index);
    mDisableModification = true;

    // Page size.
    for (int i = 0; i < mPageSizes.count(); ++i) {
      std::optional<QPageSize> value = mPageSizes.at(i);
      if (((!c.pageSizeKey) && (!value)) ||
          (c.pageSizeKey && value && (*c.pageSizeKey == value->key()))) {
        mUi->cbxPageSize->setCurrentIndex(i);
        break;
      }
    }

    // Orientation.
    if (c.orientation == GraphicsExportSettings::Orientation::Landscape) {
      mUi->rbtnOrientationLandscape->setChecked(true);
    } else if (c.orientation == GraphicsExportSettings::Orientation::Portrait) {
      mUi->rbtnOrientationPortrait->setChecked(true);
    } else {
      mUi->rbtnOrientationAuto->setChecked(true);
    }

    // Margins.
    mUi->edtMarginLeft->setValue(c.marginLeft);
    mUi->edtMarginTop->setValue(c.marginTop);
    mUi->edtMarginRight->setValue(c.marginRight);
    mUi->edtMarginBottom->setValue(c.marginBottom);

    // Rotate.
    mUi->cbxRotate->setChecked(c.rotate);

    // Mirror.
    mUi->cbxMirror->setChecked(c.mirror);

    // Scale.
    mUi->cbxScaleAuto->setChecked(!c.scale);
    mUi->spbxScaleFactor->setValue(
        c.scale ? *c.scale : UnsignedRatio(Ratio::fromPercent(100)));

    // Pixmap DPI.
    mUi->spbxResolutionDpi->setValue(c.pixmapDpi);

    // Monochrome.
    mUi->cbxMonochrome->setChecked(c.monochrome);

    // Background color.
    if (c.backgroundColor == Qt::white) {
      mUi->rbtnBackgroundWhite->setChecked(true);
    } else if (c.backgroundColor == Qt::black) {
      mUi->rbtnBackgroundBlack->setChecked(true);
    } else {
      mUi->rbtnBackgroundNone->setChecked(true);
    }

    // Minimum line width.
    mUi->edtMinLineWidth->setValue(c.minLineWidth);

    // Layers.
    int innerLayerCount = 0;
    foreach (const Board* board, mProject.getBoards()) {
      innerLayerCount = std::max(innerLayerCount, board->getInnerLayerCount());
    }
    Theme t;
    GraphicsExportSettings s;
    s.loadColorsFromTheme(
        t, (c.type == GraphicsOutputJob::Content::Type::Schematic),
        (c.type != GraphicsOutputJob::Content::Type::Schematic),
        innerLayerCount);
    mUi->lstLayerColors->clear();
    foreach (const auto& pair, s.getColors()) {
      const bool enabled = c.layers.contains(pair.first);
      QListWidgetItem* item =
          new QListWidgetItem(t.getColor(pair.first).getNameTr());
      item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
      item->setCheckState(enabled ? Qt::Checked : Qt::Unchecked);
      item->setData(Qt::DecorationRole,
                    enabled ? c.layers.value(pair.first) : pair.second);
      item->setData(Qt::UserRole, pair.first);
      mUi->lstLayerColors->addItem(item);
    }
  }

  mDisableModification = false;
}

void GraphicsOutputJobWidget::updateContentList() noexcept {
  mUi->lstContent->clear();
  foreach (const auto& content, mJob->getContent()) {
    QListWidgetItem* item = new QListWidgetItem(content.title, mUi->lstContent);
    item->setFlags(Qt::ItemIsEditable | Qt::ItemIsSelectable |
                   Qt::ItemIsEnabled);
    if (content.type == GraphicsOutputJob::Content::Type::Schematic) {
      item->setIcon(QIcon(":/img/actions/schematic.png"));
    } else if (content.type == GraphicsOutputJob::Content::Type::Board) {
      item->setIcon(QIcon(":/img/actions/board_editor.png"));
    }
  }
}

void GraphicsOutputJobWidget::layerListItemDoubleClicked(
    QListWidgetItem* item) noexcept {
  Q_ASSERT(item);
  QColor color = item->data(Qt::DecorationRole).value<QColor>();
  color = QColorDialog::getColor(color, this, QString(),
                                 QColorDialog::ShowAlphaChannel);
  if (color.isValid()) {
    item->setData(Qt::DecorationRole, color);
  }
}

void GraphicsOutputJobWidget::modify(
    std::function<void(GraphicsOutputJob::Content&)> fun) noexcept {
  if (mDisableModification) {
    return;
  }

  const int index = mUi->lstContent->currentRow();
  auto content = mJob->getContent();
  if ((index >= 0) && (index < content.count())) {
    fun(content[index]);
    mJob->setContent(content);
    updatePreview();
  }
}

void GraphicsOutputJobWidget::updatePreview() noexcept {
  try {
    auto pages = mPreviewRunner->buildPages(*mJob, false);
    mUi->previewWidget->setNumberOfPages(pages.count());
    mPreviewGraphicsExport->startPreview(pages);
  } catch (const Exception& e) {
    qCritical() << "Graphics preview failed:" << e.getMsg();
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
