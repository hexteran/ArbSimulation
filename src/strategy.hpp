#include "simulation.hpp"

namespace ArbSimulation
{
    class BasicStrategy: public Subscriber
    {
    public:
        void OnL1Update(L1UpdatePtr update)
        {

        };

        void OnOwnTrade(L1UpdatePtr)
        {
        };

        void OnNewMessage(MessagePtr message)
        {
            switch(message->Type)
            {
                case (MessageType::L1Update):
                {
                    OnL1Update(std::static_pointer_cast<MDUpdateMessage>(message)->Update);
                    break;
                }
                case (MessageType::OrderFilled):
                {
                    OnL1Update(std::static_pointer_cast<OrderFilledMessage>(message)->Order);
                    break;
                }
            }
        }
    }
}