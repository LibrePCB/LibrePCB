/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
 * http://librepcb.org/
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

#ifndef LIBREPCB_PROJECT_SCHEMATICSELECTIONQUERY_H
#define LIBREPCB_PROJECT_SCHEMATICSELECTIONQUERY_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/common/exceptions.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace project {

class SI_Symbol;
class SI_SymbolPin;
class SI_NetSegment;
class SI_NetLine;
class SI_NetPoint;
class SI_NetLabel;

/*******************************************************************************
 *  Class SchematicSelectionQuery
 ******************************************************************************/

/**
 * @brief The SchematicSelectionQuery class
 */
class SchematicSelectionQuery final : public QObject {
  Q_OBJECT

public:
  // Constructors / Destructor
  SchematicSelectionQuery()                                     = delete;
  SchematicSelectionQuery(const SchematicSelectionQuery& other) = delete;
  SchematicSelectionQuery(const QList<SI_Symbol*>&     symbols,
                          const QList<SI_NetSegment*>& netsegments,
                          QObject*                     parent = nullptr);
  ~SchematicSelectionQuery() noexcept;

  // Getters
  const QSet<SI_Symbol*>& getSymbols() const noexcept { return mResultSymbols; }
  const QSet<SI_NetPoint*>& getNetPoints() const noexcept {
    return mResultNetPoints;
  }
  const QSet<SI_NetLine*>& getNetLines() const noexcept {
    return mResultNetLines;
  }
  const QSet<SI_NetLabel*>& getNetLabels() const noexcept {
    return mResultNetLabels;
  }
  int  getResultCount() const noexcept;
  bool isResultEmpty() const noexcept { return (getResultCount() == 0); }

  // General Methods
  void addSelectedSymbols() noexcept;
  void addSelectedNetPoints() noexcept;
  void addSelectedNetLines() noexcept;
  void addSelectedNetLabels() noexcept;
  void addNetPointsOfNetLines() noexcept;
  void addNetLinesOfSymbolPins() noexcept;

  // Operator Overloadings
  SchematicSelectionQuery& operator=(const SchematicSelectionQuery& rhs) =
      delete;

private:
  // references to the Schematic object
  const QList<SI_Symbol*>&     mSymbols;
  const QList<SI_NetSegment*>& mNetSegments;

  // query result
  QSet<SI_Symbol*>   mResultSymbols;
  QSet<SI_NetPoint*> mResultNetPoints;
  QSet<SI_NetLine*>  mResultNetLines;
  QSet<SI_NetLabel*> mResultNetLabels;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace project
}  // namespace librepcb

#endif  // LIBREPCB_PROJECT_SCHEMATICSELECTIONQUERY_H
