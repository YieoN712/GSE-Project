#pragma once

#include <memory>
#include <string>

// Read-only loopback HTTP transport. No game objects are accessed by the worker thread.
class MonitorServer
{
public:
    MonitorServer();
    ~MonitorServer();
    MonitorServer(const MonitorServer&) = delete;
    MonitorServer& operator=(const MonitorServer&) = delete;
    bool Start();
    void Stop();
    void Publish(std::string snapshot);

private:
    struct Implementation;
    std::unique_ptr<Implementation> implementation;
};
