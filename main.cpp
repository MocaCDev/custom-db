#include <iostream>
//#define something
#include "db_backend/database.hpp"

int main(int args, char *argv[])
{
    Database db(database_method::DB_CREATE_AND_AUTO_COMMIT, UC_PTR "../MY_MODB/my_modb.modb");

    db.set_new_db_name((unsigned char *) "my_new_database");
    db.set_new_db_ip_addr((unsigned char *) "127.0.0.1");
    db.set_new_db_host((unsigned char *) "my_custom_host.net");
    db.set_new_db_port((unsigned char *) "8080");
    //db.commit_db();

    return 0;
}