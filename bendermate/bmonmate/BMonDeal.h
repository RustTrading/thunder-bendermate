#ifndef BMONDEAL_H
#define BMONDEAL_H

#include "Data/BDeal.h"

namespace MonMate {

class BMonDeal
{
public:
    BMonDeal(const BDealPtr& deal)
    : m_deal(deal)
        {}

    std::int64_t size() const;
    double price() const;
    BDefs::OrderSide side() const;

private:
    BDealPtr m_deal;
};

}
/****************************************************************************/

#endif
