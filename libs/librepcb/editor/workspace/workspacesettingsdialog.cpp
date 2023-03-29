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
#include "workspacesettingsdialog.h"

#include "../dialogs/filedialog.h"
#include "../editorcommandset.h"
#include "../modelview/comboboxdelegate.h"
#include "../modelview/editablelistmodel.h"
#include "../modelview/keyboardshortcutsmodel.h"
#include "../modelview/keysequencedelegate.h"
#include "../utils/editortoolbox.h"
#include "desktopservices.h"
#include "ui_workspacesettingsdialog.h"

#include <librepcb/core/application.h>
#include <librepcb/core/norms.h>
#include <librepcb/core/utils/toolbox.h>
#include <librepcb/core/workspace/workspace.h>
#include <librepcb/core/workspace/workspacesettings.h>

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

WorkspaceSettingsDialog::WorkspaceSettingsDialog(Workspace& workspace,
                                                 QWidget* parent)
  : QDialog(parent),
    mWorkspace(workspace),
    mSettings(workspace.getSettings()),
    mLibLocaleOrderModel(new LibraryLocaleOrderModel()),
    mLibNormOrderModel(new LibraryNormOrderModel()),
    mRepositoryUrlsModel(new RepositoryUrlModel()),
    mKeyboardShortcutsModel(new KeyboardShortcutsModel(this)),
    mKeyboardShortcutsFilterModel(new QSortFilterProxyModel(this)),
    mUi(new Ui::WorkspaceSettingsDialog) {
  mUi->setupUi(this);

  const EditorCommandSet& cmd = EditorCommandSet::instance();

  // Initialize application locale widgets
  {
    mUi->cbxAppLocale->addItem(tr("System Language"), QString(""));
    QMap<QString, QString> map;  // map will be sorted by key
    foreach (const QString& locale, Application::getTranslationLocales()) {
      map.insert(Toolbox::prettyPrintLocale(locale), locale);
    }
    QMap<QString, QString>::const_iterator i = map.constBegin();
    while (i != map.constEnd()) {
      mUi->cbxAppLocale->addItem(i.key(), i.value());
      ++i;
    }
  }

  // Initialize "reset dismissed messages" button
  updateDismissedMessagesCount();
  connect(mUi->btnResetDismissedMessages, &QPushButton::clicked, this,
          [this]() {
            try {
              mSettings.dismissedMessages.restoreDefault();
              mWorkspace.saveSettings();  // can throw
            } catch (const Exception& e) {
              QMessageBox::critical(this, tr("Error"), e.getMsg());
            }
            updateDismissedMessagesCount();
          });

  // Initialize library locale order widgets
  {
    QList<QLocale> locales = QLocale::matchingLocales(
        QLocale::AnyLanguage, QLocale::AnyScript, QLocale::AnyCountry);
    QStringList localesStr;
    foreach (const QLocale& l, locales) { localesStr.append(l.name()); }
    mLibLocaleOrderModel->setPlaceholderText(tr("Click here to add a locale"));
    mLibLocaleOrderModel->setDefaultValue(QString(""));
    mLibLocaleOrderModel->setChoices(localesStr);
    mUi->tblLibLocaleOrder->setShowMoveButtons(true);
    mUi->tblLibLocaleOrder->setModel(mLibLocaleOrderModel.data());
    mUi->tblLibLocaleOrder->setItemDelegateForColumn(
        LibraryLocaleOrderModel::COLUMN_TEXT,
        new ComboBoxDelegate(false, this));
    mUi->tblLibLocaleOrder->horizontalHeader()->setSectionResizeMode(
        LibraryLocaleOrderModel::COLUMN_TEXT, QHeaderView::Stretch);
    mUi->tblLibLocaleOrder->horizontalHeader()->setSectionResizeMode(
        LibraryLocaleOrderModel::COLUMN_ACTIONS, QHeaderView::ResizeToContents);
    connect(mUi->tblLibLocaleOrder, &EditableTableWidget::btnAddClicked,
            mLibLocaleOrderModel.data(), &LibraryLocaleOrderModel::addItem);
    connect(mUi->tblLibLocaleOrder, &EditableTableWidget::btnRemoveClicked,
            mLibLocaleOrderModel.data(), &LibraryLocaleOrderModel::removeItem);
    connect(mUi->tblLibLocaleOrder, &EditableTableWidget::btnMoveUpClicked,
            mLibLocaleOrderModel.data(), &LibraryLocaleOrderModel::moveItemUp);
    connect(mUi->tblLibLocaleOrder, &EditableTableWidget::btnMoveDownClicked,
            mLibLocaleOrderModel.data(),
            &LibraryLocaleOrderModel::moveItemDown);
  }

  // Initialize library norm order widgets
  {
    mLibNormOrderModel->setPlaceholderText(tr("Click here to add a norm"));
    mLibNormOrderModel->setDefaultValue(QString(""));
    mLibNormOrderModel->setChoices(getAvailableNorms());
    mUi->tblLibNormOrder->setShowMoveButtons(true);
    mUi->tblLibNormOrder->setModel(mLibNormOrderModel.data());
    mUi->tblLibNormOrder->setItemDelegateForColumn(
        LibraryNormOrderModel::COLUMN_TEXT, new ComboBoxDelegate(true, this));
    mUi->tblLibNormOrder->horizontalHeader()->setSectionResizeMode(
        LibraryNormOrderModel::COLUMN_TEXT, QHeaderView::Stretch);
    mUi->tblLibNormOrder->horizontalHeader()->setSectionResizeMode(
        LibraryNormOrderModel::COLUMN_ACTIONS, QHeaderView::ResizeToContents);
    connect(mUi->tblLibNormOrder, &EditableTableWidget::btnAddClicked,
            mLibNormOrderModel.data(), &LibraryNormOrderModel::addItem);
    connect(mUi->tblLibNormOrder, &EditableTableWidget::btnRemoveClicked,
            mLibNormOrderModel.data(), &LibraryNormOrderModel::removeItem);
    connect(mUi->tblLibNormOrder, &EditableTableWidget::btnMoveUpClicked,
            mLibNormOrderModel.data(), &LibraryNormOrderModel::moveItemUp);
    connect(mUi->tblLibNormOrder, &EditableTableWidget::btnMoveDownClicked,
            mLibNormOrderModel.data(), &LibraryNormOrderModel::moveItemDown);
  }

  // Initialize repository URL widgets
  {
    mRepositoryUrlsModel->setPlaceholderText(tr("Click here a add an URL"));
    mUi->tblRepositoryUrls->setShowMoveButtons(true);
    mUi->tblRepositoryUrls->setModel(mRepositoryUrlsModel.data());
    mUi->tblRepositoryUrls->horizontalHeader()->setSectionResizeMode(
        RepositoryUrlModel::COLUMN_TEXT, QHeaderView::Stretch);
    mUi->tblRepositoryUrls->horizontalHeader()->setSectionResizeMode(
        RepositoryUrlModel::COLUMN_ACTIONS, QHeaderView::ResizeToContents);
    connect(mUi->tblRepositoryUrls, &EditableTableWidget::btnAddClicked,
            mRepositoryUrlsModel.data(), &RepositoryUrlModel::addItem);
    connect(mUi->tblRepositoryUrls, &EditableTableWidget::btnRemoveClicked,
            mRepositoryUrlsModel.data(), &RepositoryUrlModel::removeItem);
    connect(mUi->tblRepositoryUrls, &EditableTableWidget::btnMoveUpClicked,
            mRepositoryUrlsModel.data(), &RepositoryUrlModel::moveItemUp);
    connect(mUi->tblRepositoryUrls, &EditableTableWidget::btnMoveDownClicked,
            mRepositoryUrlsModel.data(), &RepositoryUrlModel::moveItemDown);
    connect(mUi->lblRepositoriesInfo, &QLabel::linkActivated, this,
            [this](const QString& url) {
              DesktopServices ds(mWorkspace.getSettings(), this);
              ds.openWebUrl(QUrl(url));
            });
  }

  // Initialize external applications widgets
  {
    auto placeholderFilePath =
        std::make_pair(QString("{{FILEPATH}}"),
                       tr("Absolute path to the file to open",
                          "Decription for '{{FILEPATH}}' placeholder"));
    auto placeholderUrl =
        std::make_pair(QString("{{URL}}"),
                       tr("URL to the file to open (file://)",
                          "Decription for '{{URL}}' placeholder"));

    connect(mUi->lstExternalApplications, &QListWidget::currentRowChanged, this,
            &WorkspaceSettingsDialog::externalApplicationListIndexChanged);

    mUi->lstExternalApplications->addItem(new QListWidgetItem(
        QIcon(":/img/actions/open_browser.png"), tr("Web Browser")));
    mExternalApplications.append(ExternalApplication{
        &mSettings.externalWebBrowserCommands,
        "firefox",
        "\"{{URL}}\"",
        {std::make_pair(
            QString("{{URL}}"),
            tr("Website URL to open", "Decription for '{{URL}}' placeholder"))},
        {},
    });

    mUi->lstExternalApplications->addItem(new QListWidgetItem(
        QIcon(":/img/actions/open.png"), tr("File Manager")));
    mExternalApplications.append(ExternalApplication{
        &mSettings.externalFileManagerCommands,
        "explorer",
        "\"{{FILEPATH}}\"",
        {placeholderFilePath, placeholderUrl},
        {},
    });

    mUi->lstExternalApplications->addItem(
        new QListWidgetItem(QIcon(":/img/actions/pdf.png"), tr("PDF Reader")));
    mExternalApplications.append(ExternalApplication{
        &mSettings.externalPdfReaderCommands,
        "evince",
        "\"{{FILEPATH}}\"",
        {placeholderFilePath, placeholderUrl},
        {},
    });

    mUi->lstExternalApplications->setMinimumWidth(
        mUi->lstExternalApplications->sizeHintForColumn(0) + 20);
    mUi->lstExternalApplications->setCurrentRow(0);
  }

  // Initialize keyboard shortcuts widgets
  {
    mKeyboardShortcutsFilterModel->setSourceModel(
        mKeyboardShortcutsModel.data());
    mKeyboardShortcutsFilterModel->setFilterCaseSensitivity(
        Qt::CaseInsensitive);
    mKeyboardShortcutsFilterModel->setFilterKeyColumn(-1);  // All columns.
#if (QT_VERSION >= QT_VERSION_CHECK(5, 10, 0))
    mKeyboardShortcutsFilterModel->setRecursiveFilteringEnabled(true);
#else
    // Filtering would be more complicated with Qt < 5.10, not really worth
    // the effort. Thus simply don't provide the filter feature.
    mUi->line->hide();
    mUi->edtCommandFilter->hide();
#endif
    connect(mUi->edtCommandFilter, &QLineEdit::textChanged,
            mKeyboardShortcutsFilterModel.data(),
            &QSortFilterProxyModel::setFilterFixedString);
    connect(mUi->edtCommandFilter, &QLineEdit::textChanged,
            mUi->treeKeyboardShortcuts, &QTreeView::expandAll);
    mUi->treeKeyboardShortcuts->setModel(mKeyboardShortcutsFilterModel.data());
    mUi->treeKeyboardShortcuts->header()->setMinimumSectionSize(
        QKeySequenceEdit().sizeHint().width());
    mUi->treeKeyboardShortcuts->header()->setSectionResizeMode(
        0, QHeaderView::ResizeToContents);
    mUi->treeKeyboardShortcuts->header()->setSectionResizeMode(
        1, QHeaderView::Stretch);
    mUi->treeKeyboardShortcuts->header()->setSectionResizeMode(
        2, QHeaderView::ResizeToContents);
    KeySequenceDelegate* delegate = new KeySequenceDelegate(this);
    mUi->treeKeyboardShortcuts->setItemDelegateForColumn(2, delegate);
    mUi->treeKeyboardShortcuts->addAction(cmd.find.createAction(
        this, this,
        [this]() { mUi->edtCommandFilter->setFocus(Qt::ShortcutFocusReason); },
        EditorCommand::ActionFlag::WidgetShortcut));
  }

  // Initialize themes widgets.
  {
    for (QComboBox* cbx :
         {mUi->cbxSchematicGridStyle, mUi->cbxBoardGridStyle}) {
      cbx->addItem(tr("None", "Grid style"),
                   QVariant::fromValue(Theme::GridStyle::None));
      cbx->addItem(tr("Dots", "Grid style"),
                   QVariant::fromValue(Theme::GridStyle::Dots));
      cbx->addItem(tr("Lines", "Grid style"),
                   QVariant::fromValue(Theme::GridStyle::Lines));
    }
    auto askName = [this](const QString& title, const QString& defaultName) {
      return QInputDialog::getText(this, title, tr("Name:"), QLineEdit::Normal,
                                   defaultName);
    };
    connect(
        mUi->cbxThemes,
        static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
        this, &WorkspaceSettingsDialog::themeIndexChanged);
    connect(mUi->btnRemoveTheme, &QToolButton::clicked, this, [this]() {
      if (Theme* theme = getCurrentTheme()) {
        mThemes.remove(theme->getUuid());
        updateThemesList(mSettings.themes.getActiveUuid());
      }
    });
    connect(mUi->btnRenameTheme, &QToolButton::clicked, this,
            [this, askName]() {
              if (Theme* theme = getCurrentTheme()) {
                const QString name =
                    askName(mUi->btnRenameTheme->text(), theme->getName());
                if (!name.isEmpty()) {
                  theme->setName(name);
                  updateThemesList(theme->getUuid());
                }
              }
            });
    connect(mUi->btnResetTheme, &QToolButton::clicked, this, [this]() {
      if (Theme* theme = getCurrentTheme()) {
        theme->restoreDefaults();
        updateThemesList(theme->getUuid());
      }
    });
    connect(mUi->btnDuplicateTheme, &QToolButton::clicked, this,
            [this, askName]() {
              if (Theme* theme = getCurrentTheme()) {
                const QString name =
                    askName(mUi->btnDuplicateTheme->text(),
                            tr("Copy of %1").arg(theme->getName()));
                if (!name.isEmpty()) {
                  const Theme copy(Uuid::createRandom(), name, *theme);
                  mThemes.insert(copy.getUuid(), copy);
                  updateThemesList(copy.getUuid());
                }
              }
            });
    connect(mUi->btnNewTheme, &QToolButton::clicked, this, [this, askName]() {
      const QString name = askName(mUi->btnNewTheme->text(), tr("New Theme"));
      if (!name.isEmpty()) {
        const Theme theme(Uuid::createRandom(), name);
        mThemes.insert(theme.getUuid(), theme);
        updateThemesList(theme.getUuid());
      }
    });
    mUi->treeThemeColors->header()->setSectionsMovable(false);
    mUi->treeThemeColors->header()->setSectionResizeMode(
        0, QHeaderView::ResizeToContents);
    mUi->treeThemeColors->header()->setSectionResizeMode(
        1, QHeaderView::ResizeToContents);
    mUi->treeThemeColors->header()->setSectionResizeMode(
        2, QHeaderView::ResizeToContents);
    mUi->treeThemeColors->header()->setSectionResizeMode(3,
                                                         QHeaderView::Stretch);
    connect(mUi->treeThemeColors, &QTreeWidget::itemDoubleClicked, this,
            [this](QTreeWidgetItem* item, int column) {
              Theme* theme = getCurrentTheme();
              if ((!item) || (column < 0) || (column > 1) || (!theme)) return;
              QList<ThemeColor> colors = theme->getColors();
              const int index = mUi->treeThemeColors->indexOfTopLevelItem(item);
              if ((index < 0) || (index >= colors.count())) return;
              QColor value = (column == 1) ? colors[index].getSecondaryColor()
                                           : colors[index].getPrimaryColor();
              if (!value.isValid()) return;
              value = QColorDialog::getColor(value, this, QString(),
                                             QColorDialog::ShowAlphaChannel);
              if (!value.isValid()) return;
              if (column == 1) {
                colors[index].setSecondaryColor(value);
              } else {
                colors[index].setPrimaryColor(value);
              }
              theme->setColors(colors);
              initColorTreeWidgetItem(*item, colors[index]);
            });
    for (auto cfg : {
             std::make_pair(mUi->cbxSchematicGridStyle,
                            &Theme::setSchematicGridStyle),
             std::make_pair(mUi->cbxBoardGridStyle, &Theme::setBoardGridStyle),
         }) {
      connect(cfg.first,
              static_cast<void (QComboBox::*)(int)>(
                  &QComboBox::currentIndexChanged),
              this, [this, cfg](int index) {
                if (Theme* theme = getCurrentTheme()) {
                  const Theme::GridStyle style =
                      cfg.first->itemData(index).value<Theme::GridStyle>();
                  (theme->*cfg.second)(style);
                }
              });
    }
  }

  // Now load all current settings
  loadSettings();

  // Load the window geometry
  QSettings clientSettings;
  restoreGeometry(
      clientSettings.value("workspace_settings_dialog/window_geometry")
          .toByteArray());

  // Just in case that the wrong tab is selected in the UI designer:
  mUi->tabWidget->setCurrentIndex(0);

  // Connect event handlers
  connect(mUi->buttonBox, &QDialogButtonBox::clicked, this,
          &WorkspaceSettingsDialog::buttonBoxClicked);
}

WorkspaceSettingsDialog::~WorkspaceSettingsDialog() {
  // Save the window geometry
  QSettings clientSettings;
  clientSettings.setValue("workspace_settings_dialog/window_geometry",
                          saveGeometry());
}

/*******************************************************************************
 *  Private Slots for the GUI elements
 ******************************************************************************/

void WorkspaceSettingsDialog::buttonBoxClicked(
    QAbstractButton* button) noexcept {
  switch (mUi->buttonBox->buttonRole(button)) {
    case QDialogButtonBox::RejectRole: {
      reject();
      break;
    }

    case QDialogButtonBox::AcceptRole: {
      saveSettings();
      accept();
      break;
    }

    case QDialogButtonBox::ApplyRole: {
      saveSettings();
      break;
    }

    case QDialogButtonBox::ResetRole: {
      int answer = QMessageBox::question(
          this, tr("Restore default settings"),
          tr("Are you sure to reset all settings to their default values?\n\n"
             "Attention: This will be applied immediately and cannot be "
             "undone!"));
      if (answer == QMessageBox::Yes) {
        mSettings.restoreDefaults();
        loadSettings();  // updating all widgets with the new values
        saveSettings();  // save now since "cancel" does not revert!
      }
      break;
    }

    default:
      Q_ASSERT(false);
      break;
  }
}

void WorkspaceSettingsDialog::keyPressEvent(QKeyEvent* event) noexcept {
  // If the keyboard shortcuts tab is opened and a filter is active, discard
  // the filter with the escape key.
  if ((event->key() == Qt::Key_Escape) &&
      (mUi->tabWidget->currentWidget() == mUi->keyboardShortcutsTab) &&
      (!mUi->edtCommandFilter->text().isEmpty())) {
    mUi->edtCommandFilter->clear();
    return;
  }
  QDialog::keyPressEvent(event);
}

void WorkspaceSettingsDialog::externalApplicationListIndexChanged(
    int index) noexcept {
  if ((index < 0) || (index >= mExternalApplications.count())) {
    return;
  }

  while (mUi->layoutExternalApplicationCommands->count() > 0) {
    QLayoutItem* item = mUi->layoutExternalApplicationCommands->takeAt(0);
    Q_ASSERT(item);
    EditorToolbox::deleteLayoutItemRecursively(item);
  }

  const EditorCommandSet& cmd = EditorCommandSet::instance();
  QStringList commands = mExternalApplications[index].currentValue;
  for (int i = 0; i <= commands.count(); ++i) {
    QLineEdit* edit = new QLineEdit(commands.value(i), this);
    edit->setPlaceholderText(
        tr("Example:") % " " % mExternalApplications[index].exampleExecutable %
        " " % mExternalApplications[index].defaultArgument);
    if (i < commands.count()) {
      connect(edit, &QLineEdit::textChanged, edit, [this, index, edit, i]() {
        mExternalApplications[index].currentValue.replace(i, edit->text());
      });
    } else {
      connect(edit, &QLineEdit::editingFinished, edit, [this, index, edit]() {
        if (!edit->text().isEmpty()) {
          mExternalApplications[index].currentValue.append(edit->text());
        }
      });
      connect(edit, &QLineEdit::editingFinished, edit,
              [this]() {
                externalApplicationListIndexChanged(
                    mUi->lstExternalApplications->currentRow());
              },
              Qt::QueuedConnection);
    }
    mUi->layoutExternalApplicationCommands->addWidget(edit);

    // Add 'browse' action.
    QAction* aBrowse = cmd.inputBrowse.createAction(
        edit, this,
        [this, edit, index]() {
          QString fp = FileDialog::getOpenFileName(
              this, tr("Select executable"), QDir::rootPath());
          if (!fp.isEmpty()) {
            edit->setText(fp % " " %
                          mExternalApplications[index].defaultArgument);
            emit edit->editingFinished();
          }
        },
        EditorCommand::ActionFlag::WidgetShortcut);
    edit->addAction(aBrowse, QLineEdit::TrailingPosition);

    // Add 'remove' action.
    if (i < commands.count()) {
      QAction* aRemove = cmd.inputRemove.createAction(
          edit, this,
          [this, index, i]() {
            mExternalApplications[index].currentValue.removeAt(i);
            externalApplicationListIndexChanged(index);
          },
          EditorCommand::ActionFlag::WidgetShortcut |
              EditorCommand::ActionFlag::QueuedConnection);
      edit->addAction(aRemove, QLineEdit::TrailingPosition);
    }
  }

  QString placeholdersText =
      "<p>" % tr("Available placeholders:") % "</p><p><ul>";
  foreach (const auto& p, mExternalApplications[index].placeholders) {
    placeholdersText += "<li><tt>" % p.first % "</tt>: " % p.second % "</li>";
  }
  placeholdersText += "</ul></p>";
  mUi->lblExternalApplicationsPlaceholders->setText(placeholdersText);
}

void WorkspaceSettingsDialog::updateThemesList(
    const Uuid& selectedTheme) noexcept {
  mUi->cbxThemes->clear();
  foreach (const Theme& theme, mThemes) {
    mUi->cbxThemes->addItem(theme.getName(), theme.getUuid().toStr());
  }
  const int index =
      std::max(0, mUi->cbxThemes->findData(selectedTheme.toStr()));
  mUi->cbxThemes->setCurrentIndex(index);
}

void WorkspaceSettingsDialog::themeIndexChanged(int index) noexcept {
  const bool valid = (index >= 0) && (index < mThemes.count());
  const Theme theme = mThemes.values().value(index);

  mUi->btnRemoveTheme->setEnabled(valid);
  mUi->btnRenameTheme->setEnabled(valid);
  mUi->btnResetTheme->setEnabled(valid);
  mUi->btnDuplicateTheme->setEnabled(valid);

  // Colors.
  mUi->treeThemeColors->clear();
  for (int i = 0; i < theme.getColors().count(); ++i) {
    QTreeWidgetItem* item = new QTreeWidgetItem(mUi->treeThemeColors);
    initColorTreeWidgetItem(*item, theme.getColors().at(i));
  }
  mUi->treeThemeColors->setEnabled(valid);

  // Grid style.
  for (const auto& cfg : {
           std::make_pair(mUi->cbxSchematicGridStyle,
                          theme.getSchematicGridStyle()),
           std::make_pair(mUi->cbxBoardGridStyle, theme.getBoardGridStyle()),
       }) {
    const int index = cfg.first->findData(QVariant::fromValue(cfg.second));
    cfg.first->setCurrentIndex(index);
    cfg.first->setEnabled(valid);
  }
}

void WorkspaceSettingsDialog::initColorTreeWidgetItem(
    QTreeWidgetItem& item, const ThemeColor& color) noexcept {
  auto init = [&item](int column, const QString& toolTip, const QColor& value) {
    item.setBackground(column, value.isValid() ? value : Qt::transparent);
    item.setToolTip(
        column,
        toolTip.arg(value.isValid() ? value.name(QColor::HexArgb).toUpper()
                                    : tr("N/A")));
    item.setText(column, value.isValid() ? "" : "✖");
    item.setTextAlignment(column, Qt::AlignCenter);
  };
  init(0, tr("Primary color: %1"), color.getPrimaryColor());
  init(1, tr("Secondary color: %1"), color.getSecondaryColor());

  item.setText(2, color.getCategoryTr());
  item.setText(3, color.getNameTr());
}

Theme* WorkspaceSettingsDialog::getCurrentTheme() noexcept {
  const int index = mUi->cbxThemes->currentIndex();
  const tl::optional<Uuid> uuid =
      Uuid::tryFromString(mUi->cbxThemes->itemData(index).toString());
  return (uuid && mThemes.contains(*uuid)) ? &mThemes[*uuid] : nullptr;
}

void WorkspaceSettingsDialog::updateDismissedMessagesCount() noexcept {
  const int count = mSettings.dismissedMessages.get().count();
  mUi->btnResetDismissedMessages->setText(tr("Reset") % " (" %
                                          QString::number(count) % ")");
  mUi->btnResetDismissedMessages->setToolTip(
      tr("Reset all permanently dismissed messages (something like \"do not "
         "show again\") to make them appear again.") %
      "\n\n" %
      tr("Currently there are %1 dismissed message(s).", nullptr, count)
          .arg(count));
}

void WorkspaceSettingsDialog::loadSettings() noexcept {
  // User Name
  mUi->edtUserName->setText(mSettings.userName.get());

  // Application Locale
  mUi->cbxAppLocale->setCurrentIndex(
      mUi->cbxAppLocale->findData(mSettings.applicationLocale.get()));

  // Default Length Unit
  mUi->cbxDefaultLengthUnit->clear();
  foreach (const LengthUnit& unit, LengthUnit::getAllUnits()) {
    mUi->cbxDefaultLengthUnit->addItem(unit.toStringTr(), unit.getIndex());
  }
  mUi->cbxDefaultLengthUnit->setCurrentIndex(
      mSettings.defaultLengthUnit.get().getIndex());

  // Autosave Interval
  mUi->spbAutosaveInterval->setValue(
      mSettings.projectAutosaveIntervalSeconds.get());

  // Use OpenGL
  mUi->cbxUseOpenGl->setChecked(mSettings.useOpenGl.get());

  // Library Locale Order
  mLibLocaleOrderModel->setValues(mSettings.libraryLocaleOrder.get());

  // Library Norm Order
  mLibNormOrderModel->setValues(mSettings.libraryNormOrder.get());

  // Repository URLs
  mRepositoryUrlsModel->setValues(mSettings.repositoryUrls.get());

  // External Applications
  for (auto& app : mExternalApplications) {
    app.currentValue = app.setting->get();
  }
  externalApplicationListIndexChanged(
      mUi->lstExternalApplications->currentRow());

  // Keyboard Shortcuts
  mKeyboardShortcutsModel->setOverrides(mSettings.keyboardShortcuts.get());
  mUi->treeKeyboardShortcuts->expandAll();

  // Themes
  mThemes = mSettings.themes.getAll();
  updateThemesList(mSettings.themes.getActiveUuid());
}

void WorkspaceSettingsDialog::saveSettings() noexcept {
  try {
    // User Name
    mSettings.userName.set(mUi->edtUserName->text().trimmed());

    // Application Locale
    if (mUi->cbxAppLocale->currentIndex() >= 0) {
      mSettings.applicationLocale.set(
          mUi->cbxAppLocale->currentData().toString());
    }

    // Default Length Unit
    if (mUi->cbxDefaultLengthUnit->currentIndex() >= 0) {
      mSettings.defaultLengthUnit.set(LengthUnit::fromIndex(
          mUi->cbxDefaultLengthUnit->currentIndex()));  // can throw
    }

    // Autosave Interval
    mSettings.projectAutosaveIntervalSeconds.set(
        mUi->spbAutosaveInterval->value());

    // Use OpenGL
    mSettings.useOpenGl.set(mUi->cbxUseOpenGl->isChecked());

    // Library Locale Order
    mSettings.libraryLocaleOrder.set(mLibLocaleOrderModel->getValues());

    // Library Norm Order
    mSettings.libraryNormOrder.set(mLibNormOrderModel->getValues());

    // Repository URLs
    mSettings.repositoryUrls.set(mRepositoryUrlsModel->getValues());

    // External Applications
    for (auto& app : mExternalApplications) {
      QStringList commands;
      foreach (const QString& cmd, app.currentValue) {
        if (!cmd.trimmed().isEmpty()) {
          commands.append(cmd.trimmed());
        }
      }
      app.setting->set(commands);
    }

    // Keyboard shortcuts
    mSettings.keyboardShortcuts.set(mKeyboardShortcutsModel->getOverrides());

    // Themes
    mSettings.themes.setAll(mThemes);
    if (const Theme* theme = getCurrentTheme()) {
      mSettings.themes.setActiveUuid(theme->getUuid());
    }

    // Save settings to disk.
    mWorkspace.saveSettings();  // can throw
  } catch (const Exception& e) {
    QMessageBox::critical(this, tr("Error"), e.getMsg());
  }
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
