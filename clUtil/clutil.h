#ifndef _CL_UTIL_H
#define _CL_UTIL_H

#define CL_HPP_TARGET_OPENCL_VERSION 120
#define CL_TARGET_OPENCL_VERSION 120
#include <CL/cl.h>
#include <string>
#include <vector>

namespace cl {
    std::string getErrorString(cl_int err);

    typedef struct {
        cl_device_id id;
        int cores;
        uint64_t mem;
        std::string name;

    }CLDeviceInfo;

    class CLException {
    public:
        int error;
        std::string msg;

        CLException(cl_int err)
        {
            this->error = err;
            this->msg = getErrorString(err);
        }

        CLException(cl_int err, std::string msg)
        {
            this->error = err;
            this->msg = msg;
        }
    };

    CLDeviceInfo getDeviceInfo(int device);

    std::vector<CLDeviceInfo> getDevices();

    int getDeviceCount();

    void clCall(cl_int err);

}

#endif