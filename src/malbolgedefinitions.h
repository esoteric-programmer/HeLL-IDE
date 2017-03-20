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

#ifndef MALBOLGEDEFINITIONS_H
#define MALBOLGEDEFINITIONS_H

#define MALBOLGE_C0 0
#define MALBOLGE_C1 29524
#define MALBOLGE_C20 59046
#define MALBOLGE_C21 59047
#define MALBOLGE_C2 59048
#define MALBOLGE_EOF MALBOLGE_C2

#define MALBOLGE_JMP 4
#define MALBOLGE_OUT 5
#define MALBOLGE_IN 23
#define MALBOLGE_ROT 39
#define MALBOLGE_MOVD 40
#define MALBOLGE_OPR 62
#define MALBOLGE_NOP 68
#define MALBOLGE_HALT 81

#endif // MALBOLGEDEFINITIONS_H
