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
#include "outputjobhomewidget.h"

#include "../../workspace/desktopservices.h"
#include "ui_outputjobhomewidget.h"

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

OutputJobHomeWidget::OutputJobHomeWidget(const WorkspaceSettings& settings,
                                         const Project& project,
                                         QWidget* parent) noexcept
  : QWidget(parent),
    mSettings(settings),
    mProject(project),
    mUi(new Ui::OutputJobHomeWidget) {
  mUi->setupUi(this);

  const FilePath outDir = mProject.getCurrentOutputDir();
  const QString relOutDir =
      mProject.getCurrentOutputDir().toRelativeNative(mProject.getPath());

  QString info;
  info += "<p>" %
      tr("Output jobs allow you to generate any production data, documentation "
         "or other output files in a unified, reproducible way. Since their "
         "configuration is stored in the project, the complete output can even "
         "be generated headless with the <a href=\"%1\">LibrePCB CLI</a>.")
          .arg("https://librepcb.org/docs/cli/") %
      "</p>";
  info += "<p>" %
      tr("For each project version, a separate output directory is created.") %
      "<br>" %
      tr("The current output directory is <a href=\"%1\">%2</a>.")
          .arg(outDir.toQUrl().toString())
          .arg("." % QDir::separator() % relOutDir % QDir::separator()) %
      "</p>";
  info += "<p><b>" %
      tr("Previously generated files which are not generated anymore will "
         "automatically be deleted!") %
      "</b> " %
      tr("To detect them, a file named <tt>%1</tt> is created within the "
         "output directory.")
          .arg(".librepcb-output") %
      "</p>";
  mUi->lblInfo->setText(info);
  connect(mUi->lblInfo, &QLabel::linkActivated, this,
          [this](const QString& url) {
            DesktopServices ds(mSettings);
            ds.openUrl(QUrl(url));
          });
}

OutputJobHomeWidget::~OutputJobHomeWidget() noexcept {
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
