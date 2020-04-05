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

#include "lmaodebuginformations.h"
#include "malbolgedefinitions.h"
#include <QFile>
#include <QDebug>
#include <QTextStream>
#include <QRegExp>

LMAODebugInformations::LMAODebugInformations(QObject *parent) :
    QObject(parent)
{
    steps_until_entry_point = -1;

}

int LMAODebugInformations::read_from_file(QString filename) {

    enum {
        UNDEFINED = 0,
        LABELS = 1,
        SOURCEPOSITIONS = 2,
        EXECUTION_STEPS_UNTIL_ENTRY_POINT = 3,
        SOURCE_FILE = 4,
        MALBOLGE_FILE = 5,
        XLAT2_CYCLES = 6
    } state = UNDEFINED;

    // clean up!
    labels_in_memory.clear();
    memory_cells_in_source.clear();
    xlat2_in_source.clear();
    steps_until_entry_point = -1;
    hell_source_file = "";
    malbolge_file = "";

    if (filename.length() <= 0) {
        qDebug() << "No file with debugging information given.";
        return -1;
    }

    QFile file(filename);
    if(!file.open(QIODevice::ReadOnly)) {
        qDebug() << "Cannot open file with debugging information: " << filename;
        return -1; //cannot open file
    }

    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine();
        if (line.length() <= 0)
            continue;
        if (line.at(0) == ':') {
            // TODO: change state!
            line = line.toUpper();
            if (line == ":LABELS:") {
                state = LABELS;
            } else if (line == ":SOURCEPOSITIONS:") {
                state = SOURCEPOSITIONS;
            } else if (line == ":EXECUTION_STEPS_UNTIL_ENTRY_POINT:") {
                state = EXECUTION_STEPS_UNTIL_ENTRY_POINT;
            } else if (line == ":SOURCE_FILE:") {
                state = SOURCE_FILE;
            } else if (line == ":MALBOLGE_FILE:") {
                state = MALBOLGE_FILE;
            } else if (line == ":XLAT2:") {
                state = XLAT2_CYCLES;
            } else{
                state = UNDEFINED;
            }

        }else{
            switch (state) {
            case UNDEFINED:
                break; // cannot handle line
            case LABELS:
            {
                QRegExp re(QString("^([a-zA-Z_][a-zA-Z0-9_]*): (DATA|CODE) ([0-9]+)($|\\r|\\n)"));
                int match = re.indexIn(line);
                if (match >= 0) {
                    QString label = re.cap(1);
                    MemoryAddress destination;
                    if (re.cap(2)=="DATA")
                        destination.type = destination.DATA;
                    else
                        destination.type = destination.CODE;
                    destination.address = re.cap(3).toInt();
                    labels_in_memory.insert(label, destination);
                }
                break;
            }
            case SOURCEPOSITIONS:
            {
                QRegExp re(QString("^([0-9_]+): (DATA|CODE) ([0-9]+):([0-9]+) - ([0-9]+):([0-9]+)($|\\r|\\n)"));
                int match = re.indexIn(line);
                if (match >= 0) {
                    int address = re.cap(1).toInt();
                    SourcePosition source;
                    if (re.cap(2)=="DATA")
                        source.type = source.DATA;
                    else
                        source.type = source.CODE;
                    source.first_line = re.cap(3).toInt();
                    source.first_column = re.cap(4).toInt();
                    source.last_line = re.cap(5).toInt();
                    source.last_column = re.cap(6).toInt();
                    memory_cells_in_source.insert(address, source);
                }
                break;
            }
                break;
            case EXECUTION_STEPS_UNTIL_ENTRY_POINT:
                if (steps_until_entry_point == -1){
                    bool ok = false;
                    int entry = line.toInt(&ok);
                    if (ok)
                        steps_until_entry_point = entry;
                }
                break;
            case SOURCE_FILE:
                if (hell_source_file.length() <= 0)
                    hell_source_file = line;
                break;
            case MALBOLGE_FILE:
                if (malbolge_file.length() <= 0)
                    malbolge_file = line;
                break;
            case XLAT2_CYCLES:
                {
                    QRegExp re(QString("^([0-9_]+) ([!-~]) ([0-9]+):([0-9]+) - ([0-9]+):([0-9]+)($|\\r|\\n)"));
                    int match = re.indexIn(line);
                    if (match >= 0) {
                        int address = re.cap(1).toInt();
                        char symbol = re.cap(2).data()->toLatin1();
                        SourcePosition source;
                        source.type = source.CODE;
                        source.first_line = re.cap(3).toInt();
                        source.first_column = re.cap(4).toInt();
                        source.last_line = re.cap(5).toInt();
                        source.last_column = re.cap(6).toInt();
                        xlat2_in_source.push_back(QPair<QPair<int,char>,SourcePosition>(QPair<int,char>(address, symbol),source));
                    }
                }
                break;
            default:
                break; // should not occur
            }
        }
    }


    file.close();

    return 0; // success
}

bool LMAODebugInformations::get_address_label(int address, QString &label, bool &is_inside_codesection) {
    QHash<QString, MemoryAddress>::const_iterator i = labels_in_memory.begin();
    while (i != labels_in_memory.end()) {
        MemoryAddress m = i.value();
        if (m.address == address) {
            label = i.key();
            if (m.type == m.CODE)
                is_inside_codesection = true;
            else
                is_inside_codesection = false;
            return true;
        }
        i++;
    }
    return false; // not found
}

int LMAODebugInformations::get_label_address(QString label, bool* is_inside_codesection) {
    QHash<QString, MemoryAddress>::const_iterator i = labels_in_memory.find(label);
    if (i != labels_in_memory.end() && i.key() == label) {
        MemoryAddress m = i.value();
        if (is_inside_codesection != 0) {
            if (m.type == m.CODE)
                *is_inside_codesection = true;
            else
                *is_inside_codesection = false;
        }
        return m.address;
    }
    return -1; // not found
}

int LMAODebugInformations::get_source_position(int memory_cell, int& first_line, int& first_column, int& last_line, int& last_column, bool* is_inside_codesection) {
    QHash<int, SourcePosition>::const_iterator i = memory_cells_in_source.find(memory_cell);
    if (i != memory_cells_in_source.end() && i.key() == memory_cell) {
        SourcePosition s = i.value();
        /*if (is_inside_codesection != 0) {
            if (m.type == m.CODE)
                *is_inside_codesection = true;
            else
                *is_inside_codesection = false;
        }
        return m.address;*/
        first_line = s.first_line;
        first_column = s.first_column;
        last_line = s.last_line;
        last_column = s.last_column;
        if (is_inside_codesection != 0) {
            if (s.type == s.CODE) {
                *is_inside_codesection = true;
            }else{
                *is_inside_codesection = false;
            }
        }
        return 0; // success
    }
    return -1; // not found
}

int LMAODebugInformations::get_first_memory_cell_starting_at_line(int line, int &memory_cell, bool* is_inside_codesection) {

    if (line < 0)
        return -1; // error

    int best_match_startline = -1;
    int best_match_startcolumn = -1;
    int best_match_memorycell = -1;
    bool best_match_inside_codesection = false;

    QHash<int, SourcePosition>::iterator i = memory_cells_in_source.begin();
    while (i != memory_cells_in_source.end()) {
        SourcePosition sp = i.value();
        if (sp.first_line < line) {
            i++;
            continue;
        }
        if (best_match_startline == -1 || best_match_startline > sp.first_line || (best_match_startline == sp.first_line && best_match_startcolumn > sp.first_column) ){
            best_match_startline = sp.first_line;
            best_match_startcolumn = sp.first_column;
            best_match_memorycell = i.key();
            best_match_inside_codesection = (sp.type == sp.CODE);
        }
        i++;
    }

    if (best_match_startline == -1 || best_match_memorycell == -1)
        return -1; // not found

    memory_cell = best_match_memorycell;
    if (is_inside_codesection != 0)
        *is_inside_codesection = best_match_inside_codesection;
    return 0; // success
}

int LMAODebugInformations::get_steps_until_entry_point() {
    return steps_until_entry_point;
}

QLinkedList<LMAODebugInformations::SourcePosition> LMAODebugInformations::get_active_xlat2_positions(unsigned int memory[59050]) {
    QLinkedList<SourcePosition> ret;
    QPair<QPair<int, char>, SourcePosition> xlat2;
    foreach (xlat2, xlat2_in_source) {
        int mem_pos = xlat2.first.first;
        if (mem_pos < 0 || mem_pos > 59048) {
            continue;
        }
        if (memory[mem_pos]>127) {
            continue;
        }
        if (memory[mem_pos] == (unsigned int)xlat2.first.second) {
            ret.push_back(xlat2.second);
        }
    }
    return ret;
}
