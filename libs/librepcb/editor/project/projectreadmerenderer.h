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

#ifndef LIBREPCB_EDITOR_PROJECTREADMERENDERER_H
#define LIBREPCB_EDITOR_PROJECTREADMERENDERER_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/core/fileio/filepath.h>

#include <QtCore>
#include <QtGui>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Class ProjectReadmeRenderer
 ******************************************************************************/

/**
 * @brief Renders a README.md or other file types as a QPixmap
 */
class ProjectReadmeRenderer : public QObject {
  Q_OBJECT

public:
  // Constructors / Destructor
  ProjectReadmeRenderer() = delete;
  ProjectReadmeRenderer(const ProjectReadmeRenderer& other) = delete;
  explicit ProjectReadmeRenderer(QObject* parent = nullptr) noexcept;
  virtual ~ProjectReadmeRenderer() noexcept;

  // General Methods
  void request(const FilePath& fp, int width) noexcept;

  // Operator Overloadings
  ProjectReadmeRenderer& operator=(const ProjectReadmeRenderer& rhs) = delete;

signals:
  void runningChanged(bool running);
  void finished(const QPixmap& result);

private:
  void start() noexcept;
  static QPixmap render(const FilePath& fp, int width) noexcept;

private:
  FilePath mPath;
  int mWidth;
  QTimer mDelayTimer;
  std::unique_ptr<QFutureWatcher<QPixmap>> mWatcher;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
