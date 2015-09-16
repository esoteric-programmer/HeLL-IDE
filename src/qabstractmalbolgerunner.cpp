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

#include "qabstractmalbolgerunner.h"
#include "qsleep.h"
#include "malbolgedefinitions.h"
#include <QRegExp>
#include <QFile>
#include <QTextStream>
#include <QTextCodec>

QAbstractMalbolgeRunner::QAbstractMalbolgeRunner(QString cmd, QStringList arguments, QObject *parent) :
    QIOProcessWorker(cmd, arguments, parent)
{
    abort_reading_flag = false;
    malbolge_started = false;
}

int QAbstractMalbolgeRunner::run() {
    int assembly = this->executeProcess();

    if (assembly != 0)
        return assembly;


    QRegExp re(QString("[\\r\\n]Malbolge code written to ([^\\r\\n]*)[\\r\\n]"));
    int match = re.indexIn(this->child_process_stdout);

    QString malbolge_file;
    if (match >= 0)
        malbolge_file = re.cap(1);
    else {
        malbolge_file = this->malbolge_filename;
        if (malbolge_file.length() <= 0)
            return -1;
    }
    symbols_printed_since_last_sleep = 0;
    return execute_malbolge(malbolge_file);
}


const char* QAbstractMalbolgeRunner::translation = "5z]&gqtyfr$(we4{WP)H-Zn,[%\\3dL+Q;>U!pJS72FhOA1CB6v^=I_0/8|jsb9m<.TVac`uY*MK'X~xDl}REokN:#?G\"i@";

unsigned int QAbstractMalbolgeRunner::crazy(unsigned int a, unsigned int d){
    unsigned int crz[] = {1,0,0,1,0,2,2,2,1};
    int position = 0;
    unsigned int output = 0;
    while (position < 10){
        unsigned int i = a%3;
        unsigned int j = d%3;
        unsigned int out = crz[i+3*j];
        unsigned int multiple = 1;
        int k;
        for (k=0;k<position;k++)
            multiple *= 3;
        output += multiple*out;
        a /= 3;
        d /= 3;
        position++;
    }
    return output;
}

unsigned int QAbstractMalbolgeRunner::rotateR(unsigned int d){
    unsigned int carry = d%3;
    d /= 3;
    d += 19683 * carry;
    return d;
}



void QAbstractMalbolgeRunner::char_out(char out) {
    this->output_buffer.append(out);
    while (this->output_buffer.size() > 0) {
        if (symbols_printed_since_last_sleep > 0) {
            QSleep::msleep(20);
            symbols_printed_since_last_sleep = 0;
        }
        if ((this->output_buffer.at(0) & 0x80) == 0) {
            this->out(QString::fromUtf8(output_buffer.left(1)));
            symbols_printed_since_last_sleep++;
            this->output_buffer.remove(0, 1);
        } else if ((this->output_buffer.at(0) & 0x40) == 0) {
            // invalid coding
            this->err("[Invalid UTF-8 character]");
            symbols_printed_since_last_sleep++;
            this->output_buffer.remove(0, 1);
        } else if ((this->output_buffer.at(0) & 0x20) == 0) {
            if (this->output_buffer.size() < 2)
                break;
            this->out(QString::fromUtf8(output_buffer.left(2)));
            symbols_printed_since_last_sleep++;
            this->output_buffer.remove(0, 2);
        } else if ((this->output_buffer.at(0) & 0x10) == 0) {
            if (this->output_buffer.size() < 3)
                break;
            this->out(QString::fromUtf8(output_buffer.left(3)));
            symbols_printed_since_last_sleep++;
            this->output_buffer.remove(0, 3);
        } else if ((this->output_buffer.at(0) & 0x08) == 0) {
            if (this->output_buffer.size() < 4)
                break;
            this->out(QString::fromUtf8(output_buffer.left(4)));
            symbols_printed_since_last_sleep++;
            this->output_buffer.remove(0, 4);
        } else {
            // invalid coding
            this->err("[Invalid UTF-8 character]");
            symbols_printed_since_last_sleep++;
            this->output_buffer.remove(0, 1);
        }
    }
}

void QAbstractMalbolgeRunner::setMalbolgeFileName(QString filename) {
    this->malbolge_filename = filename;
}

void QAbstractMalbolgeRunner::clear_output_buffer() {
    this->output_buffer = QByteArray();
}

void QAbstractMalbolgeRunner::set_abort_reading(bool value) {
    this->abort_reading_flag = value;
}

int QAbstractMalbolgeRunner::load_malbolge_program(QString filename) {
    this->clear_output_buffer();

    QFile file(filename);
    if(!file.open(QIODevice::ReadOnly)) {
        this->err(QString("Cannot open file ").append(filename).append(": ").append(file.errorString()).append("\n"));
        return -1;
    }

    QTextStream in(&file);

    a=0;
    c=0;
    d=0;

    while (!in.atEnd() && d < 59050){
        unsigned int instr;
        memory[d] = 0;
        QString result = in.read(1);
        if (result.length() > 1)
            return -1;
        if (result.length() == 0)
            break;
        memory[d] = result.toUtf8().at(0);
        if (memory[d] == 0x1a || memory[d] == 0x04)
            break;
        instr = (memory[d] + d)%94;
        if (memory[d]==' ' || memory[d] == '\t' || memory[d] == '\r' || memory[d] == '\n');
        else if (memory[d] >= 33 && memory[d] < 127 && (instr == 4 || instr == 5 || instr == 23 || instr == 39 || instr == 40 || instr == 62 || instr == 68 || instr == 81))
            d+=result.length();
        else{
            this->err(QString("Invalid character %1 at %2.\n").arg(memory[d]).arg(d));
            return -1; //invalid characters are not accepted.
            //that makes the "hacked" in-out-program unrunnable
        }
    }
    file.close();
    if (d == 59050) {
        this->err("Maximum program length of 59049 exceeded.\n");
        return -1;
    }
    if (d < 2) {
        this->err("Minimal program length of 2 deceeded.\n");
        return -1;
    }

    while (d < 59049){
        memory[d] = crazy(memory[d-1], memory[d-2]);
        d++;
    }
    d = 0;
    return 0;
}

int QAbstractMalbolgeRunner::get_instruction(unsigned int memory_c, unsigned int pos) {
    if (memory_c < 33 || memory_c > 126) {
        return -1;
    }
    int instruction = (memory_c+pos)%94;
    switch (instruction){
        case MALBOLGE_JMP:
        case MALBOLGE_OUT:
        case MALBOLGE_IN:
        case MALBOLGE_ROT:
        case MALBOLGE_MOVD:
        case MALBOLGE_OPR:
        case MALBOLGE_HALT:
        case MALBOLGE_NOP:
            return instruction;
        default:
            return MALBOLGE_NOP;
    }
}

QString QAbstractMalbolgeRunner::instructionname(int instruction) {
        switch (instruction){
            case MALBOLGE_JMP:
                return "Jmp";
            case MALBOLGE_OUT:
                return "Out";
            case MALBOLGE_IN:
                return "In";
            case MALBOLGE_ROT:
                return "Rot";
            case MALBOLGE_MOVD:
                return "MovD";
            case MALBOLGE_OPR:
                return "Opr";
            case MALBOLGE_HALT:
                return "Hlt";
            case MALBOLGE_NOP:
                return "Nop";
            default:
                return "invalid";
        }
}

void QAbstractMalbolgeRunner::translate(unsigned int& memory_c) {
    // if memory[c] has been modified by the command above bring it into valid range
    if (memory_c < 33)
        memory_c += 94;
    memory_c-=33;
    if (memory_c > 93)
        memory_c %= 94;
    // encrypt command
    memory_c = translation[memory_c];
}

int QAbstractMalbolgeRunner::execute_malbolge_step(int &executed_command) {
    int instruction = get_instruction(memory[c],c);
    if (instruction < 0) {
        this->err(QString("Invalid command %1 at %2.\n").arg(memory[c]).arg(c));
        return -1;
    }
    executed_command = instruction;
    switch (instruction){
        case MALBOLGE_JMP:
            c = memory[d];
            break;
        case MALBOLGE_OUT:
            {
                this->char_out((char)a);
            }
            break;
        case MALBOLGE_IN:
            {
                char read = 0;
                bool result = this->in(read, &abort_reading_flag);
                a = (int)((unsigned char)read);
                if (!result)
                    a = 59048;
            }
            break;
        case MALBOLGE_ROT:
            a = (memory[d] = rotateR(memory[d]));
            break;
        case MALBOLGE_MOVD:
            d = memory[d];
            break;
        case MALBOLGE_OPR:
            a = (memory[d] = crazy(a, memory[d]));
            break;
        case MALBOLGE_HALT:
            return 1;
        case MALBOLGE_NOP:
        default:
            break;
    }

    translate(memory[c]);


    c = (c+1)%59049;
    d = (d+1)%59049;

    return 0;
}

void QAbstractMalbolgeRunner::set_malbolge_started() {
    malbolge_started = true;
}

QString QAbstractMalbolgeRunner::get_termination_message() {
    if (malbolge_started)
        return QString("Malbolge program terminated.\n");
    else
        return QIOProcessWorker::get_termination_message();
}
