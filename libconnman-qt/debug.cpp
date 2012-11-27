/*
 * Copyright © 2012, Cercacor Labs.
 *
 * This program is licensed under the terms and conditions of the
 * Apache License, version 2.0.  The full text of the Apache License is at
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 */

#include "debug.h"

QDebug operator<<(QDebug lhs, const __Debug &rhs)
{
    lhs.nospace() << "libconnman-qt(" << rhs.file << ":" << rhs.func << ":" << rhs.line << ")";

    return lhs.space();
}

