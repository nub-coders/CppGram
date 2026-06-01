#pragma once
#include <string>
#include <vector>
#include <optional>
#include "cppgram/types.hpp"

namespace cppgram {

struct PhotoSize {
    FileId file_id{};
    std::string file_unique_id;
    int width{};
    int height{};
    std::int64_t file_size{};
};

struct Photo {
    std::vector<PhotoSize> sizes;

    bool empty() const noexcept { return sizes.empty(); }

    const PhotoSize& largest() const {
        const PhotoSize* best = &sizes[0];
        for (auto& s : sizes)
            if (s.width * s.height > best->width * best->height) best = &s;
        return *best;
    }

    const PhotoSize& smallest() const {
        const PhotoSize* best = &sizes[0];
        for (auto& s : sizes)
            if (s.width * s.height < best->width * best->height) best = &s;
        return *best;
    }
};

struct VideoInfo {
    FileId file_id{};
    std::string file_unique_id;
    int width{};
    int height{};
    int duration{};
    std::string mime_type;
    std::string file_name;
    std::int64_t file_size{};
    std::optional<PhotoSize> thumbnail;
};

struct DocumentInfo {
    FileId file_id{};
    std::string file_unique_id;
    std::string file_name;
    std::string mime_type;
    std::int64_t file_size{};
    std::optional<PhotoSize> thumbnail;
};

struct AudioInfo {
    FileId file_id{};
    std::string file_unique_id;
    int duration{};
    std::string title;
    std::string performer;
    std::string mime_type;
    std::int64_t file_size{};
    std::optional<PhotoSize> thumbnail;
};

struct VoiceNoteInfo {
    FileId file_id{};
    std::string file_unique_id;
    int duration{};
    std::string mime_type;
    std::int64_t file_size{};
};

struct VideoNoteInfo {
    FileId file_id{};
    std::string file_unique_id;
    int length{};
    int duration{};
    std::int64_t file_size{};
    std::optional<PhotoSize> thumbnail;
};

struct AnimationInfo {
    FileId file_id{};
    std::string file_unique_id;
    int width{};
    int height{};
    int duration{};
    std::string mime_type;
    std::string file_name;
    std::int64_t file_size{};
    std::optional<PhotoSize> thumbnail;
};

struct StickerInfo {
    FileId file_id{};
    std::string file_unique_id;
    int width{};
    int height{};
    std::string emoji;
    std::string set_name;
    bool is_animated{false};
    bool is_video{false};
    std::int64_t file_size{};
    std::optional<PhotoSize> thumbnail;
};

struct FileInfo {
    FileId file_id{};
    std::string remote_file_id;
    std::string file_unique_id;
    std::int64_t size{};
    std::int64_t expected_size{};
    std::string local_path;
    bool is_downloading{false};
    bool is_downloaded{false};
    double download_progress{};
};

} // namespace cppgram
