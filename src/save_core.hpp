#pragma once
// Suzerain save reader/editor. C++17, standard library only.
// The outer JSON and Lua string are scanned; no whole-file reserialization occurs.
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <locale>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace rizia {

inline bool isSupportedStory(const std::string& story) {
    return story == "StoryPack_Rizia" || story == "StoryPack_Main";
}
inline std::string fieldPrefix(const std::string& story) {
    if (story == "StoryPack_Rizia") return "RiziaDLC.";
    if (story == "StoryPack_Main") return "BaseGame.";
    return {};
}

struct Field {
    std::string key;
    std::string value;
    bool is_bool = false;
    std::size_t start = 0;
    std::size_t end = 0;
};

struct Document {
    std::string original, version, story, turn, campaignName;
    std::vector<Field> fields;
    static Document parse(const std::string& input);
    void validateChanges(const std::map<std::string, std::string>& changes) const;
    std::string render(const std::map<std::string, std::string>& changes) const;
    const Field* find(const std::string& key) const;
private:
    // Positions survive copies and moves of a parsed document.
    std::unordered_map<std::string, std::size_t> fieldIndex_;
    struct Patch {
        std::size_t start, end;
        const std::string* value;
    };
    std::size_t planEdits(const std::map<std::string, std::string>& changes,
                          std::vector<Patch>* patches) const;
};

namespace detail {
constexpr std::size_t kMaxFileSize = 50u * 1024u * 1024u;
constexpr std::size_t kMaxDepth = 128;

[[noreturn]] inline void fail(const char* message) {
    throw std::runtime_error(message);
}
inline bool digit(char c) { return c >= '0' && c <= '9'; }
inline bool jsonSpace(char c) { return c == ' ' || c == '\t' || c == '\r' || c == '\n'; }
inline bool luaSpace(char c) { return jsonSpace(c) || c == '\v' || c == '\f'; }
inline bool idStart(char c) {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_';
}
inline bool idPart(char c) { return idStart(c) || digit(c); }
inline int hex(char c) {
    if (digit(c)) return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}
inline void appendUtf8(std::string& out, std::uint32_t cp) {
    if (cp > 0x10ffffu || (cp >= 0xd800u && cp <= 0xdfffu))
        fail("Invalid Unicode code point");
    if (cp <= 0x7fu) out.push_back(static_cast<char>(cp));
    else if (cp <= 0x7ffu) {
        out.push_back(static_cast<char>(0xc0u | (cp >> 6)));
        out.push_back(static_cast<char>(0x80u | (cp & 0x3fu)));
    } else if (cp <= 0xffffu) {
        out.push_back(static_cast<char>(0xe0u | (cp >> 12)));
        out.push_back(static_cast<char>(0x80u | ((cp >> 6) & 0x3fu)));
        out.push_back(static_cast<char>(0x80u | (cp & 0x3fu)));
    } else {
        out.push_back(static_cast<char>(0xf0u | (cp >> 18)));
        out.push_back(static_cast<char>(0x80u | ((cp >> 12) & 0x3fu)));
        out.push_back(static_cast<char>(0x80u | ((cp >> 6) & 0x3fu)));
        out.push_back(static_cast<char>(0x80u | (cp & 0x3fu)));
    }
}
inline std::uint32_t readHex4(const std::string& s, std::size_t& p) {
    if (p > s.size() || s.size() - p < 4) fail("Incomplete JSON Unicode escape");
    std::uint32_t cp = 0;
    for (int n = 0; n != 4; ++n) {
        int h = hex(s[p++]);
        if (h < 0) fail("Invalid JSON Unicode escape");
        cp = cp * 16u + static_cast<unsigned>(h);
    }
    return cp;
}
// p is immediately after the backslash. Also used by the streaming Lua cursor.
inline std::string decodeJsonEscape(const std::string& s, std::size_t& p) {
    if (p >= s.size()) fail("Incomplete JSON escape");
    char c = s[p++];
    switch (c) {
    case '"': return "\"";
    case '\\': return "\\";
    case '/': return "/";
    case 'b': return "\b";
    case 'f': return "\f";
    case 'n': return "\n";
    case 'r': return "\r";
    case 't': return "\t";
    case 'u': {
        auto cp = readHex4(s, p);
        if (cp >= 0xd800u && cp <= 0xdbffu) {
            if (p > s.size() || s.size() - p < 6 || s[p] != '\\' || s[p + 1] != 'u')
                fail("Unpaired JSON high surrogate");
            p += 2;
            auto low = readHex4(s, p);
            if (low < 0xdc00u || low > 0xdfffu) fail("Invalid JSON surrogate pair");
            cp = 0x10000u + ((cp - 0xd800u) << 10) + low - 0xdc00u;
        } else if (cp >= 0xdc00u && cp <= 0xdfffu) {
            fail("Unpaired JSON low surrogate");
        }
        std::string out;
        appendUtf8(out, cp);
        return out;
    }
    default: fail("Invalid JSON escape");
    }
}

struct StringSpan { std::size_t begin = 0, end = 0; };

class JsonReader {
    const std::string& s_;
    Document& doc_;
    std::size_t p_ = 0;
    bool haveVariables_ = false;
    StringSpan variables_;

    void space() { while (p_ < s_.size() && jsonSpace(s_[p_])) ++p_; }
    void expect(char c) {
        space();
        if (p_ >= s_.size() || s_[p_] != c) fail("Malformed JSON punctuation");
        ++p_;
    }
    void utf8(std::string* out) {
        const std::size_t first = p_;
        unsigned char lead = static_cast<unsigned char>(s_[p_++]);
        unsigned count = 0;
        std::uint32_t cp = 0, minimum = 0;
        if (lead >= 0xc2u && lead <= 0xdfu) { count = 1; cp = lead & 0x1fu; minimum = 0x80; }
        else if (lead >= 0xe0u && lead <= 0xefu) { count = 2; cp = lead & 0x0fu; minimum = 0x800; }
        else if (lead >= 0xf0u && lead <= 0xf4u) { count = 3; cp = lead & 0x07u; minimum = 0x10000; }
        else fail("Invalid UTF-8 in JSON");
        if (s_.size() - p_ < count) fail("Truncated UTF-8 in JSON");
        for (unsigned i = 0; i < count; ++i) {
            unsigned char c = static_cast<unsigned char>(s_[p_++]);
            if ((c & 0xc0u) != 0x80u) fail("Invalid UTF-8 continuation");
            cp = (cp << 6) | (c & 0x3fu);
        }
        if (cp < minimum || cp > 0x10ffffu || (cp >= 0xd800u && cp <= 0xdfffu))
            fail("Invalid UTF-8 code point");
        if (out) out->append(s_, first, p_ - first);
    }
    StringSpan string(std::string* out = nullptr) {
        expect('"');
        StringSpan span{p_, 0};
        while (p_ < s_.size()) {
            unsigned char c = static_cast<unsigned char>(s_[p_]);
            if (c == '"') { span.end = p_++; return span; }
            if (c < 0x20u) fail("Unescaped control character in JSON string");
            if (c == '\\') {
                ++p_;
                std::string decoded = decodeJsonEscape(s_, p_);
                if (out) *out += decoded;
            } else if (c >= 0x80u) {
                utf8(out);
            } else {
                if (out) out->push_back(s_[p_]);
                ++p_;
            }
        }
        fail("Unterminated JSON string");
    }
    std::string number() {
        const auto start = p_;
        if (p_ < s_.size() && s_[p_] == '-') ++p_;
        if (p_ >= s_.size()) fail("Incomplete JSON number");
        if (s_[p_] == '0') ++p_;
        else {
            if (s_[p_] < '1' || s_[p_] > '9') fail("Invalid JSON number");
            while (p_ < s_.size() && digit(s_[p_])) ++p_;
        }
        if (p_ < s_.size() && s_[p_] == '.') {
            ++p_;
            if (p_ >= s_.size() || !digit(s_[p_])) fail("Invalid JSON fraction");
            while (p_ < s_.size() && digit(s_[p_])) ++p_;
        }
        if (p_ < s_.size() && (s_[p_] == 'e' || s_[p_] == 'E')) {
            ++p_;
            if (p_ < s_.size() && (s_[p_] == '+' || s_[p_] == '-')) ++p_;
            if (p_ >= s_.size() || !digit(s_[p_])) fail("Invalid JSON exponent");
            while (p_ < s_.size() && digit(s_[p_])) ++p_;
        }
        return s_.substr(start, p_ - start);
    }
    void literal(const char* value) {
        while (*value) {
            if (p_ >= s_.size() || s_[p_++] != *value++) fail("Invalid JSON literal");
        }
    }
    void value(std::size_t depth) {
        space();
        if (p_ >= s_.size()) fail("Missing JSON value");
        const char c = s_[p_];
        if (c == '{') object(depth + 1, false);
        else if (c == '[') array(depth + 1);
        else if (c == '"') string();
        else if (c == 't') literal("true");
        else if (c == 'f') literal("false");
        else if (c == 'n') literal("null");
        else if (c == '-' || digit(c)) number();
        else fail("Invalid JSON value");
    }
    void array(std::size_t depth) {
        if (depth > kMaxDepth) fail("JSON nesting exceeds 128 levels");
        expect('[');
        space();
        if (p_ < s_.size() && s_[p_] == ']') { ++p_; return; }
        for (;;) {
            value(depth);
            space();
            if (p_ >= s_.size()) fail("Unterminated JSON array");
            if (s_[p_] == ']') { ++p_; return; }
            expect(',');
        }
    }
    void object(std::size_t depth, bool root) {
        if (depth > kMaxDepth) fail("JSON nesting exceeds 128 levels");
        expect('{');
        std::unordered_set<std::string> keys;
        space();
        if (p_ < s_.size() && s_[p_] == '}') { ++p_; return; }
        for (;;) {
            std::string key;
            string(&key);
            if (!keys.insert(key).second) fail("Duplicate JSON key: ambiguous save");
            expect(':');
            space();
            if (root && key == "variables") {
                if (haveVariables_) fail("Multiple variables containers");
                if (p_ >= s_.size() || s_[p_] != '"') fail("variables must be a JSON string");
                variables_ = string();
                haveVariables_ = true;
            } else if (root && (key == "version" || key == "currentStoryPack" || key == "campaignName")) {
                if (p_ >= s_.size() || s_[p_] != '"') fail("Invalid save metadata type");
                std::string decoded;
                string(&decoded);
                if (key == "version") doc_.version = std::move(decoded);
                else if (key == "currentStoryPack") doc_.story = std::move(decoded);
                else doc_.campaignName = std::move(decoded);
            } else if (root && key == "turnNo") {
                if (p_ < s_.size() && s_[p_] == '"') string(&doc_.turn);
                else if (p_ < s_.size() && (s_[p_] == '-' || digit(s_[p_]))) doc_.turn = number();
                else fail("Invalid turnNo metadata type");
            } else value(depth);
            space();
            if (p_ >= s_.size()) fail("Unterminated JSON object");
            if (s_[p_] == '}') { ++p_; return; }
            expect(',');
        }
    }
public:
    JsonReader(const std::string& s, Document& doc) : s_(s), doc_(doc) {}
    StringSpan read() {
        if (s_.size() >= 3 && static_cast<unsigned char>(s_[0]) == 0xefu &&
            static_cast<unsigned char>(s_[1]) == 0xbbu &&
            static_cast<unsigned char>(s_[2]) == 0xbfu) p_ = 3;
        space();
        if (p_ >= s_.size() || s_[p_] != '{') fail("Save must be a JSON object");
        object(1, true);
        space();
        if (p_ != s_.size()) fail("Unexpected data after JSON document");
        if (!haveVariables_) fail("Save has no top-level variables string");
        return variables_;
    }
};

// Decodes the JSON string lazily. This keeps memory bounded even at the 50 MiB
// limit and gives each Lua token its exact original-file byte span.
class StringCursor {
    const std::string* s_;
    std::size_t p_, limit_, after_ = 0, offset_ = 0;
    std::string pending_;
    void load() {
        if (offset_ < pending_.size() || p_ == limit_) return;
        after_ = p_;
        if ((*s_)[after_] == '\\') {
            ++after_;
            pending_ = decodeJsonEscape(*s_, after_);
        } else {
            pending_.assign(1, (*s_)[after_++]);
        }
        offset_ = 0;
        if (after_ > limit_) fail("JSON string boundary error");
    }
public:
    StringCursor(const std::string& s, StringSpan span) : s_(&s), p_(span.begin), limit_(span.end) {}
    bool end() const { return p_ == limit_ && offset_ >= pending_.size(); }
    std::size_t pos() const { return p_; }
    char peek() { load(); return end() ? '\0' : pending_[offset_]; }
    char get() {
        load();
        if (end()) fail("Unexpected end of variables string");
        char c = pending_[offset_++];
        if (offset_ == pending_.size()) {
            p_ = after_;
            pending_.clear();
            offset_ = 0;
        }
        return c;
    }
    char ahead(std::size_t n) const {
        auto copy = *this;
        while (n-- && !copy.end()) copy.get();
        return copy.end() ? '\0' : copy.peek();
    }
};

enum class TokenKind { End, String, Number, Identifier, Punctuation };
struct Token {
    TokenKind kind = TokenKind::End;
    std::string text;
    std::size_t start = 0, end = 0;
    bool is(char c) const { return kind == TokenKind::Punctuation && text.size() == 1 && text[0] == c; }
};

class LuaLexer {
    StringCursor cur_;
    bool buffered_ = false;
    Token buffer_;
    // If the cursor starts with [=*[ return its number of '=' characters.
    int longLevel() const {
        auto c = cur_;
        if (c.end() || c.get() != '[') return -1;
        std::size_t level = 0;
        while (!c.end() && c.peek() == '=') { c.get(); ++level; }
        if (!c.end() && c.peek() == '[') {
            if (level > 1024) fail("Lua long string delimiter is too large");
            return static_cast<int>(level);
        }
        return -1;
    }
    std::string longString(int level, bool retain) {
        cur_.get();
        for (int i = 0; i < level; ++i) cur_.get();
        cur_.get();
        // Lua discards one initial line break in long strings.
        if (!cur_.end() && (cur_.peek() == '\n' || cur_.peek() == '\r')) {
            char first = cur_.get();
            if (!cur_.end() && ((first == '\r' && cur_.peek() == '\n') ||
                                (first == '\n' && cur_.peek() == '\r'))) cur_.get();
        }
        std::string text;
        while (!cur_.end()) {
            if (cur_.peek() == ']') {
                auto copy = cur_;
                copy.get();
                int n = 0;
                while (n < level && !copy.end() && copy.peek() == '=') { copy.get(); ++n; }
                if (n == level && !copy.end() && copy.peek() == ']') {
                    copy.get(); cur_ = copy; return text;
                }
            }
            char c = cur_.get();
            if (retain) text.push_back(c);
        }
        fail("Unterminated Lua long string/comment");
    }
    void space() {
        for (;;) {
            while (!cur_.end() && luaSpace(cur_.peek())) cur_.get();
            if (cur_.end() || cur_.peek() != '-' || cur_.ahead(1) != '-') return;
            cur_.get(); cur_.get();
            int level = longLevel();
            if (level >= 0) longString(level, false);
            else while (!cur_.end() && cur_.peek() != '\r' && cur_.peek() != '\n') cur_.get();
        }
    }
    std::string quoted() {
        char quote = cur_.get();
        std::string out;
        while (!cur_.end()) {
            char c = cur_.get();
            if (c == quote) return out;
            if (c == '\n' || c == '\r' || c == '\0') fail("Unescaped newline/control in Lua string");
            if (c != '\\') { out.push_back(c); continue; }
            if (cur_.end()) fail("Incomplete Lua escape");
            c = cur_.get();
            switch (c) {
            case 'a': out.push_back('\a'); break;
            case 'b': out.push_back('\b'); break;
            case 'f': out.push_back('\f'); break;
            case 'n': out.push_back('\n'); break;
            case 'r': out.push_back('\r'); break;
            case 't': out.push_back('\t'); break;
            case 'v': out.push_back('\v'); break;
            case '\\': case '"': case '\'': out.push_back(c); break;
            case '\n': case '\r':
                if (!cur_.end() && ((c == '\r' && cur_.peek() == '\n') ||
                                    (c == '\n' && cur_.peek() == '\r'))) cur_.get();
                out.push_back('\n'); break;
            case 'z':
                while (!cur_.end() && luaSpace(cur_.peek())) cur_.get();
                break;
            case 'x': {
                unsigned n = 0;
                for (int i = 0; i != 2; ++i) {
                    if (cur_.end()) fail("Incomplete Lua hex escape");
                    int h = hex(cur_.get());
                    if (h < 0) fail("Invalid Lua hex escape");
                    n = n * 16u + static_cast<unsigned>(h);
                }
                out.push_back(static_cast<char>(n)); break;
            }
            case 'u': {
                if (cur_.end() || cur_.get() != '{') fail("Invalid Lua Unicode escape");
                std::uint32_t cp = 0;
                unsigned count = 0;
                while (!cur_.end() && cur_.peek() != '}') {
                    int h = hex(cur_.get());
                    if (h < 0 || ++count > 6) fail("Invalid Lua Unicode escape");
                    cp = cp * 16u + static_cast<unsigned>(h);
                }
                if (!count || cur_.end()) fail("Incomplete Lua Unicode escape");
                cur_.get(); appendUtf8(out, cp); break;
            }
            default:
                if (!digit(c)) fail("Unsupported Lua string escape");
                unsigned n = static_cast<unsigned>(c - '0');
                for (int i = 1; i < 3 && !cur_.end() && digit(cur_.peek()); ++i)
                    n = n * 10u + static_cast<unsigned>(cur_.get() - '0');
                if (n > 255) fail("Lua decimal escape exceeds 255");
                out.push_back(static_cast<char>(n));
            }
        }
        fail("Unterminated Lua string");
    }
    Token scan() {
        space();
        Token t;
        t.start = t.end = cur_.pos();
        if (cur_.end()) return t;
        char c = cur_.peek();
        if (c == '"' || c == '\'') { t.kind = TokenKind::String; t.text = quoted(); }
        else if (c == '[' && longLevel() >= 0) {
            t.kind = TokenKind::String; t.text = longString(longLevel(), true);
        } else if (idStart(c)) {
            t.kind = TokenKind::Identifier;
            do { t.text.push_back(cur_.get()); } while (!cur_.end() && idPart(cur_.peek()));
        } else if (digit(c) || (c == '.' && digit(cur_.ahead(1))) ||
                   (c == '-' && (digit(cur_.ahead(1)) || (cur_.ahead(1) == '.' && digit(cur_.ahead(2)))))) {
            t.kind = TokenKind::Number;
            if (c == '-') t.text.push_back(cur_.get());
            while (!cur_.end() && digit(cur_.peek())) t.text.push_back(cur_.get());
            if (!cur_.end() && cur_.peek() == '.' && cur_.ahead(1) != '.') {
                t.text.push_back(cur_.get());
                while (!cur_.end() && digit(cur_.peek())) t.text.push_back(cur_.get());
            }
            if (!cur_.end() && (cur_.peek() == 'e' || cur_.peek() == 'E')) {
                t.text.push_back(cur_.get());
                if (!cur_.end() && (cur_.peek() == '+' || cur_.peek() == '-')) t.text.push_back(cur_.get());
                if (cur_.end() || !digit(cur_.peek())) fail("Invalid Lua number exponent");
                while (!cur_.end() && digit(cur_.peek())) t.text.push_back(cur_.get());
            }
        } else {
            t.kind = TokenKind::Punctuation;
            t.text.push_back(cur_.get());
        }
        t.end = cur_.pos();
        return t;
    }
public:
    LuaLexer(const std::string& s, StringSpan span) : cur_(s, span) {}
    const Token& peek() {
        if (!buffered_) { buffer_ = scan(); buffered_ = true; }
        return buffer_;
    }
    Token next() {
        if (buffered_) { buffered_ = false; return std::move(buffer_); }
        return scan();
    }
    void expect(char c) { if (!next().is(c)) fail("Malformed Lua variables table"); }
};

class LuaReader {
    LuaLexer lex_;
    Document& doc_;
    // A data value is eligible only if it is exactly one numeric/bool literal.
    // Nested tables/strings and arbitrary other expressions are never exposed.
    Token readValue() {
        Token first = lex_.next();
        if (first.kind == TokenKind::End || first.is(',') || first.is(';') || first.is('}'))
            fail("Missing Lua variable value");
        std::vector<char> stack;
        std::size_t count = 0;
        auto consume = [&](const Token& t) {
            ++count;
            if (t.is('{') || t.is('[') || t.is('(')) {
                stack.push_back(t.text[0]);
                if (stack.size() + 1 > kMaxDepth) fail("Lua nesting exceeds 128 levels");
            } else if (t.is('}') || t.is(']') || t.is(')')) {
                char expected = t.is('}') ? '{' : t.is(']') ? '[' : '(';
                if (stack.empty() || stack.back() != expected) fail("Unbalanced Lua expression");
                stack.pop_back();
            }
        };
        consume(first);
        for (;;) {
            const Token& t = lex_.peek();
            if (t.kind == TokenKind::End) fail("Unterminated Lua variables table");
            if (stack.empty() && (t.is(',') || t.is(';') || t.is('}'))) break;
            consume(lex_.next());
        }
        if (!stack.empty()) fail("Unbalanced Lua variable value");
        if (count != 1) first.kind = TokenKind::Punctuation;
        return first;
    }
public:
    LuaReader(const std::string& s, StringSpan span, Document& doc) : lex_(s, span), doc_(doc) {}
    void read() {
        const auto prefix = fieldPrefix(doc_.story);
        Token name = lex_.next();
        if (name.kind != TokenKind::Identifier || name.text != "Variable")
            fail("Unsupported variables container (expected Variable table)");
        lex_.expect('=');
        lex_.expect('{');
        std::unordered_set<std::string> keys;
        if (lex_.peek().is('}')) lex_.next();
        else for (;;) {
            std::string key;
            bool haveKey = false;
            if (lex_.peek().is('[')) {
                lex_.next();
                Token k = lex_.next();
                if (k.kind != TokenKind::String && k.kind != TokenKind::Number &&
                    !(k.kind == TokenKind::Identifier && (k.text == "true" || k.text == "false")))
                    fail("Unsupported Lua table key");
                haveKey = k.kind == TokenKind::String;
                key = k.text;
                std::string unique = (haveKey ? "s:" : k.kind == TokenKind::Number ? "n:" : "b:") + key;
                if (!keys.insert(unique).second) fail("Duplicate Lua variable key: ambiguous save");
                lex_.expect(']');
                lex_.expect('=');
            } else {
                // Save files use keyed fields, not executable Lua or array entries.
                Token k = lex_.next();
                if (k.kind != TokenKind::Identifier) fail("Unsupported Lua table entry");
                key = k.text;
                haveKey = true;
                if (!keys.insert("s:" + key).second) fail("Duplicate Lua variable key: ambiguous save");
                lex_.expect('=');
            }
            Token value = readValue();
            bool isBool = value.kind == TokenKind::Identifier && (value.text == "true" || value.text == "false");
            if (haveKey && !prefix.empty() && key.size() > prefix.size() && key.compare(0, prefix.size(), prefix) == 0 &&
                (value.kind == TokenKind::Number || isBool)) {
                doc_.fields.push_back(Field{key, value.text, isBool, value.start, value.end});
            }
            Token delimiter = lex_.next();
            if (delimiter.is('}')) break;
            if (!delimiter.is(',') && !delimiter.is(';')) fail("Missing Lua table separator");
            if (lex_.peek().is('}')) { lex_.next(); break; }
        }
        if (lex_.peek().is(';')) lex_.next();
        if (lex_.next().kind != TokenKind::End) fail("Multiple or executable variables containers are not supported");
    }
};

inline bool validNumberSyntax(const std::string& value) {
    if (value.empty()) return false;
    std::size_t p = 0;
    if (value[p] == '-') ++p;
    if (p == value.size()) return false;
    bool any = false;
    while (p < value.size() && digit(value[p])) { ++p; any = true; }
    if (p < value.size() && value[p] == '.') {
        ++p;
        while (p < value.size() && digit(value[p])) { ++p; any = true; }
    }
    if (!any) return false;
    if (p < value.size() && (value[p] == 'e' || value[p] == 'E')) {
        ++p;
        if (p < value.size() && (value[p] == '+' || value[p] == '-')) ++p;
        std::size_t start = p;
        while (p < value.size() && digit(value[p])) ++p;
        if (p == start) return false;
    }
    return p == value.size();
}
inline void validateNumber(const std::string& value) {
    if (value.size() > kMaxFileSize) fail("Replacement number is too long");
    if (!validNumberSyntax(value)) fail("Enter a finite decimal number, not text or an expression");
    // Compare decimal magnitude exactly. Floating conversion alone could round
    // 2147483647.0000000000000000001 down to the allowed maximum.
    std::string digits;
    std::size_t p = value[0] == '-' ? 1u : 0u;
    long long fraction = 0;
    bool afterPoint = false;
    while (p < value.size() && value[p] != 'e' && value[p] != 'E') {
        if (value[p] == '.') afterPoint = true;
        else { digits.push_back(value[p]); if (afterPoint) ++fraction; }
        ++p;
    }
    long long exponent = 0;
    bool negativeExponent = false;
    if (p < value.size()) {
        ++p;
        if (value[p] == '+' || value[p] == '-') { negativeExponent = value[p] == '-'; ++p; }
        for (; p < value.size(); ++p) {
            if (exponent < 1000000000LL)
                exponent = std::min(1000000000LL, exponent * 10 + (value[p] - '0'));
        }
    }
    if (negativeExponent) exponent = -exponent;
    const auto nonzero = digits.find_first_not_of('0');
    if (nonzero != std::string::npos) {
        digits.erase(0, nonzero);
        const auto integerDigits = static_cast<long long>(digits.size()) + exponent - fraction;
        if (integerDigits > 10) fail("Number exceeds the allowed magnitude 2147483647");
        if (integerDigits == 10) {
            auto leading = digits.substr(0, 10);
            leading.append(10 - leading.size(), '0');
            const int compared = leading.compare("2147483647");
            if (compared > 0 || (compared == 0 && digits.size() > 10 &&
                digits.find_first_not_of('0', 10) != std::string::npos))
                fail("Number exceeds the allowed magnitude 2147483647");
        }
    }
    std::istringstream input(value);
    input.imbue(std::locale::classic());
    long double n = 0;
    input >> std::noskipws >> n;
    if (input.fail() || !input.eof() || !std::isfinite(n) || std::fabs(n) > 2147483647.0L)
        fail("Number must be finite and within -2147483647 to 2147483647");
}
} // namespace detail

inline Document Document::parse(const std::string& input) {
    if (input.size() > detail::kMaxFileSize) detail::fail("Save exceeds the 50 MiB size limit");
    Document doc;
    doc.original = input;
    auto span = detail::JsonReader(doc.original, doc).read();
    detail::LuaReader(doc.original, span, doc).read();
    doc.fieldIndex_.reserve(doc.fields.size());
    for (std::size_t index = 0; index < doc.fields.size(); ++index)
        doc.fieldIndex_.emplace(doc.fields[index].key, index);
    return doc;
}

inline const Field* Document::find(const std::string& key) const {
    const auto it = fieldIndex_.find(key);
    return it == fieldIndex_.end() ? nullptr : &fields[it->second];
}

inline std::size_t Document::planEdits(const std::map<std::string, std::string>& changes,
                                       std::vector<Patch>* patches) const {
    if (!isSupportedStory(story)) detail::fail("Saving is blocked: unsupported story pack");
    const auto prefix = fieldPrefix(story);
    std::uint64_t resultingSize = original.size();
    for (const auto& change : changes) {
        const Field* field = find(change.first);
        if (!field || field->key.compare(0, prefix.size(), prefix) != 0)
            detail::fail("Unknown, foreign-story or non-editable field");
        if (field->is_bool) {
            if (change.second != "true" && change.second != "false")
                detail::fail("Boolean fields accept only true or false");
        } else detail::validateNumber(change.second);
        if (field->start > field->end || field->end > original.size())
            detail::fail("Invalid original-file field span");
        if (change.second == field->value) continue;
        std::size_t oldSize = field->end - field->start;
        if (resultingSize < oldSize || change.second.size() > detail::kMaxFileSize)
            detail::fail("Invalid replacement size");
        resultingSize = resultingSize - oldSize + change.second.size();
        if (patches) patches->push_back({field->start, field->end, &change.second});
    }
    if (resultingSize > detail::kMaxFileSize) detail::fail("Edited save exceeds the 50 MiB size limit");
    return static_cast<std::size_t>(resultingSize);
}

inline void Document::validateChanges(const std::map<std::string, std::string>& changes) const {
    planEdits(changes, nullptr);
}

inline std::string Document::render(const std::map<std::string, std::string>& changes) const {
    std::vector<Patch> patches;
    patches.reserve(changes.size());
    const auto resultingSize = planEdits(changes, &patches);
    std::sort(patches.begin(), patches.end(), [](const Patch& a, const Patch& b) { return a.start < b.start; });
    std::string output;
    output.reserve(resultingSize);
    std::size_t cursor = 0;
    for (const auto& patch : patches) {
        if (patch.start < cursor) detail::fail("Overlapping or duplicate edits");
        output.append(original, cursor, patch.start - cursor);
        output += *patch.value;
        cursor = patch.end;
    }
    output.append(original, cursor, original.size() - cursor);
    return output;
}

} // namespace rizia
