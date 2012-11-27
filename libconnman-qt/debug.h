/*
 * Copyright © 2012, Cercacor Labs.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 */

#ifndef DEBUG_H
#define DEBUG_H

#include <QDebug>

struct __Debug {
    __Debug(const char *_file, const char *_func, int _line)
        : file(_file), func(_func), line(_line) {}

    const char *file;
    const char *func;
    int line;
};

QDebug operator<<(QDebug lhs, const __Debug &rhs);

#define pr_dbg() qDebug() << __Debug(__FILE__, __func__, __LINE__)

#endif // DEBUG_H
