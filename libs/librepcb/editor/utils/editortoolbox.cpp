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

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "editortoolbox.h"

#include "../project/partinformationprovider.h"
#include "../workspace/desktopservices.h"
#include "menubuilder.h"

#include <librepcb/core/3d/occmodel.h>
#include <librepcb/core/application.h>
#include <librepcb/core/library/cmp/component.h>
#include <librepcb/core/library/dev/device.h>
#include <librepcb/core/project/board/items/bi_device.h>
#include <librepcb/core/project/circuit/circuit.h>
#include <librepcb/core/project/circuit/componentinstance.h>
#include <librepcb/core/project/project.h>
#include <librepcb/core/project/projectlibrary.h>
#include <librepcb/core/systeminfo.h>
#include <librepcb/core/workspace/workspace.h>
#include <librepcb/core/workspace/workspacelibrarydb.h>
#include <librepcb/core/workspace/workspacesettings.h>

#include <QtCore>
#include <QtNetwork>
#include <QtWidgets>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace editor {

/*******************************************************************************
 *  Non-Member Functions
 ******************************************************************************/

int q2s(int i) noexcept {
  return i;
}

slint::LogicalPosition q2s(const QPointF& p) noexcept {
  return slint::LogicalPosition(
      {static_cast<float>(p.x()), static_cast<float>(p.y())});
}

slint::PhysicalPosition q2s(const QPoint& p) noexcept {
  return slint::PhysicalPosition({p.x(), p.y()});
}

slint::PhysicalSize q2s(const QSize& s) noexcept {
  return slint::PhysicalSize({static_cast<uint32_t>(std::max(s.width(), 0)),
                              static_cast<uint32_t>(std::max(s.height(), 0))});
}

slint::SharedString q2s(const QString& s) noexcept {
  return slint::SharedString(s.toUtf8().data());
}

slint::Image q2s(const QPixmap& p) noexcept {
  if (p.isNull()) {
    return slint::Image();
  }

  QImage img = p.toImage();
  img.convertTo(QImage::Format_RGBA8888);
  return slint::Image(slint::SharedPixelBuffer<slint::Rgba8Pixel>(
      img.width(), img.height(),
      reinterpret_cast<const slint::Rgba8Pixel*>(img.bits())));
}

slint::Color q2s(const QColor& c) noexcept {
  return slint::Color::from_argb_uint8(c.alpha(), c.red(), c.green(), c.blue());
}

slint::private_api::MouseCursor q2s(Qt::CursorShape s) noexcept {
  switch (s) {
    case Qt::ArrowCursor:
      return slint::private_api::MouseCursor::Default;
    case Qt::PointingHandCursor:
      return slint::private_api::MouseCursor::Pointer;
    case Qt::CrossCursor:
      return slint::private_api::MouseCursor::Crosshair;
    default: {
      qWarning() << "Unsupported cursor shape:" << s;
      return slint::private_api::MouseCursor::Default;
    }
  }
}

QPointF s2q(const slint::LogicalPosition& p) noexcept {
  return QPointF(p.x, p.y);
}

QPoint s2q(const slint::PhysicalPosition& p) noexcept {
  return QPoint(p.x, p.y);
}

QSize s2q(const slint::PhysicalSize& s) noexcept {
  return QSize(s.width, s.height);
}

QString s2q(const slint::SharedString& s) noexcept {
  std::string_view view(s);
  return QString::fromUtf8(view.data(), view.size());
}

Qt::MouseButton s2q(const slint::private_api::PointerEventButton& b) noexcept {
  switch (b) {
    case slint::private_api::PointerEventButton::Left:
      return Qt::LeftButton;
    case slint::private_api::PointerEventButton::Right:
      return Qt::RightButton;
    case slint::private_api::PointerEventButton::Middle:
      return Qt::MiddleButton;
    case slint::private_api::PointerEventButton::Back:
      return Qt::BackButton;
    case slint::private_api::PointerEventButton::Forward:
      return Qt::ForwardButton;
    default:
      return Qt::NoButton;
  }
}

Qt::KeyboardModifiers s2q(
    const slint::private_api::KeyboardModifiers& m) noexcept {
  Qt::KeyboardModifiers ret;
  ret.setFlag(Qt::ShiftModifier, m.shift);
  ret.setFlag(Qt::ControlModifier, m.control);
  ret.setFlag(Qt::AltModifier, m.alt);
  ret.setFlag(Qt::MetaModifier, m.meta);
  return ret;
}

bool operator==(const QString& s1, const slint::SharedString& s2) noexcept {
  return s1.toUtf8().data() == std::string_view(s2);
}

bool operator!=(const QString& s1, const slint::SharedString& s2) noexcept {
  return s1.toUtf8().data() != std::string_view(s2);
}

bool operator==(const slint::SharedString& s1, const QString& s2) noexcept {
  return std::string_view(s1) == s2.toUtf8().data();
}

bool operator!=(const slint::SharedString& s1, const QString& s2) noexcept {
  return std::string_view(s1) != s2.toUtf8().data();
}

static slint::SharedString getError(const QString& input) {
  if (input.trimmed().isEmpty()) {
    return q2s(QCoreApplication::translate("AppToolbox", "Required"));
  } else {
    return q2s(QCoreApplication::translate("AppToolbox", "Invalid"));
  }
}

std::optional<ElementName> validateElementName(
    const QString& input, slint::SharedString& error) noexcept {
  const std::optional<ElementName> ret =
      parseElementName(cleanElementName(input));
  if (!ret) {
    error = getError(input);
  } else {
    error = slint::SharedString();
  }
  return ret;
}

std::optional<Version> validateVersion(const QString& input,
                                       slint::SharedString& error) noexcept {
  const std::optional<Version> ret = Version::tryFromString(input.trimmed());
  if (!ret) {
    error = getError(input);
  } else {
    error = slint::SharedString();
  }
  return ret;
}

std::optional<FileProofName> validateFileProofName(
    const QString& input, slint::SharedString& error,
    const QString& requiredSuffix) noexcept {
  std::optional<FileProofName> ret =
      parseFileProofName(cleanFileProofName(input));
  if (!ret) {
    error = getError(input);
  } else if ((!requiredSuffix.isEmpty()) &&
             (!input.trimmed().endsWith(requiredSuffix))) {
    ret = std::nullopt;
    error = q2s(QCoreApplication::translate("AppToolbox", "Suffix '%1' missing")
                    .arg(requiredSuffix));
  } else {
    error = slint::SharedString();
  }
  return ret;
}

std::optional<QUrl> validateUrl(const QString& input,
                                slint::SharedString& error,
                                bool allowEmpty) noexcept {
  const QUrl url = QUrl::fromUserInput(input.trimmed());
  const std::optional<QUrl> ret =
      url.isValid() ? std::make_optional(url) : std::nullopt;
  if ((!ret) && ((!input.isEmpty()) || (!allowEmpty))) {
    error = getError(input);
  } else {
    error = slint::SharedString();
  }
  return ret;
}

/*******************************************************************************
 *  Static Methods
 ******************************************************************************/

void EditorToolbox::removeFormLayoutRow(QLabel& label) noexcept {
  if (auto layout = label.parentWidget()->layout()) {
    if (removeFormLayoutRow(*layout, label)) {
      return;
    }
  }
  qWarning().nospace() << "Failed to remove form layout row "
                       << label.objectName() << ".";
}

void EditorToolbox::deleteLayoutItemRecursively(QLayoutItem* item) noexcept {
  Q_ASSERT(item);
  if (QWidget* widget = item->widget()) {
    delete widget;
  } else if (QLayout* layout = item->layout()) {
    for (int i = layout->count() - 1; i >= 0; --i) {
      deleteLayoutItemRecursively(layout->takeAt(i));
    }
  } else if (QSpacerItem* spacer = item->spacerItem()) {
    delete spacer;
  }
  delete item;
}

bool EditorToolbox::startToolBarTabFocusCycle(
    QToolBar& toolBar, QWidget& returnFocusToWidget) noexcept {
  QWidget* previousWidget = nullptr;
  foreach (QAction* action, toolBar.actions()) {
    QWidget* widget = toolBar.widgetForAction(action);
    if (widget && (widget->focusPolicy() & Qt::TabFocus)) {
      if (!previousWidget) {
        widget->setFocus(Qt::TabFocusReason);
      } else {
        toolBar.setTabOrder(previousWidget, widget);
      }
      previousWidget = widget;
    }
  }
  if (previousWidget) {
    toolBar.setTabOrder(previousWidget, &returnFocusToWidget);
    return true;
  } else {
    return false;
  }
}

ResourceList EditorToolbox::getComponentResources(
    const WorkspaceLibraryDb& db, const ComponentInstance& cmp,
    const std::optional<Uuid>& filterDev) noexcept {
  // Helper to skip duplicate URLs.
  ResourceList resources;
  QSet<QUrl> urls;
  auto addResources = [&](const ResourceList& list) {
    for (const Resource& res : list) {
      if (res.getUrl().isValid() && (!urls.contains(res.getUrl()))) {
        resources.append(std::make_shared<Resource>(res));
        urls.insert(res.getUrl());
      }
    }
  };

  // Helper to catch exceptions and provide fallback resources.
  auto tryAddResources = [&](std::function<ResourceList()> getter,
                             const ResourceList& fallback) {
    ResourceList lst;
    try {
      lst = getter();  // can throw
    } catch (const Exception& e) {
      qWarning() << "Failed to get resources:" << e.getMsg();
    }
    addResources(lst.isEmpty() ? fallback : lst);
  };

  // Get resources of component.
  tryAddResources(
      [&]() {
        return db.getResources<Component>(
            db.getLatest<Component>(cmp.getLibComponent().getUuid()));
      },
      cmp.getLibComponent().getResources());

  // Determine relevant devices.
  QList<Uuid> devices;
  if (filterDev) {
    devices.append(*filterDev);
  } else {
    for (const BI_Device* dev : cmp.getDevices()) {
      if (!devices.contains(dev->getLibDevice().getUuid())) {
        devices.append(dev->getLibDevice().getUuid());
      }
    }
    for (const ComponentAssemblyOption& option : cmp.getAssemblyOptions()) {
      if (!devices.contains(option.getDevice())) {
        devices.append(option.getDevice());
      }
    }
  }

  // Get resources of devices.
  for (const Uuid& uuid : devices) {
    const Device* dev =
        cmp.getCircuit().getProject().getLibrary().getDevice(uuid);
    tryAddResources(
        [&]() { return db.getResources<Device>(db.getLatest<Device>(uuid)); },
        dev ? dev->getResources() : ResourceList());
  }

  return resources;
}

void EditorToolbox::addResourcesToMenu(const Workspace& ws, MenuBuilder& mb,
                                       const ComponentInstance& cmp,
                                       const std::optional<Uuid>& filterDev,
                                       QWidget& editor, QMenu& root) noexcept {
  QPointer<QWidget> editorPtr = &editor;

  // Get all relevant resources.
  ResourceList resources =
      getComponentResources(ws.getLibraryDb(), cmp, filterDev);

  // Limit number of resources.
  while (resources.count() > 15) {
    resources.remove(resources.count() - 1);
  }

  // Detect duplicate names.
  QStringList names;
  for (const auto& res : resources) {
    names.append(*res.getName());
  }

  // Build list of actions.
  QList<QAction*> actions;
  for (const Resource& res : resources) {
    QString name = *res.getName();
    if (names.count(name) > 1) {
      name += " (" % res.getUrl().fileName() % ")";
    }
    if (name.length() > 100) {
      name = name.left(97) + "…";
    }
    QAction* a =
        new QAction(QIcon(":/img/actions/pdf.png"), name % "...", &root);
    QObject::connect(a, &QAction::triggered, editorPtr,
                     [&ws, res, editorPtr]() {
                       DesktopServices::downloadAndOpenResourceAsync(
                           ws.getSettings(), *res.getName(), res.getMediaType(),
                           res.getUrl(), editorPtr);
                     });
    actions.append(a);
  }

  // If MPNs are available, provide search through API.
  if (!ws.getSettings().apiEndpoints.get().isEmpty()) {
    QList<Part> parts;
    for (const ComponentAssemblyOption& ao : cmp.getAssemblyOptions()) {
      for (const Part& part : ao.getParts()) {
        std::shared_ptr<PartInformationProvider::PartInformation> info =
            PartInformationProvider::instance().getPartInfo(
                PartInformationProvider::Part{*part.getMpn(),
                                              *part.getManufacturer()});
        if ((!part.getMpn()->isEmpty()) &&
            (!part.getManufacturer()->isEmpty()) &&
            ((!info) || (info->resources.value(0).url.isValid())) &&
            (!parts.contains(part)) && (actions.count() < 20)) {
          QAction* a = new QAction(
              QIcon(":/img/actions/search.png"),
              tr("Search datasheet for '%1'").arg(*part.getMpn()) % "...",
              &root);
          QObject::connect(
              a, &QAction::triggered, editorPtr, [&ws, part, editorPtr]() {
                searchAndOpenDatasheet(ws, *part.getMpn(),
                                       *part.getManufacturer(), editorPtr);
              });
          actions.append(a);
          parts.append(part);
        }
      }
    }
  }

  // Add menu items.
  if (!actions.isEmpty()) {
    mb.addSeparator();
  }
  const int nRoot = actions.count() > 3 ? 2 : 3;
  for (int i = 0; i < std::min(static_cast<int>(actions.count()), nRoot); ++i) {
    mb.addAction(actions.at(i));
  }
  if (actions.count() > nRoot) {
    QMenu* sm = mb.addSubMenu(&MenuBuilder::createMoreResourcesMenu);
    MenuBuilder smb(sm);
    for (int i = nRoot; i < actions.count(); ++i) {
      smb.addAction(actions.at(i));
    }
  }
}

QString EditorToolbox::buildAppVersionDetails() noexcept {
  // Always English, not translatable!
  QStringList details;
  const QString date = Application::getBuildDate().toString(Qt::ISODate);
  QString qt = QString(qVersion()) + " (built against " + QT_VERSION_STR + ")";
  details << "LibrePCB Version: " + Application::getVersion();
  details << "Git Revision:     " + Application::getGitRevision();
  details << "Build Date:       " + date;
  if (!Application::getBuildAuthor().isEmpty()) {
    details << "Build Author:     " + Application::getBuildAuthor();
  }
  details << "Qt Version:       " + qt;
  details << "CPU Architecture: " + QSysInfo::currentCpuArchitecture();
  details << "Operating System: " + QSysInfo::prettyProductName();
  details << "Platform Plugin:  " + qApp->platformName();
  details << "TLS Library:      " + QSslSocket::sslLibraryVersionString();
  details << "OCC Library:      " + OccModel::getOccVersionString();
  if (!SystemInfo::detectRuntime().isEmpty()) {
    details << "Runtime:          " + SystemInfo::detectRuntime();
  }
  return details.join("\n");
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

bool EditorToolbox::removeFormLayoutRow(QLayout& layout,
                                        QLabel& label) noexcept {
  if (auto formLayout = dynamic_cast<QFormLayout*>(&layout)) {
    for (int i = 0; i < formLayout->rowCount(); ++i) {
      QLayoutItem* labelItem = formLayout->itemAt(i, QFormLayout::LabelRole);
      QLayoutItem* fieldItem = formLayout->itemAt(i, QFormLayout::FieldRole);
      if ((labelItem) && (labelItem->widget() == &label) && (fieldItem)) {
        hideLayoutItem(*labelItem);
        hideLayoutItem(*fieldItem);
        formLayout->takeRow(i);  // Avoid ugly space caused by the empty rows.
        return true;
      }
    }
  }
  for (int i = 0; i < layout.count(); ++i) {
    if (QLayoutItem* item = layout.itemAt(i)) {
      if (QLayout* child = item->layout()) {
        if (removeFormLayoutRow(*child, label)) {
          return true;
        }
      }
    }
  }
  return false;
}

void EditorToolbox::hideLayoutItem(QLayoutItem& item) noexcept {
  if (QWidget* widget = item.widget()) {
    widget->hide();
  } else if (QLayout* layout = item.layout()) {
    for (int i = 0; i < layout->count(); ++i) {
      if (QLayoutItem* child = layout->itemAt(i)) {
        hideLayoutItem(*child);
      }
    }
  }
}

void EditorToolbox::searchAndOpenDatasheet(const Workspace& ws,
                                           const QString& mpn,
                                           const QString& manufacturer,
                                           QPointer<QWidget> parent) noexcept {
  auto openPartDatasheet =
      [&ws,
       parent](std::shared_ptr<PartInformationProvider::PartInformation> info) {
        if (info && (!info->resources.isEmpty()) &&
            (info->resources[0].url.isValid())) {
          DesktopServices::downloadAndOpenResourceAsync(
              ws.getSettings(), info->mpn, info->resources[0].mediaType,
              info->resources[0].url, parent);
        } else {
          QMessageBox::information(
              parent, tr("No datasheet found"),
              tr("Sorry, no datasheet found for the requested part :-("));
        }
      };

  PartInformationProvider& pip = PartInformationProvider::instance();
  const PartInformationProvider::Part pipPart{mpn, manufacturer};
  if (auto info = pip.getPartInfo(pipPart)) {
    openPartDatasheet(info);
    return;
  }
  QGuiApplication::setOverrideCursor(Qt::WaitCursor);
  if ((!pip.isOperational()) && (!pip.startOperation(5000))) {
    QGuiApplication::restoreOverrideCursor();
    QMessageBox::critical(parent, tr("Error"),
                          "Sorry, the API server is currently not "
                          "available. Please try again later.");
    return;
  }
  if (!pip.isOngoing(pipPart)) {
    pip.scheduleRequest(pipPart);
  }
  pip.requestScheduledParts();
  auto part = pip.waitForPartInfo(pipPart, 5000);
  QGuiApplication::restoreOverrideCursor();
  openPartDatasheet(part);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb
