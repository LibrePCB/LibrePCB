/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2016 The LibrePCB developers
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
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <librepcb/common/signalslot.h>

#include <QtCore>

#include <memory>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace tests {

/*******************************************************************************
 *  Helpers
 ******************************************************************************/

struct Sender {
  Signal<Sender, int> signal;
  Sender() : signal(*this) {}
};

struct Receiver {
  Slot<Sender, int> slot;
  Receiver() : slot(*this, &Receiver::callback) {}
  MOCK_METHOD2(callback, void(const Sender&, int));
};

/*******************************************************************************
 *  Test Class
 ******************************************************************************/
class SignalSlotTest : public ::testing::Test {};

/*******************************************************************************
 *  Test Methods
 ******************************************************************************/

TEST(SignalSlotTest, testDuringCallbackAttachedSlotsAreNotCalled) {
  Sender sender;
  QList<std::shared_ptr<Receiver>> receivers;
  Slot<Sender, int> slot([&](const Sender&, int) {
    for (int i = 0; i < 100; ++i) {
      std::shared_ptr<Receiver> receiver = std::make_shared<Receiver>();
      sender.signal.attach(receiver->slot);
      receivers.append(receiver);
    }
  });
  sender.signal.attach(slot);

  EXPECT_EQ(1, sender.signal.getSlotCount());
  sender.signal.notify(42);
  EXPECT_EQ(101, sender.signal.getSlotCount());
  foreach (const auto& receiver, receivers) {
    EXPECT_CALL(*receiver, callback(testing::_, testing::_)).Times(0);
  }
}

TEST(SignalSlotTest, testDuringCallbackDetachedSlotsAreNotCalled) {
  int callbackCounter = 0;
  Sender sender;
  QList<std::shared_ptr<Slot<Sender, int>>> receivers;
  for (int i = 0; i < 100; ++i) {
    std::shared_ptr<Slot<Sender, int>> receiver =
        std::make_shared<Slot<Sender, int>>([&](const Sender&, int) {
          ++callbackCounter;
          foreach (const auto& r, receivers) { sender.signal.detach(*r); }
        });
    receivers.append(receiver);
    sender.signal.attach(*receiver);
  }

  EXPECT_EQ(100, sender.signal.getSlotCount());
  sender.signal.notify(42);
  EXPECT_EQ(0, sender.signal.getSlotCount());
  EXPECT_EQ(1, callbackCounter);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace librepcb
