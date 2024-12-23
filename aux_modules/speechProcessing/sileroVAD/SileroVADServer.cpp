#include "SileroVADServer.h"

SileroVADServer::SileroVADServer(std::shared_ptr<Detector> audioCallback) : m_detector(audioCallback) {}

void SileroVADServer::setThreshold(const double threshold) {
    m_detector->m_vadThreshold = static_cast<float>(threshold);
}

void SileroVADServer::setGapAllowance(int gapAllowance) {
    m_detector->m_vadGapAllowance = gapAllowance;
}