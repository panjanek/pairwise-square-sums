#include <stdio.h>
#include <time.h>
#include <pthread.h>
#include <stdint.h>

typedef uint64_t number_t;
int number_size;

struct batch_params {
    number_t start;
    number_t count;
};

static __device__ __forceinline__ number_t gpu_sqrt_round_double(number_t x)
{
	number_t round_sqrt = round(sqrt((double)x));
	return round_sqrt;
}

static __device__ __forceinline__ number_t gpu_sqrt_round_float(number_t x)
{
	number_t round_sqrt = roundf(sqrtf((float)x));
	return round_sqrt;
}

static __device__ __forceinline__ number_t gpu_sqrt_floor_double(number_t x)
{
	number_t floor_sqrt = floor(sqrt((double)x));
	return floor_sqrt;
}

static __device__ __forceinline__ number_t gpu_sqrt_floor_float(number_t x)
{
	number_t floor_sqrt = floorf(sqrtf((float)x));
	return floor_sqrt;
}

__device__ unsigned long long int umul_wide (unsigned int a, unsigned int b)
{
    unsigned long long int r;
    asm ("mul.wide.u32 %0,%1,%2;\n\t" : "=l"(r) : "r"(a), "r"(b));
    return r;
}

__device__ uint32_t isqrtll (uint64_t a)   //exact floor up to 10mld
{
    uint64_t rem, arg;
    uint32_t b, r, s, scal;

    arg = a;
    /* Normalize argument */
    scal = __clzll (a) & ~1;
    a = a << scal;
    b = a >> 32;
    /* Approximate rsqrt accurately. Make sure it's an underestimate! */
    float fb, fr;
    fb = (float)b;
    asm ("rsqrt.approx.ftz.f32 %0,%1; \n\t" : "=f"(fr) : "f"(fb));
    r = (uint32_t) fmaf (1.407374884e14f, fr, -438.0f);
    /* Compute sqrt(a) as a * rsqrt(a) */
    s = __umulhi (r, b);
    /* NR iteration combined with back multiply */
    s = s * 2;
    rem = a - umul_wide (s, s);
    r = __umulhi ((uint32_t)(rem >> 32) + 1, r);
    s = s + r;
    /* Denormalize result */
    s = s >> (scal >> 1);
    /* Make sure we get the floor correct; can be off by one to either side */
    rem = arg - umul_wide (s, s);
    if ((int64_t)rem < 0) s--;
    else if (rem >= ((uint64_t)s * 2 + 1)) s++;
    return (arg == 0) ? 0 : s;
}


static __device__ __forceinline__ number_t gpu_is_square_double(number_t x)
{
	number_t round_sqrt = gpu_sqrt_round_double(x);
	return (round_sqrt * round_sqrt == x)*2;
}

static __device__ __forceinline__ number_t gpu_is_square_float(number_t x)
{
	number_t round_sqrt = roundf(sqrtf((float)x));
	return (round_sqrt * round_sqrt == x)*2;
}

static __device__ __forceinline__ number_t gpu_is_square_isqrtll(number_t x)  //exact up to 10mld
{
	number_t floor_sqrt = isqrtll(x);
	return (floor_sqrt * floor_sqrt == x)*2;
}

static __device__ __forceinline__ number_t gpu_mulsq2_double(number_t x)
{
    return ceil((double)x*1.41421356237309504880168872);
}

static __device__ __forceinline__ number_t gpu_mulsq2_long(number_t x)   //round
{
    const unsigned long long K = 6074000999ULL; // sqrt(2) * 2^32
    const int SHIFT = 32;

    unsigned long long lo = (unsigned long long)x * K;
    unsigned long long hi = __umul64hi((unsigned long long)x, K);

    // Combine high and low for 128-bit value
    // result = (hi << 64 | lo) >> SHIFT with rounding
    unsigned long long carry = (lo >> (SHIFT - 1)) & 1ULL; // rounding bit
    unsigned long long res = (hi << (64 - SHIFT)) | (lo >> SHIFT);
    return (long long)(res + carry);
}

__device__ __forceinline__ unsigned long long ceil_x_sqrt2_u64_sat(unsigned long long x) {    //ceil
    if (!x) return 0ull;

    // Max x that keeps ceil(x*sqrt(2)) within 64 bits:
    const unsigned long long X_MAX_64 =
        13043817825332782211ull;

    if (x > X_MAX_64) return 0xffffffffffffffffull;  // saturate

    const unsigned long long C_lo = 0x6a09e667f3bcc908ull;

    // Fixed-point multiply and ceil divide by 2^64
    unsigned long long lo = x * C_lo;
    unsigned long long hi = __umul64hi(x, C_lo) + x; // +x from the implicit 1<<64 in C

    unsigned long long y  = hi + (lo != 0);

    // One-step exactness check; bump if needed
    unsigned long long y_hi = __umul64hi(y, y), y_lo = y * y;

    // two*x*x (low 128 bits)
    unsigned long long xx_hi = __umul64hi(x, x), xx_lo = x * x;
    unsigned long long two_hi = (xx_hi << 1) | (xx_lo >> 63);
    unsigned long long two_lo = (xx_lo << 1);

    // If y^2 < 2*x^2, increment (guaranteed at most once)
    if ( (y_hi < two_hi) || (y_hi == two_hi && y_lo < two_lo) )
        ++y;

    return y;
}


// ---------------------------------------------------------------------------------------------------------------

__device__ __forceinline__ number_t gpu_compare(number_t x, number_t x1, number_t x2)
{
	if (x1 != x2){
	    number_t delta = abs((long)((long)x1-(long)x2));
		if (delta > 1)
	        printf("[%llu] %lu != %lu (off by %lu)\n", x, x1, x2, delta);
		return 1+delta;
    }
	
	return 1;
}

__device__ __forceinline__ number_t gpu_check_validity(number_t x)
{
    number_t x1 = gpu_mulsq2_double(x);
	number_t x2 = ceil_x_sqrt2_u64_sat(x);
	
	return gpu_compare(x, x1, x2);
}

__device__ __forceinline__ number_t gpu_check_speed(number_t x)
{
    //20000, gpu_sqrt_floor_double : 32.0s
	//20000, gpu_sqrt_floor_float  :  6.3s
	//20000, isqrtll               :  2.7s
	
	//20000, gpu_mulsq2_double     :  0.246s
    number_t dummy = 0;
	
	for(number_t xx = 0; xx<20000; xx++)
	    dummy += ceil_x_sqrt2_u64_sat(xx);
	
	return 1+dummy%128;
}

__global__ void kernel(number_t* results, batch_params* p)
{
    number_t i = (number_t)(threadIdx.x + blockIdx.x * blockDim.x);
	if (i < p->count)
	{
	    number_t x = i + p->start;
	    number_t r = gpu_check_validity(x);     //gpu_check_speed of gpu_check_speed
	    results[i] = r;
    }
}

void gpu_start(number_t start, number_t end)
{
    int nblocks = 4096;
    int blocksize = 256;
	number_t count = nblocks * blocksize;

	printf("GPU start. From %lu to %lu in %lu batches\n", start, end, count);
    printf("preparing data for GPU\n");	
	
	//results array
	cudaError_t error;
    number_t* cpu_results = (number_t *)malloc(number_size * count);
    number_t* gpu_results;
	error = cudaMalloc((void**)&gpu_results,count * number_size);
	if (error != cudaSuccess) 
	    printf("cudaMalloc: %d - %s\n\n", error, cudaGetErrorName(error));
	
	//params
	struct batch_params *cpu_params = (struct batch_params *)malloc(sizeof(struct batch_params));
	batch_params* gpu_params;
    error = cudaMalloc((void**)&gpu_params,sizeof(struct batch_params));
	if (error != cudaSuccess) 
	    printf("cudaMalloc: %d - %s\n\n", error, cudaGetErrorName(error));
	
    clock_t global_start,global_end, batch_start, batch_end;
	global_start = clock();
	cpu_params->start = start;
	cpu_params->count = count;
	printf("entering main loop\n");
	fflush(stdout);
	number_t errors_count = 0;
	number_t errors_max = 0;
    while(cpu_params->start < end)
	{
	    batch_start = clock();
	    //copy results --> device
	    memset(cpu_results, 0, count * number_size);
	    error = cudaMemcpy(gpu_results, cpu_results, count * number_size, cudaMemcpyHostToDevice);
		if (error != cudaSuccess) 
	        printf("cudaMemcpy: %d - %s\n\n", error, cudaGetErrorName(error));
		
		//copy parameters --> device
        error = cudaMemcpy(gpu_params, cpu_params, sizeof(struct batch_params), cudaMemcpyHostToDevice);
		if (error != cudaSuccess)
	        printf("cudaMemcpy: %d - %s\n\n", error, cudaGetErrorName(error));
		
		//printf("starting kernels <%d/%d> start=%lu, count=%lu...", nblocks, blocksize, cpu_params->start, cpu_params->count);
		printf(".");
		fflush(stdout);
		kernel<<<nblocks, blocksize>>>(gpu_results, gpu_params);
		error = cudaPeekAtLastError();
		if (error != cudaSuccess)
		    printf("cudaPeekAtLastError: %d - %s\n\n", error, cudaGetErrorName(error));
		error = cudaDeviceSynchronize();
		if (error != cudaSuccess)
		    printf("cudaDeviceSynchronize: %d - %s\n\n", error, cudaGetErrorName(error));
		
		batch_end = clock();
		double batch_time_spent = (double)(batch_end - batch_start) / (double)CLOCKS_PER_SEC;
	    //printf("done in %.10f secs (total: %.10f s) \n", batch_time_spent, (double)(batch_end-global_start) / (double)CLOCKS_PER_SEC);
		fflush(stdout);
		
		error = cudaMemcpy(cpu_results, gpu_results, count * number_size, cudaMemcpyDeviceToHost);
		if (error != cudaSuccess)
		    printf("cudaMemcpy: %d - %s\n\n", error, cudaGetErrorName(error));
		
		for(number_t k=0; k<count; k++)
		{
		    if (cpu_results[k] == 0)
			    printf("ERROR - results[%lu] skipped!\n", k);
				
			if (cpu_results[k] > 1) {
                //printf("ERROR for %lu : %lu\n", k, cpu_results[k]);
				errors_count++;
				number_t delta = cpu_results[k]-1;
				if (delta > errors_max)
				    errors_max = delta;
			}
		}
	
		cpu_params->start = cpu_params->start + count;
	}
	
	global_end = clock();
	double global_time_spent = (double)(global_end - global_start) / (double)CLOCKS_PER_SEC;
	printf("\nErrors count: %lu,  max error:%lu\n", errors_count, errors_max);
	printf("Done. total time: %.10f secs\n\n", global_time_spent);
}

void gpu_tune()
{
    int N = 1 << 20; // example size (1M elements)

    int minGridSize = 0;   // suggested minimum grid size
    int blockSize = 0;     // threads per block

    // Query optimal block size
    cudaOccupancyMaxPotentialBlockSize(
        &minGridSize,       // out: minimum grid size needed
        &blockSize,         // out: block size (threads per block)
        kernel,             // kernel function
        0,                  // dynamic shared memory per block (bytes)
        0                   // block size limit (0 = no limit)
    );

    // Compute grid size to cover all elements
    int gridSize = (N + blockSize - 1) / blockSize;
	
	printf("CUDA tuning: blockSize=%d, minGridSize=%d, gridSize=%d\n", blockSize, minGridSize, gridSize);
}

int main(int argc, char **argv)
{
    // p2.xlarge
	// https://forums.developer.nvidia.com/t/integer-square-root/198642
	// nvcc -Xptxas -O3 perf-gpu.cu -o perf-gpu
	// nvcc -Xptxas -O3 --use_fast_math perf.cu -o perf
	// watch -n0.1 nvidia-smi
	// 1mld .. 1017170432 - tested
	
	// nvcc -arch=sm_89 -O3 -Xptxas -O3,-v,-dlcm=ca test-gpu.cu -o test-gpu
	
	number_size = sizeof(number_t);
	gpu_tune();
	number_t start = 1000000000;
	number_t end = 2000000000;
	
	printf("params: %d\n", argc);
	if (argc == 1)
	{
	    printf("Usage: test <start> <end>\n");
		printf("No parameters supplied, using defaults: start=%lu, end=%lu\n", start, end);
	}
	else if (argc == 2)
	{
	    printf("Invalid parameters.\n");
		return 1;
	}
	else if (argc == 3)
	{
	    start = atol(argv[1]);
		end = atol(argv[2]);
	}
	else
	{
	    printf("Invalid parameters.\n");
		return 1;
	}
	
	gpu_start(start, end);
	
    return 0;
}