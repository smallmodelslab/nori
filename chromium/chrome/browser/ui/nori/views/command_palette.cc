// Copyright 2026 The Nori Authors
// You can use, redistribute, and/or modify this source code under
// the terms of the GPL-3.0 license that can be found in the LICENSE file.

#include "chrome/browser/ui/nori/views/command_palette.h"

#include <algorithm>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "base/functional/bind.h"
#include "base/i18n/case_conversion.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/autocomplete/autocomplete_classifier_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser_commands.h"
#include "chrome/browser/ui/browser_shortcuts/browser_shortcut_service.h"
#include "chrome/browser/ui/browser_shortcuts/browser_shortcut_service_factory.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/navigator/browser_navigator.h"
#include "chrome/browser/ui/navigator/browser_navigator_params.h"
#include "chrome/browser/ui/nori/commands/commands.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/browser/ui/toolbar/app_menu_model.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "components/omnibox/browser/autocomplete_classifier.h"
#include "components/omnibox/browser/autocomplete_match.h"
#include "components/tabs/public/tab_interface.h"
#include "content/public/browser/web_contents.h"
#include "ui/accessibility/ax_enums.mojom.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/base/models/menu_model.h"
#include "ui/color/color_id.h"
#include "ui/events/event.h"
#include "ui/gfx/font_list.h"
#include "ui/views/accessibility/view_accessibility.h"
#include "ui/views/background.h"
#include "ui/views/border.h"
#include "ui/views/bubble/bubble_dialog_delegate_view.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/controls/label.h"
#include "ui/views/controls/scroll_view.h"
#include "ui/views/controls/textfield/textfield.h"
#include "ui/views/controls/textfield/textfield_controller.h"
#include "ui/views/layout/box_layout.h"
#include "ui/views/widget/widget.h"

namespace nori {
namespace {

// Use the same native command and menu models as the browser, including
// commands without a shortcut. Never maintain a parallel list of actions.
struct Entry {
  int id;
  std::u16string name;
  std::u16string shortcut;
  std::u16string keywords;
  bool tab = false;
  raw_ptr<ui::MenuModel> menu = nullptr;
  size_t menu_index = 0;
};

bool IsObsoleteLayoutCommand(int id) {
  return id == IDC_BROWSER_LAYOUT_CLASSIC || id == IDC_BROWSER_LAYOUT_COMPACT ||
         id == IDC_BROWSER_LAYOUT_VERTICAL || id == IDC_TOGGLE_VERTICAL_TABS ||
         id == IDC_TOGGLE_VERTICAL_TABS_COLLAPSE ||
         id == IDC_BROWSER_LAYOUT_VERTICAL_RIGHT ||
         id == IDC_BROWSER_LAYOUT_DYNAMIC ||
         id == IDC_TOGGLE_ZEN_MODE_TOP_CHROME_PIN;
}

class CommandRow : public views::Button {
  METADATA_HEADER(CommandRow, views::Button)
 public:
  CommandRow(PressedCallback callback,
             const std::u16string& name,
             const std::u16string& shortcut)
      : Button(std::move(callback)) {
    auto* layout = SetLayoutManager(std::make_unique<views::BoxLayout>(
        views::BoxLayout::Orientation::kHorizontal, gfx::Insets::VH(10, 12),
        20));
    auto* title = AddChildView(std::make_unique<views::Label>(name));
    title->SetHorizontalAlignment(gfx::ALIGN_LEFT);
    layout->SetFlexForView(title, 1);
    auto* key = AddChildView(std::make_unique<views::Label>(shortcut));
    key->SetEnabledColor(ui::kColorLabelForegroundSecondary);
    SetFocusBehavior(FocusBehavior::NEVER);
    GetViewAccessibility().SetRole(ax::mojom::Role::kListBoxOption);
    GetViewAccessibility().SetName(name);
  }

  void Select(bool selected) {
    selected_ = selected;
    GetViewAccessibility().SetIsSelected(selected);
    UpdateBackground();
  }

 private:
  void StateChanged(ButtonState old_state) override {
    Button::StateChanged(old_state);
    UpdateBackground();
  }

  void UpdateBackground() {
    SetBackground(selected_ || GetState() == STATE_HOVERED ||
                          GetState() == STATE_PRESSED
                      ? views::CreateRoundedRectBackground(
                            ui::kColorMenuItemBackgroundSelected, 8)
                      : nullptr);
  }

  bool selected_ = false;
};

BEGIN_METADATA(CommandRow)
END_METADATA

class CommandPalette : public views::View, public views::TextfieldController {
  METADATA_HEADER(CommandPalette, views::View)
 public:
  explicit CommandPalette(BrowserWindowInterface* browser) : browser_(browser) {
    auto* layout = SetLayoutManager(std::make_unique<views::BoxLayout>(
        views::BoxLayout::Orientation::kVertical, gfx::Insets(12), 8));
    layout->set_cross_axis_alignment(
        views::BoxLayout::CrossAxisAlignment::kStretch);
    input_ = AddChildView(std::make_unique<views::Textfield>());
    input_->SetPlaceholderText(u"Search tabs, commands, or the web");
    input_->GetViewAccessibility().SetName(u"Nori command palette");
    input_->GetViewAccessibility().SetRole(
        ax::mojom::Role::kTextFieldWithComboBox);
    input_->GetViewAccessibility().SetIsExpanded();
    input_->SetFontList(gfx::FontList().DeriveWithSizeDelta(7));
    input_->SetBorder(views::CreateEmptyBorder(gfx::Insets::VH(12, 12)));
    input_->set_controller(this);

    auto* scroll = AddChildView(std::make_unique<views::ScrollView>());
    scroll->ClipHeightTo(0, 344);
    scroll->SetHorizontalScrollBarMode(
        views::ScrollView::ScrollBarMode::kDisabled);
    results_ = scroll->SetContents(std::make_unique<views::View>());
    results_->GetViewAccessibility().SetRole(ax::mojom::Role::kListBox);
    results_->GetViewAccessibility().SetName(u"Results");
    auto* rows = results_->SetLayoutManager(std::make_unique<views::BoxLayout>(
        views::BoxLayout::Orientation::kVertical));
    rows->set_cross_axis_alignment(
        views::BoxLayout::CrossAxisAlignment::kStretch);

    LoadEntries();
    Filter();
  }

  views::View* input() { return input_; }

 private:
  void LoadEntries() {
    const auto* service =
        browser_shortcuts::BrowserShortcutServiceFactory::GetForProfile(
            browser_->GetProfile());
    std::set<int> added;
    if (service) {
      // Put Nori's own commands first, with their actual configured shortcuts.
      for (const Command& command : Commands()) {
        for (const auto& item : service->GetCommands()) {
          if (item.command_id == command.id)
            AddCommand(item, added);
        }
      }
      for (const auto& item : service->GetCommands())
        AddCommand(item, added);
    }
    menu_ = std::make_unique<AppMenuModel>(
        BrowserView::GetBrowserViewForBrowser(browser_), browser_);
    menu_->Init();
    AddMenu(*menu_, added);

    TabStripModel* tabs = browser_->GetTabStripModel();
    for (int i = 0; i < tabs->count(); ++i) {
      auto* contents = tabs->GetWebContentsAt(i);
      std::u16string title = contents->GetTitle();
      if (title.empty())
        title = u"New tab";
      entries_.push_back(
          {tabs->GetTabAtIndex(i)->GetHandle().raw_value(), title, u"Tab",
           u"tab " + base::UTF8ToUTF16(contents->GetVisibleURL().spec()),
           true});
    }
  }

  void AddCommand(const browser_shortcuts::CommandInfo& command,
                  std::set<int>& added) {
    if (command.command_id == IDC_NORI_POPUP ||
        IsObsoleteLayoutCommand(command.command_id) ||
        !chrome::SupportsCommand(browser_, command.command_id) ||
        !chrome::IsCommandEnabled(browser_, command.command_id) ||
        !added.insert(command.command_id).second)
      return;
    std::u16string shortcut;
    for (const auto& key : command.accelerators) {
      if (!shortcut.empty())
        shortcut += u"  ";
      shortcut += key.display_text;
    }
    entries_.push_back(
        {command.command_id, command.name, shortcut, u"command"});
  }

  void AddMenu(ui::MenuModel& menu, std::set<int>& added) {
    for (size_t i = 0; i < menu.GetItemCount(); ++i) {
      if (!menu.IsVisibleAt(i))
        continue;
      if (auto* submenu = menu.GetSubmenuModelAt(i)) {
        AddMenu(*submenu, added);
        continue;
      }
      if (menu.GetTypeAt(i) == ui::MenuModel::TYPE_SEPARATOR ||
          !menu.IsEnabledAt(i))
        continue;
      const int id = menu.GetCommandIdAt(i);
      if (IsObsoleteLayoutCommand(id) || !added.insert(id).second)
        continue;
      std::u16string name;
      base::RemoveChars(menu.GetLabelAt(i), u"&", &name);
      if (name.empty())
        continue;
      entries_.push_back({id, name, u"", u"command", false, &menu, i});
    }
  }

  void ContentsChanged(views::Textfield*, const std::u16string&) override {
    Filter();
  }

  bool HandleKeyEvent(views::Textfield*, const ui::KeyEvent& event) override {
    if (event.type() != ui::EventType::kKeyPressed || input_->IsIMEComposing())
      return false;
    if (event.key_code() == ui::VKEY_ESCAPE ||
        ((event.IsControlDown() || event.IsCommandDown()) &&
         event.key_code() == ui::VKEY_B)) {
      GetWidget()->Close();
      return true;
    }
    if (event.key_code() == ui::VKEY_RETURN) {
      Run(selected_);
      return true;
    }
    if (event.key_code() == ui::VKEY_UP || event.key_code() == ui::VKEY_DOWN) {
      if (!rows_.empty()) {
        selected_ = (selected_ + rows_.size() +
                     (event.key_code() == ui::VKEY_DOWN ? 1 : -1)) %
                    rows_.size();
        Select();
      }
      return true;
    }
    return false;
  }

  void Filter() {
    input_->GetViewAccessibility().ClearActiveDescendant();
    rows_.clear();
    results_->RemoveAllChildViews();
    matches_.clear();
    const std::u16string query = base::i18n::ToLower(input_->GetText());
    // Match words independently, so "split right" and "right split" agree.
    std::vector<std::u16string> words;
    size_t start = 0;
    while (start < query.size()) {
      const size_t end = query.find(u' ', start);
      if (end != start)
        words.push_back(query.substr(start, end - start));
      if (end == std::u16string::npos)
        break;
      start = end + 1;
    }
    for (size_t i = 0; i < entries_.size(); ++i) {
      const auto& entry = entries_[i];
      const auto text = base::i18n::ToLower(entry.name + u" " + entry.keywords);
      if (std::ranges::all_of(words, [&](const auto& word) {
            return text.find(word) != std::u16string::npos;
          }))
        matches_.push_back(i);
    }
    for (size_t i = 0; i < matches_.size(); ++i) {
      const auto& entry = entries_[matches_[i]];
      AddRow(i, entry.name, entry.shortcut);
    }
    if (!query.empty()) {
      AddRow(matches_.size(),
             u"Search or open “" + std::u16string(input_->GetText()) + u"”",
             u"↵");
    }
    selected_ = 0;
    Select();
    PreferredSizeChanged();
    if (GetWidget()) {
      if (auto* bubble =
              GetWidget()->widget_delegate()->AsBubbleDialogDelegate())
        bubble->SizeToContents();
    }
  }

  void AddRow(size_t index,
              const std::u16string& name,
              const std::u16string& shortcut) {
    rows_.push_back(results_->AddChildView(std::make_unique<CommandRow>(
        base::BindRepeating(&CommandPalette::Run, base::Unretained(this),
                            index),
        name, shortcut)));
  }

  void Select() {
    for (size_t i = 0; i < rows_.size(); ++i)
      rows_[i]->Select(i == selected_);
    if (!rows_.empty()) {
      rows_[selected_]->ScrollViewToVisible();
      input_->GetViewAccessibility().SetActiveDescendant(*rows_[selected_]);
    }
  }

  void Run(size_t index) {
    if (index >= rows_.size())
      return;
    // Closing is asynchronous; copy the action before a command can close a
    // tab, window, or this palette itself.
    BrowserWindowInterface* browser = browser_;
    const std::u16string query(input_->GetText());
    const bool navigate = index == matches_.size();
    const Entry entry = navigate ? Entry{} : entries_[matches_[index]];
    // Restore focus before dispatching. Closing a still-visible bubble later
    // can otherwise return focus to the tab the action just switched away from.
    GetWidget()->Hide();
    GetWidget()->Close();
    if (navigate) {
      AutocompleteMatch match;
      AutocompleteClassifierFactory::GetForProfile(browser->GetProfile())
          ->Classify(query, false, false, metrics::OmniboxEventProto::OTHER,
                     &match, nullptr);
      if (match.destination_url.is_valid()) {
        NavigateParams params(browser, match.destination_url, match.transition);
        params.disposition = WindowOpenDisposition::CURRENT_TAB;
        Navigate(&params);
      }
      return;
    }
    if (entry.tab) {
      TabStripModel* tabs = browser->GetTabStripModel();
      for (int i = 0; i < tabs->count(); ++i) {
        if (tabs->GetTabAtIndex(i)->GetHandle().raw_value() == entry.id) {
          tabs->ActivateTabAt(i);
          tabs->GetWebContentsAt(i)->Focus();
          break;
        }
      }
    } else if (entry.menu) {
      entry.menu->ActivatedAt(entry.menu_index);
    } else if (chrome::IsCommandEnabled(browser, entry.id)) {
      chrome::ExecuteCommand(browser, entry.id);
    }
  }

  const raw_ptr<BrowserWindowInterface> browser_;
  raw_ptr<views::Textfield> input_ = nullptr;
  raw_ptr<views::View> results_ = nullptr;
  std::unique_ptr<AppMenuModel> menu_;
  std::vector<Entry> entries_;
  std::vector<size_t> matches_;
  std::vector<raw_ptr<CommandRow>> rows_;
  size_t selected_ = 0;
};

BEGIN_METADATA(CommandPalette)
END_METADATA

class PaletteBubble : public views::BubbleDialogDelegate {
 public:
  explicit PaletteBubble(BrowserView* view)
      : BubbleDialogDelegate(view, views::BubbleBorder::NONE), view_(view) {}

  gfx::Rect GetBubbleBounds() override {
    const gfx::Rect area = view_->GetBoundsInScreen();
    const gfx::Size size = BubbleDialogDelegate::GetBubbleBounds().size();
    return gfx::Rect(area.x() + (area.width() - size.width()) / 2,
                     area.y() + std::max(24, area.height() / 5), size.width(),
                     std::min(size.height(), area.height() - 48));
  }

 private:
  const raw_ptr<BrowserView> view_;
};

}  // namespace

base::WeakPtr<views::Widget> ShowCommandPalette(
    BrowserWindowInterface* browser) {
  BrowserView* view = BrowserView::GetBrowserViewForBrowser(browser);
  auto delegate = std::make_unique<PaletteBubble>(view);
  delegate->SetTitle(u"Nori");
  delegate->SetAccessibleTitle(u"Nori command palette");
  delegate->SetShowTitle(false);
  delegate->SetShowCloseButton(false);
  delegate->SetButtons(0);
  delegate->set_margins(gfx::Insets());
  delegate->set_corner_radius(16);
  delegate->set_close_on_deactivate(true);
  delegate->set_fixed_width(std::min(660, std::max(240, view->width() - 48)));
  auto* palette =
      delegate->SetContentsView(std::make_unique<CommandPalette>(browser));
  delegate->SetInitiallyFocusedView(palette->input());
  auto* widget = views::BubbleDialogDelegate::CreateBubbleDeprecated(
      std::move(delegate),
      views::Widget::InitParams::NATIVE_WIDGET_OWNS_WIDGET);
  widget->Show();
  return widget->GetWeakPtr();
}

}  // namespace nori
