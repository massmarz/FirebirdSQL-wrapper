/* 
 * File:   fb-max.h
 * Created on 27 luglio 2018
 */

#ifndef FB_WRAPPER_H
#define FB_WRAPPER_H

#include <firebird/Interface.h>

enum class DBStateEvents { 
    ATTACHMENT_DELETE,
    ATTACHMENT_CONNECT,
    ATTACHMENT_DISCONNECT,
    TRANSACTION_DELETE,
    TRANSACTION_CONNECT,
    TRANSACTION_DISCONNECT
};

namespace Firebird {
    // forward declaration
    class IAttachment;
    class ITransaction;
    class IStatement;
    class IResultSet;
    class ThrowStatusWrapper;
    class IProvider;
    class IUtil;
    class IXpbBuilder;
    class FbException;
    class IMessageMetadata;
}

using namespace Firebird;

extern IMaster* master;

#endif /* FB_WRAPPER_H */