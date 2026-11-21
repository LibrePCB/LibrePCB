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
#include <gtest/gtest.h>
#include <librepcb/core/exceptions.h>
#include <librepcb/core/serialization/sexpression.h>
#include <librepcb/core/types/url.h>

#include <QtCore>

#include <optional>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace tests {

/*******************************************************************************
 *  Test Data Type
 ******************************************************************************/

struct UrlTestData {
  QString input;
  QString canonical;
  bool isValid() const noexcept { return !canonical.isEmpty(); }
};

/*******************************************************************************
 *  Test Class
 ******************************************************************************/

class UrlTest : public ::testing::TestWithParam<UrlTestData> {
protected:
  static std::string str(const QString& s) noexcept { return s.toStdString(); }
};

/*******************************************************************************
 *  Test Methods
 ******************************************************************************/

TEST_P(UrlTest, testFromString) {
  const UrlTestData& data = GetParam();

  if (data.isValid()) {
    EXPECT_EQ(
        str(data.input),
        str(Url::fromString(data.input, QUrl::TolerantMode, false).toStr()));
    EXPECT_EQ(
        str(data.canonical),
        str(Url::fromString(data.input, QUrl::TolerantMode, true).toStr()));
    if (data.canonical == data.input) {
      EXPECT_EQ(
          str(data.input),
          str(Url::fromString(data.input, QUrl::StrictMode, false).toStr()));
      EXPECT_EQ(
          str(data.canonical),
          str(Url::fromString(data.input, QUrl::StrictMode, true).toStr()));
    } else {
      EXPECT_THROW(Url::fromString(data.input, QUrl::StrictMode, false),
                   Exception);
      EXPECT_THROW(Url::fromString(data.input, QUrl::StrictMode, true),
                   Exception);
    }
  } else {
    EXPECT_THROW(Url::fromString(data.input, QUrl::TolerantMode, false),
                 Exception);
    EXPECT_THROW(Url::fromString(data.input, QUrl::TolerantMode, true),
                 Exception);
    EXPECT_THROW(Url::fromString(data.input, QUrl::StrictMode, false),
                 Exception);
    EXPECT_THROW(Url::fromString(data.input, QUrl::StrictMode, true),
                 Exception);
  }
}

TEST_P(UrlTest, testTryFromString) {
  const UrlTestData& data = GetParam();

  if (data.isValid()) {
    const auto tolerant =
        Url::tryFromString(data.input, QUrl::TolerantMode, false);
    ASSERT_TRUE(tolerant.has_value());
    EXPECT_EQ(data.input, tolerant->toStr());
    const auto canonical =
        Url::tryFromString(data.input, QUrl::TolerantMode, true);
    ASSERT_TRUE(canonical.has_value());
    EXPECT_EQ(data.canonical, canonical->toStr());
    if (data.canonical == data.input) {
      const auto strict =
          Url::tryFromString(data.input, QUrl::StrictMode, false);
      ASSERT_TRUE(strict.has_value());
      EXPECT_EQ(data.input, strict->toStr());
      const auto strictCanonical =
          Url::tryFromString(data.input, QUrl::StrictMode, true);
      ASSERT_TRUE(strictCanonical.has_value());
      EXPECT_EQ(data.canonical, strictCanonical->toStr());
    } else {
      EXPECT_EQ(std::nullopt,
                Url::tryFromString(data.input, QUrl::StrictMode, false));
      EXPECT_EQ(std::nullopt,
                Url::tryFromString(data.input, QUrl::StrictMode, true));
    }
  } else {
    EXPECT_EQ(std::nullopt,
              Url::tryFromString(data.input, QUrl::TolerantMode, false));
    EXPECT_EQ(std::nullopt,
              Url::tryFromString(data.input, QUrl::TolerantMode, true));
    EXPECT_EQ(std::nullopt,
              Url::tryFromString(data.input, QUrl::StrictMode, false));
    EXPECT_EQ(std::nullopt,
              Url::tryFromString(data.input, QUrl::StrictMode, true));
  }
}

TEST_F(UrlTest, testTryFromUserInput) {
  // A scheme is automatically added if missing.
  auto obj = Url::tryFromUserInput("librepcb.org");
  ASSERT_TRUE(obj.has_value());
  EXPECT_EQ("http://librepcb.org", str(obj->toStr()));

  // Whitespace is removed, and the URL is always canonicalized.
  obj = Url::tryFromUserInput(" https://example.com/some image.jpg ");
  ASSERT_TRUE(obj.has_value());
  EXPECT_EQ("https://example.com/some%20image.jpg", str(obj->toStr()));

  // Empty or invalid input is rejected.
  EXPECT_EQ(std::nullopt, Url::tryFromUserInput(QString()));
  EXPECT_EQ(std::nullopt, Url::tryFromUserInput(" "));
  EXPECT_EQ(std::nullopt, Url::tryFromUserInput("not a url at all"));
}

TEST_P(UrlTest, testCopyConstructor) {
  const UrlTestData& data = GetParam();

  if (data.isValid()) {
    const Url source = Url::fromString(data.input, QUrl::TolerantMode, false);
    const Url copy(source);
    EXPECT_TRUE(copy == source);
    EXPECT_EQ(str(source.toStr()), str(copy.toStr()));
    EXPECT_EQ(source.toQUrl(), copy.toQUrl());
  }
}

TEST_P(UrlTest, testToQUrl) {
  const UrlTestData& data = GetParam();

  if (data.isValid()) {
    const Url url = Url::fromString(data.input, QUrl::TolerantMode, false);
    EXPECT_TRUE(url.toQUrl().isValid());
    EXPECT_EQ(QUrl(data.input, QUrl::TolerantMode), url.toQUrl());
  }
}

TEST_P(UrlTest, testToPrettyStr) {
  const UrlTestData& data = GetParam();

  if (data.isValid()) {
    const Url url = Url::fromString(data.input, QUrl::TolerantMode, false);
    EXPECT_EQ(url.toQUrl().toString(QUrl::PrettyDecoded), url.toPrettyStr());
  }
}

TEST_P(UrlTest, testOperatorAssign) {
  const UrlTestData& data = GetParam();

  if (data.isValid()) {
    const Url source = Url::fromString(data.input, QUrl::TolerantMode, false);
    Url destination =
        Url::fromString("https://librepcb.org", QUrl::TolerantMode, false);
    destination = source;
    EXPECT_EQ(str(source.toStr()), str(destination.toStr()));
    EXPECT_EQ(source.toQUrl(), destination.toQUrl());
  }
}

TEST_P(UrlTest, testOperatorEquals) {
  const UrlTestData& data = GetParam();

  if (data.isValid()) {
    const Url url1 = Url::fromString(data.input, QUrl::TolerantMode, false);
    Url url2 = Url::fromString("https://librepcb.org", QUrl::StrictMode, false);
    EXPECT_FALSE(url2 == url1);
    EXPECT_FALSE(url1 == url2);
    EXPECT_TRUE(url2 != url1);
    EXPECT_TRUE(url1 != url2);
    url2 = url1;
    EXPECT_TRUE(url2 == url1);
    EXPECT_TRUE(url1 == url2);
    EXPECT_FALSE(url2 != url1);
    EXPECT_FALSE(url1 != url2);
  }
}

TEST_P(UrlTest, testSerialize) {
  const UrlTestData& data = GetParam();

  if (data.isValid()) {
    const Url url = Url::fromString(data.input, QUrl::TolerantMode, false);
    EXPECT_EQ(str(data.input), str(serialize(url)->getValue()));
    EXPECT_EQ(str(data.input),
              str(serialize(std::make_optional(url))->getValue()));
  }
}

TEST_P(UrlTest, testDeserialize) {
  const UrlTestData& data = GetParam();

  const std::unique_ptr<SExpression> sexpr =
      SExpression::createString(data.input);
  if (data.isValid()) {
    EXPECT_EQ(str(data.input), str(deserialize<Url>(*sexpr).toStr()));
    const auto opt = deserialize<std::optional<Url>>(*sexpr);
    ASSERT_TRUE(opt.has_value());
    EXPECT_EQ(str(data.input), str(opt->toStr()));
  } else if (data.input.isEmpty()) {
    // An empty string deserializes to an empty optional instead of throwing.
    EXPECT_THROW(deserialize<Url>(*sexpr), Exception);
    EXPECT_EQ(std::nullopt, deserialize<std::optional<Url>>(*sexpr));
  } else {
    EXPECT_THROW(deserialize<Url>(*sexpr), Exception);
    EXPECT_THROW(deserialize<std::optional<Url>>(*sexpr), Exception);
  }
}

TEST_F(UrlTest, testSerializeOptional) {
  const std::optional<Url> url = std::nullopt;
  EXPECT_EQ("", serialize(url)->getValue());
}

TEST_F(UrlTest, testDeserializeOptional) {
  const std::unique_ptr<SExpression> sexpr = SExpression::createString("");
  EXPECT_EQ(std::nullopt, deserialize<std::optional<Url>>(*sexpr));
}

// Serialization roundtrip must never modify the stored URL, no matter if it
// is canonical (i.e. fully encoded) or not!
TEST_P(UrlTest, testSerializationRoundtrip) {
  const UrlTestData& data = GetParam();

  if (data.isValid()) {
    const auto sexpr = SExpression::createString(data.input);
    const Url obj = deserialize<Url>(*sexpr);
    EXPECT_EQ(str(data.input), str(obj.toStr()));
    EXPECT_EQ(str(data.input), str(serialize(obj)->getValue()));
  }
}

/*******************************************************************************
 *  Test Data
 ******************************************************************************/

// clang-format off
INSTANTIATE_TEST_SUITE_P(UrlTest, UrlTest, ::testing::Values(
  // Valid URLs.
  UrlTestData({"https://example.com",
               "https://example.com"}),
  UrlTestData({"https://example.com/some/image.jpg",
               "https://example.com/some/image.jpg"}),
  UrlTestData({"https://www.example.com/products/en?keywords=LM358",
               "https://www.example.com/products/en?keywords=LM358"}),
  // Real-world DigiKey image URL containing unencoded spaces.
  UrlTestData({"https://example.com/278_DO-204AH%2C DO-35%2C Axial.jpg",
               "https://example.com/278_DO-204AH%2C%20DO-35%2C%20Axial.jpg"}),
  // Local file URL containing unencoded space.
  UrlTestData({"file:///home/user/My Project/board.lp",
               "file:///home/user/My%20Project/board.lp"}),
  // URLs containing unencoded non-ASCII characters.
  UrlTestData({"https://www.example.com/Überstraße.jpg",
               "https://www.example.com/%C3%9Cberstra%C3%9Fe.jpg"}),
  // Relative URLs are valid as well (no scheme).
  UrlTestData({"docs/manual.pdf",
               "docs/manual.pdf"}),
  // Invalid URLs.
  UrlTestData({"", ""}),
  UrlTestData({"http://[", ""})
));
// clang-format on

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace librepcb
