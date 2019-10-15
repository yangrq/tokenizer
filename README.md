# A simple & header-only string tokenizer - based on regular expression

## usage
```C++
#include "tokenizer.hpp"
#include <iostream>

using std::string;
using std::cout;
using std::endl;

int main() {
	string s = "word +- 123.43 -13727\r\n \n 'w' \"\\\"\"  \"Test String\""; // a complex string ... hard to analyse ...

	enum user_token_type{
		ADD = 1,
		SUB,
		MUL,
		DIV
	};

	tokenizer lexer;

	lexer.add_builtin_token_type(NEWLINE);
	lexer.add_builtin_token_type(SPACE);
	lexer.add_builtin_token_type(IDENTIFIER);
	lexer.add_builtin_token_type(NUMBER_LITERAL);
	lexer.add_builtin_token_type(CHAR_LITERAL);
	lexer.add_builtin_token_type(STRING_LITERAL);
	lexer.add_token_type("\\+", ADD, "ADD");
	lexer.add_token_type("\\-", SUB, "SUB");
	lexer.assign(s);

	while (lexer.succeed(lexer.next()))
		cout << lexer.current_token_type_string() << endl;
	return 0;
}
```
```s
CONSOLE OUTPUT:

IDENTIFIER
SPACE
ADD
SUB
SPACE
NUMBER
SPACE
NUMBER
NEWLINE
SPACE
NEWLINE
SPACE
CHAR
SPACE
STRING
SPACE
STRING

```