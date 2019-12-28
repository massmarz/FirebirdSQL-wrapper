/* 
 * File:   Transaction.cpp
 * Created on 27 luglio 2018
 */

#include "Transaction.h"

Transaction::Transaction() {
    status = new ThrowStatusWrapper(master->getStatus());
}

void Transaction::setAttachment(Attachment* attachemnt) {
    if (this->attachment) {
        attachemnt->dispatcher.removeCallBack(attachmentCallbackID);
    }

    release();

    this->attachment = attachemnt;

    attachmentCallbackID = attachemnt->dispatcher.addCallBack([this](DBStateEvents evt) {
        switch (evt) {
            case DBStateEvents::ATTACHMENT_DISCONNECT:
                release();
                break;
            case DBStateEvents::ATTACHMENT_DELETE:
                release();
                dispatcher.broadcast(DBStateEvents::TRANSACTION_DISCONNECT);
                this->attachment = nullptr;
                break;
            default:
                break;
        }
    });
}

Transaction::~Transaction() {
    dispatcher.broadcast(DBStateEvents::TRANSACTION_DELETE);
    if (tra_) {
        tra_->release();
    }

    if (status) {
        status->dispose();
        delete status;
    }
    if(attachment)
        attachment->dispatcher.removeCallBack(attachmentCallbackID);
}

void Transaction::release() {
    try {
        rollback();
    } catch (const FbException& e) {
        if (tra_) {
            tra_->release();
            tra_ = nullptr;
        }
        char buf[256];
        formatExceptionMessage(e, buf, 256);
        fprintf(stderr, "%s\n", buf);
    }
}

void Transaction::connect() {
    if (attachment) {
        if (!tra_) {
            attachment->connect();
            tra_ = attachment->att_->startTransaction(status, 0, nullptr);
        }
    } else
        throw std::logic_error("Transaction: set attachment before connect!");

}

bool Transaction::isConnected() {
    return tra_ != nullptr;
}

void Transaction::commit() {
    if (tra_) {
        tra_->commit(status);
        tra_ = nullptr;
        dispatcher.broadcast(DBStateEvents::TRANSACTION_DISCONNECT);
    }
}

void Transaction::commitRetain() {
    if (tra_)
        tra_->commitRetaining(status);
}

void Transaction::rollback() {
    if (tra_) {
        dispatcher.broadcast(DBStateEvents::TRANSACTION_DISCONNECT);
        tra_->rollback(status);
        tra_ = nullptr;
    }
}

void Transaction::rollbackRetaining() {
    if (tra_)
        tra_->rollbackRetaining(status);
}

