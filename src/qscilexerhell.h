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

#ifndef QSCILEXERHELL_H
#define QSCILEXERHELL_H

#include <Qsci/qsciscintilla.h>
#include <Qsci/qscilexercustom.h>

#define SCI_FOLDLINE 2237
#define SC_FOLDACTION_EXPAND 1


class QsciLexerHeLL : public QsciLexerCustom
{
    Q_OBJECT
public:
    explicit QsciLexerHeLL(QObject *parent = 0);

    void styleText(int start, int end);

    const char* language() const;
    QString description(int style) const;
    QColor defaultColor(int style) const;
    QColor defaultPaper(int style) const;
    const char* keywords(int set) const;
    bool eolFill(int style) const;

    ~QsciLexerHeLL();
    

    enum {
        Default = 0,
        Comment = 1,
        Keyword = 2,
        MemoryCell = 3,
        EmptyLine = 4,
        Number = 5,
        Label = 6,
        EntryLabel = 7,
        String = 8,
        Error = 9,
        ReservedBlock = 10
    };

private:
    void styleWithRegexp(QString source, int source_offset, int style, QString regexp, int regnum, QString inner_regexp = "^[\\s\\S]*$", int inner_regnum = 0);

    bool is_empty_line(const QString& line);

    void style_line(const QString& line, int linestart, bool& inside_braces, bool& inside_multiline_comment, bool& contains_section_start, bool& contains_block_start, bool& inside_block_at_eol);

    /*void setLineState(int line, int value);
    int getLineState(int line);*/
    
};

#endif // QSCILEXERHELL_H
