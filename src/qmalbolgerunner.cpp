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

#include "qmalbolgerunner.h"
#include "qsleep.h"
#include <QRegExp>
#include <QFile>
#include <QTextStream>
#include <QTextCodec>

QMalbolgeRunner::QMalbolgeRunner(QString cmd, QStringList arguments, QObject *parent) :
    QAbstractMalbolgeRunner(cmd, arguments, parent)
{
}


int QMalbolgeRunner::execute_malbolge(QString filename) {

    this->info(QString("Executing ").append(filename).append("...\n"));
    int i=0;

    int ret = load_malbolge_program(filename);
    if (ret != 0)
        return ret;

    this->set_malbolge_started();

    // execute program
    while (!this->should_terminate()) {
        int command;
        int step = execute_malbolge_step(command);
        if (step < 0)
            return step; // ERROR
        else if (step > 0)
            break; // TERMINATED NORMALLY
        if (++i > 1000000) {
            i = 0;
            this->flush_out_buffer();
        }
    }
    this->clear_output_buffer(true);
    this->flush_out_buffer();
    return 0;
}

