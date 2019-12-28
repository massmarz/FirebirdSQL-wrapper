# fb-wrapper
C++ Library for Firebird SQL using the new object oriented APIs.

# Requirements
C++11

# Usage
```c++
#include <iostream>
#include "Attachment.h"
#include "Transaction.h"
#include "Statement.h"

#define SERVER ...
#define DATABASE ...

int main() {

    Attachment attachment;
    Transaction transaction;
    Statement statement;
    
    attachment.createDatabase(SERVER, DATABASE, "sysdba", "masterkey", "UTF8");
    attachment.connect();
    
    transaction.setAttachment(&attachment);
    statement.setTransaction(&transaction);
    
    statement.setSql("CREATE TABLE TEST (ID INTEGER PRIMARY KEY, DESC VARCHAR(10), VAL DOUBLE PRECISION)");
    statement.execute();
    transaction.commitRetain();
    

    statement.setSql("INSERT INTO TEST(ID, DESC, VAL) VALUES (1, 'AAA', 1.11)");
    statement.execute();
    statement.setSql("INSERT INTO TEST(ID, DESC, VAL) VALUES (2, 'BBB', 2.22)");
    statement.execute();
    statement.setSql("INSERT INTO TEST(ID, DESC, VAL) VALUES (3, 'CCC', 3.33)");
    statement.execute();
    statement.setSql("INSERT INTO TEST(ID, DESC, VAL) VALUES (4, 'DDD', 4.44)");
    statement.execute();
    statement.setSql("INSERT INTO TEST(ID, DESC, VAL) VALUES (5, 'EEE', 5.55)");
    statement.execute();
    transaction.commitRetain();

    statement.setSql("SELECT * FROM TEST WHERE ID >= :ID");
    statement.paramByName("ID").setInt(3);
    statement.open();
    while(statement.fetch()){
        std::cout << "ID: " << statement.fieldByName("ID").asInteger();
        std::cout << " DESC: " << statement.fieldByName("DESC").asString();
        std::cout << " VAL: " << statement.fieldByName("VAL").asDouble() << std::endl;
    }
    statement.close();
    
    std::cout << std::endl;
    
    statement.setSql("SELECT * FROM TEST WHERE ID <= ?");
    statement.parameter(0).setInt(2);
    statement.open();
    while(statement.fetch()){
        std::cout << "ID: " << statement.field(0).asInteger();
        std::cout << " DESC: " << statement.field(1).asString();
        std::cout << " VAL: " << statement.field(2).asDouble() << std::endl;
    }
    statement.close();
    
    attachment.disconnect();
    
    return 0;
}
```
