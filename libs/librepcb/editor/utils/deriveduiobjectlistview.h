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

#ifndef LIBREPCB_EDITOR_DERIVEDUIOBJECTLISTVIEW_H
#define LIBREPCB_EDITOR_DERIVEDUIOBJECTLISTVIEW_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "uiobjectlist.h"

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Class DerivedUiObjectList
 ******************************************************************************/

/**
 * @brief The DerivedUiObjectList class
 */
template <typename TList, typename TDerived, typename TDerivedUiData>
class DerivedUiObjectList : public slint::Model<TDerivedUiData> {
public:
  // Constructors / Destructor
  DerivedUiObjectList(const DerivedUiObjectList& other) = delete;
  explicit DerivedUiObjectList(const std::shared_ptr<TList>& list) noexcept
    : mList(list),
      mOnListEditedSlot(
          *this,
          &DerivedUiObjectList<TList, TDerived,
                               TDerivedUiData>::listEditedHandler),
      mOnDerivedUiDataChangedSlot(
          *this,
          &DerivedUiObjectList<TList, TDerived, TDerivedUiData>::
              elementDerivedUiDataChangedHandler) {
    Q_ASSERT(mList);
    for (auto obj : *mList) {
      if (auto o = std::dynamic_pointer_cast<TDerived>(obj)) {
        o->onDerivedUiDataChanged.attach(mOnDerivedUiDataChangedSlot);
      }
    }
    mList->onEdited.attach(mOnListEditedSlot);
  }
  ~DerivedUiObjectList() noexcept {}

  // Implementations
  std::size_t row_count() const noexcept override { return mList->row_count(); }
  std::optional<TDerivedUiData> row_data(std::size_t i) const override {
    if (auto obj = std::dynamic_pointer_cast<TDerived>(mList->value(i))) {
      return obj->getDerivedUiData();
    }
    return std::nullopt;
  }
  void set_row_data(size_t i, const TDerivedUiData& data) noexcept override {
    if (auto obj = std::dynamic_pointer_cast<TDerived>(mList->value(i))) {
      obj->setDerivedUiData(data);
    }
  }

  // Operator Overloadings
  DerivedUiObjectList& operator=(const DerivedUiObjectList& rhs) = delete;

private:
  void listEditedHandler(
      const TList& list, int index,
      const std::shared_ptr<const typename TList::Element>& obj,
      typename TList::Event event) noexcept {
    Q_UNUSED(list);
    Q_UNUSED(obj);
    switch (event) {
      case TList::Event::ElementAdded: {
        slint::Model<TDerivedUiData>::row_added(index, 1);
        if (auto o = std::dynamic_pointer_cast<TDerived>(mList->value(index))) {
          Q_ASSERT(o == obj);
          o->onDerivedUiDataChanged.attach(mOnDerivedUiDataChangedSlot);
        }
        break;
      }
      case TList::Event::ElementRemoved: {
        slint::Model<TDerivedUiData>::row_removed(index, 1);
        if (auto o = std::dynamic_pointer_cast<TDerived>(mList->value(index))) {
          Q_ASSERT(o == obj);
          o->onDerivedUiDataChanged.detach(mOnDerivedUiDataChangedSlot);
        }
        break;
      }
      default:
        break;
    }
  }

  void elementDerivedUiDataChangedHandler(const TDerived& obj) noexcept {
    if (auto index =
            mList->indexOf(static_cast<const typename TList::Element*>(&obj))) {
      slint::Model<TDerivedUiData>::row_changed(*index);
    }
  }

  std::shared_ptr<TList> mList;
  typename TList::OnEditedSlot mOnListEditedSlot;
  Slot<TDerived> mOnDerivedUiDataChangedSlot;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
