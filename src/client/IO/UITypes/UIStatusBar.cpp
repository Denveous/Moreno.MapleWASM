//////////////////////////////////////////////////////////////////////////////
// This file is part of the Journey MMORPG client                           //
// Copyright © 2015-2016 Daniel Allendorf                                   //
//                                                                          //
// This program is free software: you can redistribute it and/or modify     //
// it under the terms of the GNU Affero General Public License as           //
// published by the Free Software Foundation, either version 3 of the       //
// License, or (at your option) any later version.                          //
//                                                                          //
// This program is distributed in the hope that it will be useful,          //
// but WITHOUT ANY WARRANTY; without even the implied warranty of           //
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            //
// GNU Affero General Public License for more details.                      //
//                                                                          //
// You should have received a copy of the GNU Affero General Public License //
// along with this program.  If not, see <http://www.gnu.org/licenses/>.    //
//////////////////////////////////////////////////////////////////////////////
#include "UIStatusBar.h"

#include "../UI.h"
#include "../Components/MapleButton.h"

#include "../../Character/ExpTable.h"
#include "../../Constants.h"
#include "../../Graphics/Geometry.h"

#include "nlnx/nx.hpp"


namespace jrc
{
    constexpr Point<int16_t> UIStatusbar::POSITION;
    constexpr Point<int16_t> UIStatusbar::DIMENSION;

    UIStatusbar::UIStatusbar(const CharStats& st)
        : UIElement(POSITION, DIMENSION), stats(st), chatbar(POSITION)
    {
        nl::node statusbar = nl::nx::ui["StatusBar.img"];
        nl::node mainbar = statusbar["base"];

        sprites.emplace_back(mainbar["backgrnd"]);

        expbar = {
            statusbar.resolve("gauge/bar"),
            statusbar.resolve("gauge/bar"),
            statusbar.resolve("gauge/bar"),
            308, 0.0f
        };
        hpbar = {
            statusbar.resolve("gauge/hpFlash/0"),
            statusbar.resolve("gauge/hpFlash/1"),
            statusbar.resolve("gauge/hpFlash/0"),
            137, 0.0f
        };
        mpbar = {
            statusbar.resolve("gauge/mpFlash/0"),
            statusbar.resolve("gauge/mpFlash/1"),
            statusbar.resolve("gauge/mpFlash/0"),
            137, 0.0f
        };

        statset   = { nl::nx::ui["Basic.img"]["Number"], Charset::RIGHT };
        levelset  = { nl::nx::ui["Basic.img"]["Number"], Charset::LEFT  };

        joblabel  = { Text::A11M, Text::LEFT, Text::YELLOW };
        namelabel = { Text::A13M, Text::LEFT, Text::WHITE  };

        buttons[BT_WHISPER]   = std::make_unique<MapleButton>(statusbar["BtWhisper"], 518, 45);
        buttons[BT_CALLGM]    = std::make_unique<MapleButton>(statusbar["BtClaim"], 533, 45);

        buttons[BT_CASHSHOP]  = std::make_unique<MapleButton>(statusbar["BtShop"], 574, 28);
        buttons[BT_TRADE]     = std::make_unique<MapleButton>(statusbar["BtNPT"], 632, 28);
        buttons[BT_MENU]      = std::make_unique<MapleButton>(statusbar["BtMenu"], 690, 28);
        buttons[BT_OPTIONS]   = std::make_unique<MapleButton>(statusbar["BtShort"], 748, 28);

        buttons[BT_CHARACTER] = std::make_unique<MapleButton>(statusbar["StatKey"], 574, 6);
        buttons[BT_STATS]     = std::make_unique<MapleButton>(statusbar["StatKey"], 574, 6);
        buttons[BT_QUEST]     = std::make_unique<MapleButton>(statusbar["KeySet"], 604, 6);
        buttons[BT_INVENTORY] = std::make_unique<MapleButton>(statusbar["InvenKey"], 634, 6);
        buttons[BT_EQUIPS]    = std::make_unique<MapleButton>(statusbar["EquipKey"], 664, 6);
        buttons[BT_SKILL]     = std::make_unique<MapleButton>(statusbar["SkillKey"], 694, 6);

        update_layout_position();
        active = true;
    }

    void UIStatusbar::draw(float alpha) const
    {
        UIElement::draw(alpha);

        int16_t level = stats.get_stat(Maplestat::LEVEL);
        int16_t hp    = stats.get_stat(Maplestat::HP);
        int16_t mp    = stats.get_stat(Maplestat::MP);
        int32_t maxhp = stats.get_total(Equipstat::HP);
        int32_t maxmp = stats.get_total(Equipstat::MP);
        int64_t exp   = stats.get_exp();

        ColorBox hpfill(static_cast<int16_t>(109 * gethppercent()), 8, Geometry::HPBAR_DARKRED, 0.9f);
        ColorBox mpfill(static_cast<int16_t>(109 * getmppercent()), 8, Geometry::HPBAR_GREEN, 0.9f);
        ColorBox expfill(static_cast<int16_t>(340 * getexppercent()), 6, Geometry::HPBAR_LIGHTGREEN, 0.9f);
        hpfill.draw(position + Point<int16_t>(205, 42));
        mpfill.draw(position + Point<int16_t>(376, 42));
        expfill.draw(position + Point<int16_t>(205, 58));

        std::string expstring = std::to_string(100 * getexppercent());
        statset.draw(
            std::to_string(exp) + "[" + expstring.substr(0, expstring.find('.') + 3) + "%]",
            position + Point<int16_t>(512, 47)
        );
        statset.draw(
            "[" + std::to_string(hp) + "/" + std::to_string(maxhp) + "]",
            position + Point<int16_t>(341, 27)
        );
        statset.draw(
            "[" + std::to_string(mp) + "/" + std::to_string(maxmp) + "]",
            position + Point<int16_t>(512, 27)
        );
        levelset.draw(
            std::to_string(level),
            position + Point<int16_t>(32, 32)
        );

        joblabel.draw(position + Point<int16_t>(78, 44));
        namelabel.draw(position + Point<int16_t>(78, 29));

        chatbar.draw(alpha);
    }

    void UIStatusbar::update()
    {
        update_layout_position();
        UIElement::update();

        chatbar.update();

        expbar.update(getexppercent());
        hpbar.update(gethppercent());
        mpbar.update(getmppercent());

        namelabel.change_text(stats.get_name());
        joblabel.change_text(stats.get_jobname());

        for (auto iter : message_cooldowns)
        {
            iter.second -= Constants::TIMESTEP;
        }
    }

    void UIStatusbar::update_layout_position()
    {
        position = {
            POSITION.x(),
            static_cast<int16_t>(Constants::viewheight() - DIMENSION.y())
        };
        chatbar.set_position(position);
    }

    Button::State UIStatusbar::button_pressed(uint16_t id)
    {
        switch (id)
        {
        case BT_STATS:
            UI::get().send_menu(KeyAction::CHARSTATS);
            return Button::NORMAL;
        case BT_INVENTORY:
            UI::get().send_menu(KeyAction::INVENTORY);
            return Button::NORMAL;
        case BT_EQUIPS:
            UI::get().send_menu(KeyAction::EQUIPS);
            return Button::NORMAL;
        case BT_SKILL:
            UI::get().send_menu(KeyAction::SKILLBOOK);
            return Button::NORMAL;
        default:
            return Button::PRESSED;
        }
    }

    bool UIStatusbar::is_in_range(Point<int16_t> cursorpos) const
    {
        Rectangle<int16_t> bounds(
            position,
            position + dimension
        );

        return bounds.contains(cursorpos) || chatbar.is_in_range(cursorpos);
    }

    bool UIStatusbar::remove_cursor(bool clicked, Point<int16_t> cursorpos)
    {
        if (chatbar.remove_cursor(clicked, cursorpos))
        {
            return true;
        }

        return UIElement::remove_cursor(clicked, cursorpos);
    }

    UIElement::CursorResult UIStatusbar::send_cursor(bool pressed, Point<int16_t> cursorpos)
    {
        if (chatbar.is_in_range(cursorpos))
        {
            if (CursorResult child_result = chatbar.send_cursor(pressed, cursorpos))
            {
                return child_result;
            }

            return UIElement::send_cursor(pressed, cursorpos);
        }

        chatbar.send_cursor(pressed, cursorpos);
        return UIElement::send_cursor(pressed, cursorpos);
    }

    void UIStatusbar::send_chatline(const std::string& line, UIChatbar::LineType type)
    {
        chatbar.send_line(line, type);
    }

    void UIStatusbar::focus_chatfield()
    {
        chatbar.focus_chatfield();
    }

    void UIStatusbar::set_chat_target(UIChatbar::ChatTarget target)
    {
        chatbar.set_chat_target(target);
    }

    void UIStatusbar::cycle_chat_target()
    {
        chatbar.cycle_chat_target();
    }

    void UIStatusbar::set_pending_party_invite(int32_t party_id, const std::string& inviter)
    {
        chatbar.set_pending_party_invite(party_id, inviter);
    }

    void UIStatusbar::clear_pending_party_invite()
    {
        chatbar.clear_pending_party_invite();
    }

    void UIStatusbar::set_party_state(int32_t party_id, int32_t leader_id, const std::vector<UIChatbar::PartyMember>& members)
    {
        chatbar.set_party_state(party_id, leader_id, members);
    }

    void UIStatusbar::clear_party_state()
    {
        chatbar.clear_party_state();
    }

    void UIStatusbar::set_party_leader(int32_t leader_id)
    {
        chatbar.set_party_leader(leader_id);
    }

    void UIStatusbar::update_party_member_hp(int32_t cid, int32_t hp, int32_t max_hp)
    {
        chatbar.update_party_member_hp(cid, hp, max_hp);
    }

    int32_t UIStatusbar::get_party_id() const
    {
        return chatbar.get_party_id();
    }

    int32_t UIStatusbar::get_party_leader_id() const
    {
        return chatbar.get_party_leader_id();
    }

    int32_t UIStatusbar::get_pending_party_invite_id() const
    {
        return chatbar.get_pending_party_invite_id();
    }

    const std::string& UIStatusbar::get_pending_party_inviter() const
    {
        return chatbar.get_pending_party_inviter();
    }

    const std::vector<UIChatbar::PartyMember>& UIStatusbar::get_party_members() const
    {
        return chatbar.get_party_members();
    }

    void UIStatusbar::display_message(Messages::Type line, UIChatbar::LineType type)
    {
        if (message_cooldowns[line] > 0)
        {
            return;
        }

        std::string message{ Messages::messages[line] };
        chatbar.send_line(message, type);

        message_cooldowns[line] = MESSAGE_COOLDOWN;
    }

    float UIStatusbar::getexppercent() const
    {
        int16_t level = stats.get_stat(Maplestat::LEVEL);
        if (level >= ExpTable::LEVELCAP)
        {
            return 0.0f;
        }

        int64_t exp = stats.get_exp();
        return static_cast<float>(
            static_cast<double>(exp) / ExpTable::values[level]
        );
    }

    float UIStatusbar::gethppercent() const
    {
        int16_t hp = stats.get_stat(Maplestat::HP);
        int32_t maxhp = stats.get_total(Equipstat::HP);

        return static_cast<float>(hp) / maxhp;
    }

    float UIStatusbar::getmppercent() const
    {
        int16_t mp = stats.get_stat(Maplestat::MP);
        int32_t maxmp = stats.get_total(Equipstat::MP);

        return static_cast<float>(mp) / maxmp;
    }
}
