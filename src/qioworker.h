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

#ifndef QIOWORKER_H
#define QIOWORKER_H

//#include <QObject>
#include <QProcess>
#include <QMutex>

class QIOWorker : public QObject
{
    Q_OBJECT
public:
    explicit QIOWorker(QObject *parent = 0);
    void StdInRead(QString in);
    void StdInEOF();
    void Stop();
    virtual QString get_termination_message();
    virtual ~QIOWorker();

protected:
    virtual int run();
    void out(QString);
    void info(QString);
    void err(QString);
    bool in(char& input, volatile bool* termination_flag = 0);
    bool should_terminate();
    void set_can_read(bool value);
    bool get_can_read();
    virtual void reading_paused(); // set_can_read wurde auf false gesetzt, während auf Eingabe gewartet wird...
    void clear_inputqueue();

private:
    QMutex input_mutex;
    QList<QByteArray> input_queue;
    bool input_eof;
    QMutex thread_mutex;
    QThread* thread;
    QMutex terminate_mutex;
    bool terminate;
    bool can_read;


signals:
    void StdOutWrite(QString out);
    void InfoWrite(QString out);
    void StdErrWrite(QString out);
    void Terminated(int exitCode, QProcess::ExitStatus exitStatus);

public slots:
    void Execute(); // QMetaObject::invokeMethod

};

#endif // QIOWORKER_H
