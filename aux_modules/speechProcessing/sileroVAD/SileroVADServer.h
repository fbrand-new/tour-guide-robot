#include <memory>

#include "SileroVADMsgs.h"
#include "Detector.h"

class SileroVADServer : public SileroVADMsgs
{
    std::shared_ptr<Detector> m_detector;
 
public:
    SileroVADServer(std::shared_ptr<Detector> audioCallback);
    void setThreshold(const double threshold);
    void setGapAllowance(int gapAllowance);
};