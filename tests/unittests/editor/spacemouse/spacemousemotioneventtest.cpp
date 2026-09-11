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

// DISCLAIMER: Claude AI assisted in the writing of this file.
// It was reviewed by a human.

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <gtest/gtest.h>
#include <librepcb/editor/spacemouse/if_spacemouseinputbackend.h>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {
namespace tests {

/*******************************************************************************
 *  Test Methods
 *
 *  Note: The actual input backend (SpaceMouseInputRust, via the
 *  cross-platform rust-spacemouse crate) talks directly to OS-level raw HID
 *  input and thus needs real hardware to test meaningfully.  It is not
 *  unit-testable here. This file only covers the platform-independent
 *  SpaceMouseMotionEvent value type it produces.
 ******************************************************************************/

TEST(SpaceMouseMotionEventTest, testDefaultConstructedIsAllZero) {
  const SpaceMouseMotionEvent event;
  EXPECT_EQ(0, event.translationX);
  EXPECT_EQ(0, event.translationY);
  EXPECT_EQ(0, event.translationZ);
  EXPECT_EQ(0, event.rotationX);
  EXPECT_EQ(0, event.rotationY);
  EXPECT_EQ(0, event.rotationZ);
}

TEST(SpaceMouseMotionEventTest, testEqualityOperator) {
  SpaceMouseMotionEvent a;
  a.translationX = 12;
  a.rotationZ = -34;

  SpaceMouseMotionEvent b = a;
  EXPECT_TRUE(a == b);
  EXPECT_FALSE(a != b);

  b.translationY = 1;  // Differs in a field `a` doesn't set.
  EXPECT_FALSE(a == b);
  EXPECT_TRUE(a != b);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace editor
}  // namespace librepcb
