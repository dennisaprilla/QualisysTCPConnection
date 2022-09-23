#include <Synch.h>

bool synch::stop_= false;
std::mutex synch::stopMutex_;

std::condition_variable synch::startCondVar_;
std::mutex synch::startMutex_;
bool synch::start_ = false;