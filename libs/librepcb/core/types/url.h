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

#ifndef LIBREPCB_CORE_URL_H
#define LIBREPCB_CORE_URL_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <QtCore>

#include <optional>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class Url
 ******************************************************************************/

/**
 * @brief The Url class represents a valid URL
 *
 * This class wraps a `QUrl` while guaranteeing that it always contains a
 * valid URL, i.e. an URL where `QUrl::isValid()` returns `true`. So it is not
 * possible to create an #Url object from an invalid URL. If you need a
 * nullable URL, use `std::optional<Url>` instead.
 *
 * @note  Currently the serialization of URLs is not guaranteed to be
 *        canonical since it is (unfortunately) saved with
 *        `QUrl::PrettyDecoded`. We cannot currently change this in
 *        LibrePCB 2.x since this would be a breaking file format change.
 *        Future file formats shall migrate URLs to `QUrl::FullyEncoded` to
 *        get a canonical file format.
 */
class Url final {
  Q_DECLARE_TR_FUNCTIONS(Url)

public:
  // Constructors / Destructor

  /**
   * @brief Default constructor (disabled to avoid creating invalid URLs)
   */
  Url() = delete;

  /**
   * @brief Copy constructor
   *
   * @param other     Another #Url object.
   */
  Url(const Url& other) noexcept = default;

  /**
   * @brief Destructor
   */
  ~Url() noexcept = default;

  // Getters

  /**
   * @brief Get the URL as a string
   *
   * @return The URL exactly as it was created from (never modified).
   */
  const QString& toStr() const noexcept { return mUrl; }

  /**
   * @brief Get the URL as a pretty decoded string
   *
   * @return The URL in a pretty encoded form.
   */
  QString toPrettyStr() const noexcept {
    return mObj.toString(QUrl::PrettyDecoded);
  }

  /**
   * @brief Get the URL as `QUrl` object
   *
   * @return The URL as `QUrl` (always valid).
   */
  const QUrl& toQUrl() const noexcept { return mObj; }

  //@{
  /**
   * @brief Operator overloadings
   *
   * @param rhs   The other object to assign or compare.
   *
   * @return Result of the operation.
   */
  Url& operator=(const Url& rhs) noexcept = default;
  bool operator==(const Url& rhs) const noexcept { return mUrl == rhs.mUrl; }
  bool operator!=(const Url& rhs) const noexcept { return mUrl != rhs.mUrl; }
  //@}

  // Static Methods

  /**
   * @brief Create an #Url from a string
   *
   * @param str           Input string.
   * @param mode          Parsing mode. In strict mode, strings which are not
   *                      fully encoded are rejected.
   * @param canonicalize  Convert the string to its fully encoded form
   *                      (ignored in strict mode, where the string must
   *                      already be fully encoded anyway).
   *
   * @return The created #Url object.
   *
   * @throws Exception if the string does not contain a valid URL.
   */
  static Url fromString(const QString& str, QUrl::ParsingMode mode,
                        bool canonicalize);

  /**
   * @brief Try creating an #Url from a string
   *
   * @param str           Input string.
   * @param mode          Parsing mode. In strict mode, strings which are not
   *                      fully encoded are rejected.
   * @param canonicalize  Convert the string to its fully encoded form
   *                      (ignored in strict mode, where the string must
   *                      already be fully encoded anyway).
   *
   * @return  The created #Url object if str was valid, `std::nullopt`
   *          otherwise.
   */
  static std::optional<Url> tryFromString(const QString& str,
                                          QUrl::ParsingMode mode,
                                          bool canonicalize) noexcept;

  /**
   * @brief Try creating an #Url from a user input string
   *
   * @param str   Input string (untrusted user input).
   *
   * @return  The created #Url object if `str` was valid, `std::nullopt`
   *          otherwise.
   */
  static std::optional<Url> tryFromUserInput(const QString& str) noexcept;

private:  // Methods
  explicit Url(const QString& url, const QUrl& obj) noexcept
    : mUrl(url), mObj(obj) {}

private:  // Data
  QString mUrl;  ///< Separate member for guaranteed identical roundtrip.
  QUrl mObj;  ///< Guaranteed to always contain a valid URL.
};

/*******************************************************************************
 *  Non-Member Functions
 ******************************************************************************/

inline QDebug operator<<(QDebug stream, const Url& url) noexcept {
  stream << QString("Url(%1)").arg(url.toStr());
  return stream;
}

inline std::size_t qHash(const Url& key, std::size_t seed = 0) noexcept {
  return ::qHash(key.toStr(), seed);
}

inline std::size_t qHash(const std::optional<Url>& key,
                         std::size_t seed = 0) noexcept {
  return ::qHash(key ? key->toStr() : QString(), seed);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
