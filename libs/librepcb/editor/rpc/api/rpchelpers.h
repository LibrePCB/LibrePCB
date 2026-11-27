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

#ifndef LIBREPCB_EDITOR_RPCHELPERS_H
#define LIBREPCB_EDITOR_RPCHELPERS_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../rpcrequest.h"
#include "rpcerrors.h"
#include "rpctypes.h"

#include <librepcb/core/attribute/attribute.h>
#include <librepcb/core/attribute/attributetype.h>
#include <librepcb/core/attribute/attributeunit.h>
#include <librepcb/core/fileio/filepath.h>
#include <librepcb/core/geometry/circle.h>
#include <librepcb/core/geometry/hole.h>
#include <librepcb/core/geometry/polygon.h>
#include <librepcb/core/geometry/stroketext.h>
#include <librepcb/core/geometry/text.h>
#include <librepcb/core/geometry/zone.h>
#include <librepcb/core/library/pkg/footprintpad.h>
#include <librepcb/core/library/resource.h>
#include <librepcb/core/library/sym/symbolpin.h>
#include <librepcb/core/types/elementname.h>
#include <librepcb/core/types/fileproofname.h>
#include <librepcb/core/types/uuid.h>
#include <type_traits>

#include <QtCore>

#include <optional>
#include <slint.h>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {
namespace rpc {

/*******************************************************************************
 *  RPC Helpers
 ******************************************************************************/

inline QVariant toPrivacyPath(const FilePath& fp, const FilePath& base) {
  if (fp.isValid() && base.isValid() && fp.isLocatedInDir(base)) {
    return fp.toRelative(base);
  } else {
    return fp.toStr();
  }
}

/// Convert a string list into the protocol's list-of-strings
/// representation (QStringList is not part of the protocol's value
/// domain, see the RPC API specification).
inline QVariant toVariant(const QStringList& list) noexcept {
  return QVariant(list).value<QVariantList>();
}

inline QVariant toVariant(const QList<QByteArray>& list) noexcept {
  QVariantList l;
  for (const auto& item : list) {
    l.append(item);
  }
  return l;
}

template <typename T, typename F>
inline QVariantList toVariant(const T& list, F convertItem) {
  QVariantList result;
  for (const auto& obj : list) {
    result.append(convertItem(obj));
  }
  return result;
}

/// Convert a Uuid into its canonical 36-character string form. Note that
/// the protocol uses no msgpack extension types — UUIDs are plain strings.
inline QVariant toVariant(const Uuid& obj) noexcept {
  return obj.toStr();
}

inline QVariant toVariant(const std::optional<Uuid>& obj) noexcept {
  return obj ? toVariant(*obj) : QVariant();
}

inline QVariant toVariant(const FilePath& obj) noexcept {
  return obj.isValid() ? obj.toStr() : QVariant();
}

inline QVariant toVariant(const Length& obj) noexcept {
  return static_cast<qint64>(obj.toNm());
}

inline QVariant toVariant(const Point& obj) noexcept {
  return QVariantList{
      toVariant(obj.getX()),
      toVariant(obj.getY()),
  };
}

inline QVariant toVariant(const Angle& obj) noexcept {
  return obj.toMicroDeg();
}

inline QVariant toVariant(const Ratio& obj) noexcept {
  return obj.toPpm();
}

inline QVariant toVariant(const Vertex& obj) noexcept {
  return QVariantList{
      toVariant(obj.getPos().getX()),
      toVariant(obj.getPos().getY()),
      toVariant(obj.getAngle()),
  };
}

inline QVariant toVariant(const Attribute& obj) {
  return QVariantMap{
      {"name", *obj.getKey()},
      {"type", obj.getType().getName()},
      {"value", obj.getValue()},
      {"unit", obj.getUnit() ? obj.getUnit()->getName() : QVariant()},
  };
}

inline QVariant toVariant(const Path& obj) noexcept {
  return toVariant(obj.getVertices(),
                   [](const Vertex& obj) { return toVariant(obj); });
}

inline QVariant toVariant(const HAlign& obj) noexcept {
  if (obj == HAlign::left()) {
    return "left";
  } else if (obj == HAlign::right()) {
    return "right";
  } else {
    return "center";
  }
}

inline QVariant toVariant(const VAlign& obj) noexcept {
  if (obj == VAlign::top()) {
    return "top";
  } else if (obj == VAlign::bottom()) {
    return "bottom";
  } else {
    return "center";
  }
}

inline QVariant toVariant(const Alignment& obj) noexcept {
  const auto h = toVariant(obj.getH());
  const auto v = toVariant(obj.getV());
  return (h == v) ? h : (h.toString() + "_" + v.toString());
}

inline QVariant toVariant(const Resource& obj) noexcept {
  return QVariantMap({
      {"name", *obj.getName()},
      {"media_type", obj.getMediaType()},
      {"url", obj.getUrl().toString(QUrl::FullyEncoded)},
  });
}

inline QVariant toVariant(const Polygon& obj) noexcept {
  return QVariantMap({
      {"uuid", toVariant(obj.getUuid())},
      {"layer", obj.getLayer().getId()},
      {"line_width", toVariant(*obj.getLineWidth())},
      {"fill", obj.isFilled()},
      {"grab_area", obj.isGrabArea()},
      {"vertices", toVariant(obj.getPath())},
  });
}

inline QVariant toVariant(const Circle& obj) noexcept {
  return QVariantMap({
      {"uuid", toVariant(obj.getUuid())},
      {"layer", obj.getLayer().getId()},
      {"line_width", toVariant(*obj.getLineWidth())},
      {"fill", obj.isFilled()},
      {"grab_area", obj.isGrabArea()},
      {"center", toVariant(obj.getCenter())},
      {"diameter", toVariant(*obj.getDiameter())},
  });
}

inline QVariant toVariant(const Text& obj) noexcept {
  return QVariantMap({
      {"uuid", toVariant(obj.getUuid())},
      {"layer", obj.getLayer().getId()},
      {"text", obj.getText()},
      {"position", toVariant(obj.getPosition())},
      {"rotation", toVariant(obj.getRotation())},
      {"height", toVariant(*obj.getHeight())},
      {"alignment", toVariant(obj.getAlign())},
      {"lock", obj.isLocked()},
  });
}

inline QVariant toVariant(const StrokeText& obj) noexcept {
  return QVariantMap({
      {"uuid", toVariant(obj.getUuid())},
      {"layer", obj.getLayer().getId()},
      {"text", obj.getText()},
      {"position", toVariant(obj.getPosition())},
      {"rotation", toVariant(obj.getRotation())},
      {"height", toVariant(*obj.getHeight())},
      {"stroke_width", toVariant(*obj.getStrokeWidth())},
      {"alignment", toVariant(obj.getAlign())},
      {"mirror", obj.getMirrored()},
      {"auto_rotate", obj.getAutoRotate()},
      {"lock", obj.isLocked()},
  });
}

inline QVariant toVariant(const SymbolPin& obj) noexcept {
  return QVariantMap({
      {"uuid", toVariant(obj.getUuid())},
      {"name", *obj.getName()},
      {"position", toVariant(obj.getPosition())},
      {"rotation", toVariant(obj.getRotation())},
      {"length", toVariant(*obj.getLength())},
      {"name_position", toVariant(obj.getNamePosition())},
      {"name_rotation", toVariant(obj.getNameRotation())},
      {"name_height", toVariant(*obj.getNameHeight())},
      {"name_alignment", toVariant(obj.getNameAlignment())},
  });
}

inline QVariant toVariant(const Hole& obj) noexcept {
  return QVariantMap({
      {"uuid", toVariant(obj.getUuid())},
      {"diameter", toVariant(*obj.getDiameter())},
      {"vertices", toVariant(*obj.getPath())},
      {"stop_mask", serialize(obj.getStopMaskConfig())->getValue()},
  });
}

inline QVariant toVariant(const Pad& obj) noexcept {
  const auto holes = toVariant(obj.getHoles(), [](const PadHole& obj) {
    return QVariantMap{
        {"uuid", toVariant(obj.getUuid())},
        {"diameter", toVariant(*obj.getDiameter())},
        {"vertices", toVariant(*obj.getPath())},
    };
  });

  return QVariantMap({
      {"uuid", toVariant(obj.getUuid())},
      {"position", toVariant(obj.getPosition())},
      {"rotation", toVariant(obj.getRotation())},
      {"shape", serialize(obj.getShape())->getValue()},
      {"width", toVariant(*obj.getWidth())},
      {"height", toVariant(*obj.getHeight())},
      {"corner_radius", toVariant(*obj.getRadius())},
      {"custom_outline", toVariant(obj.getCustomShapeOutline())},
      {"stop_mask", serialize(obj.getStopMaskConfig())->getValue()},
      {"solder_paste", serialize(obj.getSolderPasteConfig())->getValue()},
      {"copper_clearance", toVariant(*obj.getCopperClearance())},
      {"component_side", serialize(obj.getComponentSide())->getValue()},
      {"function", serialize(obj.getFunction())->getValue()},
      {"holes", holes},
  });
}

inline QVariant toVariant(const FootprintPad& obj) noexcept {
  QVariantMap result = toVariant(static_cast<const Pad&>(obj)).toMap();
  result.insert({
      {"package_pad", toVariant(obj.getPackagePadUuid())},
  });
  return result;
}

inline QVariant toVariant(const Zone::Layers& obj) noexcept {
  return QVariantMap({
      {"top", obj.testFlag(Zone::Layer::Top)},
      {"inner", obj.testFlag(Zone::Layer::Inner)},
      {"bottom", obj.testFlag(Zone::Layer::Bottom)},
  });
}

inline QVariant toVariant(const Zone::Rules& obj) noexcept {
  return QVariantMap({
      {"no_copper", obj.testFlag(Zone::Rule::NoCopper)},
      {"no_planes", obj.testFlag(Zone::Rule::NoPlanes)},
      {"no_exposure", obj.testFlag(Zone::Rule::NoExposure)},
      {"no_devices", obj.testFlag(Zone::Rule::NoDevices)},
  });
}

inline QVariant toVariant(const Zone& obj) noexcept {
  return QVariantMap({
      {"uuid", toVariant(obj.getUuid())},
      {"layers", toVariant(obj.getLayers())},
      {"rules", toVariant(obj.getRules())},
      {"vertices", toVariant(obj.getOutline())},
  });
}

template <typename T, typename F>
inline QVariantList modelToVariant(const slint::Model<T>& model,
                                   F convertItem) {
  QVariantList result;
  for (std::size_t i = 0; i < model.row_count(); ++i) {
    if (auto obj = model.row_data(i)) {
      result.append(convertItem(*obj));
    }
  }
  return result;
}

/*******************************************************************************
 *  Class RpcArgTraits
 ******************************************************************************/

/**
 * @brief How to read an argument of type T from a request
 *
 * Specialized for every supported argument type (see the API
 * specification). Arguments of any other type fail to compile.
 */
template <typename T>
struct RpcArgTraits;

template <>
struct RpcArgTraits<QString> {
  static constexpr const char* NAME = "string";
  static std::optional<QString> tryFromVariant(const QVariant& v) noexcept {
    if (v.userType() == QMetaType::QString) return v.toString();
    return std::nullopt;
  }
};

template <>
struct RpcArgTraits<bool> {
  static constexpr const char* NAME = "bool";
  static std::optional<bool> tryFromVariant(const QVariant& v) noexcept {
    if (v.userType() == QMetaType::Bool) return v.toBool();
    return std::nullopt;
  }
};

template <>
struct RpcArgTraits<qint64> {
  static constexpr const char* NAME = "int";
  static std::optional<qint64> tryFromVariant(const QVariant& v) noexcept {
    if (v.userType() == QMetaType::LongLong) return v.toLongLong();
    return std::nullopt;
  }
};

template <>
struct RpcArgTraits<double> {
  static constexpr const char* NAME = "float";
  static std::optional<double> tryFromVariant(const QVariant& v) noexcept {
    if (v.userType() == QMetaType::Double) return v.toDouble();
    return std::nullopt;
  }
};

template <>
struct RpcArgTraits<QByteArray> {
  static constexpr const char* NAME = "binary";
  static std::optional<QByteArray> tryFromVariant(const QVariant& v) noexcept {
    if (v.userType() == QMetaType::QByteArray) return v.toByteArray();
    return std::nullopt;
  }
};

template <>
struct RpcArgTraits<QColor> {
  static constexpr const char* NAME = "color";
  static std::optional<QColor> tryFromVariant(const QVariant& v) noexcept {
    const QColor color(v.toString());
    if (color.isValid()) return color;
    return std::nullopt;
  }
};

template <>
struct RpcArgTraits<QVariantMap> {
  static constexpr const char* NAME = "map";
  static std::optional<QVariantMap> tryFromVariant(const QVariant& v) noexcept {
    if (v.userType() == QMetaType::QVariantMap) return v.toMap();
    return std::nullopt;
  }
};

// template <>
// struct RpcArgTraits<FilePath> {
//   static constexpr const char* NAME = "FilePath";
//   static std::optional<FilePath> tryFromVariant(const QVariant& v) noexcept {
//     const FilePath fp(v.toString());
//     return fp.isValid() ? std::make_optional(fp) : std::nullopt;
//   }
// };

/// A UUID, passed as its canonical 36-character string form (the protocol
/// uses no msgpack extension types).
template <>
struct RpcArgTraits<Uuid> {
  static constexpr const char* NAME = "UUID";
  static std::optional<Uuid> tryFromVariant(const QVariant& v) noexcept {
    if (v.userType() == QMetaType::QString) {
      return Uuid::tryFromString(v.toString());
    }
    return std::nullopt;
  }
};

template <>
struct RpcArgTraits<ElementName> {
  static constexpr const char* NAME = "ElementName";
  static std::optional<ElementName> tryFromVariant(const QVariant& v) noexcept {
    return parseElementName(v.toString());
  }
};

template <>
struct RpcArgTraits<FileProofName> {
  static constexpr const char* NAME = "FileProofName";
  static std::optional<FileProofName> tryFromVariant(
      const QVariant& v) noexcept {
    return parseFileProofName(v.toString());
  }
};

template <>
struct RpcArgTraits<std::shared_ptr<Attribute>> {
  static constexpr const char* NAME = "AttributeKey";
  static std::optional<std::shared_ptr<Attribute>> tryFromVariant(
      const QVariant& v) noexcept {
    // Very ugly!
    const auto map = v.toMap();
    const auto key = AttributeKey(map.value("name").toString());
    const auto& type =
        ::librepcb::AttributeType::fromString(map.value("type").toString());
    const auto value = map.value("value").toString();
    const auto unit = type.getUnitFromString(map.value("unit").toString());
    return std::make_shared<Attribute>(key, type, value, unit);
  }
};

/// Any enum type — delegates to the generated RpcEnumTraits.
template <typename T>
  requires std::is_enum_v<T>
struct RpcArgTraits<T> {
  static constexpr const char* NAME = RpcEnumTraits<T>::NAME;
  static std::optional<T> tryFromVariant(const QVariant& v) noexcept {
    if (v.userType() == QMetaType::QString) {
      const auto& map = RpcEnumTraits<T>::mapFromString();
      const auto it = map.constFind(v.toString());
      if (it != map.constEnd()) return *it;
    }
    return std::nullopt;
  }
};

/// A list of any supported type.
template <typename T>
struct RpcArgTraits<QList<T>> {
  static constexpr const char* NAME = "array";
  static std::optional<QList<T>> tryFromVariant(const QVariant& v) noexcept {
    if (v.userType() != QMetaType::QVariantList) return std::nullopt;
    QList<T> result;
    for (const auto& item : v.toList()) {
      if (auto converted = RpcArgTraits<T>::tryFromVariant(item)) {
        result.append(*converted);
      } else {
        return std::nullopt;
      }
    }
    return result;
  }
};

/// A set of any supported type.
template <typename T>
struct RpcArgTraits<QSet<T>> {
  static constexpr const char* NAME = "array";
  static std::optional<QSet<T>> tryFromVariant(const QVariant& v) noexcept {
    if (auto list = RpcArgTraits<QList<T>>::tryFromVariant(v)) {
      const QSet<T> set(list->begin(), list->end());
      if (set.count() == list->count()) {
        return set;
      }
    }
    return std::nullopt;
  }
};

template <>
struct RpcArgTraits<AttributeList> {
  static constexpr const char* NAME = "AttributeList";
  static std::optional<AttributeList> tryFromVariant(
      const QVariant& v) noexcept {
    if (auto list =
            RpcArgTraits<QList<std::shared_ptr<Attribute>>>::tryFromVariant(
                v)) {
      AttributeList l;
      for (const auto& item : *list) {
        l.append(item);
      }
      return l;
    }
    return std::nullopt;
  }
};

/*******************************************************************************
 *  Typed Argument Reading
 ******************************************************************************/

/// Read an optional argument, returning std::nullopt if it is not
/// provided at all. If it is provided but has an invalid value, an
/// InvalidArgs error is thrown.
template <typename T>
std::optional<T> getOptionalArg(const RpcRequest& request,
                                const QString& argName) {
  const auto& args = request.getArgs();
  const auto it = args.constFind(argName);
  if (it == args.constEnd()) {
    return std::nullopt;
  }
  if (auto converted = RpcArgTraits<T>::tryFromVariant(*it)) {
    return *converted;
  }
  if constexpr (std::is_enum_v<T>) {
    throw RpcInvalidArgs(
        QString("Unknown value '%1' for argument '%2' of enum %3. "
                "Supported values are [%4].")
            .arg(it->toString(), argName, RpcEnumTraits<T>::NAME,
                 RpcEnumTraits<T>::mapFromString().keys().join(", ")));
  } else {
    throw RpcInvalidArgs(
        QString("Invalid value '%1' for argument '%2' (expected %3).")
            .arg(it->toString(), argName, RpcArgTraits<T>::NAME));
  }
}

/// Read a mandatory argument. If it is not provided, an InvalidArgs error
/// is thrown. If it is provided but has an invalid value (e.g. nil, or a
/// wrong type), an InvalidArgs error is thrown as well.
template <typename T>
T getArg(const RpcRequest& request, const QString& argName) {
  if (auto value = getOptionalArg<T>(request, argName)) {
    return *value;
  }
  throw RpcInvalidArgs(QString("Missing value for argument '%1' (expected %2).")
                           .arg(argName, RpcArgTraits<T>::NAME));
}

inline FilePath parseFilePath(const QString& path, const FilePath& base) {
  FilePath fp = FilePath(path);
  if ((!fp.isValid()) && base.isValid()) {
    fp = base.getPathTo(path);
  }
  if (fp.isValid() && (fp != base)) {
    return fp;
  }
  throw RpcInvalidArgs(
      QString("The value '%1' is not a valid file path.").arg(path));
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace rpc
}  // namespace editor
}  // namespace librepcb

#endif
