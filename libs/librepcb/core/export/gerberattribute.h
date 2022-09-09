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

#ifndef LIBREPCB_CORE_GERBERATTRIBUTE_H
#define LIBREPCB_CORE_GERBERATTRIBUTE_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Angle;
class Uuid;

/*******************************************************************************
 *  Class GerberAttribute
 ******************************************************************************/

/**
 * @brief A Gerber X2 attribute
 */
class GerberAttribute final {
  Q_DECLARE_TR_FUNCTIONS(GerberAttribute)

public:
  // Types
  enum class Type { Invalid, File, Aperture, Object, Delete };
  enum class Polarity { Positive, Negative };
  enum class BoardSide { Top, Bottom };
  enum class CopperSide { Top, Inner, Bottom };
  enum class MountType { Tht, Smt, Fiducial, Other };
  enum class ApertureFunction {
    // Available on all layers:
    Profile,  ///< Board outline

    // Available only on drill/rout layers:
    ViaDrill,  ///< Drill of a via (usually plated)
    ComponentDrill,  ///< Drill for component pads (usually plated)
    MechanicalDrill,  ///< Drill for mechanical purpose (usually not plated)

    // Available only on copper layers:
    Conductor,  ///< Copper with electrical function
    NonConductor,  ///< Copper without electrical function
    ComponentPad,  ///< THT pad
    SmdPadCopperDefined,  ///< SMT pad, copper-defined
    SmdPadSolderMaskDefined,  ///< SMT pad, stopmask-defined
    ViaPad,  ///< Via

    // Available only on component layers:
    ComponentMain,  ///< Center of component
    ComponentPin,  ///< Component pin
    ComponentOutlineBody,  ///< Component body outline
    ComponentOutlineCourtyard,  ///< Component courtyard outline
  };

  // Constructors / Destructor
  GerberAttribute() noexcept;
  GerberAttribute(const GerberAttribute& other) noexcept;
  ~GerberAttribute() noexcept;

  // Getters
  Type getType() const noexcept { return mType; }
  const QString& getKey() const noexcept { return mKey; }
  const QStringList& getValues() const noexcept { return mValues; }

  // General Methods
  QString toGerberString() const noexcept;
  QString toExcellonString() const noexcept;

  // Operator Overloadings
  GerberAttribute& operator=(const GerberAttribute& rhs) noexcept;
  bool operator==(const GerberAttribute& rhs) const noexcept;

  // Static Methods
  static GerberAttribute unset(const QString& key) noexcept;
  static GerberAttribute fileGenerationSoftware(
      const QString& vendor, const QString& application,
      const QString& version) noexcept;
  static GerberAttribute fileCreationDate(const QDateTime& date) noexcept;
  static GerberAttribute fileProjectId(const QString& name, const Uuid& uuid,
                                       const QString& revision) noexcept;
  static GerberAttribute filePartSingle() noexcept;
  static GerberAttribute fileSameCoordinates(
      const QString& identifier) noexcept;
  static GerberAttribute fileFunctionProfile(bool plated) noexcept;
  static GerberAttribute fileFunctionCopper(int layer,
                                            CopperSide side) noexcept;
  static GerberAttribute fileFunctionSolderMask(BoardSide side) noexcept;
  static GerberAttribute fileFunctionLegend(BoardSide side) noexcept;
  static GerberAttribute fileFunctionPaste(BoardSide side) noexcept;
  static GerberAttribute fileFunctionPlatedThroughHole(int fromLayer,
                                                       int toLayer) noexcept;
  static GerberAttribute fileFunctionNonPlatedThroughHole(int fromLayer,
                                                          int toLayer) noexcept;
  static GerberAttribute fileFunctionMixedPlating(int fromLayer,
                                                  int toLayer) noexcept;
  static GerberAttribute fileFunctionComponent(int layer,
                                               BoardSide side) noexcept;
  static GerberAttribute filePolarity(Polarity polarity) noexcept;
  static GerberAttribute fileMd5(const QString& md5) noexcept;
  static GerberAttribute apertureFunction(ApertureFunction function) noexcept;
  static GerberAttribute apertureFunctionMixedPlatingDrill(
      bool plated, ApertureFunction function) noexcept;
  static GerberAttribute objectNet(const QString& net) noexcept;
  static GerberAttribute objectComponent(const QString& component) noexcept;
  static GerberAttribute objectPin(const QString& component, const QString& pin,
                                   const QString& signal) noexcept;
  static GerberAttribute componentRotation(const Angle& rotation) noexcept;
  static GerberAttribute componentManufacturer(
      const QString& manufacturer) noexcept;
  static GerberAttribute componentMpn(const QString& mpn) noexcept;
  static GerberAttribute componentValue(const QString& value) noexcept;
  static GerberAttribute componentMountType(MountType type) noexcept;
  static GerberAttribute componentFootprint(const QString& footprint) noexcept;

private:  // Methods
  GerberAttribute(Type type, const QString& key,
                  const QStringList& values) noexcept;
  QString toString() const noexcept;
  static QString escapeValue(const QString& value, bool strictAscii) noexcept;

private:  // Data
  Type mType;
  QString mKey;
  QStringList mValues;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
