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

#ifndef LIBREPCB_EDITOR_COMPONENTCATEGORYEDITORWIDGET_H
#define LIBREPCB_EDITOR_COMPONENTCATEGORYEDITORWIDGET_H

/*******************************************************************************
 *  Includes
 ******************************************************************************/
#include "../editorwidgetbase.h"

#include <librepcb/core/types/uuid.h>

#include <QtCore>
#include <QtWidgets>

#include <memory>
#include <optional>

/*******************************************************************************
 *  Namespace / Forward Declarations
 ******************************************************************************/
namespace librepcb {

class ComponentCategory;

namespace editor {

namespace Ui {
class ComponentCategoryEditorWidget;
}

/*******************************************************************************
 *  Class ComponentCategoryEditorWidget
 ******************************************************************************/

/**
 * @brief The ComponentCategoryEditorWidget class
 */
class ComponentCategoryEditorWidget final : public EditorWidgetBase {
  Q_OBJECT

public:
  // Constructors / Destructor
  ComponentCategoryEditorWidget() = delete;
  ComponentCategoryEditorWidget(const ComponentCategoryEditorWidget& other) =
      delete;
  ComponentCategoryEditorWidget(const Context& context, const FilePath& fp,
                                QWidget* parent = nullptr);
  ~ComponentCategoryEditorWidget() noexcept;

  // Getters
  QSet<Feature> getAvailableFeatures() const noexcept override;

  // Operator Overloadings
  ComponentCategoryEditorWidget& operator=(
      const ComponentCategoryEditorWidget& rhs) = delete;

public slots:
  bool save() noexcept override;

private:  // Methods
  void updateMetadata() noexcept;
  QString commitMetadata() noexcept;
  bool isInterfaceBroken() const noexcept override { return false; }
  bool runChecks(RuleCheckMessageList& msgs) const override;
  template <typename MessageType>
  void fixMsg(const MessageType& msg);
  template <typename MessageType>
  bool fixMsgHelper(std::shared_ptr<const RuleCheckMessage> msg, bool applyFix);
  bool processRuleCheckMessage(std::shared_ptr<const RuleCheckMessage> msg,
                               bool applyFix) override;
  void ruleCheckApproveRequested(std::shared_ptr<const RuleCheckMessage> msg,
                                 bool approve) noexcept override;
  void btnChooseParentCategoryClicked() noexcept;
  void btnResetParentCategoryClicked() noexcept;
  void updateCategoryLabel() noexcept;

private:  // Data
  QScopedPointer<Ui::ComponentCategoryEditorWidget> mUi;
  std::unique_ptr<ComponentCategory> mCategory;
  std::optional<Uuid> mParentUuid;
};

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace editor
}  // namespace librepcb

#endif
