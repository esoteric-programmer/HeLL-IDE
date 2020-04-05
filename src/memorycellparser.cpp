/*
 * This file is part of HeLL IDE, IDE for the low-level Malbolge
 * assembly language HeLL.
 * Copyright (C) 2013 Matthias Ernst
 *
 * HeLL IDE is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * HeLL IDE is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "memorycellparser.h"

#include "qabstractmalbolgerunner.h"
#include <limits.h>
#include <stdexcept>

#define SYNTAX_ERROR std::runtime_error("syntax error");

// TODO: access registers; e.g. by keyword .A, .C, .D
// TODO: parse display type, e.g. by keyword .AS

QString MemoryCellParser::expression = QString();
LMAODebugInformations* MemoryCellParser::debug_info=0;
unsigned int* MemoryCellParser::malbolge_memory=0;
QList<QChar> MemoryCellParser::start_label_chars;
QList<QChar> MemoryCellParser::label_chars;

/*
public:
typedef struct DisplayType {
	enum Type { TRINARY, DECIMAL, LABEL, STRINGPTR } type;
	int param;
} DisplayType;
*/
/*
grammar:
expression = shift | shift " as " display
display = type | type "(" shift ")"

shift = crazy | shift "<<" crazy | shift ">>" crazy;
crazy = sum | crazy ! sum;
sum =  product | sum "+" product | sum "-" product ;
product = term | product "*" term | product "/" term ;
term = - term | "(" sum ")" | number ;
number = label | "[" shift "]" | trinary | decimal | constant
*/


bool MemoryCellParser::parse_expression(QString expression, unsigned int* malbolge_memory, LMAODebugInformations* debug_info, int &result, DisplayType &display) {
	if (malbolge_memory == 0 || debug_info == 0)
		return false;
    if (start_label_chars.isEmpty()) {
        for (unsigned char i='A';i<='Z';i++) {
            start_label_chars.append(QChar::fromLatin1(i));
        }
        for (unsigned char i='a';i<='z';i++) {
            start_label_chars.append(QChar::fromLatin1(i));
        }
        start_label_chars.append(QChar::fromLatin1('_'));
    }
    if (label_chars.isEmpty()) {
        label_chars = QList<QChar>(start_label_chars);
        for (unsigned char i='0';i<='9';i++) {
            label_chars.append(QChar::fromLatin1(i));
        }
    }
    try {
        MemoryCellParser::expression = expression;
        MemoryCellParser::malbolge_memory = malbolge_memory;
        MemoryCellParser::debug_info = debug_info;
        skip_blanks();
        if (!get_shift(result)) {
            MemoryCellParser::expression = QString();
            MemoryCellParser::malbolge_memory = 0;
            MemoryCellParser::debug_info = 0;
            return false;
        }
        result = to_malbolge_range(result);
        skip_blanks();
        display.type = DisplayType::UNSET;
        display.param = INT_MIN;
        if (!get_display_separator()) {
            bool empty = MemoryCellParser::expression.isEmpty();
            MemoryCellParser::expression = QString();
            MemoryCellParser::malbolge_memory = 0;
            MemoryCellParser::debug_info = 0;
            return empty;
        }
        if (!get_display(display)) {
            MemoryCellParser::expression = QString();
            MemoryCellParser::malbolge_memory = 0;
            MemoryCellParser::debug_info = 0;
            return false;
        }
        skip_blanks();
        bool empty = MemoryCellParser::expression.isEmpty();
        MemoryCellParser::expression = QString();
        MemoryCellParser::malbolge_memory = 0;
        MemoryCellParser::debug_info = 0;
        return empty;
    } catch (const std::runtime_error &) {
        MemoryCellParser::expression = QString();
        MemoryCellParser::malbolge_memory = 0;
        MemoryCellParser::debug_info = 0;
        return false;
    }
}



bool MemoryCellParser::get_label(int &result_address) {
    skip_blanks();
    bool r_prefix = get_r_prefix();
    if (expression.length() <= 0) {
        if (r_prefix) {
            throw SYNTAX_ERROR;
        }
        return false;
    }
    QString result_label = QString();

    bool is_label = false;
    if (MemoryCellParser::start_label_chars.contains(MemoryCellParser::expression.at(0))) {
        is_label = true;
    }else if (MemoryCellParser::label_chars.contains(MemoryCellParser::expression.at(0)) && r_prefix){
        r_prefix = false;
        result_label = QString("R_");
        is_label = true;
    }
    if (is_label) {
        int label_length=0;
        while (expression.length() > label_length && MemoryCellParser::label_chars.contains(MemoryCellParser::expression.at(label_length))) {
            label_length++;
        }
        result_label = result_label + expression.left(label_length);
        expression = expression.right(expression.length() - label_length);
    }else{
        if (r_prefix) {
            throw SYNTAX_ERROR;
        }
        return false;
    }
    //[A-Za-z_][0-9A-Za-z_]{0,99}

    // check if character in _A-Za-z0-9, first char not in 0-9
    // copy those characters to result_label;
    // use lmaodebuginfos...

    int address = MemoryCellParser::debug_info->get_label_address(result_label);
    if (address < 0) {
        if (r_prefix) {
            throw SYNTAX_ERROR;
        }
        return false;
    }
    result_address = address;
    if (r_prefix)
        result_address++;
    result_address = MemoryCellParser::to_malbolge_range(result_address);
    return true;
}

bool MemoryCellParser::get_r_prefix() {
    if (expression.length() < 2)
        return false;
    if (expression.at(0).toLatin1() == 'R' && expression.at(1).toLatin1() == '_') {
        expression = expression.right(expression.length() - 2);
        return true;
    }
    return false;
}

bool MemoryCellParser::get_display_separator() {
    // TODO
    return false;
}

bool MemoryCellParser::get_display_type(enum DisplayType::Type&) {
    // TODO
    return false;
}




bool MemoryCellParser::get_character(QChar chr) {
	if (expression.at(0) == chr) {
		expression = expression.right(expression.size()-1);
		return true;
	}
	return false;
}

bool MemoryCellParser::skip_blanks() {
	bool result = false;
	while(get_character(' ') || get_character('\t') || get_character('\n') || get_character('\r') || get_character('\f'))
		result = true;
	return result;
}


bool MemoryCellParser::get_decimal(int &result) {
    int digits = 0;
    while(expression.length() > digits && expression.at(digits).toLatin1() >= '0' && expression.at(digits).toLatin1() <= '9') {
        digits++;
    }
    if (digits > 0 && (expression.length() == digits || expression.at(digits).toLatin1() == ' ' || expression.at(digits).toLatin1() == '\n' || expression.at(digits).toLatin1() == '\r' || expression.at(digits).toLatin1() == '\t' || expression.at(digits).toLatin1() == '\f'
                        || expression.at(digits).toLatin1() == '+' || expression.at(digits).toLatin1() == '-' || expression.at(digits).toLatin1() == '*' || expression.at(digits).toLatin1() == '/' || expression.at(digits).toLatin1() == ')' || expression.at(digits).toLatin1() == '<' || expression.at(digits).toLatin1() == '>'
                        || expression.at(digits).toLatin1() == '!' || expression.at(digits).toLatin1() == ']')) {
        QString number = expression.left(digits);
        expression = expression.right(expression.length() - digits);
        result = 0;
        result = number.toInt();
        return true;
    }
	return false;
}

bool MemoryCellParser::get_trinary(int &result) {
    if (expression.length() < 3)
        return false;
    if (expression.at(0).toLatin1() != '0' && expression.at(1).toLatin1() != 't')
        return false;
    int digits = 2;
    while(expression.length() > digits && expression.at(digits).toLatin1() >= '0' && expression.at(digits).toLatin1() <= '2') {
        digits++;
    }
    if (digits > 0 && (expression.length() == digits || expression.at(digits).toLatin1() == ' ' || expression.at(digits).toLatin1() == '\n' || expression.at(digits).toLatin1() == '\r' || expression.at(digits).toLatin1() == '\t' || expression.at(digits).toLatin1() == '\f'
                        || expression.at(digits).toLatin1() == '+' || expression.at(digits).toLatin1() == '-' || expression.at(digits).toLatin1() == '*' || expression.at(digits).toLatin1() == '/' || expression.at(digits).toLatin1() == ')' || expression.at(digits).toLatin1() == '<' || expression.at(digits).toLatin1() == '>'
                        || expression.at(digits).toLatin1() == '!' || expression.at(digits).toLatin1() == ']')) {
        QString number = expression.left(digits);
        number = number.right(number.length()-2);
        expression = expression.right(expression.length() - digits);
        result = 0;
        result = number.toInt(0,3);
        return true;
    }
    return false;
}

bool MemoryCellParser::get_constant(int &result) {
    // check for C0, C1, C20, C21, C2, EOF
    // ugly code...
    if (expression.length() < 2)
        return false;
    int digits=2;
    if (expression.at(0).toLatin1() == 'C' && expression.at(1).toLatin1() == '0' && (expression.length() == digits || expression.at(digits).toLatin1() == ' ' || expression.at(digits).toLatin1() == '\n' || expression.at(digits).toLatin1() == '\r' || expression.at(digits).toLatin1() == '\t' || expression.at(digits).toLatin1() == '\f'
                        || expression.at(digits).toLatin1() == '+' || expression.at(digits).toLatin1() == '-' || expression.at(digits).toLatin1() == '*' || expression.at(digits).toLatin1() == '/' || expression.at(digits).toLatin1() == ')' || expression.at(digits).toLatin1() == '<' || expression.at(digits).toLatin1() == '>'
                        || expression.at(digits).toLatin1() == '!' || expression.at(digits).toLatin1() == ']')) {
        result = 0;
        expression = expression.right(expression.length() - digits);
        return true;
    }
    if (expression.at(0).toLatin1() == 'C' && expression.at(1).toLatin1() == '1' && (expression.length() == digits || expression.at(digits).toLatin1() == ' ' || expression.at(digits).toLatin1() == '\n' || expression.at(digits).toLatin1() == '\r' || expression.at(digits).toLatin1() == '\t' || expression.at(digits).toLatin1() == '\f'
                        || expression.at(digits).toLatin1() == '+' || expression.at(digits).toLatin1() == '-' || expression.at(digits).toLatin1() == '*' || expression.at(digits).toLatin1() == '/' || expression.at(digits).toLatin1() == ')' || expression.at(digits).toLatin1() == '<' || expression.at(digits).toLatin1() == '>'
                        || expression.at(digits).toLatin1() == '!' || expression.at(digits).toLatin1() == ']')) {
        result = 29524;
        expression = expression.right(expression.length() - digits);
        return true;
    }
    if (expression.at(0).toLatin1() == 'C' && expression.at(1).toLatin1() == '2' && (expression.length() == digits || expression.at(digits).toLatin1() == ' ' || expression.at(digits).toLatin1() == '\n' || expression.at(digits).toLatin1() == '\r' || expression.at(digits).toLatin1() == '\t' || expression.at(digits).toLatin1() == '\f'
                        || expression.at(digits).toLatin1() == '+' || expression.at(digits).toLatin1() == '-' || expression.at(digits).toLatin1() == '*' || expression.at(digits).toLatin1() == '/' || expression.at(digits).toLatin1() == ')' || expression.at(digits).toLatin1() == '<' || expression.at(digits).toLatin1() == '>'
                        || expression.at(digits).toLatin1() == '!' || expression.at(digits).toLatin1() == ']')) {
        result = 59048;
        expression = expression.right(expression.length() - digits);
        return true;
    }
    digits=3;
    if (expression.at(0).toLatin1() == 'C' && expression.at(1).toLatin1() == '2' && expression.at(2).toLatin1() == '0' && (expression.length() == digits || expression.at(digits).toLatin1() == ' ' || expression.at(digits).toLatin1() == '\n' || expression.at(digits).toLatin1() == '\r' || expression.at(digits).toLatin1() == '\t' || expression.at(digits).toLatin1() == '\f'
                        || expression.at(digits).toLatin1() == '+' || expression.at(digits).toLatin1() == '-' || expression.at(digits).toLatin1() == '*' || expression.at(digits).toLatin1() == '/' || expression.at(digits).toLatin1() == ')' || expression.at(digits).toLatin1() == '<' || expression.at(digits).toLatin1() == '>'
                        || expression.at(digits).toLatin1() == '!' || expression.at(digits).toLatin1() == ']')) {
        result = 59048-2;
        expression = expression.right(expression.length() - digits);
        return true;
    }
    if (expression.at(0).toLatin1() == 'C' && expression.at(1).toLatin1() == '2' && expression.at(2).toLatin1() == '1' && (expression.length() == digits || expression.at(digits).toLatin1() == ' ' || expression.at(digits).toLatin1() == '\n' || expression.at(digits).toLatin1() == '\r' || expression.at(digits).toLatin1() == '\t' || expression.at(digits).toLatin1() == '\f'
                        || expression.at(digits).toLatin1() == '+' || expression.at(digits).toLatin1() == '-' || expression.at(digits).toLatin1() == '*' || expression.at(digits).toLatin1() == '/' || expression.at(digits).toLatin1() == ')' || expression.at(digits).toLatin1() == '<' || expression.at(digits).toLatin1() == '>'
                        || expression.at(digits).toLatin1() == '!' || expression.at(digits).toLatin1() == ']')) {
        result = 59048-1;
        expression = expression.right(expression.length() - digits);
        return true;
    }
    if (expression.at(0).toLatin1() == 'E' && expression.at(1).toLatin1() == 'O' && expression.at(2).toLatin1() == 'F' && (expression.length() == digits || expression.at(digits).toLatin1() == ' ' || expression.at(digits).toLatin1() == '\n' || expression.at(digits).toLatin1() == '\r' || expression.at(digits).toLatin1() == '\t' || expression.at(digits).toLatin1() == '\f'
                        || expression.at(digits).toLatin1() == '+' || expression.at(digits).toLatin1() == '-' || expression.at(digits).toLatin1() == '*' || expression.at(digits).toLatin1() == '/' || expression.at(digits).toLatin1() == ')' || expression.at(digits).toLatin1() == '<' || expression.at(digits).toLatin1() == '>'
                        || expression.at(digits).toLatin1() == '!' || expression.at(digits).toLatin1() == ']')) {
        result = 59048;
        expression = expression.right(expression.length() - digits);
        return true;
    }
	return false;
}

bool MemoryCellParser::get_rotate_operator(bool& result_rotate_right) {
    if (expression.length() < 2)
        return false;
    if (expression.at(0).toLatin1() == '<' && expression.at(1).toLatin1() == '<') {
        result_rotate_right = false;
        expression = expression.right(expression.length() - 2);
        return true;
    }
    if (expression.at(0).toLatin1() == '>' && expression.at(1).toLatin1() == '>') {
        result_rotate_right = true;
        expression = expression.right(expression.length() - 2);
        return true;
    }
	return false;
}

bool MemoryCellParser::get_display_param(int &result_param) {
	skip_blanks();
	if (!get_character('('))
		return false;
	skip_blanks();
	if (!get_shift(result_param))
		throw SYNTAX_ERROR;
	skip_blanks();
	if (!get_character(')'))
		throw SYNTAX_ERROR;
	return true;
}

bool MemoryCellParser::get_display(DisplayType &result) {
	skip_blanks();
	if (!get_display_type(result.type))
		return false;
	result.param = INT_MIN;
	get_display_param(result.param);
	return true;
}

bool MemoryCellParser::get_plusminus(bool &minus) {
    if (expression.length() < 1)
        return false;
    if (expression.at(0).toLatin1() == '+') {
        minus = false;
        expression = expression.right(expression.length() - 1);
        return true;
    }
    if (expression.at(0).toLatin1() == '-') {
        minus = true;
        expression = expression.right(expression.length() - 1);
        return true;
    }
    return false;
}

bool MemoryCellParser::get_muldiv(bool &div) {
    if (expression.length() < 1)
        return false;
    if (expression.at(0).toLatin1() == '*') {
        div = false;
        expression = expression.right(expression.length() - 1);
        return true;
    }
    if (expression.at(0).toLatin1() == '/') {
        div = true;
        expression = expression.right(expression.length() - 1);
        return true;
    }
    return false;
}


int MemoryCellParser::to_malbolge_range(int number) {
	if (number < 0)
		return (59049-((-number)%59049))%59049;
	return number%59049;
}


bool MemoryCellParser::get_shift(int &result) {
	// shift = crazy | shift "<<" crazy | shift ">>" crazy;
	skip_blanks();
	if (!get_crazy(result))
		return false;
	while (1) {
		bool rot_right;
		skip_blanks();
		if (!get_rotate_operator(rot_right))
			return true;
		skip_blanks();
		int res2;
		if (!get_crazy(res2))
			throw SYNTAX_ERROR;
		if (res2 < 0) {
			rot_right = !rot_right;
			res2 = -res2;
		}
		res2 %= 10;
		if (!rot_right) {
			res2 = 10-res2;
		}	
		for (int i=0; i<res2; i++) {
			result = QAbstractMalbolgeRunner::rotateR(to_malbolge_range(result));
		}
	}
}

bool MemoryCellParser::get_crazy(int &result) {
	skip_blanks();
	if (!get_sum(result))
		return false;
	while (1) {
		skip_blanks();
		if (!get_character('!'))
			return true;
		skip_blanks();
		int res2;
		if (!get_sum(res2))
			throw SYNTAX_ERROR;
		result = QAbstractMalbolgeRunner::crazy(to_malbolge_range(result), to_malbolge_range(res2));
	}
}

bool MemoryCellParser::get_sum(int &result) {
	skip_blanks();
	if (!get_product(result))
		return false;
	while (1) {
		bool minus;
		skip_blanks();
		if (!get_plusminus(minus))
			return true;
		skip_blanks();
		int res2;
        if (!get_product(res2))
			throw SYNTAX_ERROR;
		if (minus)
			result = result - res2;
		else
			result = result + res2;
	}
}

bool MemoryCellParser::get_product(int &result) {
	skip_blanks();
	if (!get_term(result))
		return false;
	while (1) {
		bool div;
		skip_blanks();
		if (!get_muldiv(div))
			return true;
		skip_blanks();
		int res2;
		if (!get_term(res2))
			throw SYNTAX_ERROR;
		if (div)
			result = result / res2;
		else
			result = result * res2;
	}
}

bool MemoryCellParser::get_term(int &result) {
	skip_blanks();
	if (get_character('-')) {
		bool res = get_term(result);
		if (res)
			result = -result;
		return res;
	}
	if (get_character('(')) {
		skip_blanks();
		if (!get_shift(result))
			throw SYNTAX_ERROR;
		skip_blanks();
		if (!get_character(')'))
			throw SYNTAX_ERROR;
		return true;
	}
	return get_mem_dest(result);
}
bool MemoryCellParser::get_mem_dest(int &result) {
	skip_blanks();
	if (get_character('[')) {
		skip_blanks();
		if (!get_shift(result))
			throw SYNTAX_ERROR;
		skip_blanks();
		if (!get_character(']'))
			throw SYNTAX_ERROR;
		result = malbolge_memory[to_malbolge_range(result)];
		return true;
	}
	return get_number(result);
}

bool MemoryCellParser::get_number(int &result) {
	if (get_constant(result))
		return true;
	if (get_decimal(result))
		return true;
	if (get_trinary(result))
		return true;
	if (get_label(result))
		return true;
	return false;
}


