/* -*- mode: c++; indent-tabs-mode: nil -*- */
/*
    MsgPack.h

    Qore MessagePack module

    Copyright (C) 2018 - 2026 Qore Technologies, s.r.o.

    Permission is hereby granted, free of charge, to any person obtaining a
    copy of this software and associated documentation files (the "Software"),
    to deal in the Software without restriction, including without limitation
    the rights to use, copy, modify, merge, publish, distribute, sublicense,
    and/or sell copies of the Software, and to permit persons to whom the
    Software is furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in
    all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
    DEALINGS IN THE SOFTWARE.
*/

#ifndef _QORE_MODULE_MSGPACK_MSGPACK_H
#define _QORE_MODULE_MSGPACK_MSGPACK_H

// qore
#include "qore/Qore.h"

// module sources
#include "msgpack_enums.h"
#include "msgpack_pack.h"
#include "msgpack_unpack.h"
#include "MsgPackException.h"

namespace msgpack {

//! MessagePack class used for packing and unpacking data.
/**
    Serves as encapsulation for the msgpack module methods @ref msgpack::msgpack_pack "msgpack_pack()" and @ref msgpack::msgpack_unpack "msgpack_unpack()"
*/
class MsgPack : public AbstractPrivateData {
private:
    msgpack::OperationMode mode;

public:
    //! Constructor.
    DLLLOCAL MsgPack(msgpack::OperationMode m = msgpack::MSGPACK_SIMPLE_MODE) : mode(m) {}

    //! Get operation mode used for packing and unpacking.
    DLLLOCAL msgpack::OperationMode getOperationMode() const { return mode; }

    //! Set operation mode used for packing and unpacking.
    DLLLOCAL void setOperationMode(msgpack::OperationMode m) { mode = m; }

    //! Pack passed value into MessagePack format binary.
    DLLLOCAL QoreValue pack(ExceptionSink* xsink, QoreValue value) {
        try {
            QoreValue result(msgpack::intern::msgpack_pack(value, mode, xsink));
            if (xsink && *xsink)
                return QoreValue();
            return result;
        }
        catch (const msgpack::MsgPackException& ex) {
            ex.raise(xsink, "PACK-ERROR");
        }
        return QoreValue();
    }

    //! Unpack passed MessagePack data.
    DLLLOCAL QoreValue unpack(ExceptionSink* xsink, QoreValue value) {
        try {
            QoreValue result(msgpack::intern::msgpack_unpack(
                value.get<BinaryNode>(),
                mode,
                xsink
            ));
            if (xsink && *xsink)
                return QoreValue();
            return result;
        }
        catch (const msgpack::MsgPackException& ex) {
            ex.raise(xsink, "UNPACK-ERROR");
        }
        return QoreValue();
    }
};

} // namespace msgpack

#endif // _QORE_MODULE_MSGPACK_MSGPACK_H

