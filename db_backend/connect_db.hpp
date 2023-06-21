#ifndef connect_db
#define connect_db

enum class SS_STATUS: unsigned char
{
    /* Waiting for client to "post" request to add something to the DB. */
    SS_WAITING      = 0xA,
    /* The server is listening for a client-side connection. */
    SS_LISTENING    = 0xB,
    /* The server is obtaining data sent by client. */
    SS_READING      = 0xC,
    /* The server is "publishing" the data to the database. */
    SS_PUBLISHING   = 0xD,
    /* The server is performing a "command". 
     * A command can specify to the server the client is sending data,
     * wants data, wants to close all connections etc.*/
    SS_PERFORMING_COMMAND = 0xE,
    /* The server has closed. */
    SS_CLOSED       = 0xF
};

#define CS_REQUEST_CREATE_NEW_DB_ENTRY          0xF0 // requires a new DB-entry ID that will represent the entry
#define CS_REQUEST_DELETE_DB_ENTRY              0xF1 // requires DB-entry ID that the client wants to delete
#define CS_REQUEST_CREATE_NEW_POD               0xF2 // requires DB-entry ID as well the type of data and the name for the Piece Of Data (POD)
#define CS_REQUEST_DELETE_POD                   0xF3 // requires DB-entry ID as well as the name for the Piece Of Data (POD)
#define CS_REQUEST_TO_STORE_IN                  0xF4 // requires DB-entry ID as well as the name for the POD and the value to be assigned
#define CS_CREATING_POD_WITH_TYPE_SBYTE         0xD1 // goes with `CS_REQUEST_CREATE_NEW_POD`, tells server-side to create a DB POD expecting a single byte (SBYTE)
#define CS_CREATING_POD_WITH_TYPE_BYTE_STREAM   0xD2 // goes with `CS_REQUEST_CREATE_NEW_POD`, tells server-side to create a DB POD expecting a stream of bytes
#define CS_CREATING_POD_WITH_TYPE_SWORD         0xD3 // goes with `CS_REQUEST_CREATE_NEW_POD`, tells server-side to create a DB POD expecting a single word (SWORD)
#define CS_CREATING_POD_WITH_TYPE_WORD_STREAM   0xD4 // goes with `CS_REQUEST_CREATE_NEW_POD`, tells server-side to create a DB POD expecting a stream of words
#define CS_CREATING_POD_WITH_TYPE_SDWORD        0xD5 // goes with `CS_REQUEST_CREATE_NEW_POD`, tells server-side to create a DB POD expecting a single dword (SDWORD)
#define CS_CREATING_POD_WITH_TYPE_DWORD_STREAM  0xD6 // goes with `CS_REQUEST_CREATE_NEW_POD`, tells server-side to create a DB POD expecting a stream of dwords
#define CS_STORING_SBYTE                        0xDA // goes with `CS_REQUEST_TO_STORE_IN`, tells server-side a single byte is being assigned (enables it to do accurate checks as well)
#define CS_STORING_BYTE_STREAM                  0xDB // goes with `CS_REQUEST_TO_STORE_IN`, tells server-side a stream of bytes is being assigned (enables it to do accurate checks as well)
#define CS_STORING_SWORD                        0xDC // goes with `CS_REQUEST_TO_STORE_IN`, tells server-side a single word is being assigned (enables it to do accurate checks as well)
#define CS_STORING_WORD_STREAM                  0xDD // goes with `CS_REQUEST_TO_STORE_IN`, tells server-side a stream of word is being assigned (enables it to do accurate checks as well)
#define CS_STORING_SDWORD                       0xDE // goes with `CS_REQUEST_TO_STORE_IN`, tells server-side a single dword is being assigned (enables it to do accurate checks as well)
#define CS_STORING_DWORD_STREAM                 0xDF // goes with `CS_REQUEST_TO_STORE_IN`, tells server-side a stream of dwords is being assigned (enables it to do accurate checks as well)


#define server_status_name      UC_PTR "/server_status"
#define client_status_name      UC_PTR "/client_status"

typedef struct DatabaseServerSide
{
    /* DB IP Address. */
    unsigned char       *SS_DB_IP_ADDR = nullptr;
    /* Reallocation for IP addr pointer. */
    void DB_SS_UCPTR_REALLOC_IP_ADDR(size_t new_size) { SS_DB_IP_ADDR = reallocate_UC_ptr(SS_DB_IP_ADDR, new_size); }

    /* DB Host. */
    unsigned char       *SS_DB_HOST = nullptr;
    void DB_SS_UCPTR_REALLOC_HOST(size_t new_size) { SS_DB_HOST = reallocate_UC_ptr(SS_DB_HOST, new_size); }

    /* DB Port. */
    unsigned char       SS_DB_PORT[5] = {0, 0, 0, 0, '\0'};

    /* DB Name. */
    unsigned char       *SS_DB_NAME = nullptr;
    void DB_SS_UCPTR_REALLOC_DB_NAME(size_t new_size) { SS_DB_NAME = reallocate_UC_ptr(SS_DB_NAME, new_size); }

    /* Has a client connected to the SS? */
    bool                CS_HAS_CONNECTED = false;

    /* Is the server listening? */
    bool                SS_IS_LISTENING = false;

    /* Server-side "server_status" file. */
    unsigned char       *server_status_path = nullptr;
    FILE                *server_status = NULL;
    unsigned char       status = static_cast<unsigned char> (SS_STATUS::SS_WAITING);
    
    /* Client-side "client_status" file. */
    unsigned char       *client_status_path = nullptr;
    FILE                *client_status = NULL;

    /* MODB folder for user. */
    unsigned char       *SS_MODB_FOLDER = nullptr;
    void DB_SS_UCPTR_REALLOC_MODB_FOLDER(size_t new_size) { SS_MODB_FOLDER = reallocate_UC_ptr(SS_MODB_FOLDER, new_size); }

    DatabaseServerSide()
    {
        SS_DB_IP_ADDR = UC_PTR calloc(1, sizeof(*SS_DB_IP_ADDR));
        SS_DB_HOST = UC_PTR calloc(1, sizeof(*SS_DB_HOST));
        SS_DB_NAME = UC_PTR calloc(1, sizeof(*SS_DB_NAME));
        SS_MODB_FOLDER = UC_PTR calloc(1, sizeof(*SS_MODB_FOLDER));
        server_status_path = UC_PTR calloc(1, sizeof(*server_status_path));
        client_status_path = UC_PTR calloc(1, sizeof(*client_status_path));

        database_assert(SS_DB_IP_ADDR && SS_DB_HOST &&
                        SS_DB_NAME && SS_MODB_FOLDER &&
                        server_status_path && client_status_path,
            "\nError allocating initial memory for a, or multiple, variables residing in `_DatabaseServerSide`.\n")
    }

    bool wait_for_client_connection()
    {
        client_status = fopen(NCC_PTR client_status_path, "rb");

        while(!client_status)
            client_status = fopen(NCC_PTR client_status_path, "rb");
        
        fclose(client_status);
        client_status = fopen(NCC_PTR client_status_path, "rb");
        if(!client_status) return false;

        fseek(client_status, 0, SEEK_END);
        if(!(ftell(client_status) > 0)) { fclose(client_status); return false; }

        return true;
    }

    /*
     * listen - start listening on the server side; if `cont_run` is true, `listen` will handle everything for the programmer.
     *  returns: nothing
     *  on error: this function does not error directly
     * */
    void start(bool cont_run)
    {
        SS_IS_LISTENING = true;

        /* Prompt to the user that the SS is listening at IP using port. 
         * EX: "Server is listening on 127.0.0.1 using port 8080.""
         * */
        std::cout << "\n\nServer is listening on " << SS_DB_IP_ADDR << " using port " << SS_DB_PORT << "\n" << std::endl;

        /* The file `server_status` will be read by the client-side.
         * The file `client_status` will be read by the server-side.
         * */
        server_status_path = reallocate_UC_ptr(server_status_path, strlen(NCC_PTR SS_MODB_FOLDER));
        client_status_path = reallocate_UC_ptr(client_status_path, strlen(NCC_PTR SS_MODB_FOLDER));

        memcpy(server_status_path, SS_MODB_FOLDER, strlen(NCC_PTR SS_MODB_FOLDER));
        memcpy(client_status_path, SS_MODB_FOLDER, strlen(NCC_PTR SS_MODB_FOLDER));

        server_status_path = reallocate_UC_ptr(server_status_path, strlen(NCC_PTR SS_MODB_FOLDER) + 14);
        client_status_path = reallocate_UC_ptr(client_status_path, strlen(NCC_PTR SS_MODB_FOLDER) + 14);

        memcpy(&server_status_path[strlen(NCC_PTR SS_MODB_FOLDER)], server_status_name, 14);
        memset(&server_status_path[strlen(NCC_PTR SS_MODB_FOLDER) + 14], 0, 1);
        memcpy(&client_status_path[strlen(NCC_PTR SS_MODB_FOLDER)], client_status_name, 14);
        memset(&client_status_path[strlen(NCC_PTR SS_MODB_FOLDER) + 14], 0, 1);

        server_status = fopen(NCC_PTR server_status_path, "wb");
        database_assert(server_status, "\nError opening up %s for server-side.\n", server_status_path)
        fwrite(&status, sizeof(unsigned char), 1, server_status);


        /* If the programmer wants to handle things themselves, just return. */
        if(!cont_run) return;

        if(wait_for_client_connection())
        {
            connection:
            std::cout << "Client connection!" << std::endl;
        }
        else
        {
            std::cout << "It appears there has been a connection, but the client sent no initial data." << std::endl;
            while(!wait_for_client_connection());
            goto connection;
        }
    }

    ~DatabaseServerSide()
    {
        if(SS_DB_IP_ADDR) free(SS_DB_IP_ADDR);
        if(SS_DB_HOST) free(SS_DB_HOST);
        if(SS_DB_NAME) free(SS_DB_NAME);
        if(SS_MODB_FOLDER) free(SS_MODB_FOLDER);
        if(server_status_path) free(server_status_path);
        if(client_status_path) free(client_status_path);

        SS_DB_IP_ADDR = SS_DB_HOST = 
            SS_DB_NAME = SS_MODB_FOLDER =
            server_status_path = client_status_path = nullptr;
        
        fclose(server_status);
        if(client_status) fclose(client_status);
    }
} _DatabaseServerSide;

typedef struct DatabaseClientSide
{

} _DatabaseClientSide;

/* Bytes to skip (depending on OS type). */
#ifdef _WIN32
#define bytes_to_skip       6
#endif

#ifdef _WIN64
#define bytes_to_skip       6
#endif

#if defined(__unix) || defined(__unix__) || defined(__linux__)
#define bytes_to_skip       5
#endif

#if defined(__APPLE__) || defined(__MACH__)
#define bytes_to_skip       4
#endif

#if (!defined(_WIN32) || !defined(_WIN64)) && (!defined(__unix) || !defined(__unix__) || !defined(__linux__)) && (!defined(__APPLE__) || !defined(__MACH__))
#define bytes_to_skip       12
#endif

class DatabaseConnect
{
private:
#ifdef SERVER_SIDE
    /* Server side data for MODB database. 
     * DB_SS - DataBase Server Side.
     * */
    _DatabaseServerSide *DB_SS = nullptr;
#endif

#ifdef CLIENT_SIDE
    /* Client side data for MODB database. 
     * DB_CS - DataBase Client Side.
     * */
    _DatabaseClientSide *DB_CS = nullptr;
#endif

    unsigned char *modb_path = nullptr;

public:
    DatabaseConnect(unsigned char *modb_binary_path)
    {
        FILE *modb_binary = fopen(NCC_PTR modb_binary_path, "rb");
        database_assert(modb_binary, "\nThe MODB binary file %s does not exist.\n", modb_binary_path)

        fseek(modb_binary, 0, SEEK_END);
        size_t modb_bin_size = ftell(modb_binary);
        fseek(modb_binary, 0, SEEK_SET);

        unsigned char *modb_bin_data = UC_PTR calloc(modb_bin_size, sizeof(*modb_bin_data));
        database_assert(modb_bin_data, "\nError allocating initial memory for storing MODB binary data.\n")
        fread(modb_bin_data, sizeof(unsigned char), modb_bin_size, modb_binary);

        fclose(modb_binary);

#if defined(SERVER_SIDE) && defined(CLIENT_SIDE)
        database_error("\nCannot have both SERVER_SIDE and CLIENT_SIDE defined in one program.\n")
#endif

#ifdef SERVER_SIDE
        DB_SS = new _DatabaseServerSide;
#endif

#ifdef CLIENT_SIDE
        DB_CS = new _DatabaseClientSide;
#endif

        size_t bin_index = bytes_to_skip;
        unsigned short ind = 0;
        while(modb_bin_data[bin_index] != static_cast<unsigned char> (modb_sections::MODB_END))
        {
            switch(modb_bin_data[bin_index])
            {
                case static_cast<unsigned char> (modb_sections::MODB_IP_ADDRESS): {
                    bin_index++;

                    while(modb_bin_data[bin_index] != 0)
                    {
#ifdef SERVER_SIDE
                        DB_SS->SS_DB_IP_ADDR[ind] = modb_bin_data[bin_index];
                        ind++;

                        DB_SS->DB_SS_UCPTR_REALLOC_IP_ADDR(ind + 1);
#endif
#ifdef CLIENT_SIDE
                        /* TODO: Add client-side. */
#endif
                        bin_index++;
                    }
                    memset(&DB_SS->SS_DB_IP_ADDR[ind], 0, 1);

                    std::cout << "\nmodb_sections::MODB_IP_ADDRESS:    " << DB_SS->SS_DB_IP_ADDR << std::endl;
                    ind = 0;
                    break;
                }
                case static_cast<unsigned char> (modb_sections::MODB_PORT_AND_HOST): {
                    bin_index++;

                    /* Host. */
                    while(modb_bin_data[bin_index] != 0)
                    {
#ifdef SERVER_SIDE
                        DB_SS->SS_DB_HOST[ind] = modb_bin_data[bin_index];
                        ind++;

                        DB_SS->DB_SS_UCPTR_REALLOC_HOST(ind + 1);
#endif
#ifdef CLIENT_SIDE
                        /* TODO: Add client-side. */
#endif
                        bin_index++;
                    }
                    memset(&DB_SS->SS_DB_HOST[ind], 0, 1);

                    bin_index++;
                    ind = 0;
                    while(modb_bin_data[bin_index] != 0)
                    {
#ifdef SERVER_SIDE
                        DB_SS->SS_DB_PORT[ind] = modb_bin_data[bin_index];
                        ind++;
#endif
#ifdef CLIENT_SIDE
                        /* TODO: Add client-side. */
#endif
                        bin_index++;
                    }

                    std::cout << "modb_sections::MODB_PORT_AND_HOST: " << DB_SS->SS_DB_PORT << ", " << DB_SS->SS_DB_HOST << std::endl;
                    ind = 0;
                    break;
                }
                case static_cast<unsigned char> (modb_sections::MODB_DB_NAME): {
                    bin_index++;

                    while(modb_bin_data[bin_index] != 0)
                    {
#ifdef SERVER_SIDE
                        DB_SS->SS_DB_NAME[ind] = modb_bin_data[bin_index];
                        ind++;

                        DB_SS->DB_SS_UCPTR_REALLOC_DB_NAME(ind + 1);
#endif
#ifdef CLIENT_SIDE
                        /* TODO: Add client-side. */
#endif

                        bin_index++;
                    }
                    memset(&DB_SS->SS_DB_NAME[ind], 0, 1);

                    std::cout << "modb_sections::MODB_DB_NAME:       " << DB_SS->SS_DB_NAME << std::endl;
                    ind = 0;
                    break;
                }
                case static_cast<unsigned char> (modb_sections::MODB_PATH): {
                    bin_index++;

                    while(modb_bin_data[bin_index] != static_cast<unsigned char> (modb_sections::MODB_END))
                    {
#ifdef SERVER_SIDE
                        DB_SS->SS_MODB_FOLDER[ind] = modb_bin_data[bin_index];
                        ind++;

                        DB_SS->DB_SS_UCPTR_REALLOC_MODB_FOLDER(ind + 1);
#endif
#ifdef CLIENT_SIDE
                        /* TODO: Add client-side. */
#endif
                        bin_index++;
                    }
                    memset(&DB_SS->SS_MODB_FOLDER[ind], 0, 1);

                    bin_index--;
                    std::cout << "modb_sections::MODB_PATH:\t   " << DB_SS->SS_MODB_FOLDER << std::endl;
                    ind = 0;
                    break;
                }
                default: break;
            }
            bin_index++;
        }

        std::cout << std::endl;
        free(modb_bin_data);
        modb_bin_data = nullptr;
    }

#ifdef SERVER_SIDE

    /*
     * SS_listen - start listening at the IP address using the port.
     *  cont_run - continuous run; does the user want the server-side struct to automatically handle everything?
     *  returns: nothing
     *  on error: this function does not error directly.
     * */
    void SS_start(bool cont_run)
    {
        DB_SS->start(cont_run);
    }

#endif

    ~DatabaseConnect()
    {
#ifdef SERVER_SIDE
        delete DB_SS;
        DB_SS = nullptr;
#endif

#ifdef CLIENT_SIDE
        delete DB_CS;
        DB_CS = nullptr;
#endif
    }
};

#endif