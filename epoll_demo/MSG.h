#ifndef MSG_H_
#define MSG_H_


enum MSG_TYPE {
    MSG_REQ_NAME = 1,
    MSG_REQ_NUM,


    MSG_RES_NAME = 100,
    MSG_RES_NUM
};


struct Msg
{
    std::shared_ptr<char []> msg;
    int32_t msglen;
    MSG_TYPE msgtype;

    Msg() = default;   // Msg must have default ctr, because it will be save in continer!!!!!

    Msg(const std::shared_ptr<char []>& buf, int32_t len, MSG_TYPE type): 
        msg(buf), msglen(len), msgtype(type) {}
};

#endif
