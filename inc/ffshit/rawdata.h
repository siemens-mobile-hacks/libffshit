#ifndef LIBFFSHIT_FULLFLASH_FILESYSTEM_DATA_H
#define LIBFFSHIT_FULLFLASH_FILESYSTEM_DATA_H

#include <cstddef>
#include <memory>
#include <fstream>

#if defined(_MSC_VER)
#include <BaseTsd.h>
typedef SSIZE_T ssize_t;
#endif

namespace FULLFLASH {

class RawData {
    public:
        using Data = std::shared_ptr<char[]>;

        RawData();
        RawData(const RawData &prev);
        RawData(const RawData &prev, size_t offset, size_t data_size);
        RawData(std::ifstream &file, size_t offset, size_t data_size);

        RawData(char *data, size_t data_size);
    
        void            add(char *data, size_t add_size);
        void            add(const RawData &data);

        void            add_top(char *data, size_t add_size);

        void            write(size_t offset, char *data, size_t write_size);
        void            read(size_t offset, char *data, size_t read_size) const;
        void            read_string(size_t offset, std::string &str, size_t step = 1) const;
        void            read_wstring(size_t offset, std::wstring &str) const;

        // ELKA Align
        RawData         read_aligned(size_t offset, size_t read_size) const;

        template<typename T>
        void read_type(const size_t offset, T *dst, size_t count = 1) const {
            read(offset, reinterpret_cast<char *>(dst), count * sizeof(T));
        }

        template<typename T>
        void read(size_t &offset, char *dst, size_t count = 1) const {
            read(offset, dst, count * sizeof(T));

            offset += count * sizeof(T);
        }

        Data            get_data() const;
        const size_t    get_size() const;

    private:

        size_t  size;
        Data    data;
};

};

#endif
