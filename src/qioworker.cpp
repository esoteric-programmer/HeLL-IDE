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

#include "qioworker.h"
#include "qsleep.h"
#include <QThread>
#include <QCoreApplication>

#include <QMessageBox>

QIOWorker::QIOWorker(QObject *parent) :
    QObject(parent)
{
    this->thread = NULL;
    this->input_eof = false;
    this->can_read = true;
}

QIOWorker::~QIOWorker() {

}

void QIOWorker::StdInRead(QString in) {
    this->input_mutex.lock();
    if (!this->input_eof) {
        this->input_queue << in.toUtf8();
    }
    this->input_mutex.unlock();
}

void QIOWorker::StdInEOF() {
    this->input_mutex.lock();
    this->input_eof = true;
    this->input_mutex.unlock();
}


QString QIOWorker::get_termination_message() {
    return QString("Terminated.");
}


void QIOWorker::Execute() { // QMetaObject::invokeMethod
    this->thread_mutex.lock();
    if (this->thread != NULL) {
        // cannot start thread!
        emit Terminated(-1, QProcess::NormalExit);
        this->thread_mutex.unlock();
        return;
    }
    this->thread = QThread::currentThread();
    this->thread_mutex.unlock();
    this->terminate_mutex.lock();
    this->terminate = false;
    this->terminate_mutex.unlock();
    int code = this->run();
    this->thread_mutex.lock();
    this->thread = NULL;
    this->thread_mutex.unlock();
    emit Terminated(code, QProcess::NormalExit);
    QThread::currentThread()->exit(0);
}


void QIOWorker::Stop() {
    this->terminate_mutex.lock();
    this->terminate = true;
    this->terminate_mutex.unlock();
}

int QIOWorker::run() {

    while(!this->should_terminate()) {
        char input;
        if (this->in(input)) {
            this->out(QString("I read: '").append(QString(input)).append("'.\n"));
            this->flush_out_buffer();
        }
    }

    return 0;
}

void QIOWorker::out(QString out) {
    emit StdOutWrite(out);
}

void QIOWorker::out_buffered(QString out) {
    this->out_buffer.append(out);
}
void QIOWorker::flush_out_buffer() {
    if (this->out_buffer.length() > 0) {
        emit StdOutWrite(this->out_buffer);
        this->out_buffer.clear();
    }
}

void QIOWorker::info(QString out) {
    this->flush_out_buffer();
    emit InfoWrite(out);
}

void QIOWorker::err(QString out) {
    this->flush_out_buffer();
    emit StdErrWrite(out);
}


void QIOWorker::clear_inputqueue() {
    this->input_mutex.lock();
    this->input_queue.clear();
    this->input_mutex.unlock();

}

bool QIOWorker::in(char& input, volatile bool *termination_flag) {

    this->flush_out_buffer();
    while (1) {

        bool paused_notification_sent = false;

        if (this->should_terminate())
            return false;
        if (termination_flag != NULL)
            if ((volatile bool)(*termination_flag))
                return false;

        if (this->can_read) {
            paused_notification_sent = false;

            this->input_mutex.lock();
            if (this->input_eof) {
                this->input_mutex.unlock();
                return false;
            }

            if (this->input_queue.isEmpty()) {
                this->input_mutex.unlock();
                QCoreApplication::processEvents();
                QSleep::msleep(20);
                continue;
            }

            if (this->input_queue.first().length() <= 0) {
                this->input_queue.removeFirst();
                this->input_mutex.unlock();
                continue;
            }

            QByteArray string = this->input_queue.first();
            input = string.at(0);
            string.remove(0,1);
            this->input_queue.replace(0,string);
            this->input_mutex.unlock();
            return true;
        }else{
            if (!paused_notification_sent) {
                reading_paused();
                paused_notification_sent = true;
            }
            QCoreApplication::processEvents();
            QSleep::msleep(20);
            continue;
        }
    }
}

bool QIOWorker::should_terminate() {
    this->terminate_mutex.lock();
    bool ret = this->terminate;
    this->terminate_mutex.unlock();
    return ret;
}

void QIOWorker::set_can_read(bool value) {
    this->can_read = value;
}

bool QIOWorker::get_can_read() {
    return this->can_read;
}

void QIOWorker::reading_paused() {
    // do nothing...
}
