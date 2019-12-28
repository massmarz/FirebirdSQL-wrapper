/* 
 * File:   test.cpp
 */

#include <iostream>
#include "Attachment.h"
#include "Transaction.h"
#include "Statement.h"

/*
 * 
 */
int main() {
    Attachment attachment;
    Transaction transaction;
    Statement statement;
    
    attachment.createDatabase("10.10.10.80", "E:\\database\\fb-test.gdb", "sysdba", "masterkey", "UTF8");
    attachment.connect();
    
    transaction.setAttachment(&attachment);
    
    statement.setSql("CREATE TABLE TEST (ID INTEGER PRIMARY KEY, DESC VARCHAR(10))");
    statement.setTransaction(&transaction);
    statement.execute();
    transaction.commitRetain();
    

    statement.setSql("INSERT INTO TEST(ID, DESC) VALUES (1, 'AAA')");
    statement.execute();
    statement.setSql("INSERT INTO TEST(ID, DESC) VALUES (2, 'BBB')");
    statement.execute();
    statement.setSql("INSERT INTO TEST(ID, DESC) VALUES (3, 'CCC')");
    statement.execute();
    statement.setSql("INSERT INTO TEST(ID, DESC) VALUES (4, 'DDD')");
    statement.execute();
    statement.setSql("INSERT INTO TEST(ID, DESC) VALUES (5, 'EEE')");
    statement.execute();
    transaction.commitRetain();

    statement.setSql("SELECT * FROM TEST WHERE ID >= :ID");
    statement.paramByName("ID").setInt(3);
    statement.open();
    while(statement.fetch()){
        std::cout << "ID: " << statement.fieldByName("ID").asInteger();
        std::cout << " DESC: " << statement.fieldByName("DESC").asString() << std::endl;
    }
    statement.close();
    
    std::cout << std::endl;
    
    statement.setSql("SELECT * FROM TEST WHERE ID <= ?");
    statement.parameter(0).setInt(2);
    statement.open();
    while(statement.fetch()){
        std::cout << "ID: " << statement.field(0).asInteger();
        std::cout << " DESC: " << statement.field(1).asString() << std::endl;
    }
    statement.close();
    
    attachment.disconnect();
    
    return 0;
}

