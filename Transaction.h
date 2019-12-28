/* 
 * File:   Transaction.h
 * Created on 27 luglio 2018
 */

#ifndef TRANSACTION_H
#define TRANSACTION_H

#include "Attachment.h"

// forward declaration
class Statement;

class Transaction {
    friend class Statement;
public:
    Transaction();
    void setAttachment(Attachment* attachemnt);
    virtual ~Transaction();
    void commit();
    void commitRetain();
    void connect();
    void rollback(); 
    void rollbackRetaining();
    bool isConnected();
    
    EventDispatcher<DBStateEvents> dispatcher; 
private:
    void release();
    Attachment* attachment;
    ITransaction* tra_ = nullptr;
    ThrowStatusWrapper* status = nullptr;
    
    EventDispatcher<DBStateEvents>::CBID attachmentCallbackID;
};
#endif /* TRANSACTION_H */

