#include <iostream>
#define SERVER_SIDE
#include "db_backend/database.hpp"

int main(int args, char *argv[])
{
    DatabaseConnect db_con(UC_PTR "../MY_MODB/my_modb.modb");
    db_con.SS_start(true);

    return 0;
}