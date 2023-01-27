#pragma once

#include <regex>

#include "util/standard_includes.h"

namespace util {

class Tokenizer {
 public:
  // The input to be parsed; what characters should be considered whitespace
  // (and hence automatically skipped); what strings are considered delimiters
  // (i.e., tokens in their own right, no matter what is around them, and
  // reading them when they aren't expected is an error); what strings are
  // considered reserved words (i.e., reading them when they aren't expected is
  // an error); what two strings (if any) are considered to delimit "raw" parts
  // of the input that should be considered tokens verbatim (i.e., all
  // delimiters and whitespace inside those delimiters are preserved).
  //
  // Note that '\n' is always considered a delimiter, but if it is also included
  // in whitespace then it is ignored when reading tokens. Also, if one
  // delimiter is a prefix of another then the input is tokenized based on the
  // longer delimiter first, then the shorter delimiter.
  Tokenizer(const string& input, const set<char>& whitespace,
            const set<string>& delimiters, const set<string>& reserved_words,
            const optional<std::pair<string, string>>& raw = nullopt)
      : delimiters_(delimiters), reserved_words_(reserved_words) {
    // Is '\n' considered whitespace?
    newline_is_whitespace_ = whitespace.count('\n');

    // For tokenization, '\n' is always considered a delimiter.
    delimiters_.insert("\n");

    // Break the input into raw and non-raw pieces; turn the raw pieces directly
    // into tokens and tokenize the non-raw pieces.
    if (raw) {
      auto [left, right] = *raw;

      // Treat the raw delimiters also as regular delimiters.
      delimiters_.insert(left);
      delimiters_.insert(right);

      size_t start = input.find(left), end = 0;
      while (start != string::npos) {
        Tokenize(input.substr(end, start - end), whitespace);
        tokens_.push_back(left);
        start += left.size();
        end = input.find(right, start);
        CHECK_NE(end, string::npos)
            << "Left raw delimiter unmatched by right raw delimiter";
        tokens_.push_back(input.substr(start, end - start));
        tokens_.push_back(right);
        end += right.size();
        start = input.find(left, end);
      }
      Tokenize(input.substr(end), whitespace);
    } else {
      Tokenize(input, whitespace);
    }

    // Reverse so that the beginning of the input is at the end of tokens_.
    std::reverse(tokens_.begin(), tokens_.end());
  }

  // Confirms that the next token is 'str' and consumes it; FATALs if the next
  // token is not 'str'.
  void Consume(const string& str) {
    string token = ConsumeNextToken();
    CHECK_EQ(token, str) << ErrorMessage(string("unexpected token ") + token);
  }

  // Returns whether the next token is 'str' and consumes it if so.
  bool QueryConsume(const string& str) {
    string token = ReturnNextToken();
    if (token == str) {
      ConsumeNextToken();
      return true;
    }
    return false;
  }

  // Returns whether the next token is 'str'; doesn't consume it either way.
  bool QueryNoConsume(const string& str) { return ReturnNextToken() == str; }

  // Consumes and returns the next token; FATALs if that token is a delimiter or
  // reserved word or if we're at the end of the input.
  string ConsumeToken() {
    string token = ConsumeNextToken();
    CHECK(!delimiters_.count(token) && !reserved_words_.count(token))
        << ErrorMessage(string("read delimiter or reserved word: ") + token);
    return token;
  }

  // Acts like ConsumeToken() except that it doesn't check the contents of the
  // token against whitespace, delimiters, or reserved words; also makes sure to
  // track line numbers correctly even if the raw token contains one or more
  // newlines. This is the preferred way to extract raw tokens, otherwise weird
  // problems may happen.
  string ConsumeRaw() {
    CHECK(!tokens_.empty()) << ErrorMessage("unexpected end of input");
    string token = tokens_.back();
    tokens_.pop_back();
    line_number_ += std::count(token.begin(), token.end(), '\n');
    return token;
  }

  // Consumes and returns the next character; FATALS if that token is a
  // delimiter or reserved word or if we're at the end of the input.
  char ConsumeChar() {
    string token = ReturnNextToken();
    CHECK_NE(token, "") << ErrorMessage("unexpected end of input");

    char retval = token[0];
    CHECK(!delimiters_.count(string(1, retval)) &&
          !reserved_words_.count(string(1, retval)))
        << ErrorMessage(string("read delimiter or reserved word: ") + token);

    tokens_.pop_back();
    if (token.size() > 1) {
      tokens_.push_back(token.substr(1));
    }

    return retval;
  }

  // Returns whether the next token is reserved or a delimiter.
  bool IsNextReserved() {
    auto token = Peek(0);
    return delimiters_.count(token) || reserved_words_.count(token);
  }

  // Put a token onto the token stream; it will be the next token to be read.
  void Put(const string& token) { tokens_.push_back(token); }

  // Return the token in 'ahead' position from the beginning of the stream
  // (starting with 0). If 'ahead' exceeds the number of remaining tokens
  // returns the empty string.
  string Peek(int ahead) {
    int idx = tokens_.size() - 1;

    auto skip = [&]() {
      if (!newline_is_whitespace_) return;
      while (idx >= 0 && tokens_[idx] == "\n") idx--;
    };

    skip();
    while (ahead) {
      idx--;
      ahead--;
      skip();
    }

    if (idx < 0) return "";
    return tokens_[idx];
  }

  // Returns whether we've reached the end of the input or not.
  bool EndOfInput() { return ReturnNextToken() == ""; }

 private:
  // Returns the next token without consuming it; returns empty string if we're
  // at the end of the input.
  string ReturnNextToken() {
    if (newline_is_whitespace_) {
      while (!tokens_.empty() && tokens_.back() == "\n") {
        line_number_++;
        tokens_.pop_back();
      }
    }

    if (tokens_.empty()) return "";
    return tokens_.back();
  }

  // Consumes and returns the next token; FATALs if we're at the end of the
  // input.
  string ConsumeNextToken() {
    string token = ReturnNextToken();
    CHECK_NE(token, "") << ErrorMessage("unexpected end of input");

    if (token == "\n") line_number_++;
    tokens_.pop_back();
    return token;
  }

  string ErrorMessage(const string& err) {
    return string("Syntax error on line ") + std::to_string(line_number_) +
           ": " + err;
  }

  void Tokenize(const string& str, const set<char>& whitespace) {
    // During tokenization '\n' is not considered
    // whitespace (so that we can keep track of line numbers during parsing).
    string space;
    for (char symbol : whitespace) {
      if (symbol != '\n') space.push_back(symbol);
    }

    // Tokenize based on whitespace and delimiters.
    size_t start = str.find_first_not_of(space), end = 0;
    while (start != string::npos && end != string::npos) {
      end = str.find_first_of(space, start);
      string seq = str.substr(start, end - start);
      DelimitAndAddTokens(seq);
      if (end != string::npos) {
        start = str.find_first_not_of(space, end);
      }
    }
  }

  // Separates 'str' into pieces based on delimiters_ and adds them to tokens_.
  void DelimitAndAddTokens(const string& str) {
    // We sort the delimiters by size to guarantee that if delimiter A is a
    // prefix of delimiter B, then B is processed before A is processed.
    vector<string> delimiters(delimiters_.begin(), delimiters_.end());
    std::sort(delimiters.begin(), delimiters.end(),
              [](string& s1, string& s2) { return s1.size() > s2.size(); });

    // Find minimum position of any delimiter.
    const auto find_min = [&](const string& str) {
      size_t min_pos = string::npos;
      int length = 0;
      for (const auto& delimit : delimiters_) {
        auto pos = str.find(delimit);
        if (pos < min_pos) {
          min_pos = pos;
          length = delimit.size();
        }
      }
      return pair<size_t, int>{min_pos, length};
    };

    string token = str;
    CHECK_NE(token, "") << "Empty token";

    while (true) {
      auto [pos, length] = find_min(token);
      if (pos == string::npos) {
        if (token != "") tokens_.push_back(token);
        break;
      }
      if (pos != 0) tokens_.push_back(token.substr(0, pos));
      tokens_.push_back(token.substr(pos, length));
      token = token.substr(pos + length);
    }
  }

  // The tokenized input, in reverse (the back of the vector is the front of the
  // input). '\n' is always considered a delimiter even if it's specified as
  // whitespace; '\n' being whitespace is handled specially by ReturnNextToken()
  // and ConsumeNextToken().
  vector<string> tokens_;

  // The current line number within the input being parsed.
  int line_number_ = 1;

  // Special strings: delimiters are always individual tokens; delimiters and
  // reserved words cause an error if they are consumed by ConsumeToken().
  set<string> delimiters_;
  set<string> reserved_words_;

  // Remembers whether '\n' should be considered a whitespace character.
  bool newline_is_whitespace_ = false;
};

}  // namespace util
