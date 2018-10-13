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

#ifndef LIBREPCB_PROJECT_PROJECTMETADATA_H
#define LIBREPCB_PROJECT_PROJECTMETADATA_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/common/attributes/attribute.h>
#include <librepcb/common/elementname.h>
#include <librepcb/common/fileio/filepath.h>
#include <librepcb/common/fileio/serializableobject.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class SmartSExprFile;

namespace project {

class Project;

/*******************************************************************************
 *  Class ProjectMetadata
 ******************************************************************************/

/**
 * @brief The ProjectMetadata class
 *
 * @author ubruhin
 * @date 2017-09-25
 */
class ProjectMetadata final : public QObject, public SerializableObject {
  Q_OBJECT

public:
  // Constructors / Destructor
  ProjectMetadata()                             = delete;
  ProjectMetadata(const ProjectMetadata& other) = delete;
  ProjectMetadata(Project& project, bool restore, bool readOnly, bool create);
  ~ProjectMetadata() noexcept;

  // Getters
  Project&    getProject() const noexcept { return mProject; }
  const Uuid& getUuid() const noexcept { return mUuid; }

  /**
   * @brief Get the name of the project
   *
   * @return The name of the project
   */
  const ElementName& getName() const noexcept { return mName; }

  /**
   * @brief Get the author of the project
   *
   * @return The author of the project
   */
  const QString& getAuthor() const noexcept { return mAuthor; }

  /**
   * @brief Get the version of the project
   *
   * @return The version of the project (arbitrary string)
   */
  const QString& getVersion() const noexcept { return mVersion; }

  /**
   * @brief Get the date and time when the project was created
   *
   * @return The local date and time of creation
   */
  const QDateTime& getCreated() const noexcept { return mCreated; }

  /**
   * @brief Get the date and time when the project was last modified
   *
   * @return The local date and time of last modification
   *
   * @todo    Dynamically determine the datetime of the last modification from
   *          version control system, file attributes or something like that.
   */
  const QDateTime& getLastModified() const noexcept { return mLastModified; }

  /**
   * @brief Get the list of attributes
   *
   * @return All attributes in a specific order
   */
  const AttributeList& getAttributes() const noexcept { return mAttributes; }

  // Setters

  /**
   * @brief Set the name of the project
   *
   * @param newName           The new name
   *
   * @undocmd{librepcb::project::CmdProjectMetadataEdit}
   */
  void setName(const ElementName& newName) noexcept;

  /**
   * @brief Set the author of the project
   *
   * @param newAuthor         The new author
   *
   * @undocmd{librepcb::project::CmdProjectMetadataEdit}
   */
  void setAuthor(const QString& newAuthor) noexcept;

  /**
   * @brief Set the version of the project
   *
   * @param newVersion        The new version (can be an arbitrary string)
   *
   * @undocmd{librepcb::project::CmdProjectMetadataEdit}
   */
  void setVersion(const QString& newVersion) noexcept;

  /**
   * @brief Set all project attributes
   *
   * @param newAttributes     The new list of attributes
   *
   * @undocmd{librepcb::project::CmdProjectMetadataEdit}
   */
  void setAttributes(const AttributeList& newAttributes) noexcept;

  /**
   * @brief Update the last modified datetime
   */
  void updateLastModified() noexcept;

  // General Methods
  bool save(bool toOriginal, QStringList& errors) noexcept;

  // Operator Overloadings
  ProjectMetadata& operator=(const ProjectMetadata& rhs) = delete;

signals:
  void attributesChanged();

private:  // Methods
  /// @copydoc librepcb::SerializableObject::serialize()
  void serialize(SExpression& root) const override;

private:  // Data
  // General
  Project& mProject;  ///< a reference to the Project object (from the ctor)

  // File "project/metadata.lp"
  FilePath                       mFilepath;
  QScopedPointer<SmartSExprFile> mFile;

  // Metadata
  Uuid        mUuid;          ///< the UUID of the project
  ElementName mName;          ///< the name of the project
  QString     mAuthor;        ///< the author of the project
  QString     mVersion;       ///< the version of the project (arbitrary string)
  QDateTime   mCreated;       ///< the datetime of the project creation
  QDateTime   mLastModified;  ///< the datetime of the last project modification
  AttributeList mAttributes;  ///< all attributes in a specific order
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace project
}  // namespace librepcb

#endif  // LIBREPCB_PROJECT_PROJECTMETADATA_H
