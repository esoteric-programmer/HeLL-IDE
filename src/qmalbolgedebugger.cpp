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

#include "qmalbolgedebugger.h"
#include "malbolgedefinitions.h"
#include "memorycellparser.h"
#include <QCoreApplication>
#include <QDebug>
#include "qsleep.h"

QMalbolgeDebugger::QMalbolgeDebugger(QString cmd, QStringList arguments, QObject *parent) :
    QAbstractMalbolgeRunner(cmd, arguments, parent)
{
    execute_single_step = false;
    running_until_breakpoint = false;
    pause_emitted = false;
    reading_was_paused = false;
    force_reset = false;
    recommend_goto_codesection_on_pause = false;
    last_jmp_destination_label = "(unknown)";
    last_jmp_destination_label_offset = 0;
    this->debug_filename = QString();
}


int QMalbolgeDebugger::execute_malbolge(QString filename) {


    do {

        execute_single_step = false;
        running_until_breakpoint = false;
        pause_emitted = false;
        reading_was_paused = false;
        force_reset = false;
        recommend_goto_codesection_on_pause = false;
        set_abort_reading(false);
        set_can_read(true);
        breakpoints.clear();

    QRegExp re(QString("[\\r\\n]Debug(ging)? information(s?) written to ([^\\r\\n]*)[\\r\\n]"));
    int match = re.indexIn(this->child_process_stdout);

    QString debug_file;
    if (match >= 0) {
        debug_file = re.cap(3);
    } else {
        debug_file = this->debug_filename;
        if (debug_file.length() <= 0) {
            this->StdErrWrite("Cannot open file with debugging information.");
            return -1;
        }
    }


    this->info(QString("Debugging ").append(filename).append("...\n"));

    int ret = load_malbolge_program(filename);
    if (ret != 0)
        return ret;

    // Load debug informations
    if (debug_infos.read_from_file(debug_file)<0) {
        this->StdErrWrite("Cannot parse file with debugging information.");
        return -1; // cannot read debug infos
    }

    //qDebug() << "Read debug infos.";

    execute_single_step = false;
    running_until_breakpoint = false;

    int steps_until_entry_point = debug_infos.get_steps_until_entry_point();
    bool entry_point_reached = false;
    int current_step = 0;
    last_jmp_destination_label = "(unknown)";
    last_jmp_destination_label_offset = 0;

    bool has_been_running = true;

    emit query_breakpoints();

    this->set_malbolge_started();

    // execute program
    while (!this->should_terminate() && !force_reset) {

        if (current_step >= steps_until_entry_point) {
            entry_point_reached = true;
        }else{
            last_jmp_destination_label = "somewhere in the initialization section";
            last_jmp_destination_label_offset = 0;
        }
        if (current_step == steps_until_entry_point) {
            this->flush_out_buffer();
        }
        bool should_run = false;
        if (current_step < steps_until_entry_point) {
            should_run = true;
        }else if (running_until_breakpoint) {
            // TODO: CHECK FOR BREAKPOINT!!!
            should_run = true;
        }else if (execute_single_step) {
            execute_single_step = false;
            should_run = true;
        }

        if (!should_run) {
            if (has_been_running) {
                has_been_running = false;
                this->flush_out_buffer();
                if (!pause_emitted) {
                    this->emit_pause_state();
                    reading_was_paused = false;
                }

            }
            QCoreApplication::processEvents();
            QSleep::msleep(20);
            continue;
        } else {
            set_can_read(true);
        }

        if (pause_emitted) {
            emit execution_continued();
            //qDebug() << "CONTINUE EMITTED.";
            pause_emitted = false;
        }

        has_been_running = true;
        QCoreApplication::processEvents();
        if (!get_can_read()) {
            continue; // seems that the state has been changed...
        }

/*
        if (entry_point_reached)
            address_to_label
            */

        int command;
        int execution_step_result = execute_malbolge_step(command);
        if (command == MALBOLGE_JMP && entry_point_reached) {
            int inside_codesection = 1;
            last_jmp_destination_label = address_to_label(c, inside_codesection, last_jmp_destination_label_offset);
        }else if (entry_point_reached){
            last_jmp_destination_label_offset++;
        }
        if (execution_step_result < 0) {
            this->clear_output_buffer(true);
            this->flush_out_buffer();
            return execution_step_result; // ERROR
        } else if (execution_step_result > 0) {
            break; // TERMINATED NORMALLY
        }

        if (running_until_breakpoint) {
            bool breakpoint = false;
            bool only_codesection_bp = true;
            QHash<unsigned int, QPair<int, bool> >::iterator i = breakpoints.find(d);
            while (i != breakpoints.end() && i.key() == d) {
                QPair<int, bool> bp = i.value();
                if (!bp.second) {
                    breakpoint = true;
                    only_codesection_bp = false;
                }
                i++;
            }
            i = breakpoints.find(c);
            while (i != breakpoints.end() && i.key() == c) {
                QPair<int, bool> bp = i.value();
                if (bp.second) {
                    breakpoint = true;
                }
                i++;
            }
            if (breakpoint) {
                running_until_breakpoint = false;
                if (only_codesection_bp) {
                    recommend_goto_codesection_on_pause = true;
                }
            }
        }

        if (!running_until_breakpoint || current_step % 100000 == 0) {
            this->flush_out_buffer();
        }

        current_step++;
    }
    if (force_reset) {
        this->flush_out_buffer();
        emit clear_userinput();
        clear_inputqueue();
        continue;
    }
    break;
    }while(1);
    this->clear_output_buffer(true);
    this->flush_out_buffer();
    return 0;
}


QString QMalbolgeDebugger::trinary(unsigned int value){
    QString tri =  QString::number(value, 3);
    while (tri.length()<10)
        tri = "0"+tri;
    return QString("0t") + tri;
}

QString QMalbolgeDebugger::trinary_and_char(unsigned int value){
    unsigned int charval = value % 256;
    QString charstr = QString("");
    if (charval == '\n')
        charstr = " '\\n'";
    else if (charval == '\r')
        charstr = " '\\r'";
    else if (charval == '\t')
        charstr = " '\\t'";
    else if (charval == '\f')
        charstr = " '\\f'";
    else if (charval == '\'')
        charstr = " '\\''";
    else if (charval >= 32 && charval <=127)
        charstr = QString(" '") + QString::fromLatin1((char*)&charval, 1) + QString("'");
    return trinary(value)+charstr;

}

QString QMalbolgeDebugger::malbolge_xlat_cycle(unsigned int memory_c, unsigned int pos) {
    QString result = "";
    bool only_nops = true;
    unsigned int current = memory_c;
    int counter = 0;
    do {
        int normalized = get_instruction(current,pos);
        if (normalized < 0)
            return "invalid";
        if (!result.isEmpty())
            result.append(QString("/"));
        result.append(instructionname(normalized));
        if (normalized != MALBOLGE_NOP)
            only_nops = false;
        translate(current);
        if (current == memory_c)
            break;
        counter++;
        if (counter >= 9) {
            result.append("/...");
            only_nops = false;
            break;
        }
    }while(1);
    if (only_nops)
        return "RNop";
    return result;
}


void QMalbolgeDebugger::setDebugFileName(QString filename) {
    this->debug_filename = filename;
}

void QMalbolgeDebugger::step() {
    //qDebug() << "Step!";
    if (reading_was_paused) {
        set_can_read(true);
        if (pause_emitted) {
            emit execution_continued();
            pause_emitted = false;
        }
    }else{
        execute_single_step = true;
    }
}

void QMalbolgeDebugger::run_until_breakpoint() {
    //qDebug() << "Step!";
    running_until_breakpoint = true;
    reading_was_paused = false;
    recommend_goto_codesection_on_pause = false;
    if (!get_can_read()) {
        set_can_read(true);
        if (pause_emitted) {
            emit execution_continued();
            pause_emitted = false;
        }
    }
}

void QMalbolgeDebugger::reset() {
    this->force_reset = true;
    set_abort_reading(true);
}

void QMalbolgeDebugger::emit_pause_state() {
    if (pause_emitted)
        return;
    int line_d, column_d, line_c, column_c, lld, lcd, llc, lcc = -1;
    bool inside_codesection = true;
    if (debug_infos.get_source_position(d, line_d, column_d, lld, lcd, &inside_codesection)<0) {
        line_d = -1;
        column_d = -1;
    }
    if (inside_codesection) {
        line_d = -1;
        column_d = -1;
    }
    inside_codesection = false;
    if (debug_infos.get_source_position(c, line_c, column_c, llc, lcc, &inside_codesection)<0) {
        line_c = -1;
        column_c = -1;
    }
    if (!inside_codesection) {
        line_c = -1;
        column_c = -1;
    }
    emit execution_paused(line_d, column_d, lld, lcd, line_c, column_c, llc, lcc, recommend_goto_codesection_on_pause);
    pause_emitted = true;

    QString offset = "";
    if (last_jmp_destination_label_offset != 0) {
        offset = (last_jmp_destination_label_offset>0?QString(" + "):QString(" - ")) + QString::number(last_jmp_destination_label_offset>0?last_jmp_destination_label_offset:-last_jmp_destination_label_offset);
    }
    QString c_label = last_jmp_destination_label + offset;
    //qDebug() << "C: " << c_label;

    QString mem_c_xlat = malbolge_xlat_cycle(memory[c], c);
    //qDebug() << "[C]: " << mem_c_xlat;

    QString a_reg_val = trinary_and_char(a);
    //qDebug() << "A: " << a_reg_val;

    emit registers_changed(c_label, mem_c_xlat, d_mem(), a_reg_val);

    QList<QString>::const_iterator i;
    int idx;
    for (i = runtime_expressions.constBegin(), idx = 0; i != runtime_expressions.constEnd(); i++, idx++) {
        qDebug() << "Expression nr. " << idx << ": " << *i << " evaluated to "<<this->evaluate_runtime_expression(*i);
        emit runtime_expression_value_changed(this->evaluate_runtime_expression(*i), idx);
    }
}

QString QMalbolgeDebugger::d_mem() {
    int instr = this->get_instruction(memory[c],c);
    if (instr==MALBOLGE_JMP || instr==MALBOLGE_MOVD) {
        int inside_codesection = (instr == MALBOLGE_JMP?1:0);
        int offset = 0;
        QString label = address_to_label(memory[d]+1, inside_codesection, offset);
        offset--;
        QString offset_str = "";
        if (offset != 0) {
            offset_str = (offset>0?QString(" + "):QString(" - ")) + QString::number(offset>0?offset:-offset);
        }
        QString d_mem = label + offset_str;
        return d_mem;
    }
    if (instr==MALBOLGE_OPR || instr==MALBOLGE_ROT) {
        return trinary_and_char(memory[d]);
    }
    if (instr==MALBOLGE_IN || instr==MALBOLGE_OUT || instr==MALBOLGE_HALT) {
        return "?-";
    }
    // nop instruction; TODO: check if xlat-cycle contains exactly one non-nop instruction
    return "?-";

}

void QMalbolgeDebugger::reading_paused() {
    // handle break while reading...
    if (!pause_emitted) {
        this->emit_pause_state();
        reading_was_paused = true;
    }

    // TODO: diesen methodenaufruf irgendwo merken, denn wenn als nächstes ein einzelner STEP ausgeführt werden soll,
    // sollte execute_single_step nicht gesetzt werden, sondern stattdessen nur set_can_read(true); aufgerufen werden!!!
}

QString QMalbolgeDebugger::address_to_label(int address, int &inside_codesection, int &offset) {
    bool success = false;
    QString result;
    bool is_inside_codesection;
    if (address < 0 || address > 59048) {
        inside_codesection = -1;
        offset = 0;
        return QString("(unknown)");
    }
    success = debug_infos.get_address_label(address, result, is_inside_codesection);
    if (success && (inside_codesection == -1 || ((inside_codesection!=0)==is_inside_codesection))) {
        inside_codesection = (is_inside_codesection?1:0);
        offset = 0;
        return result;
    }
    if (inside_codesection > 0) {
        success = debug_infos.get_address_label((address-1)%59049, result, is_inside_codesection);
        if (success && is_inside_codesection) {
            offset = 0;
            result = "R_" + result;
            return result;
        }
    }
    for (int i=0;i<59049;i++) {
        int test_address = address + i;
        test_address = test_address%59049;

        success = debug_infos.get_address_label(test_address, result, is_inside_codesection);
        if (success && (inside_codesection == -1 || ((inside_codesection!=0)==is_inside_codesection))) {
            inside_codesection = (is_inside_codesection?1:0);
            offset = -i;
            return result;
        }
    }
    inside_codesection = -1;
    offset = 0;
    return QString("(unknown)");
}


QString QMalbolgeDebugger::evaluate_runtime_expression(QString expression) {
    // parse expression
    int result = 0;
    MemoryCellParser::DisplayType display;
    bool success = MemoryCellParser::parse_expression(expression, this->memory, &this->debug_infos, result, display);
    if (!success)
        return QString("invalid expression");
    QString ret;
    switch (display.type) {
        case MemoryCellParser::DisplayType::DECIMAL:
            ret = QString::number(result, 10);
            break;
        case MemoryCellParser::DisplayType::TRINARY:
        case MemoryCellParser::DisplayType::UNSET: // TODO
            ret = this->trinary_and_char(result);
            break;
        case MemoryCellParser::DisplayType::LABEL:
        case MemoryCellParser::DisplayType::STRINGPTR:
            // TODO
            ret = QString("not implemented yet");
            break;
        default:
            ret = QString("internal error");
            break;
    }
    return ret;
}

void QMalbolgeDebugger::pause() {
    running_until_breakpoint = false;
    execute_single_step = false;
    set_can_read(false);
}

void QMalbolgeDebugger::add_breakpoint(int line) {
    int memory_cell = 0;
    bool inside_codesection = false;
    if (debug_infos.get_first_memory_cell_starting_at_line(line, memory_cell, &inside_codesection)>=0) {
        breakpoints.insert(memory_cell,QPair<int, bool>(line, inside_codesection));
    }
}

void QMalbolgeDebugger::remove_breakpoint(int line) {
    if (line < 0)
        return; // error

    QHash<unsigned int, QPair<int, bool> >::iterator i = breakpoints.begin();
    while (i != breakpoints.end()) {
        QPair<int, bool> bp = i.value();
        if (bp.first == line) {
            i = breakpoints.erase(i);
        }else{
            i++;
        }
    }
}

void QMalbolgeDebugger::register_runtime_expression(QString expression, int expression_id) {
    if (expression_id < 0) {
        // invalid param!
        return;
    }
    if (expression_id < runtime_expressions.size()) {
        runtime_expressions.replace(expression_id, expression);
    }else if (expression_id == runtime_expressions.size()) {
        runtime_expressions.append(expression);
    } else {
        // error: missing elements!
        // insert dummy elements...
        for (int i=runtime_expressions.size();i<expression_id;i++) {
            runtime_expressions.append(QString());
        }
        runtime_expressions.append(expression);
    }
    if (pause_emitted) {
        emit runtime_expression_value_changed(this->evaluate_runtime_expression(expression), expression_id);
    }
}

void QMalbolgeDebugger::remove_runtime_expression(int expression_id) {
    if (expression_id < 0 || expression_id >= runtime_expressions.size())
        return; // invalid expression
    runtime_expressions.removeAt(expression_id);
}

void QMalbolgeDebugger::clear_runtime_expressions() {
    runtime_expressions.clear();
}

