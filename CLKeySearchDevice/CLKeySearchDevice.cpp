#include <cmath>
#include "Logger.h"
#include "util.h"
#include "CLKeySearchDevice.h"
#include <KeyFinder.h>
//using namespace KeyFinder;
//using namespace KeyFinder;

// Defined in bitcrack_cl.cpp which gets build in the pre-build event
extern char _bitcrack_cl[];

typedef struct {
    int idx;
    bool compressed;
    unsigned int x[8];
    unsigned int y[8];
    unsigned int digest[8];
}CLDeviceResult;


CLKeySearchDevice::CLKeySearchDevice(uint64_t device, int threads, int pointsPerThread, int blocks)
{
    _threads = threads;
    _blocks = blocks;
    _points = pointsPerThread * threads * blocks;
    _device = (cl_device_id)device;


    if(threads <= 0 || threads % 32 != 0) {
        throw KeySearchException("The number of threads must be a multiple of 32");
    }

    if(pointsPerThread <= 0) {
        throw KeySearchException("At least 1 point per thread required");
    }

    try {
        // Create the context
        _clContext = new cl::CLContext(_device);
        Logger::log(LogLevel::Info, "Compiling OpenCL kernels 1...");
        _clProgram = new cl::CLProgram(*_clContext, _bitcrack_cl);
        Logger::log(LogLevel::Info, "Compiling OpenCL kernels 2...");
        // Load the kernels
        _initKeysKernel = new cl::CLKernel(*_clProgram, "multiplyStepKernel");
        Logger::log(LogLevel::Info, "Compiling OpenCL kernels 3...");
        _initKeysKernel->getWorkGroupSize();

        _stepKernel = new cl::CLKernel(*_clProgram, "keyFinderKernel");
        _stepKernelWithDouble = new cl::CLKernel(*_clProgram, "keyFinderKernelWithDouble");

        _globalMemSize = _clContext->getGlobalMemorySize();

        _deviceName = _clContext->getDeviceName();
    } catch(cl::CLException ex) {
        throw KeySearchException(ex.msg);
    }

    _iterations = 0;
}

CLKeySearchDevice::~CLKeySearchDevice()
{
    _clContext->free(_x);
    _clContext->free(_y);
    _clContext->free(_xTable);
    _clContext->free(_yTable);
    _clContext->free(_xInc);
    _clContext->free(_yInc);
    _clContext->free(_deviceResults);
    _clContext->free(_deviceResultsCount);

    delete _stepKernel;
    delete _stepKernelWithDouble;
    delete _initKeysKernel;
    delete _clContext;
}

void CLKeySearchDevice::allocateBuffers()
{
    size_t numKeys = (size_t)_points;
    size_t size = numKeys * 8 * sizeof(unsigned int);

    // X values
    _x = _clContext->malloc(size);
    _clContext->memset(_x, -1, size);

    // Y values
    _y = _clContext->malloc(size);
    _clContext->memset(_y, -1, size);

    // Multiplicaiton chain for batch inverse
    _chain = _clContext->malloc(size);

    // Private keys for initialization
    _privateKeys = _clContext->malloc(size, CL_MEM_READ_ONLY);

    // Lookup table for initialization
    _xTable = _clContext->malloc(256 * 8 * sizeof(unsigned int), CL_MEM_READ_ONLY);
    _yTable = _clContext->malloc(256 * 8 * sizeof(unsigned int), CL_MEM_READ_ONLY);

    // Value to increment by
    _xInc = _clContext->malloc(8 * sizeof(unsigned int), CL_MEM_READ_ONLY);
    _yInc = _clContext->malloc(8 * sizeof(unsigned int), CL_MEM_READ_ONLY);

    // Buffer for storing results
    _deviceResults = _clContext->malloc(128 * sizeof(CLDeviceResult));
    _deviceResultsCount = _clContext->malloc(sizeof(unsigned int));
}

void CLKeySearchDevice::setIncrementor(secp256k1::ecpoint &p)
{
    unsigned int buf[8];

    p.x.exportWords(buf, 8, secp256k1::uint256::BigEndian);
    _clContext->copyHostToDevice(buf, _xInc, 8 * sizeof(unsigned int));

    p.y.exportWords(buf, 8, secp256k1::uint256::BigEndian);
    _clContext->copyHostToDevice(buf, _yInc, 8 * sizeof(unsigned int));
}

void CLKeySearchDevice::init(const secp256k1::uint256& start, int compression, const secp256k1::uint256& stride)
//void CLKeySearchDevice::init(const secp256k1::uint256& start, int compression, const secp256k1::uint256& stride, secp256k1::ecpoint mypoint)
{
    if(start.cmp(secp256k1::N) >= 0) {
        throw KeySearchException("Starting key is out of range");
    }

    _start = start;
    _stride = stride;

    _compression = compression;
  //  secp256k1::ecpoint _mypoint = mypoint;
//    const char* hex_str = "04ffffca9232429f94061c29550445d29bbe032f8677a6ab17ea8b518e01342d089101936fdda8fe0ea03d5513f64a88357c84a5fdd057e4d846f30cc88ac7d5f6"; // Replace with your actual hex string

//    secp256k1::ecpoint mypoint = secp256k1::parsePublicKey(hex_str);

    try {
        allocateBuffers();

        generateStartingPoints();

        // Set the incrementor
        secp256k1::ecpoint g = secp256k1::G();
 //       secp256k1::ecpoint p = secp256k1::multiplyPoint(secp256k1::uint256((uint64_t)_points ) * _stride, g);
        secp256k1::ecpoint p = secp256k1::multiplyPoint(secp256k1::uint256((uint64_t)_points), g);
  /*     if (_compression < 2) {
            p = secp256k1::addPoints(p, mypoint);
        }*/ 
 //        secp256k1::ecpoint p = p1;
        setIncrementor(p);
    } catch(cl::CLException ex) {
        throw KeySearchException(ex.msg);
    }
}

void CLKeySearchDevice::doStep()
{
    try {
        uint64_t numKeys = (uint64_t)_points;

        if(_iterations < 2 && _start.cmp(numKeys) <= 0) {
            _stepKernelWithDouble->set_args(
                _points,
                _compression,
                _chain,
                _x,
                _y,
                _xInc,
                _yInc,
                _deviceTargetList.ptr,
                _deviceTargetList.size,
                _deviceTargetList.mask,
                _deviceResults,
                _deviceResultsCount);
            _stepKernelWithDouble->call(_blocks, _threads);
        } else {

            _stepKernel->set_args(
                _points,
                _compression,
                _chain,
                _x,
                _y,
                _xInc,
                _yInc,
                _deviceTargetList.ptr,
                _deviceTargetList.size,
                _deviceTargetList.mask,
                _deviceResults,
                _deviceResultsCount);
            _stepKernel->call(_blocks, _threads);
        }
        fflush(stdout);

        getResultsInternal();

        _iterations++;
    } catch(cl::CLException ex) {
        throw KeySearchException(ex.msg);
    }
}

void CLKeySearchDevice::setTargetsList()
{
    size_t count = _targetList.size();

    _targets = _clContext->malloc(5 * sizeof(unsigned int) * count);

    for(size_t i = 0; i < count; i++) {
        unsigned int h[5];

        _clContext->copyHostToDevice(h, _targets, i * 5 * sizeof(unsigned int), 5 * sizeof(unsigned int));
    }

    _targetMemSize = count * 5 * sizeof(unsigned int);
    _deviceTargetList.ptr = _targets;
    _deviceTargetList.size = count;
}

void CLKeySearchDevice::setTargetsInternal()
{
    // Clean up existing list
    if(_deviceTargetList.ptr != NULL) {
        _clContext->free(_deviceTargetList.ptr);
    }

        setTargetsList();
}

void CLKeySearchDevice::setTargets(const std::set<KeySearchTarget> &targets)
{
    try {
        _targetList.clear();

        for(std::set<KeySearchTarget>::iterator i = targets.begin(); i != targets.end(); ++i) {
            hash160 h(i->value);
            _targetList.push_back(h);
        }

        setTargetsInternal();
    } catch(cl::CLException ex) {
        throw KeySearchException(ex.msg);
    }
}

size_t CLKeySearchDevice::getResults(std::vector<KeySearchResult> &results)
{
    size_t count = _results.size();
    for(size_t i = 0; i < count; i++) {
        results.push_back(_results[i]);
    }
    _results.clear();

    return count;
}

uint64_t CLKeySearchDevice::keysPerStep()
{
    return (uint64_t)_points;
}

std::string CLKeySearchDevice::getDeviceName()
{
    return _deviceName;
}

void CLKeySearchDevice::getMemoryInfo(uint64_t &freeMem, uint64_t &totalMem)
{
    freeMem = _globalMemSize - _targetMemSize - _pointsMemSize;
    totalMem = _globalMemSize;
}

void CLKeySearchDevice::splatBigInt(unsigned int *ptr, int idx, secp256k1::uint256 &k)
{
    unsigned int buf[8];

    k.exportWords(buf, 8, secp256k1::uint256::BigEndian);

    memcpy(ptr + idx * 8, buf, sizeof(unsigned int) * 8);

}

void CLKeySearchDevice::getResultsInternal()
{
    unsigned int numResults = 0;

    _clContext->copyDeviceToHost(_deviceResultsCount, &numResults, sizeof(unsigned int));

    if(numResults > 0) {
        CLDeviceResult *ptr = new CLDeviceResult[numResults];

        _clContext->copyDeviceToHost(_deviceResults, ptr, sizeof(CLDeviceResult) * numResults);

        unsigned int actualCount = 0;

        for(unsigned int i = 0; i < numResults; i++) {

            actualCount++;

            KeySearchResult minerResult;

            // Calculate the private key based on the number of iterations and the current thread
            secp256k1::uint256 offset = secp256k1::uint256((uint64_t)_points * _iterations) + secp256k1::uint256(ptr[i].idx) * _stride;
            secp256k1::uint256 privateKey = secp256k1::addModN(_start, offset);

            minerResult.privateKey = privateKey;
            minerResult.compressed = ptr[i].compressed;

            memcpy(minerResult.hash, ptr[i].digest, 20);

        

            minerResult.publicKey = secp256k1::ecpoint(secp256k1::uint256(ptr[i].x, secp256k1::uint256::BigEndian), secp256k1::uint256(ptr[i].y, secp256k1::uint256::BigEndian));

//            removeTargetFromList(ptr[i].digest);

            _results.push_back(minerResult);
        }

        // Reset device counter
        numResults = 0;
        _clContext->copyHostToDevice(&numResults, _deviceResultsCount, sizeof(unsigned int));
    }
}


secp256k1::uint256 CLKeySearchDevice::readBigInt(unsigned int *src, int idx)
{
    unsigned int value[8] = {0};

    for(int k = 0; k < 8; k++) {
        value[k] = src[idx * 8 + k];
    }

    secp256k1::uint256 v(value, secp256k1::uint256::BigEndian);

    return v;
}

void CLKeySearchDevice::initializeBasePoints()
{
    // generate a table of points G, 2G, 4G, 8G...(2^255)G
    std::vector<secp256k1::ecpoint> table;

    table.push_back(secp256k1::G());
    for(uint64_t i = 1; i < 256; i++) {

        secp256k1::ecpoint p = doublePoint(table[i - 1]);
        if(!pointExists(p)) {
            throw std::string("Point does not exist!");
        }
        table.push_back(p);
    }

    size_t count = 256;

    unsigned int *tmpX = new unsigned int[count * 8];
    unsigned int *tmpY = new unsigned int[count * 8];

    for(int i = 0; i < 256; i++) {
        unsigned int bufX[8];
        unsigned int bufY[8];
        table[i].x.exportWords(bufX, 8, secp256k1::uint256::BigEndian);
        table[i].y.exportWords(bufY, 8, secp256k1::uint256::BigEndian);

        for(int j = 0; j < 8; j++) {
            tmpX[i * 8 + j] = bufX[j];
            tmpY[i * 8 + j] = bufY[j];
        }
    }

    _clContext->copyHostToDevice(tmpX, _xTable, count * 8 * sizeof(unsigned int));

    _clContext->copyHostToDevice(tmpY, _yTable, count * 8 * sizeof(unsigned int));
}



void CLKeySearchDevice::generateStartingPoints()
{
    uint64_t totalPoints = (uint64_t)_points; //  total points from pointsPerThread * threads * blocks;
    uint64_t totalMemory = totalPoints * 40;

    std::vector<secp256k1::uint256> exponents;

    initializeBasePoints();

    _pointsMemSize = totalPoints * sizeof(unsigned int) * 16 + _points * sizeof(unsigned int) * 8;

    Logger::log(LogLevel::Info, "Generating " + util::formatThousands(totalPoints) + " starting points (" + util::format("%.1f", (double)totalMemory / (double)(1024 * 1024)) + "MB)");

    // Generate key pairs for k, k+1, k+2 ... k + <total points in parallel - 1>
    secp256k1::uint256 privKey = _start;

    exponents.push_back(privKey);

    for(uint64_t i = 1; i < totalPoints; i++) {
        privKey = privKey.add(_stride);  // creating starting points
        exponents.push_back(privKey);
    }

    unsigned int *privateKeys = new unsigned int[8 * totalPoints];

    for(int index = 0; index < _points; index++) {
        splatBigInt(privateKeys, index, exponents[index]);
    }

    // Copy to device
    _clContext->copyHostToDevice(privateKeys, _privateKeys, totalPoints * 8 * sizeof(unsigned int));

    delete[] privateKeys;

    // Show progress in 10% increments
    double pct = 10.0;
    for(int i = 0; i < 256; i++) {
        _initKeysKernel->set_args(_points, i, _privateKeys, _chain, _xTable, _yTable, _x, _y);
        _initKeysKernel->call(_blocks, _threads);

        if(((double)(i+1) / 256.0) * 100.0 >= pct) {
            Logger::log(LogLevel::Info, util::format("%.1f%%", pct));
            pct += 10.0;
        }
    }

    Logger::log(LogLevel::Info, "Done");
}


secp256k1::uint256 CLKeySearchDevice::getNextKey()
{
    uint64_t totalPoints = (uint64_t)_points * _threads * _blocks;

    return _start + secp256k1::uint256(totalPoints) * _iterations * _stride;
}