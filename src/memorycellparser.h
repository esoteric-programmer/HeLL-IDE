/*
 * This file is part of HeLL IDE, IDE for the low-level Malbolge
 * assembly language HeLL.
 * Copyright (C) 2013 Matthias Lutter
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

#ifndef MEMORYCELLPARSER_INCLUDED
#define MEMORYCELLPARSER_INCLUDED

#include "lmaodebuginformations.h"


class MemoryCellParser {

public:
typedef struct DisplayType {
	enum Type { UNSET, TRINARY, DECIMAL, LABEL, STRINGPTR } type;
	int param;
} DisplayType;

/*
grammar:
expression = shift | shift " as " display
display = type | type "(" shift ")"

shift = crazy | shift "<<" crazy | shift ">>" crazy;
crazy = sum | crazy ! sum;
sum =  product | sum "+" product | sum "-" product ;
product = term | product "*" term | product "/" term ;
term = - term | "(" shift ")" | mem_dest ;
mem_dest = number | "[" shift "]" ;
number = label | trinary | decimal | constant
*/


public:
	static bool parse_expression(QString expression, unsigned int* malbolge_memory, LMAODebugInformations* debug_info, int &result, DisplayType &display);

private:
	static QString expression;
	static LMAODebugInformations* debug_info;
	static unsigned int* malbolge_memory;
    static QList<QChar> start_label_chars;
    static QList<QChar> label_chars;
	//int result;
	//QString last_label;
	//int last_number;
	//QChar last_char;
	//DisplayType display;
	
	static bool get_character(QChar chr);
	static bool get_label(int &result_address);
	static bool get_r_prefix();
	static bool get_decimal(int &result);
	static bool get_trinary(int &result);
	static bool get_constant(int &result);
	static bool get_rotate_operator(bool& result_rotate_right);
	static bool get_display_separator();
    static bool get_display_type(enum DisplayType::Type &);
	static bool get_plusminus(bool &minus);
	static bool get_muldiv(bool &div);
	
	static bool get_display_param(int &result_param);
	static bool get_display(DisplayType &result);
	
	static bool skip_blanks(); // return true iff at least one blank symbol has been skipped

	static bool get_shift(int &result);
	static bool get_crazy(int &result);
	static bool get_sum(int &result);
	static bool get_product(int &result);
	static bool get_term(int &result);
	static bool get_mem_dest(int &result);
	static bool get_number(int &result);
	
	static int to_malbolge_range(int number);
	

};

#endif

