#pragma once


#ifndef __WFS_H__
#define __WFS_H__

#include<stdio.h>
#include<vector>
#include<string>
#include<unordered_map>
#include<algorithm>
#include<mutex>

inline static bool strcompar(const std::string& s1, const std::string& s2)
{
    if (s1.size() != s2.size())
        return false;
    // return s1 == s2;

    const char* a = s1.data();
    const char* b = s2.data();
    const char* ae = a + s1.size();
    while (a < ae)
    {

        if (*a++ != *b++)
            return false;

    }

    return true;
}

#define __NORM(data, size) (((data - 1) / size + 1) * size)

class WFS
{
    struct spinlock
    {
        std::atomic<bool> locked{ false };
        void lock()
        {
            while (true)
            {
                if (!locked.exchange(true)) return;
                while (locked.load());
            }
        }
        void unlock() { locked.store(false); }

        struct spinlock_guard
        {
            spinlock* lock;
            spinlock_guard(spinlock* lock) :
                lock(lock)
            {
                lock->lock();
            }
            ~spinlock_guard()
            {
                lock->unlock();
            }
        };

        spinlock& spinlock::operator=(const spinlock&)
        {
            return spinlock();
        }

        spinlock_guard guard() { return spinlock_guard(this); }
    };
    
    spinlock f_lock;
    spinlock t_lock;

public:

    enum EFileType : unsigned int
    {
        EFileType_Undefined = 0,
        EFileType_File = 1,
        EFileType_Folder = 2
    };

private:
    FILE* fs_file = nullptr;

    struct fs_header
    {
        char magic[4] = "WFS";
        unsigned int version = 1;
        unsigned int other_segmented_file_count;
        unsigned int block_size;
        unsigned int chunk_count;
        unsigned int chunk_block_count;
        unsigned long long chunk_size;
        unsigned long long chunk_offset;
    };

    struct fs_object_header
    {
        char magic[4] = "\nFO";
        long size = 0;
        unsigned int name_size = 0;
        unsigned int type = EFileType_Undefined;
        unsigned int props_count = 0;
        unsigned int child_count = 0;
        unsigned int chunk_count = 0;
    };

    struct fs_prop_header
    {
        char magic[4] = "\nPR";
        unsigned int name_size = 0;
        unsigned int data_size = 0;
    };

    struct chunk_t
    {
        unsigned int id;
        unsigned int offset;
        unsigned int size;
    };

    struct tree_item : fs_object_header
    {
        long seek = -1;
        std::string name;
        std::vector<tree_item> folders;
        std::vector<tree_item> files;
        std::unordered_map<std::string, std::vector<uint8_t>> props;
        std::vector<chunk_t> chunk;
        tree_item* parent = nullptr;
        spinlock lock;

        inline static tree_item* get_folder(tree_item* item, const std::string& name)
        {
            for (int i = 0; i < item->folders.size(); i++)
                if (strcompar(item->folders[i].name, name))
                    return &(item->folders[i]);
            return nullptr;
        }

        inline static tree_item* get_file(tree_item* item, const std::string& name)
        {
            for (int i = 0; i < item->files.size(); i++)
                if (strcompar(item->files[i].name, name))
                    return &(item->files[i]);
            return nullptr;
        }

        void add_file(const tree_item& item)
        {
            files.push_back(item);
            files.back().parent = this;
        }

        void add_folder(const tree_item& item)
        {
            folders.push_back(item);
            folders.back().parent = this;
        }
    };

    fs_header header;

    tree_item root;

    struct free_space_t
    {
        std::vector<chunk_t> free;
        std::vector<unsigned int> reserved;
        size_t total_free = 0;
    };


    struct writer_t
    {
        FILE* fs_file = nullptr;
        size_t offset = sizeof(fs_header);
        size_t chunk_offset;

        bool write(tree_item* item)
        {
            // item->props_count = 0;
            // for (auto& el : item->props)
            //     item->props_count++;

            item->name_size = (unsigned int)item->name.size();
            item->child_count = (unsigned int)(item->folders.size() + item->files.size());
            item->chunk_count = (unsigned int)item->chunk.size();
            item->props_count = (unsigned int)item->props.size();
            offset += sizeof(fs_object_header);
            offset += item->name_size;
            offset += sizeof(chunk_t) * item->chunk_count;

            if (offset > chunk_offset)
                return false;
            memcpy(item->magic, "\nFO", 4);
            fwrite(item, sizeof(fs_object_header), 1, fs_file);

            fwrite(item->name.data(), item->name_size, 1, fs_file);
            fwrite(item->chunk.data(), sizeof(chunk_t), item->chunk_count, fs_file);

            for (auto& el : item->props)
            {
                offset += sizeof(fs_prop_header);
                fs_prop_header prop;
                prop.name_size = el.first.size();
                prop.data_size = el.second.size();
                fwrite(&prop, sizeof(fs_prop_header), 1, fs_file);
                fwrite(el.first.data(),  sizeof(prop.name_size), 1, fs_file);
                fwrite(el.second.data(), sizeof(prop.data_size), 1, fs_file);
            }
            for (unsigned int i = 0; i < item->files.size(); i++)
                if (!write(&item->files[i]))
                    return false;

            for (unsigned int i = 0; i < item->folders.size(); i++)
                if (!write(&item->folders[i]))
                    return false;

            return true;
        }
    };
    
    struct reader_t
    {
        FILE* fs_file = nullptr;

        bool read(tree_item* parent)
        {
            tree_item titem;

            tree_item* item = &titem;
            item->parent = parent;

            fread(item, sizeof(fs_object_header), 1, fs_file);

            if (*(uint32_t*)item->magic != 83466u)
                return false;

            item->name.resize(item->name_size, ' ');
            fread(item->name.data(), item->name_size, 1, fs_file);

            item->chunk.resize(item->chunk_count);
            fread(item->chunk.data(), sizeof(chunk_t), item->chunk_count, fs_file);

            for (unsigned int i = 0; i < item->props_count; i++)
            {
                fs_prop_header prop;
                fread(&prop, sizeof(fs_prop_header), 1, fs_file);
                std::string name(prop.name_size, '\0');
                std::vector<uint8_t> data(prop.data_size);
                fread(name.data(), sizeof(prop.name_size), 1, fs_file);
                fread(data.data(), sizeof(prop.data_size), 1, fs_file);
                item->props.insert({name, data});
            }

            for (unsigned int i = 0; i < item->child_count; i++)
                read(item);

            if (item->type == EFileType_File)
                parent->add_file(titem);
            else if (item->type == EFileType_Folder)
                parent->add_folder(titem);

            return true;
        }
    };


    std::vector<free_space_t> free_space;

    int flush_tree()
    {
        if (!fs_file)
            return 650;

        writer_t writer;
        writer.fs_file = fs_file;

        bool run = true;
        while (run)
        {
            {
                auto l_f = f_lock.guard();
                auto l_t = t_lock.guard();
                run = false;
                writer.offset = sizeof(fs_header) + sizeof(fs_object_header);
                writer.chunk_offset = header.chunk_offset;
                fseek(fs_file, sizeof(fs_header), SEEK_SET);

                for (unsigned int i = 0; !run && i < root.files.size(); i++)
                    if (!writer.write(&root.files[i]))
                    {
                        run = true;
                        break;
                    }

                for (unsigned int i = 0; !run && i < root.folders.size(); i++)
                    if (!writer.write(&root.folders[i]))
                    {
                        run = true;
                        break;
                    }
            }
            if (run)
                move_chunk_to_end(1);
        }
        {
            auto l_f = f_lock.guard();
            auto l_t = t_lock.guard();
            fs_object_header empty;
            empty.magic[1] = 'E';
            empty.magic[2] = 'E';
            fwrite(&empty, sizeof(fs_object_header), 1, fs_file);
            fseek(fs_file, 0, SEEK_SET);
            fwrite(&header, sizeof(fs_header), 1, fs_file);
        }


        return 0;
    }

    int get_parent(std::string path, tree_item*& parent_item, std::string& name)
    {
        name = "";
        parent_item = &root;
        for (int __i = 0; __i < path.size(); __i++)
        {
            char c = path[__i];
            if (c == '/')
            {
                if (name != "")
                {
                    parent_item = tree_item::get_folder(parent_item, name);
                    if (!parent_item)
                        return 1;
                }

                name = "";
            }
            else
                name += c;
        }
        if (name == "")
        {
            return 2;
        }

        return 0;
    }

    void update_free_space(tree_item* item = nullptr)
    {
        bool rt = !item;
        if (rt)
        {
            free_space.clear();
            free_space.resize(header.chunk_count);
            for (auto& el : free_space)
            {
                el.total_free = header.chunk_block_count;
            }
            item = &root;
        }

        for (const auto& ch : item->chunk) if (ch.size > 0)
        {
            auto& sp = free_space[ch.id];
            sp.total_free -= ch.size;
            std::vector<unsigned int> old_reserved = sp.reserved;
            sp.reserved.resize(0);

            unsigned int start = ch.offset;
            unsigned int end = ch.offset + ch.size;

            if (old_reserved.size())
            {
                for (int i = 0; i < old_reserved.size(); i++)
                {
                    if (start < old_reserved[i])
                    {
                        sp.reserved.push_back(start);
                        i--;
                        start = (unsigned int)-1;
                    }
                    else if (start == old_reserved[i])
                    {

                        start = (unsigned int)-1;
                    }
                    else if (end < old_reserved[i])
                    {
                        sp.reserved.push_back(end);
                        i--;
                        end = (unsigned int)-1;
                    }
                    else if (end == old_reserved[i])
                    {
                        end = (unsigned int)-1;
                    }
                    else
                        sp.reserved.push_back(old_reserved[i]);
                }
                if (start != (unsigned int)-1)
                {
                    sp.reserved.push_back(start);
                }
                if (end != (unsigned int)-1)
                {
                    sp.reserved.push_back(end);
                }
            }
            else
            {
                sp.reserved.push_back(start);
                sp.reserved.push_back(end);
            }
        }

        for (auto& ch : item->files)
        {
            update_free_space(&ch);
        }

        if (rt)
        {
            for (unsigned int k = 0; k < free_space.size(); k++)
            {
                auto& sp = free_space[k];
                sp.free.clear();
                sp.free.push_back({ k, 0 , header.chunk_block_count });

                for (int i = 0; i < sp.reserved.size(); i += 2)
                {
                    auto nsize = sp.reserved[i] - sp.free.back().offset;
                    if (nsize)
                        sp.free.back().size = nsize;
                    else
                        sp.free.pop_back();

                    sp.free.push_back({ k, sp.reserved[i + 1] , header.chunk_block_count - sp.reserved[i + 1] });
                }

            }
        }
    }

    void free_chunk(const chunk_t& ch)
    {
        auto& sp = free_space[ch.id];

        unsigned int start = ch.offset;
        unsigned int end = ch.offset + ch.size;

        for (int i = 0; i < sp.free.size(); i++)
        {
            if (sp.free[i].offset == end)
            {
                sp.free[i].offset -= ch.size;
                sp.free[i].size += ch.size;
                return;
            }
            else if (sp.free[i].offset + sp.free[i].size == start)
            {
                sp.free[i].size += ch.size;
                return;
            }
        }

        sp.free.push_back(ch);

        sp.total_free += ch.size;
    }

    bool reserve_chunk(const chunk_t& ch)
    {
        auto& sp = free_space[ch.id];

        unsigned int start = ch.offset;
        unsigned int end = ch.offset + ch.size;

        for (int i = 0; i < sp.free.size(); i++)
        {
            if (sp.free[i].offset == start && sp.free[i].size == ch.size) // eq
            {
                sp.total_free -= ch.size;
                sp.free[i].offset = header.chunk_block_count;
                sp.free[i].size = 0;
                return true;
            }
            else if (sp.free[i].offset == start) // start
            {
                sp.total_free -= ch.size;
                sp.free[i].offset += ch.size;
                sp.free[i].size -= ch.size;
                return true;
            }
            else if (sp.free[i].offset + sp.free[i].size == end) // end
            {
                sp.total_free -= ch.size;
                sp.free[i].size -= ch.size;
                return true;
            }
            else if (sp.free[i].offset < start && sp.free[i].offset + sp.free[i].size > end) // in
            {
                sp.total_free -= ch.size;
                sp.free[i].size = start - sp.free[i].offset;
                sp.free.push_back({ ch.id, end, sp.free[i].offset + sp.free[i].size - end });

                return true;
            }
        }

        return false; // not valid
    }

    std::vector<chunk_t> get_all_free_chunk()
    {
        std::vector<chunk_t> ret;
        for (unsigned int k = 0; k < free_space.size(); k++)
        {
            auto& sp = free_space[k];
            for (int i = 0; i < sp.free.size(); i++)
                ret.push_back({ k, sp.free[i].offset, sp.free[i].size });
        }
        return ret;
    }

    void move_chunk_to_end(unsigned int count)
    {
        auto old = header.chunk_count;
        {
            Expand_FS(header.chunk_size * count, false);
        }
        {
            auto l_f = f_lock.guard();
            std::vector<uint8_t> buff(header.chunk_size * count, 0);
            fseek(fs_file, header.chunk_offset, SEEK_SET);
            fread(buff.data(), header.chunk_size * count, 1, fs_file);

            fseek(fs_file, header.chunk_offset + old * header.chunk_size, SEEK_SET);
            fwrite(buff.data(), header.chunk_size * count, 1, fs_file);

            auto l_t = t_lock.guard();

            header.chunk_offset += header.chunk_size * count;
            header.chunk_count = old;

            struct walker
            {
                unsigned int count;
                unsigned int size;
                void move(tree_item* item)
                {
                    for (int i = 0; i < item->chunk.size(); i++)
                        if (item->chunk[i].id < count)
                        {
                            item->chunk[i].id += size - count;
                        }
                        else
                        {
                            item->chunk[i].id -= count;
                        }
                    for (int i = 0; i < item->files.size(); i++)
                        move(&item->files[i]);
                    for (int i = 0; i < item->folders.size(); i++)
                        move(&item->folders[i]);

                }
            };

            walker w;
            w.size = old;
            w.count = count;
            w.move(&root);

            update_free_space();
        }
    }

public:

    void Create_FS(const char* path, size_t size, unsigned int chunk_block_count = 0x8000, unsigned int block_size = 0x20)
    {
        static const char __e[0x2000];
        Close_FS();
        fs_file = fopen(path, "w");
        fclose(fs_file);
        fs_file = fopen(path, "rb+");

        header.version = 1;
        header.other_segmented_file_count = 0;
        header.block_size = block_size;

        header.chunk_block_count = chunk_block_count;
        header.chunk_size = chunk_block_count * block_size;

        header.chunk_count = 0;
        header.chunk_offset = ((sizeof(fs_header) - 1) / header.chunk_size + 1) * header.chunk_size;

        fseek(fs_file, 0, SEEK_SET);

        size_t fullsize = header.chunk_offset;
        size_t writed = 0;
        for (int i = 0; i < (fullsize / 0x10000); i++, writed += 0x10000)
            fwrite(__e, 0x10000, 1, fs_file);
        fwrite(__e, fullsize - (fullsize / 0x10000) * 0x10000, 1, fs_file);

        Expand_FS(size);
    }

    void Expand_FS(size_t size, bool update_free = true)
    {
        auto l_f = f_lock.guard();
        size_t writed = header.chunk_offset + header.chunk_count * header.chunk_size;
        fseek(fs_file, writed, SEEK_SET);
        static const char __e[0x10000];
        unsigned int new_chunk_count = (unsigned int)((size - 1) / header.chunk_size + 1);

        size_t fullsize = header.chunk_offset + header.chunk_size * (header.chunk_count + new_chunk_count);

        fullsize = fullsize - writed;

        for (int i = 0; i < (fullsize / 0x10000); i++, writed += 0x10000)
            fwrite(__e, 0x10000, 1, fs_file);
        fwrite(__e, fullsize - (fullsize / 0x10000) * 0x10000, 1, fs_file);

        header.chunk_count = header.chunk_count + new_chunk_count;

        fseek(fs_file, 0, SEEK_SET);
        fwrite(&header, sizeof(fs_header), 1, fs_file);

        if (update_free)
            update_free_space();
    }

    void Open_FS(const char* path)
    {
        Close_FS();
        fs_file = fopen(path, "rb+");

        fseek(fs_file, 0, SEEK_SET);
        fread(&header, sizeof(fs_header), 1, fs_file);

        root = tree_item();
        reader_t reader;
        reader.fs_file = fs_file;
        while (reader.read(&root));

        update_free_space();
    }

    void Close_FS()
    {
        if (!fs_file)
            return;
        flush_tree();
        fclose(fs_file);
        fs_file = nullptr;
        root = tree_item();
        free_space.clear();
    }

    int mkdir(std::string path)
    {
        if (!fs_file)
            return 650;
        auto l_t = t_lock.guard();
        tree_item* parent_item;
        std::string name;
        int ret = get_parent(path, parent_item, name);
        if (ret)
            return ret;

        if (tree_item::get_folder(parent_item, name))
            return 3;

        if (tree_item::get_file(parent_item, name))
            return 3;


        tree_item n;
        n.name = name;
        n.parent = parent_item;
        n.type = EFileType_Folder;
        parent_item->add_folder(n);

        return 0;
    }

    int open(std::string path, void** r, bool reopen = false)
    {
        *r = nullptr;
        if (!fs_file)
            return 650;
        tree_item* parent_item;
        std::string name;
        int ret = get_parent(path, parent_item, name);
        if (ret)
            return ret;

        tree_item* exists = tree_item::get_file(parent_item, name);

        if (exists)
        {
            if (exists->type != EFileType_File)
                return 12;
            *r = exists;
            exists->seek = -1;
        }
        else
        {
            tree_item n;
            n.type = EFileType_File;
            n.name = name;
            n.parent = parent_item;
            n.seek = -1;
            parent_item->add_file(n);
            *r = &parent_item->files[parent_item->files.size() - 1];
        }

        return 0;
    }

    int rm(std::string path, bool rec = false)
    {
        if (!fs_file)
            return 650;

        tree_item* parent_item;
        std::string name;
        int ret = get_parent(path, parent_item, name);
        if (ret)
            return ret;

        int exists_folder = -1;
        int exists_file = -1;

        for (int i = 0; i < parent_item->folders.size(); i++)
            if (strcompar(parent_item->folders[i].name, name))
            {
                exists_folder = i;
                break;
            }



        if (exists_folder == -1)
            for (int i = 0; i < parent_item->files.size(); i++)
                if (strcompar(parent_item->files[i].name, name))
                {
                    exists_file = i;
                    break;
                }

        if (exists_folder >= 0 || exists_file >= 0)
        {
            tree_item* el;
            if (exists_folder >= 0)
                el = &(parent_item->folders[exists_folder]);
            else
                el = &(parent_item->files[exists_file]);

            if (el->files.size() + el->folders.size() > 0)
            {
                if (!rec)
                    return 8515;

                rm(path + "/" + el->name, rec);
            }

            for (auto ch : el->chunk)
                free_chunk(ch);
            if (exists_folder >= 0)
                parent_item->folders.erase(parent_item->folders.begin() + exists_folder);
            else
                parent_item->files.erase(parent_item->files.begin() + exists_file);

        }

        return 0;
    }

    int write(void* handler, const void* idata, size_t size, bool auto_expand = true)
    {
        const unsigned char* data = (const unsigned char*)idata;
        if (!fs_file)
            return 650;
        if (!handler)
            return 144;
        tree_item* file = (tree_item*)handler;
        size_t file_max_size = __NORM(file->size, header.block_size);
        if (file->seek == -1)
            file->seek = file->size;
        if (size + file->seek > file_max_size)
        {

            size_t need_to_reserve = ((size - file->size + file->seek - 1) / header.block_size + 1);
            t_lock.lock();
            // auto free = find_free_space();
            auto free = get_all_free_chunk();

            std::sort(free.begin(), free.end(),
                [](chunk_t const& a, chunk_t const& b) { return a.size != b.size ? a.size > b.size : (a.id != b.id ? a.id > b.id : (a.offset < b.offset)); });


            if (free.size() == 0)
                return 5568;
            auto os = file->chunk.size();
            size_t reserved = 0;
            if (free[0].size >= need_to_reserve)
                for (long i = free.size() - 1; i >= 0; i--)
                {
                    auto chunk = free[i];
                    if (need_to_reserve < chunk.size)
                    {
                        chunk.size = (unsigned int)(need_to_reserve);
                        file->chunk.push_back(chunk);
                        reserved = need_to_reserve;
                        break;
                    }
                }
            if (need_to_reserve > reserved)
            {
                for (int i = 0; i < free.size() && need_to_reserve > reserved; i++)
                {
                    auto chunk = free[i];
                    if (need_to_reserve - reserved < chunk.size)
                    {
                        chunk.size = (unsigned int)(need_to_reserve - reserved);
                    }
                    file->chunk.push_back(chunk);
                    reserved += chunk.size;
                }
            }
            if (need_to_reserve > reserved)
            {
                auto ns = file->chunk.size();
                for (int i = 0; i < ns - os; i++)
                    file->chunk.pop_back();
                if (auto_expand)
                {
                    t_lock.unlock();
                    Expand_FS(header.block_size);
                    return write(handler, idata, size);
                }
                else
                    return 555;
            }
            else
            {
                auto ns = file->chunk.size();
                for (int i = os; i < ns; i++)
                    reserve_chunk(file->chunk[i]);
            }
            t_lock.unlock();
        }
        if (size + file->seek > file->size)
        {
            file->size = size + file->seek;
        }
        {
            auto l_f = f_lock.guard();
            size_t offset = 0;
            size_t chunk_offset = file->seek / header.block_size;
            size_t block_offset = file->seek - chunk_offset * header.block_size;
            int i = 0;
            for (; i < file->chunk.size(); i++)
            {
                if (chunk_offset < file->chunk[i].size)
                    break;
                chunk_offset -= file->chunk[i].size;
            }
            for (; i < file->chunk.size(); i++)
            {
                auto c = file->chunk[i];
                c.offset += (unsigned int)chunk_offset;
                c.size -= (unsigned int)chunk_offset;
                size_t write_size =
                    (c.size * header.block_size - block_offset > (size - offset))
                    ? (unsigned int)(size - offset)
                    : (unsigned int)(c.size * header.block_size - block_offset);
                fseek(fs_file, header.chunk_offset + c.id * header.chunk_size + c.offset * header.block_size + block_offset, SEEK_SET);
                fwrite(data + offset, write_size, 1, fs_file);
                offset += write_size;
                file->seek += write_size;
                block_offset = 0;
                chunk_offset = 0;
            }
        }

        return 0;
    }

    int read(void* handler, unsigned char* data, size_t size)
    {
        if (!fs_file)
            return 650;
        if (!handler)
            return 144;
        tree_item* file = (tree_item*)handler;
        auto l_f = f_lock.guard();
        if (file->seek == -1)
            file->seek = 0;
        size_t offset = 0;
        size_t chunk_offset = file->seek / header.block_size;
        size_t block_offset = file->seek - chunk_offset * header.block_size;
        int i = 0;
        for (; i < file->chunk.size(); i++)
        {
            if (chunk_offset < file->chunk[i].size)
                break;
            chunk_offset -= file->chunk[i].size;
        }
        for (; i < file->chunk.size(); i++)
        {
            auto c = file->chunk[i];
            c.offset += (unsigned int)chunk_offset;
            c.size -= (unsigned int)chunk_offset;
            size_t read_size =
                (c.size * header.block_size - block_offset > (size - offset))
                ? (unsigned int)(size - offset)
                : (unsigned int)(c.size * header.block_size - block_offset);
            fseek(fs_file, header.chunk_offset + c.id * header.chunk_size + c.offset * header.block_size + block_offset, SEEK_SET);
            fread(data + offset, read_size, 1, fs_file);
            offset += read_size;
            file->seek += read_size;
            block_offset = 0;
            chunk_offset = 0;
        }

        return 0;
    }

    long seek(void* handler, long _Offset, int _Origin = SEEK_SET)
    {
        if (!fs_file)
            return -1;
        if (!handler)
            return -1;
        tree_item* file = (tree_item*)handler;
        auto l_f = f_lock.guard();
        switch (_Origin)
        {
        case SEEK_SET:
            file->seek = _Offset;
            break;
        case SEEK_CUR:
            file->seek += _Offset;
            break;
        case SEEK_END:
            file->seek = file->size - _Offset;
            break;
        default:
            break;
        }
        if (file->seek > file->size)
            file->seek = file->size;
        else if (file->seek < 0)
            file->seek = 0;

        return file->seek;
    }

    fs_object_header info(std::string path)
    {
        if (!fs_file)
            return fs_object_header();

        tree_item* parent_item;
        std::string name;
        int ret = get_parent(path, parent_item, name);
        if (ret)
            return fs_object_header();


        for (int i = 0; i < parent_item->files.size(); i++)
            if (strcompar(parent_item->files[i].name, name))
            {
                auto& a = parent_item->files[i];
                a.child_count = a.files.size() + a.folders.size();
                a.chunk_count = a.chunk.size();
                return a;
            }
        for (int i = 0; i < parent_item->folders.size(); i++)
            if (strcompar(parent_item->folders[i].name, name))
            {
                auto& a = parent_item->folders[i];
                a.child_count = a.files.size() + a.folders.size();
                a.chunk_count = a.chunk.size();
                return a;
            }
        return fs_object_header();
    }

    std::pair<std::vector<std::string>, std::vector<std::string>> childs(std::string path)
    {
        if (!fs_file)
            return std::pair<std::vector<std::string>, std::vector<std::string>>();

        tree_item* parent_item;
        std::string name;
        auto retcode = get_parent(path, parent_item, name);
        if (retcode == 1)
            return std::pair<std::vector<std::string>, std::vector<std::string>>();
        else if (retcode == 2)
        {
            std::pair<std::vector<std::string>, std::vector<std::string>> ret;
            for (int i = 0; i < root.folders.size(); i++)
                ret.first.push_back(root.folders[i].name);
            for (int i = 0; i < root.files.size(); i++)
                ret.second.push_back(root.files[i].name);
            return ret;
        }

        tree_item* exists = tree_item::get_folder(parent_item, name);

        if (!exists)
            return std::pair<std::vector<std::string>, std::vector<std::string>>();

        std::pair<std::vector<std::string>, std::vector<std::string>> ret;
        exists->child_count = exists->folders.size() + exists->files.size();
        for (int i = 0; i < exists->folders.size(); i++)
            ret.first.push_back(exists->folders[i].name);
        for (int i = 0; i < exists->files.size(); i++)
            ret.second.push_back(exists->files[i].name);
        return ret;
    }

    std::vector<uint8_t> get_prop(std::string path, std::string prop)
    {
        if (!fs_file)
            return std::vector<uint8_t>();

        tree_item* parent_item;
        std::string name;
        auto retcode = get_parent(path, parent_item, name);
        if (retcode == 1)
            return std::vector<uint8_t>();
        else if (retcode == 2)
        {
            auto f = root.props.find(prop);
            if(f != root.props.end())
                return f->second;
            return
                std::vector<uint8_t>();
        }

        tree_item* exists = tree_item::get_folder(parent_item, name);

        if (!exists)
            return std::vector<uint8_t>();
        auto f = exists->props.find(prop);
        if(f != exists->props.end())
            return f->second;
        return
            std::vector<uint8_t>();
    }

    void set_prop(std::string path, std::string prop, std::vector<uint8_t> data)
    {
        if (!fs_file)
            return;

        tree_item* parent_item;
        std::string name;
        auto retcode = get_parent(path, parent_item, name);
        if (retcode == 1)
            return;
        else if (retcode == 2)
        {
            auto f = root.props.find(prop);
            if(f != root.props.end())
                root.props[prop] = data;
            return
                root.props.insert({prop, data});
        }

        tree_item* exists = tree_item::get_folder(parent_item, name);

        if (!exists)
            return;
        auto f = exists->props.find(prop);
        if(f != exists->props.end())
            exists->props[prop] = data;
        return
            exists->props.insert({prop, data});
    }

    ~WFS()
    {
        Close_FS();
    }

};

#endif // __WFS_H__