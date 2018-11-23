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

#ifndef LIBREPCB_SERIALIZABLEOBJECTLIST_H
#define LIBREPCB_SERIALIZABLEOBJECTLIST_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../uuid.h"
#include "serializableobject.h"

#include <QtCore>

#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class SerializableObjectList
 ******************************************************************************/

/**
 * @brief The SerializableObjectList class implements a list of
 * librepcb::SerializableObject
 *
 * This template class lets you hold a list of serializable objects and provides
 * some useful features:
 * - The method #loadFromDomElement() to deserialize from a
 * librepcb::DomElement.
 * - The method #serialize() to serialize the whole list into a
 * librepcb::DomElement.
 * - Iterators (for example to use in C++11 range based for loops).
 * - Methods to find elements by UUID and/or name (if supported by template type
 * `T`).
 * - Method #sortedByUuid() to create a copy of the list with elements sorted by
 * UUID.
 * - Observer pattern to get notified about added and removed elements.
 * - Undo commands librepcb::CmdListElementInsert,
 * librepcb::CmdListElementRemove and ibrepcb::CmdListElementsSwap.
 * - Const correctness: A const list always returns pointers/references to const
 * elements.
 *
 * @tparam T  The type of the list items. The type must provide following
 * functionality:
 *              - Optional: A nothrow copy constructor (to make the list
 * copyable)
 *              - Optional: A constructor with one parameter of type `const
 * DomElement&`
 *              - Optional: A method `serialize()` according to
 * librepcb::SerializableObject
 *              - Optional: Comparison operator overloadings
 *              - Optional: A method `Uuid getUuid() const noexcept`
 *              - Optional: A method `QString getName() const noexcept`
 * @tparam P  A class which provides the S-Expression node tag name of the list
 * items. Example: `struct MyNameProvider {static constexpr const char* tagname
 * = "item";};`
 *
 * @note    Instead of directly storing elements of type `T`, elements are
 * always wrapped into a `std::shared_ptr<T>` before adding them to the list.
 * This is done to ensure that elements never have to be copied or moved for
 * adding or removing them to/from the list. Otherwise it would not be possible
 * to use this list in undo commands as references/pointers to elements would
 * become invalid. Using pointers ensures that the objects are located at the
 * same address over the whole lifetime. To still minimize the risk of memory
 * leaks, `std::shared_ptr` is used instead of raw pointers.
 *
 * @warning Using Qt's `foreach` keyword on a #SerializableObjectList is not
 * recommended because it always creates a deep copy of the list! You should use
 * range based for loops (since C++11) instead.
 */
template <typename T, typename P>
class SerializableObjectList : public SerializableObject {
  Q_DECLARE_TR_FUNCTIONS(SerializableObjectList)

public:
  // Observer Type
  class IF_Observer {
  public:
    virtual void listObjectAdded(const SerializableObjectList<T, P>& list,
                                 int                                 newIndex,
                                 const std::shared_ptr<T>& ptr) noexcept   = 0;
    virtual void listObjectRemoved(const SerializableObjectList<T, P>& list,
                                   int                                 oldIndex,
                                   const std::shared_ptr<T>& ptr) noexcept = 0;
  };

  // Iterator Types
  template <typename I, typename O>
  class Iterator {
    I it;

  public:
    Iterator() = delete;
    Iterator(const Iterator& other) noexcept : it(other.it) {}
    Iterator(const I& it) noexcept : it(it) {}
    bool operator!=(const Iterator& rhs) const noexcept { return it != rhs.it; }
    Iterator& operator++() {
      ++it;
      return *this;
    }
    O&                 operator*() { return **it; }
    std::shared_ptr<O> ptr() noexcept {
      return std::const_pointer_cast<O>(*it);
    }
    ~Iterator() {}
  };
  using iterator = Iterator<typename QVector<std::shared_ptr<T>>::iterator, T>;
  using const_iterator =
      Iterator<typename QVector<std::shared_ptr<T>>::const_iterator, const T>;

  // Constructors / Destructor
  explicit SerializableObjectList(IF_Observer* observer = nullptr) noexcept {
    if (observer) registerObserver(observer);
  }
  SerializableObjectList(const SerializableObjectList<T, P>& other,
                         IF_Observer* observer = nullptr) noexcept {
    *this = other;  // copy all elements
    if (observer) registerObserver(observer);
  }
  SerializableObjectList(SerializableObjectList<T, P>&& other,
                         IF_Observer* observer = nullptr) noexcept {
    mObjects = other.mObjects;  // copy all pointers (NOT the objects!)
    other.clear();  // remove all other's elements with notifying its observers
    if (observer) registerObserver(observer);
  }
  SerializableObjectList(std::initializer_list<std::shared_ptr<T>> elements,
                         IF_Observer* observer = nullptr) noexcept {
    mObjects = elements;
    if (observer) registerObserver(observer);
  }
  SerializableObjectList(std::initializer_list<T> elements,
                         IF_Observer*             observer = nullptr) noexcept {
    mObjects.reserve(elements.size());
    for (const T& obj : elements) {
      append(std::make_shared<T>(obj));
    }  // copy element
    if (observer) registerObserver(observer);
  }
  explicit SerializableObjectList(const SExpression& node,
                                  IF_Observer*       observer = nullptr) {
    loadFromDomElement(node);  // can throw
    if (observer) registerObserver(observer);
  }
  virtual ~SerializableObjectList() noexcept {}

  // Getters
  bool              isEmpty() const noexcept { return mObjects.empty(); }
  int               count() const noexcept { return mObjects.count(); }
  std::vector<Uuid> getUuids() const noexcept {
    std::vector<Uuid> uuids;
    uuids.reserve(mObjects.count());
    foreach (const std::shared_ptr<T>& obj, mObjects) {
      uuids.push_back(obj->getUuid());
    }
    return uuids;
  }
  QSet<Uuid> getUuidSet() const noexcept {
    QSet<Uuid> uuids;
    uuids.reserve(mObjects.count());
    foreach (const std::shared_ptr<T>& obj, mObjects) {
      uuids.insert(obj->getUuid());
    }
    return uuids;
  }

  // Element Query
  int indexOf(const T* obj) const noexcept {
    for (int i = 0; i < count(); ++i) {
      if (mObjects[i].get() == obj) {
        return i;
      }
    }
    return -1;
  }
  int indexOf(const Uuid& key) const noexcept {
    for (int i = 0; i < count(); ++i) {
      if (mObjects[i]->getUuid() == key) {
        return i;
      }
    }
    return -1;
  }
  int indexOf(const QString& name) const noexcept {
    for (int i = 0; i < count(); ++i) {
      if (mObjects[i]->getName() == name) {
        return i;
      }
    }
    return -1;
  }
  bool contains(int index) const noexcept {
    return index >= 0 && index < mObjects.count();
  }
  bool contains(const T* obj) const noexcept { return indexOf(obj) >= 0; }
  bool contains(const Uuid& key) const noexcept { return indexOf(key) >= 0; }
  bool contains(const QString& name) const noexcept {
    return indexOf(name) >= 0;
  }

  // "Soft" Element Access (null if not found)
  std::shared_ptr<T> value(int index) noexcept { return mObjects.value(index); }
  std::shared_ptr<const T> value(int index) const noexcept {
    return std::const_pointer_cast<const T>(mObjects.value(index));
  }
  std::shared_ptr<T> find(const T* obj) noexcept { return value(indexOf(obj)); }
  std::shared_ptr<T> find(const Uuid& key) noexcept {
    return value(indexOf(key));
  }
  std::shared_ptr<const T> find(const Uuid& key) const noexcept {
    return value(indexOf(key));
  }
  std::shared_ptr<T> find(const QString& name) noexcept {
    return value(indexOf(name));
  }
  std::shared_ptr<const T> find(const QString& name) const noexcept {
    return value(indexOf(name));
  }

  // "Hard" Element Access (assertion or exception if not found!)
  std::shared_ptr<const T> at(int index) const noexcept {
    return std::const_pointer_cast<const T>(mObjects.at(index));
  }  // always read-only!
  std::shared_ptr<T>&      first() noexcept { return mObjects.first(); }
  std::shared_ptr<const T> first() const noexcept { return mObjects.first(); }
  std::shared_ptr<T>&      last() noexcept { return mObjects.last(); }
  std::shared_ptr<const T> last() const noexcept { return mObjects.last(); }
  std::shared_ptr<T>       get(const T* obj) {
    std::shared_ptr<T> ptr = find(obj);
    if (!ptr) throw LogicError(__FILE__, __LINE__);
    return ptr;
  }
  std::shared_ptr<T> get(const Uuid& key) {
    std::shared_ptr<T> ptr = find(key);
    if (!ptr) throwKeyNotFoundException(key);
    return ptr;
  }
  std::shared_ptr<const T> get(const Uuid& key) const {
    std::shared_ptr<const T> ptr = find(key);
    if (!ptr) throwKeyNotFoundException(key);
    return ptr;
  }
  std::shared_ptr<T> get(const QString& name) {
    std::shared_ptr<T> ptr = find(name);
    if (!ptr) throwNameNotFoundException(name);
    return ptr;
  }
  std::shared_ptr<const T> get(const QString& name) const {
    std::shared_ptr<const T> ptr = find(name);
    if (!ptr) throwNameNotFoundException(name);
    return ptr;
  }

  // Iterator Access
  const_iterator begin() const noexcept { return mObjects.begin(); }
  const_iterator end() const noexcept { return mObjects.end(); }
  const_iterator cbegin() noexcept { return mObjects.cbegin(); }
  const_iterator cend() noexcept { return mObjects.cend(); }
  iterator       begin() noexcept { return mObjects.begin(); }
  iterator       end() noexcept { return mObjects.end(); }

  // General Methods
  int loadFromDomElement(const SExpression& node) {
    clear();
    foreach (const SExpression& node, node.getChildren(P::tagname)) {
      append(std::make_shared<T>(node));  // can throw
    }
    return count();
  }
  void swap(int i, int j) noexcept {
    // do not call mObjects.swap() because it would not notify the observers
    qBound(0, i, count() - 1);
    qBound(0, j, count() - 1);
    if (i == j) return;
    if (i > j) qSwap(i, j);
    std::shared_ptr<T> oj = take(j);
    std::shared_ptr<T> oi = take(i);
    insert(i, oj);
    insert(j, oi);
  }
  int insert(int index, const std::shared_ptr<T>& obj) noexcept {
    Q_ASSERT(obj);
    qBound(0, index, count());
    mObjects.insert(index, obj);
    notifyObjectAdded(index, obj);
    return index;
  }
  int append(const std::shared_ptr<T>& obj) noexcept {
    return insert(count(), obj);
  }
  std::shared_ptr<T> take(int index) noexcept {
    Q_ASSERT(contains(index));
    std::shared_ptr<T> obj = mObjects.takeAt(index);
    notifyObjectRemoved(index, obj);
    return std::move(obj);
  }
  std::shared_ptr<T> take(const T* obj) noexcept { return take(indexOf(obj)); }
  std::shared_ptr<T> take(const Uuid& uuid) noexcept {
    return take(indexOf(uuid));
  }
  std::shared_ptr<T> take(const QString& name) noexcept {
    return take(indexOf(name));
  }
  void remove(int index) noexcept { take(index); }
  void remove(const T* obj) noexcept { take(obj); }
  void remove(const Uuid& uuid) noexcept { take(uuid); }
  void remove(const QString& name) noexcept { take(name); }
  void clear() noexcept {
    // do not call mObjects.clear() because it would not notify the observers
    for (int i = count() - 1; i >= 0; --i) {
      remove(i);
    }
  }
  /// @copydoc librepcb::SerializableObject::serialize()
  void serialize(SExpression& root) const override {
    serializePointerContainer(root, mObjects, P::tagname);  // can throw
  }

  // Convenience Methods
  SerializableObjectList<T, P> sortedByUuid() const noexcept {
    SerializableObjectList<T, P> copiedList;
    copiedList.mObjects = mObjects;  // copy only the pointers, not the objects!
    qSort(copiedList.mObjects.begin(), copiedList.mObjects.end(),
          [](const std::shared_ptr<T>& ptr1, const std::shared_ptr<T>& ptr2) {
            return ptr1->getUuid() < ptr2->getUuid();
          });
    return copiedList;
  }
  SerializableObjectList<T, P> sortedByName() const noexcept {
    SerializableObjectList<T, P> copiedList;
    copiedList.mObjects = mObjects;  // copy only the pointers, not the objects!
    qSort(copiedList.mObjects.begin(), copiedList.mObjects.end(),
          [](const std::shared_ptr<T>& ptr1, const std::shared_ptr<T>& ptr2) {
            return ptr1->getName() < ptr2->getName();
          });
    return copiedList;
  }

  // Observer Methods
  void registerObserver(IF_Observer* o) noexcept {
    Q_ASSERT(o);
    mObservers.append(o);
  }
  void unregisterObserver(IF_Observer* o) noexcept {
    Q_ASSERT(o);
    mObservers.removeOne(o);
  }

  // Operator Overloadings
  std::shared_ptr<T> operator[](int i) noexcept {
    Q_ASSERT(contains(i));
    return mObjects[i];
  }
  std::shared_ptr<const T> operator[](int i) const noexcept {
    Q_ASSERT(contains(i));
    return mObjects[i];
  }
  bool operator==(const SerializableObjectList<T, P>& rhs) const noexcept {
    if (rhs.mObjects.count() != mObjects.count()) return false;
    for (int i = 0; i < mObjects.count(); ++i) {
      if (*rhs.mObjects[i] != *mObjects[i]) return false;
    }
    return true;
  }
  bool operator!=(const SerializableObjectList<T, P>& rhs) const noexcept {
    return !(*this == rhs);
  }
  SerializableObjectList<T, P>& operator=(
      const SerializableObjectList<T, P>& rhs) noexcept {
    clear();
    mObjects.reserve(rhs.count());
    foreach (const std::shared_ptr<T>& ptr, rhs.mObjects) {
      append(std::make_shared<T>(*ptr));  // call copy constructor of object
    }
    return *this;
  }
  SerializableObjectList<T, P>& operator=(
      SerializableObjectList<T, P>&& rhs) noexcept {
    clear();
    mObjects.reserve(rhs.count());
    foreach (const std::shared_ptr<T>& ptr, rhs.mObjects) {
      append(ptr);  // copy only the pointer, NOT the object
    }
    rhs.clear();
    return *this;
  }

protected:  // Methods
  void notifyObjectAdded(int index, const std::shared_ptr<T>& obj) noexcept {
    foreach (IF_Observer* observer, mObservers) {
      observer->listObjectAdded(*this, index, obj);
    }
  }
  void notifyObjectRemoved(int index, const std::shared_ptr<T>& obj) noexcept {
    foreach (IF_Observer* observer, mObservers) {
      observer->listObjectRemoved(*this, index, obj);
    }
  }
  void throwKeyNotFoundException(const Uuid& key) const {
    throw RuntimeError(
        __FILE__, __LINE__,
        QString(
            tr("There is "
               "no element of type \"%1\" with the UUID \"%2\" in the list."))
            .arg(P::tagname)
            .arg(key.toStr()));
  }
  void throwNameNotFoundException(const QString& name) const {
    throw RuntimeError(
        __FILE__, __LINE__,
        QString(
            tr("There is "
               "no element of type \"%1\" with the name \"%2\" in the list."))
            .arg(P::tagname)
            .arg(name));
  }

protected:  // Data
  QVector<std::shared_ptr<T>> mObjects;
  QList<IF_Observer*>         mObservers;
};

}  // namespace librepcb

/*******************************************************************************
 * Prevent from using SerializableObjectList in a foreach loop because it always
 *would create a deep copy of the list! You should use C++11 range based for
 *loops instead.
 ******************************************************************************/

#if (QT_VERSION > QT_VERSION_CHECK(5, 9, 0))
namespace QtPrivate {
#endif

template <typename T, typename P>
class QForeachContainer<librepcb::SerializableObjectList<T, P>> {
public:
  ~QForeachContainer() = delete;
};
template <typename T, typename P>
class QForeachContainer<const librepcb::SerializableObjectList<T, P>> {
public:
  ~QForeachContainer() = delete;
};

#if (QT_VERSION > QT_VERSION_CHECK(5, 9, 0))
}  // namespace QtPrivate
#endif

/*******************************************************************************
 *  End of File
 ******************************************************************************/

#endif  // LIBREPCB_SERIALIZABLEOBJECTLIST_H
