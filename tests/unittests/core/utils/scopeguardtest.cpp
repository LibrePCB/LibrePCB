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
#include <gtest/gtest.h>
#include <librepcb/common/scopeguard.h>
#include <librepcb/common/scopeguardlist.h>

#include <chrono>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace tests {

/*******************************************************************************
 *  Test Class
 ******************************************************************************/
class ScopeGuardTest : public ::testing::Test {};

/*******************************************************************************
 *  ScopeGuard Test Methods
 ******************************************************************************/

TEST(ScopeGuardTest, testScopeGuard) {
  bool setByGuard0 = false;
  {
    auto guard1 = scopeGuard([&] { setByGuard0 = true; });
  }
  EXPECT_TRUE(setByGuard0);
}

TEST(ScopeGuardTest, testScopeGuardDismiss) {
  bool setByGuard0 = false;
  {
    auto guard0 = scopeGuard([&] { setByGuard0 = true; });
    guard0.dismiss();
  }
  EXPECT_FALSE(setByGuard0);
}

TEST(ScopeGuardTest, testScopeGuardPerformance) {
  std::chrono::time_point<std::chrono::high_resolution_clock> start, end;
  // make volatile to avoid optimizations
  volatile int setByGuard0 = 0;
  start = std::chrono::high_resolution_clock::now();
  for (int n = 0; n < 10000000; ++n) {
    auto guard0 = scopeGuard([&] { setByGuard0 += 1; });
  }
  end = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> elapsed_seconds = end - start;
  std::cout << "Needed " << elapsed_seconds.count() << "s for " << setByGuard0
            << " loops\n";
}

/*******************************************************************************
 *  ScopeGuardList Test Methods
 ******************************************************************************/

TEST(ScopeGuardTest, testScopeGuardList) {
  bool setByGuard0 = false;
  bool setByGuard1 = false;
  {
    auto guardList = ScopeGuardList();

    // do stuff 0
    guardList.add([&] { setByGuard0 = true; });

    // do stuff 1
    guardList.add([&] { setByGuard1 = true; });
  }
  EXPECT_TRUE(setByGuard0);
  EXPECT_TRUE(setByGuard1);
}

TEST(ScopeGuardTest, testScopeGuardListDismiss) {
  bool setByGuard0 = false;
  bool setByGuard1 = false;
  {
    auto guardList = ScopeGuardList();

    guardList.add([&] { setByGuard0 = true; });
    guardList.add([&] { setByGuard1 = true; });

    guardList.dismiss();
  }
  EXPECT_FALSE(setByGuard0);
  EXPECT_FALSE(setByGuard1);
}

// check if entries are executed in reverse order
TEST(ScopeGuardTest, testScopeGuardListOrder) {
  int i = 0;
  {
    auto guardList = ScopeGuardList();

    guardList.add([&] { i *= 2; });
    guardList.add([&] { i += 1; });
  }
  // if the order of execution is correct: (0+1)*2 == 2
  EXPECT_EQ(2, i);
}

// check if presized scopeGuardList works
TEST(ScopeGuardTest, testScopeGuardListSizedConstructor) {
  auto guardList = ScopeGuardList(5);
}

// check if presized scopeGuardList works
TEST(ScopeGuardTest, testScopeGuardListEmptyCallback) {
  auto guardList = ScopeGuardList(2);
  guardList.add(std::function<void()>());
}

TEST(ScopeGuardTest, testScopeGuardListPerformance) {
  std::chrono::time_point<std::chrono::high_resolution_clock> start, end;
  // make volatile to avoid optimizations
  volatile int setByGuard0 = 0;
  start = std::chrono::high_resolution_clock::now();
  for (int n = 0; n < 10000000; ++n) {
    auto guardList = ScopeGuardList();
    guardList.add([&] { setByGuard0 += 1; });
  }
  end = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> elapsed_seconds = end - start;
  std::cout << "Needed " << elapsed_seconds.count() << "s for " << setByGuard0
            << " loops\n";
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace librepcb
