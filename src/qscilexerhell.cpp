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


#include "qscilexerhell.h"
#include <QRegExp>
#include <QDebug>

QsciLexerHeLL::QsciLexerHeLL(QObject *parent) :
    QsciLexerCustom(parent)
{
    //this->setAutoIndentStyle(QsciScintilla::AiMaintain);
    QFont font("Courier", 12);
    font.setFixedPitch(true);
    this->setDefaultFont(font);
}

QString QsciLexerHeLL::description(int style) const
{
    switch(style) {
    case Default:
        return "Default";
    case Comment:
        return "Comment";
    case Keyword:
            return "Keyword";
    case MemoryCell:
            return "Command";
    case EmptyLine:
            return "Empty line";
    case Number:
            return "Number";
    case Label:
            return "Label";
    case EntryLabel:
            return "Entrypoint (label)";
    case String:
            return "String";
    case Error:
            return "Syntax error";
    case ReservedBlock:
            return "Unused memory cell";
    case ActiveXlat2:
            return "Current Xlat2 cycle position";
    }
    return ""; //return QString(style);
}

const char* QsciLexerHeLL::language() const
{
    return "HeLL";
}

const char* QsciLexerHeLL::keywords(int set) const
{
    if (set == 1)
        return ".DATA .CODE .OFFSET @";
    if (set == 2)
        return "RNop In Out Hlt Jmp MovD Nop Opr Rot";
    return 0;
}

QColor QsciLexerHeLL::defaultColor(int style) const {
    switch(style) {
    case Comment:
        return Qt::darkGreen;
    case Keyword:
        return Qt::blue;
    case Label:
    case EntryLabel:
        return Qt::darkRed;
    case Number:
        return Qt::blue;
    case MemoryCell:
    case ActiveXlat2:
        return Qt::darkYellow;
    case String:
        return Qt::darkGreen;
    case ReservedBlock:
            return Qt::blue;
    }
    return Qt::black;
}

QFont QsciLexerHeLL::defaultFont(int style) const {
    switch (style) {
    case ActiveXlat2:
        return QFont("Courier", 12, QFont::Bold);
    default:
        return QFont("Courier", 12);
    }
}

bool QsciLexerHeLL::eolFill(int) const {
    return false; //(style == EmptyLine);
}

QColor QsciLexerHeLL::defaultPaper(int style) const {
    switch(style) {
    /*case EmptyLine:
    case ReservedBlock:
        return QColor(224,224,224);
    case MemoryCell:
        return QColor(192,255,255);
    case EntryLabel:
        return Qt::yellow;*/
    case Error:
        return Qt::red;
    }
    return Qt::white;
}


void QsciLexerHeLL::styleWithRegexp(QString source, int source_offset, int style, QString regexp, int regnum, QString inner_regexp, int inner_regnum) {
    QRegExp re(regexp);
    int pos = 0;

    while ((pos=re.indexIn(source, pos)) >= 0) {
        QRegExp regInner(inner_regexp);
        int pos_inner = 0;
        while ((pos_inner = regInner.indexIn(re.cap(regnum),pos_inner)) >= 0) {
            startStyling(source_offset + re.pos(regnum) + regInner.pos(inner_regnum));
            setStyling(regInner.cap(inner_regnum).length(), style);
            pos_inner++;
        }
        pos++;
    }
}

#define LINE_STATE_COMMENT 1
#define LINE_STATE_BLOCK   2

void QsciLexerHeLL::styleText(int start, int end){
    if (!this->editor())
        return;
    QString whole_text = this->editor()->text();
    // style entire document (slow / not recommended; but it fixes some issues)
    // TODO: fix issues with SC_CP_DBCS and/or SC_CP_UTF8
    start=0;
    end=whole_text.length();
    QStringList code_section_labels;
    // TODO: check, if inside a comment
    QRegExp re(QString("\\.CODE(((?!\\.DATA)[\\s\\S])*)(\\.DATA|$)"));
    int pos = 0;

    while ((pos=re.indexIn(whole_text, pos)) >= 0) {

        QRegExp regLabel(QString("(^|[\\s:])([a-zA-Z_][a-zA-Z0-9_]*)\\:"));
        int pos_label = 0;
        while ((pos_label = regLabel.indexIn(re.cap(0),pos_label)) >= 0) {
            // TODO: check, if inside a comment
            code_section_labels << regLabel.cap(2);
            pos_label++;
        }
        pos++;
    }
    /*do{
        char* data = new char[end - start + 1];
        this->editor()->SendScintilla(QsciScintilla::SCI_GETTEXTRANGE, start, end, data);
        QString source;
        if (this->editor()->isUtf8()) {
            source = QString::fromUtf8(data);
        }else{
            source = QString::fromLatin1(data);
        }
        delete[] data;
        if (source.isEmpty())
            return;

        int start_line = 0;
        int start_index = 0;
        this->editor()->lineIndexFromPosition(start, &start_line, &start_index);

*/
        startStyling(start);
        setStyling(end-start, Default);

        QString source = whole_text;
        this->styleWithRegexp(source, start, MemoryCell, "(^|[\\s:])((RNop|(In|Out|Hlt|Jmp|MovD|Nop|Opr|Rot)(/(In|Out|Hlt|Jmp|MovD|Nop|Opr|Rot))*)([\\s]+(RNop|(In|Out|Hlt|Jmp|MovD|Nop|Opr|Rot)(/(In|Out|Hlt|Jmp|MovD|Nop|Opr|Rot))*))*)($|[\\s;#%}]|//)", 2, "(RNop|(In|Out|Hlt|Jmp|MovD|Nop|Opr|Rot)(/(In|Out|Hlt|Jmp|MovD|Nop|Opr|Rot))*)", 0);


        foreach(QString word, code_section_labels) {
            this->styleWithRegexp(source, start, MemoryCell, QString("(^|[\\s:])((U_|R_)?").append(word).append("(([\\s]+)(U_|R_)?").append(word).append(")*)($|[\\s#%;}]|//)"), 2, QString("(U_|R_)?").append(word), 0);
        }
        this->styleWithRegexp(source, start, Number, "(^|[\\s\\(\\)\\+\\-\\*\\/\\<\\>\\!:@])(([0-9]+|0t[0-2]+|C0|C1|C20|C21|C2|EOF)([\\s\\(\\)\\+\\-\\*\\/\\<\\>\\!]+([0-9]+|0t[0-2]+|C0|C1|C20|C21|C2|EOF))*)($|[\\s\\(\\)\\+\\-\\*\\/\\<\\>\\!;#%{}])", 2, "0t[0-2]+|[0-9]+|C0|C1|C20|C21|C2|EOF", 0);
        this->styleWithRegexp(source, start, Number, "(^|[\\s:])(\\?([\\s]+\\?)*)($|//|[\\s;#%}])", 2, "\\?", 0);
        this->styleWithRegexp(source, start, ReservedBlock, "(^|[\\s:])(\\?\\-([\\s]+\\?\\-)*)($|//|[\\s;#%}])", 2, "\\?\\-", 0);

        this->styleWithRegexp(source, start, Label, "(^|[\\s:{])([A-Za-z_][A-Za-z0-9_]*:)", 2);

        this->styleWithRegexp(source, start, EntryLabel, "(^|[\\s:{])(ENTRY:)", 2);

        this->styleWithRegexp(source, start, Keyword, "(^|[\\s}])((\\.CODE|\\.DATA|\\.OFFSET)([\\s]+(\\.CODE|\\.DATA|\\.OFFSET))*)($|[\\s;#%@{]|//)", 2, "\\.CODE|\\.DATA|\\.OFFSET", 0);
        this->styleWithRegexp(source, start, Keyword, "@", 0);



        /*if (getLineState(start_line-1) & LINE_STATE_COMMENT) {
            inside_multiline_comment = true;
            startStyling(start);
        }
        if (getLineState(start_line-1) & LINE_STATE_BLOCK) {
            inside_block = true;
            startStyling(start);
        }*/
        //int last_updated_line = start_line-1;

//        int last_line_checked_for_empty_line = -1;
        QStringList lines = source.split("\n"); // QRegExp("[\r\n]") statt "\n"??
        bool inside_braces = false;
        bool inside_multiline_comment = false;
        int linestart = 0;
        int line_number = 0;
        int prev_level = QsciScintilla::SC_FOLDLEVELBASE;
        bool prev_empty_flag = true;
        bool prev_inside_block_at_eol = false;
        bool inside_any_section = false;
        foreach (const QString &line, lines) {

            // RESET FOLDING FOR CURRENT LINE!
            int cur_level = QsciScintilla::SC_FOLDLEVELBASE;
            bool cur_empty_flag = false;

            // TODO: if line contains \n: internal error (split did not work)
            // TODO: if line contains \r: internal error (line-ending not as expected!) [TODO: HANDLE IT! – PROBLEM: line_number must stay correct!]

            bool contains_section_start = false;
            bool contains_block_start = false; // e.g. {, ...
            bool cur_inside_block_at_eol = false;

            if (is_empty_line(line)) {
                if (inside_multiline_comment) {
                    /*startStyling(linestart);
                    setStyling(1, Comment);*/
                    cur_level = QsciScintilla::SC_FOLDLEVELBASE + 1 + (inside_any_section?1:0);
                    cur_inside_block_at_eol = true;
                } else if (inside_braces) {
                    cur_level = QsciScintilla::SC_FOLDLEVELBASE + 1 + (inside_any_section?1:0);
                    cur_inside_block_at_eol = true;
                } else {
                    // END OF BLOCK...
                    cur_empty_flag = true;
                    cur_level = QsciScintilla::SC_FOLDLEVELBASE + (inside_any_section?1:0);
                    cur_inside_block_at_eol = false;
                }
            }else{
                style_line(line, linestart, inside_braces, inside_multiline_comment, contains_section_start, contains_block_start, cur_inside_block_at_eol);
                if (contains_section_start) {
                    inside_any_section = true;
                }
                if (contains_section_start) {
                    cur_level = QsciScintilla::SC_FOLDLEVELBASE;
                } else if (contains_block_start || !prev_inside_block_at_eol) {
                    cur_level = QsciScintilla::SC_FOLDLEVELBASE + (inside_any_section?1:0);
                } else {
                    cur_level = QsciScintilla::SC_FOLDLEVELBASE + 1 + (inside_any_section?1:0);
                }
            }

            // FOLD IT!

            bool prev_header = false;
            if (prev_level < cur_level) {
                prev_header = true;
            }
            if (line_number > 0) {
                int fold_level = prev_level | (prev_header?QsciScintilla::SC_FOLDLEVELHEADERFLAG:0) | (prev_empty_flag?QsciScintilla::SC_FOLDLEVELWHITEFLAG:0);
                int old_fold_level = this->editor()->SendScintilla(QsciScintilla::SCI_GETFOLDLEVEL, line_number-1);
                if (fold_level != old_fold_level) {
                    if (line_number-1 > 0) {
                        this->editor()->SendScintilla(QsciScintilla::SCI_ENSUREVISIBLE, line_number-2);
                    }
                    if (this->editor()->SendScintilla(QsciScintilla::SCI_GETFOLDEXPANDED, line_number-1)==0) {
                        this->editor()->SendScintilla(SCI_FOLDLINE, line_number-1, SC_FOLDACTION_EXPAND);
                    }
                    this->editor()->SendScintilla(QsciScintilla::SCI_SETFOLDLEVEL, line_number-1, fold_level);
                }
            }

            prev_level = cur_level;
            prev_empty_flag = cur_empty_flag;
            prev_inside_block_at_eol = cur_inside_block_at_eol;
            linestart += line.length() + 1;
            line_number++;
        }
        int fold_level = prev_level | (prev_empty_flag?QsciScintilla::SC_FOLDLEVELWHITEFLAG:0);
        int old_fold_level = this->editor()->SendScintilla(QsciScintilla::SCI_GETFOLDLEVEL, line_number-1);
        if (fold_level != old_fold_level) {
            if (line_number-1 > 0) {
                this->editor()->SendScintilla(QsciScintilla::SCI_ENSUREVISIBLE, line_number-2);
            }
            if (this->editor()->SendScintilla(QsciScintilla::SCI_GETFOLDEXPANDED, line_number-1)==0) {
                this->editor()->SendScintilla(SCI_FOLDLINE, line_number-1, SC_FOLDACTION_EXPAND);
            }
            this->editor()->SendScintilla(QsciScintilla::SCI_SETFOLDLEVEL, line_number-1, fold_level);
        }
    {
    LMAODebugInformations::SourcePosition pos;
    foreach(pos, active_xlat2) {
        if (pos.first_line <= 0)
            pos.first_line = 1;
        if (pos.first_column <= 0)
            pos.first_column = 1;
        if (pos.last_line <= 0)
            pos.last_line = 1;
        if (pos.last_column <= 0)
            pos.last_column = 1;
        int index_line_start = this->editor()->positionFromLineIndex(pos.first_line-1,0);
        int index_line_end = this->editor()->positionFromLineIndex(pos.last_line-1,0);
        int index = index_line_start + pos.first_column - 1;
        int end_index = index_line_end + pos.last_column;
        if (end_index > index && end_index <= end) {
            startStyling(index);
            int len = end_index - index;
            setStyling(len, ActiveXlat2);
        }
    }
    }
}

bool QsciLexerHeLL::is_empty_line(const QString& line) {
    bool empty_line = true;
    for (int i=0; i<line.length(); i++) {
        if (line.at(i) != QChar('\r') && line.at(i) != QChar('\n') && line.at(i) != QChar('\t') && line.at(i) != QChar('\f') && line.at(i) != QChar('\v') && line.at(i) != QChar(' ')) {
            empty_line = false;
            break;
        }
    }
    return empty_line;
}

void QsciLexerHeLL::style_line(const QString& line, int linestart, bool& inside_braces, bool& inside_multiline_comment, bool& contains_section_start, bool& contains_block_start, bool& inside_block_at_eol) {

    bool inside_string = false;
    bool inside_char = false;
    bool backslash_read = false;
    bool inside_comment = false;
    bool read_slash = false;
    bool read_star = false;
    bool whitespace_read = true;
    int inside_char_since = 0;

    contains_section_start = false;
    contains_block_start = false;
    bool block_start_if_text_follows = false;


    inside_block_at_eol = true;

    startStyling(linestart);

    for (int i=0; i<line.length(); i++) {

            if (block_start_if_text_follows && !whitespace_read) {
                contains_block_start = true;
            }
            // String?
            if (line.at(i) == QChar('"') && !backslash_read && !inside_comment && !inside_char && !inside_multiline_comment) {
                // style position i as string
                startStyling(linestart+i);
                setStyling(1, String);
                inside_string = !inside_string;
            } else if (inside_string) {
                // style position i as string
                setStyling(1, String);
            }

            // Character?
            if (line.at(i) == QChar('\'') && !backslash_read && !inside_string && !inside_comment && !inside_multiline_comment) {
                // style position i as character/number
                startStyling(linestart+i);
                if (inside_char && inside_char_since==0) {
                    setStyling(1, Error);
                } else {
                    setStyling(1, Number);
                }
                inside_char = !inside_char;
                if (inside_char) {
                    inside_char_since = 0;
                }
            } else if (inside_char) {
                // style position i as character/number
                if (inside_char_since > 1) {
                    setStyling(1, Error);
                } else if (inside_char_since > 0) {
                    if (line.at(i-1) != QChar('\\')) {
                        setStyling(1, Error);
                    } else {
                        setStyling(1, Number);
                    }
                } else {
                    setStyling(1, Number);
                }
                inside_char_since++;
            }


            if (line.at(i) == QChar('\\') && !backslash_read) {
                backslash_read = true;
            } else {
                backslash_read = false;
            }

            // Comment?
            if ((line.at(i) == QChar('#') || line.at(i) == QChar('%') || line.at(i) == QChar(';') || (line.at(i) == QChar('/') && read_slash)) && !inside_string && !inside_char && !inside_multiline_comment) {
                inside_comment = true;
                if (line.at(i) == QChar('/')) {
                    startStyling(linestart+i-1);
                    setStyling(1, Comment);
                }else{
                    startStyling(linestart+i);
                }
            }

            // End of multi line comment?
            bool slash_used = false;
            if (line.at(i) == QChar('/') && read_star && !inside_string && !inside_char && inside_multiline_comment) {
                inside_multiline_comment = false;
                slash_used = true;
                setStyling(1, Comment);
            }

            read_star = false;
            // Start of multi line comment?
            if (line.at(i) == QChar('*') && read_slash && !inside_string && !inside_char && !inside_comment && !inside_multiline_comment) {
                inside_multiline_comment = true;
                startStyling(linestart+i-1);
                setStyling(1, Comment);

            }else if (line.at(i) == QChar('*')) {
                read_star = true;
            }

            // Code and/or Data blocks: Curly brackets?
            if (line.at(i) == QChar('{') && !inside_string && !inside_char && !inside_comment && !inside_multiline_comment) {
                if (inside_braces) {
                    // error
                    startStyling(linestart+i);
                    setStyling(1, Error);
                }else{
                    inside_braces = true;
                    contains_block_start = true;
                }
            }
            if (line.at(i) == QChar('}') && !inside_string && !inside_char && !inside_comment && !inside_multiline_comment) {
                if (!inside_braces) {
                    // error
                    startStyling(linestart+i);
                    setStyling(1, Error);
                }else{
                    inside_braces = false;
                    block_start_if_text_follows = true;
                    inside_block_at_eol = false;
                }
            }

            // Comment...
            if (inside_comment || inside_multiline_comment) {
                startStyling(linestart+i);
                setStyling(1, Comment);
            }

            if (line.at(i) == QChar('.') && whitespace_read && !inside_string && !inside_char && !inside_multiline_comment && !inside_comment) {
                // TODO: CHECK FOR .DATA and .CODE!
                if (i+4<line.length()) {
                    if ( (line.at(i+1) == QChar('D') && line.at(i+2) == QChar('A') && line.at(i+3) == QChar('T') && line.at(i+4) == QChar('A'))
                         || (line.at(i+1) == QChar('C') && line.at(i+2) == QChar('O') && line.at(i+3) == QChar('D') && line.at(i+4) == QChar('E')) )
                    {
                        bool whitespace = false;
                        if (i+5==line.length()) {
                            whitespace = true;
                        } else {
                            if (line.at(i+5) == QChar(' ') || line.at(i+5) == QChar('\t') || line.at(i+5) == QChar('\f') || line.at(i+5) == QChar('\v')){
                                whitespace = true;
                            }
                        }
                        if (whitespace) {
                            contains_section_start = true; // maybe
                            i+=4;
                            inside_block_at_eol = false;
                            whitespace_read = false;
                            read_slash = false;
                            continue;
                        }
                    }
                }
            }

            if (line.at(i) == QChar('/') && !slash_used) {
                read_slash = true;
            }else{
                read_slash = false;
            }
            whitespace_read = false;
            if (line.at(i) == QChar(' ') || line.at(i) == QChar('\t') || line.at(i) == QChar('\f') || line.at(i) == QChar('\v')){
                whitespace_read = true;
            }
            if (!whitespace_read && !inside_comment && !inside_multiline_comment && !read_slash) {
                inside_block_at_eol = true;
            }


        }
    if (read_slash && !inside_comment) {
        inside_block_at_eol = true;
    }
    /*
    // display error...
    if (inside_string || inside_char) {
        startStyling(linestart+line.length());
        setStyling(1, Error);
    }
    if (inside_multiline_comment) {
        startStyling(linestart+line.length());
        setStyling(1, Comment);
    }
    */
}

/*
void QsciLexerHeLL::setLineState(int line, int value) {
    if (line >= 0) {
        this->editor()->SendScintilla(QsciScintilla::SCI_SETLINESTATE, line, value);
    }
}


int QsciLexerHeLL::getLineState(int line) {
    int ret = 0;
    if (line >= 0) {
        ret = this->editor()->SendScintilla(QsciScintilla::SCI_GETLINESTATE, line);
    }
    return ret;
}*/

void QsciLexerHeLL::update_active_xlat2(QLinkedList<LMAODebugInformations::SourcePosition> xlat2) {
    active_xlat2 = xlat2;
    styleText(0,this->editor()->text().length());
}

void QsciLexerHeLL::remove_active_xlat2() {
    active_xlat2 = QLinkedList<LMAODebugInformations::SourcePosition>();
    styleText(0,this->editor()->text().length());
}

QsciLexerHeLL::~QsciLexerHeLL() {
}

