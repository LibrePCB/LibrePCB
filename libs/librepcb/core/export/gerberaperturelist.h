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

#ifndef LIBREPCB_COMMON_GERBERAPERTURELIST_H
#define LIBREPCB_COMMON_GERBERAPERTURELIST_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../exceptions.h"
#include "../fileio/filepath.h"
#include "../geometry/path.h"
#include "../units/length.h"
#include "../uuid.h"
#include "gerberattribute.h"

#include <QtCore>

#include <optional.hpp>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Angle;

/*******************************************************************************
 *  Class GerberApertureList
 ******************************************************************************/

/**
 * @brief A helper class to generate the aperture definitions for a Gerber file
 *
 * The class provides methods to add certain apertures. Identical Apertures are
 * added only once, i.e. if you call #addCircle() multiple times with a
 * diameter of 1mm, only one circle aperture of 1mm is created.
 *
 * In addition, methods will always create the most simple aperture which
 * represents the desired image. For example, if you call #addObround() with
 * both width and height set to the same value, a circle aperture is added
 * instead of an obround (and the rotation parameter is ignored).
 *
 * @warning The implementation of this class is very critical for generating
 *          correct Gerber files widely compatible with CAM software used by
 *          PCB fabricators. A lot of know how is contained in the implementtion
 *          to avoid issues with PCB fabricators. When changing anything here,
 *          read the Gerber specs very carefully, follow their recommendations
 *          and try to determine the compatibility with CAM software like
 *          CAM350 or Generis2000. In addition, add unit tests for each new
 *          requirement.
 */
class GerberApertureList final {
  Q_DECLARE_TR_FUNCTIONS(GerberApertureList)

public:
  // Types
  using Function = tl::optional<GerberAttribute::ApertureFunction>;

  // Constructors / Destructor
  GerberApertureList() noexcept;
  GerberApertureList(const GerberApertureList& other) = delete;
  ~GerberApertureList() noexcept;

  // Getters

  /**
   * @brief Generate the aperture definitions string
   *
   * @return String containing 0..n lines
   */
  QString generateString() const noexcept;

  // General Methods

  /**
   * @brief Add a circle aperture
   *
   * @param dia       Circle diameter. According Gerber specs, it's allowed to
   *                  create a circle with a diameter of zero.
   * @param function  Function attribute.
   *
   * @return Aperture number.
   */
  int addCircle(const UnsignedLength& dia, Function function);

  /**
   * @brief Add an obround aperture
   *
   * @note If w==h, a circle aperture will be created.
   *
   * @param w         Total width.
   * @param h         Total height.
   * @param rot       Rotation.
   * @param function  Function attribute.
   *
   * @return Aperture number.
   */
  int addObround(const PositiveLength& w, const PositiveLength& h,
                 const Angle& rot, Function function) noexcept;

  /**
   * @brief Add a rectangular aperture
   *
   * @param w         Width.
   * @param h         Height.
   * @param rot       Rotation.
   * @param function  Function attribute.
   *
   * @return Aperture number.
   */
  int addRect(const PositiveLength& w, const PositiveLength& h,
              const Angle& rot, Function function) noexcept;

  /**
   * @brief Add an octagon aperture
   *
   * @param w         Width.
   * @param h         Height.
   * @param rot       Rotation.
   * @param function  Function attribute.
   *
   * @return Aperture number.
   */
  int addOctagon(const PositiveLength& w, const PositiveLength& h,
                 const Angle& rot, Function function) noexcept;

  // Operator Overloadings
  GerberApertureList& operator=(const GerberApertureList& rhs) = delete;

private:  // Methods
  /**
   * @brief Add a custom outline aperture
   *
   * @note  This is private because it does not implement proper error handling
   *        yet, so you could create invalid Gerber files when passing invalid
   *        parameters! Let's implement proper error handling once we need to
   *        make it public.
   *
   * @param name      Macro name (use only characters A..Z!).
   * @param path      The vertices. ATTENTION: After closing the path, it must
   *                  contain at least 4 vertices and it must not contain any
   *                  arc segment (i.e. all angles must be zero)!!!
   * @param rot       Rotation.
   * @param function  Function attribute.
   *
   * @return Aperture number.
   */
  int addOutline(const QString& name, Path path, const Angle& rot,
                 Function function) noexcept;

  /**
   * @brief Helper method to actually add a new or get an existing aperture
   *
   * @note If the same aperture already exists, nothing is added and the
   *       number of the existing aperture is returned.
   *
   * @param aperture    The full content of the aperture to add (except the
   *                    X2 attributes).
   * @param function    Function attribute.
   *
   * @return Aperture number.
   */
  int addAperture(QString aperture, Function function) noexcept;

private:  // Data
  /// Added apertures
  ///
  /// - key:    Aperture number (>= 10).
  /// - value:  Aperture function and definition, with the placeholder "{}"
  ///           instead of the aperture number. Needs to be substituted by the
  ///           aperture number when serializing.
  QMap<int, std::pair<Function, QString>> mApertures;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
