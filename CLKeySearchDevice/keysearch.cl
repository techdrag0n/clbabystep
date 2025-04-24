#define COMPRESSED 0
#define UNCOMPRESSED 1
#define BOTH 2

unsigned int endian(unsigned int x)
{
	return (x << 24) | ((x << 8) & 0x00ff0000) | ((x >> 8) & 0x0000ff00) | (x >> 24);
}

typedef struct {
	int idx;
	bool compressed;
	unsigned int x[8];
	unsigned int y[8];
	unsigned int digest[8];
}CLDeviceResult;

__kernel void multiplyStepKernel(
	int totalPoints, int step, __global uint256_t* privateKeys, __global uint256_t* chain,
	__global uint256_t* gxPtr,
	__global uint256_t* gyPtr,
	__global uint256_t* xPtr,
	__global uint256_t* yPtr)
{
	uint256_t gx;
	uint256_t gy;
	int gid = get_local_size(0) * get_group_id(0) + get_local_id(0);
	int dim = get_global_size(0);

	gx = gxPtr[step];
	gy = gyPtr[step];

	// Multiply together all (_Gx - x) and then invert
	uint256_t inverse = { {0,0,0,0,0,0,0,1} };

	int batchIdx = 0;
	int i = gid;
	for (; i < totalPoints; i += dim) {

		unsigned int p;
		p = readWord256k(privateKeys, i, 7 - step / 32);

		unsigned int bit = p & (1 << (step % 32));

		uint256_t x = xPtr[i];

		if (bit != 0) {
			if (!isInfinity256k(x)) {
				beginBatchAddWithDouble256k(gx, gy, xPtr, chain, i, batchIdx, &inverse);
				batchIdx++;
			}
		}
	}

	//doBatchInverse(inverse);
  //  inverse = doBatchInverse256k(inverse);
	inverse = invModP256k(inverse);

	i -= dim;
	for (; i >= 0; i -= dim) {
		uint256_t newX;
		uint256_t newY;

		unsigned int p;
		p = readWord256k(privateKeys, i, 7 - step / 32);
		unsigned int bit = p & (1 << (step % 32));

		uint256_t x = xPtr[i];
		bool infinity = isInfinity256k(x);

		if (bit != 0) {
			if (!infinity) {
				batchIdx--;
				completeBatchAddWithDouble256k(gx, gy, xPtr, yPtr, i, batchIdx, chain, &inverse, &newX, &newY);
			} else {
				newX = gx;
				newY = gy;
			}

			xPtr[i] = newX;
			yPtr[i] = newY;
		}
	}
}

void atomicListAdd(__global CLDeviceResult* results, __global unsigned int* numResults, CLDeviceResult* r)
{
	unsigned int count = atomic_add(numResults, 1);

	results[count] = *r;
}

void setResultFound(int idx, bool compressed, uint256_t x, uint256_t y, unsigned int digest[8], __global CLDeviceResult* results, __global unsigned int* numResults)
{
	CLDeviceResult r;
//	r.digest = digest;
	r.idx = idx;
	r.compressed = compressed;
	//   r.x = x.v;
	for (int i = 0; i < 8; i++) {
		r.x[i] = x.v[i];
		r.y[i] = y.v[i];
	}

	//   doRMD160FinalRound(digest, r.digest);

	atomicListAdd(results, numResults, &r);
}

void doIteration(
	size_t totalPoints,
	int compression,
	__global uint256_t* chain,
	__global uint256_t* xPtr,
	__global uint256_t* yPtr,
	__global uint256_t* incXPtr,
	__global uint256_t* incYPtr,
	__global unsigned int* targetList,
	size_t numTargets,
	ulong mask,
	__global CLDeviceResult* results,
	__global unsigned int* numResults)
{
	int gid = get_local_size(0) * get_group_id(0) + get_local_id(0);
	int dim = get_global_size(0);

	uint256_t incX = *incXPtr;
	uint256_t incY = *incYPtr;

	// Multiply together all (_Gx - x) and then invert
	uint256_t inverse = { {0,0,0,0,0,0,0,1} };
	int i = gid;

	int batchIdx = 0;

	for (; i < totalPoints; i += dim) {
		uint256_t x;
		uint256_t y;

		unsigned int digest[8];

//		digest[0] = 1;

		x = xPtr[i];
		y = yPtr[i];

		//           hashPublicKeyCompressed(y, readLSW256k(yPtr, i), digest);

		if (y.v[7] % 0x1000000 == 0 || y.v[7] % 0x1000000 == 0xFFFC2F)
		{
			uint256_t y = yPtr[i];
			setResultFound(i, true, x, y, digest, results, numResults);
		}

		beginBatchAdd256k(incX, x, chain, i, batchIdx, &inverse);
		batchIdx++;
	}

	//   inverse = doBatchInverse256k(inverse);
	inverse = invModP256k(inverse);

	i -= dim;

	for (; i >= 0; i -= dim) {

		uint256_t newX;
		uint256_t newY;
		batchIdx--;
		completeBatchAdd256k(incX, incY, xPtr, yPtr, i, batchIdx, chain, &inverse, &newX, &newY);

		xPtr[i] = newX;
		yPtr[i] = newY;
	}
}

void doIterationWithDouble(
	size_t totalPoints,
	int compression,
	__global uint256_t* chain,
	__global uint256_t* xPtr,
	__global uint256_t* yPtr,
	__global uint256_t* incXPtr,
	__global uint256_t* incYPtr,
	__global unsigned int* targetList,
	size_t numTargets,
	ulong mask,
	__global CLDeviceResult* results,
	__global unsigned int* numResults)
{
	int gid = get_local_size(0) * get_group_id(0) + get_local_id(0);
	int dim = get_global_size(0);

	uint256_t incX = *incXPtr;
	uint256_t incY = *incYPtr;

	// Multiply together all (_Gx - x) and then invert
	uint256_t inverse = { {0,0,0,0,0,0,0,1} };

	int i = gid;
	int batchIdx = 0;
	for (; i < totalPoints; i += dim) {
		uint256_t x;
		uint256_t y;

		unsigned int digest[8];
		digest[1] = 2;
		digest[2] = 2;
		digest[3] = 2;
		digest[4] = 2;
		digest[0] = 2;
		x = xPtr[i];
		y = yPtr[i];

		//         hashPublicKeyCompressed(x, readLSW256k(yPtr, i), digest);
		/*
		if (y.v[7] % 0x1000000 == 0 || y.v[7] % 0x1000000 == 0xFFFC2F) {
			uint256_t y = yPtr[i];
			setResultFound(i, true, x, y, digest, results, numResults);
		}
*/
		beginBatchAddWithDouble256k(incX, incY, xPtr, chain, i, batchIdx, &inverse);
		batchIdx++;
	}

	//  inverse = doBatchInverse256k(inverse);
	inverse = invModP256k(inverse);

	i -= dim;

	for (; i >= 0; i -= dim) {
		uint256_t newX;
		uint256_t newY;
		batchIdx--;
		completeBatchAddWithDouble256k(incX, incY, xPtr, yPtr, i, batchIdx, chain, &inverse, &newX, &newY);
		xPtr[i] = newX;
		yPtr[i] = newY;
	}
}

/* * Performs a single iteration */
__kernel void keyFinderKernel(
	unsigned int totalPoints,
	int compression,
	__global uint256_t* chain,
	__global uint256_t* xPtr,
	__global uint256_t* yPtr,
	__global uint256_t* incXPtr,
	__global uint256_t* incYPtr,
	__global unsigned int* targetList,
	ulong numTargets, ulong mask,
	__global CLDeviceResult* results,
	__global unsigned int* numResults)
{
	doIteration(totalPoints, compression, chain, xPtr, yPtr, incXPtr, incYPtr, targetList, numTargets, mask, results, numResults);
}

__kernel void keyFinderKernelWithDouble(
	unsigned int totalPoints,
	int compression,
	__global uint256_t* chain,
	__global uint256_t* xPtr,
	__global uint256_t* yPtr,
	__global uint256_t* incXPtr,
	__global uint256_t* incYPtr,
	__global unsigned int* targetList,
	ulong numTargets, ulong mask,
	__global CLDeviceResult* results,
	__global unsigned int* numResults)
{
	doIterationWithDouble(totalPoints, compression, chain, xPtr, yPtr, incXPtr, incYPtr, targetList, numTargets, mask, results, numResults);
}
