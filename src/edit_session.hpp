#pragma once
// Editing state and shareable, metadata-free patches for the Rizia save editor.
// C++17, standard library only.
//
// API:
//   EditSession session(document);             // document must outlive session
//   session.applyBatch({{key, value}, ...});    // one atomic, undoable operation
//   session.pending();                        // read-only normalized changes
//   session.undo(); session.redo();
//   session.clearChanges();                   // undoable "revert all changes"
//   session.reset();                          // discard changes AND all history
//   serializePatch(session.pending());        // no campaign or player metadata
//   session.applyBatch(parsePatch(text));      // validate against this document
//
// A no-op does not create history or discard redo. Changes are compared against
// the original lexical value, so "1" and "1.0" are intentionally distinct.
// History stores only each operation's changed keys, not copies of whole saves.
#include "save_core.hpp"
#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace rizia {

using ChangeMap = std::map<std::string, std::string>;

class EditSession {
    struct Delta {
        std::string key;
        std::optional<std::string> before;
        std::optional<std::string> after;
    };
    struct Step { std::vector<Delta> deltas; };

    const Document& document_;
    ChangeMap pending_;
    std::vector<Step> undo_;
    std::vector<Step> redo_;

    static Step difference(const ChangeMap& before, const ChangeMap& after) {
        Step step;
        auto a = before.begin();
        auto b = after.begin();
        while (a != before.end() || b != after.end()) {
            if (b == after.end() || (a != before.end() && a->first < b->first)) {
                step.deltas.push_back(Delta{a->first, a->second, std::nullopt});
                ++a;
            } else if (a == before.end() || b->first < a->first) {
                step.deltas.push_back(Delta{b->first, std::nullopt, b->second});
                ++b;
            } else {
                if (a->second != b->second)
                    step.deltas.push_back(Delta{a->first, a->second, b->second});
                ++a;
                ++b;
            }
        }
        return step;
    }

    // Every potentially throwing operation happens before the state swap.
    bool commit(ChangeMap next) {
        Step step = difference(pending_, next);
        if (step.deltas.empty()) return false;
        undo_.push_back(std::move(step));
        pending_.swap(next);
        redo_.clear();
        return true;
    }

    static ChangeMap replay(const ChangeMap& from, const Step& step, bool forward) {
        ChangeMap next = from;
        for (const auto& delta : step.deltas) {
            const auto& value = forward ? delta.after : delta.before;
            if (value) next[delta.key] = *value;
            else next.erase(delta.key);
        }
        return next;
    }

public:
    explicit EditSession(const Document& document) : document_(document) {}
    // Avoid accidentally retaining a reference to a temporary parsed document.
    EditSession(Document&&) = delete;
    EditSession(const Document&&) = delete;

    const ChangeMap& pending() const noexcept { return pending_; }
    std::size_t changedCount() const noexcept { return pending_.size(); }
    bool canUndo() const noexcept { return !undo_.empty(); }
    bool canRedo() const noexcept { return !redo_.empty(); }

    bool applyBatch(const ChangeMap& values) {
        // Validate ALL supplied entries before normalization. This prevents an
        // unknown key or invalid number being hidden by an otherwise valid batch.
        ChangeMap next = pending_;
        document_.validateChanges(values);
        for (const auto& entry : values) {
            const Field* original = document_.find(entry.first);
            // validateChanges() has already checked membership and type.
            if (!original) detail::fail("Unknown or non-editable Rizia field");
            if (entry.second == original->value) next.erase(entry.first);
            else next[entry.first] = entry.second;
        }
        // The combined batch must also fit when earlier edits are pending.
        if (!pending_.empty()) document_.validateChanges(next);
        return commit(std::move(next));
    }

    bool undo() {
        if (undo_.empty()) return false;
        ChangeMap next = replay(pending_, undo_.back(), false);
        redo_.push_back(undo_.back());
        pending_.swap(next);
        undo_.pop_back();
        return true;
    }

    bool redo() {
        if (redo_.empty()) return false;
        ChangeMap next = replay(pending_, redo_.back(), true);
        undo_.push_back(redo_.back());
        pending_.swap(next);
        redo_.pop_back();
        return true;
    }

    bool revertField(const std::string& key) {
        if (!pending_.count(key)) return false;
        // Remove the override without re-validating the game's original lexeme.
        ChangeMap next = pending_;
        next.erase(key);
        return commit(std::move(next));
    }

    bool clearChanges() {
        // An unchanged original file may contain an out-of-range value. Revert
        // removes that patch rather than trying to re-enter the original number.
        document_.validateChanges({});
        return commit({});
    }

    void reset() noexcept {
        pending_.clear();
        undo_.clear();
        redo_.clear();
    }
};

inline constexpr std::size_t kMaxPatchEntries = 2000;
inline constexpr std::size_t kMaxPatchBytes = 256u * 1024u;
inline constexpr const char* kPatchHeader = "# SuzerainPatch 1";
inline constexpr const char* kLegacyPatchHeader = "# RiziaPatch 1";

namespace patch_detail {

inline bool space(char c) { return c == ' ' || c == '\t' || c == '\r' || c == '\n'; }
inline std::string trim(const std::string& text) {
    std::size_t begin = 0, end = text.size();
    while (begin < end && space(text[begin])) ++begin;
    while (end > begin && space(text[end - 1])) --end;
    return text.substr(begin, end - begin);
}

// Actual game keys use ASCII letters, digits, underscores and periods.
// The whitelist also prevents line, comment and assignment injection.
inline bool validKey(const std::string& key, const std::string& story) {
    const auto prefix = fieldPrefix(story);
    const auto prefixSize = prefix.size();
    if (prefix.empty() || key.size() <= prefixSize || key.compare(0, prefixSize, prefix) != 0 ||
        !detail::idStart(key[prefixSize])) return false;
    for (std::size_t i = prefixSize + 1; i < key.size(); ++i)
        if (!detail::idPart(key[i]) && key[i] != '.') return false;
    return true;
}

inline bool validValueSyntax(const std::string& value) {
    return value == "true" || value == "false" || detail::validNumberSyntax(value);
}

[[noreturn]] inline void lineError(std::size_t line, const char* reason) {
    throw std::runtime_error("Patch line " + std::to_string(line) + ": " + reason);
}

} // namespace patch_detail

// Parses only a data map. It does not bind keys to a save or decide their type.
// applyBatch() performs document membership, bool/number and range validation.
// Accepted whitespace: spaces/tabs around keys, '=' and values; LF/CRLF/CR.
// Blank lines, # comments and inline # comments are accepted. The version header
// is required once, before the first assignment; an initial UTF-8 BOM is allowed.
inline ChangeMap parsePatch(const std::string& text, const std::string& expectedStory = "StoryPack_Rizia") {
    if (!isSupportedStory(expectedStory)) detail::fail("Unsupported target story pack");
    if (text.size() > kMaxPatchBytes) detail::fail("Patch exceeds the 256 KiB size limit");
    for (unsigned char c : text)
        if ((c < 0x20u && c != '\t' && c != '\r' && c != '\n') || c == 0x7fu)
            detail::fail("Patch contains a binary or unsupported control character");

    ChangeMap result;
    bool header = false;
    std::size_t start = 0, lineNumber = 0;
    if (text.size() >= 3 && static_cast<unsigned char>(text[0]) == 0xefu &&
        static_cast<unsigned char>(text[1]) == 0xbbu &&
        static_cast<unsigned char>(text[2]) == 0xbfu) start = 3;

    while (start < text.size()) {
        ++lineNumber;
        const auto newline = text.find_first_of("\r\n", start);
        const auto end = newline == std::string::npos ? text.size() : newline;
        std::string line = patch_detail::trim(text.substr(start, end - start));
        start = end;
        if (start < text.size()) {
            const char first = text[start++];
            if (start < text.size() &&
                ((first == '\r' && text[start] == '\n') || (first == '\n' && text[start] == '\r'))) ++start;
        }

        if (line.empty()) continue;
        if (line == std::string(kPatchHeader) + ' ' + expectedStory || line == kLegacyPatchHeader) {
            if (header) patch_detail::lineError(lineNumber, "duplicate version header");
            if (line == kLegacyPatchHeader && expectedStory != "StoryPack_Rizia")
                patch_detail::lineError(lineNumber, "legacy Rizia patch cannot be used with this story");
            header = true;
            continue;
        }
        if (line.rfind("# RiziaPatch", 0) == 0 || line.rfind("# SuzerainPatch", 0) == 0)
            patch_detail::lineError(lineNumber, "unsupported patch version or mismatched story pack");
        if (line[0] == '#') continue;
        if (!header) patch_detail::lineError(lineNumber, "missing patch header and story binding");

        const auto comment = line.find('#');
        if (comment != std::string::npos) line = patch_detail::trim(line.substr(0, comment));
        const auto equals = line.find('=');
        if (equals == std::string::npos || line.find('=', equals + 1) != std::string::npos)
            patch_detail::lineError(lineNumber, "expected one key=value assignment");
        std::string key = patch_detail::trim(line.substr(0, equals));
        std::string value = patch_detail::trim(line.substr(equals + 1));
        if (!patch_detail::validKey(key, expectedStory))
            patch_detail::lineError(lineNumber, "invalid field name or mismatched story prefix");
        if (!patch_detail::validValueSyntax(value))
            patch_detail::lineError(lineNumber, "value must be a decimal number, true or false");
        if (!result.emplace(std::move(key), std::move(value)).second)
            patch_detail::lineError(lineNumber, "duplicate field name");
        if (result.size() > kMaxPatchEntries)
            patch_detail::lineError(lineNumber, "patch exceeds 2000 entries");
    }
    if (!header) detail::fail("Patch is missing its header and story binding");
    return result;
}

// Deterministic, metadata-free export. Syntax and file limits are checked here;
// map membership and field type require a Document and are checked on import.
inline std::string serializePatch(const ChangeMap& changes, const std::string& story = "StoryPack_Rizia") {
    if (!isSupportedStory(story)) detail::fail("Unsupported patch story pack");
    if (changes.size() > kMaxPatchEntries) detail::fail("Patch exceeds 2000 entries");
    std::string text = std::string(kPatchHeader) + ' ' + story + '\n';
    for (const auto& entry : changes) {
        if (!patch_detail::validKey(entry.first, story)) detail::fail("Patch contains an invalid or foreign-story field name");
        if (!patch_detail::validValueSyntax(entry.second))
            detail::fail("Patch values must be decimal numbers, true or false");
        if (entry.first.size() > kMaxPatchBytes || entry.second.size() > kMaxPatchBytes ||
            entry.first.size() + entry.second.size() + 2 > kMaxPatchBytes - text.size())
            detail::fail("Patch exceeds the 256 KiB size limit");
        text += entry.first;
        text += '=';
        text += entry.second;
        text += '\n';
    }
    return text;
}

} // namespace rizia
