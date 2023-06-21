#ifndef database
#define database
#include <stdlib.h>
#include <cstring>

#define database_error(err_msg, ...)            \
{                                               \
    fprintf(stderr, err_msg, ##__VA_ARGS__);    \
    exit(EXIT_FAILURE);                         \
}
#define database_assert(cond, err_msg, ...)     \
    if(!(cond))                                 \
        database_error(err_msg, ##__VA_ARGS__)

#define UC_PTR        (unsigned char *)       // (U)nsigned (C)har
#define UCC_PTR       (unsigned const char *) // (U)nsigned (C)onstant (C)har
#define NC_PTR        (char *)                // (N)ormal (C)har
#define NCC_PTR       (const char *)          // (N)orma (C)onstant (C)har
#define SC_PTR        (signed char *)         // (S)igned (C)har
#define SCC_PTR       (signed constant char *)// (S)igned (C)onstant (C)har

/*
 * reallocate_UC_ptr - reallocate memory for a Unsigned Char pointer.
 *  returns: unsigned char pointer with new memory
 *  on error: this function will error if there was a memory allocation error
 *
 *  Note: make sure `new_size` is always 1 more than the old size.
 * */
unsigned char *reallocate_UC_ptr(unsigned char *src, size_t new_size)
{
    unsigned char *ptr = src;

    /* Make sure `src` has memory (checked via `ptr`). 
     * If `src` does not have memory, go ahead and allocate `new_size`. 
     * */
    if(!(ptr))
    {
        ptr = UC_PTR calloc(new_size, sizeof(*ptr));
        return ptr;
    }

    ptr = UC_PTR realloc(ptr, new_size * sizeof(*ptr));

    /* Make sure `ptr` didn't come back `NULL`. */
    database_assert(ptr, "\nError reallocating memory for UC pointer.\n")
        
    src = ptr;
    return src;
}

#include "create_new_db.hpp"
#include "connect_db.hpp"

enum class database_method
{
    /* Create a new database (if one doesn't exist). */
    DB_CREATE,
    /* Create a new database and automatically committ when program terminates. */
    DB_CREATE_AND_AUTO_COMMIT,
    /* Connect to existing databse (if one exists). */
    DB_CONNECT,
    /* Delete existing database (if one exists) and create a new one. */
    DB_NEW
};


class Database
{
private:
    /* What are we doing? */
    database_method DB_method;

    /* For creating new database. */
    CreateDB *database_create = nullptr;

    /* For existing database. */
public:
    Database(enum database_method db_method, unsigned char *path)
    {
        DB_method = db_method;

        switch(DB_method)
        {
            case database_method::DB_CREATE: 
            case database_method::DB_CREATE_AND_AUTO_COMMIT: database_create = new CreateDB(path);break;
            case database_method::DB_NEW: {
                database_create = new CreateDB(path);
                database_create->modb_init_new_db_entry();
                break;
            }
            default: database_error("\nInvalid Method.\n")
        }
    }

    /* ----- CREATING DB FUNCTIONALITY. -------- */

    /*
     * set_new_db_name - assign the new database a name.
     *  returns: nothing
     *  on error: this function does not error directly
     * */
    void set_new_db_name(unsigned char *new_db_name) { database_create->new_name(new_db_name); }

    /*
     * set_new_db_ip_addr - assign new ip address to the database.
     *  returns: nothing
     *  on error: this function does not error directly
     * */
    void set_new_db_ip_addr(unsigned char *new_db_ip_addr) { database_create->new_ip_address(new_db_ip_addr); }

    /*
     * set_new_db_host - assign new host to the database.
     *  returns: nothing
     *  on error: this function does not error directly
     * */
    void set_new_db_host(unsigned char *new_db_host) { database_create->new_host(new_db_host); }

    /*
     * set_new_db_port - assign new port to the database.
     *  returns: nothing
     *  on error: this function does not error directly
     * */
    void set_new_db_port(unsigned char *new_db_port) { database_create->new_port(new_db_port); }

    /*
     * commit_db - committ the database to the `.modb` binary file.
     *  returns: nothing (TODO - should this return something?)
     *  on error: this function does not error directly
     * */
    void commit_db() { database_create->commit_new_database(); }

    /* ----- END DB CREATION FUNCTIONALITY -----*/

    ~Database()
    {
        /* If the user did not commit, let them know. */
        if(!(database_create->db_has_been_committed()) && (DB_method == database_method::DB_CREATE_AND_AUTO_COMMIT || DB_method == database_method::DB_NEW))
            database_create->commit_new_database();
        /*if(DB_method == database_method::DB_CREATE_AND_AUTO_COMMITT || !(database_create->db_has_been_committed()))
            database_create->committ_new_database();
        else
            database_create->check_if_not_committed();*/

        if(database_create != nullptr)
            delete database_create;
    }
};

#endif