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

#ifndef LIBREPCB_CLI_COMMANDLINEINTERFACE_H
#define LIBREPCB_CLI_COMMANDLINEINTERFACE_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Application;
class FilePath;

namespace cli {

/*******************************************************************************
 *  Class CommandLineInterface
 ******************************************************************************/

/**
 * @brief The CommandLineInterface class
 */
class CommandLineInterface final {
  Q_DECLARE_TR_FUNCTIONS(CommandLineInterface);

public:
  // Constructors / Destructor
  CommandLineInterface() = delete;
  explicit CommandLineInterface(const Application& app) noexcept;
  ~CommandLineInterface() noexcept = default;

  // General Methods
  int execute() noexcept;

private:  // Methods
  bool openProject(const QString& projectFile, bool runErc,
                   const QStringList& exportSchematicsFiles,
                   bool               exportPcbFabricationData,
                   const QString&     pcbFabricationSettingsPath,
                   const QStringList& boards, bool save) const noexcept;
  bool openLibrary(const QString& libDir, bool all, bool save) const noexcept;
  static QString prettyPath(const FilePath& path,
                            const QString&  style) noexcept;
  static void    print(const QString& str, int newlines = 1) noexcept;
  static void    printErr(const QString& str, int newlines = 1) noexcept;

private:  // Data
  const Application& mApp;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace cli
}  // namespace librepcb

#endif  // LIBREPCB_CLI_COMMANDLINEINTERFACE_H
