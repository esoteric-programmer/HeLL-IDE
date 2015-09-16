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

#ifndef QMALBOLGERUNNER_H
#define QMALBOLGERUNNER_H

#include "qabstractmalbolgerunner.h"
#include <QByteArray>

class QMalbolgeRunner : public QAbstractMalbolgeRunner
{
    Q_OBJECT
public:
    explicit QMalbolgeRunner(QString cmd, QStringList arguments, QObject *parent = 0);

signals:

public slots:

protected:
    virtual int execute_malbolge(QString filename);


};

#endif // QMALBOLGERUNNER_H
