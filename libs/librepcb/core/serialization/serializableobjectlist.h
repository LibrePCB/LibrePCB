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

#ifndef LIBREPCB_CORE_SERIALIZABLEOBJECTLIST_H
#define LIBREPCB_CORE_SERIALIZABLEOBJECTLIST_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../exceptions.h"
#include "../types/uuid.h"
#include "../utils/signalslot.h"
#include "sexpression.h"

#include <QtCore>

#include <algorithm>
#include <memory>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Class SerializableObjectList
 ******************************************************************************/

/**
 * @brief The SerializableObjectList class implements a list of serializable
 *        objects
 *
 * This template class lets you hold a list of serializable objects and provides
 * some useful features:
 * - The method #loadFromSExpression() to deserialize from a
 *   librepcb::SExpression.
 * - The method #serialize() to serialize the whole list into a
 *   librepcb::SExpression.
 * - Iterators (for example to use in C++11 range based for loops).
 * - Methods to find elements by UUID and/or name (if supported by template type
 *   `T`).
 * - Method #sortedByUuid() to create a copy of the list with elements sorted by
 *   UUID.
 * - Signals to get notified about added, removed and modified elements.
 * - Undo commands ::librepcb::editor::CmdListElementInsert,
 *   ::librepcb::editor::CmdListElementRemove and
 *   ::librepcb::editor::CmdListElementsSwap.
 * - Const correctness: A const list always returns pointers/references to const
 *   elements.
 *
 * @tparam T  The type of the list items. The type must provide following
 *            functionality:
 *              - Optional: A nothrow copy constructor (to make the list
 *                copyable)
 *              - Optional: A constructor with one parameter of type `const
 *                SExpression&`
 *              - Optional: A method `void serialize(SExpression&) const`
 *              - Optional: Comparison operator overloadings
 *              - Optional: A method `Uuid getUuid() const noexcept`
 *              - Optional: A method `QString getName() const noexcept`
 * @tparam P  A class which provides the S-Expression node tag name of the list
 *            items. Example:
 *   `struct MyNameProvider {static constexpr const char* tagname = "item";};`
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
 * @warning Using Qt's `foreach` keyword on a ::librepcb::SerializableObjectList
 * is not recommended because it always creates a deep copy of the list! You
 * should use range based for loops (since C++11) instead.
 */
template <typename T, typename P, typename... OnEditedArgs>
class SerializableObjectList {
  Q_DECLARE_TR_FUNCTIONS(SerializableObjectList)

public:
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
    O& operator*() { return **it; }
    std::shared_ptr<O> ptr() noexcept {
      return std::const_pointer_cast<O>(*it);
    }
    ~Iterator() {}
  };
  using iterator = Iterator<typename QVector<std::shared_ptr<T>>::iterator, T>;
  using const_iterator =
      Iterator<typename QVector<std::shared_ptr<T>>::const_iterator, const T>;

  // Signals
  enum class Event {
    ElementAdded,
    ElementRemoved,
    ElementEdited,
  };
  Signal<SerializableObjectList<T, P, OnEditedArgs...>, int,
         const std::shared_ptr<const T>&, Event>
      onEdited;
  typedef Slot<SerializableObjectList<T, P, OnEditedArgs...>, int,
               const std::shared_ptr<const T>&, Event>
      OnEditedSlot;
  Signal<SerializableObjectList<T, P, OnEditedArgs...>, int,
         const std::shared_ptr<const T>&, OnEditedArgs...>
      onElementEdited;
  typedef Slot<SerializableObjectList<T, P, OnEditedArgs...>, int,
               const std::shared_ptr<const T>&, OnEditedArgs...>
      OnElementEditedSlot;

  // Constructors / Destructor
  SerializableObjectList() noexcept
    : onEdited(*this),
      onElementEdited(*this),
      mOnEditedSlot(
          *this,
          &SerializableObjectList<T, P,
                                  OnEditedArgs...>::elementEditedHandler) {}
  SerializableObjectList(
      const SerializableObjectList<T, P, OnEditedArgs...>& other) noexcept
    : onEdited(*this),
      onElementEdited(*this),
      mOnEditedSlot(
          *this,
          &SerializableObjectList<T, P,
                                  OnEditedArgs...>::elementEditedHandler) {
    *this = other;  // copy all elements
  }
  SerializableObjectList(
      SerializableObjectList<T, P, OnEditedArgs...>&& other) noexcept
    : onEdited(*this),
      onElementEdited(*this),
      mOnEditedSlot(
          *this,
          &SerializableObjectList<T, P,
                                  OnEditedArgs...>::elementEditedHandler) {
    while (!other.isEmpty()) {
      append(other.take(0));  // copy all pointers (NOT the objects!)
    }
  }
  SerializableObjectList(
      std::initializer_list<std::shared_ptr<T>> elements) noexcept
    : onEdited(*this),
      onElementEdited(*this),
      mOnEditedSlot(
          *this,
          &SerializableObjectList<T, P,
                                  OnEditedArgs...>::elementEditedHandler) {
    foreach (const std::shared_ptr<T>& obj, elements) { append(obj); }
  }
  explicit SerializableObjectList(const SExpression& node)
    : onEdited(*this),
      onElementEdited(*this),
      mOnEditedSlot(
          *this,
          &SerializableObjectList<T, P,
                                  OnEditedArgs...>::elementEditedHandler) {
    loadFromSExpression(node);  // can throw
  }
  virtual ~SerializableObjectList() noexcept {}

  // Getters
  bool isEmpty() const noexcept { return mObjects.empty(); }
  int count() const noexcept { return mObjects.count(); }
  const QVector<std::shared_ptr<T>>& values() const noexcept {
    return mObjects;
  }
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
  std::shared_ptr<T>& first() noexcept { return mObjects.first(); }
  std::shared_ptr<const T> first() const noexcept { return mObjects.first(); }
  std::shared_ptr<T>& last() noexcept { return mObjects.last(); }
  std::shared_ptr<const T> last() const noexcept { return mObjects.last(); }
  std::shared_ptr<T> get(const T* obj) {
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
  iterator begin() noexcept { return mObjects.begin(); }
  iterator end() noexcept { return mObjects.end(); }

  // General Methods
  int loadFromSExpression(const SExpression& node) {
    clear();
    foreach (const SExpression* child, node.getChildren(P::tagname)) {
      append(std::make_shared<T>(*child));  // can throw
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
    insertElement(index, obj);
    return index;
  }
  int append(const std::shared_ptr<T>& obj) noexcept {
    return insert(count(), obj);
  }
  void append(SerializableObjectList& list) noexcept {  // shallow -> NOT const!
    mObjects.reserve(mObjects.count() + list.count());
    foreach (const std::shared_ptr<T>& ptr, list.mObjects) {
      append(ptr);  // copy only the pointer, NOT the object
    }
  }
  std::shared_ptr<T> take(int index) noexcept {
    Q_ASSERT(contains(index));
    return takeElement(index);
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
    Q_ASSERT(isEmpty() && mObjects.isEmpty());
  }

  /**
   * @brief Serialize into ::librepcb::SExpression node
   *
   * @param root    Root node to serialize into.
   */
  void serialize(SExpression& root) const {
    foreach (const std::shared_ptr<T>& ptr, mObjects) {
      root.ensureLineBreak();
      ptr->serialize(root.appendList(P::tagname));  // can throw
    }
    root.ensureLineBreak();
  }

  // Convenience Methods
  template <typename Compare>
  SerializableObjectList<T, P, OnEditedArgs...> sorted(Compare lessThan) const
      noexcept {
    SerializableObjectList<T, P, OnEditedArgs...> copiedList;
    copiedList.mObjects = mObjects;  // copy only the pointers, not the objects!
    std::sort(copiedList.mObjects.begin(), copiedList.mObjects.end(),
              [&lessThan](const std::shared_ptr<T>& ptr1,
                          const std::shared_ptr<T>& ptr2) {
                return lessThan(*ptr1, *ptr2);
              });
    return copiedList;
  }
  SerializableObjectList<T, P, OnEditedArgs...> sortedByUuid() const noexcept {
    return sorted([](const T& lhs, const T& rhs) {
      return lhs.getUuid() < rhs.getUuid();
    });
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
  bool operator==(
      const SerializableObjectList<T, P, OnEditedArgs...>& rhs) const noexcept {
    if (rhs.mObjects.count() != mObjects.count()) return false;
    for (int i = 0; i < mObjects.count(); ++i) {
      if (*rhs.mObjects[i] != *mObjects[i]) return false;
    }
    return true;
  }
  bool operator!=(
      const SerializableObjectList<T, P, OnEditedArgs...>& rhs) const noexcept {
    return !(*this == rhs);
  }
  SerializableObjectList<T, P, OnEditedArgs...>& operator=(
      const SerializableObjectList<T, P, OnEditedArgs...>& rhs) noexcept {
    clear();
    mObjects.reserve(rhs.count());
    foreach (const std::shared_ptr<T>& ptr, rhs.mObjects) {
      append(std::make_shared<T>(*ptr));  // call copy constructor of object
    }
    return *this;
  }
  SerializableObjectList<T, P, OnEditedArgs...>& operator=(
      SerializableObjectList<T, P, OnEditedArgs...>&& rhs) noexcept {
    clear();
    mObjects.reserve(rhs.count());
    foreach (const std::shared_ptr<T>& ptr, rhs.mObjects) {
      append(ptr);  // copy only the pointer, NOT the object
    }
    rhs.clear();
    return *this;
  }

protected:  // Methods
  void insertElement(int index, const std::shared_ptr<T>& obj) noexcept {
    mObjects.insert(index, obj);
    obj->onEdited.attach(mOnEditedSlot);
    onEdited.notify(index, obj, Event::ElementAdded);
  }
  std::shared_ptr<T> takeElement(int index) noexcept {
    std::shared_ptr<T> obj = mObjects.takeAt(index);
    obj->onEdited.detach(mOnEditedSlot);
    onEdited.notify(index, obj, Event::ElementRemoved);
    return obj;
  }
  void elementEditedHandler(const T& obj, OnEditedArgs... args) noexcept {
    int index = indexOf(&obj);
    if (contains(index)) {
      onElementEdited.notify(index, at(index), args...);
      onEdited.notify(index, at(index), Event::ElementEdited);
    } else {
      qCritical() << "Received notification from unknown list element!";
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
  Slot<T, OnEditedArgs...> mOnEditedSlot;
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

template <typename T, typename P, typename... OnEditedArgs>
class QForeachContainer<
    librepcb::SerializableObjectList<T, P, OnEditedArgs...>> {
public:
  ~QForeachContainer() = delete;
};
template <typename T, typename P, typename... OnEditedArgs>
class QForeachContainer<
    const librepcb::SerializableObjectList<T, P, OnEditedArgs...>> {
public:
  ~QForeachContainer() = delete;
};

#if (QT_VERSION > QT_VERSION_CHECK(5, 9, 0))
}  // namespace QtPrivate
#endif

/*******************************************************************************
 *  End of File
 ******************************************************************************/

#endif
