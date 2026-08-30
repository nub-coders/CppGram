#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <optional>
#include <variant>
#include <memory>
#include "types.hpp"
#include "keyboard.hpp"

namespace cppgram {

// ============================================================================
// Telegram Bot API 10.3 Rich Text Entities
// ============================================================================

enum class RichTextStyle {
    Plain,
    Bold,
    Italic,
    Underline,
    Strikethrough,
    Spoiler,
    DateTime,
    TextMention,
    Subscript,
    Superscript,
    Marked,
    Code,
    CustomEmoji,
    MathematicalExpression,
    Url,
    EmailAddress,
    PhoneNumber,
    BankCardNumber,
    Mention,
    Hashtag,
    Cashtag,
    BotCommand,
    Button,         // Added in Bot API 10.3
    Anchor,
    AnchorLink,
    Reference,
    ReferenceLink
};

struct RichTextSpan {
    RichTextStyle style{RichTextStyle::Plain};
    std::string text;
    std::string argument; // URL, mention ID, date format, or language
};

/**
 * @brief Represents rich formatted text with granular span entities.
 */
struct RichText {
    std::string text;
    std::vector<RichTextSpan> spans;

    RichText() = default;
    RichText(std::string plain) : text(std::move(plain)) {}
    RichText(std::string text_val, std::vector<RichTextSpan> span_val)
        : text(std::move(text_val)), spans(std::move(span_val)) {}

    bool empty() const noexcept { return text.empty() && spans.empty(); }
};

// ============================================================================
// Bot API 10.3 Rich Message Buttons
// ============================================================================

/**
 * @brief Button embedded within rich message blocks or rich text spans (Bot API 10.3).
 */
struct RichMessageButton {
    std::string text;
    std::string style{"primary"}; // "danger", "success", "primary", "link"
    std::optional<std::string> url;
    std::optional<std::string> callback_data;
    std::optional<std::string> copy_text;
    bool disabled{false}; // Added in Bot API 10.3

    static RichMessageButton make_url(std::string text, std::string url, std::string style = "primary") {
        RichMessageButton b;
        b.text = std::move(text);
        b.url = std::move(url);
        b.style = std::move(style);
        return b;
    }

    static RichMessageButton make_callback(std::string text, std::string data, std::string style = "primary") {
        RichMessageButton b;
        b.text = std::move(text);
        b.callback_data = std::move(data);
        b.style = std::move(style);
        return b;
    }

    static RichMessageButton make_copy(std::string text, std::string copy_val, std::string style = "primary") {
        RichMessageButton b;
        b.text = std::move(text);
        b.copy_text = std::move(copy_val);
        b.style = std::move(style);
        return b;
    }
};

// ============================================================================
// Bot API 10.3 Rich Block Entities
// ============================================================================

enum class RichBlockType {
    Paragraph,
    SectionHeading,
    Preformatted,
    Footer,
    Divider,
    MathematicalExpression,
    Anchor,
    List,
    BlockQuotation,
    ExpandableBlockQuotation, // Bot API 10.3
    PullQuotation,
    Collage,
    Slideshow,
    Table,                    // Enhanced in Bot API 10.3 with is_compact
    Details,
    Map,
    Buttons,                  // Bot API 10.3
    Document,                 // Bot API 10.3 (tg://document?id=)
    Animation,
    Audio,
    Photo,
    Video,
    VoiceNote,
    Thinking                  // Bot API 10.1+ draft streaming
};

struct RichBlockTableCell {
    std::string text;
    bool is_header{false};
    int colspan{1};
    int rowspan{1};
    std::string align{"left"};  // "left", "center", "right"
    std::string valign{"top"};  // "top", "middle", "bottom"
};

struct RichBlockTable {
    std::vector<std::vector<RichBlockTableCell>> cells;
    bool is_bordered{true};
    bool is_striped{false};
    bool is_compact{false}; // Bot API 10.3
    std::optional<std::string> caption;
};

struct RichBlockExpandableBlockQuotation { // Bot API 10.3
    std::string text;
    std::optional<std::string> credit;
};

struct RichBlockButtons { // Bot API 10.3
    std::vector<RichMessageButton> buttons;
    std::string align{"left"}; // "left", "center", "right"
};

struct RichBlockDocument { // Bot API 10.3
    std::string document; // File ID, URL, or tg://document?id=...
    std::optional<std::string> caption;
};

struct RichBlockSectionHeading {
    std::string text;
};

struct RichBlockParagraph {
    std::string text;
};

struct RichBlockPreformatted {
    std::string text;
    std::string language;
};

struct RichBlockDivider {};

struct RichBlockThinking {
    std::string text;
};

using RichBlockPayload = std::variant<
    RichBlockParagraph,
    RichBlockSectionHeading,
    RichBlockPreformatted,
    RichBlockDivider,
    RichBlockTable,
    RichBlockExpandableBlockQuotation,
    RichBlockButtons,
    RichBlockDocument,
    RichBlockThinking
>;

struct RichBlock {
    RichBlockType type{RichBlockType::Paragraph};
    RichBlockPayload payload;

    RichBlock() : type(RichBlockType::Paragraph), payload(RichBlockParagraph{}) {}

    template<typename T>
    RichBlock(RichBlockType t, T&& p)
        : type(t), payload(std::forward<T>(p)) {}
};

// ============================================================================
// Input & Output Rich Message
// ============================================================================

struct InputRichMessageMedia {
    std::string id;
    std::string media;
};

/**
 * @brief Describes a rich message to be sent via Bot API 10.3.
 */
struct InputRichMessage {
    std::optional<std::string> html;
    std::optional<std::string> markdown;
    std::vector<RichBlock> blocks;
    std::vector<InputRichMessageMedia> media;
    bool is_rtl{false};
    bool skip_entity_detection{false};

    static InputRichMessage from_html(std::string html_content, bool is_rtl = false) {
        InputRichMessage msg;
        msg.html = std::move(html_content);
        msg.is_rtl = is_rtl;
        return msg;
    }

    static InputRichMessage from_markdown(std::string markdown_content, bool is_rtl = false) {
        InputRichMessage msg;
        msg.markdown = std::move(markdown_content);
        msg.is_rtl = is_rtl;
        return msg;
    }

    static InputRichMessage from_blocks(std::vector<RichBlock> blocks_list, bool is_rtl = false) {
        InputRichMessage msg;
        msg.blocks = std::move(blocks_list);
        msg.is_rtl = is_rtl;
        return msg;
    }
};

/**
 * @brief Received or processed rich formatted message.
 */
struct RichMessage {
    std::vector<RichBlock> blocks;
    bool is_rtl{false};
};

/**
 * @brief Transmission options for sendRichMessage.
 */
struct SendRichMessageOptions {
    std::optional<int64_t> message_thread_id;
    std::optional<int64_t> direct_messages_topic_id;
    bool disable_notification{false};
    bool protect_content{false};
    bool allow_paid_broadcast{false};
    std::optional<std::string> message_effect_id;
    std::optional<MessageId> reply_to;
    std::optional<int64_t> ephemeral_receiver_user_id; // Bot API 10.3 EphemeralMessageParameters
    bool replace_callback_query_message{false};        // Bot API 10.3
    std::optional<ReplyMarkup> reply_markup;
};

// ============================================================================
// Fluent RichMessageBuilder (Bot API 10.3)
// ============================================================================

class RichMessageBuilder {
public:
    RichMessageBuilder() = default;

    RichMessageBuilder& heading(std::string text);
    RichMessageBuilder& paragraph(std::string text);
    RichMessageBuilder& preformatted(std::string code, std::string language = "");
    RichMessageBuilder& divider();
    RichMessageBuilder& table(RichBlockTable table);
    RichMessageBuilder& expandable_quote(std::string text, std::optional<std::string> credit = std::nullopt);
    RichMessageBuilder& buttons(std::vector<RichMessageButton> btn_list, std::string align = "left");
    RichMessageBuilder& document(std::string doc_id_or_link, std::optional<std::string> caption = std::nullopt);
    RichMessageBuilder& thinking(std::string thought_process);
    RichMessageBuilder& set_rtl(bool rtl);
    RichMessageBuilder& set_skip_entity_detection(bool skip);
    RichMessageBuilder& add_media(std::string id, std::string media_target);

    InputRichMessage build() const;

private:
    InputRichMessage message_;
};

} // namespace cppgram
