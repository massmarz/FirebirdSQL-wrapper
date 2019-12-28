/* 
 * File:   Attachment.h
 * Created on 27 luglio 2018
 */

#ifndef ATTACHMENT_H
#define ATTACHMENT_H


#include <vector>
#include "EventDispatcher.h"
#include "fb-wrapper.h"

class Transaction;
class Statement;

class Attachment {
    friend class Transaction;
    friend class Statement;
public:
    Attachment();
    void setParameter(std::string server, std::string database, std::string username, std::string password, std::string charset);
    void createDatabase(std::string server, std::string database, std::string username, std::string password, std::string charset);
    virtual ~Attachment();
    void connect();
    void disconnect();
    void startTransaction();
    IAttachment* getHandle();
    EventDispatcher<DBStateEvents> dispatcher; 
private:
    IAttachment* att_ = nullptr;
    ThrowStatusWrapper* status = nullptr;
    IXpbBuilder* dpb = nullptr;

    std::string server;
    std::string database;
    std::string username;
    std::string password;
    std::string charset;
    std::string connectionString;
    
};

extern void formatExceptionMessage(const FbException& error, char *msg, unsigned int len);

#endif /* ATTACHMENT_H */

