#include "ffshit/rawdata.h"
#include "ffshit/ex.h"

#include <cstring>

namespace FULLFLASH {

RawData::RawData() : size(0) { 
}

RawData::RawData(char *data, size_t size) {
    if (size == 0) {
        this->size = 0;

        throw Exception("RawData() size == 0");
    }

    this->data = Data(new char[size]);

    memcpy(this->data.get(), data, size);

    this->size = size;
}

RawData::RawData(const RawData &prev) {
    if (!prev.data) {
        this->size = 0;

        return;
    }

    this->size = prev.size;
    this->data = Data(new char[prev.size]);

    memcpy(this->data.get(), prev.data.get(), prev.size);
}

RawData::RawData(const RawData &prev, size_t offset, size_t size) {
    if (!prev.data) {
        this->size = 0;

        return;
    }

    if (offset + size > prev.get_size()) {
        throw Exception("RawData() offset + size > prev. size");
    }

    this->size = size;
    this->data = Data(new char[size]);

    memcpy(this->data.get(), prev.data.get() + offset, size);
}

RawData::RawData(std::ifstream &file, size_t offset, size_t size) {
    this->size = size;
    this->data = Data(new char[size]);

    file.seekg(offset, std::ios_base::beg);
    file.read(data.get(), size);
}

void RawData::add(char *data, size_t size) {
    size_t  size_new = this->size + size;
    Data    tmp_data;

    if (this->size > 0) {
        tmp_data = Data(new char[this->size]);
        
        memcpy(tmp_data.get(), this->data.get(), this->size);
    }

    this->data = Data(new char[size_new]);

    if (this->size > 0) {
        memcpy(this->data.get(), tmp_data.get(), this->size);
    }

    memcpy(this->data.get() + this->size, data, size);

    this->size = size_new;
}

void RawData::add_top(char *data, size_t size) {
    size_t  size_new = this->size + size;
    Data    tmp_data;

    if (this->data) {
        tmp_data = Data(new char[this->size]);
        
        memcpy(tmp_data.get(), this->data.get(), this->size);
    }

    this->data = Data(new char[size_new]);

    memcpy(this->data.get(), data, size);

    if (tmp_data) {
        memcpy(this->data.get() + size, tmp_data.get(), this->size);
    }

    this->size = size_new;
}


void RawData::add(const RawData &data) {
    add(data.data.get(), data.size);
}

void RawData::write(size_t offset, char *data, size_t size) {
    if (offset >= this->size) {
        throw Exception("RawData::write() Offset >= data size. Offset: {}, Data size: {}", offset, this->size);
    }

    if (offset + size > this->size) {
        throw Exception("RawData::write() Write size + offset > data size; Offset: {}, Write size: {}, Data size: {}", offset, size, this->size);
    }

    memcpy(this->data.get() + offset, data, size);
}

void RawData::read(size_t offset, char *data, size_t size) const {
    if (offset >= this->size) {
        throw Exception("RawData::read() Offset >= data size. Offset: {}, Data size: {}", offset, this->size);
    }

    if (offset + size > this->size) {
        throw Exception("RawData::read() Read size + offset > data size; Offset: {}, Read size: {}, Data size: {}", offset, size, this->size);
    }

    memcpy(data, this->data.get() + offset, size);
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

RawData::Data RawData::get_data() const {
    return data;
}

const size_t RawData::get_size() const {
    return size;
}

};
