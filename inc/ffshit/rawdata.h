#ifndef LIBFFSHIT_FULLFLASH_FILESYSTEM_DATA_H
#define LIBFFSHIT_FULLFLASH_FILESYSTEM_DATA_H

#include <cstddef>
#include <memory>
#include <fstream>

namespace FULLFLASH {

class RawData {
    public:
        using Data = std::shared_ptr<char[]>;

        RawData();
        RawData(const RawData &prev);
        RawData(const RawData &prev, size_t offset, size_t size);
        RawData(std::ifstream &file, size_t offset, size_t size);

        RawData(char *data, size_t size);

        void            reverse(size_t align_size);
    
        void            add(char *data, size_t size);
        void            add(const RawData &data);

        void            add_top(char *data, size_t size);

        void            write(size_t offset, char *data, size_t size);
        void            read(size_t offset, char *data, size_t size) const;
        void            read_string(size_t offset, std::string &str, size_t step = 1) const;
        void            read_wstring(size_t offset, std::wstring &str) const;

        template<typename T>
        void read(size_t &offset, char *dst, size_t count) const {
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
