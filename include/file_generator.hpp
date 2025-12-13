#pragma once
#include <cassert>
#include <cstdint>
#include <deque>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <set>
#include <string>
#include <string_view>
#include <vector>

namespace sopho
{

    std::string read_file(std::filesystem::path fs_path)
    {
        std::ifstream file_stream(fs_path, std::ios::binary);
        assert(file_stream.is_open());
        const auto file_size = std::filesystem::file_size(fs_path);

        std::string content;
        content.resize(file_size);
        file_stream.read(content.data(), file_size);
        return content;
    }

    std::vector<std::string_view> split_lines(std::string_view str)
    {
        std::vector<std::string_view> result;

        size_t start = 0;
        while (start < str.size())
        {
            size_t end = str.find('\n', start);

            if (end == std::string_view::npos)
            {
                result.emplace_back(str.substr(start));
                break;
            }

            size_t line_end = end;
            if (line_end > start && str[line_end - 1] == '\r')
            {
                line_end -= 1;
            }

            result.emplace_back(str.substr(start, line_end - start));

            start = end + 1;
        }

        return result;
    }

    std::string_view ltrim(std::string_view sv)
    {
        size_t i = 0;
        while (i < sv.size() && (sv[i] == ' ' || sv[i] == '\t'))
        {
            ++i;
        }
        return sv.substr(i);
    }

    bool starts_with(std::string_view sv, std::string_view prefix)
    {
        return sv.size() >= prefix.size() && sv.compare(0, prefix.size(), prefix) == 0;
    }

    struct FileEntry
    {
        std::string name{};
        std::uint64_t size{};
        std::uint64_t hash{};
        std::unique_ptr<std::string> content{};

        friend bool operator<(const FileEntry& a, const FileEntry& b)
        {
            if (a.name != b.name)
                return a.name < b.name;
            if (a.size != b.size)
                return a.size < b.size;
            if (a.hash != b.hash)
                return a.hash < b.hash;

            return *a.content < *b.content;
        }
    };


    FileEntry make_entry(std::filesystem::path fs_path)
    {
        auto file_content = read_file(fs_path);

        FileEntry entry{};
        entry.name = fs_path.filename().string();
        entry.size = file_content.size();
        entry.hash = std::hash<std::string>{}(file_content);
        entry.content = std::make_unique<std::string>(std::move(file_content));

        return entry;
    }

    struct Context
    {
        std::filesystem::path include_path{};
        std::deque<std::string> file_content{};
        std::set<FileEntry> file_entries{};
        std::set<std::string> std_header{};
    };


    std::vector<std::string_view> collect_file(std::string_view file_path, Context& context)
    {
        std::vector<std::string_view> result{};

        std::string file_name_comment = "// " + std::string(file_path);
        context.file_content.emplace_back(file_name_comment);
        result.emplace_back(context.file_content.back());

        std::filesystem::path fs_path = file_path;
        assert(std::filesystem::exists(fs_path));
        auto [iter, inserted] = context.file_entries.emplace(make_entry(fs_path));

        if (!inserted)
        {
            return {};
        }

        auto lines = split_lines(std::string_view(*iter->content));

        for (const auto& line : lines)
        {
            auto line_content = ltrim(line);
            if (line_content.length() == 0)
            {
                continue;
            }
            if (line_content[0] != '#')
            {
                result.emplace_back(line);
                continue;
            }
            line_content = line_content.substr(1);
            line_content = ltrim(line_content);
            if (starts_with(line_content, "include"))
            {
                line_content = line_content.substr(7);
                line_content = ltrim(line_content);
                if (line_content[0] == '<')
                {
                    line_content = line_content.substr(1);
                    auto index = line_content.find('>');
                    assert(index != std::string_view::npos);
                    auto file_name = line_content.substr(0, index);
                    if (context.std_header.find(std::string(file_name)) != context.std_header.end())
                    {
                        continue;
                    }
                    context.std_header.emplace(file_name);
                    result.emplace_back(line);
                    continue;
                }
                else if (line_content[0] == '"')
                {
                    line_content = line_content.substr(1);
                    auto index = line_content.find('"');
                    assert(index != std::string_view::npos);
                    std::string file_name{line_content.substr(0, index)};
                    std::filesystem::path new_fs_path = fs_path.parent_path() / file_name;
                    if (!std::filesystem::exists(new_fs_path))
                    {
                        new_fs_path = context.include_path / file_name;
                    }
                    auto file_content = collect_file(std::string_view(new_fs_path.string()), context);
                    result.insert(result.end(), file_content.begin(), file_content.end());
                }
                else
                {
                    result.emplace_back(line);
                }
            }
            else if (starts_with(line_content, "pragma"))
            {
                line_content = line_content.substr(6);
                line_content = ltrim(line_content);
                if (starts_with(line_content, "once"))
                {
                    continue;
                }
                result.emplace_back(line);
            }
            else
            {
                result.emplace_back(line);
            }
        }
        return result;
    }

    void single_header_generator(std::string_view file_path)
    {
        Context context{};
        std::filesystem::path fs_path = file_path;
        assert(std::filesystem::exists(fs_path));
        context.include_path = fs_path.parent_path();
        auto lines = collect_file(file_path, context);
        std::ofstream out("sob.hpp", std::ios::binary);
        assert(out.is_open());

        for (auto sv : lines)
        {
            out.write(sv.data(), sv.size());
            out.put('\n');
        }
    }
} // namespace sopho
