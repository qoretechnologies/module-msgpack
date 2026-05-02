/* -*- mode: c++; indent-tabs-mode: nil -*- */
/*
  msgpack_pack.cpp

  Qore MessagePack module

  Copyright (C) 2018 Qore Technologies, s.r.o.

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

#include "msgpack_pack.h"

// std
#include <cstddef>
#include <cstdint>
#include <memory>

// qore
#include "qore/qore_bitopts.h"

// module sources
#include "msgpack_enums.h"
#include "msgpack_extensions.h"
#include "MsgPackException.h"
#include "QC_MsgPackExtension.h"

namespace msgpack {
namespace intern {

//-------------------------------------
// Qore nodes/values writing functions
//-------------------------------------

void msgpack_pack_qore_binary(mpack_writer_t* writer, const BinaryNode* value) {
    msgpack_pack_binary(writer, static_cast<const char*>(value->getPtr()), value->size());
}

void msgpack_pack_qore_bool(mpack_writer_t* writer, QoreValue value) {
    msgpack_pack_bool(writer, value.getAsBool());
}

void msgpack_pack_qore_date(mpack_writer_t* writer, const DateTimeNode* value, OperationMode mode) {
    switch (mode) {
        case MSGPACK_SIMPLE_MODE: {
            if (value->isAbsolute()) {
                msgpack_pack_ext_timestamp(writer, value);
            }
            else {
                QoreString str;
                value->format(str, "IF");
                msgpack_pack_utf8(writer, str.c_str(), static_cast<uint32_t>(str.size()));
            }
            break;
        }
        case MSGPACK_QORE_MODE:
            msgpack_pack_ext_date(writer, value);
            break;
        default:
            break;
    }
}

void msgpack_pack_qore_float(mpack_writer_t* writer, QoreValue value) {
    msgpack_pack_double(writer, value.getAsFloat());
}

void msgpack_pack_qore_hash(mpack_writer_t* writer, const QoreHashNode* value, OperationMode mode, ExceptionSink* xsink) {
    qore_size_t size = value->size();

    // start map writing
    mpack_start_map(writer, static_cast<uint32_t>(size));

    // write map elements
    HashIterator it(const_cast<QoreHashNode*>(value));
    while (it.next()) {
        // write key
        std::unique_ptr<QoreString> key(it.getKeyString());
        msgpack_pack_qore_string(writer, key.get(), mode, xsink);

        // write value
        msgpack_pack_qore_value(writer, it.get(), mode, xsink);
    }

    // finish map writing
    mpack_finish_map(writer);
}

void msgpack_pack_qore_int(mpack_writer_t* writer, QoreValue value) {
    msgpack_pack_int(writer, value.getAsBigInt());
}

void msgpack_pack_qore_list(mpack_writer_t* writer, QoreValue value, OperationMode mode, ExceptionSink* xsink) {
    QoreListNode* listNode = value.get<QoreListNode>();
    size_t size = listNode->size();

    // start array writing
    mpack_start_array(writer, static_cast<uint32_t>(size));

    // write array elements
    for (size_t i = 0; i < size; i++) {
        msgpack_pack_qore_value(writer, listNode->retrieveEntry(i), mode, xsink);
    }

    // finish array writing
    mpack_finish_array(writer);
}

void msgpack_pack_qore_nothing(mpack_writer_t* writer) {
    msgpack_pack_nil(writer);
}

void msgpack_pack_qore_null(mpack_writer_t* writer, OperationMode mode) {
    switch (mode) {
        case MSGPACK_SIMPLE_MODE:
            msgpack_pack_nil(writer); break;
        case MSGPACK_QORE_MODE:
            msgpack_pack_ext_null(writer); break;
        default:
            break;
    }
}

void msgpack_pack_qore_number(mpack_writer_t* writer, const QoreNumberNode* value, OperationMode mode) {
    switch (mode) {
        case MSGPACK_SIMPLE_MODE: {
            msgpack_pack_double(writer, value->getAsFloat());
            break;
        }
        case MSGPACK_QORE_MODE:
            msgpack_pack_ext_number(writer, value);
            break;
        default:
            break;
    }
}

void msgpack_pack_qore_string(mpack_writer_t* writer, const QoreString* value, OperationMode mode, ExceptionSink* xsink) {
    if (value->getEncoding() == QCS_UTF8) {
        msgpack_pack_utf8(writer, value->c_str(), value->size());
    }
    else {
        switch (mode) {
            case MSGPACK_SIMPLE_MODE: {
                TempEncodingHelper temp(value, QCS_UTF8, xsink);
                if (xsink && *xsink)
                    mpack_writer_flag_error(writer, mpack_error_data);
                if (*temp)
                    msgpack_pack_utf8(writer, temp->c_str(), temp->size());
                break;
            }
            case MSGPACK_QORE_MODE:
                msgpack_pack_ext_string(writer, value);
                break;
            default:
                break;
        }
    }
}

void msgpack_pack_qore_value(mpack_writer_t* writer, QoreValue value, OperationMode mode, ExceptionSink* xsink) {
    switch (value.getType()) {
        case NT_BINARY:                     // BinaryNode
            msgpack_pack_qore_binary(writer, value.get<const BinaryNode>()); break;
        case NT_BOOLEAN:                    // bool
            msgpack_pack_qore_bool(writer, value); break;
        case NT_DATE:                       // DateTimeNode
            msgpack_pack_qore_date(writer, value.get<const DateTimeNode>(), mode); break;
        case NT_FLOAT:                      // double
            msgpack_pack_qore_float(writer, value); break;
        case NT_HASH:                       // QoreHashNode
            msgpack_pack_qore_hash(writer, value.get<const QoreHashNode>(), mode, xsink); break;
        case NT_INT:                        // int64 (long long)
            msgpack_pack_qore_int(writer, value); break;
        case NT_LIST:                       // QoreListNode
            msgpack_pack_qore_list(writer, value, mode, xsink); break;
        case NT_NOTHING:                    // QoreNothingNode
            msgpack_pack_qore_nothing(writer); break;
        case NT_NULL:                       // QoreNullNode
            msgpack_pack_qore_null(writer, mode); break;
        case NT_NUMBER:                     // QoreNumberNode
            msgpack_pack_qore_number(writer, value.get<const QoreNumberNode>(), mode); break;
        case NT_OBJECT: {
            const QoreObject* obj = value.get<QoreObject>();
            if (obj->getClass(CID_MSGPACKEXTENSION)) {
                PrivateDataRefHolder<MsgPackExtension> holder(obj, CID_MSGPACKEXTENSION, xsink);
                MsgPackExtension* ext = *holder;
                if (ext)
                    msgpack_pack_ext_ext(writer, ext);
                break;
            }
            throw msgpack::MsgPackExceptionMaker("serializing objects is not supported (class: '%s')", obj->getClassName());
        }
        case NT_STRING: {                   // QoreStringNode
            QoreStringNodeValueHelper str(value);
            msgpack_pack_qore_string(writer, *str, mode, xsink); break;
        }
        default: {
            throw msgpack::MsgPackExceptionMaker("serializing values of type '%s' is not supported", value.getTypeName());
        }
    }
}


//-----------------------
// msgpack_pack function
//-----------------------

QoreValue msgpack_pack(QoreValue& data, OperationMode mode, ExceptionSink* xsink) {
    size_t size = 0;
    char* buffer = nullptr;
    mpack_writer_t writer;

    // initialize writer
    mpack_writer_init_growable(&writer, &buffer, &size);

    // pack the data
    msgpack_pack_qore_value(&writer, data, mode, xsink);

    // finish writing
    mpack_error_t result = mpack_writer_destroy(&writer);
    if (result != mpack_ok) {
        throw msgpack::getMsgPackException(result);
    }

    // return a binary node
    QoreValue bin(new BinaryNode(buffer, size));
    return bin;
}

} // namespace intern
} // namespace msgpack
