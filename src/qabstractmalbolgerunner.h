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

#ifndef QABSTRACTMALBOLGERUNNER_H
#define QABSTRACTMALBOLGERUNNER_H

#include "qioprocessworker.h"
#include <QByteArray>

class QAbstractMalbolgeRunner : public QIOProcessWorker
{
    Q_OBJECT
public:
    explicit QAbstractMalbolgeRunner(QString cmd, QStringList arguments, QObject *parent = 0);
    virtual int run();
    void setMalbolgeFileName(QString filename);
    virtual QString get_termination_message();
    static unsigned int crazy(unsigned int a, unsigned int d);
    static unsigned int rotateR(unsigned int d);

signals:

public slots:

protected:
    static const char* translation;

    unsigned int memory[59050];
    unsigned int a, c, d;

    virtual int execute_malbolge(QString filename)=0;
    int load_malbolge_program(QString filename);
    int execute_malbolge_step(int &executed_command);
    void char_out(char out);
    void clear_output_buffer(bool output_incomplete);
    void set_abort_reading(bool value);
    void set_malbolge_started();

    QString instructionname(int instruction);
    void translate(unsigned int& memory_c);
    int get_instruction(unsigned int memory_c, unsigned int pos);

private:
    QString malbolge_filename;
    //int symbols_printed_since_last_sleep;
    QByteArray output_buffer;
    volatile bool abort_reading_flag;
    volatile bool malbolge_started;

};

#endif // QABSTRACTMALBOLGERUNNER_H

