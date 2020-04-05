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

#ifndef QIOPROCESSWORKER_H
#define QIOPROCESSWORKER_H

#include "qioworker.h"

class QIOProcessWorker : public QIOWorker
{
    Q_OBJECT
public:
    explicit QIOProcessWorker(QString cmd, QStringList arguments, QObject *parent = 0);
    virtual int run();
    virtual ~QIOProcessWorker();

protected:
    int executeProcess();
    QString child_process_stdout;
    QString child_process_stderr;

private:
    QProcess* child_process;
    QString cmd;
    QStringList arguments;
    int child_exit_code;
    bool child_terminated;

signals:
    void AssemblyOutput(QString);

public slots:
    void OnChildStdOutWrite();
    void OnChildStdErrWrite();
    void OnChildTerminated(int exitCode, QProcess::ExitStatus exitStatus);
    void OnChildError(QProcess::ProcessError error);

};

#endif // QIOPROCESSWORKER_H
