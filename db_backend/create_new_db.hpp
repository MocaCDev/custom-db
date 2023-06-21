#ifndef create_new_db
#define create_new_db

/* These values indicate what is being described at a specific point in the modb binary file. */
enum class modb_sections: unsigned char
{
    MODB_PORT_AND_HOST      = 0xD2,
    MODB_DB_NAME            = 0xD3,
    MODB_IP_ADDRESS         = 0xD1,
    MODB_PATH               = 0xD4,
    MODB_END                = 0xD5
};

/* Values that are not wanted when writing to the modb binary file. */
unsigned char unwanted_values[1] = {0x2E};

/* TODO: Make a struct that outlines the MODB binary data structure.
 *  This will be used to check if the values already existing in the MODB binary file
 *  match the ones the user is trying to create. If they do, the program will error.
 * */

class CreateDB
{
private:
    /* Database binary information, including where the "binary" is located,
     * the file that will be used to write the binary etc.
     * */
    unsigned char *path = nullptr;
    unsigned char *folder = nullptr;
    FILE *db_bin_file = NULL;
    size_t db_bin_file_size = 0;
    unsigned char *existing_db_info = nullptr;
    size_t existing_db_size = 0;

    /* Database information. */
    unsigned char *db_name          = nullptr;
    unsigned char db_port[5]        = {0, 0, 0, 0, '\0'};
    unsigned char *db_ip_address    = nullptr;
    unsigned char *db_host          = nullptr;

    /* If this is true then the database already exists. */
    bool db_exists = false;

    /* We want to know if the database got committed throughout the lifetime of the program.
     * This is used in `check_if_not_committed`.
     * */
    bool db_committed = false;

    /* The `modb` binary file header is OS-specific.
     * TODO: Make it to where we can represent this with a enum value instead of having
     *       the entire OS-specific name.
     *       Too lazy to do that right now lol.
     * 
     *  Version 0.0.1: `modb` binary file OS-specific header describes the type of OS.
     * */
#ifdef _WIN32
    unsigned char modb_header[6] = {'W', 'I', 'N', '3', '2', 0};
#endif

#ifdef _WIN64
    unsigned char modb_header[6] = {'W', 'I', 'N', '6', '4', 0};
#endif

#if defined(__unix) || defined(__unix__) || defined(__linux__)
    unsigned char modb_header[5] = {'U', 'N', 'I', 'X', 0};
#endif

#if defined(__APPLE__) || defined(__MACH__)
    unsigned char modb_header[4] = {'M', 'A', 'C', 0};
#endif

#if (!defined(_WIN32) || !defined(_WIN64)) && (!defined(__unix) || !defined(__unix__) || !defined(__linux__)) && (!defined(__APPLE__) || !defined(__MACH__))
    unsigned char modb_header[12] = {'U', 'N', 'S', 'U', 'P', 'P', 'O', 'R', 'T', 'E', 'D', 0};
#endif

    /*
     * RUCPTR_assign_do_again - reallocate memory for a Unsigned Char pointer, assign the new index a value and reallocate again.
     *  returns: unsigned char pointer with newly assigned data and new memory
     *  on error: this function will error if there was a memory allocation error
     *
     * Note: NI in `NI_value` stand for New Index; `NewIndex_value` so to say
     * */
    unsigned char *RUCPTR_assign_do_again(unsigned char *src, size_t new_size, unsigned char NI_value)
    {
        /* Allocate. */
        src = reallocate_UC_ptr(src, new_size);
        
        /* Assign. */
        src[new_size] = NI_value;
        
        /* Allocate Again. */
        src = reallocate_UC_ptr(src, new_size + 1);

        return src;
    }

    /*
     * UC_ptr_check - check all elements in the pointer and make sure they are valid.
     *  returns: unsigned char pointer passed to the function; the pointer may be modified after
     *  on error: this function does not error
     * */
    unsigned char *UC_ptr_check(unsigned char *src, size_t ptr_size)
    {
        /* Make sure `src` has valid memory. */
        database_assert(src, "\nMemory allocated for UC pointer is invalid.\n")

        /* Clear out any unwanted values. */
        while(ptr_size != 0)
        {
            for(int i = 0; i <= sizeof(unwanted_values)/sizeof(unwanted_values[0]); i++)
                if(src[ptr_size] == unwanted_values[i]) { src[ptr_size] = 0x0; break; }
            
            ptr_size--;
        }

        return src;
    }

    /*
     * check_modb_bin_file - check if the file is not `NULL`. If it is, open in `wb` mode.
     *  returns: nothing
     *  on error: this function does not error
     * */
    void check_modb_bin_file()
    {
        if(db_bin_file == NULL)
            db_bin_file = fopen(NCC_PTR path, "wb");
    }

public:
    CreateDB(unsigned char *path_for_modb_binary)
    {
        path = UC_PTR path_for_modb_binary;

        /* Make sure it's inside a folder. */
        size_t i = 0;
        while(path[i] != '\0')
        {
            database_assert(!(path[i + 1] == '\0'),
                "\nThe MODB binary file needs to be located inside a folder.\n")
            
            if(path[i] == '/' && !(path[i - 1] == '.')) break;
            i++;
        }

        /* Save the folder. */
        folder = UC_PTR calloc(i, sizeof(*folder));
        for(int x = 0; x < i; x++)
            memset(&folder[x], path[x], 1);

        /* Check if a database exists. */
        FILE *db_file = fopen(NCC_PTR path, "rb");

        if(db_file) { db_exists = true; fclose(db_file); }
    }

    /*
     * modb_bin_file_seek_end_for_new - seek to the end of the modb binary file just in case the user is adding
     *                                  a new database.
     *  returns: nothing
     *  on error: this function does not error.
     * */
    void modb_init_new_db_entry() { 
        db_exists = false;
        db_bin_file = fopen(NCC_PTR path, "rb");

        /* Make sure the modb binary file exists. */
        database_assert(db_bin_file, 
            "\nThe MODB database binary file %s does not exist.\nTry using `database_method::DB_CREATE`.\n\tIf you want the program to auto comit the database, use `database_method::DB_CREATE_AND_AUTO_COMMIT`.\n",
            path)


        /* Get size. */
        fseek(db_bin_file, 0, SEEK_END);
        existing_db_size = ftell(db_bin_file);
        fseek(db_bin_file, 0, SEEK_SET);

        existing_db_info = UC_PTR calloc(existing_db_size, sizeof(*existing_db_info));
        fread(existing_db_info, sizeof(unsigned char), existing_db_size, db_bin_file);
        fclose(db_bin_file);

        /* Open `path` to write bytes. */
        db_bin_file = fopen(NCC_PTR path, "wb");
    }

    /*
     * database_exists - just returns `db_exists`.
     *  returns: true if the database exists else false
     *  on error: this function does not error
     * */
    bool database_exists() { return db_exists; }

    /*
     * database_bin_file_exists - check if `db_bin_file` is not `NULL`.
     *  return: true if it exists else false.
     *  on error: this function does not error
     * */
    bool database_bin_file_exists() { return db_bin_file ? true : false; }

    /*
     * new_name - assign `db_name`.
     *  returns: nothing
     *  on error: this function will error if a memory allocation fails
     * */
    void new_name(unsigned char *name)
    {
        db_name = UC_PTR calloc(1, sizeof(*db_name));
        
        /* Make sure `db_name` got allocated and did not come back `NULL`. */
        database_assert(db_name, "\nError allocating initial memory for `db_name`.\n")

        size_t index = 0;
        while(name[index] != '\0')
        {
            db_name[index] = name[index];
            index++;

            db_name = reallocate_UC_ptr(db_name, index + 1);
        }
    }

    /*
     * new_ip_address - assign `db_ip_address`.
     *  returns: nothing
     *  on error: this function will error if a memory allocation fails
     * */
    void new_ip_address(unsigned char *ip_address)
    {
        db_ip_address = UC_PTR calloc(1, sizeof(*db_ip_address));

        /* Make sure `db_ip_address` got allocated and did not come back `NULL`. */
        database_assert(db_ip_address, "\nError allocating initial memory for `db_ip_address`.\n")

        size_t index = 0;
        while(ip_address[index] != '\0')
        {
            db_ip_address[index] = ip_address[index];
            index++;

            db_ip_address = reallocate_UC_ptr(db_ip_address, index + 1);
        }

        memset(&db_ip_address[index], 0, 1);
    }

    /*
     * new_host - assign `db_host`.
     *  returns: nothing
     *  on error: this function will error if the length of `*host` is > 4 or if there is a value that is not a number
     * */
    void new_host(unsigned char *host)
    {
        db_host = UC_PTR calloc(1, sizeof(*db_host));

        /* Make sure `db_host` got allocated and did not come back `NULL`. */
        database_assert(db_host, "\nError allocating initial memory for `db_host`.\n")

        size_t index = 0;
        while(host[index] != '\0')
        {
            db_host[index] = host[index];
            index++;

            db_host = reallocate_UC_ptr(db_host, index + 1);
        }

        memset(&db_host[index], 0, 1);
    }

    /*
     * new_port - assign `db_port`.
     *  returns: nothing
     *  on error: this functions will error if memory allocation fails
     * */
    void new_port(unsigned char *port)
    {
        unsigned char i = 0;
        while(port[i] != '\0')
        {
            /* Make sure the value is not non-ascii related. */
            database_assert(!((port[i] >= 'a' && port[i] <= 'z') || (port[i] >= 'A' && port[i] <= 'Z')),
                "\n`port` must be represented by just numbers, no letters.\n")
            
            i++;
        }

        /* If the length of `port` is > 4, error. */
        database_assert(i == 4, "\n`port` must be represented by 4 digits, not %d.\n", i)

        for(i = 0; i < 6; i++)
            memset(&db_port[i], port[i], 1);
    }

    /*
     * user_has_assigned_host - check if the user has assigned a host (this function is called in `new_db_commit` located in `Database`).
     *  returns: true if index 0 of `db_host` is not zero else false
     *  on error: this function does not error
     * */
    bool user_has_assigned_host() { return db_port[0] != 0 ? true : false; }

    /*
     * give_default_host_value - assign `db_host` to the default value `8080`.
     *  returns: nothing
     *  on error: this function does not error
     * */
    void give_default_host_value()
    {
        memset(&db_port[0], '8', 1);
        memset(&db_port[1], '0', 1);
        memset(&db_port[2], '8', 1);
        memset(&db_port[3], '0', 1);
        memset(&db_port[4], '\0', 1);
    }

    /*
     * committ_new_database - committ the database and write to `db_bin_file`.
     *  returns: nothing
     *  on error: this function will error if there was a problem writing to `db_bin_file`, or if there was
     *            a memory allocation error
     * */
    void commit_new_database()
    {
        /* Make sure `db_bin_file` is valid. */
        check_modb_bin_file();

        std::cout << "Committing database to: " << path << "\n\tOS: " << modb_header << std::endl;

        /* If the user did not assign a host, assign `db_host` to the default host. */
        if(!(user_has_assigned_host()))
            give_default_host_value();
        
        /* Make sure everything else has been initialized. */
        database_assert(db_port, "\nMissing database port.\n\tRun `set_new_db_port` to setup the database port.\n")
        database_assert(db_ip_address, "\nMissing database IP address.\n\tRun `set_new_db_ip_addr` to setup the database IP address.\n")
        database_assert(db_name, "\nMissing database name.\n\tRun `set_new_db_name` to setup the database name.\n")

        size_t bin_data_size = sizeof(modb_header)/sizeof(modb_header[0]);
        unsigned char *modb_db_binary = UC_PTR calloc(bin_data_size, sizeof(*modb_db_binary));

        /* Make sure `modb_db_binary` got allocated and did not come back `NULL`. */
        database_assert(modb_db_binary, "\nError allocating initial memory for `modb_db_binary`.\n")

        /* Assign the `modb_header` first. */
        memcpy(modb_db_binary, modb_header, bin_data_size);

        /* "Clear" out all memory in `modb_header` (set all elements to zero). */
        memset(modb_header, 0, bin_data_size);

        /* Database IP Address sections. */
        modb_db_binary = reallocate_UC_ptr(modb_db_binary, bin_data_size + 1);
        memset(&modb_db_binary[bin_data_size], static_cast<unsigned char> (modb_sections::MODB_IP_ADDRESS), 1);
        bin_data_size++;

        /* Get the length of the IP address. */
        size_t val_size = 0;
        while(db_ip_address[val_size] != '\0') val_size++;

        /* Assign the IP address followed by a byte of padding. */
        modb_db_binary = reallocate_UC_ptr(modb_db_binary, (bin_data_size + val_size) + 1);
        memcpy(&modb_db_binary[bin_data_size], db_ip_address, val_size);
        memset(&modb_db_binary[bin_data_size + val_size + 1], 0, 1);
        bin_data_size += val_size + 1;

        /* Database port and host section. */
        modb_db_binary = reallocate_UC_ptr(modb_db_binary, bin_data_size + 1);
        memset(&modb_db_binary[bin_data_size], static_cast<unsigned char> (modb_sections::MODB_PORT_AND_HOST), 1);
        bin_data_size++;

        /* Get the length of the port. */
        val_size = 0;
        while(db_host[val_size] != '\0') val_size++;

        /* Assign the port followed by a byte of padding. */
        modb_db_binary = reallocate_UC_ptr(modb_db_binary, (bin_data_size + val_size) + 1);
        memcpy(&modb_db_binary[bin_data_size], db_host, val_size);
        memset(&modb_db_binary[bin_data_size + val_size + 1], 0, 1);
        bin_data_size += val_size + 1;
        
        /* Assign the host followed by a byte of padding. */
        modb_db_binary = reallocate_UC_ptr(modb_db_binary, bin_data_size + 5);
        memcpy(&modb_db_binary[bin_data_size], db_port, 5);
        bin_data_size += 5;

        /* Database name section. */
        modb_db_binary = reallocate_UC_ptr(modb_db_binary, bin_data_size + 1);
        memset(&modb_db_binary[bin_data_size], static_cast<unsigned char> (modb_sections::MODB_DB_NAME), 1);
        bin_data_size++;
        
        /* Get the database name size. */
        val_size = 0;
        while(db_name[val_size] != '\0') val_size++;

        /* Assign the database name. */
        modb_db_binary = reallocate_UC_ptr(modb_db_binary, (bin_data_size + val_size) + 1);
        memcpy(&modb_db_binary[bin_data_size], db_name, val_size);
        memset(&modb_db_binary[bin_data_size + val_size + 1], 0, 1);
        bin_data_size += val_size + 1;

        modb_db_binary = reallocate_UC_ptr(modb_db_binary, bin_data_size + 1);
        memset(&modb_db_binary[bin_data_size], static_cast<unsigned char> (modb_sections::MODB_PATH), 1);
        bin_data_size++;

        modb_db_binary = reallocate_UC_ptr(modb_db_binary, bin_data_size + strlen(NCC_PTR folder));
        memcpy(&modb_db_binary[bin_data_size], folder, strlen(NCC_PTR folder));
        bin_data_size += strlen(NCC_PTR folder);

        modb_db_binary = reallocate_UC_ptr(modb_db_binary, bin_data_size + 1);
        memset(&modb_db_binary[bin_data_size], static_cast<unsigned char> (modb_sections::MODB_END), 1);
        bin_data_size++;

        //UC_ptr_check(modb_db_binary, bin_data_size);

        /* If there is existing database data, write that before writing new data.
         * This will only happen if the user passes `database_method::DB_NEW` to `Database` class
         * initializer.
         * */
        if(existing_db_info != nullptr)
            fwrite(existing_db_info, sizeof(unsigned char), existing_db_size, db_bin_file);

        fwrite(modb_db_binary, sizeof(unsigned char), bin_data_size, db_bin_file);

        /* Free out memory used for `modb_db_binary`. */
        memset(modb_db_binary, 0, bin_data_size);
        free(modb_db_binary);
        modb_db_binary = nullptr;

        db_committed = true;
    }

    /*
     * check_if_not_committed - check if the database was commited. If it was, do nothing else prompt to the user
     *                          that the database failed to get committed.
     *  returns: nothing
     *  on error: this function does not error
     * */
    void check_if_not_committed()
    {
        if(!db_committed) std::cout << "\nMODB Notice:\n\tYour program initiated a new database but did not committ it.\n\n\tIf you want the program to automatically committ the database for you\n\tpass `DB_CREATE_AND_AUTO_COMMITT` when initializing `Database`.\n" << std::endl;
    }

    /*
     * db_has_been_committed - check if the database has been committed.
     *  returns: if the database has been committed it will return true else false
     *  on error: this function does not error
     * */
    bool db_has_been_committed() { return db_committed; }

    ~CreateDB()
    {
        /* TODO: Do checks on the binary file to make sure it was written correctly. */
        if(db_bin_file)
            fclose(db_bin_file);
        
        if(db_name) free(db_name);
        if(db_host) free(db_host);
        if(db_ip_address) free(db_ip_address);
        if(existing_db_info) free(existing_db_info);
        if(folder) free(folder);

        db_name = nullptr;
        db_host = nullptr;
        db_ip_address = nullptr;
        existing_db_info = nullptr;
        folder = nullptr;
        db_bin_file = NULL;

        memset(db_port, 0, 5);
    }
};

#endif