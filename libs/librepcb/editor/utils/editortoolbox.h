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

#ifndef LIBREPCB_EDITOR_EDITORTOOLBOX_H
#define LIBREPCB_EDITOR_EDITORTOOLBOX_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include <librepcb/core/library/resource.h>
#include <librepcb/core/types/elementname.h>
#include <librepcb/core/types/fileproofname.h>
#include <librepcb/core/types/uuid.h>
#include <librepcb/core/types/version.h>
#include <librepcb/core/workspace/theme.h>

#include <QtCore>
#include <QtGui>
#include <QtWidgets>

#include <optional>
#include <slint.h>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class ComponentInstance;
class Workspace;
class WorkspaceLibraryDb;

namespace editor {

class MenuBuilder;

/*******************************************************************************
 *  Non-Member Functions
 ******************************************************************************/

int q2s(int i) noexcept;
slint::LogicalPosition q2s(const QPointF& p) noexcept;
slint::PhysicalPosition q2s(const QPoint& p) noexcept;
slint::PhysicalSize q2s(const QSize& s) noexcept;
slint::SharedString q2s(const QString& s) noexcept;
slint::Image q2s(const QPixmap& p) noexcept;
slint::Color q2s(const QColor& c) noexcept;
slint::private_api::MouseCursor q2s(Qt::CursorShape s) noexcept;

QPointF s2q(const slint::LogicalPosition& p) noexcept;
QPoint s2q(const slint::PhysicalPosition& p) noexcept;
QSize s2q(const slint::PhysicalSize& s) noexcept;
QString s2q(const slint::SharedString& s) noexcept;
Qt::MouseButton s2q(const slint::private_api::PointerEventButton& b) noexcept;
Qt::KeyboardModifiers s2q(
    const slint::private_api::KeyboardModifiers& m) noexcept;

bool operator==(const QString& s1, const slint::SharedString& s2) noexcept;
bool operator!=(const QString& s1, const slint::SharedString& s2) noexcept;
bool operator==(const slint::SharedString& s1, const QString& s2) noexcept;
bool operator!=(const slint::SharedString& s1, const QString& s2) noexcept;

template <typename TTarget, typename TSlint, typename TClass, typename TQt>
static void bind(
    QObject* context, const TTarget& target,
    void (TTarget::*setter)(const TSlint&) const, TClass* source,
    void (TClass::*signal)(TQt), const TQt& defaultValue,
    std::function<TSlint(const TQt&)> convert = [](const TQt& value) {
      return q2s(value);
    }) noexcept {
  QObject::connect(source, signal, context,
                   [&target, setter, convert](const TQt& value) {
                     (target.*setter)(convert(value));
                   });
  (target.*setter)(convert(defaultValue));
}

std::optional<ElementName> validateElementName(
    const QString& input, slint::SharedString& error) noexcept;

std::optional<Version> validateVersion(const QString& input,
                                       slint::SharedString& error) noexcept;

std::optional<FileProofName> validateFileProofName(
    const QString& input, slint::SharedString& error,
    const QString& requiredSuffix = QString()) noexcept;

std::optional<QUrl> validateUrl(const QString& input,
                                slint::SharedString& error,
                                bool allowEmpty = false) noexcept;

/*******************************************************************************
 *  Class EditorToolbox
 ******************************************************************************/

/**
 * @brief The EditorToolbox class provides some useful general purpose methods
 *        for editors (i.e. GUI stuff)
 */
class EditorToolbox final {
  Q_DECLARE_TR_FUNCTIONS(EditorToolbox)

public:
  // Constructors / Destructor
  EditorToolbox() = delete;
  EditorToolbox(const EditorToolbox& other) = delete;
  ~EditorToolbox() = delete;

  // Operator Overloadings
  EditorToolbox& operator=(const EditorToolbox& rhs) = delete;

  // Static Methods

  /**
   * @brief Remove (hide) a whole row in a QFormLayout
   *
   * @param label   The label of the row to remove.
   */
  static void removeFormLayoutRow(QLabel& label) noexcept;

  /**
   * @brief Delete a QLayout item with all its children
   *
   * @param item    The item to delete. Must not be nullptr!
   */
  static void deleteLayoutItemRecursively(QLayoutItem* item) noexcept;

  /**
   * @brief Set the focus to the first widget of a toolbar and iterate through
   *
   * - The tab order of all widgets of the passed toolbar will be configured
   *   from left to right resp. top to bottom.
   * - After the last widget, the tab order is followed by a custom widget.
   * - The first widget of the passed toolbar will get the focus.
   *
   * Intended for the command toolbar to enter focus from the graphics view,
   * navigate though all the toolbar widgets, and then return the focus
   * back to the graphics view.
   *
   * @param toolBar               The toolbar to set the focus.
   * @param returnFocusToWidget   Widget which shall have the focus after
   *                              the last widget of the toolbar.
   * @return  True if there was at least one widget and the focus has been
   *          set. False if there was no widget and the focus was not set.
   */
  static bool startToolBarTabFocusCycle(QToolBar& toolBar,
                                        QWidget& returnFocusToWidget) noexcept;

  /**
   * @brief Collect all relevant resources for a given component instance
   *
   * Resources will be collected from both the workspace library and the
   * project library.
   *
   * @param db          Workspace library database.
   * @param cmp         Component instance
   * @param filterDev   If provided, only get resources for these devices.
   *                    Otherwise, resources for all devices are returned.
   * @return List of all relevant resources.
   */
  static ResourceList getComponentResources(
      const WorkspaceLibraryDb& db, const ComponentInstance& cmp,
      const std::optional<Uuid>& filterDev = std::nullopt) noexcept;

  /**
   * @brief Add relevant resources of a component instance to a context menu
   *
   * This calls #getComponentResources() and then adds the results to a
   * context menu.
   *
   * @param ws          Workspace.
   * @param mb          Menu builder to add the new menu items.
   * @param cmp         See #getComponentResources().
   * @param filterDev   See #getComponentResources().
   * @param editor      Parent widget, used for asynchronously opening
   *                    resources when a menu item is triggered.
   * @param root        Root menu (for ownership).
   */
  static void addResourcesToMenu(const Workspace& ws, MenuBuilder& mb,
                                 const ComponentInstance& cmp,
                                 const std::optional<Uuid>& filterDev,
                                 QWidget& editor, QMenu& root) noexcept;

  /**
   * @brief Build a copyable text with all the version numbers etc.
   *
   * Contains application version, dependency versions, host architecture,
   * runtime environment etc.
   *
   * @return Text intended to be formatted as monospace
   */
  static QString buildAppVersionDetails() noexcept;

private:
  /**
   * @brief Helper for #removeFormLayoutRow(QLabel&)
   *
   * @param layout  The layout to look for the label.
   * @param label   The label to remove from the layout.
   *
   * @retval true on success.
   * @retval false if the label was not found in a form layout.
   */
  static bool removeFormLayoutRow(QLayout& layout, QLabel& label) noexcept;

  /**
   * @brief Helper for #removeFormLayoutRow()
   *
   * @param item  The item to hide.
   */
  static void hideLayoutItem(QLayoutItem& item) noexcept;

  /**
   * @brief Helper for #addResourcesToMenu()
   *
   * @param ws            Workspace.
   * @param mpn           Part MPN.
   * @param manufacturer  Part manufacturer name.
   * @param parent        Parent widget.
   */
  static void searchAndOpenDatasheet(const Workspace& ws, const QString& mpn,
                                     const QString& manufacturer,
                                     QPointer<QWidget> parent) noexcept;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
