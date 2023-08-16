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

#ifndef LIBREPCB_CORE_OUTPUTJOB_H
#define LIBREPCB_CORE_OUTPUTJOB_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../serialization/serializableobjectlist.h"
#include "../types/elementname.h"
#include "../types/uuid.h"
#include "../utils/toolbox.h"

#include <QtCore>
#include <QtGui>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class Project;

/*******************************************************************************
 *  Class OutputJob
 ******************************************************************************/

/**
 * @brief Base class for all output job types
 */
class OutputJob {
  Q_DECLARE_TR_FUNCTIONS(OutputJob)

public:
  // Signals
  enum class Event {
    UuidChanged,
    NameChanged,
    PropertyChanged,
  };
  Signal<OutputJob, Event> onEdited;
  typedef Slot<OutputJob, Event> OnEditedSlot;

  // Object set
  template <typename T>
  struct ObjectSet {
    ObjectSet(const ObjectSet& other) = default;
    ObjectSet(const SExpression& node, const QString& childName)
      : mAll(false), mDefault(false), mSet() {
      const SExpression* firstNode = node.tryGetChild(childName % "/@0");
      if (firstNode && (firstNode->getValue() == "all")) {
        mAll = true;
      } else if (firstNode && (firstNode->getValue() == "default")) {
        mDefault = true;
      } else {
        foreach (const SExpression* child, node.getChildren(childName)) {
          mSet.insert(deserialize<T>(child->getChild("@0")));
        }
      }
    }
    ObjectSet& operator=(const ObjectSet& rhs) = default;
    bool operator==(const ObjectSet<T>& rhs) const noexcept {
      return (mAll == rhs.mAll) && (mDefault == rhs.mDefault) &&
          (mSet == rhs.mSet);
    }
    bool operator!=(const ObjectSet<T>& rhs) const noexcept {
      return !(*this == rhs);
    }

    bool isAll() const noexcept { return mAll; }
    bool isDefault() const noexcept { return mDefault; }
    bool isCustom() const noexcept { return (!mAll) && (!mDefault); }
    const QSet<T>& getSet() const noexcept { return mSet; }
    void serialize(SExpression& root, const QString& key) const {
      if (mAll) {
        root.ensureLineBreak();
        root.appendChild(key, SExpression::createToken("all"));
      } else if (mDefault) {
        root.ensureLineBreak();
        root.appendChild(key, SExpression::createToken("default"));
      } else {
        foreach (const T& value, Toolbox::sortedQSet(mSet)) {
          root.ensureLineBreak();
          root.appendChild(key, value);
        }
      }
      root.ensureLineBreak();
    }

    static ObjectSet all() noexcept { return ObjectSet(true, false, {}); }
    static ObjectSet onlyDefault() noexcept {
      return ObjectSet(false, true, {});
    }
    static ObjectSet set(const QSet<T>& set) noexcept {
      return ObjectSet(false, false, set);
    }

  private:
    ObjectSet(bool all, bool onlyDefault, const QSet<T>& set)
      : mAll(all), mDefault(onlyDefault), mSet(set) {}

    bool mAll;
    bool mDefault;
    QSet<T> mSet;
  };

  // Constructors / Destructor
  OutputJob() = delete;
  virtual ~OutputJob() noexcept;

  // Getters
  const QString& getType() const noexcept { return mType; }
  virtual QString getTypeTr() const noexcept = 0;
  virtual QIcon getTypeIcon() const noexcept = 0;
  const Uuid& getUuid() const noexcept { return mUuid; }
  const ElementName& getName() const noexcept { return mName; }
  virtual QSet<Uuid> getDependencies() const noexcept { return {}; }

  // Setters
  void setUuid(const Uuid& uuid) noexcept;
  void setName(const ElementName& name) noexcept;

  // General Methods
  virtual void removeDependency(const Uuid& jobUuid) { Q_UNUSED(jobUuid); }
  virtual std::shared_ptr<OutputJob> cloneShared() const noexcept = 0;

  /**
   * @brief Serialize into ::librepcb::SExpression node
   *
   * @param root    Root node to serialize into.
   */
  virtual void serialize(SExpression& root) const;

  // Operator Overloadings
  bool operator==(const OutputJob& rhs) const noexcept;
  bool operator!=(const OutputJob& rhs) const noexcept {
    return !(*this == rhs);
  }
  OutputJob& operator=(const OutputJob& rhs) = delete;

protected:  // Methods
  OutputJob(const OutputJob& other) noexcept;
  explicit OutputJob(const SExpression& node);
  OutputJob(const QString& type, const Uuid& uuid,
            const ElementName& name) noexcept;
  virtual void serializeDerived(SExpression& root) const = 0;
  virtual bool equals(const OutputJob& rhs) const noexcept = 0;

protected:  // Data
  const QString mType;
  Uuid mUuid;
  ElementName mName;

  // Arbitrary options for forward compatibility in case we really need to
  // add new settings in a minor release.
  QMap<QString, QList<SExpression>> mOptions;
};

/*******************************************************************************
 *  Class OutputJobList
 ******************************************************************************/

struct OutputJobListNameProvider {
  static constexpr const char* tagname = "job";
};
using OutputJobList =
    SerializableObjectList<OutputJob, OutputJobListNameProvider,
                           OutputJob::Event>;

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb

#endif
