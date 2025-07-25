#include "ffshit/rawdata.h"
#include "ffshit/ex.h"

#include <cstring>

namespace FULLFLASH {

static constexpr size_t CHUNK_SIZE = 0x10000;

RawData::RawData() : size(0) {
    size_real = CHUNK_SIZE;

    data = Data(new char[size_real]);
}

RawData::RawData(char *data, size_t data_size) {
    if (data == nullptr) {
        throw Exception("RawData() from raw ptr. ptr == nullptr");
    }

    if (data_size == 0) {
        throw Exception("RawData() from raw ptr. data_size == 0");
    }

    size_t chunks = (data_size / CHUNK_SIZE) + 1;

    this->size_real = CHUNK_SIZE * chunks;
    this->data      = Data(new char[this->size_real]);

    memcpy(this->data.get(), data, data_size);

    this->size      = data_size;
}

RawData::RawData(const RawData &prev) {
    // Валидно, копирование пустого RawData

    if (prev.size == 0) {
        this->size      = 0;
        this->size_real = CHUNK_SIZE;
        this->data      = Data(new char[this->size_real]);

        return;
    }

    if (!prev.data) {
        this->size      = 0;
        this->size_real = CHUNK_SIZE;
        this->data      = Data(new char[this->size_real]);

        return;
    }

    this->size      = prev.size;
    this->size_real = prev.size_real;
    this->data      = Data(new char[prev.size_real]);

    memcpy(this->data.get(), prev.data.get(), prev.size);
}

RawData::RawData(RawData &&prev) {
    // Валидно, перемещение пустого RawData

    if (prev.size == 0) {
        this->size      = 0;
        this->size_real = CHUNK_SIZE;
        this->data      = Data(new char[this->size_real]);

        return;
    }

    if (!prev.data) {
        this->size      = 0;
        this->size_real = CHUNK_SIZE;
        this->data      = Data(new char[this->size_real]);

        return;
    }

    this->size      = prev.size;
    this->size_real = prev.size_real;
    this->data      = std::move(prev.data);
}

RawData::RawData(const RawData &prev, size_t offset, size_t data_size) {
    if (prev.size == 0) {
        throw Exception("RawData() form prev. with offset, size. prev.size == 0");
    }

    if (!prev.data) {
        throw Exception("RawData() form prev. with offset, size. prev.data invalid ptr");
    }

    if (data_size == 0) {
        throw Exception("RawData() form prev. with offset, size. data_size == 0");
    }

    if (offset + data_size > prev.get_size()) {
        throw Exception("RawData() offset + size > prev. size");
    }

    size_t chunks = (data_size / CHUNK_SIZE) + 1;

    this->size_real = CHUNK_SIZE * chunks;
    this->data      = Data(new char[this->size_real]);
    this->size      = data_size;

    memcpy(this->data.get(), prev.data.get() + offset, data_size);
}

RawData::RawData(std::ifstream &file, size_t offset, size_t data_size) {
    if (data_size == 0) {
        throw Exception("RawData() from file data_size == 0");
    }

    size_t chunks = (data_size / CHUNK_SIZE) + 1;

    this->size_real = CHUNK_SIZE * chunks;
    this->data      = Data(new char[this->size_real]);
    this->size      = data_size;

    file.seekg(offset, std::ios_base::beg);
    file.read(data.get(), data_size);
}

RawData &RawData::operator =(const RawData &prev) {
    if (this != &prev) {
        if (prev.size == 0) {
            return *this;
        }

        if (!prev.data) {
            return *this;
        }

        if (this->size >= prev.size) {
            if (this->size_real < prev.size_real) {
                size_t chunks = (prev.size / CHUNK_SIZE) + 1;

                this->size_real = CHUNK_SIZE * chunks;
                this->data      = Data(new char[this->size_real]);
            }

            this->size = prev.size;

            memcpy(this->data.get(), prev.data.get(), prev.size);
        } else {
            size_t chunks = (prev.size / CHUNK_SIZE) + 1;

            this->size      = prev.size;
            this->size_real = CHUNK_SIZE * chunks;
            this->data      = Data(new char[this->size_real]);

            memcpy(this->data.get(), prev.data.get(), prev.size);
        }
    }

    return *this;
}

RawData &RawData::operator =(RawData &&prev) {
    if (this != &prev) {
        if (prev.size == 0) {
            return *this;
        }

        if (!prev.data) {
            return *this;
        }

        this->size      = prev.size;
        this->size_real = prev.size_real;
        this->data      = std::move(prev.data);
    }

    return *this;
}

void RawData::reserve(size_t data_size) {
    size_t chunks = (data_size / CHUNK_SIZE) + 1;

    if (this->size != 0) {
        auto tmp_data = Data(new char[this->size]);

        memcpy(tmp_data.get(), this->data.get(), this->size);

        this->size_real += CHUNK_SIZE * chunks;
        this->data      = Data(new char[this->size_real]);

        memcpy(data.get(), tmp_data.get(), this->size);
    } else {
        this->size_real = CHUNK_SIZE * chunks;
        this->data      = Data(new char[this->size_real]);
    }
}

void RawData::add(char *data, size_t add_size) {
    if (add_size == 0) {
        throw Exception("RawData::add() add_size == 0");
    }

    if (data == nullptr) {
        throw Exception("RawData:add() ptr == nullptr");
    }

    size_t  size_new = this->size + add_size;

    if (size + add_size < size_real) {
        memcpy(this->data.get() + this->size, data, add_size);
        this->size = size_new;

        return;
    }

    if (size + add_size >= size_real) {
        auto tmp_data = Data(new char[this->size]);

        memcpy(tmp_data.get(), this->data.get(), this->size);

        size_t chunks = ((size + add_size) / CHUNK_SIZE) + 1;

        this->size_real = CHUNK_SIZE * chunks;
        this->data      = Data(new char[this->size_real]);

        memcpy(this->data.get(), tmp_data.get(), this->size);
        memcpy(this->data.get() + this->size, data, add_size);

        this->size = size_new;

        return;
    }

    throw Exception("Data corrupted add");
}

void RawData::add_top(char *data, size_t add_size) {
    if (add_size == 0) {
        throw Exception("RawData::add_top() add_size == 0");
    }

    if (data == nullptr) {
        throw Exception("RawData:add_top() ptr == nullptr");
    }

    size_t  size_new = this->size + add_size;

    if (size + add_size < size_real) {
        auto tmp_data = Data(new char[this->size]);
        memcpy(tmp_data.get(), this->data.get(), this->size);

        memcpy(this->data.get(), data, add_size);
        memcpy(this->data.get() + add_size, tmp_data.get(), this->size);

        this->size = size_new;

        return;
    }

    if (size + add_size >= size_real) {
        auto tmp_data = Data(new char[this->size]);
        memcpy(tmp_data.get(), this->data.get(), this->size);

        size_t chunks = ((size + add_size) / CHUNK_SIZE) + 1;

        this->size_real = CHUNK_SIZE * chunks;
        this->data      = Data(new char[this->size_real]);

        memcpy(this->data.get(), data, add_size);
        memcpy(this->data.get() + add_size, tmp_data.get(), this->size);

        this->size = size_new;
    }

    throw Exception("Data corrupted add_top");
}

void RawData::add(const RawData &new_data) {
    if (!new_data.data) {
        return;
    }

    if (new_data.get_size() == 0) {
        return;
    }

    add(new_data.data.get(), new_data.size);
}

void RawData::read(size_t offset, char *data, size_t read_size) const {
    if (read_size == 0) {
        throw Exception("RawData::read() read_size == 0");
    }

    if (data == nullptr) {
        throw Exception("RawData:read() ptr == nullptr");
    }

    if (offset >= this->size) {
        throw Exception("RawData::read() Offset >= data size. Offset: {}, Data size: {}", offset, this->size);
    }

    if (offset + read_size > this->size) {
        throw Exception("RawData::read() Read size + offset > data size; Offset: {}, Read size: {}, Data size: {}", offset, read_size, this->size);
    }

    memcpy(data, this->data.get() + offset, read_size);
}

void RawData::read_string(size_t offset, std::string &str, size_t step) const {
    size_t offset_start = offset;

    while(1) {
        if (offset >= size) {
            break;
//            throw Exception("Couldn't find end of string. Offset: {}, Size: {}", offset_start, size);
        }

        char *c = data.get() + offset;

        if (*c == 0x0) {
            break;
        }

        str.push_back(*c);

        offset += step;
    }
}

void RawData::read_wstring(size_t offset, std::wstring &str) const {
    while(1) {
        if (offset >= size) {
            break;
        }

        wchar_t *c = (wchar_t *) (data.get() + offset);

        if (*c == 0x0) {
            break;
        }

        str.push_back(*c);

        offset += 2;
    }
}

// ELKA Align
RawData RawData::read_aligned(size_t read_offset, size_t read_size) const {
    RawData         data_ret;

    if (read_size == 0) {
        throw Exception("RawData::read_aligned() read_size == 0");
    }

    if (read_offset >= this->size) {
        throw Exception("RawData::read_aligned(). offset '{}' >= data size '{}'", read_offset, size);
    }

    if (static_cast<int64_t>(size) - read_size < 0) {
        throw Exception("RawData::read_aligned(). offset '{}' - read_size '{}' < 0", read_offset, read_size);
    }

    char *          data_ptr    = data.get() + read_offset;
    ssize_t         to_read     = read_size;

    while (to_read > 0) {
        size_t read_size    = 16;
        size_t skip         = 0;

        if (to_read <= 16) {
            skip        = 16 - to_read;
        }

        // if (to_read > 16) {
        //     // read_size = 16;
        // } else {
        //     // read_size   = to_read;
        //     skip        = 16 - to_read;
        // }

        data_ptr    -= 32;
        data_ret.add_top(data_ptr + skip, read_size - skip);
        to_read     -= read_size - skip;
    }

    return data_ret;
}

const RawData::Data &RawData::get_data() const {
    return data;
}

const size_t RawData::get_size() const {
    return size;
}

};
