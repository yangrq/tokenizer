#include <functional>
#include <map>
#include <regex>
#include <string>
#include <tuple>
#include <vector>

using token_type = int;

#define _TOWSTRING(x) L##x
#define TOWSTRING(x) _TOWSTRING(x)
#define LITERAL(char_type, str)                                                \
  (std::get<const char_type *>(                                                \
      std::tuple<const char *, const wchar_t *>(str, TOWSTRING(str))))

enum builtin_token_type {
  IDENTIFIER = -7,
  NUMBER_LITERAL = -6,
  STRING_LITERAL = -5,
  CHAR_LITERAL = -4,
  SPACE = -3,
  NEWLINE = -2,
  ERROR = -1
};

template <typename char_type> class base_tokenizer {
  enum error_handle_msg { REGEX_NOT_MATCH, TOKEN_UNEXPECTED };
  using handle_type =
      int(base_tokenizer::error_handle_msg msg,
          typename std::basic_string<char_type>::const_iterator);
  std::function<handle_type> error_handle =
      [](int msg,
         typename std::basic_string<char_type>::const_iterator) -> int {
    throw std::logic_error("default error handle");
    return 1;
  };
  std::vector<token_type> type_map = {-1};
  std::vector<std::basic_string<char_type>> pattern_list;
  std::map<token_type, std::basic_string<char_type>> token_type_string_list;
  std::basic_regex<char_type> pattern;
  int status = 0;
  long long line = 1;
  std::match_results<typename std::basic_string<char_type>::const_iterator>
      match;
  typename std::basic_string<char_type>::const_iterator cur, prev;
  std::basic_string<char_type> str;

public:
  struct token {
    typename std::basic_string<char_type>::const_iterator first, last;
    token_type type;
    long long line;
  };

private:
  token tok;
  void make_pattern();

public:
  void add_token_type(std::basic_string<char_type> pattern, token_type type);
  void add_token_type(std::basic_string<char_type> pattern, token_type type,
                      std::basic_string<char_type> token_type_string);
  bool add_builtin_token_type(builtin_token_type type);
  void set_handle(handle_type handle);
  void assign(const std::basic_string<char_type> &str);
  const token &current_token();
  const std::basic_string<char_type> &current_token_type_string();
  token_type next();
  token_type peek();
  bool expect(token_type expected);
  bool tryget(token_type expected);
  bool succeed(token_type tok_t);
};

template <typename char_type>
inline void base_tokenizer<char_type>::make_pattern() {
  std::basic_string<char_type> s;
  for (auto &item : pattern_list)
    s += "(" + item + ")|";
  s += "(.)";
  pattern.assign(s, pattern.ECMAScript | pattern.optimize);
}

template <typename char_type>
inline void
base_tokenizer<char_type>::add_token_type(std::basic_string<char_type> pattern,
                                          token_type type) {
  if (status)
    throw std::logic_error("add token type after initialize");
  pattern_list.push_back(pattern);
  type_map.push_back(type);
}

template <typename char_type>
inline void base_tokenizer<char_type>::add_token_type(
    std::basic_string<char_type> pattern, token_type type,
    std::basic_string<char_type> token_type_string) {
  add_token_type(pattern, type);
  token_type_string_list.emplace(type, token_type_string);
}

template <typename char_type>
inline bool
base_tokenizer<char_type>::add_builtin_token_type(builtin_token_type type) {
  switch (type) {
  case builtin_token_type::IDENTIFIER:
    add_token_type(LITERAL(char_type, R"#([a-zA-Z_]\w*)#"),
                   builtin_token_type::IDENTIFIER,
                   LITERAL(char_type, "IDENTIFIER"));
    break;
  case builtin_token_type::NUMBER_LITERAL:
    add_token_type(LITERAL(char_type, R"#((?:-?\d+)(?:\.\d+)?)#"),
                   builtin_token_type::NUMBER_LITERAL,
                   LITERAL(char_type, "NUMBER"));
    break;
  case builtin_token_type::STRING_LITERAL:
    add_token_type(LITERAL(char_type, R"#("(?:(?!(?:\\|")).|\\.)*")#"),
                   builtin_token_type::STRING_LITERAL,
                   LITERAL(char_type, "STRING"));
    break;
  case builtin_token_type::CHAR_LITERAL:
    add_token_type(LITERAL(char_type, R"#('(?:(?!(?:\\|')).|\\.)*')#"),
                   builtin_token_type::CHAR_LITERAL,
                   LITERAL(char_type, "CHAR"));
    break;
  case builtin_token_type::SPACE:
    add_token_type(LITERAL(char_type, R"*([ \t]+)*"), builtin_token_type::SPACE,
                   LITERAL(char_type, "SPACE"));
    break;
  case builtin_token_type::NEWLINE:
    add_token_type(LITERAL(char_type, R"*((?:\r\n)+|\n+|\r+)*"),
                   builtin_token_type::NEWLINE, LITERAL(char_type, "NEWLINE"));
    break;
  default:
    return false;
  }
  return true;
}

template <typename char_type>
inline void base_tokenizer<char_type>::set_handle(handle_type handle) {
  this->handle = handle;
}

template <typename char_type>
inline void
base_tokenizer<char_type>::assign(const std::basic_string<char_type> &str) {
  this->str = str;
  make_pattern();
  prev = this->str.begin();
  cur = this->str.begin();
  type_map.push_back(builtin_token_type::ERROR);
  status = 1;
}
template <typename char_type>
inline const typename base_tokenizer<char_type>::token &
base_tokenizer<char_type>::current_token() {
  return tok;
}

template <typename char_type>
inline const std::basic_string<char_type> &
base_tokenizer<char_type>::current_token_type_string() {
  return token_type_string_list.at(tok.type);
}

template <typename char_type>
inline token_type base_tokenizer<char_type>::next() {
  tok.type = ERROR;
  if (std::regex_search(cur, str.cend(), match, pattern)) {
    auto type = [&]() -> unsigned {
      for (unsigned index = 1; index < match.size(); ++index)
        if (match[index].matched)
          return index;
      return 0;
    }();
    if (!type)
      throw std::logic_error("unavailable");
    if (type_map.at(type) == builtin_token_type::NEWLINE)
      ++line;
    tok.first = match[type].first;
    tok.last = match[type].second;
    tok.type = type_map.at(type);
    tok.line = line;
    prev = cur;
    cur = tok.last;
  } else {
    if (cur == str.end())
      return ERROR;
    if (!error_handle(REGEX_NOT_MATCH, cur))
      throw std::logic_error("unhandled error");
  }
  return tok.type;
}

template <typename char_type>
inline token_type base_tokenizer<char_type>::peek() {
  tok.type = ERROR;
  if (std::regex_search(cur, str.cend(), match, pattern)) {
    auto type = [&]() -> unsigned {
      for (unsigned index = 1; index < match.size(); ++index)
        if (match[index].matched)
          return index;
      return 0;
    }();
    if (!type)
      throw std::logic_error("unavailable");
    tok.first = match[type].first;
    tok.last = match[type].second;
    tok.type = type_map.at(type);
    tok.line = line;
  } else {
    if (cur == str.end())
      return ERROR;
    if (!error_handle(REGEX_NOT_MATCH, cur))
      throw std::logic_error("unhandled error");
  }
  return tok.type;
}

template <typename char_type>
inline bool base_tokenizer<char_type>::expect(token_type expected) {
  auto type = peek();
  if (type != expected) {
    error_handle(TOKEN_UNEXPECTED, cur);
    return false;
  }
  return true;
}

template <typename char_type>
inline bool base_tokenizer<char_type>::tryget(token_type expected) {
  auto type = peek();
  if (type == expected) {
    next();
    return true;
  }
  return false;
}

template <typename char_type>
inline bool base_tokenizer<char_type>::succeed(token_type tok_t) {
  return tok_t != ERROR;
}

using tokenizer = base_tokenizer<char>;
using wtokenizer = base_tokenizer<wchar_t>;