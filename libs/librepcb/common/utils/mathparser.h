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

#ifndef LIBREPCB_MATHPARSER_H
#define LIBREPCB_MATHPARSER_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class MathParser
 ******************************************************************************/

/**
 * @brief Mathematical expression parser
 *
 * This class interprets mathematical expression strings (e.g. "2+3") and
 * returns the result of the calculation. It is actually only a wrapper around
 * the [muparser](https://beltoforion.de/article.php?a=muparser) library, so
 * take a look at its documentation for details.
 */
class MathParser {
  Q_DECLARE_TR_FUNCTIONS(MathParser)

public:
  struct Result {
    bool    valid;
    qreal   value;
    QString error;

    Result() : valid(false), value(0), error() {}
  };

  // Constructors / Destructor
  MathParser() noexcept;
  MathParser(const MathParser& other) = delete;
  virtual ~MathParser() noexcept;

  // General Methods

  /**
   * @brief Set the locale to be used for parsing numbers
   *
   * This sets the thousand separator and decimal point to be used for the
   * evaluation.
   *
   * @param locale  The locale to use.
   */
  void setLocale(const QLocale& locale) noexcept;

  /**
   * @brief Parse expression
   *
   * @param expression  The expression to parse.
   *
   * @return  The result, either valid with a value, or invalid with an error
   *          message.
   */
  Result parse(const QString& expression) const noexcept;

  // Operator Overloadings
  MathParser& operator=(const MathParser& rhs) = delete;

private:
  QLocale mLocale;  ///< The locale used for parsing numbers
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif  // LIBREPCB_MATHPARSER_H
