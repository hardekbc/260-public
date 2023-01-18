// Tests for the tokenizer implementation.

#include "tokenizer.h"

#include <gtest/gtest.h>

namespace {

using namespace util;

TEST(TokenizerTest, Test1) {
  auto tk = Tokenizer("a aa aaa aaaa", {}, {}, {});

  EXPECT_FALSE(tk.QueryConsume("a"));
  EXPECT_FALSE(tk.QueryNoConsume("a"));
  EXPECT_FALSE(tk.EndOfInput());
  EXPECT_TRUE(tk.QueryNoConsume("a aa aaa aaaa"));
  EXPECT_TRUE(tk.QueryConsume("a aa aaa aaaa"));
  EXPECT_TRUE(tk.EndOfInput());

  tk = Tokenizer("a aa aaa aaaa", {}, {}, {});

  EXPECT_NO_FATAL_FAILURE(tk.Consume("a aa aaa aaaa"));

  tk = Tokenizer("a aa aaa aaaa", {}, {}, {});

  EXPECT_TRUE(tk.QueryConsume("a aa aaa aaaa"));

  tk = Tokenizer("a aa aaa aaaa", {}, {}, {});

  EXPECT_EQ(tk.ConsumeToken(), "a aa aaa aaaa");
}

TEST(TokenizerTest, Test2) {
  auto tk = Tokenizer("a aa aaa aaaa", {' '}, {}, {});

  EXPECT_TRUE(tk.QueryConsume("a"));
  EXPECT_FALSE(tk.QueryNoConsume("a"));
  EXPECT_TRUE(tk.QueryNoConsume("aa"));
  EXPECT_NO_FATAL_FAILURE(tk.Consume("aa"));
  EXPECT_EQ(tk.ConsumeToken(), "aaa");
  EXPECT_FALSE(tk.EndOfInput());
}

TEST(TokenizerTest, Test3) {
  auto tk = Tokenizer("a a,a aaa a,aa,a", {' '}, {","}, {});

  EXPECT_TRUE(tk.QueryConsume("a"));
  EXPECT_TRUE(tk.QueryConsume("a"));
  EXPECT_TRUE(tk.QueryConsume(","));
  EXPECT_TRUE(tk.QueryConsume("a"));
  EXPECT_TRUE(tk.QueryConsume("aaa"));
  EXPECT_TRUE(tk.QueryConsume("a"));
  EXPECT_TRUE(tk.QueryConsume(","));
  EXPECT_TRUE(tk.QueryConsume("aa"));
  EXPECT_TRUE(tk.QueryConsume(","));
  EXPECT_TRUE(tk.QueryConsume("a"));
}

TEST(TokenizerTest, Test4) {
  auto tk = Tokenizer("a \na,a a\naa \na,aa,a", {' '}, {","}, {});

  EXPECT_TRUE(tk.QueryConsume("a"));
  EXPECT_TRUE(tk.QueryConsume("\n"));
  EXPECT_TRUE(tk.QueryConsume("a"));
  EXPECT_TRUE(tk.QueryConsume(","));
  EXPECT_TRUE(tk.QueryConsume("a"));
  EXPECT_TRUE(tk.QueryConsume("a"));
  EXPECT_TRUE(tk.QueryConsume("\n"));
  EXPECT_TRUE(tk.QueryConsume("aa"));
  EXPECT_TRUE(tk.QueryConsume("\n"));
  EXPECT_TRUE(tk.QueryConsume("a"));
  EXPECT_TRUE(tk.QueryConsume(","));
  EXPECT_TRUE(tk.QueryConsume("aa"));
  EXPECT_TRUE(tk.QueryConsume(","));
  EXPECT_TRUE(tk.QueryConsume("a"));
}

TEST(TokenizerTest, Test5) {
  auto tk = Tokenizer("a \na,a a\naa \na,aa,a", {' ', '\n'}, {","}, {});

  EXPECT_TRUE(tk.QueryConsume("a"));
  EXPECT_TRUE(tk.QueryConsume("a"));
  EXPECT_TRUE(tk.QueryConsume(","));
  EXPECT_TRUE(tk.QueryConsume("a"));
  EXPECT_TRUE(tk.QueryConsume("a"));
  EXPECT_TRUE(tk.QueryConsume("aa"));
  EXPECT_TRUE(tk.QueryConsume("a"));
  EXPECT_TRUE(tk.QueryConsume(","));
  EXPECT_TRUE(tk.QueryConsume("aa"));
  EXPECT_TRUE(tk.QueryConsume(","));
  EXPECT_TRUE(tk.QueryConsume("a"));
}

TEST(TokenizerTest, Test6) {
  auto tk = Tokenizer("a \na,a a\naa \na,aa,a", {' ', '\n'}, {","}, {});

  EXPECT_EQ(tk.ConsumeChar(), 'a');
  EXPECT_EQ(tk.ConsumeChar(), 'a');
  EXPECT_NO_FATAL_FAILURE(tk.Consume(","));
  EXPECT_EQ(tk.ConsumeChar(), 'a');
  EXPECT_EQ(tk.ConsumeChar(), 'a');
  EXPECT_EQ(tk.ConsumeChar(), 'a');
  EXPECT_EQ(tk.ConsumeChar(), 'a');
  EXPECT_EQ(tk.ConsumeChar(), 'a');
  EXPECT_NO_FATAL_FAILURE(tk.Consume(","));
  EXPECT_EQ(tk.ConsumeChar(), 'a');
  EXPECT_EQ(tk.ConsumeChar(), 'a');
  EXPECT_NO_FATAL_FAILURE(tk.Consume(","));
  EXPECT_EQ(tk.ConsumeChar(), 'a');
  EXPECT_TRUE(tk.EndOfInput());
}

TEST(TokenizerTest, Test7) {
  auto tk = Tokenizer("a[a,a\n a]a , a[a,,a] a ,[\n] a [a,a]a", {' ', '\n'},
                      {","}, {}, pair<string, string>{"[", "]"});

  EXPECT_EQ(tk.ConsumeToken(), "a");
  EXPECT_NO_FATAL_FAILURE(tk.Consume("["));
  EXPECT_EQ(tk.ConsumeRaw(), "a,a\n a");
  EXPECT_NO_FATAL_FAILURE(tk.Consume("]"));
  EXPECT_EQ(tk.ConsumeToken(), "a");
  EXPECT_NO_FATAL_FAILURE(tk.Consume(","));
  EXPECT_EQ(tk.ConsumeToken(), "a");
  EXPECT_NO_FATAL_FAILURE(tk.Consume("["));
  EXPECT_EQ(tk.ConsumeRaw(), "a,,a");
  EXPECT_NO_FATAL_FAILURE(tk.Consume("]"));
  EXPECT_EQ(tk.ConsumeToken(), "a");
  EXPECT_NO_FATAL_FAILURE(tk.Consume(","));
  EXPECT_NO_FATAL_FAILURE(tk.Consume("["));
  EXPECT_EQ(tk.ConsumeRaw(), "\n");
  EXPECT_NO_FATAL_FAILURE(tk.Consume("]"));
  EXPECT_EQ(tk.ConsumeToken(), "a");
  EXPECT_NO_FATAL_FAILURE(tk.Consume("["));
  EXPECT_EQ(tk.ConsumeRaw(), "a,a");
  EXPECT_NO_FATAL_FAILURE(tk.Consume("]"));
  EXPECT_EQ(tk.ConsumeToken(), "a");
  EXPECT_TRUE(tk.EndOfInput());
}

TEST(TokenizerTest, Test8) {
  auto tk = Tokenizer("a[[a,a\n a]]a , a[[a,,a]] a ,[[\n]] a [[a,a]]a",
                      {' ', '\n'}, {","}, {}, pair<string, string>{"[[", "]]"});

  EXPECT_EQ(tk.ConsumeToken(), "a");
  EXPECT_NO_FATAL_FAILURE(tk.Consume("[["));
  EXPECT_EQ(tk.ConsumeRaw(), "a,a\n a");
  EXPECT_NO_FATAL_FAILURE(tk.Consume("]]"));
  EXPECT_EQ(tk.ConsumeToken(), "a");
  EXPECT_NO_FATAL_FAILURE(tk.Consume(","));
  EXPECT_EQ(tk.ConsumeToken(), "a");
  EXPECT_NO_FATAL_FAILURE(tk.Consume("[["));
  EXPECT_EQ(tk.ConsumeRaw(), "a,,a");
  EXPECT_NO_FATAL_FAILURE(tk.Consume("]]"));
  EXPECT_EQ(tk.ConsumeToken(), "a");
  EXPECT_NO_FATAL_FAILURE(tk.Consume(","));
  EXPECT_NO_FATAL_FAILURE(tk.Consume("[["));
  EXPECT_EQ(tk.ConsumeRaw(), "\n");
  EXPECT_NO_FATAL_FAILURE(tk.Consume("]]"));
  EXPECT_EQ(tk.ConsumeToken(), "a");
  EXPECT_NO_FATAL_FAILURE(tk.Consume("[["));
  EXPECT_EQ(tk.ConsumeRaw(), "a,a");
  EXPECT_NO_FATAL_FAILURE(tk.Consume("]]"));
  EXPECT_EQ(tk.ConsumeToken(), "a");
  EXPECT_TRUE(tk.EndOfInput());
}

TEST(TokenizerTest, Test9) {
  auto tk = Tokenizer("[a,a\n a][a,,a]", {' ', '\n'}, {","}, {},
                      pair<string, string>{"[", "]"});

  EXPECT_NO_FATAL_FAILURE(tk.Consume("["));
  EXPECT_EQ(tk.ConsumeRaw(), "a,a\n a");
  EXPECT_NO_FATAL_FAILURE(tk.Consume("]"));
  EXPECT_NO_FATAL_FAILURE(tk.Consume("["));
  EXPECT_EQ(tk.ConsumeRaw(), "a,,a");
  EXPECT_NO_FATAL_FAILURE(tk.Consume("]"));
  EXPECT_TRUE(tk.EndOfInput());
}

TEST(TokenizerTest, Test10) {
  auto tk = Tokenizer("[[a,a\n a]][[a,,a]]", {' ', '\n'}, {","}, {},
                      pair<string, string>{"[[", "]]"});

  EXPECT_NO_FATAL_FAILURE(tk.Consume("[["));
  EXPECT_EQ(tk.ConsumeRaw(), "a,a\n a");
  EXPECT_NO_FATAL_FAILURE(tk.Consume("]]"));
  EXPECT_NO_FATAL_FAILURE(tk.Consume("[["));
  EXPECT_EQ(tk.ConsumeRaw(), "a,,a");
  EXPECT_NO_FATAL_FAILURE(tk.Consume("]]"));
  EXPECT_TRUE(tk.EndOfInput());
}

TEST(TokenizerTest, Test11) {
  auto tk = Tokenizer("|a,a\n a||a,,a|", {' ', '\n'}, {","}, {},
                      pair<string, string>{"|", "|"});

  EXPECT_NO_FATAL_FAILURE(tk.Consume("|"));
  EXPECT_EQ(tk.ConsumeRaw(), "a,a\n a");
  EXPECT_NO_FATAL_FAILURE(tk.Consume("|"));
  EXPECT_NO_FATAL_FAILURE(tk.Consume("|"));
  EXPECT_EQ(tk.ConsumeRaw(), "a,,a");
  EXPECT_NO_FATAL_FAILURE(tk.Consume("|"));
  EXPECT_TRUE(tk.EndOfInput());
}

TEST(TokenizerTest, Test12) {
  auto tk = Tokenizer("reserved notreserved;", {' ', '\n'}, {";"}, {"reserved"});

  EXPECT_TRUE(tk.IsNextReserved());
  EXPECT_NO_FATAL_FAILURE(tk.Consume("reserved"));
  EXPECT_FALSE(tk.IsNextReserved());
  EXPECT_NO_FATAL_FAILURE(tk.ConsumeToken());
  EXPECT_TRUE(tk.IsNextReserved());
  EXPECT_NO_FATAL_FAILURE(tk.Consume(";"));
  EXPECT_FALSE(tk.IsNextReserved());
}

TEST(TokenizerTest, Test13) {
  auto tk = Tokenizer("a\nb c\n\nd\n", {' ', '\n'}, {}, {"b", "d"});

  EXPECT_EQ(tk.Peek(0), "a");
  EXPECT_EQ(tk.Peek(1), "b");
  EXPECT_EQ(tk.Peek(2), "c");
  EXPECT_EQ(tk.Peek(3), "d");
  EXPECT_EQ(tk.Peek(4), "");
}

TEST(TokenizerTest, Test14) {
  auto tk = Tokenizer("a b", {' ', '\n'}, {}, {});

  EXPECT_TRUE(tk.QueryNoConsume("a"));
  tk.Put("c");
  EXPECT_TRUE(tk.QueryConsume("c"));
  EXPECT_TRUE(tk.QueryConsume("a"));
  EXPECT_TRUE(tk.QueryConsume("b"));
  EXPECT_TRUE(tk.EndOfInput());
}

TEST(TokenizerDeathTest, BadConsume) {
  auto tk = Tokenizer("a aa aaa aaaa", {' '}, {}, {});
  EXPECT_DEATH(tk.Consume("aa"), "unexpected token");
}

TEST(TokenizerDeathTest, ReservedToken) {
  auto tk = Tokenizer("a aa aaa aaaa", {' '}, {}, {"aa"});
  EXPECT_TRUE(tk.QueryConsume("a"));
  EXPECT_DEATH(tk.ConsumeToken(), "read delimiter or reserved word");
}

TEST(TokenizerDeathTest, LineNumbers) {
  auto tk = Tokenizer("a \na,a a\naa \na,aa,a", {' ', '\n'}, {","}, {});

  EXPECT_TRUE(tk.QueryConsume("a"));
  EXPECT_TRUE(tk.QueryConsume("a"));
  EXPECT_TRUE(tk.QueryConsume(","));
  EXPECT_TRUE(tk.QueryConsume("a"));
  EXPECT_TRUE(tk.QueryConsume("a"));
  EXPECT_TRUE(tk.QueryConsume("aa"));
  EXPECT_DEATH(tk.Consume("aa"), "line 4");
}

TEST(TokenizerDeathTest, UnmatchedRaw) {
  EXPECT_DEATH(Tokenizer("[a,a\n a][a,,a", {' ', '\n'}, {","}, {},
                         pair<string, string>{"[", "]"}),
               "unmatched");
}

}  // namespace

int main(int argc, char* argv[]) {
  ::google::InitGoogleLogging(argv[0]);
  ::testing::InitGoogleTest(&argc, argv);

  return RUN_ALL_TESTS();
}
