/**
 * @Author  Sagi Zeevi
 * @see     https://gist.github.com/sagi-z/f8b734a2240ce203ba5b2977e54b4414#file-dispatcher-hpp
 */

#ifndef EVENTDISPATCHER_H
#define EVENTDISPATCHER_H

#include <functional>
#include <list>

template <typename... Args>
class EventDispatcher {
public:
    typedef std::function<void(Args...) > CallBackFunction;

    class CBID {
    public:
        CBID() : valid(false) {
        }
    private:
        friend class EventDispatcher<Args...>;

        CBID(typename std::list<CallBackFunction>::iterator i)
        : iter(i), valid(true) {
        }

        typename std::list<CallBackFunction>::iterator iter;
        bool valid;
    };

    // register to be notified
    CBID addCallBack(CallBackFunction cb) {
        if (cb) {
            cbs.push_back(cb);
            return CBID(--cbs.end());
        }
        return CBID();
    }

    // unregister to be notified

    void removeCallBack(CBID &id) {
        if (id.valid) {
            cbs.erase(id.iter);
        }
    }

    void broadcast(Args... args) {
        for (auto &cb : cbs) {
            cb(args...);
        }
    }

private:
    std::list<CallBackFunction> cbs;
};



#endif /* EVENTDISPATCHER_H */

