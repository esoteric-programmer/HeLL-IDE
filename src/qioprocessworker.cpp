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


#include "qioprocessworker.h"
#include <QProcess>
#include <QCoreApplication>
#include <QByteArray>

QIOProcessWorker::QIOProcessWorker(QString cmd, QStringList arguments, QObject *parent) :
    QIOWorker(parent)
{
    this->child_exit_code = 0;
    this->child_process = NULL;
    this->cmd = cmd;
    this->arguments << arguments;
}

QIOProcessWorker::~QIOProcessWorker() {

}

int QIOProcessWorker::run() {

    return this->executeProcess();

}

int QIOProcessWorker::executeProcess() {
    if (this->cmd.length() <= 0)
        return 0;
    this->child_process = new QProcess(this);
    if (this->child_process != NULL) {

        this->child_terminated = false;

        connect(this->child_process, SIGNAL(readyReadStandardOutput()), this, SLOT(OnChildStdOutWrite()));
        connect(this->child_process, SIGNAL(readyReadStandardError()), this, SLOT(OnChildStdErrWrite()));
        connect(this->child_process, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(OnChildTerminated(int, QProcess::ExitStatus)));
        connect(this->child_process, SIGNAL(error(QProcess::ProcessError)), this, SLOT(OnChildError(QProcess::ProcessError)));

        this->child_process->start(cmd, arguments, QIODevice::ReadWrite);



        while (!this->should_terminate() && !this->child_terminated) {
            char input;
            if (this->in(input, &this->child_terminated)) {
                if (this->child_process->isWritable() && this->child_process->state() == QProcess::Running)
                    this->child_process->write(&input, 1);
            }
            QCoreApplication::processEvents();
        }

        while (!this->child_terminated) {
            this->child_process->kill();
            QCoreApplication::processEvents();
        }

        delete this->child_process;
        this->child_process = NULL;

        QRegExp re(QString("[\\r\\n]Malbolge code written to (.*)[\\r\\n]"));
        int match = re.indexIn(this->child_process_stdout);
        if (match >= 0) {
            emit AssemblyOutput(re.cap(1));
        }

        return this->child_exit_code;
    }else
        return -1;
}

void QIOProcessWorker::OnChildStdOutWrite() {
    if (this->child_process != NULL) {
        QString szOutput = this->child_process->readAllStandardOutput();
        child_process_stdout.append(szOutput);
        this->out(szOutput);
    }
}

void QIOProcessWorker::OnChildStdErrWrite() {
    if (this->child_process != NULL) {
        QString szOutput = this->child_process->readAllStandardError();
        child_process_stderr.append(szOutput);
        this->err(szOutput);
    }
}

void QIOProcessWorker::OnChildError(QProcess::ProcessError error) {
    switch (error) {
        case QProcess::FailedToStart:
            this->err("The process failed to start.\n");
            this->child_exit_code = -1;
            this->child_terminated = true;
            return;
        case QProcess::Crashed:
            this->err("The process crashed.\n");
            this->child_exit_code = -1;
            this->child_terminated = true;
            return;
        case QProcess::WriteError:
            this->err("Failed to pass input to the process.\n");
            return;
        case QProcess::ReadError:
            this->err("Failed to read the process output.\n");
            return;
        default:
            this->err("An unknown error occured.\n");
            if (this->child_process != NULL)
                if (this->child_process->state() == QProcess::Running)
                    return;
            this->child_exit_code = -1;
            this->child_terminated = true;
            return;
    }
}

void QIOProcessWorker::OnChildTerminated(int exitCode, QProcess::ExitStatus exitStatus) {
    if (this->child_process != NULL) {
        this->child_exit_code = exitCode;
        if (exitStatus == QProcess::CrashExit)
            this->child_exit_code = -1;
    }else
        this->child_exit_code = -1;
    this->child_terminated = true;
}

