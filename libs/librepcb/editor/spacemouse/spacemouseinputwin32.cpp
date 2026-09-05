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
 *
 * The Windows Raw Input based device detection/decoding approach used in
 * this file follows the same approach as FreeCAD's
 * `src/Gui/3Dconnexion/GuiNativeEventWin32.cpp` (LGPL-2.1-or-later,
 * Copyright (C) 2018 Torsten Sadowski), which in turn credits David
 * Dibben's 2011 blog post on Qt + 3Dconnexion integration as its origin.
 * See the feature plan doc, section 5, for the full LGPL -> GPLv3
 * compatibility check for this provenance chain.
 */

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "spacemouseinputwin32.h"

#include <QtCore>

// clang-format off
// windows.h must come first; WIN32_LEAN_AND_MEAN keeps it from pulling in
// most of the legacy Win32 API surface this file doesn't need.
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
// clang-format on

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

namespace {

// HID vendor IDs of devices known to report 3Dconnexion-compatible
// translation/rotation packets (see the feature plan doc, section 1 - same
// IDs FreeCAD's GuiNativeEventWin32.cpp checks for).
constexpr quint16 kVendorIdLogitech3Dx = 0x046d;  ///< Legacy Logitech-branded
constexpr quint16 kVendorId3Dconnexion = 0x256f;  ///< Current 3Dconnexion

// HID usage page/ID identifying a "multi-axis controller", i.e. the device
// class a 3D mouse registers itself as.
constexpr USHORT kHidUsagePageGeneric = 0x01;
constexpr USHORT kHidUsageMultiAxisController = 0x08;

// HID report IDs used by 3Dconnexion devices on the multi-axis-controller
// interface: translation and rotation arrive as two separate reports (each
// three little-endian signed 16-bit values), buttons as a third one we
// intentionally never decode (see IF_SpaceMouseInputBackend's doc comment).
constexpr quint8 kReportIdTranslation = 1;
constexpr quint8 kReportIdRotation = 2;

bool isKnownVendor(quint16 vendorId) noexcept {
  return (vendorId == kVendorIdLogitech3Dx) ||
      (vendorId == kVendorId3Dconnexion);
}

qint16 readInt16Le(const BYTE* p) noexcept {
  return static_cast<qint16>(static_cast<quint16>(p[0]) |
                             (static_cast<quint16>(p[1]) << 8));
}

}  // namespace

/*******************************************************************************
 *  Class SpaceMouseInputWin32::NativeEventFilter
 ******************************************************************************/

/**
 * @brief Application-wide native event filter decoding `WM_INPUT` messages
 *
 * Kept as a private nested implementation so `<windows.h>` never leaks into
 * ::SpaceMouseInputWin32's header (and thus not into any file that includes
 * it), avoiding the usual `min`/`max` macro collisions with Qt/STL code.
 *
 * As a nested class it has full access to ::SpaceMouseInputWin32's members
 * (including its protected/private ones, e.g. to emit its signals), the same
 * as any other member would.
 */
class SpaceMouseInputWin32::NativeEventFilter final
  : public QAbstractNativeEventFilter {
public:
  explicit NativeEventFilter(SpaceMouseInputWin32& backend) noexcept
    : mBackend(backend), mMessageWindow(createMessageWindow()) {
    if (mMessageWindow) {
      registerForRawInput(mMessageWindow);
    }
  }

  ~NativeEventFilter() noexcept override {
    if (mMessageWindow) {
      ::DestroyWindow(mMessageWindow);
    }
  }

  bool isDeviceConnected() const noexcept { return mDeviceConnected; }

  bool nativeEventFilter(const QByteArray& eventType, void* message,
                         qintptr* result) noexcept override {
    Q_UNUSED(result);
    if (eventType != "windows_generic_MSG") {
      return false;
    }
    const MSG* msg = static_cast<const MSG*>(message);
    if (msg->message == WM_INPUT) {
      handleRawInput(reinterpret_cast<HRAWINPUT>(msg->lParam));
    }
    return false;  // Never swallow it - other filters may need it too.
  }

private:
  static HWND createMessageWindow() noexcept {
    // A message-only window (`HWND_MESSAGE` parent), using the already
    // registered built-in "STATIC" window class. RegisterRawInputDevices()
    // needs *some* target HWND to receive WM_INPUT even with
    // RIDEV_INPUTSINK, but since WM_INPUT is intercepted via the
    // application-wide Qt native event filter below (which sees every MSG
    // pulled off this thread's queue, regardless of which window it
    // targets) rather than a custom window procedure, there's no need to
    // register a dedicated window class just for this.
    HWND hwnd = ::CreateWindowExW(0, L"STATIC", L"LibrePCB SpaceMouse", 0, 0,
                                  0, 0, 0, HWND_MESSAGE, nullptr,
                                  ::GetModuleHandleW(nullptr), nullptr);
    if (!hwnd) {
      qWarning() << "SpaceMouse: Failed to create message-only window, "
                    "GetLastError() ="
                 << ::GetLastError();
    }
    return hwnd;
  }

  static void registerForRawInput(HWND target) noexcept {
    RAWINPUTDEVICE rid;
    ::ZeroMemory(&rid, sizeof(rid));
    rid.usUsagePage = kHidUsagePageGeneric;
    rid.usUsage = kHidUsageMultiAxisController;
    // RIDEV_INPUTSINK: keep receiving input even when `target` is not the
    // foreground window, so panning/zooming keeps working no matter which
    // LibrePCB window currently has focus ("Phase 3" of the feature plan
    // decides which editor *tab* the motion actually gets applied to).
    rid.dwFlags = RIDEV_INPUTSINK;
    rid.hwndTarget = target;
    if (!::RegisterRawInputDevices(&rid, 1, sizeof(rid))) {
      qWarning() << "SpaceMouse: RegisterRawInputDevices() failed, "
                    "GetLastError() ="
                 << ::GetLastError();
    }
  }

  void handleRawInput(HRAWINPUT hRawInput) noexcept {
    UINT size = 0;
    if (::GetRawInputData(hRawInput, RID_INPUT, nullptr, &size,
                          sizeof(RAWINPUTHEADER)) != 0) {
      return;
    }
    QByteArray buffer(static_cast<int>(size), Qt::Uninitialized);
    if (::GetRawInputData(hRawInput, RID_INPUT, buffer.data(), &size,
                          sizeof(RAWINPUTHEADER)) != size) {
      return;
    }
    const RAWINPUT* raw =
        reinterpret_cast<const RAWINPUT*>(buffer.constData());
    if (raw->header.dwType != RIM_TYPEHID) {
      return;
    }
    if (!isFromKnownDevice(raw->header.hDevice)) {
      return;
    }
    setDeviceConnected(true);

    const BYTE* report = raw->data.hid.bRawData;
    const DWORD reportSize = raw->data.hid.dwSizeHid;
    if ((!report) || (reportSize < 7)) {
      return;
    }
    const quint8 reportId = report[0];
    if ((reportId != kReportIdTranslation) &&
        (reportId != kReportIdRotation)) {
      return;  // Buttons or an unrecognized report - not our job, ignore.
    }

    const qint16 a = readInt16Le(report + 1);
    const qint16 b = readInt16Le(report + 3);
    const qint16 c = readInt16Le(report + 5);

    SpaceMouseMotionEvent event = mLastEvent;
    if (reportId == kReportIdTranslation) {
      event.translationX = a;
      event.translationY = b;
      event.translationZ = c;
    } else {
      event.rotationX = a;
      event.rotationY = b;
      event.rotationZ = c;
    }
    mLastEvent = event;
    emit mBackend.motionEvent(event);
  }

  bool isFromKnownDevice(HANDLE hDevice) noexcept {
    // Cache the vendor-ID lookup per device handle - GetRawInputDeviceInfo()
    // is comparatively expensive and WM_INPUT can arrive at a high rate
    // (3Dconnexion devices commonly report well above 100 Hz).
    auto it = mDeviceVendorCache.find(hDevice);
    if (it == mDeviceVendorCache.end()) {
      RID_DEVICE_INFO info;
      ::ZeroMemory(&info, sizeof(info));
      info.cbSize = sizeof(info);
      UINT size = sizeof(info);
      const bool ok =
          ::GetRawInputDeviceInfoW(hDevice, RIDI_DEVICEINFO, &info, &size) >
          0;
      const quint16 vendorId = (ok && (info.dwType == RIM_TYPEHID))
          ? static_cast<quint16>(info.hid.dwVendorId)
          : quint16(0);
      it = mDeviceVendorCache.insert(hDevice, vendorId);
    }
    return isKnownVendor(it.value());
  }

  void setDeviceConnected(bool connected) noexcept {
    if (connected != mDeviceConnected) {
      mDeviceConnected = connected;
      emit mBackend.deviceConnectedChanged(connected);
    }
  }

  SpaceMouseInputWin32& mBackend;
  HWND mMessageWindow;
  bool mDeviceConnected = false;
  SpaceMouseMotionEvent mLastEvent;
  QHash<HANDLE, quint16> mDeviceVendorCache;
};

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

SpaceMouseInputWin32::SpaceMouseInputWin32(QObject* parent) noexcept
  : IF_SpaceMouseInputBackend(parent),
    mFilter(std::make_unique<NativeEventFilter>(*this)) {
  qApp->installNativeEventFilter(mFilter.get());
}

SpaceMouseInputWin32::~SpaceMouseInputWin32() noexcept {
  if (qApp) {
    qApp->removeNativeEventFilter(mFilter.get());
  }
}

/*******************************************************************************
 *  IF_SpaceMouseInputBackend Methods
 ******************************************************************************/

bool SpaceMouseInputWin32::isDeviceConnected() const noexcept {
  return mFilter->isDeviceConnected();
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
