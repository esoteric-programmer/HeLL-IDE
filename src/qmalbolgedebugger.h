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

#ifndef QMALBOLGEDEBUGGER_H
#define QMALBOLGEDEBUGGER_H

#include "qabstractmalbolgerunner.h"
#include "lmaodebuginformations.h"

class QMalbolgeDebugger : public QAbstractMalbolgeRunner
{
    Q_OBJECT
public:
    explicit QMalbolgeDebugger(QString cmd, QStringList arguments, QObject *parent = 0);
    void setDebugFileName(QString filename);

signals:
    void execution_paused(int first_line_d, int first_column_d, int last_line_d, int last_column_d, int first_line_c, int first_column_c, int last_line_c, int last_column_c, bool recommend_displaying_code_section);
    void registers_changed(QString c_label, QString mem_c_xlat, QString mem_d, QString a_reg_val);
    void runtime_expression_value_changed(QString value, int expression_id); // position_of_value may be -1. if not, it can be used to calculate xlat1...
    void active_xlat2_changed(QLinkedList<LMAODebugInformations::SourcePosition>);
    void execution_continued();
    void query_breakpoints();
    void clear_userinput();

public slots:
    void register_runtime_expression(QString expression, int expression_id);
    void remove_runtime_expression(int expression_id);
    void clear_runtime_expressions();
    void run_until_breakpoint();
    void step();
    void pause();
    void reset();
    void add_breakpoint(int line);
    void remove_breakpoint(int line);


protected:
    virtual int execute_malbolge(QString filename);
    virtual void reading_paused();


private:
    QString debug_filename;
    LMAODebugInformations debug_infos;

    bool running_until_breakpoint;
    bool execute_single_step;
    bool pause_emitted;
    bool reading_was_paused;
    bool force_reset;
    bool recommend_goto_codesection_on_pause;

    QString last_jmp_destination_label;
    int last_jmp_destination_label_offset;

    QList<QString> runtime_expressions;

    QHash<unsigned int, QPair<int, bool> > breakpoints; // memory address, (line, type: codesection?)

    QString evaluate_runtime_expression(QString expression);
    QString address_to_label(int address, int &inside_codesection, int &offset);
    QString malbolge_xlat_cycle(unsigned int memory_c, unsigned int pos);
    QString trinary(unsigned int value);
    QString trinary_and_char(unsigned int value);
    QString d_mem();
    void emit_pause_state();


};

#endif // QMALBOLGEDEBUGGER_H
