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
#include <librepcb/core/rulecheck/rulecheckmessage.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class FilePath;
class Library;
class LibraryBaseElement;
class SExpression;
class TransactionalFileSystem;
class TranslationsCatalog;

namespace cli {

/*******************************************************************************
 *  Class CommandLineInterface
 ******************************************************************************/

/**
 * @brief The CommandLineInterface class
 */
class CommandLineInterface final {
  Q_DECLARE_TR_FUNCTIONS(CommandLineInterface)

public:
  // Constructors / Destructor
  CommandLineInterface() noexcept;
  ~CommandLineInterface() noexcept = default;

  // General Methods
  int execute(const QStringList& args) noexcept;

private:  // Methods
  // Check result structure
  struct CheckResult {
    int approvedMsgCount;
    QStringList nonApprovedMessages;
  };

  bool openProject(
      const QString& projectFile, bool runErc, bool runDrc,
      const QString& drcSettingsPath, const QStringList& runJobs,
      bool runAllJobs, const QString& customJobsPath,
      const QString& customOutDir, const QStringList& exportSchematicsFiles,
      const QStringList& exportBomFiles, const QStringList& exportBoardBomFiles,
      const QString& bomAttributes, bool exportPcbFabricationData,
      const QString& pcbFabricationSettingsPath,
      const QStringList& exportPnpTopFiles,
      const QStringList& exportPnpBottomFiles,
      const QStringList& exportNetlistFiles, const QStringList& boardNames,
      const QStringList& boardIndices, bool removeOtherBoards,
      const QStringList& avNames, const QStringList& avIndices,
      const QString& setDefaultAv, bool save, bool strict) const noexcept;
  bool openLibraries(const QStringList& libDirs, bool all, bool runCheck,
                     const QString& exportTranslationsFile,
                     bool minifyStepFiles, bool save,
                     bool strict) const noexcept;
  bool openLibrary(const QString& libDir, bool all, bool runCheck,
                   TranslationsCatalog* exportTranslationsCatalog,
                   bool minifyStepFiles, bool save, bool strict) const noexcept;

  /**
   * @brief Gather validation check messages for a library element
   *
   * This function runs validation checks on a library element and separates
   * the results into approved and non-approved messages.
   *
   * @param element The library element to check
   * @return CheckResult containing the count of approved messages and a list
   *         of non-approved messages
   */
  CheckResult gatherElementCheckMessages(
      const LibraryBaseElement& element) const;

  // Format check summary with optional header
  QStringList formatLibraryElementCheckSummary(
      const CheckResult& checkResult) const;
  QStringList formatCheckSummary(int approvedCount, int nonApprovedCount,
                                 const QString& indent = "") const;

  void processLibraryElement(const QString& libDir, const Library& lib,
                             TransactionalFileSystem& fs,
                             LibraryBaseElement& element,
                             TranslationsCatalog* exportTranslationsCatalog,
                             bool runCheck, bool minifyStepFiles, bool save,
                             bool strict, bool& success) const;
  bool openSymbol(const QString& symbolFile, bool runCheck,
                  const QString& exportFile) const noexcept;
  bool openPackage(const QString& packageFile, bool runCheck,
                   const QString& exportFile) const noexcept;
  bool openStep(const QString& filePath, bool minify, bool tesselate,
                const QString& saveTo) const noexcept;
  static QStringList prepareRuleCheckMessages(
      RuleCheckMessageList messages, const QSet<SExpression>& approvals,
      int& approvedMsgCount) noexcept;
  static QString prettyPath(const FilePath& path,
                            const QString& style) noexcept;
  static bool failIfFileFormatUnstable() noexcept;
  static void print(const QString& str) noexcept;
  static void printErr(const QString& str) noexcept;
  static bool suppressDeprecationWarnings() noexcept;
  static void printDeprecationWarning(const QString& deprecatedCommand,
                                      const QString& newCommand) noexcept;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace cli
}  // namespace librepcb

#endif
