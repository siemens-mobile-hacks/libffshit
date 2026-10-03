#ifndef LIBFFSHIT_FULLFLASH_FILESYSTEM_WRITE_RECORDS_H
#define LIBFFSHIT_FULLFLASH_FILESYSTEM_WRITE_RECORDS_H

#include "ffshit/partition/partitions.h"
#include "ffshit/platform/types.h"
#include "ffshit/rawdata.h"

#include <map>
#include <memory>
#include <unordered_map>
#include <vector>

namespace FULLFLASH {
namespace Filesystem {
namespace Write {

// The records of one partition: data kept under an id. Every flash block of the partition has a
// file index table (FIT) growing from its end, with an entry per record: its state, id, size and
// where its data is.
//
// Records are only ever appended, deleted by clearing bits of their FIT entry's flags, or changed
// in place where that only clears bits, which is what the flash allows. When no block has room
// left, a block is compacted: rewritten with its valid records only, as the firmware's reclaim
// would, with its erase counter incremented. One block without valid records is left alone, for
// the firmware to reclaim into.
class Records {
    public:
        using Ptr = std::unique_ptr<Records>;

        static Ptr              build(Platform::Type platform, Partitions::Partitions::Ptr partitions, const std::string &partition_name);

        virtual                 ~Records() = default;

        bool                    contains(uint32_t id) const;
        RawData                 read(uint32_t id) const;
        // A new record, of an id that has none
        void                    add(uint32_t id, const RawData &data);
        // Overwrites bytes of a record in place, which may only clear bits
        void                    patch(uint32_t id, size_t offset, const std::vector<uint8_t> &bytes);
        void                    remove(uint32_t id);

        // The lowest free even id from `min_id` on whose next id is free too: the firmware keeps a
        // file's data under the id after its header's
        uint32_t                allocate_pair(uint32_t min_id);

        // Undoes everything since begin() when an operation fails halfway
        void                    begin();
        void                    commit();
        void                    rollback();

    protected:
        static constexpr uint32_t   FLAGS_FREE      = 0xFFFFFFFF;
        static constexpr uint32_t   FLAGS_VALID     = 0xFFFFFFC0;
        static constexpr uint32_t   FLAGS_DELETED   = 0xFFFFFF00;

        struct Entry {
            uint32_t    fit_offset;
            uint32_t    flags;
            uint32_t    id;
            uint32_t    size;
            uint32_t    offset;
        };

        struct Block {
            uint32_t            addr;
            uint32_t            size;
            std::vector<Entry>  entries;
            // where the FIT entry of the next record goes; the FIT ends with a free entry here
            uint32_t            fit_next;
            // where the data of the next record goes
            uint32_t            data_end;
        };

        RawData &               image;
        std::vector<Block>      blocks;

        Records(Partitions::Partitions::Ptr partitions, const std::string &partition_name);

        // Reads the FITs and indexes the valid records. Called by the layouts once they are built.
        void                    scan();

        // What differs between the layouts of the blocks
        virtual void            scan_block(Block &block) const = 0;
        virtual RawData         read_entry(const Block &block, const Entry &entry) const = 0;
        // Where byte `offset` of a record is in the fullflash
        virtual size_t          entry_address(const Block &block, const Entry &entry, size_t offset) const = 0;
        virtual bool            fits(const Block &block, uint32_t size) const = 0;
        // Writes the record and its FIT entry, and moves fit_next and data_end on
        virtual void            append(Block &block, uint32_t id, const RawData &data) = 0;
        // Everything but the block's header back to 0xFF, as an erase leaves it
        virtual void            erase(Block &block) = 0;

        void                    write(size_t address, const char *data, size_t size);
        void                    write_u32(size_t address, uint32_t value);
        void                    fill(size_t address, size_t size);
        void                    increment_erase_counter(size_t address);

    private:
        static constexpr uint32_t   MAX_ID = 0xFFFE;

        std::string             partition_name;

        // id -> block and entry of its valid record
        std::unordered_map<uint32_t, std::pair<size_t, size_t>> index;

        // ids that are taken: by a valid record, by an operation that has not written them yet,
        // or because the firmware's own records mention them
        std::vector<bool>       used;
        std::vector<bool>       pending;
        std::vector<bool>       excluded;
        // every pair below it is taken
        uint32_t                free_hint;

        // a block without valid records that is never written to
        size_t                  spare;

        bool                    in_transaction;
        // the original content of the blocks the operation wrote to, by block
        std::map<size_t, RawData> saved_blocks;

        bool                    taken(uint32_t id) const;
        void                    mark_used(uint32_t id, bool is_used);

        void                    touch(size_t block_index);
        size_t                  find_block(uint32_t size);
        void                    compact(size_t block_index);
        void                    index_block(size_t block_index);
        size_t                  dead_space(const Block &block) const;
};

// SGOLD and SGOLD2: a 16 byte header at the start of the block, the data packed after it and the
// FIT's 16 byte entries growing down from the block's end
class LinearRecords : public Records {
    public:
        LinearRecords(Partitions::Partitions::Ptr partitions, const std::string &partition_name);

    protected:
        void                    scan_block(Block &block) const override final;
        RawData                 read_entry(const Block &block, const Entry &entry) const override final;
        size_t                  entry_address(const Block &block, const Entry &entry, size_t offset) const override final;
        bool                    fits(const Block &block, uint32_t size) const override final;
        void                    append(Block &block, uint32_t id, const RawData &data) override final;
        void                    erase(Block &block) override final;
};

// SGOLD2_ELKA, whose flash programs 1 KiB regions either in control mode, where only the first
// 16 bytes of every 32 hold data but can be programmed again, or in object mode, all of it, once.
// The header is at the block's end, the FIT grows down from below it in 32 byte slots with records
// up to 512 bytes inline under their entry, and larger records go to the data area growing up from
// the block's start in 1 KiB units. A record ending in up to 512 bytes past a 1 KiB multiple keeps
// that tail inline.
class ElkaRecords : public Records {
    public:
        ElkaRecords(Partitions::Partitions::Ptr partitions, const std::string &partition_name);

    protected:
        void                    scan_block(Block &block) const override final;
        RawData                 read_entry(const Block &block, const Entry &entry) const override final;
        size_t                  entry_address(const Block &block, const Entry &entry, size_t offset) const override final;
        bool                    fits(const Block &block, uint32_t size) const override final;
        void                    append(Block &block, uint32_t id, const RawData &data) override final;
        void                    erase(Block &block) override final;

    private:
        enum class Kind {
            INLINE,
            SPLIT,
            DATA_AREA,
        };

        static Kind             kind(uint32_t size);
        // how much of the FIT and of the data area a record takes
        static uint32_t         fit_size(uint32_t size);
        static uint32_t         data_area_size(uint32_t size);

        size_t                  inline_address(const Block &block, uint32_t fit_offset, uint32_t size, size_t offset) const;
        RawData                 read_inline(const Block &block, uint32_t fit_offset, uint32_t size) const;
        void                    write_inline(const Block &block, uint32_t fit_offset, const char *data, uint32_t size);
};

};
};
};

#endif
