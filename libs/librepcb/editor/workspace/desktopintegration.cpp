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
#include "desktopintegration.h"

#include <librepcb/core/application.h>
#include <librepcb/core/fileio/fileutils.h>
#include <librepcb/core/utils/toolbox.h>
#include <librepcb_build_env.h>

#include <QtCore>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Static Methods
 ******************************************************************************/

bool DesktopIntegration::isSupported() noexcept {
#if LIBREPCB_ENABLE_DESKTOP_INTEGRATION && defined(Q_OS_LINUX)
  // Only enable this feature if the desktop integration is not already
  // managed by our installer.
  const FilePath appFp(qApp->applicationFilePath());
  const QString mimeFp =
      "../../registerfileextensions/mime/librepcb-from-installer.xml";
  return !appFp.getPathTo(mimeFp).isExistingFile();
#else
  return false;
#endif
}

DesktopIntegration::Status DesktopIntegration::getStatus() noexcept {
  const FilePath configFp = getConfigFile();
  if (!configFp.isExistingFile()) {
    return Status::NothingInstalled;
  }
  const FilePath desktopFp = getDesktopFile();
  if (desktopFp.isExistingFile()) {
    QSettings desktopFile(desktopFp.toStr(), QSettings::IniFormat);
    if (desktopFile.value("Desktop Entry/Exec")
            .toString()
            .contains(getExecutable().toNative())) {
      return Status::InstalledThis;
    } else {
      return Status::InstalledOther;
    }
  } else {
    return Status::InstalledUnknown;
  }
}

FilePath DesktopIntegration::getExecutable() noexcept {
  static const QString appimage = qgetenv("APPIMAGE").trimmed();
  if (!appimage.isEmpty()) {
    return FilePath(appimage);
  } else {
    return FilePath(qApp->applicationFilePath());
  }
}

void DesktopIntegration::install() {
  QSet<FilePath> installedFiles = loadInstalledFiles();
  try {
    const QHash<FilePath, QByteArray> files = getFileContentToInstall();
    for (auto it = files.begin(); it != files.end(); it++) {
      qDebug().nospace() << "Create " << it.key().toNative() << "...";
      FileUtils::writeFile(it.key(), it.value());  // can throw
      installedFiles.insert(it.key());
    }
    storeInstalledFiles(installedFiles);  // can throw
    updateDatabase();  // can throw
  } catch (...) {
    storeInstalledFiles(installedFiles);  // can throw
    updateDatabase();  // can throw
    throw;
  }
}

void DesktopIntegration::uninstall() {
  qInfo() << "Unregister application...";

  QSet<FilePath> files = loadInstalledFiles();
  foreach (const FilePath& fp, files) {
    QFile f(fp.toStr());
    if ((!f.exists()) || f.remove()) {
      qDebug().nospace() << "Removed " << fp.toNative() << ".";
      files.remove(fp);
    } else {
      qCritical().nospace() << "Failed to remove " << fp.toNative() << ".";
    }
  }
  storeInstalledFiles(files);  // can throw
  updateDatabase();  // can throw
}

bool DesktopIntegration::execDialog(Mode mode, QWidget* parent) noexcept {
  try {
    QDialog dialog(parent);
    QVBoxLayout* layout = new QVBoxLayout(&dialog);
    QLabel* lblIntro = new QLabel(&dialog);
    lblIntro->setWordWrap(true);
    lblIntro->setTextInteractionFlags(Qt::TextSelectableByMouse);
    layout->addWidget(lblIntro);
    QTextEdit* edtFiles = new QTextEdit(&dialog);
    edtFiles->setReadOnly(true);
    edtFiles->setLineWrapMode(QTextEdit::NoWrap);
    layout->addWidget(edtFiles);
    QLabel* lblAppendix = new QLabel(&dialog);
    lblAppendix->setWordWrap(true);
    lblAppendix->setText(
        tr("To avoid troubles, only proceed if there are no other (installed) "
           "LibrePCB applications on this computer."));
    layout->addWidget(lblAppendix);
    QDialogButtonBox* btnBox = new QDialogButtonBox(&dialog);
    btnBox->setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    QObject::connect(btnBox, &QDialogButtonBox::accepted, &dialog,
                     &QDialog::accept);
    QObject::connect(btnBox, &QDialogButtonBox::rejected, &dialog,
                     &QDialog::reject);
    layout->addWidget(btnBox);

    QSet<QString> files;
    if (mode == Mode::Install) {
      dialog.setWindowTitle(tr("Install Desktop Integration"));
      lblIntro->setText(tr("This installs the following files to register the "
                           "executable <i>%1</i>:")
                            .arg(getExecutable().toNative()));
      files.insert(getConfigFile().toNative());
      foreach (const FilePath& fp, getFileContentToInstall().keys()) {
        files.insert(fp.toNative());
      }
    } else {
      dialog.setWindowTitle(tr("Uninstall Desktop Integration"));
      lblIntro->setText(tr("This removes the following files:"));
      foreach (const FilePath& fp, loadInstalledFiles()) {
        files.insert(fp.toNative());
      }
      const FilePath configFp = getConfigFile();
      if (configFp.isExistingFile()) {
        files.insert(configFp.toNative());
      }
    }
    edtFiles->setText(Toolbox::sortedQSet(files).join("\n"));
    edtFiles->setMinimumWidth(
        static_cast<int>(edtFiles->document()->size().width()) + 30);
    edtFiles->verticalScrollBar()->setValue(0);

    if (dialog.exec() == QDialog::Accepted) {
      if (mode == Mode::Install) {
        install();  // can throw
      } else {
        uninstall();  // can throw
      }
      return true;
    }
  } catch (const Exception& e) {
    QMessageBox::critical(parent, tr("Error"), e.getMsg());
  }
  return false;
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

QHash<FilePath, QByteArray> DesktopIntegration::getFileContentToInstall() {
  QHash<FilePath, QByteArray> content;
  const FilePath dst = getShareDirectory();
  const FilePath src = Application::getResourcesDir().getParentDir();
  qInfo().nospace() << "Register application from " << src.toNative() << " to "
                    << dst.toNative() << "...";

  // Desktop file.
  const QString desktopFile = "applications/org.librepcb.LibrePCB.desktop";
  const FilePath desktopSrc = src.getPathTo(desktopFile);
  const FilePath desktopDst = dst.getPathTo(desktopFile);
  QStringList lines = QString(FileUtils::readFile(desktopSrc)).split("\n");
  for (QString& line : lines) {
    if (line.startsWith("Exec=")) {
      line = QString("Exec=%1 %U")
                 .arg(getExecutable().toStr().replace(" ", "\\s"));
    }
  }
  content[desktopDst] = lines.join("\n").toUtf8() % "\n";

  // All other files.
  const QList<FilePath> srcFiles =
      FileUtils::getFilesInDirectory(src.getPathTo("icons"), {}, true) +
      FileUtils::getFilesInDirectory(src.getPathTo("mime"), {}, true);
  foreach (const FilePath& srcFile, srcFiles) {
    const FilePath dstFile = dst.getPathTo(srcFile.toRelative(src));
    content[dstFile] = FileUtils::readFile(srcFile);
  }

  return content;
}

QSet<FilePath> DesktopIntegration::loadInstalledFiles() {
  QSet<FilePath> files;
  const FilePath fp = getConfigFile();
  if (fp.isExistingFile()) {
    foreach (QString line, QString(FileUtils::readFile(fp)).split("\n")) {
      FilePath lineFp(line.trimmed());
      if (lineFp.isValid()) {
        files.insert(lineFp);
      }
    }
  }
  return files;
}

void DesktopIntegration::storeInstalledFiles(const QSet<FilePath>& files) {
  QString s;
  foreach (const FilePath& fp, Toolbox::sortedQSet(files)) {
    s += fp.toStr() % "\n";
  }

  const FilePath fp = getConfigFile();
  if (files.isEmpty() && fp.isExistingFile()) {
    qDebug().nospace() << "Remove " << fp.toNative() << "...";
    FileUtils::removeFile(fp);
  } else if (!files.isEmpty()) {
    qDebug().nospace() << "Create " << fp.toNative() << "...";
    FileUtils::writeFile(fp, s.toUtf8());
  }
}

void DesktopIntegration::updateDatabase() {
  const QString exe1 = "update-desktop-database";
  const QString arg1 = getShareDirectory().getPathTo("applications").toStr();
  qDebug().noquote() << "Run command:" << exe1 << arg1;
  const int ret1 = QProcess::execute(exe1, {arg1});

  const QString exe2 = "update-mime-database";
  const QString arg2 = getShareDirectory().getPathTo("mime").toStr();
  qDebug().noquote() << "Run command:" << exe2 << arg2;
  const int ret2 = QProcess::execute(exe2, {arg2});

  const QString msg =
      tr("Failed to run '%1'.\n\nPlease make sure this tool is available in "
         "PATH.");
  if (ret1 != 0) {
    throw RuntimeError(__FILE__, __LINE__, msg.arg(exe1 % " " % arg1));
  }
  if (ret2 != 0) {
    throw RuntimeError(__FILE__, __LINE__, msg.arg(exe2 % " " % arg2));
  }
}

FilePath DesktopIntegration::getDesktopFile() noexcept {
  return getShareDirectory().getPathTo(
      "applications/org.librepcb.LibrePCB.desktop");
}

FilePath DesktopIntegration::getConfigFile() noexcept {
  return getShareDirectory().getPathTo("librepcb/installation.txt");
}

FilePath DesktopIntegration::getShareDirectory() noexcept {
  return FilePath(
      QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation));
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
