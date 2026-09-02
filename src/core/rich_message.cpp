#include "cppgram/rich_message.hpp"
#include <sstream>

namespace cppgram {

RichMessageBuilder& RichMessageBuilder::heading(std::string text) {
    message_.blocks.emplace_back(RichBlockType::SectionHeading, RichBlockSectionHeading{std::move(text)});
    return *this;
}

RichMessageBuilder& RichMessageBuilder::paragraph(std::string text) {
    message_.blocks.emplace_back(RichBlockType::Paragraph, RichBlockParagraph{std::move(text)});
    return *this;
}

RichMessageBuilder& RichMessageBuilder::preformatted(std::string code, std::string language) {
    message_.blocks.emplace_back(RichBlockType::Preformatted, RichBlockPreformatted{std::move(code), std::move(language)});
    return *this;
}

RichMessageBuilder& RichMessageBuilder::divider() {
    message_.blocks.emplace_back(RichBlockType::Divider, RichBlockDivider{});
    return *this;
}

RichMessageBuilder& RichMessageBuilder::table(RichBlockTable table) {
    message_.blocks.emplace_back(RichBlockType::Table, std::move(table));
    return *this;
}

RichMessageBuilder& RichMessageBuilder::expandable_quote(std::string text, std::optional<std::string> credit) {
    message_.blocks.emplace_back(RichBlockType::ExpandableBlockQuotation,
                                 RichBlockExpandableBlockQuotation{std::move(text), std::move(credit)});
    return *this;
}

RichMessageBuilder& RichMessageBuilder::buttons(std::vector<RichMessageButton> btn_list, std::string align) {
    message_.blocks.emplace_back(RichBlockType::Buttons,
                                 RichBlockButtons{std::move(btn_list), std::move(align)});
    return *this;
}

RichMessageBuilder& RichMessageBuilder::document(std::string doc_id_or_link, std::optional<std::string> caption) {
    message_.blocks.emplace_back(RichBlockType::Document,
                                 RichBlockDocument{std::move(doc_id_or_link), std::move(caption)});
    return *this;
}

RichMessageBuilder& RichMessageBuilder::thinking(std::string thought_process) {
    message_.blocks.emplace_back(RichBlockType::Thinking, RichBlockThinking{std::move(thought_process)});
    return *this;
}

RichMessageBuilder& RichMessageBuilder::set_rtl(bool rtl) {
    message_.is_rtl = rtl;
    return *this;
}

RichMessageBuilder& RichMessageBuilder::set_skip_entity_detection(bool skip) {
    message_.skip_entity_detection = skip;
    return *this;
}

RichMessageBuilder& RichMessageBuilder::add_media(std::string id, std::string media_target) {
    message_.media.push_back(InputRichMessageMedia{std::move(id), std::move(media_target)});
    return *this;
}

InputRichMessage RichMessageBuilder::build() const {
    return message_;
}

namespace {
std::string escape_json_string(const std::string& s) {
    std::string out;
    out.reserve(s.size() + 16);
    for (char c : s) {
        switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\b': out += "\\b"; break;
            case '\f': out += "\\f"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    char buf[8];
                    std::snprintf(buf, sizeof(buf), "\\u%04x", c);
                    out += buf;
                } else {
                    out += c;
                }
                break;
        }
    }
    return out;
}
} // namespace

std::string serialize_rich_message_json(const InputRichMessage& msg) {
    std::string out = "{\"blocks\":[";
    bool first_block = true;
    for (const auto& block : msg.blocks) {
        std::string block_json;
        switch (block.type) {
            case RichBlockType::SectionHeading:
                if (auto* p = std::get_if<RichBlockSectionHeading>(&block.payload)) {
                    block_json = "{\"type\":\"heading\",\"text\":\"" + escape_json_string(p->text) + "\",\"size\":2}";
                }
                break;
            case RichBlockType::Paragraph:
                if (auto* p = std::get_if<RichBlockParagraph>(&block.payload)) {
                    block_json = "{\"type\":\"paragraph\",\"text\":\"" + escape_json_string(p->text) + "\"}";
                }
                break;
            case RichBlockType::Preformatted:
                if (auto* p = std::get_if<RichBlockPreformatted>(&block.payload)) {
                    block_json = "{\"type\":\"pre\",\"text\":\"" + escape_json_string(p->text) + "\"";
                    if (!p->language.empty()) {
                        block_json += ",\"language\":\"" + escape_json_string(p->language) + "\"";
                    }
                    block_json += "}";
                }
                break;
            case RichBlockType::Divider:
                block_json = "{\"type\":\"divider\"}";
                break;
            case RichBlockType::Table:
                if (auto* p = std::get_if<RichBlockTable>(&block.payload)) {
                    block_json = "{\"type\":\"table\"";
                    if (p->is_bordered) block_json += ",\"is_bordered\":true";
                    if (p->is_compact) block_json += ",\"is_compact\":true";
                    if (p->is_striped) block_json += ",\"is_striped\":true";
                    block_json += ",\"cells\":[";
                    for (size_t r = 0; r < p->cells.size(); ++r) {
                        if (r > 0) block_json += ",";
                        block_json += "[";
                        for (size_t c = 0; c < p->cells[r].size(); ++c) {
                            if (c > 0) block_json += ",";
                            const auto& cell = p->cells[r][c];
                            block_json += "{";
                            if (cell.button.has_value()) {
                                block_json += "\"text\":{\"type\":\"button\",\"button\":{\"text\":\"" +
                                    escape_json_string(cell.button->text) + "\"";
                                if (cell.button->callback_data.has_value()) {
                                    block_json += ",\"callback_data\":\"" + escape_json_string(*cell.button->callback_data) + "\"";
                                } else if (cell.button->url.has_value()) {
                                    block_json += ",\"url\":\"" + escape_json_string(*cell.button->url) + "\"";
                                }
                                block_json += "}}";
                            } else if (cell.custom_emoji_id.has_value()) {
                                block_json += "\"text\":{\"type\":\"custom_emoji\",\"custom_emoji_id\":\"" +
                                    escape_json_string(*cell.custom_emoji_id) + "\",\"alternative_text\":\"" +
                                    escape_json_string(cell.text) + "\"}";
                            } else if (cell.is_bold) {
                                block_json += "\"text\":{\"type\":\"bold\",\"text\":\"" +
                                    escape_json_string(cell.text) + "\"}";
                            } else if (cell.is_code) {
                                block_json += "\"text\":{\"type\":\"code\",\"text\":\"" +
                                    escape_json_string(cell.text) + "\"}";
                            } else {
                                block_json += "\"text\":\"" + escape_json_string(cell.text) + "\"";
                            }
                            if (cell.is_header) block_json += ",\"is_header\":true";
                            if (!cell.align.empty()) block_json += ",\"align\":\"" + cell.align + "\"";
                            if (!cell.valign.empty()) block_json += ",\"valign\":\"" + cell.valign + "\"";
                            block_json += "}";
                        }
                        block_json += "]";
                    }
                    block_json += "]}";
                }
                break;
            case RichBlockType::Buttons:
                if (auto* p = std::get_if<RichBlockButtons>(&block.payload)) {
                    block_json = "{\"type\":\"buttons\",\"align\":\"" + p->align + "\",\"buttons\":[";
                    for (size_t b = 0; b < p->buttons.size(); ++b) {
                        if (b > 0) block_json += ",";
                        const auto& btn = p->buttons[b];
                        block_json += "{\"text\":\"" + escape_json_string(btn.text) + "\"";
                        if (btn.callback_data.has_value()) {
                            block_json += ",\"callback_data\":\"" + escape_json_string(*btn.callback_data) + "\"";
                        } else if (btn.url.has_value()) {
                            block_json += ",\"url\":\"" + escape_json_string(*btn.url) + "\"";
                        }
                        block_json += "}";
                    }
                    block_json += "]}";
                }
                break;
            default:
                break;
        }
        if (!block_json.empty()) {
            if (!first_block) out += ",";
            out += block_json;
            first_block = false;
        }
    }
    out += "]}";
    return out;
}

} // namespace cppgram
