#include "Reqs.h"


uint64_t ReqMan::AddReq(const CmdReqs* pReq)
{
    // Ignored value even if in write/mask mode
    ReqItemPtr itemPtr(new ReqItem(pReq));

}



