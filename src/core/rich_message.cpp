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

} // namespace cppgram
