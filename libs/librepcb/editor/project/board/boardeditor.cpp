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
#include "boardeditor.h"

#include "../../dialogs/filedialog.h"
#include "../../guiapplication.h"
#include "../../notification.h"
#include "../../notificationsmodel.h"
#include "../../rulecheck/rulecheckmessagesmodel.h"
#include "../../undostack.h"
#include "../../utils/slinthelpers.h"
#include "../../workspace/desktopservices.h"
#include "../cmd/cmdboardedit.h"
#include "../cmd/cmdboardnetsegmentremove.h"
#include "../cmd/cmdboardplaneedit.h"
#include "../projecteditor.h"
#include "board2dtab.h"
#include "board3dtab.h"
#include "boardsetupdialog.h"

#include <librepcb/core/3d/stepexport.h>
#include <librepcb/core/application.h>
#include <librepcb/core/fileio/transactionalfilesystem.h>
#include <librepcb/core/network/orderpcbapirequest.h>
#include <librepcb/core/project/board/board.h>
#include <librepcb/core/project/board/boardplanefragmentsbuilder.h>
#include <librepcb/core/project/board/drc/boarddesignrulecheckmessages.h>
#include <librepcb/core/project/circuit/circuit.h>
#include <librepcb/core/project/project.h>
#include <librepcb/core/types/layer.h>
#include <librepcb/core/workspace/workspace.h>
#include <librepcb/core/workspace/workspacesettings.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

BoardEditor::BoardEditor(ProjectEditor& prjEditor, Board& board, int uiIndex,
                         QObject* parent) noexcept
  : QObject(parent),
    onUiDataChanged(*this),
    mProjectEditor(prjEditor),
    mProject(mProjectEditor.getProject()),
    mBoard(board),
    mUiIndex(uiIndex),
    mThtSolderTechnology(ui::SolderTechnology::None),
    mSmtSolderTechnology(ui::SolderTechnology::None),
    mIpcDensityLevel(ui::IpcDensityLevel::None),
    mTimestampOfLastPlaneRebuild(0),
    mDrc(new BoardDesignRuleCheck(this)),
    mDrcNotification(new Notification(ui::NotificationType::Progress, QString(),
                                      QString(), QString(), QString(), true)),
    mDrcUndoStackState(0),
    mOrderUploadProgressPercent(-1),
    mOrderOpenBrowser(false) {
  // Connect board.
  connect(&mBoard, &Board::nameChanged, this,
          [this]() { onUiDataChanged.notify(); });
  connect(&mBoard, &Board::preferredFootprintTagsChanged, this,
          &BoardEditor::updatePreferredFootprintTags);

  // Connect project editor.
  connect(&mProjectEditor.getUndoStack(), &UndoStack::stateModified, this,
          [this]() {
            if (!mProjectEditor.getUndoStack().isCommandGroupActive()) {
              schedulePlanesRebuild();
            }
          });

  // Connect DRC.
  connect(mDrc.get(), &BoardDesignRuleCheck::progressPercent,
          mDrcNotification.get(), &Notification::setProgress);
  connect(mDrc.get(), &BoardDesignRuleCheck::progressStatus,
          mDrcNotification.get(), &Notification::setDescription);
  connect(mDrc.get(), &BoardDesignRuleCheck::finished, this,
          &BoardEditor::setDrcResult);

  updatePreferredFootprintTags();
}

BoardEditor::~BoardEditor() noexcept {
  // Stop ongoing operations, timers etc.
  mOrderRequest.reset();

  // Unregister callbacks.
  if (mDrcMessages) {
    mDrcMessages->setAutofixHandler(nullptr);
  }

  // Request all tabs to close.
  emit aboutToBeDestroyed();

  Q_ASSERT(mActive2dTabs.isEmpty());
  Q_ASSERT(mActive3dTabs.isEmpty());
}

/*******************************************************************************
 *  General Methods
 ******************************************************************************/

void BoardEditor::setUiIndex(int index) noexcept {
  if (index != mUiIndex) {
    mUiIndex = index;
    emit uiIndexChanged();
  }
}

ui::BoardData BoardEditor::getUiData() const noexcept {
  ui::RuleCheckState drcState;
  if (mDrc->isRunning()) {
    drcState = ui::RuleCheckState::Running;
  } else if (!mDrcMessages) {
    drcState = ui::RuleCheckState::NotRunYet;
  } else if (mDrcUndoStackState ==
             mProjectEditor.getUndoStack().getUniqueStateId()) {
    drcState = ui::RuleCheckState::UpToDate;
  } else {
    drcState = ui::RuleCheckState::Outdated;
  }

  return ui::BoardData{
      q2s(*mBoard.getName()),  // Name
      ui::RuleCheckData{
          ui::RuleCheckType::Drc,  // Type
          drcState,  // State
          mDrcMessages,  // Rule check messages
          mDrcMessages ? mDrcMessages->getUnapprovedCount() : 0,  // Unapproved
          mDrcMessages ? mDrcMessages->getErrorCount() : 0,  // Errors
          q2s(mDrcExecutionError),  // Execution error
          !mProject.getDirectory().isWritable(),  // Read-only
      },
      !mBoard.getPreferredFootprintTags().isEmpty(),  // Has preferred tags
      mThtSolderTechnology,  // THT soldering
      mSmtSolderTechnology,  // SMT soldering
      mIpcDensityLevel,  // IPC density level
      q2s(mOrderStatus),  // Order status / error
      mOrderRequest ? q2s(mOrderRequest->getReceivedInfoUrl().toString())
                    : slint::SharedString(),  // Order info URL
      mOrderUploadProgressPercent,  // Order upload progress
      mOrderRequest ? q2s(mOrderRequest->getReceivedRedirectUrl().toString())
                    : slint::SharedString(),  // Order upload URL
  };
}

void BoardEditor::setUiData(const ui::BoardData& data) noexcept {
  static QHash<ui::SolderTechnology, QVector<Tag>> solderTechnologyTags = {
      {ui::SolderTechnology::HandLarge,
       {
           Tag("hand-soldering"),
           Tag("extra-large-pads"),
           Tag("ipc-density-level-a"),
           Tag("ipc-density-level-b"),
       }},
      {ui::SolderTechnology::Hand, {Tag("hand-soldering")}},
      {ui::SolderTechnology::Reflow, {Tag("reflow-soldering")}},
      {ui::SolderTechnology::Wave, {Tag("wave-soldering")}},
  };
  static QHash<ui::IpcDensityLevel, QVector<Tag>> ipcDensityLevelTags = {
      {ui::IpcDensityLevel::A,
       {
           Tag("ipc-density-level-a"),
           Tag("ipc-density-level-b"),
       }},
      {ui::IpcDensityLevel::B,
       {
           Tag("ipc-density-level-b"),
       }},
      {ui::IpcDensityLevel::C,
       {
           Tag("ipc-density-level-c"),
           Tag("ipc-density-level-b"),
       }},
  };
  auto editPreferredFootprintTags =
      [](const std::initializer_list<QVector<Tag>*>& tagsList,
         const QList<QVector<Tag>>& removeTags, const QVector<Tag>& prependTags,
         const QVector<Tag>& appendTags) {
        for (QVector<Tag>* tags : tagsList) {
          for (const QVector<Tag>& tv : removeTags) {
            for (const Tag& t : tv) {
              tags->removeAll(t);
            }
          }
          *tags = prependTags + (*tags) + appendTags;
        }
      };
  auto setPreferredFootprintTags =
      [this](std::function<void(Board::PreferredFootprintTags & tags)> cb) {
        try {
          Board::PreferredFootprintTags tags =
              mBoard.getPreferredFootprintTags();
          cb(tags);
          std::unique_ptr<CmdBoardEdit> cmd(new CmdBoardEdit(mBoard));
          cmd->setPreferredFootprintTags(tags);
          mProjectEditor.getUndoStack().execCmd(cmd.release());
        } catch (const Exception& e) {
          qCritical() << e.getMsg();
        }
      };

  if (data.tht_soldering != mThtSolderTechnology) {
    setPreferredFootprintTags([&](Board::PreferredFootprintTags& tags) {
      editPreferredFootprintTags(
          {&tags.thtTop, &tags.thtBot},
          solderTechnologyTags.values() + ipcDensityLevelTags.values(),
          solderTechnologyTags.value(data.tht_soldering), QVector<Tag>{});
    });
  }
  if (data.smt_soldering != mSmtSolderTechnology) {
    setPreferredFootprintTags([&](Board::PreferredFootprintTags& tags) {
      editPreferredFootprintTags(
          {&tags.smtTop, &tags.smtBot},
          solderTechnologyTags.values() + ipcDensityLevelTags.values(),
          solderTechnologyTags.value(data.smt_soldering), QVector<Tag>{});
    });
  }
  if (data.ipc_density_level != mIpcDensityLevel) {
    setPreferredFootprintTags([&](Board::PreferredFootprintTags& tags) {
      editPreferredFootprintTags(
          {&tags.common}, ipcDensityLevelTags.values(), QVector<Tag>{},
          ipcDensityLevelTags.value(data.ipc_density_level));
    });
  }
}

bool BoardEditor::isRebuildingPlanes() const noexcept {
  return mPlanesBuilder && mPlanesBuilder->isBusy();
}

void BoardEditor::schedulePlanesRebuild() {
  if (mPlanesRebuildTimer) mPlanesRebuildTimer->start();
}

void BoardEditor::startPlanesRebuild(bool force) noexcept {
  if ((!mPlanesBuilder) || force) {
    mPlanesBuilder.reset(new BoardPlaneFragmentsBuilder(this));
    connect(mPlanesBuilder.get(), &BoardPlaneFragmentsBuilder::finished, this,
            [this](BoardPlaneFragmentsBuilder::Result result) {
              if (result.applyToBoard() && result.board) {
                result.board->forceAirWiresRebuild();
                emit planesUpdated();
              }
              mTimestampOfLastPlaneRebuild =
                  QDateTime::currentMSecsSinceEpoch();
              emit planesRebuildStatusChanged();
            });
  }

  if (mPlanesBuilder->isBusy()) {
    return;
  }

  if (mPlanesRebuildTimer) {
    mPlanesRebuildTimer->stop();
  }

  bool started = false;

  if (force) {
    // Forced rebuild -> all layers.
    started = mPlanesBuilder->start(mBoard);
  } else {
    // Automatic rebuild -> only modified & visible layers. However, if the
    // 3D view is open, all planes on outer layers are visible!
    QSet<const Layer*> layers;
    if (!mActive3dTabs.isEmpty()) {
      layers.insert(&Layer::topCopper());
      layers.insert(&Layer::botCopper());
    }
    for (auto tab : mActive2dTabs) {
      if (tab) {
        layers |= tab->getVisibleCopperLayers();
      }
    }
    started = mPlanesBuilder->start(mBoard, &layers);
  }

  if (started) {
    emit planesRebuildStatusChanged();
  }
}

void BoardEditor::startDrc(bool quick) noexcept {
  // Abort any ongoing run.
  mDrc->cancel();

  // Show progress notification during the run.
  mDrcNotification->setTitle(
      (quick ? tr("Running Quick Check") : tr("Running Design Rule Check")) %
      "...");
  mProjectEditor.getApp().getNotifications().push(mDrcNotification);

  // Run the DRC.
  mDrcUndoStackState = mProjectEditor.getUndoStack().getUniqueStateId();
  mDrc->start(mBoard, mBoard.getDrcSettings(), quick);  // can throw
  onUiDataChanged.notify();
}

void BoardEditor::registerActiveTab(Board2dTab* tab) noexcept {
  if (!mActive2dTabs.contains(tab)) {
    mActive2dTabs.append(tab);
    registeredTabsModified();
  }
}

void BoardEditor::unregisterActiveTab(Board2dTab* tab) noexcept {
  mActive2dTabs.removeOne(tab);
  registeredTabsModified();
}

void BoardEditor::registerActiveTab(Board3dTab* tab) noexcept {
  if (!mActive3dTabs.contains(tab)) {
    mActive3dTabs.append(tab);
    registeredTabsModified();
  }
}

void BoardEditor::unregisterActiveTab(Board3dTab* tab) noexcept {
  mActive3dTabs.removeOne(tab);
  registeredTabsModified();
}

void BoardEditor::execBoardSetupDialog(bool switchToDrcSettings) noexcept {
  mProjectEditor.abortBlockingToolsInOtherEditors(this);  // Release undo stack.
  BoardSetupDialog dialog(mProjectEditor.getApp(), mBoard,
                          mProjectEditor.getUndoStack(), qApp->activeWindow());
  if (switchToDrcSettings) {
    dialog.openDrcSettingsTab();
  }
  dialog.exec();
}

void BoardEditor::execStepExportDialog() noexcept {
  // Determine default file path.
  const QString projectName = FilePath::cleanFileName(
      *mProject.getName(), FilePath::ReplaceSpaces | FilePath::KeepCase);
  const QString projectVersion = FilePath::cleanFileName(
      *mProject.getVersion(), FilePath::ReplaceSpaces | FilePath::KeepCase);
  const FilePath defaultFilePath = mProject.getPath().getPathTo(
      QString("output/%1/%2.step").arg(projectVersion, projectName));

  // Ask for file path.
  const FilePath fp(FileDialog::getSaveFileName(
      qApp->activeWindow(), tr("Export STEP Model"), defaultFilePath.toStr(),
      "STEP Models (*.step *.stp)"));
  if (!fp.isValid()) {
    return;
  }

  // Build data.
  auto av = mProject.getCircuit().getAssemblyVariants().value(0);
  auto data = mBoard.buildScene3D(av ? std::make_optional(av->getUuid())
                                     : std::nullopt);

  // Start export.
  StepExport exp;
  QProgressDialog dlg(qApp->activeWindow());
  dlg.setAutoClose(false);
  dlg.setAutoReset(false);
  connect(&exp, &StepExport::progressStatus, &dlg,
          &QProgressDialog::setLabelText);
  connect(&exp, &StepExport::progressPercent, &dlg, &QProgressDialog::setValue);
  connect(&exp, &StepExport::finished, &dlg, &QProgressDialog::close);
  connect(&dlg, &QProgressDialog::canceled, &exp, &StepExport::cancel);
  exp.start(data, fp, 700);
  dlg.exec();
  const QString errorMsg = exp.waitForFinished();
  if (!errorMsg.isEmpty()) {
    QMessageBox::critical(qApp->activeWindow(), tr("STEP Export Failure"),
                          errorMsg);
  }
}

void BoardEditor::prepareOrderPcb() noexcept {
  if (mOrderRequest) {
    return;  // Already prepared.
  }

  // Determine API endpoint.
  const auto ep =
      mProjectEditor.getWorkspace().getSettings().getApiEndpointForOrder();
  if (!ep) {
    mOrderStatus =
        tr("This feature is not available because there is no API server "
           "configured in your workspace settings.");
    onUiDataChanged.notify();
    return;
  }

  // Prepare network request.
  mOrderRequest.reset(new OrderPcbApiRequest(ep->url));
  connect(mOrderRequest.get(), &OrderPcbApiRequest::infoRequestSucceeded, this,
          [this]() { onUiDataChanged.notify(); });
  connect(mOrderRequest.get(), &OrderPcbApiRequest::infoRequestFailed, this,
          [this](QString errorMsg) {
            mOrderStatus = errorMsg;
            onUiDataChanged.notify();
          });
  connect(mOrderRequest.get(), &OrderPcbApiRequest::uploadProgressState, this,
          [this](QString state) {
            mOrderStatus = state;
            onUiDataChanged.notify();
          });
  connect(mOrderRequest.get(), &OrderPcbApiRequest::uploadProgressPercent, this,
          [this](int percent) {
            mOrderUploadProgressPercent = percent;
            onUiDataChanged.notify();
          });
  connect(mOrderRequest.get(), &OrderPcbApiRequest::uploadSucceeded, this,
          [this](QUrl redirectUrl) {
            mOrderStatus = tr("Success! Please continue in the web browser:");
            mOrderUploadProgressPercent = -1;
            onUiDataChanged.notify();
            if (mOrderOpenBrowser) {
              DesktopServices ds(mProjectEditor.getWorkspace().getSettings());
              ds.openUrl(redirectUrl);
            }
          });
  connect(mOrderRequest.get(), &OrderPcbApiRequest::uploadFailed, this,
          [this](QString errorMsg) {
            mOrderStatus = errorMsg;
            mOrderUploadProgressPercent = -1;
            onUiDataChanged.notify();
          });

  // Request status from API server.
  mOrderStatus.clear();
  mOrderRequest->startInfoRequest();
  onUiDataChanged.notify();
}

void BoardEditor::startOrderPcbUpload(bool openBrowser) noexcept {
  if ((!mOrderRequest) || (!mOrderRequest->isReadyForUpload())) {
    return;  // Not prepared.
  }

  try {
    // See explanation in ProjectEditor::execLppzExportDialog(). Unfortunately
    // this way the board is not filtered on unstable releases :-(
    if (Application::isFileFormatStable()) {
      mProject.save();  // can throw
    }

    // Filter out all other boards in a quite ugly way o_o
    // Ignore errors as this is very ugly and error-prone, especially while
    // the file format is unstable.
    QSet<QString> removedBoardDirs;
    if (mProject.getBoards().count() > 1) {
      try {
        const QString boardsFp = "boards/boards.lp";
        std::unique_ptr<SExpression> boardsRoot = SExpression::parse(
            mProject.getDirectory().read(boardsFp),
            mProject.getDirectory().getAbsPath(boardsFp));  // can throw
        for (const SExpression* node : boardsRoot->getChildren("board")) {
          const QString dir =
              QString(node->getChild("@0").getValue()).remove("/board.lp");
          if (dir != mBoard.getDirectory().getPath()) {
            boardsRoot->removeChild(*node);
            mProject.getDirectory().removeDirRecursively(dir);
            removedBoardDirs.insert(dir);
          }
        }
        if (removedBoardDirs.count() != (mProject.getBoards().count() - 1)) {
          throw LogicError(__FILE__, __LINE__);
        }
        mProject.getDirectory().write(boardsFp,
                                      boardsRoot->toByteArray());  // can throw
      } catch (const Exception& e) {
        qCritical() << "Failed to filter out boards:" << e.getMsg();
        removedBoardDirs.clear();
      }
    }

    // Export project to ZIP, but without the output directory since this can
    // be quite large and does not make sense to upload to the API server.
    // Also logs and user settings will not be exported.
    auto filter = [&removedBoardDirs](const QString& filePath) {
      if (filePath.startsWith("output/") || filePath.startsWith("logs/")) {
        return false;
      }
      if (filePath.endsWith(".user.lp")) {
        return false;
      }
      for (const QString& dir : removedBoardDirs) {
        if (filePath.startsWith(dir % "/")) {
          return false;
        }
      }
      return true;
    };
    qDebug() << "Export project to *.lppz for ordering PCBs...";
    const QByteArray lppz =
        mProject.getDirectory().getFileSystem()->exportToZip(
            filter);  // can throw

    // Start upload.
    qDebug() << "Upload *.lppz to API server...";
    mOrderStatus = tr("Uploading project...");
    mOrderUploadProgressPercent = 0;
    mOrderOpenBrowser = openBrowser;
    mOrderRequest->startUpload(lppz, QString());
  } catch (const Exception& e) {
    mOrderStatus = e.getMsg();
  }

  onUiDataChanged.notify();
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

static ui::SolderTechnology determineSolderTechnology(
    const QVector<Tag>& tags) noexcept {
  for (const Tag& tag : tags) {
    if (tag == "hand-soldering") {
      if (tags.contains(Tag("extra-large-pads"))) {
        return ui::SolderTechnology::HandLarge;
      } else {
        return ui::SolderTechnology::Hand;
      }
    } else if (tag == "reflow-soldering") {
      return ui::SolderTechnology::Reflow;
    } else if (tag == "wave-soldering") {
      return ui::SolderTechnology::Wave;
    }
  }
  return ui::SolderTechnology::None;
}

static ui::IpcDensityLevel determineIpcDensityLevel(
    const QVector<Tag>& tags) noexcept {
  for (const Tag& tag : tags) {
    if (tag == "ipc-density-level-a") {
      return ui::IpcDensityLevel::A;
    } else if (tag == "ipc-density-level-b") {
      return ui::IpcDensityLevel::B;
    } else if (tag == "ipc-density-level-c") {
      return ui::IpcDensityLevel::C;
    }
  }
  return ui::IpcDensityLevel::None;
}

template <typename T>
static T extractTagValue(const std::initializer_list<QVector<Tag>>& tags,
                         std::function<T(const QVector<Tag>&)> cb,
                         T fallback) noexcept {
  QSet<T> set;
  for (const QVector<Tag>& t : tags) {
    set.insert(cb(t));
  }
  return (set.count() == 1) ? *set.begin() : fallback;
}

template <typename T>
static bool updateMember(T& member, const T& newValue) noexcept {
  if (member != newValue) {
    member = newValue;
    return true;
  }
  return false;
}

void BoardEditor::updatePreferredFootprintTags() noexcept {
  bool modified = false;

  const Board::PreferredFootprintTags& tags =
      mBoard.getPreferredFootprintTags();
  if (updateMember(mThtSolderTechnology,
                   extractTagValue<ui::SolderTechnology>(
                       {tags.thtTop, tags.thtBot}, &determineSolderTechnology,
                       ui::SolderTechnology::None))) {
    modified = true;
  }
  if (updateMember(mSmtSolderTechnology,
                   extractTagValue<ui::SolderTechnology>(
                       {tags.smtTop, tags.smtBot}, &determineSolderTechnology,
                       ui::SolderTechnology::None))) {
    modified = true;
  }
  if (updateMember(mIpcDensityLevel, determineIpcDensityLevel(tags.common))) {
    modified = true;
  }

  if (modified) {
    onUiDataChanged.notify();
  }
}

void BoardEditor::setDrcResult(
    const BoardDesignRuleCheck::Result& result) noexcept {
  // Detect & remove disappeared messages.
  const QSet<SExpression> approvals =
      RuleCheckMessage::getAllApprovals(result.messages);
  if (mBoard.updateDrcMessageApprovals(approvals, result.quick)) {
    mProjectEditor.setManualModificationsMade();
  }

  // Update UI.
  if (!mDrcMessages) {
    mDrcMessages.reset(new RuleCheckMessagesModel());
    mDrcMessages->setAutofixHandler(std::bind(&BoardEditor::autoFixHandler,
                                              this, std::placeholders::_1,
                                              std::placeholders::_2));
    connect(mDrcMessages.get(), &RuleCheckMessagesModel::unapprovedCountChanged,
            this, [this]() { onUiDataChanged.notify(); });
    connect(mDrcMessages.get(), &RuleCheckMessagesModel::approvalChanged,
            &mBoard, &Board::setDrcMessageApproved);
    connect(mDrcMessages.get(), &RuleCheckMessagesModel::approvalChanged,
            &mProjectEditor, &ProjectEditor::setManualModificationsMade);
    connect(mDrcMessages.get(), &RuleCheckMessagesModel::highlightRequested,
            this, &BoardEditor::drcMessageHighlightRequested);
  }
  mDrcMessages->setMessages(result.messages, mBoard.getDrcMessageApprovals());
  mDrcExecutionError = result.errors.join("\n\n");
  mDrcNotification->dismiss();
  onUiDataChanged.notify();
  emit drcMessageHighlightRequested(nullptr, false);  // Clear markers.
}

void BoardEditor::registeredTabsModified() noexcept {
  mActive2dTabs.removeAll(nullptr);
  mActive3dTabs.removeAll(nullptr);
  if (mActive2dTabs.isEmpty() && mActive3dTabs.isEmpty()) {
    mPlanesRebuildTimer.reset();
    mPlanesBuilder.reset();
  } else if (!mPlanesRebuildTimer) {
    mPlanesRebuildTimer.reset(new QTimer(this));
    connect(mPlanesRebuildTimer.get(), &QTimer::timeout, this,
            &BoardEditor::planesRebuildTimerTimeout);
    mPlanesRebuildTimer->setInterval(100);
  }
  schedulePlanesRebuild();
}

void BoardEditor::planesRebuildTimerTimeout() noexcept {
  // Rebuild planes with a delay in between to avoid too high CPU load caused
  // by too frequent plane rebuilds.
  const qint64 pauseMs =
      QDateTime::currentMSecsSinceEpoch() - mTimestampOfLastPlaneRebuild;
  if (pauseMs >= 1000) {
    startPlanesRebuild(false);
  }
}

/*******************************************************************************
 *  Rule Check Autofixes
 ******************************************************************************/

bool BoardEditor::autoFixHandler(
    const std::shared_ptr<const RuleCheckMessage>& msg,
    bool checkOnly) noexcept {
  try {
    return autoFixImpl(msg, checkOnly);  // can throw
  } catch (const Exception& e) {
    if (!checkOnly) {
      QMessageBox::critical(qApp->activeWindow(), tr("Error"), e.getMsg());
    }
  }
  return false;
}

bool BoardEditor::autoFixImpl(
    const std::shared_ptr<const RuleCheckMessage>& msg, bool checkOnly) {
  if (autoFixHelper<DrcMsgEmptyNetSegment>(msg, checkOnly)) return true;
  if (autoFixHelper<DrcMsgPlaneThermalSpokeWidthIgnored>(msg, checkOnly))
    return true;
  return false;
}

template <typename MessageType>
bool BoardEditor::autoFixHelper(
    const std::shared_ptr<const RuleCheckMessage>& msg, bool checkOnly) {
  if (msg) {
    if (auto m = msg->as<MessageType>()) {
      if (checkOnly) {
        return true;
      } else {
        return autoFix(*m);  // can throw
      }
    }
  }
  return false;
}

template <>
bool BoardEditor::autoFix(const DrcMsgEmptyNetSegment& msg) {
  if (auto ns = mBoard.getNetSegments().value(msg.getUuid())) {
    mProjectEditor.getUndoStack().execCmd(new CmdBoardNetSegmentRemove(*ns));
    return true;
  }
  return false;
}

template <>
bool BoardEditor::autoFix(const DrcMsgPlaneThermalSpokeWidthIgnored& msg) {
  if (auto plane = mBoard.getPlanes().value(msg.getUuid())) {
    std::unique_ptr<CmdBoardPlaneEdit> cmd(new CmdBoardPlaneEdit(*plane));
    cmd->setThermalSpokeWidth(PositiveLength(*plane->getMinWidth()));
    mProjectEditor.getUndoStack().execCmd(cmd.release());
    return true;
  }
  return false;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
