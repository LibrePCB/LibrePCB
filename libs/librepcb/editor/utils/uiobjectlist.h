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

#ifndef LIBREPCB_EDITOR_UIOBJECTLIST_H
#define LIBREPCB_EDITOR_UIOBJECTLIST_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/core/utils/signalslot.h>

#include <QtCore>

#include <memory>
#include <slint.h>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Class UiObjectList
 ******************************************************************************/

/**
 * @brief The UiObjectList class
 */
template <typename TObj, typename TUiData>
class UiObjectList : public slint::Model<TUiData> {
public:
  typedef TObj Element;
  typedef TUiData UiData;

  // Signals
  enum class Event {
    ElementAdded,
    ElementRemoved,
    ElementUiDataChanged,
  };
  Signal<UiObjectList<TObj, TUiData>, int, const std::shared_ptr<TObj>&, Event>
      onEdited;
  typedef Slot<UiObjectList<TObj, TUiData>, int, const std::shared_ptr<TObj>&,
               Event>
      OnEditedSlot;

  // Constructors / Destructor
  UiObjectList() noexcept
    : onEdited(*this),
      mObjects(),
      mOnUiDataChangedSlot(
          *this, &UiObjectList<TObj, TUiData>::elementUiDataChangedHandler) {}
  UiObjectList(const UiObjectList& other) = delete;
  ~UiObjectList() noexcept {}

  // General Methods
  int count() const noexcept { return mObjects.count(); }
  bool isEmpty() const noexcept { return mObjects.isEmpty(); }
  std::shared_ptr<TObj> at(int index) noexcept { return mObjects.at(index); }
  std::shared_ptr<TObj> value(int index) noexcept {
    return mObjects.value(index);
  }
  void append(const std::shared_ptr<TObj>& obj) noexcept {
    insert(mObjects.count(), obj);
  }
  void insert(int index, const std::shared_ptr<TObj>& obj) noexcept {
    Q_ASSERT(obj);
    index = qBound(0, index, mObjects.count() + 1);
    mObjects.insert(index, obj);
    slint::Model<TUiData>::notify_row_added(index, 1);
    obj->onUiDataChanged.attach(mOnUiDataChangedSlot);
    onEdited.notify(index, obj, Event::ElementAdded);
  }
  bool remove(int index) noexcept { return takeAt(index) != nullptr; }
  std::shared_ptr<TObj> take(const TObj* obj) noexcept {
    if (auto index = indexOf(obj)) {
      return takeAt(*index);
    } else {
      return nullptr;
    }
  }
  std::shared_ptr<TObj> takeAt(int index) noexcept {
    if (auto obj = mObjects.takeAt(index)) {
      slint::Model<TUiData>::notify_row_removed(index, 1);
      obj->onUiDataChanged.detach(mOnUiDataChangedSlot);
      onEdited.notify(index, obj, Event::ElementRemoved);
      return obj;
    } else {
      return nullptr;
    }
  }
  void clear() noexcept {
    for (int i = mObjects.count() - 1; i >= 0; --i) {
      takeAt(i);
    }
  }
  std::optional<int> indexOf(const TObj* obj) const noexcept {
    for (int i = 0; i < mObjects.count(); ++i) {
      if (mObjects.at(i).get() == obj) {
        return i;
      }
    }
    return std::nullopt;
  }
  const QVector<std::shared_ptr<TObj>>& values() { return mObjects; }
  auto begin() noexcept { return mObjects.begin(); }
  auto end() noexcept { return mObjects.end(); }

  // Implementations
  std::size_t row_count() const noexcept override {
    return static_cast<std::size_t>(mObjects.count());
  }
  std::optional<TUiData> row_data(std::size_t i) const override {
    if (auto obj = mObjects.value(i)) {
      return obj->getUiData();
    }
    return std::nullopt;
  }
  void set_row_data(size_t i, const TUiData& data) noexcept override {
    if (auto obj = mObjects.value(i)) {
      obj->setUiData(data);
    }
  }

  // Operator Overloadings
  UiObjectList& operator=(const UiObjectList& rhs) = delete;

private:
  void elementUiDataChangedHandler(const TObj& obj) noexcept {
    if (auto index = indexOf(&obj)) {
      slint::Model<TUiData>::notify_row_changed(*index);
      onEdited.notify(*index, at(*index), Event::ElementUiDataChanged);
    }
  }

  QVector<std::shared_ptr<TObj>> mObjects;
  Slot<TObj> mOnUiDataChangedSlot;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
