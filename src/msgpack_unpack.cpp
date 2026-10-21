/* -*- mode: c++; indent-tabs-mode: nil -*- */
/*
    msgpack_unpack.cpp

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

#include "msgpack_unpack.h"

// std
#include <cassert>
#include <climits>
#include <cstdint>
#include <cstring>

// module sources
#include "msgpack_extensions.h"
#include "MsgPackException.h"
#include "MsgPackExtension.h"
#include "QC_MsgPackExtension.h"

namespace msgpack {
namespace intern {

//! Destroys an MPack reader when it goes out of scope
/** If the reader was not destroyed with destroy(), such as when a C++ exception is thrown while unpacking, the
    reader is put in an error state first, so its tracking data is freed without an incomplete-read check.
*/
class MpackReaderHelper {
public:
    DLLLOCAL MpackReaderHelper(mpack_reader_t& reader) : reader(reader) {
    }

    DLLLOCAL ~MpackReaderHelper() {
        if (!destroyed) {
            mpack_reader_flag_error(&reader, mpack_error_data);
            mpack_reader_destroy(&reader);
        }
    }

    //! destroys the reader and returns its error state
    DLLLOCAL mpack_error_t destroy() {
        destroyed = true;
        return mpack_reader_destroy(&reader);
    }

private:
    mpack_reader_t& reader;
    bool destroyed = false;
};

//! Returns the message for a map key with a NUL character, which a Qore hash key cannot hold
/** The key is shown with at most its first 64 characters, with control characters, quotes and backslashes
    escaped, so that the NUL character and where it is can be seen.
*/
static std::string get_nul_key_desc(const QoreString& key, const char* nul) {
    static constexpr size_t max_chars = 64;
    const char* p = key.c_str();
    size_t len = key.size();
    bool invalid = false;
    size_t show = key.getEncoding()->getByteLen(p, p + len, max_chars, invalid);
    if (invalid || show > len) {
        show = len < max_chars ? len : max_chars;
    }
    QoreString desc("map key \"");
    for (size_t i = 0; i < show; ++i) {
        unsigned char c = static_cast<unsigned char>(p[i]);
        if (c < 0x20 || c == 0x7f) {
            desc.sprintf("\\x%02x", c);
        } else if (c == '"' || c == '\\') {
            desc.concat('\\');
            desc.concat(static_cast<char>(c));
        } else {
            desc.concat(static_cast<char>(c));
        }
    }
    if (show < len) {
        desc.concat("...");
    }
    desc.sprintf("\" contains a NUL character at byte offset %zu, which a Qore hash key cannot hold",
        static_cast<size_t>(nul - p));
    return desc.c_str();
}

QoreListNode* msgpack_unpack_array(mpack_reader_t* reader, mpack_tag_t tag, OperationMode mode, ExceptionSink* xsink) {
    // prepare list
    ReferenceHolder<QoreListNode> list(new QoreListNode, xsink);

    // read all elements
    uint32_t size = mpack_tag_array_count(&tag);
    for (uint32_t i = 0; i < size; i++) {
        list->push(msgpack_unpack_value(reader, mode, xsink), xsink);
    }

    mpack_done_array(reader);
    return list.release();
}

BinaryNode* msgpack_unpack_binary(mpack_reader_t* reader, mpack_tag_t tag, ExceptionSink* xsink) {
    uint32_t size = mpack_tag_bin_length(&tag);

    // prepare binary node
    SimpleRefHolder<BinaryNode> bin(new BinaryNode);
    bin->preallocate(size);

    // read binary
    mpack_read_bytes(reader, static_cast<char*>(const_cast<void*>(bin->getPtr())), size);

    mpack_done_bin(reader);
    return bin.release();
}

AbstractQoreNode* msgpack_unpack_ext(mpack_reader_t* reader, mpack_tag_t tag, OperationMode mode, ExceptionSink* xsink) {
    switch (mode) {
        case MSGPACK_SIMPLE_MODE: {
            // handle MessagePack built-in extension types first
            if (mpack_tag_ext_exttype(&tag) == MPACK_EXTTYPE_TIMESTAMP)
                return msgpack_unpack_ext_timestamp(reader, tag, xsink);

            // otherwise unpack as an extension object
            uint32_t size = mpack_tag_ext_length(&tag);
            SimpleRefHolder<BinaryNode> bin(new BinaryNode);
            bin->preallocate(size);
            mpack_read_bytes(reader, (char*) bin->getPtr(), size);
            mpack_done_ext(reader);
            return new QoreObject(QC_MSGPACKEXTENSION, getProgram(), new MsgPackExtension(mpack_tag_ext_exttype(&tag), bin.release()));
        }
        case MSGPACK_QORE_MODE: {
            switch (mpack_tag_ext_exttype(&tag)) {
                // MessagePack built-in types
                case MPACK_EXTTYPE_TIMESTAMP:
                    return msgpack_unpack_ext_timestamp(reader, tag, xsink);

                // Qore extension types
                case MSGPACK_EXT_QORE_NULL:
                    return msgpack_unpack_ext_null(reader, tag, xsink);
                case MSGPACK_EXT_QORE_DATE:
                    return msgpack_unpack_ext_date(reader, tag, xsink);
                case MSGPACK_EXT_QORE_NUMBER:
                    return msgpack_unpack_ext_number(reader, tag, xsink);
                case MSGPACK_EXT_QORE_STRING:
                    return msgpack_unpack_ext_string(reader, tag, xsink);
                default:
                    mpack_reader_flag_error(reader, mpack_error_data);
                    break;
            }
            break;
        }
        default:
            break;
    }

    return nullptr;
}

QoreHashNode* msgpack_unpack_map(mpack_reader_t* reader, mpack_tag_t tag, OperationMode mode, ExceptionSink* xsink) {
    // prepare hash
    ReferenceHolder<QoreHashNode> hash(new QoreHashNode, xsink);

    // read all elements
    uint32_t size = mpack_tag_map_count(&tag);
    for (uint32_t i = 0; i < size; i++) {
        // read key and value
        ValueHolder key(msgpack_unpack_value(reader, mode, xsink), xsink);
        ValueHolder value(msgpack_unpack_value(reader, mode, xsink), xsink);

        // check key and value
        if (!(*key || *value) || key->getType() != NT_STRING) {
            mpack_reader_flag_error(reader, mpack_error_data);
            break;
        }

        // a hash key is stored in the default encoding; a key in another encoding (a Qore string extension in Qore
        // mode) names the same key as the equivalent string in the default encoding
        QoreStringValueHelper key_val(*key);
        TempEncodingHelper key_str(*key_val, QCS_DEFAULT, xsink);
        if (*xsink) {
            // stop reading; the conversion error explains the failure
            mpack_reader_flag_error(reader, mpack_error_data);
            break;
        }

        // a hash key cannot hold a NUL character; the key would be cut at the NUL and name another key
        const char* nul = static_cast<const char*>(memchr(key_str->c_str(), '\0', key_str->size()));
        if (nul) {
            throw MsgPackException(get_nul_key_desc(**key_str, nul).c_str());
        }

        // add element to hash
        hash->setKeyValue(key_str->c_str(), value.release(), xsink);
    }

    mpack_done_map(reader);
    return hash.release();
}

QoreStringNode* msgpack_unpack_string(mpack_reader_t* reader, mpack_tag_t tag, ExceptionSink* xsink) {
    uint32_t size = mpack_tag_str_length(&tag);

    // prepare string node and buffer for reading
    SimpleRefHolder<QoreStringNode> str(new QoreStringNode);
    str->allocate(size+1);
    qore_size_t allocated = str->capacity();
    char* buffer = str->giveBuffer();

    // read string
    mpack_read_utf8(reader, buffer, size);
    buffer[size] = '\0';
    str->set(buffer, size, allocated, QCS_UTF8);

    mpack_done_str(reader);
    return str.release();
}


QoreValue msgpack_unpack_value(mpack_reader_t* reader, OperationMode mode, ExceptionSink* xsink) {
    mpack_tag_t tag = mpack_read_tag(reader);

    switch (mpack_tag_type(&tag)) {
        case mpack_type_array:
            return msgpack_unpack_array(reader, tag, mode, xsink);
        case mpack_type_bin:
            return msgpack_unpack_binary(reader, tag, xsink);
        case mpack_type_bool:
            return mpack_tag_bool_value(&tag);
        case mpack_type_double:
            return mpack_tag_double_value(&tag);
        case mpack_type_ext:
            return msgpack_unpack_ext(reader, tag, mode, xsink);
        case mpack_type_float:
            return mpack_tag_float_value(&tag);
        case mpack_type_int:
            return mpack_tag_int_value(&tag);
        case mpack_type_map:
            return msgpack_unpack_map(reader, tag, mode, xsink);
        case mpack_type_nil:
            return QoreValue();
        case mpack_type_str:
            return msgpack_unpack_string(reader, tag, xsink);
        case mpack_type_uint: {
            uint64_t val = mpack_tag_uint_value(&tag);
            if (val <= LLONG_MAX)
                return static_cast<int64>(val);
            return new QoreNumberNode(static_cast<double>(val));
        }
        default:
            mpack_reader_flag_error(reader, mpack_error_data);
            break;
    }
    return QoreValue();
}


//-------------------------
// msgpack_unpack function
//-------------------------

QoreValue msgpack_unpack(const BinaryNode* data, OperationMode mode, ExceptionSink* xsink) {
    assert(xsink);
    ValueHolder unpacked(xsink);
    const char* dataCheck = nullptr;
    const char* buffer = static_cast<const char*>(data->getPtr());
    size_t remaining = 0;
    size_t size = data->size();
    mpack_reader_t reader;

    // return nothing if no data
    if (buffer == nullptr || size == 0)
        return QoreValue();

    // initialize reader; the helper destroys it also when a C++ exception is thrown while unpacking
    mpack_reader_init_data(&reader, buffer, size);
    MpackReaderHelper reader_helper(reader);

    // unpack the data
    do {
        QoreValue node = msgpack_unpack_value(&reader, mode, xsink);
        if (*unpacked) {
            ReferenceHolder<QoreListNode> list(xsink);
            if (unpacked->getType() == NT_LIST)
                list = unpacked.release().get<QoreListNode>();
            else
                list = new QoreListNode(autoTypeInfo);
            list->push(node, xsink);
            unpacked = list.release();
        }
        else {
            unpacked = node;
        }
        remaining = mpack_reader_remaining(&reader, &dataCheck);
    }
    while (remaining && dataCheck);

    // finish reading
    mpack_error_t result = reader_helper.destroy();
    if (*xsink) {
        // a Qore exception, such as an encoding conversion error, explains the failure
        return QoreValue();
    }
    if (result != mpack_ok) {
        throw msgpack::getMsgPackException(result);
    }

    // return unpacked Qore node
    return unpacked.release();
}

} // namespace intern
} // namespace msgpack
