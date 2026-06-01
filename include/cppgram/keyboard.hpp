#pragma once
#include <string>
#include <vector>
#include <variant>

namespace cppgram {

struct InlineKeyboardButton {
    std::string text;
    std::string url;
    std::string callback_data;
    std::string switch_inline_query;
    std::string switch_inline_query_current_chat;

    static InlineKeyboardButton callback(const std::string& label,
                                         const std::string& data) {
        InlineKeyboardButton b;
        b.text = label;
        b.callback_data = data;
        return b;
    }
    static InlineKeyboardButton link(const std::string& label,
                                     const std::string& target_url) {
        InlineKeyboardButton b;
        b.text = label;
        b.url = target_url;
        return b;
    }
    static InlineKeyboardButton switchInline(const std::string& label,
                                             const std::string& query,
                                             bool current_chat = false) {
        InlineKeyboardButton b;
        b.text = label;
        if (current_chat) b.switch_inline_query_current_chat = query;
        else              b.switch_inline_query = query;
        return b;
    }
};

using InlineKeyboardRow = std::vector<InlineKeyboardButton>;

struct InlineKeyboard {
    std::vector<InlineKeyboardRow> rows;

    InlineKeyboard& addRow(InlineKeyboardRow row) {
        rows.push_back(std::move(row));
        return *this;
    }
    InlineKeyboard& addButton(int row, InlineKeyboardButton btn) {
        while (static_cast<int>(rows.size()) <= row)
            rows.emplace_back();
        rows[row].push_back(std::move(btn));
        return *this;
    }
};

struct ReplyKeyboardButton {
    std::string text;
    bool request_contact{false};
    bool request_location{false};
};

struct ReplyKeyboard {
    std::vector<std::vector<ReplyKeyboardButton>> rows;
    bool resize_keyboard{true};
    bool one_time_keyboard{false};
    bool is_persistent{false};
    std::string input_field_placeholder;

    ReplyKeyboard& addRow(std::vector<ReplyKeyboardButton> row) {
        rows.push_back(std::move(row));
        return *this;
    }
};

struct RemoveKeyboard {
    bool selective{false};
};

struct ForceReply {
    bool selective{false};
    std::string input_field_placeholder;
};

using ReplyMarkup = std::variant<InlineKeyboard, ReplyKeyboard,
                                 RemoveKeyboard, ForceReply>;

} // namespace cppgram
